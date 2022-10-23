/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <aml_ge2d.h>
#include <aml_ipc_internal.h>
#include <jpeglib.h>
#include <math.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc_jpegenc.h"

AML_LOG_DEFINE(jpegencsw);
#define AML_LOG_DEFAULT AML_LOG_GET_CAT(jpegencsw)

AML_OBJ_PROP_INT(PROP_PASSTHROUGH, "pass-through", "pass through jpeg encoder",
                 0, 1, 0);
AML_OBJ_PROP_INT(PROP_QUALITY, "quality", "jpeg encoder quality", 0, 100, 80);

struct AmlIPCJpegEncSWPriv {
  struct AmlIPCGE2D *ge2d;
};

struct aml_jpeg_error_mgr {
  struct jpeg_error_mgr pub; /* "public" fields */

  jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct aml_jpeg_error_mgr *error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
error_exit(j_common_ptr cinfo) {
  /* cinfo->err really points to a aml_jpeg_error_mgr struct, so coerce pointer
   */
  error_ptr err = (error_ptr)cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message)(cinfo);

  /* Return control to the setjmp point */
  longjmp(err->setjmp_buffer, 1);
}

static size_t jpeg_encode(unsigned char *image_buffer, int image_width,
                          int image_height, int quality,
                          unsigned char **outbuf) {
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct aml_jpeg_error_mgr jerr;
  /* More stuff */
  FILE *outfile = NULL;    /* target file */
  JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
  int row_stride;          /* physical row width in image buffer */
  unsigned char *mem = NULL;
  size_t mem_size = 0;

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = error_exit;
  /* Establish the setjmp return context for error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_compress(&cinfo);
    if (outfile) {
      fclose(outfile);
    }
    return 0;
  }
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */
  jpeg_mem_dest(&cinfo, &mem, &mem_size);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; /* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 3;     /* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * 3; /* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
    (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  if (outbuf)
    *outbuf = mem;

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
  return mem_size;
}

static bool jpeg_decode(FILE *fp, int *pwidth, int *pheight, int *pstride,
                        unsigned char *outbuf) {
  bool ret = false;
  if (NULL == fp)
    return ret;

  struct jpeg_decompress_struct cinfo;
  struct aml_jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = error_exit;
  /* Establish the setjmp return context for error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    return ret;
  }
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, fp);

  if (JPEG_HEADER_OK != jpeg_read_header(&cinfo, true)) {
    goto bail;
  }

  int stride = ((cinfo.image_width * cinfo.num_components + 31) >> 5) << 5;
  int align_width = stride / cinfo.num_components;

  if (outbuf == NULL) {
    ret = true;
    goto skip_decompress_exit;
  }

  unsigned char *rdata = outbuf;

  if (!jpeg_start_decompress(&cinfo)) {
    goto bail;
  }

  JSAMPROW row_pointer[1];

  while (cinfo.output_scanline < cinfo.output_height) {
    row_pointer[0] = &rdata[cinfo.output_scanline * stride];
    jpeg_read_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_decompress(&cinfo);

  ret = true;

skip_decompress_exit:

  if (pwidth)
    *pwidth = align_width;
  if (pheight)
    *pheight = cinfo.image_height;
  if (pstride)
    *pstride = stride;

bail:
  jpeg_destroy_decompress(&cinfo);

  return ret;
}

size_t aml_ipc_jpegencsw_encode(unsigned char *imgbuf, int w, int h,
                                int quality, unsigned char **outbuf) {
  return jpeg_encode(imgbuf, w, h, quality, outbuf);
}

bool aml_ipc_jpegencsw_decode_file(char *filename, int *pwidth, int *pheight,
                                   int *pstride, unsigned char *outbuf) {
  if (filename == NULL) {
    return false;
  }
  FILE *fp;
  if ((fp = fopen(filename, "rb")) == NULL) {
    return false;
  }
  bool ret = jpeg_decode(fp, pwidth, pheight, pstride, outbuf);
  fclose(fp);
  return ret;
}

AmlStatus aml_ipc_jpegencsw_set_passthrough(struct AmlIPCJpegEncSW *jpegenc,
                                            bool passthrough) {
  jpegenc->pass_through = passthrough;
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_jpegencsw_set_quality(struct AmlIPCJpegEncSW *jpegenc,
                                        int quality) {
  if (jpegenc->quality != quality) {
    jpegenc->quality = quality;
  }
  return AML_STATUS_OK;
}

AmlStatus aml_ipc_jpegencsw_encode_frame(struct AmlIPCJpegEncSW *enc,
                                         struct AmlIPCVideoFrame *frame,
                                         struct AmlIPCFrame **out) {

  if (enc == NULL || frame == NULL || out == NULL)
    return AML_STATUS_WRONG_PARAM;

  enum AmlPixelFormat pixfmt = frame->format.pixfmt;
  if (pixfmt >= AML_PIXFMT_SDK_MAX) {
    AML_LOGW("pixfmt not support %d", pixfmt);
    return AML_STATUS_FAIL;
  }

  struct AmlIPCVideoFrame *imgproc_frame = NULL;
  // convert video format
  if (pixfmt != AML_PIXFMT_RGB_888) {
    int w, h;
    w = frame->format.width;
    h = frame->format.height;
    struct AmlIPCVideoFormat fmt = {AML_PIXFMT_RGB_888, w, h, 0};
    struct AmlIPCVideoFrame *dst =
        (struct AmlIPCVideoFrame *)aml_ipc_osd_frame_new_alloc(&fmt);
    if (enc->priv->ge2d) {
      if (aml_ipc_ge2d_bitblt(enc->priv->ge2d, frame, dst, NULL, 0, 0) ==
          AML_STATUS_OK) {
        imgproc_frame = dst;
      } else {
        aml_obj_release(AML_OBJECT(dst));
        AML_LOGW("failed to convert video format\n");
        return AML_STATUS_FAIL;
      }
    }
  } else {
    imgproc_frame = frame;
  }

  if (imgproc_frame != NULL) {
    int width = imgproc_frame->format.width;
    int height = imgproc_frame->format.height;
    struct AmlIPCVideoBuffer *dmabuf = imgproc_frame->dmabuf;
    aml_ipc_dma_buffer_map(dmabuf, AML_MAP_READ);
    // copy from uncached dma buffer
    // speedup the sw encoding
    unsigned char *inbuf = (unsigned char *)malloc(dmabuf->size);
    memcpy(inbuf, dmabuf->data, dmabuf->size);
    aml_ipc_dma_buffer_unmap(dmabuf);

    unsigned char *outbuf = NULL;
    size_t size = jpeg_encode(inbuf, width, height, enc->quality, &outbuf);
    free(inbuf);

    if (size == 0) {
      if (outbuf != NULL)
        free(outbuf);
      AML_LOGW("error encoding");
      return AML_STATUS_FAIL;
    }

    struct AmlIPCFrame *frm = AML_OBJ_NEW(AmlIPCFrame);
    frm->size = size;
    frm->data = malloc(frm->size);
    frm->pts_us = AML_IPC_FRAME(imgproc_frame)->pts_us;
    memcpy(frm->data, outbuf, size);
    *out = frm;

    if (outbuf != NULL)
      free(outbuf);
    if (imgproc_frame != frame)
      aml_obj_release(AML_OBJECT(imgproc_frame));

    return AML_STATUS_OK;
  }

  AML_LOGW("imgproc frame is empty");
  return AML_STATUS_FAIL;
}

static AmlStatus handle_frame(struct AmlIPCComponent *comp, int ch,
                              struct AmlIPCFrame *frame) {
  AML_SCTIME_LOGD("frame %dx%d", AML_IPC_VIDEO_FRAME(frame)->format.width,
                  AML_IPC_VIDEO_FRAME(frame)->format.height);

  struct AmlIPCJpegEncSW *jpegenc = (struct AmlIPCJpegEncSW *)comp;
  struct AmlIPCVideoFrame *vfrm = (struct AmlIPCVideoFrame *)frame;

  // by pass
  if (jpegenc->pass_through) {
    struct AmlIPCFrame *frm = AML_OBJ_NEW(AmlIPCFrame);
    frm->size = 0;
    frm->data = NULL;
    frm->pts_us = 0;
    return aml_ipc_send_frame(comp, AML_JPEGENC_OUTPUT, frm);
  }

  // use jpeg software encoder to encode
  struct AmlIPCFrame *outframe = NULL;
  AmlStatus status = aml_ipc_jpegencsw_encode_frame(jpegenc, vfrm, &outframe);
  aml_obj_release(AML_OBJECT(vfrm));
  if (status == AML_STATUS_OK && outframe != NULL) {
    return aml_ipc_send_frame(AML_IPC_COMPONENT(jpegenc), AML_JPEGENC_OUTPUT,
                              outframe);
  }

  return AML_STATUS_FAIL;
}

static void jpegenc_init(struct AmlIPCJpegEncSW *jpegenc) {
  AML_ALLOC_PRIV(jpegenc->priv);
  jpegenc->quality = 80;
  jpegenc->priv->ge2d = aml_ipc_ge2d_new(AML_GE2D_OP_BITBLT_TO);

  aml_ipc_add_channel(AML_IPC_COMPONENT(jpegenc), AML_JPEGENCSW_INPUT,
                      AML_CHANNEL_INPUT, "input");
  aml_ipc_add_channel(AML_IPC_COMPONENT(jpegenc), AML_JPEGENCSW_OUTPUT,
                      AML_CHANNEL_OUTPUT, "output");
}

static void jpegenc_free(struct AmlIPCJpegEncSW *jpegenc) {
  if (jpegenc->priv->ge2d != NULL)
    aml_obj_release(AML_OBJECT(jpegenc->priv->ge2d));
  free(jpegenc->priv);
  jpegenc->priv = NULL;
}

static AmlStatus jpegenc_set_property(struct AmlObject *obj,
                                      struct AmlPropertyInfo *prop, void *val) {
  if (prop == PROP_PASSTHROUGH)
    return aml_ipc_jpegencsw_set_passthrough((struct AmlIPCJpegEncSW *)obj,
                                             *(int *)val);
  if (prop == PROP_QUALITY)
    return aml_ipc_jpegencsw_set_quality((struct AmlIPCJpegEncSW *)obj,
                                         *(int *)val);

  return ((struct AmlObjectClass *)AML_OBJ_PARENT_CLASS(obj))
      ->set_property(obj, prop, val);
}

static void jpegenc_class_init(struct AmlIPCComponentClass *cls) {
  struct AmlObjectClass *objcls = (struct AmlObjectClass *)cls;

  aml_obj_add_properties(
      objcls, PROP_PASSTHROUGH, offsetof(struct AmlIPCJpegEncSW, pass_through),
      PROP_QUALITY, offsetof(struct AmlIPCJpegEncSW, quality), NULL);

  cls->handle_frame = handle_frame;
  objcls->set_property = jpegenc_set_property;
}

AML_OBJ_DEFINE_TYPEID(AmlIPCJpegEncSW, AmlIPCComponent, AmlIPCComponentClass,
                      jpegenc_class_init, jpegenc_init, jpegenc_free);
