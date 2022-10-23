AUTOMAKE_OPTIONS = subdir-objects
noinst_LTLIBRARIES = libweston-client.la
bin_PROGRAMS = \
			   weston-dmabuf-ion \
			   weston-dmabuf-drm \
			   weston-dmabuf-v4l

BUILT_SOURCES = \
				protocol/linux-dmabuf-unstable-v1-client-protocol.h \
				protocol/linux-dmabuf-unstable-v1-protocol.c \
				protocol/xdg-shell-client-protocol.h \
				protocol/fullscreen-shell-unstable-v1-client-protocol.h \
				protocol/fullscreen-shell-unstable-v1-protocol.c \
				protocol/xdg-shell-protocol.c \
				protocol/xdg-shell-unstable-v6-client-protocol.h \
				protocol/xdg-shell-unstable-v6-protocol.c \
				protocol/linux-explicit-synchronization-unstable-v1-client-protocol.h \
				protocol/linux-explicit-synchronization-unstable-v1-protocol.c

AM_CFLAGS = $(WESTON_SAMPLES_CFLAGS) -I$(srcdir)/protocol
AM_CXXFLAGS = $(WESTON_SAMPLES_CFLAGS)-I$(srcdir)/protocol
AM_LDFLAGS = $(WESTON_SAMPLES_LIBS)

if DEBUG
AM_CFLAGS += -DDEBUG=1
AM_CXXFLAGS += -DDEBUG=1
endif

nodist_libweston_client_la_SOURCES = \
									 protocol/linux-dmabuf-unstable-v1-protocol.c \
									 protocol/xdg-shell-client-protocol.h \
									 protocol/fullscreen-shell-unstable-v1-protocol.c \
									 protocol/xdg-shell-protocol.c \
									 protocol/xdg-shell-unstable-v6-protocol.c \
									 protocol/linux-explicit-synchronization-unstable-v1-protocol.c


weston_dmabuf_drm_SOURCES =	simple-dmabuf-drm.c
weston_dmabuf_drm_LDADD = libweston-client.la $(LIBDRM_LIBS)
weston_dmabuf_drm_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS)

if HAVE_EGL
bin_PROGRAMS += weston-dmabuf-egl-yuv-on-osd

weston_dmabuf_egl_yuv_on_osd_SOURCES = simple-dmabuf-egl-yuv-on-osd.c
weston_dmabuf_egl_yuv_on_osd_LDADD = $(AM_LDFLAGS) libweston-client.la $(EGL_LIBS)
weston_dmabuf_egl_yuv_on_osd_CFLAGS = $(AM_CFLAGS) $(EGL_CFLAGS) $(LIBDRM_CFLAGS)
endif

weston_dmabuf_ion_SOURCES = simple-dmabuf-ion.c
weston_dmabuf_ion_LDADD = libweston-client.la -lamvdec

weston_dmabuf_v4l_SOURCES = simple-dmabuf-v4l.c
weston_dmabuf_v4l_LDADD = libweston-client.la


.SECONDEXPANSION:
# Copy form weston/Makefile.am
define protostability
$(if $(findstring unstable,$1),unstable,stable)
endef

define protoname
$(shell echo $1 | $(SED) 's/\([a-z\-]\+\)-[a-z]\+-v[0-9]\+/\1/')
endef

protocol/%-protocol.c : $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability, $$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) code < $< > $@

protocol/%-client-protocol.h : $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability, $$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) client-header < $< > $@


