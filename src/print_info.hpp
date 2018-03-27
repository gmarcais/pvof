#ifndef __PRINT_INFO_HPP__
#define __PRINT_INFO_HPP__

#include <ostream>
#include <src/lsof.hpp>
#include <src/tty_writer.hpp>

void prepare_display();
void print_file_list(const std::vector<file_list>& lists, const io_info_list& ios, tty_writer& writer);

std::string numerical_field_to_str(double val);
std::string seconds_to_str(double seconds);
std::string shorten_string(std::string s, unsigned int length);
#endif
