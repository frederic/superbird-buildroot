/*
 * Copyright (C) 2010 Amlogic Corporation.
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



#define LOG_TAG "audio_hw_utils"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <utils/Timers.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <linux/ioctl.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

#include "audio_hw_utils.h"

#include "audio_hwsync.h"
#include "audio_hw.h"
#include "amlAudioMixer.h"
#include <audio_utils/primitives.h>

#ifdef LOG_NDEBUG_FUNCTION
#define LOGFUNC(...) ((void)0)
#else
#define LOGFUNC(...) (ALOGD(__VA_ARGS__))
#endif

int64_t aml_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)(tv.tv_sec) * 1000000 + (int64_t)(tv.tv_usec));
}
int get_sysfs_uint(const char *path, uint *value)
{
    int fd;
    char valstr[64];
    uint val = 0;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr, 0, 64);
        read(fd, valstr, 64 - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        ALOGE("unable to open file %s\n", path);
        return -1;
    }
    if (sscanf(valstr, "0x%x", &val) < 1) {
        ALOGE("unable to get pts from: %s", valstr);
        return -1;
    }
    *value = val;
    return 0;
}

int sysfs_set_sysfs_str(const char *path, const char *val)
{
    int fd;
    int bytes;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        bytes = write(fd, val, strlen(val));
        close(fd);
        return 0;
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

int  sysfs_get_sysfs_str(const char *path, char *valstr, int size)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr,0,size);
        read(fd, valstr, size - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
        sprintf(valstr, "%s", "fail");
        return -1;
    };
    //LOGI("get_sysfs_str=%s\n", valstr);
    return 0;
}

int get_sysfs_int(const char *path)
{
    int val = 0;
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        char bcmd[16];
        read(fd, bcmd, sizeof(bcmd));
        val = strtol(bcmd, NULL, 10);
        close(fd);
    } else {
        ALOGD("[%s]open %s node failed! return 0\n", path, __FUNCTION__);
    }
    return val;
}
int mystrstr(char *mystr, char *substr)
{
    int i = 0;
    int j = 0;
    int score = 0;
    int substrlen = strlen(substr);
    int ok = 0;
    for (i = 0; i < 1024 - substrlen; i++) {
        for (j = 0; j < substrlen; j++) {
            score += (substr[j] == mystr[i + j]) ? 1 : 0;
        }
        if (score == substrlen) {
            ok = 1;
            break;
        }
        score = 0;
    }
    return ok;
}
void set_codec_type(int type)
{
    char buf[16];
    int fd = open("/sys/class/audiodsp/digital_codec", O_WRONLY);

    if (fd >= 0) {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "%d", type);

        write(fd, buf, sizeof(buf));
        close(fd);
    }
}
unsigned char codec_type_is_raw_data(int type)
{
    switch (type) {
    case TYPE_AC3:
    case TYPE_EAC3:
    case TYPE_TRUE_HD:
    case TYPE_DTS:
    case TYPE_DTS_HD:
    case TYPE_DTS_HD_MA:
        return 1;
    default:
        return 0;
    }
}

int get_codec_type(int format)
{
    switch (format) {
    case AUDIO_FORMAT_AC3:
        return TYPE_AC3;
    case AUDIO_FORMAT_E_AC3:
        return TYPE_EAC3;
    case AUDIO_FORMAT_DTS:
        return TYPE_DTS;
    case AUDIO_FORMAT_DTS_HD:
        return TYPE_DTS_HD_MA;
    case AUDIO_FORMAT_DOLBY_TRUEHD:
    case AUDIO_FORMAT_MAT:
        return TYPE_TRUE_HD;
    case AUDIO_FORMAT_AC4:
        return TYPE_AC4;
    case AUDIO_FORMAT_PCM:
        return TYPE_PCM;
    default:
        return TYPE_PCM;
    }
}
int getprop_bool(const char *path)
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;

    ret = property_get(path, buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "true") == 0 || strcmp(buf, "1") == 0) {
            return 1;
        }
    }
    return 0;
}

int is_txlx_chip()
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;

    ret = property_get("ro.board.platform", buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "txlx") == 0) {
            return true;
        }
    }
    return false;
}

int is_txl_chip()
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;

    ret = property_get("ro.board.platform", buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "txl") == 0) {
            return true;
        }
    }
    return false;
}


/*
convert audio formats to supported audio format
8 ch goes to 32 bit
2 ch can be 16 bit or 32 bit
@return input buffer used by alsa drivers to do the data write
*/
void *convert_audio_sample_for_output(int input_frames, int input_format, int input_ch, void *input_buf, int *out_size)
{
    float lvol = 1.0;
    int *out_buf = NULL;
    short *p16 = (short*)input_buf;
    int *p32 = (int*)input_buf;
    int max_ch =  input_ch;
    int i;
    //ALOGV("intput frame %d,input ch %d,buf ptr %p,vol %f\n", input_frames, input_ch, input_buf, lvol);
    ALOG_ASSERT(input_buf);
    if (input_ch > 2) {
        max_ch = 8;
    }
    //our HW need round the frames to 8 channels
    out_buf = malloc(sizeof(int) * max_ch * input_frames);
    if (out_buf == NULL) {
        ALOGE("malloc buffer failed\n");
        return NULL;
    }
    switch (input_format) {
    case AUDIO_FORMAT_PCM_16_BIT:
        break;
    case AUDIO_FORMAT_PCM_32_BIT:
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        for (i = 0; i < input_frames * input_ch; i++) {
            p32[i] = p32[i] << 8;
        }
        break;
    case AUDIO_FORMAT_PCM_FLOAT:
        memcpy_to_i16_from_float((short*)out_buf, input_buf, input_frames * input_ch);
        memcpy(input_buf, out_buf, sizeof(short)*input_frames * input_ch);
        break;
    }
    //current all the data are in the input buffer
    if (input_ch == 8) {
        short *p16_temp;
        int i, NumSamps;
        int *p32_temp = out_buf;
        float m_vol = lvol;
        NumSamps = input_frames * input_ch;
        //here to swap the channnl data here
        //actual now:L,missing,R,RS,RRS,,LS,LRS,missing
        //expect L,C,R,RS,RRS,LRS,LS,LFE (LFE comes from to center)
        //actual  audio data layout  L,R,C,none/LFE,LRS,RRS,LS,RS
        if (input_format == AUDIO_FORMAT_PCM_16_BIT) {
            p16_temp = (short*)out_buf;
            for (i = 0; i < NumSamps; i = i + 8) {
                p16_temp[0 + i]/*L*/ = m_vol * p16[0 + i];
                p16_temp[1 + i]/*R*/ = m_vol * p16[1 + i];
                p16_temp[2 + i] /*LFE*/ = m_vol * p16[3 + i];
                p16_temp[3 + i] /*C*/ = m_vol * p16[2 + i];
                p16_temp[4 + i] /*LS*/ = m_vol * p16[6 + i];
                p16_temp[5 + i] /*RS*/ = m_vol * p16[7 + i];
                p16_temp[6 + i] /*LRS*/ = m_vol * p16[4 + i];
                p16_temp[7 + i]/*RRS*/ = m_vol * p16[5 + i];
            }
            memcpy(p16, p16_temp, NumSamps * sizeof(short));
            for (i = 0; i < NumSamps; i++) { //suppose 16bit/8ch PCM
                p32_temp[i] = p16[i] << 16;
            }
        } else {
            p32_temp = out_buf;
            for (i = 0; i < NumSamps; i = i + 8) {
                p32_temp[0 + i]/*L*/ = m_vol * p32[0 + i];
                p32_temp[1 + i]/*R*/ = m_vol * p32[1 + i];
                p32_temp[2 + i] /*LFE*/ = m_vol * p32[3 + i];
                p32_temp[3 + i] /*C*/ = m_vol * p32[2 + i];
                p32_temp[4 + i] /*LS*/ = m_vol * p32[6 + i];
                p32_temp[5 + i] /*RS*/ = m_vol * p32[7 + i];
                p32_temp[6 + i] /*LRS*/ = m_vol * p32[4 + i];
                p32_temp[7 + i]/*RRS*/ = m_vol * p32[5 + i];
            }

        }
        *out_size = NumSamps * sizeof(int);

    } else if (input_ch == 6) {
        int j, NumSamps, real_samples;
        short *p16_temp;
        int *p32_temp = out_buf;
        float m_vol = lvol;
        NumSamps = input_frames * input_ch;
        real_samples = NumSamps;
        NumSamps = real_samples * 8 / 6;
        //ALOGI("6ch to 8 ch real %d, to %d\n",real_samples,NumSamps);
        if (input_format == AUDIO_FORMAT_PCM_16_BIT) {
            p16_temp = (short*)out_buf;
            for (i = 0; i < real_samples; i = i + 6) {
                p16_temp[0 + i]/*L*/ = m_vol * p16[0 + i];
                p16_temp[1 + i]/*R*/ = m_vol * p16[1 + i];
                p16_temp[2 + i] /*LFE*/ = m_vol * p16[3 + i];
                p16_temp[3 + i] /*C*/ = m_vol * p16[2 + i];
                p16_temp[4 + i] /*LS*/ = m_vol * p16[4 + i];
                p16_temp[5 + i] /*RS*/ = m_vol * p16[5 + i];
            }
            memcpy(p16, p16_temp, real_samples * sizeof(short));
            memset(p32_temp, 0, NumSamps * sizeof(int));
            for (i = 0, j = 0; j < NumSamps; i = i + 6, j = j + 8) { //suppose 16bit/8ch PCM
                p32_temp[j + 0] = p16[i] << 16;
                p32_temp[j + 1] = p16[i + 1] << 16;
                p32_temp[j + 2] = p16[i + 2] << 16;
                p32_temp[j + 3] = p16[i + 3] << 16;
                p32_temp[j + 4] = p16[i + 4] << 16;
                p32_temp[j + 5] = p16[i + 5] << 16;
            }
        } else {
            p32_temp = out_buf;
            memset(p32_temp, 0, NumSamps * sizeof(int));
            for (i = 0, j = 0; j < NumSamps; i = i + 6, j = j + 8) { //suppose 16bit/8ch PCM
                p32_temp[j + 0] = m_vol * p32[i + 0];
                p32_temp[j + 1] = m_vol * p32[i + 1] ;
                p32_temp[j + 2] = m_vol * p32[i + 2] ;
                p32_temp[j + 3] = m_vol * p32[i + 3] ;
                p32_temp[j + 4] = m_vol * p32[i + 4] ;
                p32_temp[j + 5] = m_vol * p32[i + 5] ;
            }
        }
        *out_size = NumSamps * sizeof(int);
    } else {
        //2ch with 24 bit/32/float audio
        int *p32_temp = out_buf;
        short *p16_temp = (short*)out_buf;
        for (i = 0; i < input_frames; i++) {
            p16_temp[2 * i + 0] =  lvol * p16[2 * i + 0];
            p16_temp[2 * i + 1] =  lvol * p16[2 * i + 1];
        }
        *out_size = sizeof(short) * input_frames * input_ch;
    }
    return out_buf;

}

int aml_audio_start_trigger(void *stream)
{
    struct aml_stream_out *aml_out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = aml_out->dev;
    char tempbuf[128];
    ALOGI("reset alsa to set the audio start\n");
    pcm_stop(aml_out->pcm);
    sprintf(tempbuf, "AUDIO_START:0x%x", adev->first_apts);
    ALOGI("audio start set tsync -> %s", tempbuf);
    sysfs_set_sysfs_str(TSYNC_ENABLE, "1"); // enable avsync
    sysfs_set_sysfs_str(TSYNC_MODE, "1"); // enable avsync
    if (sysfs_set_sysfs_str(TSYNC_EVENT, tempbuf) == -1) {
        ALOGE("set AUDIO_START failed \n");
        return -1;
    }
    return 0;
}

int aml_audio_get_debug_flag()
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;
    int debug_flag = 0;
    ret = property_get("media.audio.hal.debug", buf, NULL);
    if (ret > 0) {
        debug_flag = atoi(buf);
    }
    return debug_flag;
}

int aml_audio_debug_set_optical_format()
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;

    ret = property_get("media.audio.hal.optical", buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "pcm") == 0 || strcmp(buf, "0") == 0) {
            return TYPE_PCM;
        }
        if (strcasecmp(buf, "dd") == 0 || strcmp(buf, "1") == 0) {
            return TYPE_AC3;
        }
        if (strcasecmp(buf, "ddp") == 0 || strcmp(buf, "2") == 0) {
            return TYPE_EAC3;
        }
    }
    return -1;
}

int aml_audio_dump_audio_bitstreams(const char *path, const void *buf, size_t bytes)
{
    if (!path) {
        return 0;
    }

    FILE *fp = fopen(path, "a+");
    if (fp) {
        int flen = fwrite((char *)buf, 1, bytes, fp);
        fclose(fp);
    }

    return 0;
}
int aml_audio_get_arc_latency_offset(int aformat)
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;
    int latency_ms = 0;
    char *prop_name = NULL;
	(void)aformat;
    prop_name = "media.audio.hal.arc_latency.ddp";
    latency_ms = -40;
    ret = property_get(prop_name, buf, NULL);
    if (ret > 0) {
        latency_ms = atoi(buf);
    }
    return latency_ms;
}

int aml_audio_get_hwsync_latency_offset(void)
{
	char buf[PROPERTY_VALUE_MAX];
	int ret = -1;
	int latency_ms = 0;
	char *prop_name = NULL;

	prop_name = "media.audio.hal.hwsync_latency.ddp";
	latency_ms = 0;
	ret = property_get(prop_name, buf, NULL);
	if (ret > 0) {
		latency_ms = atoi(buf);
	}
	return latency_ms;
}

int aml_audio_get_ddp_frame_size()
{
    int frame_size = DDP_FRAME_SIZE;
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;
    char *prop_name = "media.audio.hal.frame_size";
    ret = property_get(prop_name, buf, NULL);
    if (ret > 0) {
        frame_size = atoi(buf);
    }
    return frame_size;
}

bool is_stream_using_mixer(struct aml_stream_out *out)
{
    return is_inport_valid(out->port_index);
}

uint32_t out_get_outport_latency(const struct audio_stream_out *stream)
{
    struct aml_stream_out *out = (struct aml_stream_out *)stream;
    struct aml_audio_device *adev = out->dev;
    struct subMixing *sm = adev->sm;
    struct amlAudioMixer *audio_mixer = sm->mixerData;
    int frames = 0, latency_ms = 0;

    if (is_stream_using_mixer(out)) {
        int outport_latency_frames = mixer_get_outport_latency_frames(audio_mixer);

        if (outport_latency_frames <= 0)
            outport_latency_frames = out->config.period_size * out->config.period_count / 2;

        frames = outport_latency_frames;
        ALOGV("%s(), total frames %d", __func__, frames);
        latency_ms = (frames * 1000) / out->config.rate;
        ALOGV("%s(), latencyMs %d, rate %d", __func__, latency_ms,out->config.rate);
    }
    return latency_ms;
}

static int get_fmt_rate(int codec_type)
{
    int rate = 1;
    if ((codec_type == TYPE_EAC3) ||
        (codec_type == TYPE_DTS_HD_MA) ||
        (codec_type == TYPE_DTS_HD))
        rate = 4;
    else if (codec_type == TYPE_TRUE_HD)
        rate = 16;
    return rate;
}

uint32_t out_get_latency_frames(const struct audio_stream_out *stream)
{
    const struct aml_stream_out *out = (const struct aml_stream_out *)stream;
    snd_pcm_sframes_t frames = 0;
    uint32_t whole_latency_frames;
    int ret = 0;
    //TODO: hal_internal_format may not be the final format to ALSA side (with MS12)
    //should we use sink and optical format?
    int codec_type = get_codec_type(out->hal_internal_format);
    int mul = 1;
    if (out->dual_output_flag) {
        if (out->hal_internal_format == AUDIO_FORMAT_E_AC3)
            mul = 1;
    } else
        mul = get_fmt_rate(codec_type);

    whole_latency_frames = out->config.period_size * out->config.period_count;
    if (!out->pcm || !pcm_is_ready(out->pcm)) {
        return whole_latency_frames / mul;
    }
    ret = pcm_ioctl(out->pcm, SNDRV_PCM_IOCTL_DELAY, &frames);
    if (ret < 0) {
        return whole_latency_frames / mul;
    }
    return frames / mul;
}

int aml_audio_get_spdif_tuning_latency(void)
{
    char *prop_name = "persist.vendor.audio.hal.spdif_ltcy_ms";
    char buf[PROPERTY_VALUE_MAX];
    int latency_ms = 0;
    int ret = -1;

    ret = property_get(prop_name, buf, NULL);
    if (ret > 0) {
        latency_ms = atoi(buf);
    }

    return latency_ms;
}

int aml_audio_get_arc_tuning_latency(audio_format_t arc_fmt)
{
    char *prop_name = NULL;
    char buf[PROPERTY_VALUE_MAX];
    int latency_ms = 0;
    int ret = -1;

    switch (arc_fmt) {
    case AUDIO_FORMAT_PCM_16_BIT:
        prop_name = "persist.audio.arc_ltcy.pcm";
        break;
    case AUDIO_FORMAT_AC3:
        prop_name = "persist.audio.arc_ltcy.dd";
        break;
    case AUDIO_FORMAT_E_AC3:
        prop_name = "persist.audio.arc_ltcy.ddp";
        break;
    case AUDIO_FORMAT_MAT:
        prop_name = "persist.audio.arc_ltcy.mat";
        break;
    default:
        ALOGE("%s(), unsupported audio arc_fmt: %#x", __func__, arc_fmt);
        return 0;
    }

    ret = property_get(prop_name, buf, NULL);
    if (ret > 0) {
        latency_ms = atoi(buf);
    }

    return latency_ms;
}

int aml_audio_get_src_tune_latency(enum patch_src_assortion patch_src) {
    char *prop_name = NULL;
    char buf[PROPERTY_VALUE_MAX];
    int latency_ms = 0;
    int ret = -1;

    switch (patch_src)
    {
    case SRC_HDMIIN:
        prop_name = "persist.audio.tune_ms.hdmiin";
        break;
    case SRC_ATV:
        prop_name = "persist.audio.tune_ms.atv";
        break;
    case SRC_LINEIN:
        prop_name = "persist.audio.tune_ms.linein";
        break;
    default:
        ALOGE("%s(), unsupported audio patch source: %d", __func__, patch_src);
        return 0;
    }

    ret = property_get(prop_name, buf, NULL);
    if (ret > 0)
    {
        latency_ms = atoi(buf);
    }

    return latency_ms;
}

void audio_fade_func(void *buf,int fade_size,int is_fadein) {
    float fade_vol = is_fadein ? 0.0 : 1.0;
    int i = 0;
    float fade_step = is_fadein ? 1.0/(fade_size/4):-1.0/(fade_size/4);
    int16_t *sample = (int16_t *)buf;
    for (i = 0; i < fade_size/2; i += 2) {
        sample[i] = sample[i]*fade_vol;
        sample[i+1] = sample[i+1]*fade_vol;
        fade_vol += fade_step;
    }
    ALOGI("do fade %s done,size %d",is_fadein?"in":"out",fade_size);

}

void ts_wait_time_us(struct timespec *ts, uint32_t time_us)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += (time_us / 1000000);
    ts->tv_nsec += (time_us * 1000);
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

int cpy_16bit_data_with_gain(int16_t *dst, int16_t *src, int size_in_bytes, float vol)
{
    int size_in_short = size_in_bytes / 2;
    int i = 0;

    if (size_in_bytes % 2) {
        ALOGE("%s(), size inval %d", __func__, size_in_bytes);
        return -EINVAL;
    }

    if (vol > 1.0 || vol < 0) {
        ALOGE("%s(), inval vol %f, should in [0,1]", __func__, vol);
        return -EINVAL;
    }

    for (i = 0; i < size_in_short; i++) {
        dst[i] = src[i] * vol;
    }

    return 0;
}

static inline uint64_t timespec_ns(struct timespec tspec)
{
    return (tspec.tv_sec * 1000000000 + tspec.tv_nsec);
}

uint64_t get_systime_ns(void)
{
    struct timespec tval;

    clock_gettime(CLOCK_MONOTONIC, &tval);

    return timespec_ns(tval);
}

int aml_audio_get_hdmi_latency_offset(int aformat)
{
    char buf[PROPERTY_VALUE_MAX];
    char *prop_name = NULL;
    int ret = -1;
    int latency_ms = 0;

    (void)aformat;
    // PCM latency
    prop_name = "media.audio.hal.hdmi_latency.pcm";
    latency_ms = -52;
    ret = property_get(prop_name, buf, NULL);
    if (ret > 0)
    {
        latency_ms = atoi(buf);
    }

    return latency_ms;
}

// tval_new *must* later than tval_old
uint32_t tspec_diff_to_us(struct timespec tval_old,
        struct timespec tval_new)
{
    return (tval_new.tv_sec - tval_old.tv_sec) * 1000000
            + (tval_new.tv_nsec - tval_old.tv_nsec) / 1000;
}

void aml_audio_switch_output_mode(int16_t *buf, size_t bytes, AM_AOUT_OutputMode_t mode)
{
    int16_t tmp;

    for (unsigned int i= 0; i < bytes / 2; i = i + 2) {
        switch (mode) {
            case AM_AOUT_OUTPUT_DUAL_LEFT:
                buf[i + 1] = buf[i];
                break;
            case AM_AOUT_OUTPUT_DUAL_RIGHT:
                buf[i] = buf[i + 1];
                break;
            case AM_AOUT_OUTPUT_SWAP:
                tmp = buf[i];
                buf[i] = buf[i + 1];
                buf[i + 1] = tmp;
                break;
            case AM_AOUT_OUTPUT_LRMIX:
                tmp = buf[i] / 2 + buf[i + 1] / 2;
                buf[i] = tmp;
                buf[i + 1] = tmp;
                break;
            default :
                break;
        }
    }
}

