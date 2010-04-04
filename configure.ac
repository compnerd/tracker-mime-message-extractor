dnl Copyright 2010 Saleem Abdulrasool <compnerd@compnerd.org>
dnl vim: set sw=4 sts=4 et fdm=marker fmr={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT(tracker-mime-message-extractor, 0.0.1, compnerd@compnerd.org)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(m4)
AM_CONFIG_HEADER(config/config.h)
AM_INIT_AUTOMAKE([1.11.1 subdir-objects dist-bzip2])
dnl }}}

dnl {{{ basic toolchain checks
AC_PROG_CC_C99
AC_PROG_CC_C_O
AC_PROG_INSTALL

LT_INIT([disable-static])
dnl }}}

dnl {{{ library checks
GMIME_REQUIRED=2.5.1
TRACKER_REQUIRED=0.8

TRACKER_API_VERSION=0.8
AC_SUBST(TRACKER_API_VERSION)

PKG_CHECK_MODULES(GMIME, [gmime-2.6 >= $GMIME_REQUIRED])
PKG_CHECK_MODULES(TRACKER, [tracker-miner-$TRACKER_API_VERSION >= $TRACKER_REQUIRED
                            tracker-client-$TRACKER_API_VERSION >= $TRACKER_REQUIRED])
dnl }}}

dnl {{{ output
AM_SILENT_RULES(yes)
AC_OUTPUT(Makefile)
dnl }}}

