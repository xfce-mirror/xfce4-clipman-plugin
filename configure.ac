dnl
dnl Clipman plugin - Clipboards manager for the Xfce4 panel
dnl
dnl 2005      Eduard Roccatello <eduard@xfce.org>
dnl 2006-2007 Nick Schermer <nick@xfce.org>
dnl 2008-2012 Mike Massonnet <mmassonnet@xfce.org>
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([1.6.7], [git])

dnl minimum required versions
m4_define([glib_min_version], [2.60.0])
m4_define([gtk_min_version], [3.22.29])
m4_define([libxfce4ui_min_version], [4.16.0])
m4_define([libxfce4util_min_version], [4.16.0])
m4_define([libxfce4panel_min_version], [4.16.0])
m4_define([xfconf_minimum_version], [4.16.0])
m4_define([libx11_min_version], [1.6.7])
m4_define([libxproto_min_version], [7.0.0])
m4_define([libxtst_min_version], [1.0.0])
m4_define([wayland_min_version], [1.15.0])
m4_define([libqrencode_min_version], [3.3.0])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2003-2024
        The Xfce development team. All rights reserved.])
AC_INIT([Clipman], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin], [xfce4-clipman-plugin],
        [https://docs.xfce.org/panel-plugins/xfce4-clipman-plugin/start])
AC_CONFIG_HEADERS([config.h])
AC_PREREQ([2.69])
AC_CANONICAL_TARGET()
AC_REVISION([xdt_version_build])
AC_CONFIG_MACRO_DIRS([m4])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 no-dist-gzip dist-bzip2 foreign subdir-objects tar-ustar])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_INSTALL()
AC_PROG_LN_S()
AC_PROG_SED()

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_CHECK_HEADERS([locale.h])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [libxfce4ui_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [libxfce4util_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [libxfce4panel_min_version])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [xfconf_min_version])
XDT_CHECK_PACKAGE_BINARY([GLIB_COMPILE_RESOURCES], [gio-2.0], [glib_compile_resources], [glib-compile-resources])

dnl ***********************************
dnl *** Check for optional packages ***
dnl ***********************************
XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_min_version])
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_min_version])
                             XDT_FEATURE_DEPENDENCY([LIBXPROTO], [xproto], [libxproto_min_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_min_version])
                             XDT_FEATURE_DEPENDENCY([WAYLAND_SCANNER], [wayland-scanner], [wayland_min_version])
                             XDT_FEATURE_DEPENDENCY([WAYLAND_CLIENT], [wayland-client], [wayland_min_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi

if test x"$ENABLE_X11" != x"yes"; then
  dnl Do not check for optional X11 packages if X11 is disabled
  enable_libxtst=no
fi
XDT_CHECK_OPTIONAL_PACKAGE([LIBXTST], [xtst], [libxtst_min_version], [libxtst], [Libxtst support])

if test x"$ENABLE_WAYLAND" = x"yes"; then
  WLR_PROTOCOLS_PKGDATADIR=`$PKG_CONFIG --variable=pkgdatadir wlr-protocols 2>/dev/null || true`
  if test x"$WLR_PROTOCOLS_PKGDATADIR" = x""; then
    WLR_PROTOCOLS_PKGDATADIR="$ac_abs_confdir/protocols/wlr-protocols"
    if test "$(echo "$WLR_PROTOCOLS_PKGDATADIR/"*)" = "$WLR_PROTOCOLS_PKGDATADIR/*"; then
      AC_MSG_ERROR([$WLR_PROTOCOLS_PKGDATADIR seems empty, did you clone the wlr-protocols submodule?])
    fi
  fi
  AC_MSG_CHECKING([for wlr-protocols])
  AC_MSG_RESULT([$WLR_PROTOCOLS_PKGDATADIR])
  AC_SUBST([WLR_PROTOCOLS_PKGDATADIR])
fi

XDT_CHECK_OPTIONAL_PACKAGE([QRENCODE], [libqrencode], [libqrencode_min_version], libqrencode, [QR Code support])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl ************************
dnl *** Makefile outputs ***
dnl ************************
AC_CONFIG_FILES([
Makefile
data/Makefile
data/appdata/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/64x64/Makefile
data/icons/scalable/Makefile
data/its/Makefile
clipboard-manager/Makefile
panel-plugin/Makefile
tests/Makefile
po/Makefile.in
protocols/Makefile
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Debug Support:      $enable_debug"
echo "* X11 Support:        ${ENABLE_X11:-no}"
if test x"$ENABLE_X11" = x"yes"; then
echo "  * Libxtst Support:  ${LIBXTST_FOUND:-no}"
fi
echo "* Wayland Support:    ${ENABLE_WAYLAND:-no}"
echo "* QR Code Support:    ${QRENCODE_FOUND:-no}"
echo
