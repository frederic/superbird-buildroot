ACLOCAL_AMFLAGS = -I m4
SUBDIRS =
if ENABLE_DRM_FUNCTION
SUBDIRS += src/extension/drm
if ENABLE_WESTON_EXTENTION
SUBDIRS += src/ipc \
		   src/extension/weston-6.0
endif
if WESTON_SAMPLES
SUBDIRS += src/samples/weston
endif

endif
