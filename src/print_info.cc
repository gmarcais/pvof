#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <src/print_info.hpp>

static volatile bool invalid_window_width = true;
static int window_width = 0;

// There is a small race condition here in the management of the
// invalid_window_width global, which could lead to some visual
// glitch. Not so important.
void sig_winch_handler(int s) {
  invalid_window_width = true;
}

void prepare_display() {
#ifdef SIGWINCH
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_handler  = sig_winch_handler;
  act.sa_flags   |= SA_RESTART;
  if(sigaction(SIGWINCH, &act, 0) == -1) // Ignore if it fails. There just won't be any updates
    std::cerr << "Setting signal failed\n";
#endif
}

int get_window_width() {
  if(invalid_window_width) {
    struct winsize w;
    if(ioctl(2, TIOCGWINSZ, &w))
      window_width = 80; // Assume some default value
    else
      window_width = w.ws_col;
    invalid_window_width = false;
  }
  return window_width;
}

std::string shorten_string(std::string s, unsigned int length) {
  if(s.size() <= length) {
    std::string padding(length - s.size(), ' ');
    s += padding;
    return s;
  }
  std::string res("...");
  res += s.substr(s.size() - length + 3, length - 3);
  return res;
}

static const char large_prefix[] = { ' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
static const char small_prefix[] = { ' ', 'm', 'u', 'n', 'p', 'f', 'a', 'z', 'y' };
std::string numerical_field_to_str(double val) {
  size_t ipref  = 0;
  char   prefix = ' ';

  char res[10];
  if(fabs(val) >= 1.0) {
    while(fabs(val) >= 1000.0 && ipref < sizeof(large_prefix)) {
      val /= 1000.0;
      ++ipref;
    }
    if(ipref == sizeof(large_prefix))
      return std::string(val > 0 ? "+infty" : "-infty");
    prefix = large_prefix[ipref];
    snprintf(res, sizeof(res), "% 5.3g%c", std::min(val, 999.0), prefix);
  } else {
    while(fabs(val) < 0.1 && ipref < sizeof(small_prefix)) {
      val *= 1000.0;
      ++ipref;
    }
    if(ipref == sizeof(small_prefix))
      return std::string(val >= 0 ? "    0 " : "   -0 ");
    prefix = small_prefix[ipref];
    if(fabs(val) < 1.0)
      snprintf(res, sizeof(res), "% 5.2g%c", std::min(val, 999.0), prefix);
    else
      snprintf(res, sizeof(res), "% 5.3g%c", std::min(val, 999.0), prefix);
  }
  return std::string(res);
}

struct time_suffix {
  double seconds;
  char   suffix;
};
static const time_suffix time_suffixes[] =
  { { 1.0, 's' }, { 60.0, 'm' }, { 3600.0, 'h' }, { 86400.0, 'd' }, { 86400.0 * 365.25, 'y' }, { 86400.0 * 365.25 * 10.0, ' ' } };
static const size_t nb_time_suffixes = sizeof(time_suffixes) / sizeof(time_suffix);
std::string seconds_to_str(double seconds) {
  char res[10];
  if(seconds < 1.0) {
    return std::string("  < 1s");
  }
  for(size_t i = 1; i < nb_time_suffixes; ++i) {
    if(seconds < time_suffixes[i].seconds) {
      snprintf(res, sizeof(res), "% 5.3g%c", seconds / time_suffixes[i-1].seconds,
              time_suffixes[i-1].suffix);
      return std::string(res);
    }
  }
  // More than a 100 years!

  return std::string(" > 10y");
}

std::string format_eta(bool writable, off_t size, off_t offset, double speed) {
  if(speed == 0.0 || writable)
    return "   -  ";
  if(speed > 0)
    return seconds_to_str((size - offset) / speed);
  return seconds_to_str(offset / -speed);
}

void print_file_list(file_list& list, std::ostream& os) {
  static const int header_width =
    6 /* offset */ + 1 /* slash */ + 6  /* size */ +
    1 /* column */ + 8 /* speed */ + 1  /* column */ +
    6 /* eta */    + 1 /* column */ + 6 /* avg_eta */ + 2 /* spaces */;
  static bool printed_no_file = false;
  static int nb_lines = 0;

  if(nb_lines == 0 && list.empty()) {
    os << "\r --- No regular file open ---";
    printed_no_file = true;
    return;
  }

  if((int)list.size() > nb_lines) {
    int new_lines = list.size() - nb_lines;
    if(printed_no_file)
      new_lines -= 2;
    printed_no_file = false;
    if(new_lines > 0)
      os << "\033[" << new_lines << "S";
  }

  nb_lines = list.size();
  if(nb_lines > 1)
    os << "\033[" << (nb_lines - 1) << "A";

  int window_width = get_window_width();
  for(auto it = list.begin(); it != list.end(); ++it) {
    if(it != list.begin())
      os << "\033[1B";

    // Print offset
    os << "\r" << numerical_field_to_str(it->offset) << "/";
    // Print file size
    if(it->writable) // Don't display size on writable files
      os << "   -  ";
    else
      os << numerical_field_to_str(it->size);
    // Print speed
    os << ":" << numerical_field_to_str(it->speed) << "/s:";
    // Display ETA
    os << format_eta(it->writable, it->size, it->offset, it->speed)
              << ":"
              << format_eta(it->writable, it->size, it->offset, it->average);

    os << "  ";
    if(!it->updated)
      os << "\033[7m";
    os << shorten_string(it->name, window_width - header_width);
    if(!it->updated)
      os << "\033[0m";
  }
  os << "\033[0m" << std::flush;
}
