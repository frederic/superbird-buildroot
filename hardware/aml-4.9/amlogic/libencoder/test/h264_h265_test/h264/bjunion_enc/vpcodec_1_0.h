#ifndef _INCLUDED_COM_VIDEOPHONE_vpCODEC
#define _INCLUDED_COM_VIDEOPHONE_vpCODEC

#include "include/enc_define.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>

#define vl_codec_handle_t long

typedef enum vl_codec_id_e {
  CODEC_ID_NONE,
  CODEC_ID_VP8,
  CODEC_ID_H261,
  CODEC_ID_H263,
  CODEC_ID_H264, /* must support */
  CODEC_ID_H265,

} vl_codec_id_t;


typedef enum vl_img_format_e {
  IMG_FMT_NONE,
  IMG_FMT_NV12, /* must support  */
  IMG_FMT_NV21,
  IMG_FMT_YV12,
  IMG_FMT_RGB888,
  IMG_FMT_BGR888
} vl_img_format_t;

typedef enum vl_frame_type_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
} vl_frame_type_t;


typedef struct mv_mb_s {
  int16_t mvx;
  int16_t mvy;
} mv_mb_t;
typedef struct mb_coord_s {
  int16_t mb_x;
  int16_t mb_y;
} mb_coord_t;
typedef struct mv_perblock_s {
  mb_coord_t mb_coord;
  mv_mb_t mv[16];
} mv_perblock_t;
typedef struct mv_hist_frame_s {
  int mv_x_hist[400];
  int mv_y_hist[400];
} mv_hist_frame_t;

typedef struct amvenc_frame_stat_s {
  int mv_hist_bias;
  mv_hist_frame_t mv_histogram;
  mv_perblock_t* mv_perblock;
  int qp_avg;
  int qp_min;
  int qp_max;
  int16_t qp_histogram[52];
  int num_intra_prediction_blocks;
  int num_inter_prediction_blocks;
  int num_p_skip_blocks;
  int num_total_macro_blocks;
} amvenc_frame_stat_t;

/* buffer type*/
typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} buffer_type_t;

/* encoder info*/
typedef struct _encode_info {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  vl_img_format_t img_format;
  int qp_mode;
} encode_info_t;

/* dma buffer info*/
/*for nv12/nv21, num_planes can be 1 or 2
  for yv21, num_planes can be 1 or 3
 */
typedef struct _dma_info {
  int shared_fd[3];
  unsigned int num_planes;
} dma_info_t;

/*When the memory type is V4L2_MEMORY_DMABUF, dma_info.shared_fd is a
  file descriptor associated with a DMABUF buffer.
  When the memory type is V4L2_MEMORY_USERPTR, in_ptr is a userspace
  pointer to the memory allocated by an application.
*/
typedef union {
  dma_info_t dma_info;
  unsigned char *in_ptr;
} buf_info_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct _buffer_info {
  buffer_type_t buf_type;
  buf_info_u buf_info;
} buffer_info_t;

/**
 * Getting version information
 *
 *@return : version information
 */
const char *vl_get_version();

typedef struct vl_param_runtime {
  int* idr;
  int bitrate;
  int frame_rate;

  bool enable_vfr;
  int min_frame_rate;
} vl_param_runtime_t;

typedef struct qp_param_s {
  int qp_min;
  int qp_max;
  int qp_I_base;
  int qp_P_base;
  int qp_I_min;
  int qp_I_max;
  int qp_P_min;
  int qp_P_max;
} qp_param_t;

typedef struct avc_encoder_params {
    char src_file[256];
    char es_file[256];
    int width;
    int height;
    int gop;
    int framerate;
    int bitrate;
    int num;
    int format;
    int buf_type;
    int num_planes;
    bool is_avc_work;
} avc_encoder_params_t;

int avc_encode(avc_encoder_params_t * param);

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
 *         img_format: image format
 *@param : qp_tbl: qp_min, qp_max, I/P init/min/max QP
 *@param : frame_info: frame encode info
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id,
                                        encode_info_t encode_info,
                                        qp_param_t* qp_tbl,
                                        amvenc_frame_stat_t* frame_info);

/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : out: data output,H.264 need header(0x00，0x00，0x00，0x01),and format must be I420(apk set param out，through jni,so modify "out" in the function,don't change address point)
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
int vl_video_encoder_encode(vl_codec_handle_t handle,
                            vl_frame_type_t type,
                            unsigned char* out,
                            buffer_info_t* buffer_info,
                            vl_param_runtime_t param_runtime);

/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_video_encoder_destroy(vl_codec_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
