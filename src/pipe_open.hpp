#ifndef _PIPE_OPEN_H_
#define _PIPE_OPEN_H_

#include <ext/stdio_filebuf.h>
#include <string>
#include <iostream>

typedef std::pair<int, pid_t> fd_pid;
class pipe_open : public fd_pid, public std::istream {
  typedef std::istream stream;
  typedef __gnu_cxx::stdio_filebuf<char> stdbuf;
public:
  /** Fork and exec the command given in cmd (the array must be 0
      terminated). The object then behaves like an istream connected
      to the standard output of cmd. If check_exec is true, then an
      error is thrown if the exec fail, instead of simply returning an
      empty stream. This allow to distinguish between a failure to
      exec the command (e.g. it can't be found in the path or the user
      does not have the execution right) and a failure of the command
      itself.
   */
  pipe_open(const char* const cmd[], bool check_exec = true,
            bool merge_stderr = false);
  /** Wait on the subprocess upon destruction. */
  virtual ~pipe_open();
  /** Get the status of the sub-process. This will wait on the sub-process. */
  fd_pid status(bool no_hang = false);
};

#endif /* _PIPE_OPEN_H_ */

