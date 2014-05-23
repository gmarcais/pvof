#ifndef __FILE_INFO_H__
#define __FILE_INFO_H__

#include <vector>
#include <string>
#include <algorithm>

// Information kept about one file
struct file_info {
  int             fd;
  ino_t           inode;
  std::string     name;
  off_t           offset;
  off_t           size;
  bool            writable;
  double          speed;
  bool            updated;
  struct timespec stamp;
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
inline file_list::iterator find_file_in_list(file_list& list, int fd, ino_t inode) {
  return std::find_if(list.begin(), list.end(), find_file(fd, inode));
}

class file_info_updater {
public:
  virtual bool update_file_info(file_list& list, const timespec& stamp) = 0;
};

#endif
