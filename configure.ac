#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# https://www.gnu.org/software/autoconf/manual/autoconf-2.60/html_node/index.html#Top

AC_PREREQ([2.63])
AC_INIT([ReMPI],
        m4_esyscmd([git describe --always | awk '/.*/ {sub(/^v/, ""); printf "%s",$1; exit}']), 
	[kento@llnl.gov])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
#AM_INIT_AUTOMAKE([ReMPI], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall -Werror], [0.1])
#LT_INIT([shared static])

### Action Macros
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_LIBTOOL

AC_SUBST([LIB_CURRENT], [0])
AC_SUBST([LIB_REVISION], [1])
AC_SUBST([LIB_AGE], [0])

AC_ARG_ENABLE([cdc],
            [AS_HELP_STRING([--enable-cdc],
              [enables CDC (clock delta compression), and output librempix.a and .so])],
            [enable_cdc=yes],
            [])
AM_CONDITIONAL([ENABLE_CDC], [test "$enable_cdc" = yes])

AC_ARG_WITH([bluegene],
            [AS_HELP_STRING([--with-bluegene],
              [build test code with static library])],
            [with_bluegene=yes],
            [with_bluegene=""])
AM_CONDITIONAL([WITH_BLUEGENE], [test "$with_bluegene" = yes])







#### Checks for programs.
AC_LANG_PUSH(C)
LX_FIND_MPI
test "x$have_C_mpi" = xyes || AC_MSG_ERROR([Failed to find C MPI Wrapper.])
AC_LANG_POP()
AC_LANG_PUSH(C++)
LX_FIND_MPI
test "x$have_CXX_mpi" = xyes || AC_MSG_ERROR([Failed to find C++ MPI Wrapper.])
AC_LANG_POP()
AC_LANG_PUSH(Fortran)
LX_FIND_MPI
test "x$have_F_mpi" = xyes || AC_MSG_ERROR([Failed to find Fortran MPI Wrapper.])
AC_LANG_POP()

AC_CHECK_PROGS([MAKE], [make], [:])
if test "$MAKE" = :; then
   AC_MSG_ERROR([This package needs make.])
fi



#### Checks for libraries.

AC_ARG_WITH([zlib-static],
            [AS_HELP_STRING([--with-zlib-static],
              [path to installation directory for libz])],
            [libz_DIR=$withval],
            [libz_DIR=""])

check_zlib="yes"
if test "$libz_DIR" = ""; then
   AC_CHECK_LIB([z], [deflate], [], [check_zlib="no"])
   AC_CHECK_LIB([z], [inflate], [], [check_zlib="no"])
if test "$check_zlib" = "no"; then
   AC_MSG_ERROR([This package needs zlib.])
fi		    
else
   AC_SUBST([libz_a_INCLUDE], [$libz_DIR/include/])
   AC_SUBST([libz_a_LIB], [$libz_DIR/lib/libz.a])
fi

#AC_CHECK_LIB([mpi], [MPI_Init])
mpi_version3="yes"
if test "$enable_cdc" = yes; then
if test "$with_bluegene" = ""; then
AC_CHECK_LIB([mpi], [MPI_Get, PMPI_Get, MPI_Win_lock_all, PMPI_Win_lock_all, MPI_Win_unlock_all, PMPI_Win_unlock_all, MPI_Win_allocate, PMPI_Win_allocate, MPI_Win_free, PMPI_Win_free, MPI_Win_flush_local_all, PMPI_Win_flush_local_all], [], [mpi_version3="no"])

if test "$mpi_version3" = "no"; then
   AC_MSG_ERROR([This package needs MPI3.])
fi		    
fi		    
fi

#### Checks for header files.

if test "$libz_DIR" = :; then
   AC_CHECK_HEADERS([zlib.h])
fi

#### Checks for typedefs, structures, and compiler characteristics.

#### Checks for library functions.

if test "$enable_cdc" = yes; then
AC_ARG_WITH([stack-pmpi],
            [AS_HELP_STRING([--with-stack-pmpi],
              [path to stack_pmpi directory])],
            [stack_pmpi_DIR=$withval],
            [])
if test "$stack_pmpi_DIR" = ""; then
   AC_MSG_ERROR([This package needs stack_pmpi (in STACKP). (Option: --with-stack-pmpi=<path to stack_pmpi directory>)])
fi
AC_SUBST(stack_pmpi_DIR)

AC_ARG_WITH([clmpi],
            [AS_HELP_STRING([--with-clmpi],
              [path to CLMPI directory])],
            [clmpi_DIR=$withval],
            [])
if test "$clmpi_DIR" = ""; then
   AC_MSG_ERROR([This package needs CLMPI library. (Option: --with-clmpi=<path to clmpi directory>)])
fi
AC_SUBST(clmpi_DIR)
fi	



#### Output files.

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile 
	src/Makefile 
	test/Makefile
	example/Makefile
	example/example_x86.sh
	example/examplex_x86.sh])
AC_OUTPUT
