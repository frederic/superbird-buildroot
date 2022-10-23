/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <uapi/linux/swab.h>
#include "../vdec_drv_if.h"
#include "../aml_vcodec_util.h"
#include "../aml_vcodec_dec.h"
#include "../aml_vcodec_adapt.h"
#include "../vdec_drv_base.h"
#include "../aml_vcodec_vfm.h"
#include "aml_mpeg4_parser.h"

#define NAL_TYPE(value)				((value) & 0x1F)
#define HEADER_BUFFER_SIZE			(32 * 1024)

/**
 * struct mpeg4_fb - mpeg4 decode frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer (luma)
 * @c_fb_dma    : dma address of C frame buffer (chroma)
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct mpeg4_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct vdec_mpeg4_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @resolution_changed  : resoltion change happen
 * @reserved		: for 8 bytes alignment
 * @bs_dma		: Input bit-stream buffer dma address
 * @y_fb_dma		: Y frame buffer dma address
 * @c_fb_dma		: C frame buffer dma address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 */
struct vdec_mpeg4_dec_info {
	uint32_t dpb_sz;
	uint32_t resolution_changed;
	uint32_t reserved;
	uint64_t bs_dma;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	uint64_t vdec_fb_va;
};

/**
 * struct vdec_mpeg4_vsi - shared memory for decode information exchange
 *                        between VPU and Host.
 *                        The memory is allocated by VPU then mapping to Host
 *                        in vpu_dec_init() and freed in vpu_dec_deinit()
 *                        by VPU.
 *                        AP-W/R : AP is writer/reader on this item
 *                        VPU-W/R: VPU is write/reader on this item
 * @hdr_buf      : Header parsing buffer (AP-W, VPU-R)
 * @list_free    : free frame buffer ring list (AP-W/R, VPU-W)
 * @list_disp    : display frame buffer ring list (AP-R, VPU-W)
 * @dec          : decode information (AP-R, VPU-W)
 * @pic          : picture information (AP-R, VPU-W)
 * @crop         : crop information (AP-R, VPU-W)
 */
struct vdec_mpeg4_vsi {
	char *header_buf;
	int sps_size;
	int pps_size;
	int sei_size;
	int head_offset;
	struct vdec_mpeg4_dec_info dec;
	struct vdec_pic_info pic;
	struct vdec_pic_info cur_pic;
	struct v4l2_rect crop;
	bool is_combine;
	int nalu_pos;
	//struct mpeg4ParamSets ps;
};

/**
 * struct vdec_mpeg4_inst - mpeg4 decoder instance
 * @num_nalu : how many nalus be decoded
 * @ctx      : point to aml_vcodec_ctx
 * @vsi      : VPU shared information
 */
struct vdec_mpeg4_inst {
	unsigned int num_nalu;
	struct aml_vcodec_ctx *ctx;
	struct aml_vdec_adapt vdec;
	struct vdec_mpeg4_vsi *vsi;
	struct vcodec_vfm_s vfm;
	struct aml_dec_params parms;
	struct completion comp;
};

static void get_pic_info(struct vdec_mpeg4_inst *inst,
			 struct vdec_pic_info *pic)
{
	*pic = inst->vsi->pic;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO,
		"pic(%d, %d), buf(%d, %d)\n",
		 pic->visible_width, pic->visible_height,
		 pic->coded_width, pic->coded_height);
	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO,
		"Y(%d, %d), C(%d, %d)\n",
		pic->y_bs_sz, pic->y_len_sz,
		pic->c_bs_sz, pic->c_len_sz);
}

static void get_crop_info(struct vdec_mpeg4_inst *inst, struct v4l2_rect *cr)
{
	cr->left = inst->vsi->crop.left;
	cr->top = inst->vsi->crop.top;
	cr->width = inst->vsi->crop.width;
	cr->height = inst->vsi->crop.height;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO,
		"l=%d, t=%d, w=%d, h=%d\n",
		 cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_mpeg4_inst *inst, unsigned int *dpb_sz)
{
	*dpb_sz = inst->vsi->dec.dpb_sz;
	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO, "sz=%d\n", *dpb_sz);
}

static u32 vdec_config_default_parms(u8 *parm)
{
	u8 *pbuf = parm;

	pbuf += sprintf(pbuf, "parm_v4l_codec_enable:1;");
	pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_mode:0;");
	pbuf += sprintf(pbuf, "parm_v4l_buffer_margin:0;");

	return pbuf - parm;
}

static void vdec_parser_parms(struct vdec_mpeg4_inst *inst)
{
	struct aml_vcodec_ctx *ctx = inst->ctx;

	if (ctx->config.parm.dec.parms_status &
		V4L2_CONFIG_PARM_DECODE_CFGINFO) {
		u8 *pbuf = ctx->config.buf;

		pbuf += sprintf(pbuf, "parm_v4l_codec_enable:1;");
		pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_mode:%d;",
			ctx->config.parm.dec.cfg.canvas_mem_mode);
		pbuf += sprintf(pbuf, "parm_v4l_buffer_margin:%d;",
			ctx->config.parm.dec.cfg.ref_buf_margin);
		ctx->config.length = pbuf - ctx->config.buf;
	} else {
		ctx->config.length = vdec_config_default_parms(ctx->config.buf);
	}

	inst->vdec.config	= ctx->config;
	inst->parms.cfg		= ctx->config.parm.dec.cfg;
	inst->parms.parms_status |= V4L2_CONFIG_PARM_DECODE_CFGINFO;
}


static int vdec_mpeg4_init(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_mpeg4_inst *inst = NULL;
	int ret = -1;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->vdec.video_type	= VFORMAT_MPEG4;
	inst->vdec.format	= VIDEO_DEC_FORMAT_MPEG4_5;
	inst->vdec.dev		= ctx->dev->vpu_plat_dev;
	inst->vdec.filp		= ctx->dev->filp;
	inst->vdec.config	= ctx->config;
	inst->vdec.ctx		= ctx;
	inst->ctx		= ctx;

	vdec_parser_parms(inst);
	/* set play mode.*/
	if (ctx->is_drm_mode)
		inst->vdec.port.flag |= PORT_FLAG_DRM;

	/* to eable mpeg4 hw.*/
	inst->vdec.port.type	= PORT_TYPE_VIDEO;

	/* init vfm */
	inst->vfm.ctx		= ctx;
	inst->vfm.ada_ctx	= &inst->vdec;
	ret = vcodec_vfm_init(&inst->vfm);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"init vfm failed.\n");
		goto err;
	}

	ret = video_decoder_init(&inst->vdec);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"vdec_mpeg4 init err=%d\n", ret);
		goto err;
	}

	/* probe info from the stream */
	inst->vsi = kzalloc(sizeof(struct vdec_mpeg4_vsi), GFP_KERNEL);
	if (!inst->vsi) {
		ret = -ENOMEM;
		goto err;
	}

	/* alloc the header buffer to be used cache sps or spp etc.*/
	inst->vsi->header_buf = kzalloc(HEADER_BUFFER_SIZE, GFP_KERNEL);
	if (!inst->vsi->header_buf) {
		ret = -ENOMEM;
		goto err;
	}

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
		"mpeg4 Instance >> %lx\n", (ulong) inst);

	init_completion(&inst->comp);
	ctx->ada_ctx	= &inst->vdec;
	*h_vdec		= (unsigned long)inst;

	//dump_init();

	return 0;

err:
	if (inst)
		vcodec_vfm_release(&inst->vfm);
	if (inst && inst->vsi && inst->vsi->header_buf)
		kfree(inst->vsi->header_buf);
	if (inst && inst->vsi)
		kfree(inst->vsi);
	if (inst)
		kfree(inst);
	*h_vdec = 0;

	return ret;
}

#if 0
static int refer_buffer_num(int level_idc, int poc_cnt,
	int mb_width, int mb_height)
{
	return 20;
}
#endif

static void fill_vdec_params(struct vdec_mpeg4_inst *inst,
	struct mpeg4_dec_param *dec_ps)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_mpeg4_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

	/* fill visible area size that be used for EGL. */
	pic->visible_width	= dec_ps->m.width;
	pic->visible_height	= dec_ps->m.height;

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= ALIGN(dec_ps->m.width, 64);
	pic->coded_height	= ALIGN(dec_ps->m.height, 64);

	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/*8(DECODE_BUFFER_NUM_DEF) */
	dec->dpb_sz = 8;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_BUFMGR,
		"The stream infos, coded:(%d x %d), visible:(%d x %d), DPB: %d\n",
		pic->coded_width, pic->coded_height,
		pic->visible_width, pic->visible_height, dec->dpb_sz);
}

static int parse_stream_ucode(struct vdec_mpeg4_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;

	ret = vdec_vframe_write(vdec, buf, size, 0);
	if (ret < 0) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"write frame data failed. err: %d\n", ret);
		return ret;
	}

	/* wait ucode parse ending. */
	wait_for_completion_timeout(&inst->comp,
		msecs_to_jiffies(1000));

	return inst->vsi->dec.dpb_sz ? 0 : -1;
}

static int parse_stream_ucode_dma(struct vdec_mpeg4_inst *inst,
	ulong buf, u32 size, u32 handle)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;

	ret = vdec_vframe_write_with_dma(vdec, buf, size, 0, handle);
	if (ret < 0) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"write frame data failed. err: %d\n", ret);
		return ret;
	}

	/* wait ucode parse ending. */
	wait_for_completion_timeout(&inst->comp,
		msecs_to_jiffies(1000));

	return inst->vsi->dec.dpb_sz ? 0 : -1;
}

static int parse_stream_cpu(struct vdec_mpeg4_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct mpeg4_param_sets *ps = NULL;

	ps = kzalloc(sizeof(struct mpeg4_param_sets), GFP_KERNEL);
	if (ps == NULL)
		return -ENOMEM;

	ret = mpeg4_decode_extradata_ps(buf, size, ps);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"parse extra data failed. err: %d\n", ret);
		goto out;
	}

	if (ps->head_parsed)
		fill_vdec_params(inst, &ps->dec_ps);

	ret = ps->head_parsed ? 0 : -1;
out:
	kfree(ps);

	return ret;
}

static int vdec_mpeg4_probe(unsigned long h_vdec,
	struct aml_vcodec_mem *bs, void *out)
{
	struct vdec_mpeg4_inst *inst =
		(struct vdec_mpeg4_inst *)h_vdec;
	u8 *buf = (u8 *)bs->vaddr;
	u32 size = bs->size;
	int ret = 0;

	if (inst->ctx->is_drm_mode) {
		if (bs->model == VB2_MEMORY_MMAP) {
			struct aml_video_stream *s =
				(struct aml_video_stream *) buf;

			if ((s->magic != AML_VIDEO_MAGIC) &&
				(s->type != V4L_STREAM_TYPE_MATEDATA))
				return -1;

			if (inst->ctx->param_sets_from_ucode) {
				ret = parse_stream_ucode(inst, s->data, s->len);
			} else {
				ret = parse_stream_cpu(inst, s->data, s->len);
			}
		} else if (bs->model == VB2_MEMORY_DMABUF ||
			bs->model == VB2_MEMORY_USERPTR) {
			ret = parse_stream_ucode_dma(inst, bs->addr, size,
				BUFF_IDX(bs, bs->index));
		}
	} else {
		if (inst->ctx->param_sets_from_ucode) {
			ret = parse_stream_ucode(inst, buf, size);
		} else {
			ret = parse_stream_cpu(inst, buf, size);
		}
	}

	inst->vsi->cur_pic = inst->vsi->pic;

	return ret;
}

static void vdec_mpeg4_deinit(unsigned long h_vdec)
{
	struct vdec_mpeg4_inst *inst = (struct vdec_mpeg4_inst *)h_vdec;

	if (!inst)
		return;

	video_decoder_release(&inst->vdec);

	vcodec_vfm_release(&inst->vfm);

	//dump_deinit();

	if (inst->vsi && inst->vsi->header_buf)
		kfree(inst->vsi->header_buf);

	if (inst->vsi)
		kfree(inst->vsi);

	kfree(inst);
}

static int vdec_mpeg4_get_fb(struct vdec_mpeg4_inst *inst, struct vdec_v4l2_buffer **out)
{
	return get_fb_from_queue(inst->ctx, out);
}

static void vdec_mpeg4_get_vf(struct vdec_mpeg4_inst *inst, struct vdec_v4l2_buffer **out)
{
	struct vframe_s *vf = NULL;
	struct vdec_v4l2_buffer *fb = NULL;

	vf = peek_video_frame(&inst->vfm);
	if (!vf) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"there is no vframe.\n");
		*out = NULL;
		return;
	}

	vf = get_video_frame(&inst->vfm);
	if (!vf) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"the vframe is avalid.\n");
		*out = NULL;
		return;
	}

	atomic_set(&vf->use_cnt, 1);

	fb = (struct vdec_v4l2_buffer *)vf->v4l_mem_handle;
	fb->vf_handle = (unsigned long)vf;
	fb->status = FB_ST_DISPLAY;

	*out = fb;

	//pr_info("%s, %d\n", __func__, fb->base_y.bytes_used);
	//dump_write(fb->base_y.va, fb->base_y.bytes_used);
	//dump_write(fb->base_c.va, fb->base_c.bytes_used);

	/* convert yuv format. */
	//swap_uv(fb->base_c.va, fb->base_c.size);
}

static int vdec_write_nalu(struct vdec_mpeg4_inst *inst,
	u8 *buf, u32 size, u64 ts)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;

	ret = vdec_vframe_write(vdec, buf, size, ts);

	return ret;
}

static int vdec_mpeg4_decode(unsigned long h_vdec, struct aml_vcodec_mem *bs,
			 u64 timestamp, bool *res_chg)
{
	struct vdec_mpeg4_inst *inst = (struct vdec_mpeg4_inst *)h_vdec;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	u8 *buf = (u8 *) bs->vaddr;
	u32 size = bs->size;
	int ret = -1;

	if (vdec_input_full(vdec))
		return -EAGAIN;

	if (inst->ctx->is_drm_mode) {
		if (bs->model == VB2_MEMORY_MMAP) {
			struct aml_video_stream *s =
				(struct aml_video_stream *) buf;

			if (s->magic != AML_VIDEO_MAGIC)
				return -1;

			ret = vdec_vframe_write(vdec,
				s->data,
				s->len,
				timestamp);
		} else if (bs->model == VB2_MEMORY_DMABUF ||
			bs->model == VB2_MEMORY_USERPTR) {
			ret = vdec_vframe_write_with_dma(vdec,
				bs->addr, size, timestamp,
				BUFF_IDX(bs, bs->index));
		}
	} else {
		ret = vdec_write_nalu(inst, buf, size, timestamp);
	}

	return ret;
}

static int vdec_mpeg4_get_param(unsigned long h_vdec,
			       enum vdec_get_param_type type, void *out)
{
	int ret = 0;
	struct vdec_mpeg4_inst *inst = (struct vdec_mpeg4_inst *)h_vdec;

	if (!inst) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"the mpeg4 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		vdec_mpeg4_get_vf(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		ret = vdec_mpeg4_get_fb(inst, out);
		break;

	case GET_PARAM_PIC_INFO:
		get_pic_info(inst, out);
		break;

	case GET_PARAM_DPB_SIZE:
		get_dpb_size(inst, out);
		break;

	case GET_PARAM_CROP_INFO:
		get_crop_info(inst, out);
		break;

	default:
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
			"invalid get parameter type=%d\n", type);
		ret = -EINVAL;
	}

	return ret;
}

static void set_param_ps_info(struct vdec_mpeg4_inst *inst,
	struct aml_vdec_ps_infos *ps)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_mpeg4_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO, "%s in\n", __func__);
	/* fill visible area size that be used for EGL. */
	pic->visible_width	= ps->visible_width;
	pic->visible_height	= ps->visible_height;

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= ps->coded_width;
	pic->coded_height	= ps->coded_height;
	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	dec->dpb_sz		= ps->dpb_size;

	inst->parms.ps 	= *ps;
	inst->parms.parms_status |=
		V4L2_CONFIG_PARM_DECODE_PSINFO;

	/*wake up*/
	complete(&inst->comp);

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
		"Parse from ucode, crop(%d x %d), coded(%d x %d) dpb: %d\n",
		ps->visible_width, ps->visible_height,
		ps->coded_width, ps->coded_height,
		dec->dpb_sz);
}

static void set_param_write_sync(struct vdec_mpeg4_inst *inst)
{
	complete(&inst->comp);
}

static int vdec_mpeg4_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	int ret = 0;
	struct vdec_mpeg4_inst *inst = (struct vdec_mpeg4_inst *)h_vdec;

	if (!inst) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"the mpeg4 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case SET_PARAM_WRITE_FRAME_SYNC:
		set_param_write_sync(inst);
		break;
	case SET_PARAM_PS_INFO:
		set_param_ps_info(inst, in);
		break;

	default:
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"invalid set parameter type=%d\n", type);
		ret = -EINVAL;
	}

	return ret;
}

static struct vdec_common_if vdec_mpeg4_if = {
	.init		= vdec_mpeg4_init,
	.probe		= vdec_mpeg4_probe,
	.decode		= vdec_mpeg4_decode,
	.get_param	= vdec_mpeg4_get_param,
	.set_param	= vdec_mpeg4_set_param,
	.deinit		= vdec_mpeg4_deinit,
};

struct vdec_common_if *get_mpeg4_dec_comm_if(void);

struct vdec_common_if *get_mpeg4_dec_comm_if(void)
{
	return &vdec_mpeg4_if;
}
