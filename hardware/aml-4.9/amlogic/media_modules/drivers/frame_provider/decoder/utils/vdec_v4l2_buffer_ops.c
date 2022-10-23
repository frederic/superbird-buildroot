#include "vdec_v4l2_buffer_ops.h"

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

