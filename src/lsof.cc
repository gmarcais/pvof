#include <iostream>
#include <config.h>
#include <src/pipe_open.hpp>
#include <src/lsof.hpp>
#include <src/file_info.hpp>


bool lsof_file_info::parse_line(std::string& line, file_info& f, bool& failed) {
  const char* ptr = line.c_str();
  const char* const end = ptr + line.size();
  int fields;

  failed = false;
  while(ptr < end) {
    switch(*ptr) {
    case 'f': // Get file descriptor
      if(strcmp("fNOFD", ptr) == 0) {
        failed = true;
        return false;
      }
      fields = sscanf(ptr, "f%d", &f.fd);
      if(fields != 1) return false;
      break;

    case 't': // Accept only regular file
      if(strcmp("tREG", ptr)) return false;
      break;

    case 'a': // Get access rights. Writable and read/write are the same
      switch(ptr[1]) {
      case 'r':
        f.writable = false; break;
      case 'w':
      case 'u':
        f.writable = true; break;
      default:
        return false;
      }
      break;

    case 'o': // Get offset
      fields = sscanf(ptr, "o0t%ld", &f.offset);
      if(fields != 1) {
        fields = sscanf(ptr, "o%li", &f.offset);
        if(fields != 1) return false;
      }
      break;

    case 's': // Get size
      fields = sscanf(ptr, "s%ld", &f.size);
      if(fields != 1) return false;
      break;

    case 'i': // Get inode
      fields = sscanf(ptr, "i%li", &f.inode);
      if(fields != 1) return false;
      break;

    case 'n': // Get name
      f.name.assign(ptr + 1);
      break;

    default:
      return false;
    }

    ptr += strlen(ptr) + 1;
  }

  return true;
}

bool lsof_file_info::update_file_info(file_list& list, const timespec& stamp) {
  const char* cmd[] = { LSOF, "-p", pid_str_.c_str(), "-o0", "-o", "-Fftiao0", 0 };
  pipe_open offsets_pipe(cmd, true, true);
  bool need_updated_name = false;

  bool return_status = update_file_info(offsets_pipe, list, stamp, need_updated_name);
  if(return_status && need_updated_name)
    return_status = update_file_names(list);

  auto status = offsets_pipe.status();
  return_status = return_status &&
    status.first == 0 &&
    WIFEXITED(status.second) &&
    WEXITSTATUS(status.second) == 0;
  return return_status;
}

bool lsof_file_info::update_file_info(std::istream& is, file_list& list, const timespec& stamp, bool& need_updated_name) {
  std::string line;

  for(auto it = list.begin(); it != list.end(); ++it)
    it->updated = false;

  need_updated_name = false;
  while(std::getline(is, line)) {
    file_info f;
    f.offset = f.size = 0;
    bool failed = false;
    if(!parse_line(line, f, failed)) {
      if(failed)
        return false;
      continue;
    }
    auto cfile = list.find(f.fd, f.inode);
    if(cfile == list.end()) {
      // Append new entry
      need_updated_name = true;
      f.updated         = true;
      f.stamp           = stamp;
      list.push_back(f);
      continue;
    }
    // Update existing entry
    cfile->speed   = (f.offset - cfile->offset) / timespec_double(stamp - (cfile->stamp));
    cfile->offset  = f.offset;
    cfile->updated = true;
    cfile->stamp   = stamp;
  }

  return true;
}

bool lsof_file_info::update_file_names(file_list& list) {
  const char* cmd[] = { LSOF, "-p", pid_str_.c_str(), "-s", "-Ffiasn0", 0 };
  pipe_open names_pipe(cmd, true, true);
  bool return_status = update_file_names(names_pipe, list);

  auto status = names_pipe.status();
  return return_status &&
    status.first == 0 &&
    WIFEXITED(status.second) &&
    WEXITSTATUS(status.second) == 0;
}

bool lsof_file_info::update_file_names(std::istream& is, file_list& list) {
  std::string line;
  while(std::getline(is, line)) {
    file_info f;
    f.offset = f.size = 0;
    bool failed;
    if(!parse_line(line, f, failed)) {
      if(failed)
        return false;
      continue;
    }
    auto cfile = list.find(f.fd, f.inode);
    if(cfile == list.end())
      continue;
    cfile->size = f.size;
    cfile->name.swap(f.name);
  }

  return true;
}
