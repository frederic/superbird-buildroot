/**
 * \file adec-internal-mgt.c
 * \brief  Audio Dec Message Loop Thread
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <Amsysfsutils.h>
#include <Amavutils.h>

#define adec_print printf

typedef struct {
//	int no;
	int audio_id;
	char type[16];
}audio_type_t;

audio_type_t audio_type[] = {
    {ACODEC_FMT_AAC, "aac"},
    {ACODEC_FMT_AC3, "ac3"},
    {ACODEC_FMT_DTS, "dts"},
    {ACODEC_FMT_FLAC, "flac"},
    {ACODEC_FMT_COOK, "cook"},
    {ACODEC_FMT_AMR, "amr"},
    {ACODEC_FMT_RAAC, "raac"},
    {ACODEC_FMT_WMA, "wma"},
    {ACODEC_FMT_WMAPRO, "wmapro"},
    {ACODEC_FMT_ALAC, "alac"},
    {ACODEC_FMT_VORBIS, "vorbis"},
    {ACODEC_FMT_AAC_LATM, "aac_latm"},
    {ACODEC_FMT_APE, "ape"},
    {ACODEC_FMT_MPEG, "mp3"},
    {ACODEC_FMT_EAC3,"eac3"},

    {ACODEC_FMT_PCM_S16BE,"pcm"},
    {ACODEC_FMT_PCM_S16LE,"pcm"},
    {ACODEC_FMT_PCM_U8,"pcm"},
    {ACODEC_FMT_PCM_BLURAY,"pcm"},
    {ACODEC_FMT_WIFIDISPLAY,"pcm"},
    {ACODEC_FMT_ALAW,"pcm"},
    {ACODEC_FMT_MULAW,"pcm"},

    {ACODEC_FMT_ADPCM,"adpcm"},
    {ACODEC_FMT_NULL, "null"},

};

static int audio_decoder = AUDIO_ARM_DECODER;

static int reset_track_enable=0;
void adec_reset_track_enable(int enable_flag)
{
    reset_track_enable=enable_flag;
    adec_print("reset_track_enable=%d\n", reset_track_enable);
}

void adec_reset_track(aml_audio_dec_t *audec)
{
    if(audec->format_changed_flag && audec->state >= INITTED && !audec->need_stop)
    {
        buffer_stream_t *g_bst=audec->g_bst;
        adec_print("reset audio_track: samplerate=%d channels=%d\n", (g_bst==NULL)?audec->samplerate:g_bst->samplerate,(g_bst==NULL)?audec->channels:g_bst->channels);

        audio_out_operations_t *out_ops = &audec->aout_ops;
		out_ops->mute(audec, 1);
		out_ops->pause(audec);

        out_ops->stop(audec);

        if(g_bst != NULL)
        {
          //4.4 code maybe run on 4.2 hardware platform: g_bst==NULL on 4.2,so add this condition
          audec->channels  =g_bst->channels;
          audec->samplerate=g_bst->samplerate;
        }

        out_ops->init(audec);

        if(audec->state == ACTIVE)
        {
            out_ops->start(audec);
            out_ops->resume(audec);
        }

        audec->format_changed_flag=0;
    }
}

/**
 * \brief start audio dec
 * \param audec pointer to audec
 * \return 0 on success otherwise -1 if an error occurred
 */
int match_types(const char *filetypestr,const char *typesetting)
{
	const char * psets=typesetting;
	const char *psetend;
	int psetlen=0;
	char typestr[64]="";
	if(filetypestr == NULL || typesetting == NULL)
		return 0;

	while(psets && psets[0]!='\0'){
		psetlen=0;
		psetend=strchr(psets,',');
		if(psetend!=NULL && psetend>psets && psetend-psets<64){
			psetlen=psetend-psets;
			memcpy(typestr,psets,psetlen);
			typestr[psetlen]='\0';
			psets=&psetend[1];//skip ";"
		}else{
			strcpy(typestr,psets);
			psets=NULL;
		}
		if(strlen(typestr)>0&&(strlen(typestr)==strlen(filetypestr))){
			if(strstr(filetypestr,typestr)!=NULL)
				return 1;
		}
	}
	return 0;
}

static int set_linux_audio_decoder(aml_audio_dec_t *audec)
{
    int audio_id;
    int i;
    int num;
    int ret;
    audio_type_t *t;
    char *value;
    audio_id = audec->format;

    num = ARRAY_SIZE(audio_type);
    for (i = 0; i < num; i++) {
        t = &audio_type[i];
        if (t->audio_id == audio_id) {
            break;
        }
    }
    value = getenv("media_arm_audio_decoder");
    adec_print("media.armdecode.audiocodec = %s, t->type = %s\n", value, t->type);

    audio_decoder = AUDIO_ARM_DECODER;
    return 0;
}

int get_audio_decoder(void)
{
	//adec_print("audio_decoder = %d\n", audio_decoder);
	return audio_decoder;
}

int vdec_pts_pause(void)
{
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x1");
}
int audiodec_init(aml_audio_dec_t *audec)
{
    int ret = 0;
    int res = 0;
    pthread_t    tid;
    char value[PROPERTY_VALUE_MAX]={0};
    adec_print("audiodec_init!\n");
    adec_message_pool_init(audec);
    get_output_func(audec);
    int nCodecType=audec->format;
    set_linux_audio_decoder(audec);
    audec->format_changed_flag=0;

    int codec_type=get_audio_decoder();
    res=RegisterDecode(audec,codec_type);
    if(!res){
        ret = pthread_create(&tid, NULL, (void *)adec_armdec_loop, (void *)audec);
    }
    if (ret != 0) {
        adec_print("Create adec main thread failed!\n");
        return ret;
    }
    adec_print("Create adec main thread success! tid = %d\n", tid);
    audec->thread_pid = tid;
    return ret;
}
