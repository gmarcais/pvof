#include <iostream>
#include <config.h>
#include <src/pipe_open.hpp>
#include <src/lsof.hpp>

file_list::iterator find_file_in_list(file_list& list, int fd, ino_t inode) {
  return std::find_if(list.begin(), list.end(), find_file(fd, inode));
}

bool parse_line(std::string& line, file_info& f) {
  const char* ptr = line.c_str();
  const char* const end = ptr + line.size();
  int fields;

  while(ptr < end) {
    switch(*ptr) {
    case 'f': // Get file descriptor
      fields = sscanf(ptr, "f%d", &f.fd);
      if(fields != 1) return false;
      break;

    case 'a': // Get access rights. Handle read only file for now
      if(ptr[1] != 'r') return false;
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

bool update_file_info(const char* pid_str, file_list& list, timespec& stamp) {
  bool return_status = true;
  const char* cmd[] = { LSOF, "-p", pid_str, "-o0", "-o", "-Ffiao0", 0 };
  pipe_open offsets_pipe(cmd, true, true);
  if(update_file_info(offsets_pipe, list, stamp))
    return_status = return_status && update_file_names(pid_str, list);

  auto status = offsets_pipe.status();
  return_status = return_status &&
    status.first == 0 && 
    WIFEXITED(status.second) &&
    WEXITSTATUS(status.second) == 0;
  return return_status;
}

bool update_file_info(std::istream& is, file_list& list, timespec& stamp) {
  std::string line;
  
  for(auto it = list.begin(); it != list.end(); ++it)
    it->updated = false;

  bool need_updated_name = false;
  while(std::getline(is, line)) {
    file_info f;
    f.offset = f.size = 0;
    if(!parse_line(line, f))
      continue;
    auto cfile = find_file_in_list(list, f.fd, f.inode);
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

  return need_updated_name;
}

bool update_file_names(const char* pid_str, file_list& list) {
  const char* cmd[] = { LSOF, "-p", pid_str, "-s", "-Ffiasn0", 0 };
  pipe_open names_pipe(cmd, true, true);
  update_file_names(names_pipe, list);
  
  auto status = names_pipe.status();
  return status.first == 0 && 
    WIFEXITED(status.second) &&
    WEXITSTATUS(status.second) == 0;
}

bool update_file_names(std::istream& is, file_list& list) {
  std::string line;
  while(std::getline(is, line)) {
    file_info f;
    f.offset = f.size = 0;
    if(!parse_line(line, f))
      continue;
    auto cfile = find_file_in_list(list, f.fd, f.inode);
    if(cfile == list.end())
      continue;
    cfile->size = f.size;
    cfile->name.swap(f.name);
  }
  
 
  return true;
}
