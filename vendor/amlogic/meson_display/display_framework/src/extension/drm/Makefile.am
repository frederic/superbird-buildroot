bin_PROGRAMS = drm-setcrtc

AM_CFLAGS = $(LIBDRM_CFLAGS)
AM_CPPFLAGS = $(LIBDRM_CFLAGS)

if DEBUG
AM_CFLAGS += -DDEBUG=1
AM_CPPFLAGS += -DDEBUG=1
AM_LDFLAGS = -lpthread
endif

drm_setcrtc_CFLAGS = $(AM_CFLAGS)
drm_setcrtc_SOURCES = drm_setcrtc.c
drm_setcrtc_LDADD =  $(LIBDRM_LIBS)



