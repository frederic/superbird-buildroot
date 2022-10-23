ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = drm-helper-client
noinst_PROGRAMS = test
noinst_LTLIBRARIES = libjsonipc.la
lib_LTLIBRARIES = libweston-drm-helper.la \
	libdrm-helper-client.la

AM_CFLAGS = -Isrc	\
	-Isrc/ipc	\
	-Isrc/extension	 \
	$(JSONC_CFLAGS)

AM_CPPFLAGS = -Isrc	\
	-Isrc/ipc	\
	-Isrc/extension	 \
	$(JSONC_CFLAGS)

libjsonipc_la_LIBADD = $(JSONC_LIBS)
libjsonipc_la_LDFLAGS = -pthread
libjsonipc_la_SOURCES = src/ipc/ipc.h src/ipc/ipc.c

libweston_drm_helper_la_LDFLAGS = -avoid-version
libweston_drm_helper_la_SOURCES = \
	src/extension/weston-6.0/compositor-drm-help.h \
	src/extension/weston-6.0/compositor-drm-help.c
libweston_drm_helper_la_LIBADD = $(LIBDRM_LIBS) $(JSONC_LIBS) libjsonipc.la
libweston_drm_helper_la_CFLAGS = $(LIBDRM_CFLAGS)

libdrm_helper_client_la_LDFLAGS = -avoid-version
libdrm_helper_client_la_SOURCES = \
	src/extension/weston-6.0/client/drm-help-client.c \
	src/extension/weston-6.0/client/drm-help-client.h
libdrm_helper_client_la_LIBADD = $(JSONC_LIBS) libjsonipc.la
libdrm_helper_client_la_CFLAGS = $(JSONC_CFLAGS)

drm_helper_client_CFLAGS =
drm_helper_client_SOURCES = \
	src/extension/weston-6.0/client/main.c
drm_helper_client_LDADD = libdrm-helper-client.la $(JSONC_LIBS)

test_CFLAGS = -DDEBUG
test_SOURCES = src/ipc/test/test_main.c
test_LDADD = libjsonipc.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	src/extension/weston-6.0/libweston-drm-helper.pc
	src/extension/weston-6.0/client/libdrm-helper-client.pc

weston_drm_helperdir = $(includedir)/weston-drm-helper
weston_drm_helper_HEADERS = \
	src/extension/weston-6.0/compositor-drm-help.h \
	src/extension/weston-6.0/client/drm-help-client.h



