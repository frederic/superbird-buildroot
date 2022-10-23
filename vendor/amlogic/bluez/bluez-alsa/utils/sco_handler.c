#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "shared/log.h"
#include <alsa/asoundlib.h>
#include "sco_handler.h"
#include <pthread.h>

/****************************************/
/*	Stream Direction:					*/
/*	1. Income Stream: BT PCM -> SPEAKER	*/
/*	2. Output Stream: MIC -> BT PCM		*/
/****************************************/

#define INFO(fmt, args...) \
	printf("[SCO_HANDLER][%s] " fmt, __func__, ##args)

static pthread_t sco_rx_thread;
static pthread_t sco_tx_thread;
static int sco_enabled = 0;

static char pcm_device_mic[]	= "plug:microphone";
static char pcm_device_btpcm[]	= "hw:0,0";
static char *pcm_device_spk		;//= "dmixer_avs_auto";
static void get_spk_device();
static void *sco_rx_cb(void *arg);
static void *sco_tx_cb(void *arg);


int set_sco_enable(int enable)
{
	INFO("%s sco stream\n", enable == 0 ? "Disable" : "Enable");
	if (sco_enabled == enable) {
		INFO("sco stream %s already\n", enable == 0 ? "disabled" : "enabled");
		return 0;
	}

	sco_enabled = enable;
	if (enable) {
		get_spk_device();
		if (pthread_create(&sco_rx_thread, NULL, sco_rx_cb, NULL)) {
			INFO("rx thread create failed: %s\n", strerror(errno));
			sco_enabled = 0;
			return -1;
		} else
			pthread_setname_np(sco_rx_thread, "sco_rx_thread");

		if (pthread_create(&sco_tx_thread, NULL, sco_tx_cb, NULL)) {
			INFO("tx thread create failed: %s\n", strerror(errno));
			sco_enabled = 0;
			return -1;
		} else
			pthread_setname_np(sco_tx_thread, "sco_tx_thread");

	} else {
		if (sco_rx_thread) {
			pthread_detach(sco_rx_thread);
			sco_rx_thread  = 0;
		}
		if (sco_tx_thread) {
			pthread_detach(sco_tx_thread);
			sco_tx_thread  = 0;
		}
	}

	return 0;

}

#define APP_ALSA_DEVICE_CONF_PATH "/etc/alsa_bsa.conf"
#define DEFAULT_SPK_DEVICE "dmixer_avs_auto"
static void get_spk_device()
{
	FILE *fp;
	struct stat st;
	static char tmp[100];
	int i = 0;

	fp = fopen(APP_ALSA_DEVICE_CONF_PATH, "r");
	if (fp  == NULL) {
		INFO("open ALSA DEVICE CONF error, use default");
		strncpy(tmp, DEFAULT_SPK_DEVICE, strlen(DEFAULT_SPK_DEVICE));
		pcm_device_spk = tmp;
		return;
	}

	fgets(tmp, sizeof(tmp), fp);
	while (tmp[i++] != '=');
	pcm_device_spk = strdup(tmp + i);
	*(pcm_device_spk + strlen(pcm_device_spk) - 1) = '\0';
	INFO("spk device = %s\n", pcm_device_spk);
	fclose(fp);

}

static void *sco_rx_cb(void *arg)
{
	snd_pcm_t *pcm_handle_capture;
	snd_pcm_t *pcm_handle_playback;
	snd_pcm_hw_params_t *snd_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int channels = 1, expected_frames = 60;
	int dir, status, buf_size, rate = 8000;
	char *buf;
	snd_pcm_uframes_t frames = 120;

	INFO("setting bt pcm\n");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_capture, pcm_device_btpcm,
						  SND_PCM_STREAM_CAPTURE,	/*bt pcm as input device*/
						  0);		 				/*ASYNC MODE*/


	if (status < 0) {
		INFO("bt pcm open fail:%s\n", strerror(errno));
		return NULL;
	}
#if 0
	status = snd_pcm_set_params(pcm_handle_capture, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								0,				/*allow alsa resample*/
								100000);		/*max latence = 100ms*/
#endif
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pcm_handle_capture, snd_params);
	snd_pcm_hw_params_set_access(pcm_handle_capture, snd_params,
								 SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle_capture, snd_params, format);
	snd_pcm_hw_params_set_channels(pcm_handle_capture, snd_params, channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle_capture, snd_params, &rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm_handle_capture, snd_params, &frames, &dir);


	status = snd_pcm_hw_params(pcm_handle_capture, snd_params);

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		return NULL;
	}

	INFO("setting speaker\n");
	/*********SPEAKER ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, pcm_device_spk,
						  SND_PCM_STREAM_PLAYBACK,	/*speaker as output device*/
						  1);						/*NOBLOCK MODE*/
	if (status < 0) {
		INFO("bt pcm open fail:%s\n", strerror(errno));
		return NULL;
	}

	status = snd_pcm_set_params(pcm_handle_playback, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								1,				/*allow alsa resample*/
								500000);		/*expected max latence = 500ms*/

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		snd_pcm_close(pcm_handle_playback);
		return NULL;
	}

#ifdef DEBUG_STREAM
	FILE *fp;
	fp = fopen("/tmp/rx_cb.wav", "w+");
	if (fp  == NULL)
		INFO("create wav file error");
#endif

	INFO("start streaming\n");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size = expected_frames * 1 * 2;	/*bytes = frames * ch * 16Bit/8 */
	buf = malloc(buf_size);
	frames = 0;
	while (sco_enabled) {
		memset(buf, 0, buf_size);
		frames = snd_pcm_readi(pcm_handle_capture, buf, expected_frames);
		if (frames == -EPIPE) {
			INFO("pcm read underrun\n");
			snd_pcm_prepare(pcm_handle_capture);
			frames = 0;
			continue;
		} else if (frames == -EBADFD) {
			INFO("pcm read EBADFD, retring\n");
			frames = 0;
			continue;
		}

#ifdef DEBUG_STREAM
		if (fp)
			fwrite(buf, frames * 2, 1, fp);
#endif

		frames = snd_pcm_writei(pcm_handle_playback, buf, frames);
		/*if write failed somehow, just ignore, we don't want to wast too much time*/
		if (frames == -EPIPE) {
			INFO("speaker write underrun\n");
			snd_pcm_prepare(pcm_handle_playback);
		} else if (frames == -EBADFD) {
			INFO("speaker write  EBADFD\n");
		}
	}
	INFO("stopping\n");
	/********STOP**********************************************/
	snd_pcm_close(pcm_handle_capture);
	snd_pcm_close(pcm_handle_playback);
	free(buf);
#ifdef DEBUG_STREAM
	fclose(fp);
#endif

	return NULL;
}


static void *sco_tx_cb(void *arg)
{
	snd_pcm_t *pcm_handle_capture;
	snd_pcm_t *pcm_handle_playback;
	snd_pcm_hw_params_t *snd_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int mic_channels = 2, expected_frames = 60;
	int dir, status, buf_size, buf2_size, i, rate = 8000;
	char *buf, *buf2;
	snd_pcm_uframes_t frames = 120;

	INFO("setting mic\n");
	/********MIC ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_capture, pcm_device_mic,
						  SND_PCM_STREAM_CAPTURE,	/*mic as input device*/
						  0);						/*ASYNC MODE*/


	if (status < 0) {
		INFO("mic open fail:%s\n", strerror(errno));
		return NULL;
	}
#if 0
	status = snd_pcm_set_params(pcm_handle_capture, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								0,				/*allow alsa resample*/
								100000);		/*max latence = 100ms*/
#endif
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pcm_handle_capture, snd_params);
	snd_pcm_hw_params_set_access(pcm_handle_capture, snd_params,
								 SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle_capture, snd_params, format);
	snd_pcm_hw_params_set_channels(pcm_handle_capture, snd_params, mic_channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle_capture, snd_params, &rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm_handle_capture, snd_params, &frames, &dir);


	status = snd_pcm_hw_params(pcm_handle_capture, snd_params);

	if (status < 0) {
		INFO("mic set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		return NULL;
	}

	INFO("setting bt pcm\n");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, pcm_device_btpcm,
						  SND_PCM_STREAM_PLAYBACK,	/*bt pcm as output device*/
						  1);						/*NOBLOCK MODE*/
	if (status < 0) {
		INFO("bt pcm open fail:%s\n", strerror(errno));
		return NULL;
	}

	status = snd_pcm_set_params(pcm_handle_playback, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								1,				/*allow alsa resample*/
								500000);		/*expected max latence = 500ms*/

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		snd_pcm_close(pcm_handle_playback);
		return NULL;
	}

#ifdef DEBUG_STREAM
	FILE *fp;
	fp = fopen("/tmp/tx_cb.wav", "w+");
	if (fp  == NULL)
		INFO("create wav file error");
#endif

	INFO("start streaming\n");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size  = expected_frames * mic_channels * 2;	/*bytes = frames * ch * 16Bit/8 */
	buf2_size = expected_frames * 1 * 2;			/*bytes = frames * ch * 16Bit/8 */
	buf  = malloc(buf_size);
	buf2 = malloc(buf2_size);

	while (sco_enabled) {

		memset(buf, 0, buf_size);
		memset(buf2, 0, buf2_size);

		frames = snd_pcm_readi(pcm_handle_capture, buf, expected_frames);
		if (frames == -EPIPE) {
			INFO("mic read underrun\n");
			snd_pcm_prepare(pcm_handle_capture);
			frames = 0;
			continue;
		} else if (frames == -EBADFD) {
			INFO("mic read EBADFD, retring\n");
			frames = 0;
			continue;
		}

#ifdef DEBUG_STREAM
		if (fp)
			fwrite(buf, frames * mic_channels * 2, 1, fp);
#endif

		/*multi channel data -> 1channel data resample*/
		for (i = 0; i < buf2_size; i++)
			buf2[i] = buf[i * mic_channels + i % mic_channels];

		frames = snd_pcm_writei(pcm_handle_playback, buf2, frames);
		/*if write failed somehow, just ignore, we don't want to wast too much time*/
		if (frames == -EPIPE) {
			INFO("bt pcm write underrun\n");
			snd_pcm_prepare(pcm_handle_playback);
		} else if (frames == -EBADFD) {
			INFO("bt pcm write  EBADFD\n");
		}
	}
	INFO("stopping\n");
	/********STOP**********************************************/
	snd_pcm_close(pcm_handle_capture);
	snd_pcm_close(pcm_handle_playback);
	free(buf);
	free(buf2);
#ifdef DEBUG_STREAM
	fclose(fp);
#endif

	return NULL;
}

