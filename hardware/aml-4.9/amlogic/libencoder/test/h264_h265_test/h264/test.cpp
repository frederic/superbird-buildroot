#include "bjunion_enc/vpcodec_1_0.h"
#include "bjunion_enc/include/enc_define.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "test_dma_api.h"

#define ENABLE_VFR 0
//#define LOGAPI(x...) \
//    do { \
//        printf(x); \
//        printf("\n"); \
//    }while(0);
//int main(int argc, const char *argv[]) {
int avc_encode(avc_encoder_params_t * params) {

  int width, height, gop, framerate, bitrate, num;
  int outfd = -1;
  FILE *fp = NULL;
  int datalen = 0;
  vl_img_format_t fmt = IMG_FMT_NONE;
  int ret = 0;
  unsigned char *vaddr = NULL;
  vl_codec_handle_t handle_enc = 0;
  vl_param_runtime_t param_runtime;
  amvenc_frame_stat_t* frame_info = NULL;
  int tmp_idr;
  int fd = -1;
  qp_param_t qp_tbl;
  int qp_mode = 1;
  int idr;
  int buf_type = 0;
  int num_planes = 1;
  int encode_num_success = 0;

  struct usr_ctx_s ctx;
  encode_info_t encode_info;


#if 0
  if (argc < 11) {
    LOGAPI("Amlogic AVC Encode API \n");
    LOGAPI(
        " usage: output "
        "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][buf type][num_planes]\n");
    LOGAPI("  options  :\n");
    LOGAPI("  srcfile  : yuv data url in your root fs\n");
    LOGAPI("  outfile  : stream url in your root fs\n");
    LOGAPI("  width    : width\n");
    LOGAPI("  height   : height\n");
    LOGAPI("  gop      : I frame refresh interval\n");
    LOGAPI("  framerate: framerate \n ");
    LOGAPI("  bitrate  : bit rate \n ");
    LOGAPI("  num      : encode frame count \n ");
    LOGAPI("  fmt      : encode input fmt 1:nv12,2:nv21,3:yv12 4:rgb888 5:bgr888\n ");
    LOGAPI("  buf_type : 0:vmalloc, 3:dma buffer\n ");
    LOGAPI("  num_planes : used for dma buffer case. 1: nv12/nv21/yv12. 2:nv12/nv21. 3:yv12\n ");
    return -1;
  } else {
    LOGAPI("%s\n", argv[1]);
    LOGAPI("%s\n", argv[2]);
  }
#endif

  width = params->width;
  if ((width < 1) || (width > 1920)) {
    LOGAPI("invalid width \n");
    return -1;
  }
  height = params->height;
  if ((height < 1) || (height > 1080)) {
    LOGAPI("invalid height \n");
    return -1;
  }


  gop = params->gop;
  framerate = params->framerate;
  bitrate = params->bitrate;
  num = params->num;
  fmt = (vl_img_format_t)params->format;
  buf_type = params->buf_type;
  num_planes = params->num_planes;
  if ((framerate < 0) || (framerate > 30)) {
    LOGAPI("invalid framerate %d \n",framerate);
    return -1;
  }
  if (bitrate <= 0) {
    LOGAPI("invalid bitrate \n");
    return -1;
  }
  if (num < 0) {
    LOGAPI("invalid num \n");
    return -1;
  }
  if (buf_type == 3 && (num_planes < 1 || num_planes > 3)) {
    LOGAPI("invalid num_planes \n");
    return -1;
  }
  LOGAPI("src_url is: %s ;\n", params->src_file);
  LOGAPI("out_url is: %s ;\n", params->es_file);
  LOGAPI("width   is: %d ;\n", width);
  LOGAPI("height  is: %d ;\n", height);
  LOGAPI("gop     is: %d ;\n", gop);
  LOGAPI("frmrate is: %d ;\n", framerate);
  LOGAPI("bitrate is: %d ;\n", bitrate);
  LOGAPI("frm_num is: %d ;\n", num);
  LOGAPI("fmt is: %d ;\n", fmt);
  LOGAPI("buf_type is: %d ;\n", buf_type);
  LOGAPI("num_planes is: %d ;\n", num_planes);


  unsigned int framesize  = width * height * 3 / 2;
  unsigned ysize  = width * height;
  unsigned usize  = width * height / 4;
  unsigned vsize  = width * height / 4;
  unsigned uvsize  = width * height / 2;
  if (fmt == IMG_FMT_RGB888) {
    framesize = width * height * 3;
  }


  unsigned int outbuffer_len = 1024 * 1024 * sizeof(char);
  unsigned char *outbuffer = (unsigned char *)malloc(outbuffer_len);
  unsigned char *inputBuffer = NULL;
  unsigned char *input[3] = {NULL};
  buffer_info_t inbuf_info;
  dma_info_t *dma_info = NULL;

  memset(&inbuf_info, 0, sizeof(buffer_info_t));
  inbuf_info.buf_type = (buffer_type_t)buf_type;

#if 1
    qp_tbl.qp_min = 0;
    qp_tbl.qp_max = 51;
    qp_tbl.qp_I_base = 30;
    qp_tbl.qp_I_min = 0;
    qp_tbl.qp_I_max = 0;
    qp_tbl.qp_P_base = 30;
    qp_tbl.qp_P_min = 0;
    qp_tbl.qp_P_max = 0;
#else
    qp_tbl.qp_min = 22;
    qp_tbl.qp_max = 40;
    qp_tbl.qp_I_base = 30;
    qp_tbl.qp_I_min = 22;
    qp_tbl.qp_I_max = 35;
    qp_tbl.qp_P_base = 32;
    qp_tbl.qp_P_min = 22;
    qp_tbl.qp_P_max = 40;
#endif
    memset(&param_runtime, 0, sizeof(param_runtime));
    param_runtime.idr = &(idr);
    param_runtime.bitrate = bitrate;
    param_runtime.frame_rate = framerate;
    param_runtime.min_frame_rate = framerate;

#if ENABLE_VFR
    // Only enable variable framerate when bitrate too low.
    if (param_runtime.bitrate > width * height) {
      param_runtime.enable_vfr = false;
      param_runtime.min_frame_rate = framerate;
    } else {
      param_runtime.enable_vfr = true;
      param_runtime.min_frame_rate = 6;
    }
#endif
  encode_info.width = width;
  encode_info.height = height;
  encode_info.bit_rate = bitrate;
  encode_info.frame_rate = framerate;
  encode_info.gop = gop;
  encode_info.img_format = fmt;
  encode_info.qp_mode = qp_mode;
  if (inbuf_info.buf_type == DMA_TYPE) {
    dma_info = &(inbuf_info.buf_info.dma_info);
    dma_info->num_planes = num_planes;
    dma_info->shared_fd[0] = -1; //dma buf fd
    dma_info->shared_fd[1] = -1;
    dma_info->shared_fd[2] = -1;

    ret = create_ctx(&ctx);
    if (ret < 0) {
        LOGAPI("gdc create fail ret=%d\n", ret);
        goto exit;
    }
    if (dma_info->num_planes == 3) {
      if (fmt != IMG_FMT_YV12) {
        LOGAPI("error fmt %d\n", fmt);
        goto exit;
      }
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
      if (ret < 0) {
        LOGAPI("alloc fail ret=%d, len=%u\n", ret, ysize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
        LOGAPI("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;

      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
      if (ret < 0) {
        LOGAPI("alloc fail ret=%d, len=%u\n", ret, usize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[1];
      if (!vaddr) {
        LOGAPI("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[1] = ret;
      input[1] = vaddr;

      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
      if (ret < 0) {
        LOGAPI("alloc fail ret=%d, len=%u\n", ret, vsize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[2];
      if (!vaddr) {
        LOGAPI("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[2] = ret;
      input[2] = vaddr;
    } else if (dma_info->num_planes == 2) {
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
      if (ret < 0) {
        LOGAPI("alloc fail ret=%d, len=%u\n", ret, ysize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
        LOGAPI("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;
      if (fmt != IMG_FMT_NV12 && fmt != IMG_FMT_NV21) {
        LOGAPI("error fmt %d\n", fmt);
        goto exit;
      }
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
      if (ret < 0) {
        LOGAPI("alloc fail ret=%d, len=%u\n", ret, uvsize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[1];
      if (!vaddr) {
        LOGAPI("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[1] = ret;
      input[1] = vaddr;
    } else if(dma_info->num_planes == 1) {
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
      if (ret < 0) {
          LOGAPI("alloc fail ret=%d, len=%u\n", ret, framesize);
          goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
          LOGAPI("mmap failed,Not enough memory\n");
          goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;
    }

    LOGAPI("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0], dma_info->shared_fd[1], dma_info->shared_fd[2]);
    LOGAPI("input[0] %p, input[1] %p,input[2] %p\n", input[0], input[1], input[2]);
  } else {
    inputBuffer = (unsigned char *)malloc(framesize);
    inbuf_info.buf_info.in_ptr = inputBuffer;
  }
  memset(&param_runtime, 0, sizeof(param_runtime));
  param_runtime.idr = &tmp_idr;
  param_runtime.bitrate = bitrate;
  param_runtime.frame_rate = framerate;
  fp = fopen(params->src_file, "rb");
  if (fp == NULL) {
    LOGAPI("open src file error! %s\n", strerror(errno));
    goto exit;
  }
  outfd = open(params->es_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (outfd < 0) {
    LOGAPI("open dest file error! %s\n", strerror(errno));
    goto exit;
  }
  handle_enc =
      vl_video_encoder_init(CODEC_ID_H264, encode_info, &qp_tbl, frame_info);
  while (num > 0) {
    if (inbuf_info.buf_type == DMA_TYPE) { //read data to dma buf vaddr
      if (dma_info->num_planes == 1) {
        if (fread(input[0], 1, framesize, fp) != framesize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
      } else if (dma_info->num_planes == 2) {
        if (fread(input[0], 1, ysize, fp) != ysize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
        if (fread(input[1], 1, uvsize, fp) != uvsize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
      } else if (dma_info->num_planes == 3) {
        if (fread(input[0], 1, ysize, fp) != ysize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
        if (fread(input[1], 1, usize, fp) != usize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
        if (fread(input[2], 1, vsize, fp) != vsize) {
          LOGAPI("read input file error!\n");
          goto exit;
        }
      }
    } else { //read data to vmalloc buf vaddr
      if (fread(inputBuffer, 1, framesize, fp) != framesize) {
        LOGAPI("read input file error!\n");
        goto exit;
      }

    }
    memset(outbuffer, 0, outbuffer_len);
    if (inbuf_info.buf_type == DMA_TYPE) {
      sync_cpu(&ctx);
    }
    datalen = vl_video_encoder_encode(handle_enc, FRAME_TYPE_AUTO, outbuffer,
                                      &inbuf_info, param_runtime);
    LOGAPI("num %d, datalen %d\n",num, datalen);
    if (datalen >= 0) {
        encode_num_success++;
      write(outfd, (unsigned char *)outbuffer, datalen);
    }
    else {
      LOGAPI("encode error %d!\n", datalen);
    }
    num--;
  }
  if (encode_num_success == params->num) {
      params->is_avc_work = true;
  }
  exit:
  if (handle_enc)
    vl_video_encoder_destroy(handle_enc);
  if (outfd >= 0)
    close(outfd);
  if (fp)
    fclose(fp);

  if (outbuffer != NULL)
    free(outbuffer);
  if (inbuf_info.buf_type == DMA_TYPE) {
    if (dma_info->shared_fd[0] >= 0)
      close(dma_info->shared_fd[0]);

    if (dma_info->shared_fd[1] >= 0)
      close(dma_info->shared_fd[1]);

    if (dma_info->shared_fd[2] >= 0)
      close(dma_info->shared_fd[2]);

    destroy_ctx(&ctx);
  } else {
    if (inputBuffer)
      free(inputBuffer);
  }
  LOGAPI("end test!\n");
  return 0;

}
