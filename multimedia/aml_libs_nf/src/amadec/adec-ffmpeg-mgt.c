#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <stdint.h>
#include <adec-pts-mgt.h>
#include <adec_write.h>
#include <unistd.h>

//#include <adec_omx_brige.h>
#include <Amsysfsutils.h>
#include <audio-dec.h>


#define HEADER_LENGTH_AFTER_IEC61937 0x4
#define adec_print printf


extern int read_buffer(unsigned char *buffer,int size);
void *audio_decode_loop(void *args);
void *audio_dtsdecode_loop(void *args);
void *audio_getpackage_loop(void *args);
static int set_sysfs_int(const char *path, int val);
static void stop_decode_thread(aml_audio_dec_t *audec);

/*audio decoder list structure*/
typedef struct
{
    //enum CodecID codec_id;
    int codec_id;
    char    name[64];
} audio_lib_t;

audio_lib_t audio_lib_list[] =
{
    {ACODEC_FMT_AAC, "libfaad_nfx.so"},
    {ACODEC_FMT_AAC_LATM, "libfaad_nfx.so"},
    {ACODEC_FMT_PCM_S16BE,"libpcm_nfx.so"},
    {ACODEC_FMT_PCM_S16LE,"libpcm_nfx.so"},
    {ACODEC_FMT_PCM_U8,"libpcm_nfx.so"},
    {ACODEC_FMT_PCM_BLURAY,"libpcm_nfx.so"},
    {ACODEC_FMT_WIFIDISPLAY,"libpcm_nfx.so"},
    {ACODEC_FMT_ALAW,"libpcm_nfx.so"},
    {ACODEC_FMT_MULAW,"libpcm_nfx.so"},
    {ACODEC_FMT_ADPCM,"libadpcm_nfx.so"},
    {ACODEC_FMT_AC3,"libdcv.so"},
    {ACODEC_FMT_EAC3,"libdcv.so"}
} ;

int find_audio_lib(aml_audio_dec_t *audec)
{
    int i;
    int num;
    audio_lib_t *f;
    void *fd = NULL;
    adec_print("[%s %d]audec->format/%d audec->codec_id/0x%x\n",__FUNCTION__,__LINE__,audec->format,audec->codec_id);
    num = ARRAY_SIZE(audio_lib_list);
    audio_decoder_operations_t *adec_ops=audec->adec_ops;
    //-------------------------
    //if(find_omx_lib(audec))
    //    return 0;
    //-----------------------
    for (i = 0; i < num; i++) {
        f = &audio_lib_list[i];
        adec_print("[%s::%d]--[f->codec_id:%d]\n",__FUNCTION__, __LINE__, f->codec_id);

        if (f->codec_id == audec->format)
        {
            fd = dlopen(audio_lib_list[i].name,RTLD_NOW);
            if (fd != 0)
            {
                adec_ops->init    = dlsym(fd, "audio_dec_init");
                adec_ops->decode  = dlsym(fd, "audio_dec_decode");
                adec_ops->release = dlsym(fd, "audio_dec_release");
                adec_ops->getinfo = dlsym(fd, "audio_dec_getinfo");
                adec_print("[%s::%d]--[audio_lib_list[i].name:%s] found it!\n",__FUNCTION__, __LINE__, audio_lib_list[i].name);
            }else{
                char const *err_str = dlerror();
                adec_print("[%s::%d]--[module=%s\n%s",__FUNCTION__, audio_lib_list[i].name, err_str ? err_str : "unknown");
                adec_print("the so appear bad file, can not find decoder lib!!\n");
                return -1;
            }
            return 0;
        }
        else {
            adec_print("[%s::%d]--[f->codec_id:%d]--not~fit-[audio_lib_list[i].name:%s]\n",
                __FUNCTION__, __LINE__, f->codec_id, audio_lib_list[i].name);
        }
    }
    return -1;
}


audio_decoder_operations_t AudioArmDecoder=
{
    "FFmpegDecoder",
    AUDIO_ARM_DECODER,
    0,
};
static int FFmpegDecoderInit(audio_decoder_operations_t *adec_ops)
{
    return 0;
}
static int FFmpegDecode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen){
    int ret;
    return ret;
}
static int FFmpegDecoderRelease(audio_decoder_operations_t *adec_ops)
{
    aml_audio_dec_t *audec=(aml_audio_dec_t *)(adec_ops->priv_data);
    return 0;
}
audio_decoder_operations_t AudioFFmpegDecoder=
{
    .name="FFmpegDecoder",
    .nAudioDecoderType=AUDIO_FFMPEG_DECODER,
    .init=FFmpegDecoderInit,
    .decode=FFmpegDecode,
    .release=FFmpegDecoderRelease,
    .getinfo=NULL,
};

int package_list_free(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->pack_list.tslock));
    while(audec->pack_list.pack_num)
    {
        struct package * p=audec->pack_list.first;
        audec->pack_list.first=audec->pack_list.first->next;
        free(p->data);
        free(p);
        audec->pack_list.pack_num--;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return 0;
}

int package_list_init(aml_audio_dec_t * audec)
{
    audec->pack_list.first=NULL;
    audec->pack_list.pack_num=0;
    audec->pack_list.current=NULL;
    lp_lock_init(&(audec->pack_list.tslock),NULL);
    return 0;
}

int package_add(aml_audio_dec_t * audec,char * data,int size)
{
    lp_lock(&(audec->pack_list.tslock));
    if(audec->pack_list.pack_num == 4) //enough
    {
        lp_unlock(&(audec->pack_list.tslock));
        return -2;
    }
    struct package *p=malloc(sizeof(struct package));
    if(!p) //malloc failed
    {
        lp_unlock(&(audec->pack_list.tslock));
        return -1;
    }
    p->data=data;
    p->size=size;
    if(audec->pack_list.pack_num == 0)//first package
    {
        audec->pack_list.first=p;
        audec->pack_list.current=p;
        audec->pack_list.pack_num=1;
    }
    else
    {
        audec->pack_list.current->next=p;
        audec->pack_list.current=p;
        audec->pack_list.pack_num++;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return 0;
}

struct package * package_get(aml_audio_dec_t * audec)
{
    lp_lock(&(audec->pack_list.tslock));
    if(audec->pack_list.pack_num==0)
    {
        lp_unlock(&(audec->pack_list.tslock));
        return NULL;
    }
    struct package *p=audec->pack_list.first;
    if(audec->pack_list.pack_num == 1)
    {
        audec->pack_list.first=NULL;
        audec->pack_list.pack_num=0;
        audec->pack_list.current=NULL;
    }else if(audec->pack_list.pack_num>1) {
        audec->pack_list.first=audec->pack_list.first->next;
        audec->pack_list.pack_num--;
    }
    lp_unlock(&(audec->pack_list.tslock));
    return p;
}

static inline float EaseNext(int method, float t, float b, float c, float d)
{
    switch (method) {
    default:
    case 0:
        return c*t/d + b;
    case 1:
        t/=d;
        return c*t*t*t + b;
    case 2:
        t=t/d-1;
        return c*(t*t*t + 1) + b;
    }
}

static void armdec_stream_ease(aml_audio_dec_t *audec, char *buf, int size)
{
    int i;
    float delta;
    short *p = (short *)buf;
    float delta_vol;
    unsigned int delta_dur;

    if (audec->volume_ease_update) {
        audec->volume_ease_start = audec->volume_ease_cur;
        if (audec->volume_ease_duration_staging == 0) {
            audec->volume_ease_cur = audec->volume_ease_end_staging;
        }
        audec->volume_ease_end = audec->volume_ease_end_staging;
        audec->volume_ease_method = audec->volume_ease_method_staging;
        audec->volume_ease_duration = audec->volume_ease_duration_staging;
        audec->volume_ease_sample = 0;
        audec->volume_ease_update = 0;
#if 0
        adec_print("armdec_stream_ease : %f, %f->%f, dur %d, method %d, delta_dur = %d, sr=%d",
                    audec->volume_ease_start, audec->volume_ease_cur, audec->volume_ease_end,
                    audec->volume_ease_duration, audec->volume_ease_method,
                    audec->volume_ease_duration * audec->samplerate / 1000, audec->samplerate);
#endif
    }

    delta_vol = audec->volume_ease_end - audec->volume_ease_start;
    delta_dur = audec->volume_ease_duration * audec->samplerate / 1000;

    if (audec->volume_ease_sample >= delta_dur) {
        if (audec->volume_ease_cur != 1.0) {
            float c = audec->volume_ease_cur;
            for (i=0; i<size/2; i++, p++) *p *= c;
        }
        return;
    }

    // ease volume
    while ((char *)p < (buf+size)) {
        if (audec->volume_ease_sample < delta_dur) {
            float s = audec->volume_ease_cur;
            audec->volume_ease_cur = EaseNext(audec->volume_ease_method,
                                              (float)audec->volume_ease_sample,
                                              audec->volume_ease_start,
                                              delta_vol,
                                              (float)delta_dur);
#if 0
            if (floor(s * 10) != floor(audec->volume_ease_cur * 10)) {
                adec_print("easing: volume_ease_cur = %f\n", audec->volume_ease_cur);
            }
#endif
            audec->volume_ease_sample++;
#if 0
            if (audec->volume_ease_sample == delta_dur) {
                adec_print("ease done, volume_ease_cur = %f, volume_ease_sample=%d!\n", audec->volume_ease_cur, audec->volume_ease_sample);
            }
#endif
        } else {
            audec->volume_ease_cur = audec->volume_ease_end;
        }

        *p *= audec->volume_ease_cur;
        p++;
        if (audec->channels > 1) {
            *p *= audec->volume_ease_cur;
            p++;
        }
    }
}

int armdec_stream_read(dsp_operations_t *dsp_ops, char *buffer, int size)
{
     int read_size=0;
     aml_audio_dec_t *audec=(aml_audio_dec_t *)dsp_ops->audec;
     read_size=read_pcm_buffer(buffer,audec->g_bst, size);
     if (read_size > 0)
         armdec_stream_ease(audec, buffer, read_size);
     audec->out_len_after_last_valid_pts+=read_size;
     return read_size;
}

int armdec_stream_read_raw(dsp_operations_t *dsp_ops, char *buffer, int size)
{
     int read_size=0;
     aml_audio_dec_t *audec=(aml_audio_dec_t *)dsp_ops->audec;
     read_size = read_pcm_buffer(buffer,audec->g_bst_raw, size);
     if ((read_size > 0) && (audec->volume_ease_cur == 0.0)) {
         memset(buffer, 0, read_size);
     }
     return read_size;
}

unsigned long  armdec_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long val,offset;
    unsigned long pts, pts_from_sample;
    int data_width,channels,samplerate;
    unsigned long long frame_nums ;
    unsigned long delay_pts;
    aml_audio_dec_t *audec=(aml_audio_dec_t *)dsp_ops->audec;
    switch(audec->g_bst->data_width)
    {
        case AV_SAMPLE_FMT_U8:
            data_width=8;
            break;
        case AV_SAMPLE_FMT_S16:
            data_width=16;
            break;
        case AV_SAMPLE_FMT_S32:
            data_width=32;
            break;
        default:
            data_width=16;
    }
    channels=audec->g_bst->channels;
    samplerate=audec->g_bst->samplerate;
    if(!channels || !samplerate){
		adec_print("warning ::::zero  channels %d, sample rate %d \n",channels,samplerate);
		if(!samplerate)
			samplerate = 48000;
		if(!channels)
			channels = 2;
    }
    offset=audec->decode_offset;

    if(dsp_ops->amstream_fd>=0){
        if(audec->g_bst->format != ACODEC_FMT_COOK && audec->g_bst->format != ACODEC_FMT_RAAC) {
            //when first  look up apts,set offset 0
            if(!audec->first_apts_lookup_over){
                offset = 0;
            }
            ioctl(dsp_ops->amstream_fd,AMSTREAM_IOC_APTS_LOOKUP,&offset);
            //adec_print("lookup %d --> pts: %d", audec->decode_offset, offset/90);
        }
    }else{
        adec_print("====abuf have not open!\n",val);
    }
    if(am_getconfig_bool("media.arm.audio.apts_add"))
       offset=0;

    pts=offset;
    if(!audec->first_apts_lookup_over)
    {
        audec->last_valid_pts = pts;
        audec->first_apts_lookup_over = 1;
        return pts;
    }

    pts_from_sample = pts;

    //if(pts==0){
        if (audec->last_valid_pts)
           pts_from_sample = audec->last_valid_pts;
        frame_nums = (audec->out_len_after_last_valid_pts * 8 / (data_width * channels));
        pts_from_sample+= (frame_nums*90000/samplerate);
        //adec_print("decode_offset:%d out_pcm:%d   pts:%d \n",decode_offset,out_len_after_last_valid_pts,pts);
        //return pts;
    //}


    int len = audec->g_bst->buf_level+audec->pcm_cache_size;
    frame_nums = (len * 8 / (data_width * channels));
    delay_pts = (frame_nums*90000/samplerate);
    if (pts > delay_pts) {
        pts -= delay_pts;
    } else {
        pts = 0;
    }
    val=pts;

    // pts_from_sample: pts calculated from last looked up pts + sample number accumulation (no gap)
    // val: pts from lookup directly
    // check netflix audio gap here, between 500-2000ms, then audio gap exist
    //adec_print("pts_from_sample = %d, pts_from_lookup=%d, delay_pts = %d",
    //            pts_from_sample/90, val/90, delay_pts/90);
    if ((audec->gap_end_pts == 0) &&
        ((int)(val - pts_from_sample) > 500 * 90) &&
        ((int)(val - pts_from_sample) < 2000 * 90)) {
        audec->gap_end_pts = pts_from_sample + delay_pts;
        adec_print("gap_end_pts = %d, pts_from_sample = %d, pts_from_lookup=%d, delay_pts = %d",
                    audec->gap_end_pts/90, pts_from_sample/90, val/90, delay_pts/90);
    }

    if ((audec->gap_end_pts) &&
        ((int)(pts_from_sample - audec->gap_end_pts) < 0)) {
        adec_print("use pts_from_sample %d during gap", pts_from_sample/90);
        return pts_from_sample;
    }

    audec->gap_end_pts = 0;

    if (pts==0) {
        return pts_from_sample;
    }

    audec->last_valid_pts=pts;
    audec->out_len_after_last_valid_pts=0;
    //adec_print("[%s::%d]--[pts:%ld]--[offset:%ld]--[frame_num:%lld]--[delay_pts:%ld]\n", __FUNCTION__, __LINE__,pts,offset,frame_nums,delay_pts);
    return val;
}

unsigned long  armdec_get_pcrscr(dsp_operations_t *dsp_ops)
{
    unsigned long val;
    if (dsp_ops->amstream_fd < 0) {
        adec_print("read error!! file descriptor for Audio have not opened\n");
        return -1;
    }
    ioctl(dsp_ops->amstream_fd, AMSTREAM_IOC_PCRSCR, &val);
    return val;
}

int  armdec_set_pts(dsp_operations_t *dsp_ops,unsigned long apts)
{
    if (dsp_ops->amstream_fd < 0) {
        adec_print("armdec_set_apts err!\n");
        return -1;
    }
    ioctl(dsp_ops->amstream_fd, AMSTREAM_IOC_SET_APTS, &apts);
    return 0;
}

static int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}
int get_decoder_status(void *p,struct adec_status *adec)
{
    aml_audio_dec_t *audec=(aml_audio_dec_t *)p;
    if(audec && audec->g_bst)
    {
        adec->channels=audec->g_bst->channels;
        adec->sample_rate=audec->g_bst->samplerate;
        adec->resolution=audec->g_bst->data_width;
        adec->error_count=audec->nDecodeErrCount;//need count
        adec->status=(audec->state> INITTED)?1:0;
        return 0;
    }else{
        return -1;
     }
}

/**
 * \brief register audio decoder
 * \param audec pointer to audec ,codec_type
 * \return 0 on success otherwise -1 if an error occurred
 */
 int RegisterDecode(aml_audio_dec_t *audec,int type)
{
    switch(type)
    {
        case AUDIO_ARM_DECODER:
            memset(&AudioArmDecoder,0,sizeof(audio_decoder_operations_t));
            audec->adec_ops=&AudioArmDecoder;
            if(find_audio_lib(audec)!=0) return -1;
            audec->adec_ops->priv_data=audec;
            break;
        case AUDIO_FFMPEG_DECODER:
            audec->adec_ops=&AudioFFmpegDecoder;
            audec->adec_ops->priv_data=audec;
            break;
        default:
            audec->adec_ops=&AudioFFmpegDecoder;
            audec->adec_ops->priv_data=audec;
            break;
    }
    return 0;
}

static int InBufferInit(aml_audio_dec_t *audec)
{
    int ret = uio_init(audec);
    if (ret < 0){
        adec_print("uio init error! \n");
        return -1;
    }
    return 0;
}

static int InBufferRelease(aml_audio_dec_t *audec)
{
    adec_print("[%s %d]", __func__, __LINE__);
    uio_deinit(audec);
    return 0;
}


static int OutBufferInit(aml_audio_dec_t *audec)
{
    audec->g_bst=malloc(sizeof(buffer_stream_t));
    if(!audec->g_bst){
        adec_print("[%s %d]g_bst malloc failed! \n",__FUNCTION__,__LINE__);
        audec->g_bst=NULL;
        return -1;
    }else{
        adec_print("[%s %d] audec->g_bst/%p",__FUNCTION__,__LINE__,audec->g_bst);
    }

    memset(audec->g_bst,0,sizeof(buffer_stream_t));

    if(audec->adec_ops->nOutBufSize<=0) //set default if not set
        audec->adec_ops->nOutBufSize=DEFAULT_PCM_BUFFER_SIZE;

    int ret=init_buff(audec->g_bst,audec->adec_ops->nOutBufSize);
    if(ret==-1){
        adec_print("[%s %d]pcm buffer init failed !\n",__FUNCTION__,__LINE__);
        return -1;
    }
    adec_print("[%s %d]pcm buffer init ok buf_size:%d\n",__FUNCTION__,__LINE__,audec->g_bst->buf_length);

    audec->g_bst->data_width=audec->data_width=AV_SAMPLE_FMT_S16;

    if(audec->channels>0)
        audec->g_bst->channels=audec->channels;
    else
        audec->g_bst->channels=audec->channels=2;

    if(audec->samplerate>0)
        audec->g_bst->samplerate=audec->samplerate;
    else
        audec->g_bst->samplerate=audec->samplerate=48000;

    audec->g_bst->format = audec->format;

    return 0;
}
static int OutBufferInit_raw(aml_audio_dec_t *audec)
{
    audec->g_bst_raw=malloc(sizeof(buffer_stream_t));
    if(!audec->g_bst_raw)
    {
        adec_print("[%s %d]g_bst_raw malloc failed!\n",__FUNCTION__,__LINE__);
        audec->g_bst_raw=NULL;
        return -1;
    }else{
        adec_print("[%s %d] audec->audec->g_bst_raw/%p\n",__FUNCTION__,__LINE__,audec->g_bst_raw);
    }

    if(audec->adec_ops->nOutBufSize<=0) //set default if not set
           audec->adec_ops->nOutBufSize=DEFAULT_PCM_BUFFER_SIZE;

    int ret=init_buff(audec->g_bst_raw,audec->adec_ops->nOutBufSize);
    if(ret==-1){
        adec_print("[%s %d]raw_buf init failed !\n",__FUNCTION__,__LINE__);
        return -1;
    }
    adec_print("[%s %d]raw buffer init ok buf_size/%d\n",__FUNCTION__,__LINE__,audec->g_bst_raw->buf_length);

    audec->g_bst_raw->data_width=audec->data_width=AV_SAMPLE_FMT_S16;
    if(audec->channels>0)
       audec->g_bst_raw->channels=audec->channels;
    else
       audec->g_bst_raw->channels=audec->channels=2;

    if(audec->samplerate>0)
       audec->g_bst_raw->samplerate=audec->samplerate;
    else
       audec->g_bst_raw->samplerate=audec->samplerate=48000;

    return 0;
}
static int OutBufferRelease(aml_audio_dec_t *audec)
{
    if(audec->g_bst){
        adec_print("[%s %d] audec->g_bst/%p",__FUNCTION__,__LINE__,audec->g_bst);
        release_buffer(audec->g_bst);
        audec->g_bst=NULL;
    }
    return 0;
}

static int OutBufferRelease_raw(aml_audio_dec_t *audec)
{
    if(audec->g_bst_raw){
        adec_print("[%s %d] audec->g_bst_raw/%p",__FUNCTION__,__LINE__,audec->g_bst_raw);
        release_buffer(audec->g_bst_raw);
        audec->g_bst_raw=NULL;
    }
    return 0;
}

static int enable_raw_output(aml_audio_dec_t *audec)
{
	int enable = 0;
	enable = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
	if(enable){
		if(audec->format== ACODEC_FMT_AC3 || audec->format==ACODEC_FMT_EAC3||audec->format==ACODEC_FMT_DTS||
			audec->format == ACODEC_FMT_TRUEHD)
			return 1;
	}
	return 0;
}
static int audio_codec_init(aml_audio_dec_t *audec)
{
      //reset static&global
      audec->exit_decode_thread=0;
      audec->exit_decode_thread_success=0;
      audec->decode_offset=0;
      audec->nDecodeErrCount=0;
      audec->g_bst=NULL;
      audec->g_bst_raw=NULL;
      audec->gap_end_pts = 0;
      audec->fd_uio=-1;
      audec->last_valid_pts=0;
      audec->out_len_after_last_valid_pts=0;
      audec->pcm_cache_size=0;
      audec->sn_threadid=-1;
      audec->sn_getpackage_threadid=-1;
      audec->OmxFirstFrameDecoded=0;
      package_list_init(audec);
      while(0!=set_sysfs_int(DECODE_ERR_PATH,DECODE_NONE_ERR))
      {
          adec_print("[%s %d]set codec fatal failed ! \n",__FUNCTION__,__LINE__);
          usleep(100000);
      }
      adec_print("[%s %d]param:data_width:%d samplerate:%d channel:%d \n",
                    __FUNCTION__,__LINE__,audec->data_width,audec->samplerate,audec->channels);

      audec->data_width=AV_SAMPLE_FMT_S16;
      if(audec->channels>0){
          int NumChSave=audec->channels;
          audec->channels=(audec->channels>2? 2:audec->channels);
          audec->adec_ops->channels=audec->channels;
          if(audec->format==ACODEC_FMT_PCM_S16BE  || audec->format==ACODEC_FMT_PCM_S16LE  ||
             audec->format==ACODEC_FMT_PCM_U8     || audec->format==ACODEC_FMT_PCM_BLURAY ||
             audec->format==ACODEC_FMT_WIFIDISPLAY|| audec->format==ACODEC_FMT_ALAW       ||
             audec->format==ACODEC_FMT_MULAW      || audec->format==ACODEC_FMT_ADPCM)
                 audec->adec_ops->channels=NumChSave;
      }

      if(audec->samplerate>0)
          audec->adec_ops->samplerate=audec->samplerate;
      else{
          audec->adec_ops->samplerate=audec->samplerate=48000;
      }
      switch(audec->data_width)
      {
          case AV_SAMPLE_FMT_U8:
              audec->adec_ops->bps=8;
              break;
          case AV_SAMPLE_FMT_S16:
              audec->adec_ops->bps=16;
              break;
          case AV_SAMPLE_FMT_S32:
              audec->adec_ops->bps=32;
              break;
          default:
              audec->adec_ops->bps=16;
      }
      adec_print("[%s %d]param_applied: bps:%d samplerate:%d channel:%d \n",
                   __FUNCTION__,__LINE__,audec->adec_ops->bps,audec->adec_ops->samplerate,audec->adec_ops->channels);

      audec->adec_ops->extradata_size=audec->extradata_size;
      if(audec->extradata_size>0)
          memcpy(audec->adec_ops->extradata,audec->extradata,audec->extradata_size);

      int ret=0;
      if(!audec->StageFrightCodecEnableType)//1-decoder init
          ret=audec->adec_ops->init(audec->adec_ops);
      if(ret==-1){
          adec_print("[%s %d]adec_ops init err\n",__FUNCTION__,__LINE__);
          goto err1;
      }

      ret=OutBufferInit(audec);//2-pcm_buffer init
      if(ret==-1){
          adec_print("[%s %d]out buffer init err\n",__FUNCTION__,__LINE__);
          goto err2;
      }
      if(enable_raw_output(audec)){
	      ret=OutBufferInit_raw(audec);
	      if(ret==-1){
	          adec_print("[%s %d]out_raw buffer init err\n",__FUNCTION__,__LINE__);
	          OutBufferRelease_raw(audec);
	          goto err2;
	      }
      }
      ret=InBufferInit(audec);//3-init uio
      if(ret==-1){
          adec_print("====in buffer  init err \n");
          goto err3;
      }

      //4-other init
      audec->adsp_ops.dsp_on = 1;
      audec->adsp_ops.dsp_read = armdec_stream_read;
      audec->adsp_ops.get_cur_pts = armdec_get_pts;
      audec->adsp_ops.get_cur_pcrscr =  armdec_get_pcrscr;
      audec->adsp_ops.set_cur_apts    = armdec_set_pts;
      audec->adsp_ops.dsp_read_raw=armdec_stream_read_raw;
      audec->pcm_bytes_readed=0;
      audec->raw_bytes_readed=0;
      audec->raw_frame_size=0;
      audec->pcm_frame_size=0;
      audec->i2s_iec958_sync_flag=1;
      audec->i2s_iec958_sync_gate=0;
      audec->codec_type=0;
      return 0;

err1:
      audec->adec_ops->release(audec->adec_ops);
      return -1;
err2:
      audec->adec_ops->release(audec->adec_ops);
      OutBufferRelease(audec);
      return -1;
err3:
      audec->adec_ops->release(audec->adec_ops);
      OutBufferRelease(audec);
      InBufferRelease(audec);
      OutBufferRelease_raw(audec);
      return -1;
}
int audio_codec_release(aml_audio_dec_t *audec)
{
    //1-decode thread quit
    if(!audec->StageFrightCodecEnableType){
        stop_decode_thread(audec);//1-decode thread quit
        audec->adec_ops->release(audec->adec_ops);//2-decoder release
    //}else{
    //    stop_decode_thread_omx(audec);
    }

    InBufferRelease(audec);//3-uio uninit
    OutBufferRelease(audec);//4-outbufferrelease
    OutBufferRelease_raw(audec);
    audec->adsp_ops.dsp_on = -1;//5-other release
    audec->adsp_ops.dsp_read = NULL;
    audec->adsp_ops.get_cur_pts = NULL;
    audec->adsp_ops.amstream_fd= -1;

    return 0;
}


static int audio_hardware_ctrl(hw_command_t cmd)
{
    int fd;
    fd = open(AUDIO_CTRL_DEVICE, O_RDONLY);
    if (fd < 0) {
        adec_print("Open Device %s Failed!", AUDIO_CTRL_DEVICE);
        return -1;
    }

    switch (cmd) {
    case HW_CHANNELS_SWAP:
        ioctl(fd, AMAUDIO_IOC_SET_CHANNEL_SWAP, 0);
        break;

    case HW_LEFT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_LEFT_MONO, 0);
        break;

    case HW_RIGHT_CHANNEL_MONO:
        ioctl(fd, AMAUDIO_IOC_SET_RIGHT_MONO, 0);
        break;

    case HW_STEREO_MODE:
        ioctl(fd, AMAUDIO_IOC_SET_STEREO, 0);
        break;

    default:
        adec_print("Unknow Command %d!", cmd);
        break;

    };

    close(fd);
    return 0;
}

static int get_first_apts_flag(dsp_operations_t *dsp_ops)
{
    int val;
    if (dsp_ops->amstream_fd < 0) {
        adec_print("[%s %d]read error!! file descriptor have not opened\n",__FUNCTION__,__LINE__);
        return -1;
    }
    ioctl(dsp_ops->amstream_fd, GET_FIRST_APTS_FLAG, &val);
    return val;
}


/**
 * \brief start audio dec when receive START command.
 * \param audec pointer to audec
 */
static int start_adec(aml_audio_dec_t *audec)
{
    int ret;
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
    unsigned long  vpts,apts;
    int times=0;
    char buf[32];
    apts = vpts = 0;
    audec->no_first_apts = 0;
    audec->apts_start_flag=0;
    audec->first_apts=0;

    if (audec->state == INITTED) {
         //get info from the audiodsp == can get from amstreamer
         while ((!get_first_apts_flag(dsp_ops)) && (!audec->need_stop) && (!audec->no_first_apts))
         {
             adec_print("wait first pts checkin complete !");
             times++;
             if (times>=100)
             {
                 amsysfs_get_sysfs_str(TSYNC_VPTS, buf, sizeof(buf));// read vpts
                 if (sscanf(buf, "0x%lx", &vpts) < 1) {
                    adec_print("unable to get vpts from: %s", buf);
                    return -1;
                 }
                 // save vpts to apts
                 if(vpts == 0) // vpts invalid too
                 {
                     times = 0; // loop again
                     continue;
                 }
                 adec_print("## can't get first apts, save vpts to apts,vpts=%lx, \n",vpts);
                 sprintf(buf, "0x%lx", vpts);
                 amsysfs_set_sysfs_str(TSYNC_APTS, buf);
                 audec->no_first_apts = 1;
             }
             usleep(100000);
         }
         adec_print("wait first pts checkin finish, time waited %d ms! auto_mute = %d\n", times/10, audec->auto_mute);

         /*start  the  the pts scr,...*/
         ret = adec_pts_start(audec);
#if 0
         if (audec->auto_mute) {
             avsync_en(0);
             adec_pts_pause();
             while ((!audec->need_stop) && track_switch_pts(audec)) {
                usleep(1000);
             }
             avsync_en(1);
             adec_pts_resume();
             audec->auto_mute = 0;
         }
         aout_ops->start(audec);
         //audec->state = ACTIVE;
#endif
         audec->state = PAUSED;
    }
    else
    {
        adec_print("amadec status invalid, start adec failed \n");
        return -1;
    }

    return 0;
}

/**
 * \brief pause audio dec when receive PAUSE command.
 * \param audec pointer to audec
 */
static void pause_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (audec->state == ACTIVE) {
        audec->state = PAUSED;
        adec_pts_pause();
        aout_ops->pause(audec);
    }
}

/**
 * \brief resume audio dec when receive RESUME command.
 * \param audec pointer to audec
 */
static void resume_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (audec->state == PAUSED) {
        audec->state = ACTIVE;

        if (audec->auto_mute) {
            avsync_en(0);
            //adec_pts_pause();
            while ((!audec->need_stop) && track_switch_pts(audec)) {
                usleep(1000);
            }
            avsync_en(1);
            adec_pts_resume();
            // set to AMASTER
            amsysfs_set_sysfs_int(TSYNC_MODE, 1);
            audec->auto_mute = 0;
        }

        aout_ops->resume(audec);
        adec_pts_resume();
    }
}

/**
 * \brief stop audio dec when receive STOP command.
 * \param audec pointer to audec
 */
static void stop_adec(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    adec_print("[%s %d]audec->state/%d\n",__FUNCTION__,__LINE__,audec->state);
    if (audec->state > INITING) {
        char buf[64];

        audec->state = STOPPED;
        aout_ops->mute(audec, 1); //mute output, some repeat sound in audioflinger after stop
        aout_ops->stop(audec);
        audio_codec_release(audec);

        sprintf(buf, "0x%lx", 0);
        amsysfs_set_sysfs_str(TSYNC_FIRSTAPTS, buf);
    }
}

/**
 * \brief release audio dec when receive RELEASE command.
 * \param audec pointer to audec
 */
static void release_adec(aml_audio_dec_t *audec)
{
    audec->state = TERMINATED;
}

/**
 * \brief mute audio dec when receive MUTE command.
 * \param audec pointer to audec
 * \param en 1 = mute, 0 = unmute
 */
static void mute_adec(aml_audio_dec_t *audec, int en)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->mute) {
        adec_print("%s the output !\n", (en ? "mute" : "unmute"));
        aout_ops->mute(audec, en);
        audec->muted = en;
    }
}

/**
 * \brief set volume to audio dec when receive SET_VOL command.
 * \param audec pointer to audec
 * \param vol volume value
 */
static void adec_set_volume(aml_audio_dec_t *audec, float vol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_volume) {
        adec_print("set audio volume! vol = %f\n", vol);
        aout_ops->set_volume(audec, vol);
    }
}

/**
 * \brief set volume to audio dec when receive SET_LRVOL command.
 * \param audec pointer to audec
 * \param lvol left channel volume value
 * \param rvol right channel volume value
 */
static void adec_set_lrvolume(aml_audio_dec_t *audec, float lvol,float rvol)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (aout_ops->set_lrvolume) {
        adec_print("set audio volume! left vol = %f,right vol:%f\n", lvol,rvol);
        aout_ops->set_lrvolume(audec, lvol,rvol);
    }
}

/**
 * \brief set volume ease to audio dec when receive SET_VOL_EASE command.
 * \param audec pointer to audec
 * \param vol volume value
 * \param duration duration to reach target volume in millisecond
 * \param method ease method
 */
static void adec_set_volume_ease(aml_audio_dec_t *audec, float vol, unsigned int duration, int method)
{
    adec_print("set audio volume ease1, cur=%f, vol=%f, duration=%d, method=%d\n", audec->volume_ease_cur, vol, duration, method);

    audec->volume_ease_end_staging = vol;
    audec->volume_ease_duration_staging = duration;
    audec->volume_ease_method_staging = method;
    audec->volume_ease_update = 1;

    if ((duration == 0) && (audec->state = PAUSED)) {
        // when in paused status, the ease operation is not running.
        // if an ease command is set after pause, especially for those
        // command with duration 0, those commands are only effective
        // when audio track resume playback. However, if there is a following
        // volume command arrives back to back before the previous volume
        // command is effective then the previous command is lost and not executed.
        // for example, with this sequence:
        // 1. pause
        // 2. set volume to 0.0 with 0 duration
        // 3. then set volume to 0.9 with 10000ms duration
        // the expected result is to get volume 0 first then fade in.
        // however, when in pause mode, the setting of 0.0 volume is lost
        // which caused the easing starts from the current volume before the
        // 0.0 command.
        // the reason that the settings are responded asynchonized is because
        // the setting of easing and the execution of easing are from different
        // threads. To avoid set the volume while audio samples are processing,
        // a staging (volume_ease_update) is used. And this staging status is
        // lost due to such test sequence.
        // ideally a mutex is needed but that is a little more complicated so
        // the decision is just to use the pause as a condition for the duration
        // 0 case and make the command effective from this thread.
        // this is a work around.
        audec->volume_ease_update = 0;
        audec->volume_ease_cur = audec->volume_ease_end_staging;
        audec->volume_ease_end = audec->volume_ease_end_staging;
        audec->volume_ease_duration = audec->volume_ease_duration_staging;
        audec->volume_ease_sample = audec->volume_ease_duration;
    }
}

static void adec_flag_check(aml_audio_dec_t *audec)
{
    audio_out_operations_t *aout_ops = &audec->aout_ops;
    if (audec->auto_mute && (audec->state > INITTED)) {
        aout_ops->pause(audec);
        usleep(10000);
        while ((!audec->need_stop) && track_switch_pts(audec)) {
            usleep(1000);
        }
        aout_ops->resume(audec);
        audec->auto_mute = 0;
    }
}


static void start_decode_thread(aml_audio_dec_t *audec)
{
    if(audec->state != INITTED){
        adec_print("decode not inited quit \n");
        return;
    }

    pthread_t    tid;
    int ret = pthread_create(&tid, NULL, (void *)audio_getpackage_loop, (void *)audec);
    audec->sn_getpackage_threadid=tid;
    adec_print("[%s]Create get package thread success! tid = %d\n",__FUNCTION__,tid);

    ret = pthread_create(&tid, NULL, (void *)audio_decode_loop, (void *)audec);
    if (ret != 0) {
        adec_print("[%s]Create ffmpeg decode thread failed!\n",__FUNCTION__);
        return;
    }
    audec->sn_threadid=tid;
	//pthread_setname_np(tid,"AmadecDecodeLP");
    adec_print("[%s]Create ffmpeg decode thread success! tid = %d\n",__FUNCTION__,tid);
}
static void stop_decode_thread(aml_audio_dec_t *audec)
{
    audec->exit_decode_thread=1;

    int ret = pthread_join(audec->sn_threadid, NULL);
    adec_print("[%s]decode thread exit success\n",__FUNCTION__);
    ret = pthread_join(audec->sn_getpackage_threadid, NULL);
    adec_print("[%s]get package thread exit success\n",__FUNCTION__);

    audec->exit_decode_thread=0;
    audec->sn_threadid=-1;
    audec->sn_getpackage_threadid=-1;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  getNextFrameSize  Get next frame size
*
* @param[in]   format     audio format
*
* @return      -1: no frame_size  use default   0: need get again  non_zero: success
*/
/* --------------------------------------------------------------------------*/

static int get_frame_size(aml_audio_dec_t *audec)
{
    int frame_szie=0;
    int ret=0;
    int extra_data=8;//?
    StartCode *start_code=&audec->start_code;

    if(start_code->status==0||start_code->status==3)
        memset(start_code,0,sizeof(StartCode));
    /*ape case*/
    if(audec->format==ACODEC_FMT_APE){
        if(start_code->status==0)//have not get the sync data
        {
            ret=read_buffer(start_code->buff,4);
            if(ret<=0)
                return 0;
            start_code->size=4;
            start_code->status=1;
        }

        if(start_code->status==1)//start find sync word
        {
            if(start_code->size<4){
                ret=read_buffer(start_code->buff+start_code->size,4-start_code->size);
                if(ret<=0)
                    return 0;
                start_code->size=4;
            }
            if(start_code->size==4){
                if((start_code->buff[0]=='A')&&(start_code->buff[1]=='P')&&(start_code->buff[2]=='T')&&(start_code->buff[3]=='S')){
                    start_code->size=0;
                    start_code->status=2;  //sync word found ,start find frame size
                }else{
                    start_code->size=3;
                    start_code->buff[0] = start_code->buff[1];
                    start_code->buff[1] = start_code->buff[2];
                    start_code->buff[2] = start_code->buff[3];
                    return 0;
                }
            }
        }

        if(start_code->status==2)
        {
            ret=read_buffer(start_code->buff,4);
            if(ret<=0)
                 return 0;
            start_code->size=4;
            frame_szie  = start_code->buff[3]<<24|start_code->buff[2]<<16|start_code->buff[1]<<8|start_code->buff[0]+extra_data;        
            frame_szie  = (frame_szie+3)&(~3);
            start_code->status=3;//found frame size
            return frame_szie;
        }
    }
    return -1;
}
// check if audio format info changed,if changed, apply new parameters to audio track
static void check_audio_info_changed(aml_audio_dec_t *audec)
{
	buffer_stream_t *g_bst = audec->g_bst;
	AudioInfo 	g_AudioInfo = {0};
	int BufLevelAllowDoFmtChg = 0;
	audio_decoder_operations_t *adec_ops  = audec->adec_ops;
	adec_ops->getinfo(audec->adec_ops, &g_AudioInfo);
	if(g_AudioInfo.channels!=0&&g_AudioInfo.samplerate!=0)
	{
		if((g_AudioInfo.channels !=g_bst->channels)||(g_AudioInfo.samplerate!=g_bst->samplerate))
		{
			// the first time we get sample rate/channel num info,we use that to set audio track.
			if(audec->channels == 0 || audec->samplerate == 0){
				g_bst->channels=audec->channels=g_AudioInfo.channels;
				g_bst->samplerate=audec->samplerate=g_AudioInfo.samplerate;
			}
			else{
			    //experienc value:0.2 Secs
				BufLevelAllowDoFmtChg=audec->samplerate*audec->channels*(audec->adec_ops->bps>>3)/5;
				while((audec->format_changed_flag|| g_bst->buf_level>BufLevelAllowDoFmtChg) && !audec->exit_decode_thread ){
					usleep(20000);
				}
				if(!audec->exit_decode_thread){
					adec_print("[%s]Info Changed: src:sample:%d  channel:%d dest sample:%d  channel:%d PCMBufLevel:%d\n",
	                                          __FUNCTION__,audec->samplerate,audec->channels,g_AudioInfo.samplerate,g_AudioInfo.channels,g_bst->buf_level);
				    g_bst->channels=g_AudioInfo.channels;
				    g_bst->samplerate=g_AudioInfo.samplerate;
					audec->aout_ops.pause(audec);
					audec->format_changed_flag = 1;
				}
			}
		}
	}
}
void *audio_getpackage_loop(void *args)
{
     int ret;
    aml_audio_dec_t *audec;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize=0;//next read frame size
    int inlen = 0;//real data size in in_buf
    int nInBufferSize=0;//full buffer size
    char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    int nAudioFormat;

    adec_print("[%s]adec_getpackage_loop start!\n",__FUNCTION__);
    audec = (aml_audio_dec_t *)args;
    adec_ops=audec->adec_ops;
    nAudioFormat=audec->format;
    inlen=0;
    nNextFrameSize=adec_ops->nInBufSize;
    while (1){
exit_decode_loop:
          if(audec->exit_decode_thread)/*detect quit condition*/
          {
              if(inbuf)
                  free(inbuf);
              package_list_free(audec);
              break;
          }

          nNextFrameSize=get_frame_size(audec);/*step 2  get read buffer size*/
          if(nNextFrameSize==-1){
               nNextFrameSize=adec_ops->nInBufSize;
          }else if(nNextFrameSize==0){
               usleep(1000);
               continue;
          }
          nInBufferSize = nNextFrameSize;/*step 3  read buffer*/
          if (inbuf != NULL) {
               free(inbuf);
               inbuf = NULL;
          }
          inbuf = malloc(nInBufferSize);

          int nNextReadSize=nInBufferSize;
          int nRet=0;
          int nReadErrCount=0;
          int nCurrentReadCount=0;
          int nReadSizePerTime=1*1024;
          rlen=0;
          int sleeptime = 0;
          while(nNextReadSize>0 && !audec->exit_decode_thread)
          {
               if(nNextReadSize<=nReadSizePerTime)
                    nReadSizePerTime=nNextReadSize;
               nRet = read_buffer(inbuf+rlen, nReadSizePerTime);//read 10K per time
               if(nRet<=0){
			sleeptime++;
                    continue;
               }
               rlen+=nRet;
               nNextReadSize-=nRet;
          }
          //adec_print(" read data %d,sleep time %d ms    \n",	rlen,sleeptime);
          sleeptime= 0;
          nCurrentReadCount=rlen;
          rlen += inlen;
          ret = -1;
          while((ret=package_add(audec,inbuf,rlen)) && !audec->exit_decode_thread)
          {
              usleep(1000);
          }
	  if(ret){
	      free(inbuf);
	  }
          inbuf=NULL;
      }
QUIT:
      adec_print("[%s]Exit audio_getpackage_loop Thread finished!",__FUNCTION__);
      pthread_exit(NULL);
      return NULL;
}

static unsigned char pcm_buf_tmp[AVCODEC_MAX_AUDIO_FRAME_SIZE];//max frame size out buf
void *audio_decode_loop(void *args)
{
    int ret;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    audio_decoder_operations_t *adec_ops;
    int nNextFrameSize=0;//next read frame size
    int inlen = 0;//real data size in in_buf
    int nRestLen=0;//left data after last decode
    int nInBufferSize=0;//full buffer size
    //int nStartDecodePoint=0;//start decode point in in_buf
    unsigned char *inbuf = NULL;//real buffer
    int rlen = 0;//read buffer ret size
    char *pRestData=NULL;
    char *inbuf2;

    int dlen = 0;//decode size one time
    int declen = 0;//current decoded size
    int nCurrentReadCount=0;
    int needdata = 0;
    char startcode[5];
    int extra_data = 8;
    int nCodecID;
    int nAudioFormat;
    unsigned char *outbuf=pcm_buf_tmp;
    int outlen = 0;
    struct package *p_Package;
    buffer_stream_t *g_bst;
    buffer_stream_t *g_bst_raw;//for ac3&eac3 passthrough
    AudioInfo g_AudioInfo;
    adec_print("[%s]audio_decode_loop start!\n",__FUNCTION__);
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;
    adec_ops=audec->adec_ops;
    memset(outbuf, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    g_bst=audec->g_bst;
    if (dgraw)
    {
        g_bst_raw=audec->g_bst_raw;
        if (g_bst_raw){
            g_bst_raw->format = audec->format;
        }
    }
    nAudioFormat=audec->format;
    g_bst->format = audec->format;
    inlen=0;
    nNextFrameSize = adec_ops->nInBufSize;
    adec_ops->nAudioDecoderType = audec->format;
    while (1){
exit_decode_loop:

          if(audec->exit_decode_thread){//detect quit condition
                if (inbuf){
                   free(inbuf);
                   inbuf = NULL;
                }
                if(pRestData){
                    free(pRestData);
                    pRestData = NULL;
                }
                audec->exit_decode_thread_success=1;
                break;
          }
          //step 2  get read buffer size
          p_Package=package_get(audec);
          if(!p_Package){
               usleep(1000);
               continue;
          }
          if (inbuf != NULL) {
               free(inbuf);
               inbuf = NULL;
          }

          if(inlen&&pRestData){
               rlen=p_Package->size+inlen;
               inbuf=malloc(rlen);
               memcpy(inbuf, pRestData, inlen);
               memcpy(inbuf+inlen,p_Package->data,p_Package->size);
               free(pRestData);
               free(p_Package->data);
               pRestData=NULL;
               p_Package->data=NULL;
          }else{
               rlen=p_Package->size;
               inbuf=p_Package->data;
               p_Package->data=NULL;
          }
          free(p_Package);
          p_Package=NULL;

          nCurrentReadCount=rlen;
          inlen=rlen;
          declen  = 0;
          if (nCurrentReadCount > 0)
          {
               while (declen<rlen && !audec->exit_decode_thread)
               {
                     outlen = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                     if(nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR){
                          if(needdata > 0){
                               pRestData=malloc(inlen);
                               if(pRestData)
                                   memcpy(pRestData, (uint8_t *)(inbuf+declen), inlen);
                               needdata = 0;
                               break;
                          }
                      }

                      dlen = adec_ops->decode(audec->adec_ops, outbuf, &outlen, inbuf+declen, inlen);
                      if (outlen > 0)
                          check_audio_info_changed(audec);
                      if (outlen > AVCODEC_MAX_AUDIO_FRAME_SIZE) {
                               adec_print("!!!!!fatal error,out buffer overwriten,out len %d,actual %d",outlen,AVCODEC_MAX_AUDIO_FRAME_SIZE);
                      }
                      if (dlen <= 0)
                      {

                          if (nAudioFormat==ACODEC_FMT_APE){
                               inlen=0;
                          }else if(inlen>0){
                               pRestData=malloc(inlen);
                               if(pRestData)
                                  memcpy(pRestData, (uint8_t *)(inbuf+declen), inlen);
                          }
                          audec->nDecodeErrCount++;//decode failed, add err_count
                          needdata = 0;
                          break;
                      }
                      audec->nDecodeErrCount=0;//decode success reset to 0
                      declen += dlen;
                      inlen -= dlen;

			/* decoder input buffer not enough,need new data burst */
                      if(nAudioFormat == ACODEC_FMT_COOK || nAudioFormat == ACODEC_FMT_RAAC || nAudioFormat == ACODEC_FMT_AMR){
                          if(inlen <= declen){
                              needdata ++;
                          }
                      }
                      // for aac decoder, if decoder cost es data but no pcm output,may need more data ,which is needed by frame resync
                      else if (nAudioFormat == ACODEC_FMT_AAC_LATM || nAudioFormat == ACODEC_FMT_AAC){
                          if (outlen == 0 && inlen) {
                              pRestData=malloc(inlen);
                              if (pRestData)
                                  memcpy(pRestData, (uint8_t *)(inbuf+declen), inlen);
                              audec->decode_offset+=dlen; //update es offset for apts look up
                              break;
                          }
                      }
                      //write to the pcm buffer
                      if(nAudioFormat == ACODEC_FMT_RAAC ||nAudioFormat == ACODEC_FMT_COOK){
                           audec->decode_offset = audec->adec_ops->pts;
                      }else{
                           audec->decode_offset+=dlen;
                      }

                      if (((AUDIO_SPDIF_PASSTHROUGH == dgraw) || (AUDIO_HDMI_PASSTHROUGH == dgraw)) &&
                         ((ACODEC_FMT_AC3 == nAudioFormat) || (ACODEC_FMT_EAC3 == nAudioFormat) || (ACODEC_FMT_DTS == nAudioFormat))) {
                            int bytesread=0;
                            while (outlen) {
                                //sub the pcm header(4bytes) and the pcm data(0x1800bytes)
                                unsigned char *output_pcm_buf, *output_raw_buf;
                                output_pcm_buf = outbuf + HEADER_LENGTH_AFTER_IEC61937 + bytesread;
                                int output_pcm_len = *(int *)outbuf;
                                audec->pcm_cache_size=output_pcm_len;
                                outlen = outlen - HEADER_LENGTH_AFTER_IEC61937 - output_pcm_len;

                                //sub the raw header(4bytes) and the raw data(AC3:0x1800bytes/EAC3:0x6000bytes)
                                output_raw_buf = output_pcm_buf + output_pcm_len + HEADER_LENGTH_AFTER_IEC61937;
                                int output_raw_len = *(int *)(outbuf + HEADER_LENGTH_AFTER_IEC61937 + output_pcm_len);
                                outlen = outlen - HEADER_LENGTH_AFTER_IEC61937 - output_raw_len;

                                bytesread += 2*HEADER_LENGTH_AFTER_IEC61937+output_pcm_len+output_raw_len;
                                //use alsa-out.c output pcm data
                                if(g_bst){
                                    int wlen=0;

                                    while(output_pcm_len && (!audec->exit_decode_thread)) {
                                    if(g_bst->buf_length-g_bst->buf_level<output_pcm_len){
                                    usleep(100000);
                                    continue;
                                    }

                                    wlen=write_pcm_buffer(output_pcm_buf, g_bst, output_pcm_len);
                                    output_pcm_len -= wlen;
                                    audec->pcm_cache_size-=wlen;
                                    }
                                }

                                //use alsa-out-raw.c output raw data
                                if( (g_bst_raw) && ((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) )
                                {
                                    int wlen=0;

                                    while(output_raw_len && !audec->exit_decode_thread) {
                                        if(g_bst_raw->buf_length-g_bst_raw->buf_level<output_raw_len){
                                            usleep(100000);
                                            continue;
                                        }

                                        wlen=write_pcm_buffer(output_raw_buf, g_bst_raw,output_raw_len);
                                        output_raw_len -= wlen;
                                    }
                                }
                            }

                        } else {
                            audec->pcm_cache_size=outlen;
                            if (g_bst) {
                                int wlen=0;

                                while (outlen && !audec->exit_decode_thread) {
                                    if (g_bst->buf_length-g_bst->buf_level<outlen) {
                                        usleep(100000);
                                        continue;
                                    }

                                    wlen=write_pcm_buffer(outbuf, g_bst,outlen);
                                    outlen-=wlen;
                                    audec->pcm_cache_size-=wlen;
                                }
                            }
                        }
                  }
            }else{
                  usleep(1000);
                  continue;
            }
    }
    adec_print("[%s]exit audio_decode_loop Thread finished!",__FUNCTION__);
    pthread_exit(NULL);
error:
    pthread_exit(NULL);
    return NULL;
}

void *adec_armdec_loop(void *args)
{
    int ret;
    int wait_msg = 0;
    aml_audio_dec_t *audec;
    audio_out_operations_t *aout_ops;
    adec_cmd_t *msg = NULL;

    audec = (aml_audio_dec_t *)args;
    aout_ops = &audec->aout_ops;

    // codec init
    audec->state = INITING;
    while(1)
    {
        if(audec->need_stop)
            goto MSG_LOOP;

        ret = audio_codec_init(audec);
        if(ret ==0)
            break;
        usleep(10000);
    }
    audec->state = INITTED;

    //start decode thread
    while(1)
    {
        if(audec->need_stop)
        {
            //no need to call release, will do it in stop_adec
            goto MSG_LOOP;
        }
        if(audec->StageFrightCodecEnableType){
            //start_decode_thread_omx(audec);
            if(audec->OmxFirstFrameDecoded != 1) // just one case, need_stop == 1
            {
                //usleep(10000); // no need
                continue;
            }
        }
        else
            start_decode_thread(audec);
        //ok
        break;
    }

    //aout init
    while(1)
    {
        if(audec->need_stop)
        {
            //no need to call release, will do it in stop_adec
            goto MSG_LOOP;
        }
        //wait the audio sr/ch ready to set audio track.
        adec_print("wait audio sr/channel begin \n");
        while ( ((!audec->channels)|| (!audec->samplerate)) && !audec->need_stop){
            usleep(10000);
        }
        adec_print("wait audio sr/channel done \n");
        ret = aout_ops->init(audec);
        if (ret)
        {
            adec_print("[%s %d]Audio out device init failed!",__FUNCTION__,__LINE__);
            usleep(10000);
            continue;
        }
        //ok
        break;
    }

    //wait first pts decoded
    start_adec(audec);

MSG_LOOP:
    do {
        adec_reset_track(audec);
        //adec_flag_check(audec);

        if (audec->state == GAPPING) {
            unsigned long systime;
            adec_print("GAPPING -> resume_adec");
            audec->state = ACTIVE;

            avsync_en(0);
            while ((!audec->need_stop) && track_switch_pts(audec)) {
                usleep(1000);
            }

            // update APTS, not related to AV sync, just for netflix using APTS to control buffering latency
            // do this when av sync is still disabled so it sets APTS value only
            systime = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
            if (systime != -1) {
                char buf[64];
                sprintf(buf, "0x%lx", systime);
                amsysfs_set_sysfs_str(TSYNC_APTS, buf);
                adec_print("GAPPING -> apts: 0x%lx", systime);
            }

            // enable sync and set to AMASTER mode
            avsync_en(1);
            amsysfs_set_sysfs_int(TSYNC_MODE, 1);

            // resume audio
            aout_ops->resume(audec);
            adec_pts_resume();
            audec->auto_mute = 0;
        }

        //adec_print("adec_get_message\n");
        msg = adec_get_message(audec);
        if (!msg) {
            if (wait_msg == 0) {
                wait_msg = 1;
                adec_print("wait next message\n");
            }
            usleep(100000);
            continue;
        }
        wait_msg = 0;

        switch (msg->ctrl_cmd) {
        case CMD_START:

            adec_print("Receive START Command!\n");
            //------------------------
            if(!audec->StageFrightCodecEnableType)
                 start_decode_thread(audec);
            //------------------------
            start_adec(audec);
            break;

        case CMD_PAUSE:

            adec_print("Receive PAUSE Command!\n");
            pause_adec(audec);
            break;

        case CMD_RESUME:

            adec_print("Receive RESUME Command!\n");
            resume_adec(audec);
            break;

        case CMD_STOP:

            adec_print("Receive STOP Command!\n");
            stop_adec(audec);
            break;

        case CMD_MUTE:

            adec_print("Receive Mute Command!\n");
            if (msg->has_arg) {
                mute_adec(audec, msg->value.en);
            }
            break;

        case CMD_SET_VOL:

            adec_print("Receive Set Vol Command!\n");
            if (msg->has_arg) {
                adec_set_volume(audec, msg->value.volume);
            }
            break;

        case CMD_SET_LRVOL:

            adec_print("Receive Set LRVol Command!\n");
            if (msg->has_arg) {
                adec_set_lrvolume(audec, msg->value.volume,msg->value_ext.volume);
            }
            break;

        case CMD_SET_VOL_EASE:

            adec_print("Receive Set Vol Ease Command!\n");
            if (msg->has_arg) {
                adec_set_volume_ease(audec, msg->value.volume, msg->value_ext2, msg->value_ext.en);
            }
            break;

        case CMD_CHANL_SWAP:

            adec_print("Receive Channels Swap Command!\n");
            audio_hardware_ctrl(HW_CHANNELS_SWAP);
            break;

        case CMD_LEFT_MONO:

            adec_print("Receive Left Mono Command!\n");
            audio_hardware_ctrl(HW_LEFT_CHANNEL_MONO);
            break;

        case CMD_RIGHT_MONO:

            adec_print("Receive Right Mono Command!\n");
            audio_hardware_ctrl(HW_RIGHT_CHANNEL_MONO);
            break;

        case CMD_STEREO:

            adec_print("Receive Stereo Command!\n");
            audio_hardware_ctrl(HW_STEREO_MODE);
            break;

        case CMD_RELEASE:

            adec_print("Receive RELEASE Command!\n");
            release_adec(audec);
            break;

        default:
            adec_print("Unknow Command!\n");
            break;

        }

        if (msg) {
            adec_message_free(msg);
            msg = NULL;
        }
    } while (audec->state != TERMINATED);

    adec_print("Exit Message Loop Thread!");
    pthread_exit(NULL);
    return NULL;
}


