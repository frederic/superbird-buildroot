/**
 ** aml_hw_profile.c
 **
 ** This program is APIs for get aml sound card/port.
 ** author: shen pengru
 **
 */
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "log.h"
//#include <cutils/str_parms.h>
//#include <cutils/properties.h>
#include "hardware.h"
#include "audio_core.h"
#include "audio_hal.h"
#include <aml_hw_profile.h>
#include <aml_android_utils.h>
#include <string.h>
#include <unistd.h>


#undef  LOG_TAG
#define LOG_TAG "audio_hw_profile"

extern int aml_sysfs_get_int16(const char *path, unsigned *val);

int aml_get_sound_card_main()
{
	int card = -1, err = 0;
	int fd = -1;
	unsigned fileSize = 512;
	char *read_buf = NULL, *pd = NULL;

	fd = open(SOUND_CARDS_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("%s: failed to open config file %s error: %s\n",
			__func__,
			SOUND_CARDS_PATH, strerror(errno));
		close(fd);
		return -EINVAL;
	}

	read_buf = (char *) malloc(fileSize);
	if (!read_buf) {
		ALOGE("%s: Failed to malloc read_buf, error: %s",
			__func__, strerror(errno));
		close(fd);
		return -ENOMEM;
	}
	memset(read_buf, 0x0, fileSize);
	err = read(fd, read_buf, (fileSize-1));
	if (err < 0) {
		ALOGE("%s: failed to read config file %s error: %s\n",
			__func__,
			SOUND_CARDS_PATH, strerror(errno));
		close(fd);
		free(read_buf);
		return -EINVAL;
	}
	pd = strstr(read_buf, "AML");
	if ((pd != NULL) && (pd >= (read_buf+3))) {///< check the pd pointer.
		card = *(pd - 3) - '0';
	}
OUT:
	free(read_buf);
	close(fd);
	return card;
}

int aml_get_sound_card_ext(ePcmStreamType stream_type)
{
	int card_num = 1;       // start num, 0 is defualt sound card.
	struct stat card_stat;
	char fpath[256];
	int ret;

	while (card_num <= MAX_CARD_NUM) {
		snprintf(fpath, sizeof(fpath), "/proc/asound/card%d", card_num);
		ret = stat(fpath, &card_stat);
		if (ret < 0) {
			ret = -1;
		} else {
			snprintf(fpath, sizeof(fpath), "/dev/snd/pcmC%uD0%c", card_num,
				stream_type ? 'c' : 'p');
			ret = stat(fpath, &card_stat);
			if (ret == 0) {
				return card_num;
			}
		}
		card_num++;
	}

	return ret;
}

static int _get_audio_port(char *name, int name_len)
{
	int port = -1, err = 0;
	int fd = -1;
	unsigned fileSize = 512;
	char *read_buf = NULL, *pd = NULL;

	fd = open(SOUND_PCM_PATH, O_RDONLY);
	if (fd < 0) {
		ALOGE("%s: failed to open config file %s error: %s\n",
			__func__,
			SOUND_PCM_PATH, strerror(errno));
		close(fd);
		return -EINVAL;
	}
	read_buf = (char *) malloc(fileSize);
	if (!read_buf) {
		ALOGE("%s: Failed to malloc read_buf error: %s", __func__, strerror(errno));
		close(fd);
		return -ENOMEM;
	}
	memset(read_buf, 0x0, fileSize);
	err = read(fd, read_buf, (fileSize-1));
	if (err < 0) {
		ALOGE("%s: failed to read config file %s error: %s\n",
			__func__,
			SOUND_PCM_PATH, strerror(errno));
		close(fd);
		free(read_buf);
		return -EINVAL;
	}
	pd = strstr(read_buf, name);
	///< pd is the ptr when name first occur in read_buf
	///< check the pd pointer.
	if ((pd != NULL) && (pd >= (read_buf+name_len))) {
		port = *(pd - name_len) - '0';
	}

OUT:
	free(read_buf);
	close(fd);

	return port;
}

int aml_get_i2s_port(void)
{
	return _get_audio_port(PCM_I2S_STRING, 3);
}

int aml_get_spdif_port(void)
{
	return _get_audio_port(PCM_SPDIF_STRING, 3);
}

int aml_get_pcm2bt_port(void)
{
	return _get_audio_port(PCM_PCM2BT_STRING, 11);
}

/******************************************************************************
 * CodingType MaxChannels SamplingFreq SampleSize
 * ------------------------------------------------------------------------
 *     PCM,            2 ch, 32/44.1/48/88.2/96/176.4/192 kHz, 16/20/24 bit
 *     PCM,            8 ch, 32/44.1/48/88.2/96/176.4/192 kHz, 16/20/24 bit
 *     AC-3,           8 ch, 32/44.1/48 kHz,                            bit
 *     DTS,            8 ch, 44.1/48 kHz,                               bit
 *     OneBitAudio,    2 ch, 44.1 kHz,                                  bit
 *     Dobly_Digital+, 8 ch, 44.1/48 kHz,                            16 bit
 *     DTS-HD,         8 ch, 44.1/48/88.2/96/176.4/192 kHz,          16 bit
 *     MAT,            8 ch, 32/44.1/48/88.2/96/176.4/192 kHz,       16 bit
 *****************************************************************************/
char* aml_get_hdmi_sink_cap(const char *keys)
{
	int i = 0;
	char * infobuf = NULL;
	int channel = 0;
	int dgraw = 0;
	int fd = -1;
	int size = 0;
	char *aud_cap = NULL;

	infobuf = (char *)malloc(1024 * sizeof(char));
	if (infobuf == NULL) {
		ALOGE("%s: malloc buffer failed, error: %s\n",
			__func__, strerror(errno));
		goto fail;
	}

	aud_cap = (char*)malloc(1024);
	if (aud_cap == NULL) {
		ALOGE("%s: malloc buffer failed, error: %s\n",
			__func__, strerror(errno));
		goto fail;
	}

	memset(aud_cap, 0, 1024);
	memset(infobuf, 0, 1024);

	fd = open("/sys/class/amhdmitx/amhdmitx0/aud_cap", O_RDONLY);
	if (fd >= 0) {
		int nread = read(fd, infobuf, 1024);
		/* check the format cap */
		if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
			size += sprintf(aud_cap, "=");
			if (aml_strstr(infobuf, "Dobly_Digital+")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_E_AC3");
			}
			if (aml_strstr(infobuf, "AC-3")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_AC3");
			}
			if (aml_strstr(infobuf, "DTS-HD")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DTS|AUDIO_FORMAT_DTSHD");
			} else if (aml_strstr(infobuf, "DTS")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DTS");
			}
			if (aml_strstr(infobuf, "MAT")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DOLBY_TRUEHD");
			}
		}
		/*check the channel cap */
		else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
			/* take the 2ch suppported as default */
			size += sprintf(aud_cap, "=%s|", "AUDIO_CHANNEL_OUT_STEREO");
			if (aml_strstr(infobuf, "PCM, 8 ch")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1");
			} else if (aml_strstr(infobuf, "PCM, 6 ch")) {
				size += sprintf(aud_cap + size, "%s|", "AUDIO_CHANNEL_OUT_5POINT1");
			}
		} else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
			/* take the 32/44.1/48 khz suppported as default */
			size += sprintf(aud_cap, "=%s|", "32000|44100|48000");
			if (aml_strstr(infobuf, "88.2")) {
				size += sprintf(aud_cap + size, "%s|", "88200");
			}
			if (aml_strstr(infobuf, "96")) {
				size += sprintf(aud_cap + size, "%s|", "96000");
			}
			if (aml_strstr(infobuf, "176.4")) {
				size += sprintf(aud_cap + size, "%s|", "176400");
			}
			if (aml_strstr(infobuf, "192")) {
				size += sprintf(aud_cap + size, "%s|", "192000");
			}
		}
	}

	if (infobuf) {
		free(infobuf);
	}
	if (fd >= 0) {
		close(fd);
	}

	return aud_cap;

fail:
	if (aud_cap) {
		free(aud_cap);
	}
	if (infobuf) {
		free(infobuf);
	}

	return NULL;
}

char*  aml_get_hdmi_arc_cap(unsigned *ad, int maxsize, const char *keys)
{
	int i = 0;
	int channel = 0;
	int dgraw = 0;
	int fd = -1;
	int size = 0;
	char *aud_cap = NULL;
	unsigned char format, ch, sr;

	aud_cap = (char*)malloc(1024);
	if (aud_cap == NULL) {
		ALOGE("%s: malloc buffer failed, err: %s\n",
			__func__, strerror(errno));
		goto fail;
	}

	memset(aud_cap, 0, 1024);
	ALOGI("%s: get_hdmi_arc_cap\n", __func__);

	/* check the format cap */
	if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
		size += sprintf(aud_cap, "=%s|", "AUDIO_FORMAT_PCM_16_BIT");
	}
	/*check the channel cap */
	else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
		//ALOGI("check channels\n");
		/* take the 2ch suppported as default */
		size += sprintf(aud_cap, "=%s|", "AUDIO_CHANNEL_OUT_STEREO");
	} else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
		/* take the 32/44.1/48 khz suppported as default */
		size += sprintf(aud_cap, "=%s|", "32000|44100|48000");
		//ALOGI("check sample rate\n");
	}

	for (i = 0; i < maxsize; i++) {
		if (ad[i] != 0) {
			format = (ad[i] >> 19) & 0xf;
			ch = (ad[i] >> 16) & 0x7;
			sr = (ad[i] > 8) & 0xf;
			ALOGI("%s: ad %x,format %d,ch %d,sr %d\n", __func__, ad[i], format, ch, sr);
			/* check the format cap */
			if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
				//ALOGI("check format\n");
				if (format == 10) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_E_AC3");
				}
				if (format == 2) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_AC3");
				}
				if (format == 11) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DTS|AUDIO_FORMAT_DTSHD");
				} else if (format == 7) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DTS");
				}
				if (format == 12) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_FORMAT_DOLBY_TRUEHD");
				}
			}
			/*check the channel cap */
			else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_CHANNELS)) {
				//ALOGI("check channels\n");
				if (/*format == 1 && */ch == 7) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_CHANNEL_OUT_5POINT1|AUDIO_CHANNEL_OUT_7POINT1");
				} else if (/*format == 1 && */ch == 5) {
					size += sprintf(aud_cap + size, "%s|", "AUDIO_CHANNEL_OUT_5POINT1");
				}
			} else if (strstr(keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
				ALOGI("%s: check sample rate\n", __func__);
				if (format == 1 && sr == 4) {
					size += sprintf(aud_cap + size, "%s|", "88200");
				}
				if (format == 1 && sr == 5) {
					size += sprintf(aud_cap + size, "%s|", "96000");
				}
				if (format == 1 && sr == 6) {
					size += sprintf(aud_cap + size, "%s|", "176400");
				}
				if (format == 1 && sr == 7) {
					size += sprintf(aud_cap + size, "%s|", "192000");
				}
			}
		} else {
			format = 0;
			ch = 0;
			sr = 0;
		}
	}

	return aud_cap;

fail:
	if (aud_cap) {
		free(aud_cap);
	}

	return NULL;
}

void aml_set_codec_type(int type)
{
    char buf[16];
    int fd = open ("/sys/class/audiodsp/digital_codec", O_WRONLY);

    if (fd >= 0) {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "%d", type);

        write(fd, buf, sizeof(buf));
        close(fd);
    }
}


int aml_get_codec_type(int format)
{
    switch (format) {
        case AUDIO_FORMAT_AC3:
            return TYPE_AC3;
        case AUDIO_FORMAT_E_AC3:
            return TYPE_EAC3;
        case AUDIO_FORMAT_DTS:
            return TYPE_DTS;
        case AUDIO_FORMAT_DTS_HD:
            return TYPE_DTS_HD;
        case AUDIO_FORMAT_DOLBY_TRUEHD:
            return TYPE_TRUE_HD;
        case AUDIO_FORMAT_PCM:
            return 0;
        default:
            return 0;
    }
}
