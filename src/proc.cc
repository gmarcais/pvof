#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>

#include <src/proc.hpp>

struct dirfd {
  DIR* d_;
  dirfd(const char* path) : d_(opendir(path)) { }
  ~dirfd() {
    if(d_)
      closedir(d_);
  }
  operator DIR*() { return d_; }
};

std::string full_path(const std::string& path) {
  size_t size = 1024;
  std::unique_ptr<char[]> buf(new char[size]);
  ssize_t len = readlink(path.c_str(), buf.get(), size - 1);
  if(len == -1) return "";
  buf[len] = '\0';
  return std::string(buf.get());
}

bool proc_file_info::update_file_info(file_list& list, const timespec& stamp) {
  for(auto& it : list)
    it.updated = false;

  struct dirfd fdinfo(fdinfo_.c_str());
  if(!fdinfo) return false;

  struct dirent* ent;
  struct stat    stat_buf;
  std::string    p;
  while((ent = readdir(fdinfo))) {
    if(!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name)) continue;
    p = fd_ + '/' + ent->d_name;
    if(stat(p.c_str(), &stat_buf) == -1) continue; // failed to stat -> skip
    if(!force_ && !S_ISREG(stat_buf.st_mode)) continue; // not regular file -> skip

    int fd = std::atoi(ent->d_name);
    auto cfile = list.find(fd, stat_buf.st_ino);
    bool new_file = cfile == list.end();
    if(new_file) {// file does not exists. Add it
      file_info fi;
      fi.fd          = fd;
      fi.inode       = stat_buf.st_ino;
      fi.name        = full_path(p);
      fi.offset      = 0;
      fi.ooffset     = 0;
      fi.size        = stat_buf.st_size;
      fi.writable    = false;
      fi.speed       = 0;
      fi.average     = 0;
      fi.updated     = true;
      fi.stamp       = stamp;
      fi.start       = stamp;
      list.push_back(fi);
      cfile = list.back_iterator();
    }

    off_t save_offset = cfile->offset;
    p = fdinfo_ + '/' + ent->d_name;
    std::ifstream fdinfo_fd(p);
    if(!fdinfo_fd.good()) continue;
    update_file_info(*cfile, stamp, fdinfo_fd, new_file);
    fdinfo_fd.close();

    if(stamp != cfile->start) {
      cfile->speed   = (cfile->offset - save_offset) / timespec_double(stamp - cfile->stamp);
      cfile->average = (cfile->offset - cfile->ooffset) / timespec_double(stamp - cfile->start);
    } else {
      cfile->ooffset = cfile->offset;
    }
    cfile->stamp   = stamp;
    cfile->updated = true;
  }
  return true;
}

bool proc_file_info::update_file_info(file_info& info, const timespec& stamp, std::istream& in, const bool is_new) {
  std::string label;

  try {
    while(in.good()) {
      in >> label;
      if(in.eof()) break;
      if(label == "pos:") {
        in >> std::dec >> info.offset;
      } else if(label == "flags:" && is_new) {
        int flags;
        in >> std::oct >> flags;
        info.writable = (flags & O_WRONLY) || (flags & O_RDWR);
      } else {
        int ignore;
        in >> ignore;
      }
    }
  } catch(std::ios_base::failure& e) {
    // Do nothing
  }
  return true;
}

bool proc_file_info::update_io_info(io_info& info, const timespec& stamp) {
  std::string label;
  uint64_t rchar, wchar, rsys, wsys, rio, wio;

  try {
    std::ifstream is(ioinfo_);
    is >> label >> rchar
       >> label >> wchar
       >> label >> rsys
       >> label >> wsys
       >> label >> rio
       >> label >> wio;
    if(!is.good())
      throw std::ios_base::failure("Couldn't read information");
  } catch(std::ios_base::failure& e) {
    ++info.dead_count;
    return false;
  }

  const bool empty = (info.start.tv_sec == 0);
  if(!empty) {
    const double speed_delta = timespec_double(stamp - info.stamp);
    const double avg_delta   = timespec_double(stamp - info.start);
    info.char_speed.read     = (rchar - info.char_counter.read) / speed_delta;
    info.char_avg.read       = (rchar - info.ochar_counter.read) / avg_delta;
    info.char_speed.write    = (wchar - info.char_counter.write) / speed_delta;
    info.char_avg.write      = (wchar - info.ochar_counter.write) / avg_delta;
    info.sys_speed.read      = (rsys - info.sys_counter.read) / speed_delta;
    info.sys_avg.read        = (rsys - info.osys_counter.read) / avg_delta;
    info.sys_speed.write     = (wsys - info.sys_counter.write) / speed_delta;
    info.sys_avg.write       = (wsys - info.osys_counter.write) / avg_delta;
    info.io_speed.read       = (rio - info.io_counter.read) / speed_delta;
    info.io_avg.read         = (rio - info.oio_counter.read) / avg_delta;
    info.io_speed.write      = (wio - info.io_counter.write) / speed_delta;
    info.io_avg.write        = (wio - info.oio_counter.write) / avg_delta;
  }

  info.char_counter.read  = rchar;
  info.char_counter.write = wchar;
  info.sys_counter.read   = rsys;
  info.sys_counter.write  = wsys;
  info.io_counter.read    = rio;
  info.io_counter.write   = wio;
  info.stamp              = stamp;

  if(empty) {
    info.ochar_counter.read  = rchar;
    info.ochar_counter.write = wchar;
    info.osys_counter.read   = rsys;
    info.osys_counter.write  = wsys;
    info.oio_counter.read    = rio;
    info.oio_counter.write   = wio;
    info.start               = stamp;
  }
  return true;
}
