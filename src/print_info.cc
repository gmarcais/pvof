#include <string.h>
#include <math.h>
#include <iostream>

#include <src/tty_writer.hpp>
#include <src/print_info.hpp>


std::string shorten_string(std::string s, unsigned int length) {
  if(s.size() <= length) {
    std::string padding(length - s.size(), ' ');
    s += padding;
    return s;
  }
  std::string res("...");
  res += s.substr(s.size() - length + 3, length - 3);
  return res;
}

static const char large_prefix[] = { ' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
static const char small_prefix[] = { ' ', 'm', 'u', 'n', 'p', 'f', 'a', 'z', 'y' };
std::string numerical_field_to_str(double val) {
  size_t ipref  = 0;
  char   prefix = ' ';

  char res[12];
  if(fabs(val) >= 1.0) {
    while(fabs(val) >= 1000.0 && ipref < sizeof(large_prefix)) {
      val /= 1000.0;
      ++ipref;
    }
    if(ipref == sizeof(large_prefix))
      return std::string(val > 0 ? "+infty" : "-infty");
    prefix = large_prefix[ipref];
    snprintf(res, sizeof(res), "% 5.3g%c", std::min(val, 999.0), prefix);
  } else {
    while(fabs(val) < 0.1 && ipref < sizeof(small_prefix)) {
      val *= 1000.0;
      ++ipref;
    }
    if(ipref == sizeof(small_prefix))
      return std::string(val >= 0 ? "    0 " : "   -0 ");
    prefix = small_prefix[ipref];
    if(fabs(val) < 1.0)
      snprintf(res, sizeof(res), "% 5.2g%c", std::min(val, 999.0), prefix);
    else
      snprintf(res, sizeof(res), "% 5.3g%c", std::min(val, 999.0), prefix);
  }
  return std::string(res);
}

struct time_suffix {
  double seconds;
  char   suffix;
};
static const time_suffix time_suffixes[] =
  { { 1.0, 's' }, { 60.0, 'm' }, { 3600.0, 'h' }, { 86400.0, 'd' }, { 86400.0 * 365.25, 'y' }, { 86400.0 * 365.25 * 10.0, ' ' } };
static const size_t nb_time_suffixes = sizeof(time_suffixes) / sizeof(time_suffix);
std::string seconds_to_str(double seconds) {
  char res[12];
  if(seconds < 1.0) {
    return std::string("  < 1s");
  }
  for(size_t i = 1; i < nb_time_suffixes; ++i) {
    if(seconds < time_suffixes[i].seconds) {
      snprintf(res, sizeof(res), "% 5.3g%c", seconds / time_suffixes[i-1].seconds,
              time_suffixes[i-1].suffix);
      return std::string(res);
    }
  }
  // More than a 100 years!

  return std::string(" > 10y");
}

std::string format_eta(bool writable, off_t size, off_t offset, double speed) {
  if(speed == 0.0 || writable)
    return "   -  ";
  if(speed > 0)
    return seconds_to_str((size - offset) / speed);
  return seconds_to_str(offset / -speed);
}

void print_file_list(const std::vector<file_list>& lists, size_t total_lines, tty_writer& writer) {
  static const int header_width =
    6 /* offset */ + 1 /* slash */ + 6  /* size */ +
    1 /* column */ + 8 /* speed */ + 1  /* column */ +
    6 /* eta */    + 1 /* column */ + 6 /* avg_eta */ + 2 /* spaces */;

  auto session = writer.start_session();
  if(total_lines == 0) {
    auto line = session.start_line();
    line << " --- No regular file open ---";
    return;
  }

  const int window_width = writer.get_window_width();
  std::string prefix;
  for(auto& list : lists) {
    prefix.clear();
    if(lists.size() > 1) {
      prefix += list.source.strid();
      prefix += ':';
    }
    for(auto it = list.begin(); it != list.end(); ++it) {
      auto line = session.start_line();
      // Print offset
      line << numerical_field_to_str(it->offset) << "/";
      // Print file size
      if(it->writable) // Don't display size on writable files
        line << "   -  ";
      else
        line << numerical_field_to_str(it->size);
      // Print speed
      line << ":" << numerical_field_to_str(it->speed) << "/s:";
      // Display ETA
      line << format_eta(it->writable, it->size, it->offset, it->speed)
         << ":"
         << format_eta(it->writable, it->size, it->offset, it->average);

      line << "  ";
      if(!it->updated)
        line << "\033[7m";
      line << shorten_string(prefix + it->name, window_width - header_width);
      if(!it->updated)
        line << "\033[0m";
    }
  }
}
