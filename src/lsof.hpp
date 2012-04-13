#ifndef __LSOF_HPP__
#define __LSOF_HPP__

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>

// Information kept about one file
struct file_info {
  int         fd;
  ino_t       inode;
  std::string name;
  off_t       offset;
  off_t       size;
  bool        updated;
};
// A file is uniquely indexed by the pair (fd, inode)
struct find_file {
  int   fd_;
  ino_t inode_;
  find_file(int fd, ino_t inode) : fd_(fd), inode_(inode) { }
  bool operator()(file_info& rhs) {
    return fd_ == rhs.fd && inode_ == rhs.inode;
  }
};
typedef std::vector<file_info> file_list;
// Find a file in the list matching (fd, inode)
file_list::iterator find_file_in_list(file_list& list, int fd, ino_t inode);

// Parse a line of the output of lsof -F and fill up f
bool parse_line(std::string& line, file_info& f);

// Exec lsof -F on the given pid and update the corresponding list of
// file information (mainly the offset).
bool update_file_info(const char* pid_str, file_list& list);
// Update list of file information from input stream (most likely a
// pipe from lsof -F).
bool update_file_info(std::istream& is, file_list& list);

// Exec lsof -F to get the file size and name information.
bool update_file_names(const char* pid_str, file_list& list);
// Update list from input stream
bool update_file_names(std::istream& is, file_list& list);

#endif
