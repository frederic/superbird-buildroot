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
    unsigned int const_color;
    unsigned int dst_op_cnt;
    unsigned int reserved;
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
    unsigned int mem_alloc_type;
    unsigned int memtype;
    char* vaddr;
    unsigned long offset;
    unsigned int canvas_w;
    unsigned int canvas_h;
    rectangle_t rect;
    int format;
    unsigned int rotation;
    int shared_fd;
    unsigned char plane_alpha;
    unsigned char layer_mode;
    unsigned char fill_color_en;
    unsigned int  def_color;
} buffer_info_t;
some bits of struct buffer_info:
mem_alloc_type:         if used as ion alloc or dma_buf alloc
memtype:                if used as mem alloc:CANVAS_ALLOC
                        if used as OSD0/OSD1:CANVAS_OSD0/CANVAS_OSD1
char* vaddr:            not need set,for debug
int shared_fd:          shared buffer fd, alloc by ion or dma_buf
unsigned offset:        buffer offset, default is 0
unsigned int canvas_w,
unsigned int canvas_h:  if used as mem alloc,for src1, set it to rect.w, rect.h;
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
	--op <0:fillrect, 1:blend, 2:strechblit, 3:blit>  ge2d operation case.
	--size <WxH>                                      define src1/src2/dst size.
	--src1_memtype <0: ion, 1: dmabuf>                define memory alloc type.
	--src2_memtype <0: ion, 1: dmabuf>                define memory alloc type.
	--dst_memtype <0: ion, 1: dmabuf>                 define memory alloc type.
	--src1_format  <num>                              define src1 format.
	--src2_format <num>                               define src2 format.
	--dst_format  <num>                               define dst format.
	--src1_size  <WxH>                                define src1 size.
	--src2_size <WxH>                                 define src2 size.
	--dst_size  <WxH>                                 define dst size.
	--src1_file  <name>                               define src1 file.
	--src2_file <name>                                define src2 file.
	--dst_file  <name>                                define dst file.
	--src1_canvas_alloc  <num>                        define whether src1 need alloc mem   0:GE2D_CANVAS_OSD0 1:GE2D_CANVAS_ALLOC.
	--src2_canvas_alloc <num>                         defien whether src2 need alloc mem  0:GE2D_CANVAS_OSD0 1:GE2D_CANVAS_ALLOC.
	--src1_rect  <x_y_w_h>                            define src1 rect.
	--src2_rect <x_y_w_h>                             define src2 rect.
	--dst_rect  <x_y_w_h>                             define dst rect.
	--bo1 <layer_mode_num>                            define src1_layer_mode.
	--bo2 <layer_mode_num>                            define src2_layer_mode.
	--gb1 <gb1_alpha>                                 define src1 global alpha.
	--gb2 <gb2_alpha>                                 define src2 global alpha.
	--strechblit <x0_y0_w_h-x1_y1_w1_h1>              define strechblit info.
	--fillrect <color_x_y_w_h>                        define fillrect info, color in rgba format.
	--src2_planenumber <num>                          define src2 plane number.
	--src1_planenumber <num>                          define src1 plane number.
	--dst_planenumber <num>                           define dst plane number.
	--help                                            Print usage information.
note: tester can change memtype to GE2D_CANVAS_ALLOC/GE2D_CANVAS_OSD0/GE2D_CANVAS_OSD1 as your requirement
      if src1 memtype is GE2D_CANVAS_ALLOC, it can read data from file via aml_read_file.

note: tester can change memtype to GE2D_CANVAS_ALLOC/GE2D_CANVAS_OSD0/GE2D_CANVAS_OSD1 as your requirement
      if src1 memtype is GE2D_CANVAS_ALLOC, you can read data from file via aml_read_file_src1
      if src2 memtype is GE2D_CANVAS_ALLOC, you can read data from file via aml_read_file_src2
      if dst memtype is GE2D_CANVAS_ALLOC, you can write data to file aml_write_file

ge2d_feature_test provide ge2d operation example.
//src used ion alloc, dst used osd
./ge2d_feature_test --op 2 --src1_memtype 0 --dst_memtype 0 --size 1920x1080 --src1_format 1 --dst_format 1 --src1_file 1080P_RGBA8888.rgb32
//src used dma_buf alloc, dst used osd
./ge2d_feature_test --op 2 --src1_memtype 1 --dst_memtype 1 --size 1920x1080 --src1_format 1 --dst_format 1 --src1_file 1080P_RGBA8888.rgb32

//src used ion alloc, dst used dma_buf alloc
./ge2d_feature_test --op 2 --src1_memtype 0 --dst_memtype 1 --size 1920x1080 --src1_format 1 --dst_format 1 --src1_file 1080P_RGBA8888.rgb32 --dst_file out_dma.rgb32

//src used dma_buf alloc, dst used ion alloc
./ge2d_feature_test --op 2 --src1_memtype 1 --dst_memtype 0 --size 1920x1080 --src1_format 1 --dst_format 1 --src1_file 1080P_RGBA8888.rgb32 --dst_file out_ion.rgb32

//src used dma_buf alloc, dst used dma_buf alloc
./ge2d_feature_test --op 2 --src1_memtype 1 --dst_memtype 1--size 1920x1080 --src1_format 1 --dst_format 1 --src1_file 1080P_RGBA8888.rgb32 --dst_file out_ion.rgb32




if dma buffer is allocated by ge2d, please add content blow in DTS:

/ {
    /* ...... */
    reserved-memory {
        /* ...... */
            ge2d_cma_reserved:linux,ge2d_cma {
                compatible = "shared-dma-pool";
                reusable;
                status = "okay";
                size = <0x0 0x1800000>;
                alignment = <0x0 0x400000>;
            };
    }
    /* ...... */
    ge2d {
        /* ...... */
        memory-region = <&ge2d_cma_reserved>;
    };
}


usage examples:
fillrect ---
./ge2d_feature_test --op 0 --dst_size 1920x1080 --src1_format 1 --dst_format 1 --fillrect 0x00ff00ff_50_50_100_100 --dst_file fillrect.rgb32

blend ---
./ge2d_feature_test --op 1 --dst_size 1920x1080 --dst_rect 100_100_400_400 --src1_size 1920x1080 --src1_rect 100_100_400_400 --src2_size 400x400 --src2_rect 0_0_400_400 --dst_format 1 --src1_format 1 --src2_format 1 --src1_file 1080P_RGBA8888.rgb32 --src2_file 400x400_RGBA8888.rgb32  --bo1 2 --bo2 2 --gb1 150 --gb2 255 --dst_file blend.rgb32

strechblit ---
./ge2d_feature_test --op 2 --size 1920x1080 --src1_format 1 --dst_format 1 --strechblit 0_0_1920_100-50_50_200_300 --src1_file 1080P_RGBA8888.rgb32 --dst_file strechblit.rgb32

blit ---
./ge2d_feature_test --op 3 --size 1920x1080 --src1_format 1 --dst_format 1 --src1_rect 0_0_1920_100 --dst_rect 0_0_1920_100 --src1_file 1080P_RGBA8888.rgb32 --dst_file blit.rgb32
multiplane fd support:
add options para if needed:
--src1_planenumber 1 src2_planenumber 1 --dst_planenumber 1
--src1_planenumber 2 src2_planenumber 2 --dst_planenumber 2
