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
#ifndef _AML_VCODEC_DRV_H_
#define _AML_VCODEC_DRV_H_

#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include "aml_vcodec_util.h"

#define AML_VCODEC_DRV_NAME	"aml_vcodec_drv"
#define AML_VCODEC_DEC_NAME	"aml-vcodec-dec"
#define AML_VCODEC_ENC_NAME	"aml-vcodec-enc"
#define AML_PLATFORM_STR	"platform:amlogic"

#define AML_VCODEC_MAX_PLANES	3
#define AML_V4L2_BENCHMARK	0
#define WAIT_INTR_TIMEOUT_MS	1000

/* codec types of get/set parms. */
#define V4L2_CONFIG_PARM_ENCODE		(0)
#define V4L2_CONFIG_PARM_DECODE		(1)

/* types of decode parms. */
#define V4L2_CONFIG_PARM_DECODE_CFGINFO	(1 << 0)
#define V4L2_CONFIG_PARM_DECODE_PSINFO	(1 << 1)
#define V4L2_CONFIG_PARM_DECODE_HDRINFO	(1 << 2)
#define V4L2_CONFIG_PARM_DECODE_CNTINFO	(1 << 3)

/* amlogic event define. */
/* #define V4L2_EVENT_SRC_CH_RESOLUTION	(1 << 0) */
#define V4L2_EVENT_SRC_CH_HDRINFO	(1 << 1)
#define V4L2_EVENT_SRC_CH_PSINFO	(1 << 2)
#define V4L2_EVENT_SRC_CH_CNTINFO	(1 << 3)

/* exception handing */
#define V4L2_EVENT_REQUEST_RESET	(1 << 8)
#define V4L2_EVENT_REQUEST_EXIT		(1 << 9)

/* eos event */
#define V4L2_EVENT_SEND_EOS		(1 << 16)

/* v4l buffer pool */
#define V4L_CAP_BUFF_MAX		(32)
#define V4L_CAP_BUFF_INVALID		(0)
#define V4L_CAP_BUFF_IN_M2M		(1)
#define V4L_CAP_BUFF_IN_DEC		(2)

/* v4l reset mode */
#define V4L_RESET_MODE_NORMAL		(1 << 0) /* reset vdec_input and decoder. */
#define V4L_RESET_MODE_LIGHT		(1 << 1) /* just only reset decoder. */

/* m2m job queue's status */
/* Instance is already queued on the job_queue */
#define TRANS_QUEUED		(1 << 0)
/* Instance is currently running in hardware */
#define TRANS_RUNNING		(1 << 1)
/* Instance is currently aborting */
#define TRANS_ABORT		(1 << 2)

/**
 * enum aml_hw_reg_idx - AML hw register base index
 */
enum aml_hw_reg_idx {
	VDEC_SYS,
	VDEC_MISC,
	VDEC_LD,
	VDEC_TOP,
	VDEC_CM,
	VDEC_AD,
	VDEC_AV,
	VDEC_PP,
	VDEC_HWD,
	VDEC_HWQ,
	VDEC_HWB,
	VDEC_HWG,
	NUM_MAX_VDEC_REG_BASE,
	/* h264 encoder */
	VENC_SYS = NUM_MAX_VDEC_REG_BASE,
	/* vp8 encoder */
	VENC_LT_SYS,
	NUM_MAX_VCODEC_REG_BASE
};

/**
 * enum aml_instance_type - The type of an AML Vcodec instance.
 */
enum aml_instance_type {
	AML_INST_DECODER		= 0,
	AML_INST_ENCODER		= 1,
};

/**
 * enum aml_instance_state - The state of an AML Vcodec instance.
 * @AML_STATE_IDLE	- default state when instance is created
 * @AML_STATE_INIT	- vcodec instance is initialized
 * @AML_STATE_PROBE	- vdec/venc had sps/pps header parsed/encoded
 * @AML_STATE_ACTIVE	- vdec is ready for work.
 * @AML_STATE_FLUSHING	- vdec is flushing. Only used by decoder
 * @AML_STATE_FLUSHED	- decoder has transacted the last frame.
 * @AML_STATE_RESET	- decoder has be reset after flush.
 * @AML_STATE_ABORT	- vcodec should be aborted
 */
enum aml_instance_state {
	AML_STATE_IDLE,
	AML_STATE_INIT,
	AML_STATE_PROBE,
	AML_STATE_READY,
	AML_STATE_ACTIVE,
	AML_STATE_FLUSHING,
	AML_STATE_FLUSHED,
	AML_STATE_RESET,
	AML_STATE_ABORT,
};

/**
 * struct aml_encode_param - General encoding parameters type
 */
enum aml_encode_param {
	AML_ENCODE_PARAM_NONE = 0,
	AML_ENCODE_PARAM_BITRATE = (1 << 0),
	AML_ENCODE_PARAM_FRAMERATE = (1 << 1),
	AML_ENCODE_PARAM_INTRA_PERIOD = (1 << 2),
	AML_ENCODE_PARAM_FORCE_INTRA = (1 << 3),
	AML_ENCODE_PARAM_GOP_SIZE = (1 << 4),
};

enum aml_fmt_type {
	AML_FMT_DEC = 0,
	AML_FMT_ENC = 1,
	AML_FMT_FRAME = 2,
};

/**
 * struct aml_video_fmt - Structure used to store information about pixelformats
 */
struct aml_video_fmt {
	u32	fourcc;
	enum aml_fmt_type	type;
	u32	num_planes;
};

/**
 * struct aml_codec_framesizes - Structure used to store information about
 *							framesizes
 */
struct aml_codec_framesizes {
	u32	fourcc;
	struct	v4l2_frmsize_stepwise	stepwise;
};

/**
 * struct aml_q_type - Type of queue
 */
enum aml_q_type {
	AML_Q_DATA_SRC = 0,
	AML_Q_DATA_DST = 1,
};


/**
 * struct aml_q_data - Structure used to store information about queue
 */
struct aml_q_data {
	unsigned int	visible_width;
	unsigned int	visible_height;
	unsigned int	coded_width;
	unsigned int	coded_height;
	enum v4l2_field	field;
	unsigned int	bytesperline[AML_VCODEC_MAX_PLANES];
	unsigned int	sizeimage[AML_VCODEC_MAX_PLANES];
	struct aml_video_fmt	*fmt;
	bool resolution_changed;
};

/**
 * struct aml_enc_params - General encoding parameters
 * @bitrate: target bitrate in bits per second
 * @num_b_frame: number of b frames between p-frame
 * @rc_frame: frame based rate control
 * @rc_mb: macroblock based rate control
 * @seq_hdr_mode: H.264 sequence header is encoded separately or joined
 *		  with the first frame
 * @intra_period: I frame period
 * @gop_size: group of picture size, it's used as the intra frame period
 * @framerate_num: frame rate numerator. ex: framerate_num=30 and
 *		   framerate_denom=1 menas FPS is 30
 * @framerate_denom: frame rate denominator. ex: framerate_num=30 and
 *		     framerate_denom=1 menas FPS is 30
 * @h264_max_qp: Max value for H.264 quantization parameter
 * @h264_profile: V4L2 defined H.264 profile
 * @h264_level: V4L2 defined H.264 level
 * @force_intra: force/insert intra frame
 */
struct aml_enc_params {
	unsigned int	bitrate;
	unsigned int	num_b_frame;
	unsigned int	rc_frame;
	unsigned int	rc_mb;
	unsigned int	seq_hdr_mode;
	unsigned int	intra_period;
	unsigned int	gop_size;
	unsigned int	framerate_num;
	unsigned int	framerate_denom;
	unsigned int	h264_max_qp;
	unsigned int	h264_profile;
	unsigned int	h264_level;
	unsigned int	force_intra;
};

/**
 * struct aml_vcodec_pm - Power management data structure
 */
struct aml_vcodec_pm {
	struct clk	*vdec_bus_clk_src;
	struct clk	*vencpll;

	struct clk	*vcodecpll;
	struct clk	*univpll_d2;
	struct clk	*clk_cci400_sel;
	struct clk	*vdecpll;
	struct clk	*vdec_sel;
	struct clk	*vencpll_d2;
	struct clk	*venc_sel;
	struct clk	*univpll1_d2;
	struct clk	*venc_lt_sel;
	struct device	*larbvdec;
	struct device	*larbvenc;
	struct device	*larbvenclt;
	struct device	*dev;
	struct aml_vcodec_dev	*amldev;
};

/**
 * struct vdec_pic_info  - picture size information
 * @visible_width: picture width
 * @visible_height: picture height
 * @coded_width: picture buffer width (64 aligned up from pic_w)
 * @coded_height: picture buffer heiht (64 aligned up from pic_h)
 * @y_bs_sz: Y bitstream size
 * @c_bs_sz: CbCr bitstream size
 * @y_len_sz: additional size required to store decompress information for y
 *		plane
 * @c_len_sz: additional size required to store decompress information for cbcr
 *		plane
 * E.g. suppose picture size is 176x144,
 *      buffer size will be aligned to 176x160.
 */
struct vdec_pic_info {
	unsigned int visible_width;
	unsigned int visible_height;
	unsigned int coded_width;
	unsigned int coded_height;
	unsigned int y_bs_sz;
	unsigned int c_bs_sz;
	unsigned int y_len_sz;
	unsigned int c_len_sz;
	int profile_idc;
	int ref_frame_count;
};

struct aml_vdec_cfg_infos {
	u32 double_write_mode;
	u32 init_width;
	u32 init_height;
	u32 ref_buf_margin;
	u32 canvas_mem_mode;
	u32 canvas_mem_endian;
};

struct aml_vdec_hdr_infos {
	/*
	 * bit 29   : present_flag
	 * bit 28-26: video_format "component", "PAL", "NTSC", "SECAM", "MAC", "unspecified"
	 * bit 25   : range "limited", "full_range"
	 * bit 24   : color_description_present_flag
	 * bit 23-16: color_primaries "unknown", "bt709", "undef", "bt601",
	 *            "bt470m", "bt470bg", "smpte170m", "smpte240m", "film", "bt2020"
	 * bit 15-8 : transfer_characteristic unknown", "bt709", "undef", "bt601",
	 *            "bt470m", "bt470bg", "smpte170m", "smpte240m",
	 *            "linear", "log100", "log316", "iec61966-2-4",
	 *            "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12",
	 *            "smpte-st-2084", "smpte-st-428"
	 * bit 7-0  : matrix_coefficient "GBR", "bt709", "undef", "bt601",
	 *            "fcc", "bt470bg", "smpte170m", "smpte240m",
	 *            "YCgCo", "bt2020nc", "bt2020c"
	 */
	u32 signal_type;
	struct vframe_master_display_colour_s color_parms;
};

struct aml_vdec_ps_infos {
	u32 visible_width;
	u32 visible_height;
	u32 coded_width;
	u32 coded_height;
	u32 profile;
	u32 mb_width;
	u32 mb_height;
	u32 dpb_size;
	u32 ref_frames;
	u32 reorder_frames;
};

struct aml_vdec_cnt_infos {
	u32 bit_rate;
	u32 frame_count;
	u32 error_frame_count;
	u32 drop_frame_count;
	u32 total_data;
};

struct aml_dec_params {
	u32 parms_status;
	struct aml_vdec_cfg_infos	cfg;
	struct aml_vdec_ps_infos	ps;
	struct aml_vdec_hdr_infos	hdr;
	struct aml_vdec_cnt_infos	cnt;
};

struct v4l2_config_parm {
	u32 type;
	u32 length;
	union {
		struct aml_dec_params dec;
		struct aml_enc_params enc;
		u8 data[200];
	} parm;
	u8 buf[4096];
};

struct v4l_buff_pool {
	/*
	 * bit 31-16: buffer state
	 * bit 15- 0: buffer index
	 */
	u32 seq[V4L_CAP_BUFF_MAX];
	u32 in, out;
};

enum aml_thread_type {
	AML_THREAD_OUTPUT,
	AML_THREAD_CAPTURE,
};

typedef void (*aml_thread_func)(struct aml_vcodec_ctx *ctx);

struct aml_vdec_thread {
	struct list_head node;
	spinlock_t lock;
	struct semaphore sem;
	struct task_struct *task;
	enum aml_thread_type type;
	void *priv;
	int stop;

	aml_thread_func func;
};

/**
 * struct aml_vcodec_ctx - Context (instance) private data.
 *
 * @id: index of the context that this structure describes.
 * @type: type of the instance - decoder or encoder.
 * @dev: pointer to the aml_vcodec_dev of the device.
 * @m2m_ctx: pointer to the v4l2_m2m_ctx of the context.
 * @ada_ctx: pointer to the aml_vdec_adapt of the context.
 * @dec_if: hooked decoder driver interface.
 * @drv_handle: driver handle for specific decode instance
 * @fh: struct v4l2_fh.
 * @ctrl_hdl: handler for v4l2 framework.
 * @slock: protect v4l2 codec context.
 * @empty_flush_buf: a fake size-0 capture buffer that indicates flush.
 * @list: link to ctx_list of aml_vcodec_dev.
 * @q_data: store information of input and output queue of the context.
 * @queue: waitqueue that can be used to wait for this context to finish.
 * @lock: protect the vdec thread.
 * @state_lock: protect the codec status.
 * @state: state of the context.
 * @decode_work: decoder work be used to output buffer.
 * @output_thread_ready: indicate the output thread ready.
 * @cap_pool: capture buffers are remark in the pool.
 * @vdec_thread_list: vdec thread be used to capture.
 * @dpb_size: store dpb count after header parsing
 * @param_change: indicate encode parameter type
 * @param_sets_from_ucode: if true indicate ps from ucode.
 * @v4l_codec_dpb_ready: queue buffer number greater than dpb.
 * @comp: comp be used for sync picture information with decoder.
 * @config: used to set or get parms for application.
 * @picinfo: store picture info after header parsing.
 * @last_decoded_picinfo: pic information get from latest decode.
 * @colorspace: enum v4l2_colorspace; supplemental to pixelformat.
 * @ycbcr_enc: enum v4l2_ycbcr_encoding, Y'CbCr encoding.
 * @quantization: enum v4l2_quantization, colorspace quantization.
 * @xfer_func: enum v4l2_xfer_func, colorspace transfer function.
 * @cap_pix_fmt: the picture format used to switch nv21 or nv12.
 * @has_receive_eos: if receive last frame of capture that be set.
 * @is_drm_mode: decoding work on drm mode if that set.
 * @is_stream_mode: vdec input used to stream mode, default frame mode.
 * @is_stream_off: the value used to handle reset active.
 * @receive_cmd_stop: if receive the cmd flush decoder.
 * @reset_flag: reset mode includes lightly and normal mode.
 * @decoded_frame_cnt: the capture buffer deque number to be count.
 * @buf_used_count: means that decode allocate how many buffs from v4l.
 */
struct aml_vcodec_ctx {
	int				id;
	enum aml_instance_type		type;
	struct aml_vcodec_dev		*dev;
	struct v4l2_m2m_ctx		*m2m_ctx;
	struct aml_vdec_adapt		*ada_ctx;
	const struct vdec_common_if	*dec_if;
	ulong				drv_handle;
	struct v4l2_fh			fh;
	struct v4l2_ctrl_handler	ctrl_hdl;
	spinlock_t			slock;
	struct aml_video_dec_buf	*empty_flush_buf;
	struct list_head		list;

	struct aml_q_data		q_data[2];
	wait_queue_head_t		queue;
	struct mutex			lock, state_lock;
	enum aml_instance_state		state;
	struct work_struct		decode_work;
	bool				output_thread_ready;
	struct v4l_buff_pool		cap_pool;
	struct list_head		vdec_thread_list;

	int				dpb_size;
	bool				param_sets_from_ucode;
	bool				v4l_codec_dpb_ready;
	struct completion		comp;
	struct v4l2_config_parm		config;
	struct vdec_pic_info		picinfo;
	struct vdec_pic_info		last_decoded_picinfo;
	enum v4l2_colorspace		colorspace;
	enum v4l2_ycbcr_encoding	ycbcr_enc;
	enum v4l2_quantization		quantization;
	enum v4l2_xfer_func		xfer_func;
	u32				cap_pix_fmt;

	bool				has_receive_eos;
	bool				is_drm_mode;
	bool				output_dma_mode;
	bool				is_stream_off;
	bool				receive_cmd_stop;
	int				reset_flag;
	int				decoded_frame_cnt;
	int				buf_used_count;
};

/**
 * struct aml_vcodec_dev - driver data
 * @v4l2_dev: V4L2 device to register video devices for.
 * @vfd_dec: Video device for decoder
 * @vfd_enc: Video device for encoder.
 *
 * @m2m_dev_dec: m2m device for decoder
 * @m2m_dev_enc: m2m device for encoder.
 * @plat_dev: platform device
 * @vpu_plat_dev: aml vpu platform device
 * @alloc_ctx: VB2 allocator context
 *	       (for allocations without kernel mapping).
 * @ctx_list: list of struct aml_vcodec_ctx
 * @irqlock: protect data access by irq handler and work thread
 * @curr_ctx: The context that is waiting for codec hardware
 *
 * @reg_base: Mapped address of AML Vcodec registers.
 *
 * @id_counter: used to identify current opened instance
 *
 * @encode_workqueue: encode work queue
 *
 * @int_cond: used to identify interrupt condition happen
 * @int_type: used to identify what kind of interrupt condition happen
 * @dev_mutex: video_device lock
 * @queue: waitqueue for waiting for completion of device commands
 *
 * @dec_irq: decoder irq resource
 * @enc_irq: h264 encoder irq resource
 * @enc_lt_irq: vp8 encoder irq resource
 *
 * @dec_mutex: decoder hardware lock
 * @enc_mutex: encoder hardware lock.
 *
 * @pm: power management control
 * @dec_capability: used to identify decode capability, ex: 4k
 * @enc_capability: used to identify encode capability
 */
struct aml_vcodec_dev {
	struct v4l2_device v4l2_dev;
	struct video_device *vfd_dec;
	struct video_device *vfd_enc;
	struct file *filp;

	struct v4l2_m2m_dev *m2m_dev_dec;
	struct v4l2_m2m_dev *m2m_dev_enc;
	struct platform_device *plat_dev;
	struct platform_device *vpu_plat_dev;//??
	struct vb2_alloc_ctx *alloc_ctx;//??
	struct list_head ctx_list;
	spinlock_t irqlock;
	struct aml_vcodec_ctx *curr_ctx;
	void __iomem *reg_base[NUM_MAX_VCODEC_REG_BASE];

	unsigned long id_counter;

	struct workqueue_struct *decode_workqueue;
	struct workqueue_struct *encode_workqueue;
	int int_cond;
	int int_type;
	struct mutex dev_mutex;
	wait_queue_head_t queue;

	int dec_irq;
	int enc_irq;
	int enc_lt_irq;

	struct mutex dec_mutex;
	struct mutex enc_mutex;

	struct aml_vcodec_pm pm;
	unsigned int dec_capability;
	unsigned int enc_capability;
};

static inline struct aml_vcodec_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct aml_vcodec_ctx, fh);
}

static inline struct aml_vcodec_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct aml_vcodec_ctx, ctrl_hdl);
}

#endif /* _AML_VCODEC_DRV_H_ */
