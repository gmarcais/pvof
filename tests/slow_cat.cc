#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>

const struct timespec tenth_second = { 0, 10000000L };
typedef std::vector<std::istream*> fd_list;

bool cat_one_line(fd_list& fds) {
  std::string line;
  bool ret = false;

  for(auto it = fds.begin(); it != fds.end(); ++it) {
    if(std::getline(**it, line)) {
      ret = true;
      std::cout << line << "\n";
    }
  }
  
  return ret;
}

int main(int argc, char* argv[]) {
  fd_list fds(argc - 1);
  
  for(int i = 1; i < argc; ++i)
    fds[i-1] = new std::ifstream((argv[i]));

  while(true) {
    if(!cat_one_line(fds))
      break;
    nanosleep(&tenth_second, 0);
  }

  return 0;
}
