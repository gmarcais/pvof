#include <fstream>

#include <src/file_info.hpp>


std::string create_identifier(bool numeric, pid_t pid) {
  const std::string strpid = std::to_string(pid);
  if(numeric) return strpid;
  std::ifstream is(std::string("/proc/" + strpid + "/cmdline"));
  if(!is.good()) return strpid;
  std::string name;
  std::getline(is, name, '\0');
  if(name.empty()) return strpid;
  const auto slash = name.find_last_of("/");
  return (slash == std::string::npos) ? name : name.substr(slash + 1);
}
