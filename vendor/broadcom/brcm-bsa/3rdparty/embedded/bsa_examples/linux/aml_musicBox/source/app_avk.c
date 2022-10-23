/*****************************************************************************
 **
 **  Name:           app_avk.c
 **
 **  Description:    Bluetooth Manager application
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
/*file I/O*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef PCM_ALSA_OPEN_BLOCKING
#include <pthread.h>
#endif
#include "buildcfg.h"

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_wav.h"
#ifdef PCM_ALSA
#include "alsa/asoundlib.h"
#define APP_AVK_ASLA_DEV "default"
#endif

#include "app_avk.h"
#include "ring_buffer.h"
#include "alsa_volume.h"
#ifdef ENABLE_AUDIOSERVICE
#include "as_client.h"
#endif
/*
 * Defines
 */
#ifndef BSA_AVK_AAC_SUPPORTED
#define BSA_AVK_AAC_SUPPORTED FALSE
#endif

#if (BSA_AVK_AAC_SUPPORTED == TRUE)
#include <fdk-aac/aacdecoder_lib.h>
HANDLE_AACDECODER aacHandle = NULL;
AAC_DECODER_ERROR aacErr;

#define AAC_INBUF_SIZE  2048
#define AAC_OUTBUF_SIZE 2048 *2
static uint8_t aacInBuf[AAC_INBUF_SIZE];
static int16_t aacOutBuf[AAC_OUTBUF_SIZE];

#define AAC_DEBUG 0
#endif

#define APP_XML_REM_DEVICES_FILE_PATH "./bt_devices.xml"
#define APP_ALSA_DEVICE_CONF_PATH "/etc/alsa_bsa.conf"
#define APP_AVK_SOUND_FILE "test_avk"

#ifndef BSA_AVK_SECURITY
#define BSA_AVK_SECURITY    BSA_SEC_AUTHORIZATION
#endif
#ifndef BSA_AVK_FEATURES
#define BSA_AVK_FEATURES    (BSA_AVK_FEAT_RCCT|BSA_AVK_FEAT_RCTG|\
                             BSA_AVK_FEAT_VENDOR|BSA_AVK_FEAT_METADATA|\
                             BSA_AVK_FEAT_BROWSE|BSA_AVK_FEAT_DELAY_RPT)
#endif

#ifndef BSA_AVK_DUMP_RX_DATA
#define BSA_AVK_DUMP_RX_DATA FALSE
#endif


/* bitmask of events that BSA client is interested in registering for notifications */
tBSA_AVK_REG_NOTIFICATIONS reg_notifications =
	(1 << (BSA_AVK_RC_EVT_PLAY_STATUS_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_TRACK_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_TRACK_REACHED_END - 1)) |
	(1 << (BSA_AVK_RC_EVT_TRACK_REACHED_START - 1)) |
	(1 << (BSA_AVK_RC_EVT_PLAY_POS_CHANGED - 1)) |
	(1 << (BSA_AVK_RC_EVT_BATTERY_STATUS_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_SYSTEM_STATUS_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_APP_SETTING_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_NOW_PLAYING_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_AVAL_PLAYERS_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_ADDR_PLAYER_CHANGE - 1)) |
	(1 << (BSA_AVK_RC_EVT_UIDS_CHANGE - 1));

/*
 * global variable
 */

tAPP_AVK_CB app_avk_cb;

#ifdef CAL_DATA
static int accu = 0;
static long long first_timestamp = 0 , second_timestamp = 0 ;
static long length = 0;
static long accu_data = 0;
#endif

#ifdef USE_RING_BUFFER
static tRingBuffer rb;
static int startPlay = 0;
static pthread_t th_play_data;

#define RBUF_SIZE 512 * 1024
#define PERIOD 128
#define START_TH 55000
#endif


#ifdef PCM_ALSA

static char *alsa_device;
#if defined(__LP64__)
//static char *alsa_device = "dmixer_auto"; /* ALSA playback device */
#else
//static char *alsa_device = "dmixer_avs_auto"; /* ALSA playback device */
#endif

#ifdef PCM_ALSA_OPEN_BLOCKING
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


static snd_mixer_t *mixerFd;
#ifdef ENABLE_AUDIOSERVICE
static double vol_backup;
#else
static unsigned int vol_backup;
#endif
#define MIXER_NAME "default"
#define ELEM_NAME  "Master"

static UINT16 saved_sample_rate = 44100;
#endif /* PCM_ALSA */

/*
 * Local functions
 */
static void app_avk_close_wave_file(tAPP_AVK_CONNECTION *connection);
static void app_avk_create_wave_file(void);
static void app_avk_uipc_cback(BT_HDR *p_msg);

#ifdef PCM_ALSA_OPEN_BLOCKING
static tAPP_AVK_CONNECTION* pStreamingConn = NULL;
#endif
/*******************************************************************************
 **
 ** Function         app_avk_get_alsa_device_conf
 **
 ** Description      get alsa device by conf file
 **
 ** Returns           Char*
 **
 *******************************************************************************/

void app_avk_get_alsa_device_conf()
{
	FILE *fp;
	//struct stat st;
	static char tmp[100];
	int i = 0;

	fp = fopen(APP_ALSA_DEVICE_CONF_PATH, "r");
	if (fp  == NULL) {
		APP_ERROR0("open APP ALSA DEVICE CONF error");
		return;
	}

	fgets(tmp, sizeof(tmp), fp);
	while (tmp[i++] != '=');
	alsa_device = strdup(tmp + i);
	*(alsa_device + strlen(alsa_device) - 1) = '\0';
	fclose(fp);

}


#if (BSA_AVK_AAC_SUPPORTED == TRUE)
/*******************************************************************************
 **
 ** Function         aacdec_strerror
 **
 ** Description      format aac errorno to string, for debug
 **
 **
 *******************************************************************************/
const char *aacdec_strerror(AAC_DECODER_ERROR err) {
	switch (err) {
	case AAC_DEC_OK:
		return "Success";
	case AAC_DEC_OUT_OF_MEMORY:
		return "Out of memory";
	case AAC_DEC_TRANSPORT_SYNC_ERROR:
		return "Transport sync error";
	case AAC_DEC_NOT_ENOUGH_BITS:
		return "Not enough bits";
	case AAC_DEC_INVALID_HANDLE:
		return "Invalid handle";
	case AAC_DEC_UNSUPPORTED_AOT:
		return "Unsupported AOT";
	case AAC_DEC_UNSUPPORTED_FORMAT:
		return "Unsupported format";
	case AAC_DEC_UNSUPPORTED_ER_FORMAT:
		return "Unsupported ER format";
	case AAC_DEC_UNSUPPORTED_EPCONFIG:
		return "Unsupported EP format";
	case AAC_DEC_UNSUPPORTED_MULTILAYER:
		return "Unsupported multilayer";
	case AAC_DEC_UNSUPPORTED_CHANNELCONFIG:
		return "Unsupported channels";
	case AAC_DEC_UNSUPPORTED_SAMPLINGRATE:
		return "Unsupported sample rate";
	case AAC_DEC_INVALID_SBR_CONFIG:
		return "Unsupported SBR";
	case AAC_DEC_SET_PARAM_FAIL:
		return "Unsupported parameter";
	case AAC_DEC_NEED_TO_RESTART:
		return "Restart required";
	case AAC_DEC_TRANSPORT_ERROR:
		return "Transport error";
	case AAC_DEC_PARSE_ERROR:
		return "Parse error";
	case AAC_DEC_UNSUPPORTED_EXTENSION_PAYLOAD:
		return "Unsupported extension payload";
	case AAC_DEC_DECODE_FRAME_ERROR:
		return "Bitstream corrupted";
	case AAC_DEC_CRC_ERROR:
		return "CRC mismatch";
	case AAC_DEC_INVALID_CODE_BOOK:
		return "Invalid codebook";
	case AAC_DEC_UNSUPPORTED_PREDICTION:
		return "Unsupported prediction";
	case AAC_DEC_UNSUPPORTED_CCE:
		return "Unsupported CCE";
	case AAC_DEC_UNSUPPORTED_LFE:
		return "Unsupported LFE";
	case AAC_DEC_UNSUPPORTED_GAIN_CONTROL_DATA:
		return "Unsupported gain control data";
	case AAC_DEC_UNSUPPORTED_SBA:
		return "Unsupported SBA";
	case AAC_DEC_TNS_READ_ERROR:
		return "TNS read error";
	case AAC_DEC_RVLC_ERROR:
		return "RVLC decode error";
	case AAC_DEC_ANC_DATA_ERROR:
		return "Ancillary data error";
	case AAC_DEC_TOO_SMALL_ANC_BUFFER:
		return "Too small ancillary buffer";
	case AAC_DEC_TOO_MANY_ANC_ELEMENTS:
		return "Too many ancillary elements";
	default:
		APP_DEBUG1("Unknown error code: %#x", err);
		return "Unknown error";
	}
}
#endif

/*******************************************************************************
 **
 ** Function         app_avk_get_label
 **
 ** Description      get transaction label (used to distinguish transactions)
 **
 ** Returns          UINT8
 **
 *******************************************************************************/
UINT8 app_avk_get_label()
{
	if (app_avk_cb.label >= 15)
		app_avk_cb.label = 0;
	return app_avk_cb.label++;
}

/*******************************************************************************
 **
 ** Function         app_avk_create_wave_file
 **
 ** Description     create a new wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_create_wave_file(void)
{
	int file_index = 0;
	int fd;
	char filename[200];

#ifdef PCM_ALSA
	return;
#endif

	/* If a wav file is currently open => close it */
	if (app_avk_cb.fd != -1)
	{
		tAPP_AVK_CONNECTION dummy;
		memset (&dummy, 0, sizeof(tAPP_AVK_CONNECTION));
		app_avk_close_wave_file(&dummy);
	}

	do
	{
		snprintf(filename, sizeof(filename), "%s-%d.wav", APP_AVK_SOUND_FILE, file_index);
		filename[sizeof(filename) - 1] = '\0';
		fd = app_wav_create_file(filename, O_EXCL);
		file_index++;
	} while (fd < 0);

	app_avk_cb.fd = fd;
}
/*******************************************************************************
 **
 ** Function         app_avk_create_aac_file
 **
 ** Description     create a new wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_create_aac_file(void)
{
	int file_index = 0;
	int fd;
	char filename[200];

	/* If a wav file is currently open => close it */
	if (app_avk_cb.fd != -1)
	{
//        close(app_avk_cb.fd);
//        app_avk_cb.fd = -1;
		return;
	}

	do
	{
		int flags = O_EXCL | O_RDWR | O_CREAT | O_TRUNC;

		snprintf(filename, sizeof(filename), "%s-%d.aac", APP_AVK_SOUND_FILE, file_index);
		filename[sizeof(filename) - 1] = '\0';

		/* Create file in read/write mode, reset the length field */
		fd = open(filename, flags, 0666);
		if (fd < 0)
		{
			APP_ERROR1("open(%s) failed: %d", filename, errno);
		}

		file_index++;
	} while ((fd < 0) && (file_index < 100));

	app_avk_cb.fd_codec_type = BSA_AVK_CODEC_M24;
	app_avk_cb.fd = fd;
}

/*******************************************************************************
 **
 ** Function         app_avk_close_wave_file
 **
 ** Description     proper header and close the wave file
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_close_wave_file(tAPP_AVK_CONNECTION *connection)
{
	tAPP_WAV_FILE_FORMAT format;

#ifdef PCM_ALSA
	return;
#endif

	if (app_avk_cb.fd == -1)
	{
		printf("app_avk_close_wave_file: no file to close\n");
		return;
	}

	format.bits_per_sample = connection->bit_per_sample;
	format.nb_channels = connection->num_channel;
	format.sample_rate = connection->sample_rate;

	app_wav_close_file(app_avk_cb.fd, &format);

	app_avk_cb.fd = -1;
}

/*******************************************************************************
**
** Function         app_avk_end
**
** Description      This function is used to close AV
**
** Returns          void
**
*******************************************************************************/
void app_avk_end(void)
{
	tBSA_AVK_DISABLE disable_param;
	tBSA_AVK_DEREGISTER deregister_param;

	/* deregister avk */
	BSA_AvkDeregisterInit(&deregister_param);
	BSA_AvkDeregister(&deregister_param);

	/* disable avk */
	BSA_AvkDisableInit(&disable_param);
	BSA_AvkDisable(&disable_param);

#ifdef PCM_ALSA
	mixer_dealloc(mixerFd);
#endif

#ifdef USE_RING_BUFFER
	APP_DEBUG0("Ring Buffer delinit");
	ring_buffer_clear(&rb);
	pthread_cancel(th_play_data);
	ring_buffer_delinit(&rb);
#endif

}

// Check whether does current system use pulse or not
int app_avk_is_pulse(void)
{
#ifdef PCM_ALSA
#ifdef ENABLE_AUDIOSERVICE
	return 1;
#else
	//compare the alsa_device
	if (!strncmp(alsa_device, "pulse", 5))
		return 1;
	else
		return 0;
#endif
#else
	// If there is no PCM_ALSA, always return false
	return 0;
#endif
}

#ifdef USE_RING_BUFFER
#ifdef PCM_ALSA
#ifdef ENABLE_AUDIOSERVICE
typedef struct alsa_param {
	snd_pcm_t *handle;
	char *audiobuf;
	snd_pcm_format_t format;
	size_t bitwidth;
	int buffer_size;
	unsigned int channels;
	unsigned int rate;
	size_t chunk_bytes;
} alsa_param_t;

static alsa_param_t *as_alsa_handle = NULL;


int set_alsa_params(alsa_param_t *alsa_params)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t chunk_size = 1024;
	unsigned period_time = 0;
	unsigned buffer_time = 0;
	snd_pcm_uframes_t period_frames = 0;
	snd_pcm_uframes_t buffer_frames = 0;

	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_t *handle = alsa_params->handle;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);

	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		APP_ERROR0("Broken configuration for this PCM: no configurations available\n");
		return err;
	}

	err = snd_pcm_hw_params_set_access(handle, params,
			SND_PCM_ACCESS_RW_INTERLEAVED);

	if (err < 0) {
		APP_ERROR0("Access type not available\n");
		return err;
	}

	// printf("Set hw format %d\n", alsa_params->format);
	err = snd_pcm_hw_params_set_format(handle, params, alsa_params->format);
	if (err < 0) {
		APP_ERROR0("Sample format non available\n");
		return err;
	}

	// printf("Set channels %d\n", alsa_params->channels);
	err = snd_pcm_hw_params_set_channels(handle, params, alsa_params->channels);
	if (err < 0) {
		APP_ERROR0("Channels count non available\n");
		return err;
	}

	// printf("before set rate near %d\n", alsa_params->rate);
	rate = alsa_params->rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &alsa_params->rate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < alsa_params->rate || (float)rate * 0.95 > alsa_params->rate) {
		APP_ERROR1("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n",
				rate, alsa_params->rate);
	}
	rate = alsa_params->rate;
	// printf("after set rate near %d\n", alsa_params->rate);

	err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
	assert(err >= 0);
	if (buffer_time > 500000)
		buffer_time = 500000;

	if (buffer_time > 0)
		period_time = buffer_time / 4;
	else
		period_frames = buffer_frames / 4;

	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle, params,
				&period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle, params,
				&period_frames, 0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
				&buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
				&buffer_frames);
	}
	assert(err >= 0);

	alsa_params->bitwidth = snd_pcm_format_physical_width(alsa_params->format);
	//monotonic = snd_pcm_hw_params_is_monotonic(params);
	//can_pause = snd_pcm_hw_params_can_pause(params);
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		APP_ERROR0("Unable to install hw params:\n");
		return err;
	}
#if 1
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		APP_ERROR1("Can't use period equal to buffer size (%lu == %lu)\n",
				chunk_size, buffer_size);
		return err;
	}
#endif
	snd_pcm_sw_params_current(handle, swparams);
	n = chunk_size;

	err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);

	/* round up to closest transfer boundary */
	n = buffer_size;
	start_threshold = n;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
	assert(err >= 0);
	stop_threshold = buffer_size;
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		APP_ERROR0("unable to install sw params\n");
		return err;
	}

	alsa_params->chunk_bytes = chunk_size * alsa_params->channels * (alsa_params->bitwidth >> 3);
	alsa_params->audiobuf = malloc(alsa_params->chunk_bytes);
	if (alsa_params->audiobuf == NULL) {
		APP_ERROR0("not enough memory\n");
		return -1;
	}

	APP_DEBUG1("malloc audio buffer size = %d\n", alsa_params->chunk_bytes);
	buffer_frames = buffer_size;	/* for position test */

	return 0;
}


int standard_alsa_output_open(void **handle, char *device_name,
		unsigned int channels, unsigned int rate,
		snd_pcm_format_t format) {
	int ret = -1;
	alsa_param_t *alsa_param;
	int block = 1;
	//char * device_name = NULL;
	alsa_param = (alsa_param_t *)calloc(1, sizeof(alsa_param_t));
	if (alsa_param == NULL) {
		printf("malloc alsa_param failed\n");
		*handle = NULL;
		return -1;
	}

	if (channels == 0 || rate == 0) {
		printf("Wrong Input parameter ch=%d rate=%d\n",
				channels, rate);
		return -1;
	}

	alsa_param->channels = channels;
	alsa_param->rate     = rate;
	alsa_param->format = format;

	printf("Open ALSA device=%s rate=%d ch=%d format=%d\n", device_name, alsa_param->rate, alsa_param->channels, alsa_param->format);
	ret = snd_pcm_open(&alsa_param->handle, device_name, SND_PCM_STREAM_PLAYBACK, block ? 0 : SND_PCM_NONBLOCK);

	if (ret < 0) {
		printf("[%s::%d]--[audio open error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
		return -1;
	} else {
		printf("[%s::%d]--[audio open(snd_pcm_open) successfully]\n", __FUNCTION__, __LINE__);
	}

	ret  = set_alsa_params(alsa_param);

	if (ret < 0) {
		printf("set_params error\n");
		snd_pcm_close(alsa_param->handle);
		free(alsa_param);
		*handle = NULL;
		return -1;
	}


	*handle = alsa_param;
	return ret;
}

void standard_alsa_output_close(void *handle)
{
	int ret = -1;
	alsa_param_t *alsa_param = (alsa_param_t *)handle;
	snd_pcm_sframes_t delay = 0;

	if (handle == NULL) {
		return;
	}

	snd_pcm_delay(alsa_param->handle, &delay);
	snd_pcm_drain(alsa_param->handle);
	//snd_pcm_drop(alsa_param->handle);
	ret = snd_pcm_close(alsa_param->handle);
	if (ret < 0) {
		printf("[%s::%d]--[audio close error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
	}
	free(alsa_param->audiobuf);
	free(alsa_param);

	return;
}

size_t standard_alsa_output_write(void *handle, const void *buffer, size_t bytes)
{
	snd_pcm_sframes_t ret = -1;
	alsa_param_t *alsa_param = (alsa_param_t *)handle;
	snd_pcm_uframes_t frames = 0;
	snd_pcm_uframes_t count = 0;
	size_t result = 0;
	unsigned char * data = NULL;
	int bytes_per_frame = 0;
	struct timespec before;
	struct timespec now;
	int64_t interval_us;

	if (handle == NULL) {
		return -1;
	}
	data = (unsigned char *)buffer;
	bytes_per_frame = alsa_param->channels * (alsa_param->bitwidth >> 3);

	if (bytes < alsa_param->chunk_bytes) {
		snd_pcm_format_set_silence(alsa_param->format,
				data + bytes,
				(alsa_param->chunk_bytes - bytes)/bytes_per_frame);
		bytes = alsa_param->chunk_bytes;
	}

	frames = bytes / (bytes_per_frame);
	count = frames;

	clock_gettime(CLOCK_MONOTONIC, &before);
	while (count > 0) {
		ret = snd_pcm_writei(alsa_param->handle, buffer, count);
		if (ret  < 0) {
			printf("[%s::%d]--[audio write error: %s]\n", __FUNCTION__, __LINE__, snd_strerror(ret));
		}
		clock_gettime(CLOCK_MONOTONIC, &now);

		if (ret == -EINTR) {
			ret = 0;
		}
		if (ret == -ESTRPIPE) {
			while ((ret = snd_pcm_resume(alsa_param->handle)) == -EAGAIN) {
				usleep(1000);
			}
		}

		if (ret < 0) {
			printf("xun in =%d -EPIPE=%d\n", ret, -EPIPE);
			if ((ret = snd_pcm_prepare(alsa_param->handle)) < 0) {
				printf("snd_pcm_prepare error=%s \n", snd_strerror(ret));
				result = 0;
				return result;
			}
		}

		if (ret > 0) {
			result += ret;
			count -= ret;
			data += ret * bytes_per_frame;
		}

		interval_us = (now.tv_sec * 1000000LL + now.tv_nsec / 1000LL) - (before.tv_sec * 1000000LL + before.tv_nsec / 1000LL);

		if (interval_us > 1 * 1000 * 1000) {
			printf("tried 1s but still failed, we return\n");
			break;
		}
	}

	if (result != frames) {
		printf(" write error =%d frames=%d\n", ret, frames);
		return -1;
	}

	return ret;
}
#endif

int open_alsa(void)
{
	int status;
	snd_pcm_format_t format;
	tAPP_AVK_CONNECTION *conn = app_avk_cb.pStreamingConn;
#ifdef ENABLE_AUDIOSERVICE
	switch (conn->bit_per_sample) {
		case 8:
			format = SND_PCM_FORMAT_U8;
			break;
		case 16:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			format = SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			format = SND_PCM_FORMAT_S32_LE;
			break;
		default:
			format = SND_PCM_FORMAT_S16_LE;
			break;
	}

	if (as_alsa_handle == NULL) {
		status = standard_alsa_output_open(&as_alsa_handle, alsa_device,
				conn->num_channel, conn->sample_rate, format);
		printf("Open alsa device %s, ch:%d, r:%d fmat:%d\n",
				alsa_device, conn->num_channel, conn->sample_rate, conn->bit_per_sample);
		/* Configure ALSA driver with PCM parameters */
		if (status < 0)
		{
			APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
			return -1;
		}
		return status;
	}

	return 0;
#else
	/* If ALSA PCM driver was already open => close it */

	if (app_avk_cb.alsa_handle == NULL)
	{
		/* Open ALSA driver */

		/* Configure as blocking */
		APP_ERROR1("---------------------------->%s", alsa_device);
		status = snd_pcm_open(&(app_avk_cb.alsa_handle), alsa_device,
							  SND_PCM_STREAM_PLAYBACK, 0);

	}
	if (status < 0)
	{
		APP_ERROR1("snd_pcm_open failed: %s", snd_strerror(status));
		return -1;

	}
	else
	{
		APP_DEBUG0("ALSA driver opened");
		switch (conn->bit_per_sample) {
		case 8:
			format = SND_PCM_FORMAT_U8;
			break;
		case 16:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			format = SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			format = SND_PCM_FORMAT_S32_LE;
			break;
		default:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		}

		/* Configure ALSA driver with PCM parameters */
		status = snd_pcm_set_params(app_avk_cb.alsa_handle,
									format,
									SND_PCM_ACCESS_RW_INTERLEAVED,
									conn->num_channel,
									conn->sample_rate,
									1,
									500000);/* 0.5sec */
		if (status < 0)
		{
			APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
			return -1;
		}
	}
	return 0;
#endif
}

void close_alsa(void)
{
#ifdef ENABLE_AUDIOSERVICE
	if (as_alsa_handle != NULL) {
		standard_alsa_output_close(as_alsa_handle);
		as_alsa_handle = NULL;
	}
#else
	if (app_avk_cb.alsa_handle != NULL)
	{
		snd_pcm_close(app_avk_cb.alsa_handle);
		app_avk_cb.alsa_handle = NULL;
	}
#endif
}
#endif
#endif

/*******************************************************************************
**
** Function         app_avk_handle_start
**
** Description      This function is the AVK start handler function
**                  Configures PCM Driver with the stream settings
** Returns          void
**
*******************************************************************************/
static void app_avk_handle_start(tBSA_AVK_MSG *p_data, tAPP_AVK_CONNECTION *connection)
{
	char *avk_format_display[12] = {"SBC", "M12", "M24", "??", "ATRAC", "PCM", "APT-X", "SEC"};
#ifdef PCM_ALSA
	int status;
	snd_pcm_format_t format;
#endif

	if (p_data->start_streaming.media_receiving.format < (sizeof(avk_format_display) / sizeof(avk_format_display[0])))
	{
		APP_INFO1("AVK start: format %s", avk_format_display[p_data->start_streaming.media_receiving.format]);
	}
	else
	{
		APP_INFO1("AVK start: format code (%u) unknown", p_data->start_streaming.media_receiving.format);
	}

	connection->format = p_data->start_streaming.media_receiving.format;

	if (connection->format == BSA_AVK_CODEC_PCM)
	{
		app_avk_create_wave_file();/* create and open a wave file */
		connection->sample_rate = p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq;
		connection->num_channel = p_data->start_streaming.media_receiving.cfg.pcm.num_channel;
		connection->bit_per_sample = p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample;
		printf("Sampling rate:%d, number of channel:%d bit per sample:%d\n",
			   p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq,
			   p_data->start_streaming.media_receiving.cfg.pcm.num_channel,
			   p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample);
	}
	else if (connection->format == BSA_AVK_CODEC_M24)
	{
#if (BSA_AVK_AAC_SUPPORTED == TRUE)
		APP_INFO0("AVK, source stream in AAC CODEC");
		APP_INFO1("obj_type = 0x%x, samp_freq = %d, chnl = 0x%x, vbr = %d, bitrate = %u",
				  p_data->start_streaming.media_receiving.cfg.aac.obj_type,
				  p_data->start_streaming.media_receiving.cfg.aac.samp_freq,
				  p_data->start_streaming.media_receiving.cfg.aac.chnl,
				  p_data->start_streaming.media_receiving.cfg.aac.vbr,
				  p_data->start_streaming.media_receiving.cfg.aac.bitrate);


		switch (p_data->start_streaming.media_receiving.cfg.aac.samp_freq) {
		case 0x0080:
			connection->sample_rate = 48000;
			break;

		case 0x0100:
			connection->sample_rate = 44100;
			break;

		case 0x0200:
			connection->sample_rate = 32000;
			break;

		case 0x1000:
			connection->sample_rate = 16000;
			break;

		default:
			connection->sample_rate = 44100;
			break;
		}


		if (p_data->start_streaming.media_receiving.cfg.aac.chnl == 8)
			connection->num_channel = 1;
		else
			connection->num_channel = 2;


		connection->bit_per_sample = 16;

		APP_INFO1("finnal: sample_freq = %d, channel = %d, ", connection->sample_rate, connection->num_channel);

		/* Initialize the decoder with the format information here */
		if (aacHandle == NULL) {
			APP_ERROR0("Initialize AAC decoder");
			if ((aacHandle = aacDecoder_Open(TT_MP4_LATM_MCP1, 1)) == NULL) {
				APP_ERROR0("Couldn't open AAC decoder");
				return;
			} else if ((aacErr = aacDecoder_SetParam(aacHandle, AAC_PCM_MIN_OUTPUT_CHANNELS, connection->num_channel)) != AAC_DEC_OK) {
				APP_ERROR0("Couldn't set min output channels");
				aacHandle = NULL;
				return;
			} else if ((aacErr = aacDecoder_SetParam(aacHandle, AAC_PCM_MAX_OUTPUT_CHANNELS, connection->num_channel)) != AAC_DEC_OK) {
				APP_ERROR0("Couldn't set max output channels");
				aacHandle = NULL;
				return;
			}
		}
#else
		/* create and open an aac file to dump data to */
		app_avk_create_aac_file();
#endif
	}

#ifndef USE_RING_BUFFER
#ifdef PCM_ALSA
	/* If ALSA PCM driver was already open => close it */
	if (app_avk_cb.alsa_handle != NULL)
	{
#ifdef PCM_ALSA_OPEN_BLOCKING
		pthread_mutex_lock(&mutex);
#endif
		snd_pcm_close(app_avk_cb.alsa_handle);
		app_avk_cb.alsa_handle = NULL;
#ifdef PCM_ALSA_OPEN_BLOCKING
		pthread_mutex_unlock(&mutex);
#endif
	}

	/* Open ALSA driver */
#ifdef PCM_ALSA_OPEN_BLOCKING
	/* Configure as blocking */
	status = snd_pcm_open(&(app_avk_cb.alsa_handle), alsa_device,
						  SND_PCM_STREAM_PLAYBACK, 0);
#else
	status = snd_pcm_open(&(app_avk_cb.alsa_handle), alsa_device,
						  SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
#endif
	if (status < 0)
	{
		APP_ERROR1("snd_pcm_open failed: %s", snd_strerror(status));

	}
	else
	{
		APP_DEBUG0("ALSA driver opened");
		if (connection->bit_per_sample == 8)
			format = SND_PCM_FORMAT_U8;
		else
			format = SND_PCM_FORMAT_S16_LE;
		/* Configure ALSA driver with PCM parameters */
		status = snd_pcm_set_params(app_avk_cb.alsa_handle, format,
									SND_PCM_ACCESS_RW_INTERLEAVED, connection->num_channel,
									connection->sample_rate, 1, 500000);/* 0.5sec */
		if (status < 0)
		{
			APP_ERROR1("snd_pcm_set_params failed: %s", snd_strerror(status));
			exit(1);
		}
	}
#endif /* PCM_ALSA */
#endif /* undef USE_RING_BUFFER */

}

/*******************************************************************************
 **
 ** Function         app_avk_cback
 **
 ** Description      This function is the AVK callback function
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_avk_cback(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	tBSA_AVK_REM_RSP RemRsp;
	snd_mixer_elem_t *elem;

	switch (event)
	{
	case BSA_AVK_OPEN_EVT:
		APP_DEBUG1("BSA_AVK_OPEN_EVT status 0x%x", p_data->sig_chnl_open.status);
#ifdef ENABLE_AUDIOSERVICE
		{
			// Notify AudioService to stop other source
			int as_ret;
			APP_DEBUG0("Start to switch to BT");

			as_ret = AS_Client_OpenInput(AML_AS_INPUT_BT);
			APP_DEBUG1("as_ret = %d\n", as_ret);
		}
#endif

		if (p_data->sig_chnl_open.status == BSA_SUCCESS)
		{
			connection = app_avk_add_connection(p_data->sig_chnl_open.bd_addr);

			if (connection == NULL)
			{
				APP_DEBUG0("BSA_AVK_OPEN_EVT cannot allocate connection cb");
				break;
			}

			connection->ccb_handle = p_data->sig_chnl_open.ccb_handle;
			connection->is_open = TRUE;
			connection->is_streaming_chl_open = FALSE;

#ifdef PCM_ALSA
#ifdef ENABLE_AUDIOSERVICE
			{
				AML_AS_Volume_t as_volume;
				if (AML_AS_SUCCESS == AS_Client_GetVolume(&as_volume)) {
				  vol_backup = as_volume.value;
				} else {
				  APP_ERROR0("Cannot get volume from AudioService\n");
				}
			}
#else
            APP_DEBUG0("backup sound card volume");
            elem = find_elem_by_name(mixerFd, ELEM_NAME);
            if( elem )
                vol_backup = volumeGet_by_elem(elem);
            else
            {
                //Get system volume by /etc/system_volume.sh get_percent
                char result_buf[64];
                int rc = 0;
                FILE *fp;
                fp = popen("/etc/system_volume.sh get_percent","r");
                if( fp != NULL )
                {
                    fscanf(fp, "%d", &vol_backup);
                    fclose(fp);
			        APP_DEBUG1("call /etc/system_volume.sh get_percent, return %d", vol_backup);
                }
            }
#endif
#endif
		}
		app_avk_cb.open_pending = FALSE;
		memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));

		printf("AVK connected to device %02X:%02X:%02X:%02X:%02X:%02X\n",
			   p_data->sig_chnl_open.bd_addr[0], p_data->sig_chnl_open.bd_addr[1], p_data->sig_chnl_open.bd_addr[2],
			   p_data->sig_chnl_open.bd_addr[3], p_data->sig_chnl_open.bd_addr[4], p_data->sig_chnl_open.bd_addr[5]);


		break;

	case BSA_AVK_CLOSE_EVT:
		/* Close event, reason BTA_AVK_CLOSE_STR_CLOSED or BTA_AVK_CLOSE_CONN_LOSS*/
		APP_DEBUG1("BSA_AVK_CLOSE_EVT status 0x%x, handle %d", p_data->sig_chnl_close.status, p_data->sig_chnl_close.ccb_handle);
		connection = app_avk_find_connection_by_bd_addr(p_data->sig_chnl_close.bd_addr);

		if (connection == NULL)
		{
			APP_DEBUG1("BSA_AVK_CLOSE_EVT unknown handle %d", p_data->sig_chnl_close.ccb_handle);
			break;
		}

		connection->is_open = FALSE;
		app_avk_cb.open_pending = FALSE;
		memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));
		if (connection->is_started_streaming == TRUE)
		{
			connection->is_started_streaming = FALSE;

			if (connection->format == BSA_AVK_CODEC_M24)
			{
#if (BSA_AVK_AAC_SUPPORTED != TRUE)
				close(app_avk_cb.fd);
				app_avk_cb.fd  = -1;
#endif
			}
			else
				app_avk_close_wave_file(connection);
		}
		app_avk_reset_connection(connection->bda_connected);
#ifdef PCM_ALSA
		APP_DEBUG0("recover sound card volume");
#ifdef ENABLE_AUDIOSERVICE
		{
			AML_AS_Volume_t as_volume;
			AS_Client_GetVolume(&as_volume);
			as_volume.value = vol_backup;
			AS_Client_SetVolume(&as_volume);
		}
#else
        elem = find_elem_by_name(mixerFd, ELEM_NAME);
        if( elem )
            volumeSet_by_elem(elem, vol_backup);
        else
        {
            //If we can't find volume mixer, call system script to change the volume
            char syscmd_buf[128];
            snprintf(syscmd_buf, sizeof(syscmd_buf), "/etc/system_volume.sh set_percent %d", vol_backup);
		    APP_DEBUG1("call %s", syscmd_buf);
            system(syscmd_buf);
        }
#endif
#endif
#ifdef ENABLE_AUDIOSERVICE
		{
			// Notify AudioService to stop other source
			int as_ret;
			APP_DEBUG0("Notify AudioService to stop BT");

			as_ret = AS_Client_CloseInput(AML_AS_INPUT_BT);
			APP_DEBUG1("as_ret = %d\n", as_ret);
		}
#endif

		break;

	case BSA_AVK_STR_OPEN_EVT:
		APP_DEBUG1("BSA_AVK_STR_OPEN_EVT status 0x%x", p_data->stream_chnl_open.status);

		connection = app_avk_find_connection_by_av_handle(p_data->stream_chnl_open.ccb_handle);
		if (connection == NULL)
		{
			break;
		}
		connection->is_streaming_chl_open = TRUE;

		break;

	case BSA_AVK_STR_CLOSE_EVT:
		APP_DEBUG1("BSA_AVK_STR_CLOSE_EVT streaming chn closed handle: %d ", p_data->stream_chnl_close.ccb_handle);
		connection = app_avk_find_connection_by_bd_addr(p_data->stream_chnl_close.bd_addr);

		if (connection == NULL)
		{
			break;
		}

		connection->is_streaming_chl_open = FALSE;
		break;

	case BSA_AVK_START_EVT:
		APP_DEBUG1("BSA_AVK_START_EVT status 0x%x", p_data->start_streaming.status);
		/* We got START_EVT for a new device that is streaming but server discards the data
		    because another stream is already active */
		if (p_data->start_streaming.discarded)
		{
			connection = app_avk_find_connection_by_bd_addr(p_data->start_streaming.bd_addr);
			if (connection)
			{
				connection->is_started_streaming = TRUE;
				connection->is_streaming_chl_open = TRUE;
			}

			break;
		}
		/* got Start event and device is streaming */
		else
		{
			app_avk_cb.pStreamingConn = NULL;

			connection = app_avk_find_connection_by_bd_addr(p_data->start_streaming.bd_addr);

			if (connection == NULL)
			{
				break;
			}

			connection->is_started_streaming = TRUE;
			connection->is_streaming_chl_open = TRUE;
			app_avk_handle_start(p_data, connection);
			app_avk_cb.pStreamingConn = connection;
#ifdef PCM_ALSA
			// saved the sample_rate which is used to calculate the time
			saved_sample_rate = connection->sample_rate;
#endif

		}
#ifdef CAL_DATA
		accu = 0;
		first_timestamp = 0;
		length = 0;
		accu_data = 0;
#endif
		break;



	case BSA_AVK_STOP_EVT:
		APP_DEBUG1("BSA_AVK_STOP_EVT handle: %d  Suspended: %d", p_data->stop_streaming.ccb_handle, p_data->stop_streaming.suspended);
		/* Stream was suspended */
		if (p_data->stop_streaming.suspended)
		{
			connection = app_avk_find_connection_by_bd_addr(p_data->stop_streaming.bd_addr);
			if (connection == NULL)
			{
				break;
			}

			connection->is_started_streaming = FALSE;
		}
		/* stream was closed */
		else
		{
			connection = app_avk_find_connection_by_av_handle(p_data->stop_streaming.ccb_handle);
			if (connection == NULL)
			{
				break;
			}

			connection->is_started_streaming = FALSE;

			if (connection->format == BSA_AVK_CODEC_M24)
			{
#if (BSA_AVK_AAC_SUPPORTED == TRUE)
				APP_DEBUG0("closing aac decoder");

				if (aacHandle != NULL) {
					aacDecoder_Close(aacHandle);
					aacHandle = NULL;
				}
#else
				close(app_avk_cb.fd);
				app_avk_cb.fd  = -1;
#endif
			}
			else
				app_avk_close_wave_file(connection);

		}
#ifdef USE_RING_BUFFER
		/*clear buffer data that not reach start_threadhold. add delay to avoid effects on that reach start_threadhold*/
		usleep((rb.buffer_length - rb.avl_room) / 44.1 / 4  * 1000); /*wait stream empty*/
		ring_buffer_clear(&rb);
#endif
		break;

	case BSA_AVK_RC_OPEN_EVT:

		if (p_data->rc_open.status == BSA_SUCCESS)
		{
			connection = app_avk_add_connection(p_data->rc_open.bd_addr);
			if (connection == NULL)
			{
				APP_DEBUG0("BSA_AVK_RC_OPEN_EVT could not allocate connection");
				break;
			}

			APP_DEBUG1("BSA_AVK_RC_OPEN_EVT peer_feature=0x%x rc_handle=%d",
					   p_data->rc_open.peer_features, p_data->rc_open.rc_handle);
			connection->rc_handle = p_data->rc_open.rc_handle;
			connection->peer_features =  p_data->rc_open.peer_features;
			connection->peer_version = p_data->rc_open.peer_version;
			connection->is_rc_open = TRUE;
		}
		break;

	case BSA_AVK_RC_PEER_OPEN_EVT:
		connection = app_avk_find_connection_by_rc_handle(p_data->rc_open.rc_handle);
		if (connection == NULL)
		{
			APP_DEBUG1("BSA_AVK_RC_PEER_OPEN_EVT could not find connection handle %d", p_data->rc_open.rc_handle);
			break;
		}

		APP_DEBUG1("BSA_AVK_RC_PEER_OPEN_EVT peer_feature=0x%x rc_handle=%d",
				   p_data->rc_open.peer_features, p_data->rc_open.rc_handle);

		connection->peer_features =  p_data->rc_open.peer_features;
		connection->peer_version = p_data->rc_open.peer_version;
		connection->is_rc_open = TRUE;
		break;

	case BSA_AVK_RC_CLOSE_EVT:
		APP_DEBUG0("BSA_AVK_RC_CLOSE_EVT");
		connection = app_avk_find_connection_by_rc_handle(p_data->rc_close.rc_handle);
		if (connection == NULL)
		{
			break;
		}

		connection->is_rc_open = FALSE;
		break;

	case BSA_AVK_REMOTE_RSP_EVT:
		APP_DEBUG0("BSA_AVK_REMOTE_RSP_EVT");
		break;

	case BSA_AVK_REMOTE_CMD_EVT:
		APP_DEBUG0("BSA_AVK_REMOTE_CMD_EVT");
		APP_DEBUG1(" label:0x%x", p_data->remote_cmd.label);
		APP_DEBUG1(" op_id:0x%x", p_data->remote_cmd.op_id);
		APP_DEBUG1(" label:0x%x", p_data->remote_cmd.label);
		APP_DEBUG0(" avrc header");
		APP_DEBUG1("   ctype:0x%x", p_data->remote_cmd.hdr.ctype);
		APP_DEBUG1("   subunit_type:0x%x", p_data->remote_cmd.hdr.subunit_type);
		APP_DEBUG1("   subunit_id:0x%x", p_data->remote_cmd.hdr.subunit_id);
		APP_DEBUG1("   opcode:0x%x", p_data->remote_cmd.hdr.opcode);
		APP_DEBUG1(" len:0x%x", p_data->remote_cmd.len);
		APP_DUMP("data", p_data->remote_cmd.data, p_data->remote_cmd.len);

		BSA_AvkRemoteRspInit(&RemRsp);
		RemRsp.avrc_rsp = BSA_AVK_RSP_ACCEPT;
		RemRsp.label = p_data->remote_cmd.label;
		RemRsp.op_id = p_data->remote_cmd.op_id;
		RemRsp.key_state = p_data->remote_cmd.key_state;
		RemRsp.handle = p_data->remote_cmd.rc_handle;
		RemRsp.length = 0;
		BSA_AvkRemoteRsp(&RemRsp);
		break;

	case BSA_AVK_VENDOR_CMD_EVT:
		APP_DEBUG0("BSA_AVK_VENDOR_CMD_EVT");
		APP_DEBUG1(" code:0x%x", p_data->vendor_cmd.code);
		APP_DEBUG1(" company_id:0x%x", p_data->vendor_cmd.company_id);
		APP_DEBUG1(" label:0x%x", p_data->vendor_cmd.label);
		APP_DEBUG1(" len:0x%x", p_data->vendor_cmd.len);
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
		APP_DUMP("A2DP Data", p_data->vendor_cmd.data, p_data->vendor_cmd.len);
#endif
		break;

	case BSA_AVK_VENDOR_RSP_EVT:
		APP_DEBUG0("BSA_AVK_VENDOR_RSP_EVT");
		APP_DEBUG1(" code:0x%x", p_data->vendor_rsp.code);
		APP_DEBUG1(" company_id:0x%x", p_data->vendor_rsp.company_id);
		APP_DEBUG1(" label:0x%x", p_data->vendor_rsp.label);
		APP_DEBUG1(" len:0x%x", p_data->vendor_rsp.len);
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
		APP_DUMP("A2DP Data", p_data->vendor_rsp.data, p_data->vendor_rsp.len);
#endif
		break;

	case BSA_AVK_CP_INFO_EVT:
		APP_DEBUG0("BSA_AVK_CP_INFO_EVT");
		if (p_data->cp_info.id == BSA_AVK_CP_SCMS_T_ID)
		{
			switch (p_data->cp_info.info.scmst_flag)
			{
			case BSA_AVK_CP_SCMS_COPY_NEVER:
				APP_INFO1(" content protection:0x%x - COPY NEVER", p_data->cp_info.info.scmst_flag);
				break;
			case BSA_AVK_CP_SCMS_COPY_ONCE:
				APP_INFO1(" content protection:0x%x - COPY ONCE", p_data->cp_info.info.scmst_flag);
				break;
			case BSA_AVK_CP_SCMS_COPY_FREE:
			case (BSA_AVK_CP_SCMS_COPY_FREE+1):
				APP_INFO1(" content protection:0x%x - COPY FREE", p_data->cp_info.info.scmst_flag);
				break;
			default:
				APP_ERROR1(" content protection:0x%x - UNKNOWN VALUE", p_data->cp_info.info.scmst_flag);
				break;
			}
		}
		else
		{
			APP_INFO0("No content protection");
		}
		break;

	case BSA_AVK_REGISTER_NOTIFICATION_EVT:
	case BSA_AVK_LIST_PLAYER_APP_ATTR_EVT:
	case BSA_AVK_LIST_PLAYER_APP_VALUES_EVT:
	case BSA_AVK_SET_PLAYER_APP_VALUE_EVT:
	case BSA_AVK_GET_PLAYER_APP_VALUE_EVT:
	case BSA_AVK_GET_ELEM_ATTR_EVT:
	case BSA_AVK_GET_PLAY_STATUS_EVT:
	case BSA_AVK_SET_ADDRESSED_PLAYER_EVT:
	case BSA_AVK_SET_BROWSED_PLAYER_EVT:
	case BSA_AVK_GET_FOLDER_ITEMS_EVT:
	case BSA_AVK_CHANGE_PATH_EVT:
	case BSA_AVK_GET_ITEM_ATTR_EVT:
	case BSA_AVK_PLAY_ITEM_EVT:
	case BSA_AVK_ADD_TO_NOW_PLAYING_EVT:
		break;

	case BSA_AVK_SET_ABS_VOL_CMD_EVT:
		APP_DEBUG0("BSA_AVK_SET_ABS_VOL_CMD_EVT");
		connection = app_avk_find_connection_by_rc_handle(p_data->abs_volume.handle);
		if ((connection != NULL) && connection->m_bAbsVolumeSupported)
		{
			/* Peer requested change in volume. Make the change and send response with new system volume. BSA is TG role in AVK */
			if (p_data->abs_volume.abs_volume_cmd.volume <= BSA_MAX_ABS_VOLUME)
			{
				app_avk_cb.volume = p_data->abs_volume.abs_volume_cmd.volume;
			}
			unsigned int vol = p_data->abs_volume.abs_volume_cmd.volume * 100 / BSA_MAX_ABS_VOLUME;
			APP_DEBUG1("volume = %d", vol);
#ifdef ENABLE_AUDIOSERVICE
			{
				AML_AS_Volume_t as_volume;
				AS_Client_GetVolume(&as_volume);
				as_volume.value = as_volume.min + vol * (as_volume.max-as_volume.min)/100;
				AS_Client_SetVolume(&as_volume);
			}
#else
            snd_mixer_elem_t *elem = find_elem_by_name(mixerFd, ELEM_NAME);
            if( elem )
                volumeSet_by_elem( elem, vol);
            else
            {
                //If we can't find volume mixer, call system script to change the volume
                char syscmd_buf[128];
                snprintf(syscmd_buf, sizeof(syscmd_buf), "/etc/system_volume.sh set_percent %d", vol);
			    APP_DEBUG1("call %s", syscmd_buf);
                system(syscmd_buf);
            }
#endif

			/* Change the code below based on which interface audio is going out to. */
			/*char buffer[100];
			sprintf(buffer, "amixer -q cset name='Headphone Playback Volume' '%d'", app_avk_cb.volume);
			system(buffer);*/
		}
		else
		{
			APP_ERROR1("not changing volume connection 0x%x m_bAbsVolumeSupported %d", connection, connection->m_bAbsVolumeSupported);
		}
		break;


	case BSA_AVK_REG_NOTIFICATION_CMD_EVT:
		APP_DEBUG0("BSA_AVK_REG_NOTIFICATION_CMD_EVT");

		if (p_data->reg_notif_cmd.reg_notif_cmd.event_id == AVRC_EVT_VOLUME_CHANGE)
		{
			connection = app_avk_find_connection_by_rc_handle(p_data->reg_notif_cmd.handle);
			if (connection != NULL)
			{
				connection->m_bAbsVolumeSupported = TRUE;
				connection->volChangeLabel = p_data->reg_notif_cmd.label;

				/* Peer requested registration for vol change event. Send response with current system volume. BSA is TG role in AVK */
				app_avk_reg_notfn_rsp(app_avk_cb.volume,
									  p_data->reg_notif_cmd.handle,
									  p_data->reg_notif_cmd.label,
									  p_data->reg_notif_cmd.reg_notif_cmd.event_id,
									  BSA_AVK_RSP_INTERIM);
			}
		}
		break;

	default:
		APP_ERROR1("Unsupported event %d", event);
		break;
	}

	/* forward the callback to registered applications */
	if (app_avk_cb.p_Callback)
		app_avk_cb.p_Callback(event, p_data);
}

/*******************************************************************************
 **
 ** Function         app_avk_open
 **
 ** Description      Example of function to open AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_open(BD_ADDR bd_addr)
{
	tBSA_STATUS status;
	int choice;
	BOOLEAN connect = FALSE;
	UINT8 *p_name;
	tBSA_AVK_OPEN open_param;
	tAPP_AVK_CONNECTION *connection = NULL;


	for (int cnt = 0; cnt < APP_AVK_MAX_CONNECTIONS; cnt++)
	{
		/*return if device is now connected*/
		if (app_avk_cb.connections[cnt].in_use == TRUE) {
			APP_DEBUG0("there is connection available");
			return;
		}
	}

	if (app_avk_cb.open_pending)
	{
		APP_ERROR0("already trying to connect");
		return;
	}

	connect = TRUE;
	for (int i = 0; i < APP_NUM_ELEMENTS(app_xml_remote_devices_db); i++) {
		if (strncmp(bd_addr, app_xml_remote_devices_db[i].bd_addr, BD_ADDR_LEN) == 0)
			p_name = app_xml_remote_devices_db[i].name;
	}

	if (connect != FALSE)
	{
		/* Open AVK stream */
		APP_DEBUG1("Connecting to AV device:%s", p_name);

		app_avk_cb.open_pending = TRUE;
		memcpy(app_avk_cb.open_pending_bda, bd_addr, sizeof(BD_ADDR));

		BSA_AvkOpenInit(&open_param);
		memcpy((char *) (open_param.bd_addr), bd_addr, sizeof(BD_ADDR));

		open_param.sec_mask = BSA_SEC_AUTHORIZATION;
		status = BSA_AvkOpen(&open_param);
		if (status != BSA_SUCCESS)
		{
			APP_ERROR1("Unable to connect to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
					   open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
					   open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);

			app_avk_cb.open_pending = FALSE;
			memset(app_avk_cb.open_pending_bda, 0, sizeof(BD_ADDR));
		}
		else
		{
			/* this is an active wait for demo purpose */
			printf("waiting for AV connection to open\n");

			while (app_avk_cb.open_pending == TRUE);

			connection = app_avk_find_connection_by_bd_addr(open_param.bd_addr);
			if (connection == NULL || connection->is_open == FALSE)
			{
				printf("failure opening AV connection  \n");
			}
			else
			{
				/* XML Database update should be done upon reception of AV OPEN event */
				APP_DEBUG1("Connected to AV device:%s", p_name);
				/* Read the Remote device xml file to have a fresh view */
				app_read_xml_remote_devices();

				/* Add AV service for this devices in XML database */
				app_xml_add_trusted_services_db(app_xml_remote_devices_db,
												APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr,
												BSA_A2DP_SERVICE_MASK | BSA_AVRCP_SERVICE_MASK);

				app_xml_update_name_db(app_xml_remote_devices_db,
									   APP_NUM_ELEMENTS(app_xml_remote_devices_db), bd_addr, p_name);

				/* Update database => write to disk */
				if (app_write_xml_remote_devices() < 0)
				{
					APP_ERROR0("Failed to store remote devices database");
				}
			}
		}
	}

}

/*******************************************************************************
 **
 ** Function         app_avk_close
 **
 ** Description      Function to close AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_close(BD_ADDR bda)
{
	tBSA_STATUS status;
	tBSA_AVK_CLOSE bsa_avk_close_param;
	tAPP_AVK_CONNECTION *connection = NULL;

	/* Close AVK connection */
	BSA_AvkCloseInit(&bsa_avk_close_param);

	connection = app_avk_find_connection_by_bd_addr(bda);

	if (connection == NULL)
	{
		APP_ERROR0("Unable to Close AVK connection , invalid BDA");
		return;
	}

	bsa_avk_close_param.ccb_handle = connection->ccb_handle;
	bsa_avk_close_param.rc_handle = connection->rc_handle;
retry:
	status = BSA_AvkClose(&bsa_avk_close_param);
	if (status == BSA_ERROR_SRV_AVK_BUSY)
	{
		APP_ERROR0("AVK busy, try again");
		sleep(1);
		goto retry;
	} else if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Close AVK connection with status %d", status);
	} else
	{
		app_avk_reset_connection(bda);
	}

}

/*******************************************************************************
 **
 ** Function         app_avk_close_all
 **
 ** Description      Function to close all AVK connections
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_close_all()
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (app_avk_cb.connections[index].in_use == TRUE)
		{
			app_avk_close(app_avk_cb.connections[index].bda_connected);
		}
	}
}

/*******************************************************************************
**
** Function         app_avk_open_rc
**
** Description      Function to opens avrc controller connection. AVK should be open before opening rc.
**
** Returns          void
**
*******************************************************************************/
void app_avk_open_rc(BD_ADDR bd_addr)
{
	tBSA_STATUS status;
	tBSA_AVK_OPEN bsa_avk_open_param;

	/* open avrc connection */
	BSA_AvkOpenRcInit(&bsa_avk_open_param);
	bdcpy(bsa_avk_open_param.bd_addr, bd_addr);
	status = BSA_AvkOpenRc(&bsa_avk_open_param);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to open arvc connection with status %d", status);
	}

}

/*******************************************************************************
**
** Function         app_avk_close_rc
**
** Description      Function to closes avrc controller connection.
**
** Returns          void
**
*******************************************************************************/
void app_avk_close_rc(UINT8 rc_handle)
{
	tBSA_STATUS status;
	tBSA_AVK_CLOSE bsa_avk_close_param;

	/* close avrc connection */
	BSA_AvkCloseRcInit(&bsa_avk_close_param);

	bsa_avk_close_param.rc_handle = rc_handle;

	status = BSA_AvkCloseRc(&bsa_avk_close_param);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to close arvc connection with status %d", status);
	}

}

/*******************************************************************************
**
** Function         app_avk_close_str
**
** Description      Function to close an A2DP Steam connection
**
** Returns          void
**
*******************************************************************************/
void app_avk_close_str(UINT8 ccb_handle)
{
	tBSA_STATUS status;
	tBSA_AVK_CLOSE_STR bsa_avk_close_str_param;

	/* close avrc connection */
	BSA_AvkCloseStrInit(&bsa_avk_close_str_param);

	bsa_avk_close_str_param.ccb_handle = ccb_handle;

	status = BSA_AvkCloseStr(&bsa_avk_close_str_param);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to close arvc connection with status %d", status);
	}

}

/*******************************************************************************
**
** Function        app_avk_register
**
** Description     Example of function to register an avk endpoint
**
** Returns         void
**
*******************************************************************************/
int app_avk_register(void)
{
	tBSA_STATUS status;
	tBSA_AVK_REGISTER bsa_avk_register_param;

	/* register an audio AVK end point */
	BSA_AvkRegisterInit(&bsa_avk_register_param);

	bsa_avk_register_param.media_sup_format.audio.pcm_supported = TRUE;
	bsa_avk_register_param.media_sup_format.audio.sec_supported = TRUE;
#if (defined(BSA_AVK_AAC_SUPPORTED) && (BSA_AVK_AAC_SUPPORTED==TRUE))
	/* Enable AAC support in the app */
	bsa_avk_register_param.media_sup_format.audio.aac_supported = TRUE;
#endif
	strncpy(bsa_avk_register_param.service_name, "BSA Audio Service",
			BSA_AVK_SERVICE_NAME_LEN_MAX);

	bsa_avk_register_param.reg_notifications = reg_notifications;

	status = BSA_AvkRegister(&bsa_avk_register_param);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to register an AV sink with status %d", status);

		if (BSA_ERROR_SRV_AVK_AV_REGISTERED == status)
		{
			APP_ERROR0("AV was registered before AVK, AVK should be registered before AV");
		}
		return -1;
	}
	/* Save UIPC channel */
	app_avk_cb.uipc_audio_channel = bsa_avk_register_param.uipc_channel;

	/* open the UIPC channel to receive the pcm */
	if (UIPC_Open(bsa_avk_register_param.uipc_channel, app_avk_uipc_cback) == FALSE)
	{
		APP_ERROR0("Unable to open UIPC channel");
		return -1;
	}

	app_avk_cb.fd = -1;

	return 0;

}

/*******************************************************************************
 **
 ** Function        app_avk_deregister
 **
 ** Description     Example of function to deregister an avk endpoint
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_avk_deregister(void)
{
	tBSA_STATUS status;
	tBSA_AVK_DEREGISTER req;

	/* register an audio AVK end point */
	BSA_AvkDeregisterInit(&req);
	status = BSA_AvkDeregister(&req);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to deregister an AV sink with status %d", status);
	}

#ifdef TODO
	if (app_avk_cb.format == BSA_AVK_CODEC_M24)
	{
		close(app_avk_cb.fd);
		app_avk_cb.fd  = -1;
	}
	else
		app_avk_close_wave_file();
#endif

	/* close the UIPC channel receiving the pcm */
	tUIPC_CH_ID chn = app_avk_cb.uipc_audio_channel;
	app_avk_cb.uipc_audio_channel = UIPC_CH_ID_BAD;
	UIPC_Close(chn);
}
#ifdef CAL_DATA
static long long current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
	return milliseconds;
}
#endif
/*******************************************************************************
**
** Function        app_avk_uipc_cback
**
** Description     uipc audio call back function.
**
** Parameters      pointer on a buffer containing PCM sample with a BT_HDR header
**
** Returns          void
**
*******************************************************************************/
#if (BSA_AVK_AAC_SUPPORTED == TRUE)
static void aacStartDecode(const char *inbuf, unsigned int len) {
	CStreamInfo *aacinf;
	unsigned int valid = len;
	if (aacHandle != NULL) {
		if ((aacErr = aacDecoder_Fill(aacHandle, &inbuf, &len, &valid)) != AAC_DEC_OK) {
			APP_ERROR1("AAC buffer fill error: %s", aacdec_strerror(aacErr));
		}
		else if ((aacErr = aacDecoder_DecodeFrame(aacHandle, aacOutBuf, AAC_OUTBUF_SIZE, 0)) != AAC_DEC_OK) {
			APP_ERROR1("AAC decode frame error: %s", aacdec_strerror(aacErr));
		} else if ((aacinf = aacDecoder_GetStreamInfo(aacHandle)) == NULL) {
			APP_ERROR0("Couldn't get AAC stream info");
		} else {
			const size_t fs = aacinf->frameSize;
			ring_buffer_push(&rb, aacOutBuf, fs * 4);
		}
	}
}
#endif
static void app_avk_uipc_cback(BT_HDR *p_msg)
{
	static UINT16 adj = 0;
#ifdef PCM_ALSA
	snd_pcm_sframes_t alsa_frames;
	snd_pcm_sframes_t alsa_frames_to_send;
#endif
	UINT8 *p_buffer;
	int dummy;
	tAPP_AVK_CONNECTION *connection = NULL;

	if (p_msg == NULL)
	{
		return;
	}
#ifdef CAL_DATA
	length += p_msg->len;
	if (first_timestamp == 0 )
		first_timestamp = current_timestamp();
	else
	{
		second_timestamp = current_timestamp();

		if ( second_timestamp - first_timestamp > 1000 ) //print every 10second
		{
			printf( "current timstamp: %lld\n", second_timestamp);
			accu_data += length - (long long)((second_timestamp - first_timestamp) * 44.1 * 2 * 2);
			printf("we expect %lld bytes in %lld miliseconds\n", (long long)((second_timestamp - first_timestamp) * 44.1 * 2 * 2), second_timestamp - first_timestamp);
			printf( "we got %ld bytes in callback,  diff = %lld, total diff: %ld\n", length, (long long)(length - ((second_timestamp - first_timestamp) * 44.1 * 2 * 2)),  accu_data);
#ifdef USE_RING_BUFFER
			pthread_mutex_lock(&rb.mutex);
			printf("data avl = %d, %d%, startPlay = %d\n", rb.buffer_length - rb.avl_room, (rb.buffer_length - rb.avl_room) * 100 / rb.buffer_length, startPlay);
			pthread_mutex_unlock(&rb.mutex);
#endif
			first_timestamp = current_timestamp();
			length = 0;
		}
	}
#endif
	p_buffer = ((UINT8 *)(p_msg + 1)) + p_msg->offset;

	connection = app_avk_find_streaming_connection();

	if (!connection)
	{
		if (app_avk_cb.uipc_audio_channel != UIPC_CH_ID_BAD)
			APP_ERROR0("Unable to find connection!!!!!!");
		GKI_freebuf(p_msg);
		/*reset aac frame adjust*/
		adj = 0;
		return;
	}

	if (connection->format == BSA_AVK_CODEC_M24)
	{
		/* Invoke AAC decoder here for the current buffer.
		    decode the AAC file that is created */
#if (BSA_AVK_AAC_SUPPORTED == TRUE)
		UINT8 *inbuf = aacInBuf;
		INT16 aacFrameLen_lf, aacFrameLen, p_buffer_len = p_msg->len;

		/*prervios packet may be uncompleted, adjust starting pointer to copy data*/
		memcpy(inbuf + adj, p_buffer, p_buffer_len);
		p_buffer_len += adj;

		adj = 0;
		while (1) {
			/*each aac frame start with 0x47 0xfc*/
			if (inbuf[0] != 0x47 || inbuf[1] != 0xfc) {
#if (AAC_DEBUG == 1)
				APP_ERROR0("unReconized packet");
				for (i = 0 ; i < p_buffer_len; i++) {
					printf("0x%02x ", inbuf[i]);
					if ((i + 1) % 15 == 0)
						printf("\n");
				}
				printf("\n=============\n");
#endif
				/*we can't caculate aacFrameLen, if datas avialable not enough
				save them, and use them next time*/
				if (p_buffer_len < 10)
					goto save_packet;

				GKI_freebuf(p_msg);
				return;
			}
			/***************************
			Stream Mux Config Header 9 bytes
			payload length info:
			byte[9]
			byte[10] -> valid only if byte[9]  == 255
			byte[11] -> valid only if byte[10] == 255
			playload length = byte[9] + byte[10] + byte[11].....
			****************************/
			char *start = inbuf + 9;

			aacFrameLen = 9;

			while (*start == 255) {
				/*255 + 1, include length byte itself*/
				aacFrameLen += 256;
				start++;
			}

			aacFrameLen += *start + 1;

			aacFrameLen_lf = aacFrameLen -  p_buffer_len;

			if (aacFrameLen_lf < 0) {
				/*this p_buffer has more than one aac frame*/
				aacStartDecode(inbuf, aacFrameLen);
				inbuf += aacFrameLen;
				p_buffer_len -= aacFrameLen;
				continue;
			} else if (aacFrameLen_lf == 0) {
				/*this section of p_buffer is a completely aac frame*/
				aacStartDecode(inbuf, aacFrameLen);
				GKI_freebuf(p_msg);
				return;

			} else if (aacFrameLen_lf > 0) {
save_packet:
				/*data not enough, wait for next p_buffer*/
#if (AAC_DEBUG == 1)
				/*datas from bsa_server always not complete, we have to do something*/
				printf("uncompletely packet, save and wait for next p_buffer\n");
				for (i = 0 ; i < p_buffer_len; i++) {
					printf("0x%02x ", inbuf[i]);
					if ((i + 1) % 15 == 0)
						printf("\n");
				}
				printf("\n=============\n");
#endif
				/*we move the data left to the starting addr of inbuf*/
				memcpy(aacInBuf, inbuf, p_buffer_len);

				adj = p_buffer_len;
				GKI_freebuf(p_msg);
				return;
			}

		}
#else
		if (app_avk_cb.fd != -1)
		{
			dummy = write(app_avk_cb.fd, p_buffer, p_msg->len);
			(void)dummy;
		}

		GKI_freebuf(p_msg);
		return;
#endif
	}

	if (connection->format == BSA_AVK_CODEC_PCM &&
			app_avk_cb.fd_codec_type != BSA_AVK_CODEC_M24)
	{
		/* Invoke AAC decoder here for the current buffer.
		    decode the AAC file that is created */
		if (app_avk_cb.fd != -1)
		{
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
			APP_DEBUG1("app_avk_uipc_cback Writing Data 0x%x, len = %d", p_buffer, p_msg->len);
#endif
			dummy = write(app_avk_cb.fd, p_buffer, p_msg->len);
			(void)dummy;
		}

#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
		APP_DUMP("A2DP Data", p_buffer, p_msg->len);
#endif
	}

	if (connection->num_channel == 0)
	{
		APP_ERROR0("connection->num_channel == 0");
		GKI_freebuf(p_msg);
		return;
	}

#ifdef PCM_ALSA
#ifdef USE_RING_BUFFER
	if (p_buffer)
	{
		ring_buffer_push(&rb, p_buffer, p_msg->len);
	}
#else
	if (app_avk_cb.alsa_handle != NULL && p_buffer)
	{
		/* Compute number of PCM samples (contained in p_msg->len bytes) */
		alsa_frames_to_send = p_msg->len / connection->num_channel;
		if (connection->bit_per_sample == 16)
			alsa_frames_to_send /= 2;

#ifdef PCM_ALSA_OPEN_BLOCKING
		while (alsa_frames_to_send > 0)
		{
			pthread_mutex_lock(&mutex);
			if (!app_avk_cb.alsa_handle)
			{
				pthread_mutex_unlock(&mutex);
				break;
			}

			alsa_frames = snd_pcm_writei(app_avk_cb.alsa_handle, p_buffer, alsa_frames_to_send);
			if (alsa_frames < 0)
			{
				if (alsa_frames == -EPIPE)
				{
					APP_DEBUG0("ALSA: underrun in frame");
					snd_pcm_prepare(app_avk_cb.alsa_handle);
					alsa_frames = 0;
				}
				else if (alsa_frames == -EBADFD)
				{
					APP_DEBUG0("ALSA: retry");
					alsa_frames = 0;
				}
				else
				{
					APP_DEBUG1("ALSA: snd_pcm_writei err %d (%s)", (int) alsa_frames, snd_strerror(alsa_frames));
					pthread_mutex_unlock(&mutex);
					break;
				}
			}

			pthread_mutex_unlock(&mutex);
			alsa_frames_to_send -= alsa_frames;
		}
		if (alsa_frames_to_send)
			APP_DEBUG1("ALSA: short write (discarded %li)", alsa_frames_to_send);
	}
#else
		alsa_frames = snd_pcm_writei(app_avk_cb.alsa_handle, p_buffer, alsa_frames_to_send);
		if (alsa_frames < 0)
		{
			alsa_frames = snd_pcm_recover(app_avk_cb.alsa_handle, alsa_frames, 0);
		}
		if (alsa_frames < 0)
		{
			APP_DEBUG1("app_avk_uipc_cback snd_pcm_writei failed %s", snd_strerror(alsa_frames));
		}
		if (alsa_frames > 0 && alsa_frames < alsa_frames_to_send)
			APP_DEBUG1("app_avk_uipc_cback Short write (expected %li, wrote %li)",
					   (long) alsa_frames_to_send, alsa_frames);
	}
	else
		APP_DEBUG0("app_avk_uipc_cback snd_pcm_writei failed FINALLY !!");
#endif /* PCM_ALSA_OPEN_BLOCKING */
#endif /* USE_RING_BUFFER */
#endif /* PCM_ALSA */

	GKI_freebuf(p_msg);
}


#ifdef USE_RING_BUFFER
// This function is used for wait the time until low level finishes handling the audio data
void wait_sometime(int samples, struct timeval *t1, struct timeval *t2)
{
	float sleep_time =  samples;
	sleep_time = sleep_time *1000*1000/saved_sample_rate;
	static print_count = 0;

	sleep_time = sleep_time;
	//printf("sleep_time = %f\n", sleep_time);
	if (t1->tv_sec > t2->tv_sec) {
		// do nothing
	} else {
		if (t2->tv_sec > t1->tv_sec) {
			sleep_time = sleep_time - (t2->tv_sec - t1->tv_sec -1);
			sleep_time = sleep_time - (1000*1000+t2->tv_usec-t1->tv_usec);
		} else {
			if (t2->tv_usec > t1->tv_usec)
				sleep_time = sleep_time - (t2->tv_usec - t1->tv_usec);
		}
	}
	//printf("second sleep_time = %f\n", sleep_time);
	if (sleep_time < 0)
		sleep_time = ((float)samples)*1000*1000/saved_sample_rate - 15000;
	// Here 15000 is hardcode about the time during pulse handle 3200 frames
	if (sleep_time > 10) {
		//if(print_count%100 == 0)
		//	printf("sleep %f us \n", sleep_time);
		print_count ++;
		usleep(sleep_time);
	}
}

void thread_play_data()
{
	snd_pcm_sframes_t alsa_frames;
	snd_pcm_sframes_t alsa_frames_to_send;
	char tmpbuf[PERIOD];
	struct timeval start_time, end_time;
	int thread_status = 0;	// 0: start to play
													// 10: try 10 time to get data from ring buffer
	int get_start_time = 0;
	int send_frames = 0;

	while (1) {
		if (rb.buffer_length - rb.avl_room > START_TH && startPlay == 0) {
			startPlay = 1;
			APP_DEBUG0("should start play now!");
		}

		while ( startPlay == 1) {
			int byte;
#ifdef PCM_ALSA
			if (open_alsa())
				break;

			do {
#ifdef ENABLE_AUDIOSERVICE
				byte = ring_buffer_pop(&rb, as_alsa_handle->audiobuf, as_alsa_handle->chunk_bytes);
				if ( byte != as_alsa_handle->chunk_bytes)
					printf("pop %d data and get %d\n", as_alsa_handle->chunk_bytes, byte);
#else
				memset(tmpbuf, 0, sizeof(tmpbuf));
				byte = ring_buffer_pop(&rb, tmpbuf, sizeof(tmpbuf));
#endif

				if (app_avk_is_pulse()) {
					if (byte <= 0) {
						while (thread_status != 10) {
							// total wait 500ms to check the ring buffer before close ALSA
							thread_status++;
							// printf("sleep again %d\n", thread_status);
							usleep(50*1000);
							// Try to get data again
							byte = ring_buffer_pop(&rb, tmpbuf, sizeof(tmpbuf));
							if (byte > 0) {
								thread_status = 0;
								break;
							}
						}
					}
				}

				alsa_frames_to_send = byte / 4;
#ifdef ENABLE_AUDIOSERVICE
				standard_alsa_output_write(as_alsa_handle, as_alsa_handle->audiobuf, byte);
#else
				while (alsa_frames_to_send > 0) {
					if (app_avk_is_pulse()) {
						if (get_start_time == 0) {
							// Save the start time and start to count the frames send to low level
							send_frames = 0;
							gettimeofday(&start_time, NULL);
							get_start_time = 1;
						}
					}
					alsa_frames = snd_pcm_writei(app_avk_cb.alsa_handle, tmpbuf, alsa_frames_to_send);
					if (alsa_frames < 0)
					{
						if (alsa_frames == -EPIPE)
						{
							APP_DEBUG0("ALSA: underrun in frame");
							snd_pcm_prepare(app_avk_cb.alsa_handle);
							alsa_frames = 0;
						}
						else if (alsa_frames == -EBADFD)
						{
							APP_DEBUG0("ALSA: retry");
							alsa_frames = 0;
						}
						else
						{
							APP_DEBUG1("ALSA: snd_pcm_writei err %d (%s)", (int) alsa_frames, snd_strerror(alsa_frames));
							break;
						}
					}
					alsa_frames_to_send -= alsa_frames;
					if (app_avk_is_pulse()) {
						send_frames += alsa_frames;
						if (send_frames >= 32*100) {
							// calculate the sleep time
							// sleep some time avoid ring buffer is empty
							gettimeofday(&end_time);
							wait_sometime(send_frames, &start_time, &end_time);
							get_start_time = 0;
						}
					}
				}
				if (alsa_frames_to_send)
					APP_DEBUG1("ALSA: short write (discarded %li)", alsa_frames_to_send);
#endif
			} while (byte > 0);
			close_alsa();
#endif /* PCM_ALSA */
			startPlay = 0;
		}
		usleep(10 * 1000);
	}
}
#endif /* USE_RING_BUFFER */
/*******************************************************************************
**
** Function         app_avk_init
**
** Description      Init Manager application
**
** Parameters       Application callback (if null, default will be used)
**
** Returns          0 if successful, error code otherwise
**
*******************************************************************************/
int app_avk_init(tBSA_AVK_CBACK pcb)
{
	tBSA_AVK_ENABLE bsa_avk_enable_param;
	tBSA_STATUS status;

	/* Initialize the control structure */
	memset(&app_avk_cb, 0, sizeof(app_avk_cb));
	app_avk_cb.uipc_audio_channel = UIPC_CH_ID_BAD;

	app_avk_cb.fd = -1;

	/* register callback */
	app_avk_cb.p_Callback = pcb;

	/* set sytem vol at 50% */
	app_avk_cb.volume = (UINT8)((BSA_MAX_ABS_VOLUME - BSA_MIN_ABS_VOLUME) >> 1);

	/* get hold on the AVK resource, synchronous mode */
	BSA_AvkEnableInit(&bsa_avk_enable_param);

	bsa_avk_enable_param.sec_mask = BSA_AVK_SECURITY;
	bsa_avk_enable_param.features = BSA_AVK_FEATURES;
	bsa_avk_enable_param.p_cback = app_avk_cback;

	status = BSA_AvkEnable(&bsa_avk_enable_param);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to enable an AV sink with status %d", status);

		if (BSA_ERROR_SRV_AVK_AV_REGISTERED == status)
		{
			APP_ERROR0("AV was registered before AVK, AVK should be registered before AV");
		}

		return -1;
	}
#ifdef PCM_ALSA
	mixer_alloc(&mixerFd, MIXER_NAME);
#endif

#ifdef USE_RING_BUFFER
	APP_DEBUG0("Ring Buffer init");
	ring_buffer_init(&rb, RBUF_SIZE);
	pthread_create(&th_play_data, NULL, thread_play_data, NULL);
#endif
	return BSA_SUCCESS;

}

/*******************************************************************************
**
** Function         app_avk_rc_send_command
**
** Description      Example of Send a RemoteControl command
**
** Returns          void
**
*******************************************************************************/
void app_avk_rc_send_command(UINT8 key_state, UINT8 command, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_REM_CMD bsa_avk_rc_cmd;

	/* Send remote control command */
	status = BSA_AvkRemoteCmdInit(&bsa_avk_rc_cmd);
	bsa_avk_rc_cmd.rc_handle = rc_handle;
	bsa_avk_rc_cmd.key_state = (tBSA_AVK_STATE)key_state;
	bsa_avk_rc_cmd.rc_id = (tBSA_AVK_RC)command;
	bsa_avk_rc_cmd.label = app_avk_get_label();

	status = BSA_AvkRemoteCmd(&bsa_avk_rc_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send AV RC Command %d", status);
	}
}

/*******************************************************************************
**
** Function         app_avk_rc_send_click
**
** Description      Send press and release states of a command
**
** Returns          void
**
*******************************************************************************/
void app_avk_rc_send_click(UINT8 command, UINT8 rc_handle)
{
	app_avk_rc_send_command(BSA_AV_STATE_PRESS, command, rc_handle);
	GKI_delay(300);
	app_avk_rc_send_command(BSA_AV_STATE_RELEASE, command, rc_handle);
}


/*******************************************************************************
**
** Function         app_avk_play_start
**
** Description      Example of start steam play
**
** Returns          void
**
*******************************************************************************/
void app_avk_play_start(UINT8 rc_handle)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	connection = app_avk_find_connection_by_rc_handle(rc_handle);
	if (connection == NULL)
	{
		APP_DEBUG1("app_avk_play_start could not find connection handle %d", rc_handle);
		return;
	}

	if ((connection->peer_features & BSA_AVK_FEAT_RCCT) && connection->is_rc_open)
	{
		app_avk_rc_send_click(BSA_AVK_RC_PLAY, rc_handle);
	}
	else
	{
		APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
				   (connection->peer_features & BSA_AVK_FEAT_RCCT), connection->is_rc_open);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_play_stop
 **
 ** Description      Example of stop steam play
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_stop(UINT8 rc_handle)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	connection = app_avk_find_connection_by_rc_handle(rc_handle);
	if (connection == NULL)
	{
		APP_DEBUG1("app_avk_play_stop could not find connection handle %d", rc_handle);
		return;
	}

	if ((connection->peer_features & BSA_AVK_FEAT_RCCT) && connection->is_rc_open)
	{
		app_avk_rc_send_click(BSA_AVK_RC_STOP, rc_handle);
	}
	else
	{
		APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
				   (connection->peer_features & BSA_AVK_FEAT_RCCT), connection->is_rc_open);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_play_pause
 **
 ** Description      Example of pause steam pause
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_pause(UINT8 rc_handle)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	connection = app_avk_find_connection_by_rc_handle(rc_handle);
	if (connection == NULL)
	{
		APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
		return;
	}

	if ((connection->peer_features & BSA_AVK_FEAT_RCCT) && connection->is_rc_open)
	{
		app_avk_rc_send_click(BSA_AVK_RC_PAUSE, rc_handle);
	}
	else
	{
		APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
				   (connection->peer_features & BSA_AVK_FEAT_RCCT), connection->is_rc_open);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_play_next_track
 **
 ** Description      Example of play next track
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_next_track(UINT8 rc_handle)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	connection = app_avk_find_connection_by_rc_handle(rc_handle);
	if (connection == NULL)
	{
		APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
		return;
	}

	if ((connection->peer_features & BSA_AVK_FEAT_RCCT) && connection->is_rc_open)
	{
		app_avk_rc_send_click(BSA_AVK_RC_FORWARD, rc_handle);
	}
	else
	{
		APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
				   (connection->peer_features & BSA_AVK_FEAT_RCCT), connection->is_rc_open);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_play_previous_track
 **
 ** Description      Example of play previous track
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_play_previous_track(UINT8 rc_handle)
{
	tAPP_AVK_CONNECTION *connection = NULL;
	connection = app_avk_find_connection_by_rc_handle(rc_handle);
	if (connection == NULL)
	{
		APP_DEBUG1("app_avk_play_pause could not find connection handle %d", rc_handle);
		return;
	}

	if ((connection->peer_features & BSA_AVK_FEAT_RCCT) && connection->is_rc_open)
	{
		app_avk_rc_send_click(BSA_AVK_RC_BACKWARD, rc_handle);
	}
	else
	{
		APP_ERROR1("Unable to send AVRC command, is support RCTG %d, is rc open %d",
				   (connection->peer_features & BSA_AVK_FEAT_RCCT), connection->is_rc_open);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_cmd
 **
 ** Description      Example of function to open AVK connection
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_cmd(UINT8 rc_handle)
{
	int choice;

	do
	{
		printf("Bluetooth AVK AVRC CMD menu:\n");
		printf("    0 play\n");
		printf("    1 stop\n");
		printf("    2 pause\n");
		printf("    3 forward\n");
		printf("    4 backward\n");
		printf("    5 rewind key_press\n");
		printf("    6 rewind key_release\n");
		printf("    7 fast forward key_press\n");
		printf("    8 fast forward key_release\n");

		printf("    99 exit\n");

		choice = app_get_choice("Select source");

		switch (choice)
		{
		case 0:
			app_avk_rc_send_click(BSA_AVK_RC_PLAY, rc_handle);
			break;

		case 1:
			app_avk_rc_send_click(BSA_AVK_RC_STOP, rc_handle);
			break;

		case 2:
			app_avk_rc_send_click(BSA_AVK_RC_PAUSE, rc_handle);
			break;

		case 3:
			app_avk_rc_send_click(BSA_AVK_RC_FORWARD, rc_handle);
			break;

		case 4:
			app_avk_rc_send_click(BSA_AVK_RC_BACKWARD, rc_handle);
			break;

		case 5:
			app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_REWIND, rc_handle);
			break;

		case 6:
			app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_REWIND, rc_handle);
			break;

		case 7:
			app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_FAST_FOR, rc_handle);
			break;

		case 8:
			app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_FAST_FOR, rc_handle);
			break;

		default:
			APP_ERROR0("Unsupported choice");
			break;
		}
	} while (choice != 99);
}


/*******************************************************************************
 **
 ** Function         app_avk_send_get_element_attributes_cmd
 **
 ** Description      Example of function to send Vendor Dependent Command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_get_element_attributes_cmd(UINT8 rc_handle)
{
	int status;
	tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

	/* Send remote control command */
	status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

	bsa_avk_vendor_cmd.rc_handle = rc_handle;
	bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_STATUS;
	bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_GET_ELEMENT_ATTR;
	bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
	bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
	bsa_avk_vendor_cmd.data[3] = 0x11; /* length low*/

	/* data 4 ~ 11 are 0, which means "identifier 0x0 PLAYING" */

	bsa_avk_vendor_cmd.data[12] = 2; /* num of attributes */

	/* data 13 ~ 16 are 0x1, which means "attribute ID 1 : Title of media" */
	bsa_avk_vendor_cmd.data[16] = 0x1;

	/* data 17 ~ 20 are 0x7, which means "attribute ID 2 : Playing Time" */
	bsa_avk_vendor_cmd.data[20] = 0x7;

	bsa_avk_vendor_cmd.length = 21;
	bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send AV Vendor Command %d", status);
	}
}

/*******************************************************************************
**
** Function         avk_is_open_pending
**
** Description      Check if AVK Open is pending
**
** Parameters
**
** Returns          TRUE if open is pending, FALSE otherwise
**
*******************************************************************************/
BOOLEAN avk_is_open_pending()
{
	return app_avk_cb.open_pending;
}

/*******************************************************************************
**
** Function         avk_set_open_pending
**
** Description      Set AVK open pending
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void avk_set_open_pending(BOOLEAN bopenpend)
{
	app_avk_cb.open_pending = bopenpend;
}

/*******************************************************************************
**
** Function         avk_is_open
**
** Description      Check if AVK is open
**
** Parameters       BDA of device to check
**
** Returns          TRUE if AVK is open, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN avk_is_open(BD_ADDR bda)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if ((app_avk_cb.connections[index].in_use == TRUE) &&
				(app_avk_cb.connections[index].is_open == TRUE))
		{
			if (bdcmp(bda, app_avk_cb.connections[index].bda_connected) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

/*******************************************************************************
**
** Function         avk_is_any_open
**
** Description      Check if any AVK is open
**
** Parameters
**
** Returns          TRUE if AVK is open, FALSE otherwise. Return BDA of the connected device
**
*******************************************************************************/
BOOLEAN avk_is_any_open(BD_ADDR bda /* out */)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if ((app_avk_cb.connections[index].in_use == TRUE) &&
				(app_avk_cb.connections[index].is_open == TRUE))
		{
			bdcpy(bda, app_avk_cb.connections[index].bda_connected);
			return TRUE;
		}
	}

	return FALSE;
}

/*******************************************************************************
**
** Function         avk_is_rc_open
**
** Description      Check if AVRC is open
**
** Parameters
**
** Returns          TRUE if AVRC is open, FALSE otherwise.
**
*******************************************************************************/
BOOLEAN avk_is_rc_open(BD_ADDR bda)
{
	tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_bd_addr(bda);

	if (connection == NULL)
		return FALSE;

	return connection->is_rc_open;
}

/*******************************************************************************
**
** Function         app_avk_cancel
**
** Description      Example of cancel connection command
**
** Returns          void
**
*******************************************************************************/
void app_avk_cancel(BD_ADDR bda)
{
	tBSA_STATUS status;
	tBSA_AVK_CANCEL_CMD bsa_avk_cancel_cmd;

	/* Send remote control command */
	status = BSA_AvkCancelCmdInit(&bsa_avk_cancel_cmd);

	bdcpy(bsa_avk_cancel_cmd.bda, bda);

	status = BSA_AvkCancelCmd(&bsa_avk_cancel_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_cancel_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_element_attr_command
 **
 ** Description      Example of get_element_attr_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_element_attr_command(UINT8 rc_handle)
{
	int status;
	tBSA_AVK_GET_ELEMENT_ATTR bsa_avk_get_elem_attr_cmd;

	UINT8 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
					  AVRC_MEDIA_ATTR_ID_ARTIST,
					  AVRC_MEDIA_ATTR_ID_ALBUM,
					  AVRC_MEDIA_ATTR_ID_TRACK_NUM,
					  AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
					  AVRC_MEDIA_ATTR_ID_GENRE,
					  AVRC_MEDIA_ATTR_ID_PLAYING_TIME
					 };
	UINT8 num_attr  = 7;

	/* Send command */
	status = BSA_AvkGetElementAttrCmdInit(&bsa_avk_get_elem_attr_cmd);
	bsa_avk_get_elem_attr_cmd.rc_handle = rc_handle;
	bsa_avk_get_elem_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_avk_get_elem_attr_cmd.num_attr = num_attr;
	memcpy(bsa_avk_get_elem_attr_cmd.attrs, attrs, sizeof(attrs));

	status = BSA_AvkGetElementAttrCmd(&bsa_avk_get_elem_attr_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_get_element_attr_command %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_list_player_attr_command
 **
 ** Description      Example of app_avk_rc_list_player_attr_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_list_player_attr_command(UINT8 rc_handle)
{
	int status;
	tBSA_AVK_LIST_PLAYER_ATTR bsa_avk_list_player_attr_cmd;

	/* Send command */
	status = BSA_AvkListPlayerAttrCmdInit(&bsa_avk_list_player_attr_cmd);
	bsa_avk_list_player_attr_cmd.rc_handle = rc_handle;
	bsa_avk_list_player_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkListPlayerAttrCmd(&bsa_avk_list_player_attr_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_list_player_attr_command %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_list_player_attr_value_command
 **
 ** Description      Example of app_avk_rc_list_player_attr_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_list_player_attr_value_command(UINT8 attr, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_LIST_PLAYER_VALUES bsa_avk_list_player_attr_cmd;

	/* Send command */
	status = BSA_AvkListPlayerValuesCmdInit(&bsa_avk_list_player_attr_cmd);
	bsa_avk_list_player_attr_cmd.rc_handle = rc_handle;
	bsa_avk_list_player_attr_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
	bsa_avk_list_player_attr_cmd.attr = attr;

	status = BSA_AvkListPlayerValuesCmd(&bsa_avk_list_player_attr_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_list_player_attr_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_player_value_command
 **
 ** Description      Example of get_player_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_player_value_command(UINT8 *attrs, UINT8 num_attr, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_GET_PLAYER_VALUE bsa_avk_get_player_val_cmd;

	/* Send command */
	status = BSA_AvkGetPlayerValueCmdInit(&bsa_avk_get_player_val_cmd);
	bsa_avk_get_player_val_cmd.rc_handle = rc_handle;
	bsa_avk_get_player_val_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
	bsa_avk_get_player_val_cmd.num_attr = num_attr;

	memcpy(bsa_avk_get_player_val_cmd.attrs, attrs, sizeof(UINT8) * num_attr);

	status = BSA_AvkGetPlayerValueCmd(&bsa_avk_get_player_val_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_get_player_value_command %d", status);
	}
}



/*******************************************************************************
 **
 ** Function         app_avk_rc_set_player_value_command
 **
 ** Description      Example of set_player_value_command command
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_player_value_command(UINT8 num_attr, UINT8 *attr_ids, UINT8 * attr_val, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_SET_PLAYER_VALUE bsa_avk_set_player_val_cmd;

	/* Send command */
	status = BSA_AvkSetPlayerValueCmdInit(&bsa_avk_set_player_val_cmd);
	bsa_avk_set_player_val_cmd.rc_handle = rc_handle;
	bsa_avk_set_player_val_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */
	bsa_avk_set_player_val_cmd.num_attr = num_attr;

	memcpy(bsa_avk_set_player_val_cmd.player_attr, attr_ids, sizeof(UINT8) * num_attr);
	memcpy(bsa_avk_set_player_val_cmd.player_value, attr_val, sizeof(UINT8) * num_attr);

	status = BSA_AvkSetPlayerValueCmd(&bsa_avk_set_player_val_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_set_player_value_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_play_status_command
 **
 ** Description      Example of get_play_status
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_play_status_command(UINT8 rc_handle)
{
	int status;
	tBSA_AVK_GET_PLAY_STATUS bsa_avk_play_status_cmd;

	/* Send command */
	status = BSA_AvkGetPlayStatusCmdInit(&bsa_avk_play_status_cmd);
	bsa_avk_play_status_cmd.rc_handle = rc_handle;
	bsa_avk_play_status_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkGetPlayStatusCmd(&bsa_avk_play_status_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_get_play_status_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_set_browsed_player_command
 **
 ** Description      Example of set_browsed_player
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_browsed_player_command(UINT16  player_id, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_SET_BROWSED_PLAYER bsa_avk_set_browsed_player;

	/* Send command */
	status = BSA_AvkSetBrowsedPlayerCmdInit(&bsa_avk_set_browsed_player);
	bsa_avk_set_browsed_player.player_id = player_id;
	bsa_avk_set_browsed_player.rc_handle = rc_handle;
	bsa_avk_set_browsed_player.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkSetBrowsedPlayerCmd(&bsa_avk_set_browsed_player);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_set_browsed_player_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_set_addressed_player_command
 **
 ** Description      Example of set_addressed_player
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_set_addressed_player_command(UINT16  player_id, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_SET_ADDR_PLAYER bsa_avk_set_player;

	/* Send command */
	status = BSA_AvkSetAddressedPlayerCmdInit(&bsa_avk_set_player);
	bsa_avk_set_player.player_id = player_id;
	bsa_avk_set_player.rc_handle = rc_handle;
	bsa_avk_set_player.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkSetAddressedPlayerCmd(&bsa_avk_set_player);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_set_addressed_player_command %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_change_path_command
 **
 ** Description      Example of change_path
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_change_path_command(UINT16   uid_counter, UINT8  direction, tAVRC_UID folder_uid, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_CHG_PATH bsa_change_path;

	/* Send command */
	status = BSA_AvkChangePathCmdInit(&bsa_change_path);

	bsa_change_path.rc_handle = rc_handle;
	bsa_change_path.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_change_path.uid_counter = uid_counter;
	bsa_change_path.direction = direction;
	memcpy(bsa_change_path.folder_uid, folder_uid, sizeof(tAVRC_UID));

	status = BSA_AvkChangePathCmd(&bsa_change_path);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_change_path_command %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_get_folder_items
 **
 ** Description      Example of get_folder_items
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_folder_items(UINT8  scope, UINT32  start_item, UINT32  end_item, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_GET_FOLDER_ITEMS bsa_get_folder_items;

	UINT32 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
					   AVRC_MEDIA_ATTR_ID_ARTIST,
					   AVRC_MEDIA_ATTR_ID_ALBUM,
					   AVRC_MEDIA_ATTR_ID_TRACK_NUM,
					   AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
					   AVRC_MEDIA_ATTR_ID_GENRE,
					   AVRC_MEDIA_ATTR_ID_PLAYING_TIME
					  };
	UINT8 num_attr  = 7;

	/* Send command */
	status = BSA_AvkGetFolderItemsCmdInit(&bsa_get_folder_items);

	bsa_get_folder_items.rc_handle = rc_handle;
	bsa_get_folder_items.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_get_folder_items.scope = scope;
	bsa_get_folder_items.start_item = start_item;
	bsa_get_folder_items.end_item = end_item;

	if (AVRC_SCOPE_PLAYER_LIST != scope)
	{
		bsa_get_folder_items.attr_count = num_attr;
		memcpy(bsa_get_folder_items.attrs, attrs, sizeof(attrs));
	}


	status = BSA_AvkGetFolderItemsCmd(&bsa_get_folder_items);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_get_folder_items %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_rc_get_items_attr
 **
 ** Description      Example of get_items_attr
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_get_items_attr(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_GET_ITEMS_ATTR bsa_get_items_attr;

	UINT8 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
					  AVRC_MEDIA_ATTR_ID_ARTIST,
					  AVRC_MEDIA_ATTR_ID_ALBUM,
					  AVRC_MEDIA_ATTR_ID_TRACK_NUM,
					  AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
					  AVRC_MEDIA_ATTR_ID_GENRE,
					  AVRC_MEDIA_ATTR_ID_PLAYING_TIME
					 };
	UINT8 num_attr  = 7;

	/* Send command */
	status = BSA_AvkGetItemsAttrCmdInit(&bsa_get_items_attr);

	bsa_get_items_attr.rc_handle = rc_handle;
	bsa_get_items_attr.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_get_items_attr.scope = scope;
	memcpy(bsa_get_items_attr.uid, uid, sizeof(tAVRC_UID));
	bsa_get_items_attr.uid_counter = uid_counter;
	bsa_get_items_attr.attr_count = num_attr;
	memcpy(bsa_get_items_attr.attrs, attrs, sizeof(attrs));

	status = BSA_AvkGetItemsAttrCmd(&bsa_get_items_attr);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_get_items_attr %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_play_item
 **
 ** Description      Example of play_item
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_play_item(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_PLAY_ITEM bsa_play_item;


	/* Send command */
	status = BSA_AvkPlayItemCmdInit(&bsa_play_item);

	bsa_play_item.rc_handle = rc_handle;
	bsa_play_item.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_play_item.scope = scope;
	memcpy(bsa_play_item.uid, uid, sizeof(tAVRC_UID));
	bsa_play_item.uid_counter = uid_counter;

	status = BSA_AvkPlayItemCmd(&bsa_play_item);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_play_item %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_rc_add_to_play
 **
 ** Description      Example of add_to_play
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_rc_add_to_play(UINT8  scope, tAVRC_UID  uid, UINT16  uid_counter, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_ADD_TO_PLAY bsa_add_play;


	/* Send command */
	status = BSA_AvkAddToPlayCmdInit(&bsa_add_play);

	bsa_add_play.rc_handle = rc_handle;
	bsa_add_play.label = app_avk_get_label(); /* Just used to distinguish commands */

	bsa_add_play.scope = scope;
	memcpy(bsa_add_play.uid, uid, sizeof(tAVRC_UID));
	bsa_add_play.uid_counter = uid_counter;

	status = BSA_AvkAddToPlayCmd(&bsa_add_play);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send app_avk_rc_add_to_play %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_set_abs_vol_rsp
 **
 ** Description      This function sends abs vol response
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_avk_set_abs_vol_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label)
{
	int status;
	tBSA_AVK_SET_ABS_VOLUME_RSP abs_vol;
	BSA_AvkSetAbsVolumeRspInit(&abs_vol);

	abs_vol.label = label;
	abs_vol.rc_handle = rc_handle;
	abs_vol.volume = volume;
	status = BSA_AvkSetAbsVolumeRsp(&abs_vol);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to send app_avk_set_abs_vol_rsp %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_reg_notfn_rsp
 **
 ** Description      This function sends reg notfn response (BSA as TG role in AVK)
 **
 ** Returns          none
 **
 *******************************************************************************/
void app_avk_reg_notfn_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label, UINT8 event, tBSA_AVK_CODE code)
{
	int status;
	tBSA_AVK_REG_NOTIF_RSP reg_notf;
	BSA_AvkRegNotifRspInit(&reg_notf);

	reg_notf.reg_notf.param.volume  = volume;
	reg_notf.reg_notf.event_id      = event;
	reg_notf.rc_handle              = rc_handle;
	reg_notf.label                  = label;
	reg_notf.code                   = code;

	status = BSA_AvkRegNotifRsp(&reg_notf);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to send app_avk_reg_notfn_rsp %d", status);
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_send_get_capabilities
 **
 ** Description      Sample code for attaining capability for events
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_get_capabilities(UINT8 rc_handle)
{
	int status;
	tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

	/* Send remote control command */
	status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

	bsa_avk_vendor_cmd.rc_handle = rc_handle;
	bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_STATUS;
	bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_GET_CAPABILITIES;
	bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
	bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
	bsa_avk_vendor_cmd.data[3] = 1; /* length low*/
	bsa_avk_vendor_cmd.data[4] = 0x03; /* for event*/

	bsa_avk_vendor_cmd.length = 5;
	bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send AV Vendor Command %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_send_register_notification
 **
 ** Description      Sample code for attaining capability for events
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_register_notification(int evt, UINT8 rc_handle)
{
	int status;
	tBSA_AVK_VEN_CMD bsa_avk_vendor_cmd;

	/* Send remote control command */
	status = BSA_AvkVendorCmdInit(&bsa_avk_vendor_cmd);

	bsa_avk_vendor_cmd.rc_handle = rc_handle;
	bsa_avk_vendor_cmd.ctype = BSA_AVK_CMD_NOTIF;
	bsa_avk_vendor_cmd.data[0] = BSA_AVK_RC_VD_REGISTER_NOTIFICATION;
	bsa_avk_vendor_cmd.data[1] = 0; /* reserved & packet type */
	bsa_avk_vendor_cmd.data[2] = 0; /* length high*/
	bsa_avk_vendor_cmd.data[3] = 5; /* length low*/
	bsa_avk_vendor_cmd.data[4] = evt; /* length low*/

	bsa_avk_vendor_cmd.length = 9;
	bsa_avk_vendor_cmd.label = app_avk_get_label(); /* Just used to distinguish commands */

	status = BSA_AvkVendorCmd(&bsa_avk_vendor_cmd);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send AV Vendor Command %d", status);
	}
}

/*******************************************************************************
 **
 ** Function         app_avk_add_connection
 **
 ** Description      This function adds a connection
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_add_connection(BD_ADDR bd_addr)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0)
		{
			app_avk_cb.connections[index].in_use = TRUE;
			return &app_avk_cb.connections[index];
		}

		if (app_avk_cb.connections[index].in_use == FALSE)
		{
			bdcpy(app_avk_cb.connections[index].bda_connected, bd_addr);
			app_avk_cb.connections[index].in_use = TRUE;
			app_avk_cb.connections[index].index = index;
			return &app_avk_cb.connections[index];
		}
	}
	return NULL;
}

/*******************************************************************************
 **
 ** Function         app_avk_reset_connection
 **
 ** Description      This function resets a connection
 **
 **
 *******************************************************************************/
void app_avk_reset_connection(BD_ADDR bd_addr)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0)
		{
			app_avk_cb.connections[index].in_use = FALSE;

			app_avk_cb.connections[index].m_uiAddressedPlayer = 0;
			app_avk_cb.connections[index].m_uidCounterAddrPlayer = 0;
			app_avk_cb.connections[index].m_uid_counter = 0;
			app_avk_cb.connections[index].m_bDeviceSupportBrowse = FALSE;
			app_avk_cb.connections[index].m_uiBrowsedPlayer = 0;
			app_avk_cb.connections[index].m_iCurrentFolderItemCount = 0;
			app_avk_cb.connections[index].m_bAbsVolumeSupported = FALSE;
			app_avk_cb.connections[index].is_streaming_chl_open = FALSE;
			app_avk_cb.connections[index].is_started_streaming = FALSE;
			break;
		}
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_av_handle
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_av_handle(UINT8 handle)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (app_avk_cb.connections[index].ccb_handle == handle)
			return &app_avk_cb.connections[index];
	}
	return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_rc_handle
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_rc_handle(UINT8 handle)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (app_avk_cb.connections[index].rc_handle == handle)
			return &app_avk_cb.connections[index];
	}
	return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_bd_addr
 **
 ** Description      This function finds the connection structure by its handle
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_bd_addr(BD_ADDR bd_addr)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if ((bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) == 0) &&
				(app_avk_cb.connections[index].in_use == TRUE))
			return &app_avk_cb.connections[index];
	}
	return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_find_streaming_connection
 **
 ** Description      This function finds the connection structure that is streaming
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_streaming_connection()
{
	int index;

	if (app_avk_cb.pStreamingConn &&
			app_avk_cb.pStreamingConn->is_streaming_chl_open &&
			app_avk_cb.pStreamingConn->is_started_streaming)
	{
		return app_avk_cb.pStreamingConn;
	}

	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (app_avk_cb.connections[index].is_streaming_chl_open == TRUE &&
				app_avk_cb.connections[index].is_started_streaming == TRUE )
			return &app_avk_cb.connections[index];
	}
	return NULL;
}

/*******************************************************************************
 **
 ** Function         app_avk_num_connections
 **
 ** Description      This function number of connections
 **
 ** Returns          number of connections
 **
 *******************************************************************************/
int app_avk_num_connections()
{
	int iCount = 0;
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if (app_avk_cb.connections[index].in_use == TRUE)
			iCount++;
	}
	return iCount;
}

/*******************************************************************************
 **
 ** Function         app_avk_display_connections
 **
 ** Description      This function displays the connections
 **
 ** Returns          status
 **
 *******************************************************************************/
void app_avk_display_connections(void)
{
	int index;
	tAPP_AVK_CONNECTION *conn = NULL;

	int num_conn = app_avk_num_connections();

	if (num_conn == 0)
	{
		APP_INFO0("    No connections");
		return;
	}

	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		conn = app_avk_find_connection_by_index(index);
		if (conn->in_use)
		{
			APP_INFO1("Connection index %d:-%02X:%02X:%02X:%02X:%02X:%02X",
					  index,
					  conn->bda_connected[0], conn->bda_connected[1],
					  conn->bda_connected[2], conn->bda_connected[3],
					  conn->bda_connected[4], conn->bda_connected[5]);
		}
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_find_connection_by_index
 **
 ** Description      This function finds the connection structure by its index
 **
 ** Returns          Pointer to the found structure or NULL
 **
 *******************************************************************************/
tAPP_AVK_CONNECTION *app_avk_find_connection_by_index(int index)
{
	if (index < APP_AVK_MAX_CONNECTIONS)
		return &app_avk_cb.connections[index];

	return NULL;
}


/*******************************************************************************
 **
 ** Function         app_avk_pause_other_streams
 **
 ** Description      This function pauses other streams when a new stream start
 **                  is received from device identified by bd_addr
 **
 ** Returns          None
 **
 *******************************************************************************/
void app_avk_pause_other_streams(BD_ADDR bd_addr)
{
	int index;
	for (index = 0; index < APP_AVK_MAX_CONNECTIONS; index++)
	{
		if ((bdcmp(app_avk_cb.connections[index].bda_connected, bd_addr) != 0) &&
				app_avk_cb.connections[index].in_use  &&
				app_avk_cb.connections[index].is_streaming_chl_open)
		{
			app_avk_cb.connections[index].is_started_streaming = FALSE;
		}
	}
}


/*******************************************************************************
 **
 ** Function         app_avk_send_delay_report
 **
 ** Description      Sample code to send delay report
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_avk_send_delay_report(UINT16 delay)
{
	tBSA_AVK_DELAY_REPORT report;
	int status;

	APP_DEBUG1("send delay report:%d", delay);

	BSA_AvkDelayReportInit(&report);
	report.delay = delay;
	status = BSA_AvkDelayReport(&report);
	if (status != BSA_SUCCESS)
	{
		APP_ERROR1("Unable to Send delay report %d", status);
	}
}
