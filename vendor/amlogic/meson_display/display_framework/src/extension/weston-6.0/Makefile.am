AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = drm-helper-client
lib_LTLIBRARIES = libweston-drm-helper.la \
	libdrm-helper-client.la

AM_CFLAGS = -I$(top_srcdir)/src $(JSONC_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/src $(JSONC_CFLAGS)

if DEBUG
AM_CFLAGS += -DDEBUG=1
AM_CPPFLAGS += -DDEBUG=1
endif

libweston_drm_helper_la_LDFLAGS = -avoid-version
libweston_drm_helper_la_SOURCES = compositor-drm-help.h \
								  compositor-drm-help.c

libweston_drm_helper_la_LIBADD = $(LIBDRM_LIBS) $(JSONC_LIBS) ../../ipc/libjsonipc.la
libweston_drm_helper_la_CFLAGS = $(LIBDRM_CFLAGS)


libdrm_helper_client_la_LDFLAGS = -avoid-version

libdrm_helper_client_la_SOURCES = client/drm-help-client.c \
								  client/drm-help-client.h

libdrm_helper_client_la_LIBADD = $(JSONC_LIBS) ../../ipc/libjsonipc.la

drm_helper_client_CFLAGS =
drm_helper_client_SOURCES = client/main.c

drm_helper_client_LDADD = libdrm-helper-client.la $(JSONC_LIBS)

weston_drm_helperdir = $(includedir)/weston-drm-helper

weston_drm_helper_HEADERS = compositor-drm-help.h \
							client/drm-help-client.h

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libweston-drm-helper.pc \
				 client/libdrm-helper-client.pc



