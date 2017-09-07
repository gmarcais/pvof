#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>

#include <src/pipe_open.hpp>
#include <src/lsof.hpp>
#include <src/print_info.hpp>
#include <src/timespec.hpp>
#include <src/pvof.hpp>
#include <src/proc.hpp>

pvof args; // The arguments
volatile bool done = false; // Done if we catch a signal
volatile bool no_display = false; // Stop display of file status

// Stop on TERM and QUIT signals
void sig_termination_handler(int s) {
  done = true;
}
void sig_toggle_display(int s) {
  no_display = !no_display;
}
void prepare_termination() {
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_handler = sig_termination_handler;
  sigaction(SIGTERM, &act, 0);
  sigaction(SIGQUIT, &act, 0);
  sigaction(SIGINT, &act, 0);
  sigaction(SIGALRM, &act, 0);

  // Toggle display on SIGUSR1
  memset(&act, '\0', sizeof(act));
  act.sa_handler = sig_toggle_display;
  sigaction(SIGUSR1, &act, 0);
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
    int exit_code = 1;
    if(pipefd[1] != -1) {
      auto s = write(pipefd[1], &errno, sizeof(errno));
      if(s == -1)
        exit_code = 2;
      close(pipefd[1]);
    }
    _exit(exit_code);
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

void wait_sub_command(pid_t pid, bool forever = false) {
  // Wait for sub command for at most 1 second
  if(!forever)
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

bool display_file_progress(int pid, std::ostream& os, bool force) {
  prepare_display();

  std::vector<file_info> info_files;
  std::unique_ptr<file_info_updater> info_updater;
#ifdef HAVE_PROC
  if(!args.lsof_flag)
    info_updater.reset(new proc_file_info(pid, force));
  else
#endif
    info_updater.reset(new lsof_file_info(pid));

  timespec time_tick;
  if(clock_gettime(CLOCK_MONOTONIC, &time_tick)) {
    os << "Can't get time" << std::endl;
    return false;
  }

  bool need_newline = false;
  while(!done) {
    bool success = info_updater->update_file_info(info_files, time_tick);
    if(!success)
      break;
    if(!no_display) {
      print_file_list(info_files, os);
      need_newline = true;
    }

    timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if(time_tick < current_time) {
      time_tick  = current_time;
      time_tick += args.seconds_arg;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_tick, 0);
  }

  os << "\033[0m";
  if(need_newline)
    os << std::endl;
  else
    os << std::flush;
  return true;
}

bool display_io_progress(int pid, std::ostream& os) {
  struct iostat {
    const std::string label;
    const size_t      start_value;
    size_t            value;
  };
  std::vector<iostat> stats;

  // First read of the /proc/<pid>/io file
  const std::string path = std::string("/proc/") + std::to_string(pid) + "/io";
  std::ifstream is(path);
  if(!is.good()) {
    os << "Can't open io file '" << path << '\'' << std::endl;
    return false;
  }
  timespec time_tick;
  if(clock_gettime(CLOCK_MONOTONIC, &time_tick)) {
    os << "Can't get time" << std::endl;
    return false;
  }
  const timespec start_time = time_tick;

  std::string label;
  size_t value;
  size_t width = 0;
  while(is >> label >> value) {
    width = std::max(width, label.size());
    stats.push_back({label, value, value});
  }
  std::cout << "stats size: " << stats.size() << '\n';
  is.close();

  bool first = true;
  while(!done) {
    is.open(path);
    if(!is.good())
      return false;

    timespec new_time;
    clock_gettime(CLOCK_MONOTONIC, &new_time);
    const double delta = timespec_double(new_time - time_tick);
    const double start_delta = timespec_double(new_time - start_time);
    if(!first)
      os << "\033[" << stats.size() << 'F';
    else
      first = false;
    for(auto& st : stats) {
      is >> label >> value;
      if(st.label != label) {
        os << "Unexpected change in '" << path << " 'format" << std::endl;
        return false;
      }
      double speed = delta > 0.0 ? (double)(value - st.value) / delta : 0.0;
      double avg = start_delta > 0.0 ? (double)(value - st.start_value) / start_delta : 0.0;
      st.value = value;
      os << std::setw(width) << std::left << label << std::right
         << ' ' << numerical_field_to_str(value)
         << ' ' << numerical_field_to_str(speed)
         << "/s:" << numerical_field_to_str(avg)
         << "/s\n";
    }
    is.close();
    os << std::flush;

    time_tick = new_time;
    new_time += args.seconds_arg;
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &new_time, NULL);
  }

  return true;
}

bool open_output(int fd, std::ofstream& os) {
  // Select which terminal to display on
  if(fd >= 0) {
    os.open(std::string("/proc/self/fd/") + std::to_string(fd));
  } else {
    for(int i = 2; i >= 0; --i) {
      if(isatty(i)) {
        os.open(std::string("/proc/self/fd/") + std::to_string(i));
        break;
      }
    }
  }
  return os.is_open() && os.good();
}

int main(int argc, char *argv[])
{
  args.parse(argc, argv);

  if(args.pid_given && !args.command_arg.empty())
    pvof::error() << "Either, but not both, a process is passed with -p or a command is given on the command line";
  if(!args.pid_given && args.command_arg.empty())
    pvof::error() << "Need a process id or a command to run";

  int pid = args.pid_arg;
  if(!args.command_arg.empty()) {
    pid = start_sub_command(args.command_arg);
    if(pid == -1)
      pvof::error() << "Command failed to run: " << strerror(errno);
  }

  prepare_termination();

  bool wait_forever = false;
  std::ofstream output;
  if(!open_output(args.fd_given ? args.fd_arg : -1, output)) {
    std::cerr << "pvof: No terminal to display on" << std::endl;
    wait_forever = true;
  }

  if(!wait_forever) {
    if(args.io_flag) {
      wait_forever = !display_io_progress(pid, output);
    } else {
      wait_forever = !display_file_progress(pid, output, args.force_flag);
    }
  }


  // If we started the subprocess, get return value or kill
  // signal. Make pvof "transparent".
  if(!args.command_arg.empty())
    wait_sub_command(pid, wait_forever);

  return 0;
}
