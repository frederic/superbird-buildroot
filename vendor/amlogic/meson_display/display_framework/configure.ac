AC_INIT([meson_display_framework], [0.1], [lingjie.li@amlogic.com])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_PROG_CC
AC_PROG_SED
AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_HEADERS([config.h])
AC_SUBST(WARN_CFLAGS)
AC_CONFIG_FILES([
  Makefile
  src/ipc/Makefile
  src/extension/weston-6.0/Makefile
  src/extension/drm/Makefile
  src/samples/weston/Makefile
])
AM_PROG_AR
AC_PROG_LIBTOOL
LT_INIT()
AC_CONFIG_MACRO_DIR([m4])
PKG_PROG_PKG_CONFIG()
AC_CHECK_LIB(json-c, json_tokener_parse,
			 have_json_c=yes, have_json_c=no)
if test "x$have_json_c" = "xyes";then
	PKG_CHECK_MODULES(JSONC, [json-c])
fi

AC_CHECK_LIB(drm, drmOpen,
			 have_libdrm=yes, have_libdrm=no)
if test "x$have_libdrm" = "xyes";then
	PKG_CHECK_MODULES(LIBDRM, [libdrm])
fi

AC_ARG_ENABLE(debug,
			  [  --enable-debug		Turn on debugging], [
if test "x$enableval" = "xyes"; then
	debug=true;
fi
], [debug=false])
AM_CONDITIONAL(DEBUG, test "x$debug" = "xtrue")


AC_ARG_WITH(weston-samples,
			  [  --with-weston-samples    build weston samples], [with_weston_samples=yes], [with_weston_samples=no])
AS_IF([test "x$with_weston_samples" != "xno"],
	  [PKG_CHECK_MODULES(WESTON_SAMPLES, [wayland-client libdrm wayland-protocols wayland-scanner],
						 [ac_wayland_protocols_pkgdatadir=`$PKG_CONFIG --variable=pkgdatadir wayland-protocols`],
						 [with_weston_samples=no && AC_MSG_NOTICE([Weston samples wouldn't be build, no wayalnd-client or libdrm])])],
	  [AC_MSG_NOTICE([Weston samples wouldn't be build])])

AS_IF([test "x$with_weston_samples" != "xno"],
	[
	 AC_SUBST(WAYLAND_PROTOCOLS_DATADIR, $ac_wayland_protocols_pkgdatadir)
	 AC_PATH_PROG([wayland_scanner], [wayland-scanner])
	 if test x$wayland_scanner = x; then
		 wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
	 fi
	 ])

AM_CONDITIONAL(WESTON_SAMPLES, test "x$with_weston_samples" != "xno")

PKG_CHECK_MODULES(EGL, [egl glesv2], [have_egl=yes], [have_egl=no])

AM_CONDITIONAL(HAVE_EGL, test "x$have_egl" != "xno")

AM_CONDITIONAL(ENABLE_WESTON_EXTENTION, test "x$have_json_c" = "xyes" -a "x$have_libdrm" = "xyes")

AM_CONDITIONAL(ENABLE_DRM_FUNCTION, test "x$have_libdrm" = "xyes")
CFLAGS="$CFLAGS -include config.h"

AC_OUTPUT

