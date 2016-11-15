#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <list>

const struct timespec tenth_second = { 0, 10000000L };
typedef std::list<std::istream*> fd_list;

bool cat_one_line(fd_list& fds) {
  std::string line;
  bool ret = false;

  for(auto it = fds.begin(); it != fds.end(); ++it) {
    if(*it && std::getline(**it, line)) {
      ret = true;
      std::cout << line << "\n";
    } else {
      delete *it;
      *it = 0;
    }
  }

  return ret;
}

int main(int argc, char* argv[]) {
  fd_list fds;

  // for(int i = 1; i < argc; ++i, ++it)
  //   *it = new std::ifstream((argv[i]));
  int i = 1;
  unsigned long j = 0;

  while(true) {
    if(!cat_one_line(fds) && i >= argc)
      break;
    nanosleep(&tenth_second, 0);
    if(++j % 64 == 0 && i < argc)
      fds.push_back(new std::ifstream(argv[i++]));
  }

  return 0;
}
