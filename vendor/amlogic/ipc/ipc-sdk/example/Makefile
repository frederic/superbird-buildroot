#ISP/GE2D/VENC/RAWVIDEOPARSE related
EXAMPLE_TARGETS =\
				 isp_dump \
				 isp_enc_dump \
				 isp_gdc_dump \
				 isp_ge2d_dump \
				 isp_osd_bufferlist_dump \
				 isp_osd_dump \
                                 filein_rawvideoparse_venc_dump \
				 $(NULL)

#pipeline/component related
EXAMPLE_TARGETS +=\
				  bind_multi \
				  bind_multi_pipeline \
				  pipeline_restart \
				  $(NULL)

#GST/NN/System related
EXAMPLE_TARGETS +=\
				  isp_nn_webstream \
				  ipc_gstbridge.so \
				  ipc_launch \
				  ipc_stream \
				  $(NULL)

#Audio related
EXAMPLE_TARGETS +=\
				  ain_aconv_aenc_dump \
				  ain_aenc_dump \
				  ain_aout_dump \
				  ain_pcm_dump \
				  play_aac \
				  push_frame_aout \
				  $(NULL)

isp_nn_webstream_CFLAGS = $(shell $(PKG_CONFIG) --cflags gstreamer-1.0 gstreamer-app-1.0 libwebsockets)
isp_nn_webstream_LDFLAGS = $(shell $(PKG_CONFIG) --libs gstreamer-1.0 gstreamer-app-1.0 libwebsockets)
isp_nn_webstream_LDFLAGS += -lnn_detect
ipc_gstbridge_LDFLAGS = $(shell $(PKG_CONFIG) --libs gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0)
ipc_gstbridge_CFLAGS = $(shell $(PKG_CONFIG) --cflags gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0)
ipc_gstbridge_LDLIBS = -shared
ipc_stream_LDFLAGS += $(shell $(PKG_CONFIG) --libs gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0)
ipc_stream_CFLAGS += $(shell $(PKG_CONFIG) --cflags gstreamer-1.0 gstreamer-app-1.0 libwebsockets gstreamer-rtsp-server-1.0)
ipc_launch_LDLIBS = -ldl

all:
	@for f in $(EXAMPLE_TARGETS); do ($(MAKE) $$f) || exit 1; done

ifneq "$(findstring $(MAKECMDGOALS),$(EXAMPLE_TARGETS))"  ""
TARGET := $(MAKECMDGOALS)
TARGET_STEM := $(basename $(TARGET))
LDLIBS += -laml_ipc_sdk
SRCS = $(TARGET_STEM:%=%.c)
ifdef $(TARGET_STEM)_LDFLAGS
LDLIBS += $($(TARGET_STEM)_LDFLAGS)
endif
ifdef $(TARGET_STEM)_CFLAGS
CFLAGS += $($(TARGET_STEM)_CFLAGS)
endif
ifdef $(TARGET_STEM)_LDLIBS
LDLIBS += $($(TARGET_STEM)_LDLIBS)
endif
include ../makefile.inc
endif

# reuse clean target in ../makefile.inc
ifeq ($(MAKECMDGOALS),clean)
TARGET = $(EXAMPLE_TARGETS)
include ../makefile.inc
endif

