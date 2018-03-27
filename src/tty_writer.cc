#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <iostream>

#include <src/tty_writer.hpp>

volatile bool tty_writer::invalid_window_width = true;
int tty_writer::window_width = 80;

void tty_writer::sig_winch_handler(int s) {
  invalid_window_width = true;
}

void tty_writer::prepare_display() {
#ifdef SIGWINCH
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_handler  = sig_winch_handler;
  act.sa_flags   |= SA_RESTART;
  if(sigaction(SIGWINCH, &act, 0) == -1) // Ignore if it fails. There just won't be any updates
    std::cerr << "Setting signal failed\n";
#endif
}

int tty_writer::get_window_width() {
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
