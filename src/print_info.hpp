#ifndef __PRINT_INFO_HPP__
#define __PRINT_INFO_HPP__

#include <ostream>
#include <src/lsof.hpp>

void prepare_display();
void print_file_list(file_list& list, std::ostream& os);

std::string numerical_field_to_str(double val);
std::string seconds_to_str(double seconds);
std::string shorten_string(std::string s, unsigned int length);
#endif
