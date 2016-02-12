AC_INIT([libvmod-i18n], [0.0.1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR(src/vmod.vcc)
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([foreign color-tests parallel-tests])

AC_DISABLE_STATIC

AC_PROG_CC
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

AC_PATH_PROGS([PYTHON], [python3 python2.7 python2.6])
test -z "$PYTHON" && AC_MSG_ERROR([Python 2.6 or greater is required.])

AC_PATH_PROG([RST2MAN], [rst2man])
test -z "$RST2MAN" && AC_MSG_ERROR([rst2man is required.])

AC_ARG_VAR([VARNISHSRC], [path to Varnish source tree (mandatory)])
test -z "$VARNISHSRC" && AC_MSG_ERROR([No Varnish source tree specified])

VARNISHSRC=`cd $VARNISHSRC && pwd`
AC_CHECK_FILE([$VARNISHSRC/include/varnishapi.h], [], [
  AC_MSG_ERROR(["$VARNISHSRC" is not a Varnish source directory.])
])

AC_CHECK_FILE([$VARNISHSRC/bin/varnishtest/varnishtest], [], [
  AC_MSG_ERROR([Cannot find "$VARNISHSRC/bin/varnishtest/varnishtest".])
])

AC_ARG_VAR([VMODDIR], [vmod installation directory @<:@LIBDIR/varnish/vmods@:>@])
if test -z "$VMODDIR"; then
  VMODDIR=`pkg-config --variable=vmoddir varnishapi`
  test -z "$VMODDIR" && AC_MSG_ERROR([Cannot determine vmod installation directory.])
fi

VMOD_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VMOD_TESTS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
