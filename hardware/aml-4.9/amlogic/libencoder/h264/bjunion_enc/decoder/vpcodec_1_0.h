#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_t long

typedef enum vl_codec_id_e {
	CODEC_ID_NONE,
	CODEC_ID_VP8,
	CODEC_ID_H261,
    CODEC_ID_H263,
	CODEC_ID_H264, /* 必须支持 */
	CODEC_ID_H265,

}vl_codec_id_t;

typedef enum vl_img_format_e {
	IMG_FMT_NONE,
	IMG_FMT_NV12, /* 必须支持 */

}vl_img_format_t;

typedef enum vl_frame_type_e {
	FRAME_TYPE_NONE,
	FRAME_TYPE_AUTO, /* 编码器自动控制（默认） */
	FRAME_TYPE_IDR,
	FRAME_TYPE_I,
	FRAME_TYPE_P,

}vl_frame_type_t;

/**
 * 获取版本信息
 *
 *@return : 版本信息
 */
const char * vl_get_version();

/**
 * 初始化视频编码器
 *
 *@param : codec_id 编码类型
 *@param : width 视频宽度
 *@param : height 视频高度
 *@param : frame_rate 帧率
 *@param : bit_rate 码率
 *@param : gop GOP值:I帧最大间隔
 *@param : img_format 图像格式
 *@return : 成功返回编码器handle，失败返回 <= 0
 */
vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format);

/**
 * 视频编码
 *
 *@param : handle 编码器handle
 *@param : type 帧类型
 *@param : in 待编码的数据
 *@param : in_size 待编码的数据长度
 *@param : out 编码后的数据,H.264需要包含(0x00，0x00，0x00，0x01)起始码， 且必须为 I420格式。（out的空间在apk层分配，通过jni传递进来，直接在函数内修改out的数据，不要修改out指向的地址）
 *@return ：成功返回编码后的数据长度，失败返回 <= 0
 */
int vl_video_encoder_encode(vl_codec_handle_t handle, vl_frame_type_t type, char * in, int in_size, char ** out);

/**
 * 销毁编码器
 *
 *@param ：handle 视频编码器handle
 *@return ：成功返回1，失败返回0
 */
int vl_video_encoder_destory(vl_codec_handle_t handle);

/**
 * 初始化解码器
 *
 *@param : codec_id 解码器类型
 *@return : 成功返回解码器handle，失败返回 <= 0
 */
vl_codec_handle_t vl_video_decoder_init(vl_codec_id_t codec_id);

/**
 * 视频解码
 *
 *@param : handle 视频解码器handle
 *@param : in 待解码的数据
 *@param : in_size 待解码的数据长度
 *@param : out 解码后的数据，内部分配空间
 *@return ：成功返回解码后的数据长度，失败返回 <= 0
 */
int vl_video_decoder_decode(vl_codec_handle_t handle, char * in, int in_size, char ** out);

/**
 * 销毁解码器
 *@param : handle 视频解码器handle
 *@return ：成功返回1，失败返回0
 */
int vl_video_decoder_destory(vl_codec_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */

