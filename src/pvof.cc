#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <algorithm>
#include <config.h>
#include <src/pvof.hpp>
#include <src/pipe_open.hpp>
#include <src/lsof.hpp>
#include <src/print_info.hpp>
#include <src/timespec.hpp>

pvof args; // The arguments
volatile bool done = false; // Done if we catch a signal

// Stop on TERM and QUIT signals
void sig_termination_handler(int s) {
  done = true;
}
void prepare_termination() {
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_handler = sig_termination_handler;
  sigaction(SIGTERM, &act, 0);
  sigaction(SIGQUIT, &act, 0);
  sigaction(SIGINT, &act, 0);
}


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

  prepare_termination();
  prepare_display();
  
  char pid_str[100];
  snprintf(pid_str, sizeof(pid_str), "%d", pid);
  std::vector<file_info> info_files;

  timespec time_tick;
  if(clock_gettime(CLOCK_MONOTONIC, &time_tick)) {
    std::cerr << "Can't get time" << std::endl;
    return 1;
  }

  while(!done) {
    bool success = update_file_info(pid_str, info_files, time_tick);
    if(!success)
      break;
    print_file_list(info_files);

    time_tick += args.seconds_arg;
    timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if(time_tick < current_time) {
      time_tick  = current_time;
      time_tick += 1;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_tick, 0);
  }
  std::cerr << std::endl;

  return 0;
}
