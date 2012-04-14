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
  if(s.size() <= length)
    return s;
  std::string res("...");
  res += s.substr(s.size() - length + 3, length - 3);
  return res;
}

static const char prefix[] = { ' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
std::string numerical_field_to_str(double val) {
  int  ipref       = 0;

  while(fabs(val) >= 1000.0) {
    val /= 1000.0;
    ++ipref;
  }
  char res[10];
  snprintf(res, sizeof(res), "% 5.3g%c", val, prefix[ipref]);
  return std::string(res);
}

void print_file_list(file_list& list) {
  static int nb_lines = 0;

  if(nb_lines > 1)
    std::cerr << "\033[" << (nb_lines - 1) << "A";

  int window_width = get_window_width();
  for(auto it = list.begin(); it != list.end(); ++it) {
    if(it != list.begin()) {
      if(it - list.begin() > nb_lines)
        std::cerr << "\n";
      else
        std::cerr << "\033[1B";
    }
    std::cerr << "\r" << numerical_field_to_str(it->offset) << "/"
              << numerical_field_to_str(it->size) << "  " 
              << shorten_string(it->name, window_width - 15);
  }
  std::cout << std::flush;

  nb_lines = list.size();
}
