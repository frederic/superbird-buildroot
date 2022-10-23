/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
libge2d user guide
API:
int ge2d_open(void);
int ge2d_close(int fd);
int ge2d_process(int fd,aml_ge2d_info_t *pge2dinfo);


typedef struct aml_ge2d_info {
    unsigned int offset;
    unsigned int blend_mode;
    GE2DOP ge2d_op;
    buffer_info_t src_info[2];
    buffer_info_t dst_info;
    unsigned int color;
    unsigned int gl_alpha;
} aml_ge2d_info_t;

some bits of struct aml_ge2d_info:
unsigned int offset:        y_offset of osd
unsigned int blend_mode:    only valid when work under blend operation
unsigned int color:         only valid when fillrectangle operation
GE2DOP ge2d_op:             supported ge2d operation
supported ge2d operation include:
    AML_GE2D_FILLRECTANGLE,
    AML_GE2D_BLEND,
    AML_GE2D_STRETCHBLIT,
    AML_GE2D_BLIT,

typedef struct buffer_info {
    unsigned int memtype;
    char* vaddr;
    unsigned long paddr;
    unsigned int canvas_w;
    unsigned int canvas_h;
    rectangle_t rect;
    int format;
    unsigned int rotation;
} buffer_info_t;
some bits of struct buffer_info:
memtype:                if used as mem alloc:CANVAS_ALLOC
                        if used as OSD0/OSD1:CANVAS_OSD0/CANVAS_OSD1
char* vaddr:            not need set,for debug
unsigned long paddr:    if used as mem alloc,it is mem phy addr
                        if used as OSD0/OSD1,leave it unset,it will set by kernel;
unsigned int canvas_w,
unsigned int canvas_h:  if used as mem alloc,for src, set it to rect.w, rect.h;
                                             for dst, set it to canvas width,canvas height,
                                             related to mem size.
                        if used as OSD0/OSD1,leave it unset,it will set by kernel;
int format:             if used as mem alloc,set pixel format
                        if used as OSD0/OSD1,leave it unset,it will set by kernel;
rectangle_t rect:       must be set,set it according real rect value.
unsigned int rotation:  it can be set GE2D_ROTATION,rotation 0/90/180/270;

1.  AML_GE2D_FILLRECTANGLE need set content:
src_info[0];
dst_info;
color;
offset;

2.AML_GE2D_BLEND need set content:
src_info[0];
src_info[1];
dst_info;
blend_mode;
offset;

3.AML_GE2D_STRETCHBLIT need set content:
src_info[0];
dst_info;
offset;

3.AML_GE2D_BLIT need set content:
src_info[0];
dst_info;
offset;

/////////////////////////////////////////////////////////////////
The test application include ge2d_load_test and ge2d_feature test.
ge2d_load_test provide ge2d loading test.
    ge2d_load_test --op 2 --duration 3000 --size 1920x1080 --pixelformat 0
    --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>    ge2d operation case.
    --duration <milliseconds>    Duration of each ge2d operation case.
    --size     <width>x<height>  Set ge2d size.
    --pixelformat <define as pixel_format_t>  Set ge2d pixelformat.

note: tester can change memtype to GE2D_CANVAS_ALLOC/GE2D_CANVAS_OSD0/GE2D_CANVAS_OSD1 as your requirement
      if src memtype is GE2D_CANVAS_ALLOC, it can read data from file via aml_read_file.

ge2d_feature_test provide ge2d operation example.
ge2d_feature_test --op 0
   --op <0:fillrect, 1:blend, 2:strechblit, 3:blit>    ge2d operation case;

note: tester can change memtype to GE2D_CANVAS_ALLOC/GE2D_CANVAS_OSD0/GE2D_CANVAS_OSD1 as your requirement
      if src1 memtype is GE2D_CANVAS_ALLOC, you can read data from file via aml_read_file_src1
      if src2 memtype is GE2D_CANVAS_ALLOC, you can read data from file via aml_read_file_src2
      if dst memtype is GE2D_CANVAS_ALLOC, you can write data to file aml_write_file
