#ifndef __PRINT_INFO_HPP__
#define __PRINT_INFO_HPP__

#include <lsof.hpp>

void prepare_display();
void print_file_list(file_list& list);

std::string numerical_field_to_str(double val);
std::string shorten_string(std::string s, unsigned int length);
#endif
