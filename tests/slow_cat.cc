#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>

const struct timespec tenth_second = { 0, 100000000L };
void slow_copy(std::istream& is) {
  std::string line;

  while(getline(is, line)) {
    std::cout << line << "\n";
    nanosleep(&tenth_second, 0);
  }
}

int main(int argc, char* argv[]) {
  if(argc == 1)
    slow_copy(std::cin);
  else
    for(int i = 1; i < argc; ++i) {
      std::ifstream input(argv[i]);
      slow_copy(input);
    }

  return 0;
}
