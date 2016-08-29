#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# https://www.gnu.org/software/autoconf/manual/autoconf-2.60/html_node/index.html#Top

AC_PREREQ([2.63])
AC_INIT([ReMPI], [0.0.1], [kento@llnl.gov])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
#AM_INIT_AUTOMAKE([ReMPI], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
#LT_INIT([shared static])

### Action Macros
AC_PROG_LIBTOOL
AC_SUBST([LIB_CURRENT], [0])
AC_SUBST([LIB_REVISION], [0])
AC_SUBST([LIB_AGE], [0])

AC_ARG_ENABLE([cdc],
            [AS_HELP_STRING([--enable-cdc],
              [enables CDC (clock delta compression), and output librempix.a and .so])],
            [enable_cdc=yes],
            [])
AM_CONDITIONAL([ENABLE_CDC], [test "$enable_cdc" = yes])
AC_ARG_WITH([bluegene],
            [AS_HELP_STRING([--with-gluegene],
              [build test codd with static library])],
            [wiht_bluegene=yes],
            [])
AM_CONDITIONAL([WITH_BLUEGENE], [test "$with_bluegene" = yes])

#### Checks for programs.
LX_FIND_MPI # TODO: AC_LANG_CASE([C++], ...) is not executed



AC_CHECK_PROGS([MAKE], [make], [:])
if test "$MAKE" = :; then
   AC_MSG_ERROR([This package needs make.])
fi

#AC_CHECK_PROGS([MPICC], [mpicc], [:])
if test "$MPICC" = :; then
   AC_MSG_ERROR([This package needs mpicc.])
fi
AC_PROG_CC([mpicc])
AM_PROG_CC_C_O([mpicc])
AC_PROG_CXX([mpicc])
AC_SUBST([CC], [$MPICC])
AC_SUBST([CXX], [$MPICC])


AC_CHECK_PROGS([MPIF90], [mpif90], [:])
if test "$MPIF90" = :; then
   AC_MSG_ERROR([This package needs mpif90.])
fi
AC_PROG_F77([mpif90])


#AC_ARG_VAR([CC], [CC for compile])

#### Checks for libraries.

AC_ARG_WITH([zlib_static],
            [AS_HELP_STRING([--with-zlib_static],
              [path to instration directory of libz])],
            [libz_DIR=$withval],
            [])

if test "$libz_DIR" = :; then
   AC_CHECK_LIB([z], [deflate])
   AC_CHECK_LIB([z], [inflate])
else
   AC_SUBST([libz_a_INCLUDE], [$libz_DIR/include/])
   AC_SUBST([libz_a_LIB], [$libz_DIR/lib/libz.a])
fi

#AC_CHECK_LIB([mpi], [MPI_Init])
#AC_CHECK_LIB([mpi], [MPI_Get])

#### Checks for header files.

if test "$libz_DIR" = :; then
   AC_CHECK_HEADERS([zlib.h])
fi

#### Checks for typedefs, structures, and compiler characteristics.

#### Checks for library functions.

#### Output files.

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile 
	src/Makefile 
	src/external/Makefile 
	src/external/clmpi/Makefile 
	test/Makefile
	example/example_x86.sh
	example/examplex_x86.sh])
AC_OUTPUT
