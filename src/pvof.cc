#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <config.h>
#include <src/pvof.hpp>
#include <src/pipe_open.hpp>
#include <src/lsof.hpp>
#include <src/print_info.hpp>

pvof args; // The arguments
volatile bool done = false; // Done if we catch a signal

int start_sub_command(std::vector<const char*> args) {
  int pid = fork();
  if(pid == 0) {
    const char** cmd = new const char*[args.size() + 1];
    std::copy(args.begin(), args.end(), cmd);
    cmd[args.size()] = 0;
    execvp(cmd[0], (char* const*)cmd);
    _exit(1);
  }
  
  return pid;
}




int main(int argc, char *argv[])
{
  args.parse(argc, argv);

  if(args.pid_given && !args.command_arg.empty()) {
    std::cerr << "Either, but not both, a process is passed with -p or a command is given on the command line" << std::endl;
    return 1;
  }
  if(!args.pid_given && args.command_arg.empty()) {
    std::cerr << "Need process id or a command to run" << std::endl;
    return 1;
  }

  int pid = args.pid_arg;
  if(!args.command_arg.empty())
    pid = start_sub_command(args.command_arg);
  if(pid < 0) {
    std::cerr << "Invalid pid or command failed to run" << std::endl;
    return 1;
  }

  if(!isatty(2))
    return 0;

  prepare_display();
  
  char pid_str[100];
  snprintf(pid_str, sizeof(pid_str), "%d", pid);
  std::vector<file_info> info_files;

  while(!done) {
    bool success = update_file_info(pid_str, info_files);
    if(!success)
      break;
    print_file_info(info_files);
    sleep(args.seconds_arg);
  }

  return 0;
}
