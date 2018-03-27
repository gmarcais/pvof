AC_PREREQ([2.63])
AC_INIT([pvof], [0.0.9], [gmarcais@cs.cmu.edu])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/pvof.cc]) 
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
# Uncomment for creating libraries
# AC_PROG_LIBTOOL


# Checks for programs.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Check for yaggo
AC_ARG_VAR([YAGGO], [Yaggo switch parser generator])
AS_IF([test "x$YAGGO" = "x"], [AC_PATH_PROG([YAGGO], [yaggo], [false])])

# Check if support /proc/<pid>/fdinfo
AC_ARG_ENABLE([proc], [AS_HELP_STRING([--enable-proc], [Use /proc/<pid>/fdinfo directly, instead of lsof])])
AS_IF([test "x$enable_proc" = "x"],
      [AC_CHECK_FILES([/proc/self/fdinfo /proc/self/fd], [enable_proc=yes])])
AS_IF([test "x$enable_proc" = "xyes"], [AC_DEFINE([HAVE_PROC], [1], [Use fd information in proc])])

# lsof to use
AC_ARG_VAR([LSOF], [Path to lsof])
AS_IF([test "x$LSOF" = "x"], [AC_PATH_PROG([LSOF], [lsof])])
AC_DEFINE_UNQUOTED([LSOF], "$LSOF", [Path to lsof software])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([])

AC_OUTPUT([Makefile])
