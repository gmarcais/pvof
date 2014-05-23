#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cerrno>
#include <src/pipe_open.hpp>
#include <src/lsof.hpp>
#include <src/print_info.hpp>
#include <src/timespec.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <iostream>
#include <algorithm>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <src/pvof.hpp>
#include <src/proc.hpp>

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
  sigaction(SIGALRM, &act, 0);
}


int start_sub_command(std::vector<const char*> args) {
  int pipefd[2];
  if(pipe(pipefd) == -1) { // Don't report error but disable error reporting!
    pipefd[0] = pipefd[1] = -1;
  } else {
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
  }

  int pid = fork();
  if(pid == 0) { // child
    close(pipefd[0]);
    const char** cmd = new const char*[args.size() + 1];
    std::copy(args.begin(), args.end(), cmd);
    cmd[args.size()] = 0;
    execvp(cmd[0], (char* const*)cmd);
    // Got here -> execvp failed!
    if(pipefd[1] != -1) {
      write(pipefd[1], &errno, sizeof(errno));
      close(pipefd[1]);
    }
    _exit(1);
  } else { // parent
    close(pipefd[1]);
    if(pipefd[0] != -1) {
      int res = read(pipefd[0], &errno, sizeof(errno));
      close(pipefd[0]);
      if(res != 0)
        return -1;
    }
  }

  return pid;
}

void wait_sub_command(pid_t pid) {
  // Wait for sub command for at most 1 second
  alarm(1);
  int status;
  pid_t res = waitpid(pid, &status, 0);
  switch(res) {
  case -1: // Ignore error. Probably timeout
  case 0: return;
  default: break;
  }

  if(WIFEXITED(status))
    exit(WEXITSTATUS(status));
  if(WIFSIGNALED(status)) {
    // Restore signal to default setting.
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(WTERMSIG(status), &act, 0); // Ignore failure here?

    // Kill myself with this signal
    kill(getpid(), WTERMSIG(status));
  }

  // Should not be reached. The process should have exited or killed
  // itself. Exit with an error if it failed.
  exit(EXIT_FAILURE);
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
  if(!args.command_arg.empty()) {
    pid = start_sub_command(args.command_arg);
    if(pid == -1) {
      std::cerr << "Command failed to run: " << strerror(errno) << std::endl;
      return 1;
    }
  }

  if(!isatty(2))
    return 0;

  prepare_termination();
  prepare_display();

  std::vector<file_info> info_files;
  std::unique_ptr<file_info_updater> info_updater;
#ifdef HAVE_PROC
  if(!args.lsof_flag)
    info_updater.reset(new proc_file_info(pid));
  else
#endif
    info_updater.reset(new lsof_file_info(pid));

  timespec time_tick;
  if(clock_gettime(CLOCK_MONOTONIC, &time_tick)) {
    std::cerr << "Can't get time" << std::endl;
    return 1;
  }

  bool need_newline = false;
  while(!done) {
    bool success = info_updater->update_file_info(info_files, time_tick);
    if(!success)
      break;
    print_file_list(info_files);
    need_newline = true;

    timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if(time_tick < current_time) {
      time_tick  = current_time;
      time_tick += args.seconds_arg;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_tick, 0);
  }

  std::cerr << "\033[0m";
  if(need_newline)
    std::cerr << std::endl;
  else
    std::cerr << std::flush;


  // If we started the subprocess, get return value or kill
  // signal. Make pvof "transparent".
  if(!args.command_arg.empty())
    wait_sub_command(pid);

  return 0;
}
