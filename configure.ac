#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(LCD2004_DAEMON , 1.0, bernard.rodriguez@databang.io)
AC_CONFIG_SRCDIR([mcp3008.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([memset])

AC_OUTPUT
