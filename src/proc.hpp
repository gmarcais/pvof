#ifndef __PROC_H__
#define __PROC_H__

#include <string>
#include <src/timespec.hpp>
#include <src/file_info.hpp>


class proc_file_info : public file_info_updater {
  const std::string fdinfo_;
  const std::string fd_;
  const std::string ioinfo_;
  const bool        force_;

public:
  explicit proc_file_info(pid_t pid, bool force = false, bool numeric = false)
    : file_info_updater(pid, create_identifier(numeric, pid))
    , fdinfo_(std::string("/proc/") + std::to_string(pid) + "/fdinfo")
    , fd_(std::string("/proc/") + std::to_string(pid) + "/fd")
    , ioinfo_(std::string("/proc/") + std::to_string(pid) + "/io")
    , force_(force)
  { }

  virtual bool update_file_info(file_list& list, const timespec& stamp);
  virtual bool update_io_info(io_info& info, const timespec& stamp);

protected:
  bool update_file_info(file_info& info, const timespec& stamp, std::istream& in, const bool is_new);
};

// Find add the process with a command that match a word in <cmds>, and append
// to pids.
void find_cmds(const std::vector<const char*>& cmds, std::vector<pid_t>& pids);

#endif /* __PROC_H__ */
