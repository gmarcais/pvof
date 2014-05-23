#ifndef __PROC_H__
#define __PROC_H__

#include <string>
#include <src/timespec.hpp>
#include <src/file_info.hpp>


class proc_file_info : public file_info_updater {
  const std::string fdinfo_;
  const std::string fd_;

public:
  proc_file_info(pid_t pid) :
    fdinfo_(std::string("/proc/") + std::to_string(pid) + "/fdinfo"),
    fd_(std::string("/proc/") + std::to_string(pid) + "/fd")
  { }

  virtual bool update_file_info(file_list& list, const timespec& stamp);

protected:
  bool update_file_info(file_info& info, const timespec& stamp, std::istream& in, const bool is_new);
};

#endif /* __PROC_H__ */
