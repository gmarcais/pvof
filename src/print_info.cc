#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
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

void print_file_info(file_list& list) {
  //  static int nb_lines = 0;

  std::cerr << __PRETTY_FUNCTION__ << " " << list.size() << std::endl;

  // if(nb_lines)
  //   std::cerr << "\033[" << nb_lines << "A";
  
  for(auto it = list.begin(); it != list.end(); ++it) {
    std::cerr << "fd " << it->fd << " inode " << it->inode 
              << " offset " << it->offset << " width " << get_window_width() 
              << " name " << it->name << " size " << it->size << "\n";
  }

  std::cout << std::flush;
}
