/*
 * Copyright (C) 2018 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "AudioALSADeviceParser"

#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "log.h"

#include "alsa_device_parser.h"

#define READ_BUFFER_SIZE     (512)
#define NAME_LEN             (128)

/* patch */
#define ALSASOUND_CARD_PATH  "/proc/asound/cards"
#define ALSASOUND_PCM_PATH   "/proc/asound/pcm"

/* sound card name */
#define CARD_NAME_AUGE       "AMLAUGESOUND"
#define CARD_NAME_MESON      "AMLMESONAUDIO"
#define CARD_NAME_M8         "AMLM8AUDIO"
#define CARD_NAME_TV         "AMLTVAUDIO"

/* for pars device port */
#define ALSAPORT_PCM         "alsaPORT-pcm" /* usally for bt pcm */
#define ALSAPORT_I2S         "alsaPORT-i2s"
#define ALSAPORT_TDM         "alsaPORT-tdm"
#define ALSAPORT_PDM         "alsaPORT-pdm"
#define ALSAPORT_SPDIF       "alsaPORT-spdif"
#define ALSAPORT_SPDIFB2HDMI "alsaPORT-spdifb2hdmi"
#define ALSAPORT_I2S2HDMI    "alsaPORT-i2s2hdmi" /* virtual link for i2s to hdmi */

struct AudioDeviceDescriptor {
	char name[NAME_LEN];

	int mCardindex;
	int mPcmIndex;

	int mPlayback;
	int mCapture;
};

struct alsa_info {
	int alsa_card_index;
	/* check whether auge sound card */
	int is_auge;
	/* whether auge pcm devices are checked */
	int deviced_checked;

	struct AudioDeviceDescriptor *pcm_descrpt;
	struct AudioDeviceDescriptor *i2s_descrpt;
	struct AudioDeviceDescriptor *tdm_descrpt;
	struct AudioDeviceDescriptor *pdm_descrpt;
	struct AudioDeviceDescriptor *spdif_descrpt;
	struct AudioDeviceDescriptor *spdifb2hdmi_descrpt;
	struct AudioDeviceDescriptor *i2s2hdmi_descrpt;
};

static struct alsa_info *p_aml_alsa_info;

static struct alsa_info *alsa_device_get_info()
{
	return p_aml_alsa_info;
}

bool alsa_device_is_auge(void)
{
	struct alsa_info *p_info = alsa_device_get_info();

	if (p_info && p_info->is_auge)
		return true;

	return false;
}

/*
 * cat /proc/asound/cards
 *	 0 [AMLAUGESOUND   ]: AML-AUGESOUND - AML-AUGESOUND
                  AML-AUGESOUND
 * mCardIndex = 0;
 */
int alsa_device_get_card_index()
{
	struct alsa_info *p_info = alsa_device_get_info();
	FILE *mCardFile = NULL;
	bool isCardIndexFound = false;
	int mCardIndex = -1;

	if (p_info)
		return p_info->alsa_card_index;

	mCardFile = fopen(ALSASOUND_CARD_PATH, "r");
	if (mCardFile) {
		char tempbuffer[READ_BUFFER_SIZE];

		ALOGD("card open success");
		p_aml_alsa_info = calloc(1, sizeof(struct alsa_info));
		if (!p_aml_alsa_info) {
			ALOGE ("NOMEM for alsa info\n");
			return -1;;
		}

		while (!feof(mCardFile)) {
			fgets(tempbuffer, READ_BUFFER_SIZE, mCardFile);

			/* this line contain '[' character */
			if (strchr(tempbuffer, '[')) {
				char *Rch = strtok(tempbuffer, "[");
				mCardIndex = atoi(Rch);
				ALOGD("\tcurrent mCardIndex = %d, Rch = %s", mCardIndex, Rch);
				Rch = strtok(NULL, " ]");
				ALOGD("\tcurrent sound card name = %s", Rch);
				if (strcmp(Rch, CARD_NAME_AUGE) == 0) {
					ALOGD("\t auge sound cardIndex found = %d", mCardIndex);
					isCardIndexFound = true;
					p_aml_alsa_info->is_auge = 1;
					break;
				} else {
					ALOGD("\t meson sound cardIndex found = %d", mCardIndex);
					isCardIndexFound = true;
					p_aml_alsa_info->is_auge = 0;
					break;
				}
			}

			memset((void *)tempbuffer, 0, READ_BUFFER_SIZE);
		}
		ALOGD("reach EOF");
		fclose(mCardFile);
	} else
		ALOGE("Pcm open fail errno %d", errno);

	if (isCardIndexFound == true) {
		p_aml_alsa_info->alsa_card_index = mCardIndex;
	} else
		ALOGE("ParseCardIndex doesn't found card index");

	ALOGD("%s() parse card index:%d\n", __FUNCTION__, mCardIndex);

	return mCardIndex;
}

void alsa_device_parser_pcm_string(struct alsa_info *p_info, char *InputBuffer)
{
	char *Rch;
	char mStreamName[256];
	char *PortName = NULL;

	ALOGD("AddPcmString p_info:%p, InputBuffer = %s\n", p_info, InputBuffer);
	if (!p_info) {
		ALOGE("alsa device info is NULL\n");
		return;
	}

	Rch = strtok(InputBuffer, "-");
	/* parse for stream name */
	if (Rch  != NULL) {
		struct AudioDeviceDescriptor *mAudioDeviceDescriptor =
			calloc(1, sizeof(struct AudioDeviceDescriptor));
		if (!mAudioDeviceDescriptor) {
			ALOGE("%s no memory for device descriptor\n", __FUNCTION__);
			return;
		}

		mAudioDeviceDescriptor->mCardindex = atoi(Rch);
		Rch = strtok(NULL, ":");
		mAudioDeviceDescriptor->mPcmIndex = atoi(Rch);
		Rch = strtok(NULL, ": ");
		if (Rch) {
			memcpy(mStreamName, Rch, 256);
			PortName = strstr(mStreamName, "alsaPORT-");
			if (PortName) {
				memcpy(mAudioDeviceDescriptor->name, PortName, strlen(PortName));

				if (!strcmp(PortName, ALSAPORT_PCM))
					p_info->pcm_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_I2S))
					p_info->i2s_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_TDM))
					p_info->tdm_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_PDM))
					p_info->pdm_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_SPDIF))
					p_info->spdif_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_SPDIFB2HDMI))
					p_info->spdifb2hdmi_descrpt = mAudioDeviceDescriptor;
				else if (!strcmp(PortName, ALSAPORT_I2S2HDMI))
					p_info->i2s2hdmi_descrpt = mAudioDeviceDescriptor;
			} else
				ALOGD("\t Unknown alsaPORT, StreamName:%s\n", mStreamName);
		}
		ALOGD("%s mCardindex:%d, mPcmindex:%d, PortName:%s\n", __FUNCTION__, mAudioDeviceDescriptor->mCardindex, mAudioDeviceDescriptor->mPcmIndex, PortName);
		Rch = strtok(NULL, ": ");
	}
}

/*
 * alsaPORT is kept same with meson sound card
 * cat /proc/asound/pcm
 * 00-00: TDM-A-dummy-alsaPORT-pcm multicodec-0 :  : playback 1 : capture 1
 * 00-01: TDM-B-dummy-alsaPORT-i2s multicodec-1 :  : playback 1 : capture 1
 * 00-02: TDM-C-dummy multicodec-2 :  : playback 1 : capture 1
 * 00-03: PDM-dummy-alsaPORT-pdm dummy-3 :	: capture 1
 * 00-04: SPDIF-dummy-alsaPORT-spdif dummy-4 :	: playback 1 : capture 1
 * 00-05: SPDIF-B-dummy-alsaPORT-hdmi dummy-5 :  : playback 1
 */
int alsa_device_get_pcm_index(int alsaPORT)
{
	struct alsa_info *p_info = alsa_device_get_info();
	int new_port = -1;

	if (!p_info || !p_info->is_auge) {
		/* fix to spdif port */
		if (alsaPORT >= 2)
			alsaPORT = 1;

		return alsaPORT;
	}

	if (!p_info->deviced_checked) {
		FILE *mPcmFile = NULL;
		char tempbuffer[READ_BUFFER_SIZE];

		mPcmFile = fopen(ALSASOUND_PCM_PATH, "r");
		if (mPcmFile) {
			ALOGD("Pcm open success");
			while (!feof(mPcmFile)) {
				fgets(tempbuffer, READ_BUFFER_SIZE, mPcmFile);
				alsa_device_parser_pcm_string(p_info, tempbuffer);
				memset((void *)tempbuffer, 0, READ_BUFFER_SIZE);
			}
			ALOGD("reach EOF");
			fclose(mPcmFile);
			p_info->deviced_checked = 1;
		} else
			ALOGD("Pcm open fail");
	}
	switch (alsaPORT) {
	case PORT_I2S:
		new_port = p_info->i2s_descrpt->mPcmIndex;
		break;
	case PORT_SPDIF:
		new_port = p_info->spdif_descrpt->mPcmIndex;
		break;
	case PORT_PCM:
		new_port = p_info->pcm_descrpt->mPcmIndex;
		break;
	case PROT_TDM:
		new_port = p_info->tdm_descrpt->mPcmIndex;
		break;
	case PROT_PDM:
		new_port = p_info->pdm_descrpt->mPcmIndex;
		break;
	case PORT_SPDIFB2HDMI:
		new_port = p_info->spdifb2hdmi_descrpt->mPcmIndex;
		break;
	case PORT_I2S2HDMI:
		new_port = p_info->i2s2hdmi_descrpt->mPcmIndex;
		break;
	default:
		new_port = p_info->i2s_descrpt->mPcmIndex;
		ALOGD("Default port is I2s\n");
		break;
	}
	ALOGD("auge sound card, fix alsaPORT:%d to :%d\n", alsaPORT, new_port);

	return new_port;
}
