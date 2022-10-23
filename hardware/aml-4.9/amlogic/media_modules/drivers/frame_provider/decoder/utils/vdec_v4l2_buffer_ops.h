#ifndef _AML_VDEC_V4L2_BUFFER_H_
#define _AML_VDEC_V4L2_BUFFER_H_

#include "../../../amvdec_ports/vdec_drv_base.h"
#include "../../../amvdec_ports/aml_vcodec_adapt.h"

int vdec_v4l_get_buffer(
	struct aml_vcodec_ctx *ctx,
	struct vdec_v4l2_buffer **out);

int vdec_v4l_get_pic_info(
	struct aml_vcodec_ctx *ctx,
	struct vdec_pic_info *pic);

int vdec_v4l_set_ps_infos(
	struct aml_vcodec_ctx *ctx,
	struct aml_vdec_ps_infos *ps);

int vdec_v4l_set_hdr_infos(
	struct aml_vcodec_ctx *ctx,
	struct aml_vdec_hdr_infos *hdr);

int vdec_v4l_write_frame_sync(
	struct aml_vcodec_ctx *ctx);

int vdec_v4l_post_evet(
	struct aml_vcodec_ctx *ctx,
	u32 event);

int vdec_v4l_res_ch_event(
	struct aml_vcodec_ctx *ctx);

#endif
