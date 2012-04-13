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
      fields = sscanf(ptr, "o%li", &f.offset);
      if(fields != 1) return false;
      break;

    case 'i': // Get inode
      fields = sscanf(ptr, "i%li", &f.inode);
      if(fields != 1) return false;
      break;
      
    default: 
      return false;
    }
    
    ptr += strlen(ptr) + 1;
  }

  return true;
}

bool update_file_info(const char* pid_str, file_list& list) {
  const char* cmd[] = { LSOF, "-p", pid_str, "-o", "-Ffiao0", 0 };
  pipe_open offsets_pipe(cmd);
  update_file_info(offsets_pipe, list);
  auto status = offsets_pipe.status();
  return status.first == 0 && 
    WIFEXITED(status.second) &&
    WEXITSTATUS(status.second) == 0;
}

bool update_file_info(std::istream& is, file_list& list) {
  std::string line;
  
  for(auto it = list.begin(); it != list.end(); ++it)
    it->updated = false;

  bool need_updated_name = false;
  while(std::getline(is, line)) {
    file_info f;
    if(!parse_line(line, f))
      continue;
    auto cfile = find_file_in_list(list, f.fd, f.inode);
    if(cfile == list.end()) {
      need_updated_name = true;
      f.updated = true;
      list.push_back(f);
    } else {
      cfile->offset  = f.offset;
      cfile->updated = true;
    }
  }
  return true;
}
