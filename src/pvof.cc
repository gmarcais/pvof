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
#include <set>

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


pid_t start_sub_command(std::vector<const char*> args) {
  int pipefd[2];
  if(pipe(pipefd) == -1) { // Don't report error but disable error reporting!
    pipefd[0] = pipefd[1] = -1;
  } else {
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
  }

  pid_t pid = fork();
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

#ifdef HAVE_PROC
void update_pid_children(std::set<pid_t>& pid_set, updater_list_type& updaters, list_of_file_list& files,
                         io_info_list& info_ios) {
  std::string pid_str;
  std::string path;
  pid_t       npid;
  timespec time_tick;
  clock_gettime(CLOCK_MONOTONIC, &time_tick);
  for(auto pid : pid_set) {
    pid_str = std::to_string(pid);
    path = "/proc/";
    path += pid_str;
    path += "/task/";
    path += pid_str;
    path += "/children";
    std::ifstream is(path);
    while(is >> npid) {
      auto is_new = pid_set.insert(npid);
      if(is_new.second) { // new pid inserted
        if(!args.lsof_flag)
          updaters.emplace_back(new proc_file_info(npid, args.force_flag, args.numeric_flag));
        else
          updaters.emplace_back(new lsof_file_info(npid, args.numeric_flag));
        files.push_back(file_list());
        info_ios.push_back(io_info());
        updaters.back()->update_io_info(info_ios.back(), time_tick);
      }
    }
  }
}
#endif // HAVE_PROC

bool display_file_progress(const std::vector<pid_t>& pids, tty_writer& writer) {
  list_of_file_list info_files;
  io_info_list      info_ios;
  updater_list_type info_updaters;
  std::set<pid_t>   pid_set;

  timespec time_tick;
  if(clock_gettime(CLOCK_MONOTONIC, &time_tick)) {
    std::cerr << "Can't get time" << std::endl;
    return false;
  }

  for(const auto pid : pids) {
#ifdef HAVE_PROC
    if(!args.lsof_flag)
      info_updaters.emplace_back(new proc_file_info(pid, args.force_flag, args.numeric_flag));
    else
#endif
      info_updaters.emplace_back(new lsof_file_info(pid, args.numeric_flag));
    info_files.push_back(file_list());
    info_ios.push_back(io_info());
    info_updaters.back()->update_io_info(info_ios.back(), time_tick);
    pid_set.insert(pid);
  }

  clock_gettime(CLOCK_MONOTONIC, &time_tick);
  std::vector<size_t> dead_processes;
  while(!done) {
    bool success = false;
    size_t total_lines = 0;
    for(size_t i = 0; i < info_updaters.size(); ++i) {
      success = info_updaters[i]->update_io_info(info_ios[i], time_tick) || success;
      success = info_updaters[i]->update_file_info(info_files[i], time_tick) || success;
      total_lines += info_files[i].size();
      if(args.clean_arg && info_ios[i].dead_count > args.clean_arg)
        dead_processes.push_back(i);
    }
    if(!success)
      break;
    if(!no_display)
      print_file_list(info_updaters, info_files, info_ios, writer);

    // Clean up
    for(auto it = dead_processes.rbegin(); it != dead_processes.rend(); ++it) {
      info_ios.erase(info_ios.begin() + *it);
      info_files.erase(info_files.begin() + *it);
      pid_set.erase(*it);
    }
    dead_processes.clear();
#ifdef HAVE_PROC
    if(args.follow_flag)
      update_pid_children(pid_set, info_updaters, info_files, info_ios);
#endif

    timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    if(time_tick < current_time) {
      time_tick  = current_time;
      time_tick += args.seconds_arg;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_tick, 0);
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

  if(args.pid_arg.empty() && args.command_arg.empty())
    pvof::error() << "A process ID (-p switch) or a command is necessary";

  std::vector<pid_t> pids(args.pid_arg.size(), -1);
  std::copy(args.pid_arg.cbegin(), args.pid_arg.cend(), pids.begin());
  if(!args.command_arg.empty()) {
    pid_t pid = start_sub_command(args.command_arg);
    if(pid == -1)
      pvof::error() << "Command failed to run: " << strerror(errno);
    pids.push_back(pid);
  }

  prepare_termination();

  bool wait_forever = false;
  std::ofstream output;
  if(!open_output(args.fd_given ? args.fd_arg : -1, output)) {
    std::cerr << "pvof: No terminal to display on" << std::endl;
    wait_forever = true;
  }
  tty_writer writer(output, !args.nocolor_flag);

  if(!wait_forever) {
    wait_forever = !display_file_progress(pids, writer);
  }


  // If we started the subprocess, get return value or kill
  // signal. Make pvof "transparent".
  if(!args.command_arg.empty())
    wait_sub_command(pids.back(), wait_forever);

  return 0;
}
