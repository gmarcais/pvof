#include <src/timespec.hpp>

static const long billion = 1000000000;

timespec& operator+=(timespec& x, const timespec& y) {
  x.tv_sec  += y.tv_sec;
  x.tv_nsec += y.tv_nsec;
  if(x.tv_nsec >= billion) {
    ++x.tv_sec;
    x.tv_nsec -= billion;
  }
  return x;
}

timespec operator+(timespec x, const timespec& y) {
  return x += y;
}

timespec& operator+=(timespec& x, const time_t y) {
  x.tv_sec += y;
  return x;
}

timespec operator+(timespec x, const time_t y) {
  return x += y;
}

bool operator<(const timespec&x, const timespec& y) {
  return (x.tv_sec < y.tv_sec) || (x.tv_sec == y.tv_sec && x.tv_nsec < y.tv_nsec);
}

bool operator==(const timespec& x, const timespec& y) {
  return x.tv_sec == y.tv_sec && x.tv_nsec == y.tv_nsec;
}

bool operator!=(const timespec& x, const timespec& y) {
  return !(x == y);
}

timespec& operator-=(timespec& x, const timespec& y) {
  x.tv_sec -= y.tv_sec;
  x.tv_nsec -= y.tv_nsec;
  if(x.tv_sec < 0) {
    --x.tv_sec;
    x.tv_nsec += billion;
  }
  return x;
}

timespec operator-(timespec x, const timespec& y) {
  return x -= y;
}

double timespec_double(const timespec x) {
  return (double)x.tv_sec + (double)x.tv_nsec / billion;
}
