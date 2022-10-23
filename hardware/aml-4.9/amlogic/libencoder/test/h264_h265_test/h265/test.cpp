#include "hevc_enc/vp_hevc_codec_1_0.h"
#include "hevc_enc/include/vdi.h"

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
#include "test.h"

int hevc_encode(hevc_encoder_params_t * params) {
  int width, height, gop, framerate, bitrate, num;
  int outfd = -1;
  FILE *fp = NULL;
  int datalen = 0;
  vl_img_format_hevc_t fmt = IMG_FMT_NONE;
  int ret = 0;
  unsigned char *vaddr = NULL;
  vl_codec_handle_hevc_t handle_enc = 0;
  int buf_type = 0;
  int num_planes = 1;
  int encode_num_success = 0;

  struct usr_ctx_s ctx;
  vl_encode_info_hevc_t encode_info;

#if 0
  if (argc < 11) {
    VLOG("Amlogic AVC Encode API \n");
    VLOG(
        " usage: output "
        "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][buf type][num_planes]\n");
    VLOG("  options  :\n");
    VLOG("  srcfile  : yuv data url in your root fs\n");
    VLOG("  outfile  : stream url in your root fs\n");
    VLOG("  width    : width\n");
    VLOG("  height   : height\n");
    VLOG("  gop      : I frame refresh interval\n");
    VLOG("  framerate: framerate \n ");
    VLOG("  bitrate  : bit rate \n ");
    VLOG("  num      : encode frame count \n ");
    VLOG("  fmt      : encode input fmt 1:nv12, 2:nv21, 3:yv12, 4:RGB888\n ");
    VLOG("  buf_type : 0:vmalloc, 3:dma buffer\n ");
    VLOG("  num_planes : used for dma buffer case. 1: nv12/nv21/yv12. 2:nv12/nv21. 3:yv12\n ");
    return -1;
  } else {
    VLOG("%s\n", argv[1]);
    VLOG("%s\n", argv[2]);
  }
#endif

  width = params->width;
  if ((width < 1) || (width > 3840/*1920*/)) {
    VLOG("invalid width \n");
    return -1;
  }
  height = params->height;
  if ((height < 1) || (height > 2160/*1080*/)) {
    VLOG("invalid height \n");
    return -1;
  }
  gop = params->gop;
  framerate = params->framerate;
  bitrate = params->bitrate;
  num = params->num;
  fmt = (vl_img_format_hevc_t)params->format;
  buf_type = params->buf_type;
  num_planes = params->num_planes;
  if ((framerate < 0) || (framerate > 30)) {
    VLOG("invalid framerate %d \n",framerate);
    return -1;
  }
  if (bitrate <= 0) {
    VLOG("invalid bitrate \n");
    return -1;
  }
  if (num < 0) {
    VLOG("invalid num \n");
    return -1;
  }
  if (buf_type == 3 && (num_planes < 1 || num_planes > 3)) {
    VLOG("invalid num_planes \n");
    return -1;
  }
  VLOG("src_url is: %s ;\n", params->src_file);
  VLOG("out_url is: %s ;\n", params->es_file);
  VLOG("width   is: %d ;\n", width);
  VLOG("height  is: %d ;\n", height);
  VLOG("gop     is: %d ;\n", gop);
  VLOG("frmrate is: %d ;\n", framerate);
  VLOG("bitrate is: %d ;\n", bitrate);
  VLOG("frm_num is: %d ;\n", num);
  VLOG("fmt is: %d ;\n", fmt);
  VLOG("buf_type is: %d ;\n", buf_type);
  VLOG("num_planes is: %d ;\n", num_planes);


  encode_info.width = width;
  encode_info.height = height;
  encode_info.bit_rate = bitrate;
  encode_info.frame_rate = framerate;
  encode_info.gop = gop;
  encode_info.img_format = fmt;
  encode_info.prepend_spspps_to_idr_frames = false;

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
  vl_buffer_info_hevc_t inbuf_info;
  vl_dma_info_hevc_t *dma_info = NULL;

  memset(&inbuf_info, 0, sizeof(vl_buffer_info_hevc_t));
  inbuf_info.buf_type = (vl_buffer_type_hevc_t)buf_type;

  if (inbuf_info.buf_type == DMA_TYPE) {
    dma_info = &(inbuf_info.buf_info.dma_info);
    dma_info->num_planes = num_planes;
    dma_info->shared_fd[0] = -1; //dma buf fd
    dma_info->shared_fd[1] = -1;
    dma_info->shared_fd[2] = -1;

    ret = create_ctx(&ctx);
    if (ret < 0) {
        VLOG("gdc create fail ret=%d\n", ret);
        goto exit;
    }
    if (dma_info->num_planes == 3) {
      if (fmt != IMG_FMT_YV12) {
        VLOG("error fmt %d\n", fmt);
        goto exit;
      }
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
      if (ret < 0) {
        VLOG("alloc fail ret=%d, len=%u\n", ret, ysize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
        VLOG("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;

      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
      if (ret < 0) {
        VLOG("alloc fail ret=%d, len=%u\n", ret, usize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[1];
      if (!vaddr) {
        VLOG("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[1] = ret;
      input[1] = vaddr;

      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
      if (ret < 0) {
        VLOG("alloc fail ret=%d, len=%u\n", ret, vsize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[2];
      if (!vaddr) {
        VLOG("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[2] = ret;
      input[2] = vaddr;
    } else if (dma_info->num_planes == 2) {
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
      if (ret < 0) {
        VLOG("alloc fail ret=%d, len=%u\n", ret, ysize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
        VLOG("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;
      if (fmt != IMG_FMT_NV12 && fmt != IMG_FMT_NV21) {
        VLOG("error fmt %d\n", fmt);
        goto exit;
      }
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
      if (ret < 0) {
        VLOG("alloc fail ret=%d, len=%u\n", ret, uvsize);
        goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[1];
      if (!vaddr) {
        VLOG("mmap failed,Not enough memory\n");
        goto exit;
      }
      dma_info->shared_fd[1] = ret;
      input[1] = vaddr;
    } else if(dma_info->num_planes == 1) {
      ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
      if (ret < 0) {
          VLOG("alloc fail ret=%d, len=%u\n", ret, framesize);
          goto exit;
      }
      vaddr = (unsigned char *)ctx.i_buff[0];
      if (!vaddr) {
          VLOG("mmap failed,Not enough memory\n");
          goto exit;
      }
      dma_info->shared_fd[0] = ret;
      input[0] = vaddr;
    }
    VLOG("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0], dma_info->shared_fd[1], dma_info->shared_fd[2]);
    VLOG("input[0] %p, input[1] %p,input[2] %p\n", input[0], input[1], input[2]);
  } else {
    inputBuffer = (unsigned char *)malloc(framesize);
    inbuf_info.buf_info.in_ptr = inputBuffer;
  }

  fp = fopen(params->src_file, "rb");
  if (fp == NULL) {
    VLOG("open src file error!\n");
    goto exit;
  }

  outfd = open(params->es_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (outfd < 0) {
    VLOG("open dest file error!\n");
    goto exit;
  }
  handle_enc = vl_video_encoder_init_hevc(CODEC_ID_H265, encode_info);
  while (num > 0) {
    if (inbuf_info.buf_type == DMA_TYPE) { //read data to dma buf vaddr
      if (dma_info->num_planes == 1) {
        if (fread(input[0], 1, framesize, fp) != framesize) {
          VLOG("read input file error!\n");
          goto exit;
        }
      } else if (dma_info->num_planes == 2) {
        if (fread(input[0], 1, ysize, fp) != ysize) {
          VLOG("read input file error!\n");
          goto exit;
        }
        if (fread(input[1], 1, uvsize, fp) != uvsize) {
          VLOG("read input file error!\n");
          goto exit;
        }
      } else if (dma_info->num_planes == 3) {
        if (fread(input[0], 1, ysize, fp) != ysize) {
          VLOG("read input file error!\n");
          goto exit;
        }
        if (fread(input[1], 1, usize, fp) != usize) {
          VLOG("read input file error!\n");
          goto exit;
        }
        if (fread(input[2], 1, vsize, fp) != vsize) {
          VLOG("read input file error!\n");
          goto exit;
        }
      }
    } else { //read data to vmalloc buf vaddr
      if (fread(inputBuffer, 1, framesize, fp) != framesize) {
        VLOG("read input file error!\n");
        goto exit;
      }
    }
    memset(outbuffer, 0, outbuffer_len);
    if (inbuf_info.buf_type == DMA_TYPE) {
      sync_cpu(&ctx);
    }
    datalen = vl_video_encoder_encode_hevc(handle_enc, FRAME_TYPE_AUTO, outbuffer_len, outbuffer, &inbuf_info);

    VLOG("num %d, datalen %d\n",num, datalen);
    if (datalen >= 0) {
        encode_num_success++;
      write(outfd, (unsigned char *)outbuffer, datalen);
    }
    else {
      VLOG("encode error %d!\n", datalen);
    }
    num--;
  }

  if (encode_num_success == params->num) {
      params->is_hevc_work = true;
  }
  exit:
  if (handle_enc)
    vl_video_encoder_destroy_hevc(handle_enc);
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
  VLOG("end test!\n");
  return 0;
}
