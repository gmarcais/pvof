#ifndef __TIMESPEC_HPP__
#define __TIMESPEC_HPP__

#include <time.h>

// Arithmetic operations on time specifications.
timespec& operator+=(timespec& x, const timespec& y);
timespec operator+(timespec x, const timespec& y);
timespec& operator+=(timespec& x, const time_t y);
timespec operator+(timespec x, const time_t y);
bool operator<(const timespec&x, const timespec& y);
bool operator==(const timespec& x, const timespec& y);
bool operator!=(const timespec& x, const timespec& y);
timespec& operator-=(timespec& x, const timespec& y);
timespec operator-(timespec x, const timespec& y);
double timespec_double(const timespec x);

#endif
