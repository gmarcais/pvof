AM_CXXFLAGS = -Wall -Werror -g -O2 -std=c++0x -I$(top_srcdir)/src
AM_LDFLAGS = -lrt

noinst_HEADERS = $(BUILT_SOURCES)
BUILT_SOURCES = yaggo.cpp yaggo.hpp
CLEANFILES = $(BUILT_SOURCES)

# Yaggo automatic rules with silencing
V_YAGGO = $(V_YAGGO_$(V))
V_YAGGO_ = $(V_YAGGO_$(AM_DEFAULT_VERBOSITY))
V_YAGGO_0 = @echo "  YAGGO " $@;
.yaggo.hpp:
	$(V_YAGGO)$(YAGGO) -o $@ $<
yaggo.cpp yaggo.hpp:
	$(V_YAGGO)$(YAGGO) --lib


##############################
# Programs
##############################
bin_PROGRAMS = pvof
pvof_SOURCES = src/pvof.cc src/pipe_open.cc src/lsof.cc	\
               src/print_info.cc src/timespec.cc
BUILT_SOURCES += src/pvof.hpp

##############################
# Unit tests
##############################
TESTS = all_tests
check_PROGRAMS = all_tests
all_tests_CXXFLAGS = $(AM_CXXFLAGS) -I$(top_srcdir)/unittests
all_tests_LDFLAGS = -lpthread
all_tests_SOURCES = unittests/gtest/gtest-all.cc unittests/gtest/gtest_main.cc unittests/gtest/gtest.h

all_tests_SOURCES += unittests/test_pipe_open.cc src/pipe_open.cc	\
                     unittests/test_lsof.cc src/lsof.cc			\
                     unittests/test_display.cc src/print_info.cc	\
                     src/timespec.cc

##############################
# Testing program
##############################
check_PROGRAMS += slow_cat
slow_cat_SOURCES = tests/slow_cat.cc
