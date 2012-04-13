#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <ext/stdio_filebuf.h>
#include <string>
#include <iostream>
#include <utility>
#include <stdexcept>

#include <src/pipe_open.hpp>

/** Close the read pipe, set stdout to the write pipe and exec the
    command. If merge_stderr is true, stderr is merged with stdout.
 */
typedef char *const *execvp_argv;
int exec_command(int pipe_fd[2], const char* const cmd[], bool merge_stderr) {
  if(close(pipe_fd[0]) == -1) return errno;
  if(dup2(pipe_fd[1], 1) == -1) return errno;
  if(merge_stderr)
    if(dup2(pipe_fd[1], 2) == -1) return errno;
  execvp(cmd[0], (execvp_argv)cmd);
  return errno;
}

/** Execute a sub-command (without any shell) and return a file
    descriptor connected to its stdout and its pid. In case of
    error, -1 is returned and errno is set appropriately. The error
    could come from pipe(2), fork(2), fcntl(2), dup2(2), close(2) or
    execvp(3).
*/
typedef std::pair<int, int> fd_pid;
fd_pid open_sub_process(const char* const cmd[], bool check_exec = true,
                        bool merge_stderr = false) {
  // pipe to tell whether the exec call worked
  int exec_worked[2] = { -1, -1 };
  // pipe connected to sub-command stdout
  int sub_stdout[2] = { -1, -1 };
  int     ret, pid;
  ssize_t child_read;

  if(pipe(sub_stdout) == -1) goto failed;
  if(check_exec) {
    if(pipe(exec_worked) == -1) goto failed;
    if(fcntl(exec_worked[1], F_SETFD, FD_CLOEXEC) == -1) goto failed;
  }
    
  switch((pid = fork())) {
  case -1: goto failed;
  case 0: // Child
    // If we fail to close the read end of pipe, we cannot report the
    // error! Simply exit.
    if(check_exec)
      if(close(exec_worked[0]) == -1) _exit(1);
    ret = exec_command(sub_stdout, cmd, merge_stderr);
    // if reached here, a problem occurred. Send it to parent
    if(check_exec)
      // The loop is here to please gcc warning and using the output
      // of write.
      while(write(exec_worked[1], &ret, sizeof(ret)) == -1)
        if(errno != EINTR)
          break;
    _exit(1);
      
  default: // Parent
    if(close(sub_stdout[1]) == -1) goto failed;
    if(check_exec) {
      if(close(exec_worked[1]) == -1) goto failed;
      child_read = ::read(exec_worked[0], (void*)&ret, sizeof(ret));
      close(exec_worked[0]);
      switch(child_read) {
      case -1: goto failed;
      case 0: break; // Success
      default:
        errno = ret;
        goto failed;
      }
    }
  }
  return std::make_pair(sub_stdout[0], pid);

 failed:
  int save_errno = errno;
  if(exec_worked[0] >= 0) close(exec_worked[0]);
  if(exec_worked[1] >= 0) close(exec_worked[1]);
  if(sub_stdout[0] >= 0) close(sub_stdout[0]);
  if(sub_stdout[1] >= 0) close(sub_stdout[1]);
  errno = save_errno;
  return std::make_pair(-1, 0);
}

/** Same as open_sub_process but throw an exception in case of error.
 */
fd_pid open_sub_process_throw(const char* const cmd[], bool check_exec = true,
                              bool merge_stderr = false) {
  fd_pid ret = open_sub_process(cmd, check_exec, merge_stderr);
  if(ret.first == -1) {
    int save_errno = errno;
    std::string error("Failed to start command '");
    for(const char* const* it = cmd; *it; ++it) {
      if(it != cmd)
        error += " ";
      error += *cmd;
    }
    error += "': ";
    error += strerror(save_errno);
    throw std::runtime_error(error);
  }
  return ret;
}


pipe_open::pipe_open(const char* const cmd[], bool check_exec, bool merge_stderr) :
  fd_pid(open_sub_process_throw(cmd, check_exec, merge_stderr)),
  stream(new stdbuf(fd_pid::first, std::ios::in))
{
  stdbuf* rdbuf = (stdbuf*)stream::rdbuf();
  if(rdbuf->fd() == -1)
    stream::setstate(std::ios::badbit);
}

pipe_open::~pipe_open() { status(); }

std::pair<int, pid_t> pipe_open::status(bool no_hang) {
  pid_t status = -1;
  errno = 0;
  waitpid(fd_pid::second, &status, no_hang ? WNOHANG : 0);
  return std::make_pair(errno, status);
}
