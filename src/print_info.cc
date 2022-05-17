#include <string.h>
#include <math.h>
#include <iostream>

#include <src/tty_writer.hpp>
#include <src/print_info.hpp>
#include <src/pvof.hpp>


std::string shorten_string(const std::string& s, unsigned int length) {
  std::string res;
  if(s.size() <= length) {
    std::string padding(length - s.size(), ' ');
    res = s + padding;
  } else {
    res = "...";
    res += s.substr(s.size() - length + 3, length - 3);
  }
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

void print_file_list(const updater_list_type& updaters,
                     const std::vector<file_list>& lists, const io_info_list& ios, tty_writer& writer) {
  constexpr int header_width =
    6 /* offset */ + 1 /* slash */ + 6  /* size */ +
    1 /* column */ + 8 /* speed */ + 1  /* column */ +
    6 /* eta */    + 1 /* column */ + 6 /* avg_eta */ + 2 /* spaces */;
  constexpr int ioheader_width =
    4 /* CHAR */ + 1 /* space */ + 6 /* rchar */ +
    1 /* pipe */ + 6 /* wchar */ + 1 /* space */ +
    8 /* rspeed */ + 1 /* column */ + 8 /* ravg */ +
    1 /* pipe */ + 8 /* wspeed */ + 1 /* column */ +
    8 /* wavg */ + 4 /* IO */  + 6 /* rchar */ +
    1 /* pipe */ + 6 /* wchar */ + 1 /* space */ +
    8 /* rspeed */ + 1 /* column */ + 8 /* ravg */ +
    1 /* pipe */ + 8 /* wspeed */ + 1 /* column */ +
    8 /* wavg */ + 2 /* spaces */;

  auto        session      = writer.start_session();
  const int   window_width = writer.get_window_width();

  for(size_t i = 0; i < lists.size(); ++i) {
    const auto& io = ios[i];
    const auto& list = lists[i];
    { auto line = session.start_line();
      line << writer.underline
           << "CHAR " << writer.read << numerical_field_to_str(io.char_counter.read) << writer.normal
           << '|' << writer.write << numerical_field_to_str(io.char_counter.write) << writer.normal
           << ' ' << writer.read << numerical_field_to_str(io.char_speed.read) << "/s" << writer.normal
           << ':' << writer.read << numerical_field_to_str(io.char_avg.read) << "/s" << writer.normal
           << '|' << writer.write << numerical_field_to_str(io.char_speed.write) << "/s" << writer.normal
           << ':' << writer.write << numerical_field_to_str(io.char_avg.write) << "/s" << writer.normal
           << " IO " << writer.read << numerical_field_to_str(io.io_counter.read) << writer.normal
           << '|' << writer.write << numerical_field_to_str(io.io_counter.write) << writer.normal
           << ' ' << writer.read << numerical_field_to_str(io.io_speed.read) << "/s" << writer.normal
           << ':' << writer.read << numerical_field_to_str(io.io_avg.read) << "/s" << writer.normal
           << '|' << writer.write << numerical_field_to_str(io.io_speed.write) << "/s" << writer.normal
           << ':' << writer.write << numerical_field_to_str(io.io_avg.write) << "/s" << writer.normal
           << ' ' << shorten_string(updaters[i]->strid(), window_width - ioheader_width)
           << writer.reset;
    }

    for(auto it = list.begin(); it != list.end(); ++it) {
      auto line = session.start_line();
      const char* color = it->writable ? writer.write : writer.read;
      // Print offset
      line << color << numerical_field_to_str(it->offset) << writer.normal << '/';
      // Print file size
      if(it->writable) // Don't display size on writable files
        line << "   -  ";
      else
        line << color << numerical_field_to_str(it->size) << writer.normal;
      // Print speed
      line << ':' << color << numerical_field_to_str(it->speed) << "/s" << writer.normal << ':';
      // Display ETA
      line << format_eta(it->writable, it->size, it->offset, it->speed)
           << ':'
           << format_eta(it->writable, it->size, it->offset, it->average);

      line << ' ';
      if(!it->updated)
        line << writer.reverse;
      line << shorten_string(it->name, window_width - header_width);
      if(!it->updated)
        line << writer.reverse;
    }
  }
}
