#ifndef __TTY_WRITER_H__
#define __TTY_WRITER_H__

#include <ostream>

// Write a number of lines over themselves in each session
class tty_writer {
  // ANSI escape code
#define CSI "\033["

  // Handling change in window size
  static volatile bool invalid_window_width;
  static int           window_width;
  // There is a small race condition here in the management of the
  // invalid_window_width global, which could lead to some visual
  // glitch. Not so important.
  static void sig_winch_handler(int s);
  static void prepare_display(); // Setup signal for SIGWINCH


  // Members
  std::ostream& m_os;
  size_t        m_nb_lines;     // Number of lines printed in the previous session
  bool          m_end_nl;       // Append a new line on termination

  class line;
  class session {
    tty_writer& m_tw;
    size_t      m_nb_lines;     // Number of lines printed in this session

  public:
    session(tty_writer& tw)
      : m_tw(tw)
      , m_nb_lines(0)
    {
      if(m_tw.m_nb_lines > 1)
        m_tw.m_os << CSI << (m_tw.m_nb_lines - 1) << 'A';
      m_tw.m_os << '\r';
    }

    ~session() {
      m_tw.m_os << CSI "0m" << std::flush;
      m_tw.m_nb_lines = m_nb_lines;
    }

    line start_line() {
      if(m_nb_lines > 0)
        m_tw.m_os << '\n';
      ++m_nb_lines;
      return line(m_tw.m_os);
    }
  };
  friend class session;

  class line {
    std::ostream& m_os;
  public:
    line(std::ostream& os)
      : m_os(os)
    { }
    ~line() {
      m_os << CSI "K" << CSI "0m";
    }

    template<typename T>
    line& operator<<(const T& x) {
      m_os << x;
      return *this;
    }
  };

public:
  tty_writer(std::ostream& os)
    : m_os(os)
    , m_nb_lines(0)
  {
    prepare_display();
  }
  ~tty_writer() {
    m_os << CSI "0m";
    if(m_end_nl)
      m_os << std::endl;
    else
      m_os << std::flush;
  }

  bool end_nl() const { return m_end_nl; }
  void end_nl(bool value) { m_end_nl = value; }
  session start_session() {
    m_end_nl = true;
    return session(*this);
  }

  int get_window_width();
#undef CSI
};

#endif /* __TTY_WRITER_H__ */
