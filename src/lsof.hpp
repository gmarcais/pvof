#ifndef __LSOF_HPP__
#define __LSOF_HPP__

#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>
#include <src/timespec.hpp>
#include <src/file_info.hpp>

class lsof_file_info : public file_info_updater {
  std::string pid_str_;
public:
  lsof_file_info(pid_t pid, bool numeric = false)
    : file_info_updater(create_identifier(numeric, pid))
    , pid_str_(std::to_string(pid))
  { }

  // Exec lsof -F on the given pid and update the corresponding list of
  // file information (mainly the offset).
  virtual bool update_file_info(file_list& list, const timespec& stamp);

protected:
  // Parse a line of the output of lsof -F and fill up f
  bool parse_line(std::string& line, file_info& f, bool& failed);

  // Update list of file information from input stream (most likely a
  // pipe from lsof -F).
  bool update_file_info(std::istream& is, file_list& list, const timespec& stamp, bool& need_updated_name);

  // Exec lsof -F to get the file size and name information.
  bool update_file_names(file_list& list);
  // Update list from input stream
  bool update_file_names(std::istream& is, file_list& list);
};

#endif
