#include "vdec_v4l2_buffer_ops.h"
#include <media/v4l2-mem2mem.h>
#include <linux/printk.h>

int vdec_v4l_get_buffer(struct aml_vcodec_ctx *ctx,
	struct vdec_v4l2_buffer **out)
{
	int ret = -1;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->get_param(ctx->drv_handle,
		GET_PARAM_FREE_FRAME_BUFFER, out);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_get_buffer);

int vdec_v4l_get_pic_info(struct aml_vcodec_ctx *ctx,
	struct vdec_pic_info *pic)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->get_param(ctx->drv_handle,
		GET_PARAM_PIC_INFO, pic);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_get_pic_info);

int vdec_v4l_set_ps_infos(struct aml_vcodec_ctx *ctx,
	struct aml_vdec_ps_infos *ps)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_PS_INFO, ps);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_set_ps_infos);

int vdec_v4l_set_hdr_infos(struct aml_vcodec_ctx *ctx,
	struct aml_vdec_hdr_infos *hdr)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_HDR_INFO, hdr);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_set_hdr_infos);

static void aml_wait_dpb_ready(struct aml_vcodec_ctx *ctx)
{
	ulong expires;

	expires = jiffies + msecs_to_jiffies(1000);
	while (!ctx->v4l_codec_dpb_ready) {
		u32 ready_num = 0;

		if (time_after(jiffies, expires)) {
			pr_err("the DPB state has not ready.\n");
			break;
		}

		ready_num = v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx);
		if ((ready_num + ctx->buf_used_count) >= ctx->dpb_size)
			ctx->v4l_codec_dpb_ready = true;
	}
}

void aml_vdec_pic_info_update(struct aml_vcodec_ctx *ctx)
{
	unsigned int dpbsize = 0;
	int ret;

	if (ctx->dec_if->get_param(ctx->drv_handle, GET_PARAM_PIC_INFO, &ctx->last_decoded_picinfo)) {
		pr_err("Cannot get param : GET_PARAM_PICTURE_INFO ERR\n");
		return;
	}

	if (ctx->last_decoded_picinfo.visible_width == 0 ||
		ctx->last_decoded_picinfo.visible_height == 0 ||
		ctx->last_decoded_picinfo.coded_width == 0 ||
		ctx->last_decoded_picinfo.coded_height == 0) {
		pr_err("Cannot get correct pic info\n");
		return;
	}

	ret = ctx->dec_if->get_param(ctx->drv_handle, GET_PARAM_DPB_SIZE, &dpbsize);
	if (dpbsize == 0)
		pr_err("Incorrect dpb size, ret=%d\n", ret);

	/* update picture information */
	ctx->dpb_size = dpbsize;
	ctx->picinfo = ctx->last_decoded_picinfo;
}

int vdec_v4l_post_evet(struct aml_vcodec_ctx *ctx, u32 event)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;
	if (event == 1)
		ctx->reset_flag = 2;
	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_POST_EVENT, &event);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_post_evet);

int vdec_v4l_res_ch_event(struct aml_vcodec_ctx *ctx)
{
	int ret = 0;
	struct aml_vcodec_dev *dev = ctx->dev;

	if (ctx->drv_handle == 0)
		return -EIO;

	/* wait the DPB state to be ready. */
	aml_wait_dpb_ready(ctx);

	aml_vdec_pic_info_update(ctx);

	mutex_lock(&ctx->state_lock);

	ctx->state = AML_STATE_FLUSHING;/*prepare flushing*/

	pr_info("[%d]: vcodec state (AML_STATE_FLUSHING-RESCHG)\n", ctx->id);

	mutex_unlock(&ctx->state_lock);

	ctx->q_data[AML_Q_DATA_SRC].resolution_changed = true;
	v4l2_m2m_job_pause(dev->m2m_dev_dec, ctx->m2m_ctx);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_res_ch_event);


int vdec_v4l_write_frame_sync(struct aml_vcodec_ctx *ctx)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_WRITE_FRAME_SYNC, NULL);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_write_frame_sync);

