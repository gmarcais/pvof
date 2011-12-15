AM_CXXFLAGS = -Wall -Werror -g -O2 -I$(top_srcdir)/src

##############################
# Programs
##############################
bin_PROGRAMS = test
test_SOURCES = src/test.cc
noinst_HEADERS = src/test.hpp

##############################
# Unit tests
##############################
TESTS = all_tests
check_PROGRAMS = all_tests
all_tests_CXXFLAGS = $(AM_CXXFLAGS) -I$(top_srcdir)/unittests
all_tests_LDFLAGS = -lpthread
all_tests_SOURCES = unittests/gtest/gtest-all.cc unittests/gtest/gtest_main.cc unittests/gtest/gtest.h

all_tests_SOURCES += unittests/test_trivial.cc
