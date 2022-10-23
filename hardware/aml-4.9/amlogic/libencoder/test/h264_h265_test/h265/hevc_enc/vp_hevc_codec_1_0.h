#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define vl_codec_handle_hevc_t long

typedef enum vl_codec_id_hevc_e {
  CODEC_ID_NONE,
  CODEC_ID_VP8,
  CODEC_ID_H261,
  CODEC_ID_H263,
  CODEC_ID_H264, /* must support */
  CODEC_ID_H265,
} vl_codec_id_hevc_t;

typedef enum vl_img_format_hevc_e {
  IMG_FMT_NONE,
  IMG_FMT_NV12, /* must support  */
  IMG_FMT_NV21,
  IMG_FMT_YV12,
  IMG_FMT_RGB888,
} vl_img_format_hevc_t;

typedef enum vl_frame_type_hevc_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
} vl_frame_type_hevc_t;

typedef enum vl_error_type_hevc_e {
  ERR_HARDWARE = -4,
  ERR_OVERFLOW = -3,
  ERR_NOTSUPPORT = -2,
  ERR_UNDEFINED = -1,
} vl_error_type_hevc_e;

/* buffer type*/
typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} vl_buffer_type_hevc_t;

/* encoder info*/
typedef struct vl_encode_info_hevc {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  bool prepend_spspps_to_idr_frames;
  vl_img_format_hevc_t img_format;
} vl_encode_info_hevc_t;

/* dma buffer info*/
typedef struct vl_dma_info_hevc {
  int shared_fd[3];
  unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vl_dma_info_hevc_t;

typedef union {
  vl_dma_info_hevc_t dma_info;
  unsigned char *in_ptr;
} vl_buf_info_hevc_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct vl_buffer_info_hevc {
  vl_buffer_type_hevc_t buf_type;
  vl_buf_info_hevc_u buf_info;
} vl_buffer_info_hevc_t;


/**
 * Getting version information
 *
 *@return : version information
 */
const char *vl_get_version_hevc();

/**
 * init encoder
 *
 *@param : codec_id: codec type
 *@param : vl_encode_info_t: encode info
 *         width:      video width
 *         height:     video height
 *         frame_rate: framerate
 *         bit_rate:   bitrate
 *         gop GOP:    max I frame interval
 *         prepend_spspps_to_idr_frames: if true, adds spspps header
 *                                       to all idr frames (keyframes).
 *         img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_hevc_t vl_video_encoder_init_hevc(vl_codec_id_hevc_t codec_id,
                                       vl_encode_info_hevc_t encode_info);

/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,HEVC need header(0x00，0x00，0x00，0x01),and format
 *must be I420(apk set param out，through jni,so modify "out" in the
 *function,don't change address point)
 *@param : buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *         buf_info.dma_info: input buf dma info.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
                           For YV12, num_planes can be 1 or 3.
                shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: input buf ptr.
 *@return ：if success return encoded data length,else return error
 */
int vl_video_encoder_encode_hevc(vl_codec_handle_hevc_t handle,
                           vl_frame_type_hevc_t type,
                           unsigned int outputBufferLen,
                           unsigned char *out,
                           vl_buffer_info_hevc_t *buffer_info);

/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_video_encoder_destroy_hevc(vl_codec_handle_hevc_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
