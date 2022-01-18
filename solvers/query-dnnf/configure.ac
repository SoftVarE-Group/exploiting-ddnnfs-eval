AC_INIT([query-dnnf], [0.4.180625])
AM_INIT_AUTOMAKE([1.13])

AC_CONFIG_MACRO_DIR([local.m4.d])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_HEADERS([config.h])

: ${CXXFLAGS=-Wno-terminate -O3}
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AX_CXX_COMPILE_STDCXX_14

AC_CHECK_LIB(gmp, __gmpz_init, , [AC_MSG_ERROR([GNU MP not found, see https://gmplib.org/])])
AC_CHECK_LIB(gmpxx, __gmpq_init, , [AC_MSG_ERROR([GNU MP C++ interface not found, see https://gmplib.org/])])

AC_OUTPUT
