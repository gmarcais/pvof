#ifndef __FILE_INFO_H__
#define __FILE_INFO_H__

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

// Information kept about one file
struct file_info {
  int             fd;
  ino_t           inode;
  std::string     name;
  off_t           offset;
  off_t           ooffset;
  off_t           size;
  bool            writable;
  double          speed;
  double          average;
  bool            updated;
  struct timespec stamp;
  struct timespec start;
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

class file_info_updater;
struct file_list {
  typedef std::vector<file_info>    list_type;
  typedef list_type::iterator       iterator;
  typedef list_type::const_iterator const_iterator;

  file_info_updater& source;
  list_type          list;

  file_list(file_info_updater& s)
    : source(s)
  { }
  iterator find(int fd, ino_t inode) { return std::find_if(list.begin(), list.end(), find_file(fd, inode)); }

  void push_back(file_info&& f) { list.push_back(std::move(f)); }
  void push_back(const file_info& f) { list.push_back(f); }
  iterator back_iterator() { return list.end() - 1; }
  iterator begin() { return list.begin(); }
  iterator end() { return list.end(); }
  const_iterator begin() const { return list.begin(); }
  const_iterator end() const { return list.end(); }
  size_t size() const { return list.size(); }
};
typedef std::vector<file_list> list_of_file_list;

std::string create_identifier(bool numeric, pid_t pid);

class file_info_updater {
  const std::string strid_;
public:
  file_info_updater() : strid_("") { }
  file_info_updater(const std::string&& s) : strid_(std::move(s)) { }
  const std::string& strid() const { return strid_; }
  virtual bool update_file_info(file_list& list, const timespec& stamp) = 0;
};
typedef std::unique_ptr<file_info_updater> updater_ptr;
typedef std::vector<updater_ptr>           updater_list_type;

#endif
