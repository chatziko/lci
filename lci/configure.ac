#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lci, 0.5, hatz-at-users.sourceforge.net)
AC_CONFIG_SRCDIR([src/run.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([readline], [readline],, AC_MSG_WARN(readline not found. command history will be disabled.))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/ioctl.h termio.h unistd.h])
AC_CHECK_HEADERS([readline/readline.h readline/history.h])

# If readline is available set USE_READLINE.
# Otherwise if all headers needed for ioctl exist set USE_IOCTL
if test "$ac_cv_lib_readline_readline" == yes -a \
		  "$ac_cv_header_readline_readline_h" == yes -a \
		  "$ac_cv_header_readline_history_h" == yes
then
	AC_DEFINE(USE_READLINE, 1, [use readline for input])
	
	# check if readline provides history_truncate_file (it might be missing even if readline
	# exists if using editline instead of readline, eg on MacOSX)
	AC_CHECK_LIB([readline], [history_truncate_file], AC_DEFINE(HAVE_HISTORY_TRUNCATE_FILE, 1, [readline provides history_truncate_file function]), \
				 AC_MSG_WARN([readline found but some functions are not present (are you using editline instead?).	I'll disable these functions]))

elif test "$ac_cv_header_sys_ioctl_h" == yes -a \
			 "$ac_cv_header_unistd_h" == yes -a \
			 "$ac_cv_header_termio_h" == yes
then
	AC_DEFINE(USE_IOCTL, 1, [use ioctl for terminal manipulation (used only if readline is not available)])
fi

# Checks for typedefs, structures, and compiler characteristics.
# check for __attribute__((packed))
AC_MSG_CHECKING([whether C compiler supports __attribute__((packed))])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(, [[struct a { int b; } __attribute__((packed));]])],
                  [gcc_have_attr_packed=yes], [gcc_have_attr_packed=no])
AC_SUBST(gcc_have_attr_packed)
if test "$gcc_have_attr_packed" = "yes"; then
  AC_DEFINE(HAVE_ATTR_PACKED, 1, [Define to 1 if C compiler supports __attribute__((packed))])
fi
AC_MSG_RESULT([$gcc_have_attr_packed])

# --enable-debug
AC_ARG_ENABLE(debug, [  --enable-debug  Enables debuggin compilation options],,)
if test "$enable_debug" != yes; then
	AC_DEFINE(NDEBUG, 1, [Define to disable assertions])
fi

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([strdup])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])
AC_OUTPUT
