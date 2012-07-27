#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <iostream>

int main(int argc, char *argv[])
{
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << " status\n";
    exit(EXIT_FAILURE);
  }

  int status = atoi(argv[1]);
  if(WIFEXITED(status)) {
    std::cout << "exit(" << WEXITSTATUS(status) << ")\n";
  } else if(WIFSIGNALED(status)) {
    std::cout << "kill(" << WTERMSIG(status) << "|" 
              << strsignal(WTERMSIG(status)) << ")";
#ifdef WCOREDUMP
    if(WCOREDUMP(status))
      std::cout << " -> core";
#endif
    std::cout << "\n";
  } else if(WIFSTOPPED(status)) {
    std::cout << "stopped(" << WSTOPSIG(status) << "|"
              << strsignal(WSTOPSIG(status)) << ")\n";
  } else if(WIFCONTINUED(status)) {
    std::cout << "continued(SIGCONT)\n";
  } else {
    std::cout << "invalid status\n";
  }

  return 0;
}
