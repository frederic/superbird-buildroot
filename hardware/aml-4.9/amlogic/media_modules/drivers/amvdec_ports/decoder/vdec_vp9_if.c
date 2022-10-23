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
#include "../aml_vcodec_drv.h"
#include "../aml_vcodec_adapt.h"
#include "../vdec_drv_base.h"
#include "../aml_vcodec_vfm.h"
#include "aml_vp9_parser.h"
#include "vdec_vp9_trigger.h"

#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_V4L2
#include <trace/events/meson_atrace.h>

#define PREFIX_SIZE	(16)

#define NAL_TYPE(value)				((value) & 0x1F)
#define HEADER_BUFFER_SIZE			(32 * 1024)
#define SYNC_CODE				(0x498342)

extern int vp9_need_prefix;
bool need_trigger;
int dump_cnt = 0;

/**
 * struct vp9_fb - vp9 decode frame buffer information
 * @vdec_fb_va  : virtual address of struct vdec_fb
 * @y_fb_dma    : dma address of Y frame buffer (luma)
 * @c_fb_dma    : dma address of C frame buffer (chroma)
 * @poc         : picture order count of frame buffer
 * @reserved    : for 8 bytes alignment
 */
struct vp9_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};

/**
 * struct vdec_vp9_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @resolution_changed  : resoltion change happen
 * @reserved		: for 8 bytes alignment
 * @bs_dma		: Input bit-stream buffer dma address
 * @y_fb_dma		: Y frame buffer dma address
 * @c_fb_dma		: C frame buffer dma address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 */
struct vdec_vp9_dec_info {
	uint32_t dpb_sz;
	uint32_t resolution_changed;
	uint32_t reserved;
	uint64_t bs_dma;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	uint64_t vdec_fb_va;
};

/**
 * struct vdec_vp9_vsi - shared memory for decode information exchange
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
struct vdec_vp9_vsi {
	char *header_buf;
	int sps_size;
	int pps_size;
	int sei_size;
	int head_offset;
	struct vdec_vp9_dec_info dec;
	struct vdec_pic_info pic;
	struct vdec_pic_info cur_pic;
	struct v4l2_rect crop;
	bool is_combine;
	int nalu_pos;
	struct vp9_param_sets ps;
};

/**
 * struct vdec_vp9_inst - vp9 decoder instance
 * @num_nalu : how many nalus be decoded
 * @ctx      : point to aml_vcodec_ctx
 * @vsi      : VPU shared information
 */
struct vdec_vp9_inst {
	unsigned int num_nalu;
	struct aml_vcodec_ctx *ctx;
	struct aml_vdec_adapt vdec;
	struct vdec_vp9_vsi *vsi;
	struct vcodec_vfm_s vfm;
	struct aml_dec_params parms;
	struct completion comp;
};

static int vdec_write_nalu(struct vdec_vp9_inst *inst,
	u8 *buf, u32 size, u64 ts);

static void get_pic_info(struct vdec_vp9_inst *inst,
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

static void get_crop_info(struct vdec_vp9_inst *inst, struct v4l2_rect *cr)
{
	cr->left = inst->vsi->crop.left;
	cr->top = inst->vsi->crop.top;
	cr->width = inst->vsi->crop.width;
	cr->height = inst->vsi->crop.height;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO,
		"l=%d, t=%d, w=%d, h=%d\n",
		 cr->left, cr->top, cr->width, cr->height);
}

static void get_dpb_size(struct vdec_vp9_inst *inst, unsigned int *dpb_sz)
{
	*dpb_sz = inst->vsi->dec.dpb_sz;
	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_EXINFO, "sz=%d\n", *dpb_sz);
}

static u32 vdec_config_default_parms(u8 *parm)
{
	u8 *pbuf = parm;

	pbuf += sprintf(pbuf, "parm_v4l_codec_enable:1;");
	pbuf += sprintf(pbuf, "parm_v4l_buffer_margin:7;");
	pbuf += sprintf(pbuf, "vp9_double_write_mode:16;");
	pbuf += sprintf(pbuf, "vp9_buf_width:1920;");
	pbuf += sprintf(pbuf, "vp9_buf_height:1088;");
	pbuf += sprintf(pbuf, "vp9_max_pic_w:4096;");
	pbuf += sprintf(pbuf, "vp9_max_pic_h:2304;");
	pbuf += sprintf(pbuf, "save_buffer_mode:0;");
	pbuf += sprintf(pbuf, "no_head:0;");
	pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_mode:0;");
	pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_endian:0;");

	return parm - pbuf;
}

static void vdec_parser_parms(struct vdec_vp9_inst *inst)
{
	struct aml_vcodec_ctx *ctx = inst->ctx;

	if (ctx->config.parm.dec.parms_status &
		V4L2_CONFIG_PARM_DECODE_CFGINFO) {
		u8 *pbuf = ctx->config.buf;

		pbuf += sprintf(pbuf, "parm_v4l_codec_enable:1;");
		pbuf += sprintf(pbuf, "parm_v4l_buffer_margin:%d;",
			ctx->config.parm.dec.cfg.ref_buf_margin);
		pbuf += sprintf(pbuf, "vp9_double_write_mode:%d;",
			ctx->config.parm.dec.cfg.double_write_mode);
		pbuf += sprintf(pbuf, "vp9_buf_width:%d;",
			ctx->config.parm.dec.cfg.init_width);
		pbuf += sprintf(pbuf, "vp9_buf_height:%d;",
			ctx->config.parm.dec.cfg.init_height);
		pbuf += sprintf(pbuf, "save_buffer_mode:0;");
		pbuf += sprintf(pbuf, "no_head:0;");
		pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_mode:%d;",
			ctx->config.parm.dec.cfg.canvas_mem_mode);
		pbuf += sprintf(pbuf, "parm_v4l_canvas_mem_endian:%d;",
			ctx->config.parm.dec.cfg.canvas_mem_endian);
		ctx->config.length = pbuf - ctx->config.buf;
	} else {
		ctx->config.parm.dec.cfg.double_write_mode = 16;
		ctx->config.parm.dec.cfg.ref_buf_margin = 7;
		ctx->config.length = vdec_config_default_parms(ctx->config.buf);
	}

	if ((ctx->config.parm.dec.parms_status &
		V4L2_CONFIG_PARM_DECODE_HDRINFO) &&
		inst->parms.hdr.color_parms.present_flag) {
		u8 *pbuf = ctx->config.buf + ctx->config.length;

		pbuf += sprintf(pbuf, "mG.x:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[0][0]);
		pbuf += sprintf(pbuf, "mG.y:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[0][1]);
		pbuf += sprintf(pbuf, "mB.x:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[1][0]);
		pbuf += sprintf(pbuf, "mB.y:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[1][1]);
		pbuf += sprintf(pbuf, "mR.x:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[2][0]);
		pbuf += sprintf(pbuf, "mR.y:%d;",
			ctx->config.parm.dec.hdr.color_parms.primaries[2][1]);
		pbuf += sprintf(pbuf, "mW.x:%d;",
			ctx->config.parm.dec.hdr.color_parms.white_point[0]);
		pbuf += sprintf(pbuf, "mW.y:%d;",
			ctx->config.parm.dec.hdr.color_parms.white_point[1]);
		pbuf += sprintf(pbuf, "mMaxDL:%d;",
			ctx->config.parm.dec.hdr.color_parms.luminance[0] / 1000);
		pbuf += sprintf(pbuf, "mMinDL:%d;",
			ctx->config.parm.dec.hdr.color_parms.luminance[1]);
		pbuf += sprintf(pbuf, "mMaxCLL:%d;",
			ctx->config.parm.dec.hdr.color_parms.content_light_level.max_content);
		pbuf += sprintf(pbuf, "mMaxFALL:%d;",
			ctx->config.parm.dec.hdr.color_parms.content_light_level.max_pic_average);
		ctx->config.length	= pbuf - ctx->config.buf;
		inst->parms.hdr		= ctx->config.parm.dec.hdr;
		inst->parms.parms_status |= V4L2_CONFIG_PARM_DECODE_HDRINFO;
	}

	inst->vdec.config	= ctx->config;
	inst->parms.cfg		= ctx->config.parm.dec.cfg;
	inst->parms.parms_status |= V4L2_CONFIG_PARM_DECODE_CFGINFO;
}

static int vdec_vp9_init(struct aml_vcodec_ctx *ctx, unsigned long *h_vdec)
{
	struct vdec_vp9_inst *inst = NULL;
	int ret = -1;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->vdec.video_type	= VFORMAT_VP9;
	inst->vdec.dev		= ctx->dev->vpu_plat_dev;
	inst->vdec.filp		= ctx->dev->filp;
	inst->vdec.ctx		= ctx;
	inst->ctx		= ctx;

	vdec_parser_parms(inst);

	/* set play mode.*/
	if (ctx->is_drm_mode)
		inst->vdec.port.flag |= PORT_FLAG_DRM;

	/* to eable vp9 hw.*/
	inst->vdec.port.type	= PORT_TYPE_HEVC;

	/* init vfm */
	inst->vfm.ctx		= ctx;
	inst->vfm.ada_ctx	= &inst->vdec;
	ret = vcodec_vfm_init(&inst->vfm);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"init vfm failed.\n");
		goto err;
	}

	/* probe info from the stream */
	inst->vsi = kzalloc(sizeof(struct vdec_vp9_vsi), GFP_KERNEL);
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

	init_completion(&inst->comp);

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
		"vp9 Instance >> %lx\n", (ulong) inst);

	ctx->ada_ctx	= &inst->vdec;
	*h_vdec		= (unsigned long)inst;

	/* init decoder. */
	ret = video_decoder_init(&inst->vdec);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"vdec_vp9 init err=%d\n", ret);
		goto err;
	}

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

static int vdec_get_dw_mode(struct vdec_vp9_inst *inst, int dw_mode)
{
	u32 valid_dw_mode = inst->parms.cfg.double_write_mode;
	int w = inst->parms.cfg.init_width;
	int h = inst->parms.cfg.init_height;
	u32 dw = 0x1; /*1:1*/

	switch (valid_dw_mode) {
	case 0x100:
		if (w > 1920 && h > 1088)
			dw = 0x4; /*1:2*/
		break;
	case 0x200:
		if (w > 1920 && h > 1088)
			dw = 0x2; /*1:4*/
		break;
	case 0x300:
		if (w > 1280 && h > 720)
			dw = 0x4; /*1:2*/
		break;
	default:
		dw = valid_dw_mode;
		break;
	}

	return dw;
}

static int vdec_pic_scale(struct vdec_vp9_inst *inst, int length, int dw_mode)
{
	int ret = 64;

	switch (vdec_get_dw_mode(inst, dw_mode)) {
	case 0x0: /* only afbc, output afbc */
		ret = 64;
		break;
	case 0x1: /* afbc and (w x h), output YUV420 */
		ret = length;
		break;
	case 0x2: /* afbc and (w/4 x h/4), output YUV420 */
	case 0x3: /* afbc and (w/4 x h/4), output afbc and YUV420 */
		ret = length >> 2;
		break;
	case 0x4: /* afbc and (w/2 x h/2), output YUV420 */
		ret = length >> 1;
		break;
	case 0x10: /* (w x h), output YUV420-8bit) */
	default:
		ret = length;
		break;
	}

	return ret;
}

static void fill_vdec_params(struct vdec_vp9_inst *inst,
	struct VP9Context *vp9_ctx)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_vp9_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;
	int dw = inst->parms.cfg.double_write_mode;
	int margin = inst->parms.cfg.ref_buf_margin;

	/* fill visible area size that be used for EGL. */
	pic->visible_width	= vdec_pic_scale(inst, vp9_ctx->render_width, dw);
	pic->visible_height	= vdec_pic_scale(inst, vp9_ctx->render_height, dw);

	/* calc visible ares. */
	rect->left		= 0;
	rect->top		= 0;
	rect->width		= pic->visible_width;
	rect->height		= pic->visible_height;

	/* config canvas size that be used for decoder. */
	pic->coded_width	= vdec_pic_scale(inst, ALIGN(vp9_ctx->width, 32), dw);
	pic->coded_height	= vdec_pic_scale(inst, ALIGN(vp9_ctx->height, 32), dw);

	pic->y_len_sz		= pic->coded_width * pic->coded_height;
	pic->c_len_sz		= pic->y_len_sz >> 1;

	/* calc DPB size */
	dec->dpb_sz = 5 + margin;//refer_buffer_num(sps->level_idc, poc_cnt, mb_w, mb_h);

	inst->parms.ps.visible_width	= pic->visible_width;
	inst->parms.ps.visible_height	= pic->visible_height;
	inst->parms.ps.coded_width	= pic->coded_width;
	inst->parms.ps.coded_height	= pic->coded_height;
	inst->parms.ps.dpb_size		= dec->dpb_sz;
	inst->parms.parms_status	|= V4L2_CONFIG_PARM_DECODE_PSINFO;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_BUFMGR,
		"The stream infos, dw: %d, coded:(%d x %d), visible:(%d x %d), DPB: %d, margin: %d\n",
		dw, pic->coded_width, pic->coded_height,
		pic->visible_width, pic->visible_height,
		dec->dpb_sz - margin, margin);
}

static int parse_stream_ucode(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;

	ret = vdec_write_nalu(inst, buf, size, 0);
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

static int parse_stream_ucode_dma(struct vdec_vp9_inst *inst,
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

static int parse_stream_cpu(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = 0;
	struct vp9_param_sets *ps = NULL;

	ps = vzalloc(sizeof(struct vp9_param_sets));
	if (ps == NULL)
		return -ENOMEM;

	ret = vp9_decode_extradata_ps(buf, size, ps);
	if (ret) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"parse extra data failed. err: %d\n", ret);
		goto out;
	}

	if (ps->head_parsed)
		fill_vdec_params(inst, &ps->ctx);

	ret = ps->head_parsed ? 0 : -1;
out:
	vfree(ps);

	return ret;
}

static int vdec_vp9_probe(unsigned long h_vdec,
	struct aml_vcodec_mem *bs, void *out)
{
	struct vdec_vp9_inst *inst =
		(struct vdec_vp9_inst *)h_vdec;
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

static void vdec_vp9_deinit(unsigned long h_vdec)
{
	ulong flags;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	struct aml_vcodec_ctx *ctx = inst->ctx;

	video_decoder_release(&inst->vdec);

	vcodec_vfm_release(&inst->vfm);

	//dump_deinit();

	spin_lock_irqsave(&ctx->slock, flags);
	if (inst->vsi && inst->vsi->header_buf)
		kfree(inst->vsi->header_buf);

	if (inst->vsi)
		kfree(inst->vsi);

	kfree(inst);

	ctx->drv_handle = 0;
	spin_unlock_irqrestore(&ctx->slock, flags);

	need_trigger = false;
	dump_cnt = 0;
}

static int vdec_vp9_get_fb(struct vdec_vp9_inst *inst, struct vdec_v4l2_buffer **out)
{
	return get_fb_from_queue(inst->ctx, out);
}

static void vdec_vp9_get_vf(struct vdec_vp9_inst *inst, struct vdec_v4l2_buffer **out)
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
	//dump_write(fb->base_y.vaddr, fb->base_y.bytes_used);
	//dump_write(fb->base_c.vaddr, fb->base_c.bytes_used);

	/* convert yuv format. */
	//swap_uv(fb->base_c.vaddr, fb->base_c.size);
}

static void add_prefix_data(struct vp9_superframe_split *s,
	u8 **out, u32 *out_size)
{
	int i;
	u8 *p = NULL;
	u32 length;

	length = s->size + s->nb_frames * PREFIX_SIZE;
	p = vzalloc(length);
	if (!p) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"alloc size %d failed.\n" ,length);
		return;
	}

	memcpy(p, s->data, s->size);
	p += s->size;

	for (i = s->nb_frames; i > 0; i--) {
		u32 frame_size = s->sizes[i - 1];
		u8 *prefix = NULL;

		p -= frame_size;
		memmove(p + PREFIX_SIZE * i, p, frame_size);
		prefix = p + PREFIX_SIZE * (i - 1);

		/*add amlogic frame headers.*/
		frame_size += 16;
		prefix[0]  = (frame_size >> 24) & 0xff;
		prefix[1]  = (frame_size >> 16) & 0xff;
		prefix[2]  = (frame_size >> 8 ) & 0xff;
		prefix[3]  = (frame_size >> 0 ) & 0xff;
		prefix[4]  = ((frame_size >> 24) & 0xff) ^ 0xff;
		prefix[5]  = ((frame_size >> 16) & 0xff) ^ 0xff;
		prefix[6]  = ((frame_size >> 8 ) & 0xff) ^ 0xff;
		prefix[7]  = ((frame_size >> 0 ) & 0xff) ^ 0xff;
		prefix[8]  = 0;
		prefix[9]  = 0;
		prefix[10] = 0;
		prefix[11] = 1;
		prefix[12] = 'A';
		prefix[13] = 'M';
		prefix[14] = 'L';
		prefix[15] = 'V';
		frame_size -= 16;
	}

	*out = p;
	*out_size = length;
}

static void trigger_decoder(struct aml_vdec_adapt *vdec)
{
	int i, ret;
	u32 frame_size = 0;
	u8 *p = vp9_trigger_header;

	for (i = 0; i < ARRAY_SIZE(vp9_trigger_framesize); i++) {
		frame_size = vp9_trigger_framesize[i];
		ret = vdec_vframe_write(vdec, p,
			frame_size, 0);
		v4l_dbg(vdec->ctx, V4L_DEBUG_CODEC_ERROR,
			"write trigger frame %d\n", ret);
		p += frame_size;
	}
}

static int vdec_write_nalu(struct vdec_vp9_inst *inst,
	u8 *buf, u32 size, u64 ts)
{
	int ret = 0;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	struct vp9_superframe_split s;
	u8 *data = NULL;
	u32 length = 0;
	bool need_prefix = vp9_need_prefix;

	memset(&s, 0, sizeof(s));

	/*trigger.*/
	if (0 && !need_trigger) {
		trigger_decoder(vdec);
		need_trigger = true;
	}

	if (need_prefix) {
		/*parse superframe.*/
		s.data = buf;
		s.data_size = size;
		ret = vp9_superframe_split_filter(&s);
		if (ret) {
			v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
				"parse frames failed.\n");
			return ret;
		}

		/*add headers.*/
		add_prefix_data(&s, &data, &length);
		ret = vdec_vframe_write(vdec, data, length, ts);
		vfree(data);
	} else {
		ret = vdec_vframe_write(vdec, buf, size, ts);
	}

	return ret;
}

static bool monitor_res_change(struct vdec_vp9_inst *inst, u8 *buf, u32 size)
{
	int ret = -1;
	u8 *p = buf;
	int len = size;
	u32 synccode = vp9_need_prefix ?
		((p[1] << 16) | (p[2] << 8) | p[3]) :
		((p[17] << 16) | (p[18] << 8) | p[19]);

	if (synccode == SYNC_CODE) {
		ret = parse_stream_cpu(inst, p, len);
		if (!ret && (inst->vsi->cur_pic.coded_width !=
			inst->vsi->pic.coded_width ||
			inst->vsi->cur_pic.coded_height !=
			inst->vsi->pic.coded_height)) {
			inst->vsi->cur_pic = inst->vsi->pic;
			return true;
		}
	}

	return false;
}

static int vdec_vp9_decode(unsigned long h_vdec, struct aml_vcodec_mem *bs,
			 u64 timestamp, bool *res_chg)
{
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	struct aml_vdec_adapt *vdec = &inst->vdec;
	u8 *buf = (u8 *) bs->vaddr;
	u32 size = bs->size;
	int ret = -1;

	if (bs == NULL)
		return -1;

	if (vdec_input_full(vdec)) {
		ATRACE_COUNTER("vdec_input_full", 0);
		return -EAGAIN;
	}

	if (inst->ctx->is_drm_mode) {
		if (bs->model == VB2_MEMORY_MMAP) {
			struct aml_video_stream *s =
				(struct aml_video_stream *) buf;

			if (s->magic != AML_VIDEO_MAGIC)
				return -1;

			if (!inst->ctx->param_sets_from_ucode &&
				(s->type == V4L_STREAM_TYPE_MATEDATA)) {
				if ((*res_chg = monitor_res_change(inst,
					s->data, s->len)))
				return 0;
			}

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
		/*checked whether the resolution changes.*/
		if ((!inst->ctx->param_sets_from_ucode) &&
			(*res_chg = monitor_res_change(inst, buf, size)))
			return 0;

		ret = vdec_write_nalu(inst, buf, size, timestamp);
	}
	ATRACE_COUNTER("v4l2_decode_write", ret);

	return ret;
}

 static void get_param_config_info(struct vdec_vp9_inst *inst,
	struct aml_dec_params *parms)
 {
	 if (inst->parms.parms_status & V4L2_CONFIG_PARM_DECODE_CFGINFO)
		 parms->cfg = inst->parms.cfg;
	 if (inst->parms.parms_status & V4L2_CONFIG_PARM_DECODE_PSINFO)
		 parms->ps = inst->parms.ps;
	 if (inst->parms.parms_status & V4L2_CONFIG_PARM_DECODE_HDRINFO)
		 parms->hdr = inst->parms.hdr;
	 if (inst->parms.parms_status & V4L2_CONFIG_PARM_DECODE_CNTINFO)
		 parms->cnt = inst->parms.cnt;

	 parms->parms_status |= inst->parms.parms_status;

	v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
		"parms status: %u\n", parms->parms_status);
 }

static int vdec_vp9_get_param(unsigned long h_vdec,
			       enum vdec_get_param_type type, void *out)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;

	if (!inst) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"the vp9 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		vdec_vp9_get_vf(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		ret = vdec_vp9_get_fb(inst, out);
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

	case GET_PARAM_CONFIG_INFO:
		get_param_config_info(inst, out);
		break;
	default:
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"invalid get parameter type=%d\n", type);
		ret = -EINVAL;
	}

	return ret;
}

static void set_param_write_sync(struct vdec_vp9_inst *inst)
{
	complete(&inst->comp);
}

static void set_param_ps_info(struct vdec_vp9_inst *inst,
	struct aml_vdec_ps_infos *ps)
{
	struct vdec_pic_info *pic = &inst->vsi->pic;
	struct vdec_vp9_dec_info *dec = &inst->vsi->dec;
	struct v4l2_rect *rect = &inst->vsi->crop;

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

	/* calc DPB size */
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
		ps->dpb_size);
}

static void set_param_hdr_info(struct vdec_vp9_inst *inst,
	struct aml_vdec_hdr_infos *hdr)
{
	if ((inst->parms.parms_status &
		V4L2_CONFIG_PARM_DECODE_HDRINFO)) {
		inst->parms.hdr = *hdr;
		inst->parms.parms_status |=
			V4L2_CONFIG_PARM_DECODE_HDRINFO;
		aml_vdec_dispatch_event(inst->ctx,
			V4L2_EVENT_SRC_CH_HDRINFO);
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
			"VP9 set HDR infos\n");
	}
}

static void set_param_post_event(struct vdec_vp9_inst *inst, u32 *event)
{
		aml_vdec_dispatch_event(inst->ctx, *event);
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_PRINFO,
			"VP9 post event: %d\n", *event);
}

static int vdec_vp9_set_param(unsigned long h_vdec,
	enum vdec_set_param_type type, void *in)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;

	if (!inst) {
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"the vp9 inst of dec is invalid.\n");
		return -1;
	}

	switch (type) {
	case SET_PARAM_WRITE_FRAME_SYNC:
		set_param_write_sync(inst);
		break;

	case SET_PARAM_PS_INFO:
		set_param_ps_info(inst, in);
		break;

	case SET_PARAM_HDR_INFO:
		set_param_hdr_info(inst, in);
		break;

	case SET_PARAM_POST_EVENT:
		set_param_post_event(inst, in);
		break;
	default:
		v4l_dbg(inst->ctx, V4L_DEBUG_CODEC_ERROR,
			"invalid set parameter type=%d\n", type);
		ret = -EINVAL;
	}

	return ret;
}

static struct vdec_common_if vdec_vp9_if = {
	.init		= vdec_vp9_init,
	.probe		= vdec_vp9_probe,
	.decode		= vdec_vp9_decode,
	.get_param	= vdec_vp9_get_param,
	.set_param	= vdec_vp9_set_param,
	.deinit		= vdec_vp9_deinit,
};

struct vdec_common_if *get_vp9_dec_comm_if(void);

struct vdec_common_if *get_vp9_dec_comm_if(void)
{
	return &vdec_vp9_if;
}

