/**
 * \file adec-pts-mgt.c
 * \brief  Functions Of Pts Manage.
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
#include <errno.h>
#include <unistd.h>

#include <adec-pts-mgt.h>
#include <sys/time.h>
#include <Amavutils.h>

//#define adec_print printf

int adec_pts_droppcm(aml_audio_dec_t *audec);
int vdec_pts_resume(void);
static int vdec_pts_pause(void);


int sysfs_get_int(char *path, unsigned long *val)
{
    char buf[64];

    if (amsysfs_get_sysfs_str(path, buf, sizeof(buf)) == -1) {
        adec_print("unable to open file %s,err: %s", path, strerror(errno));
        return -1;
    }
    if (sscanf(buf, "0x%lx", val) < 1) {
        adec_print("unable to get pts from: %s", buf);
        return -1;
    }

    return 0;
}
/*
get vpts when refresh apts,  do not use sys write servie as it is too slow sometimes.
*/
static int mysysfs_get_sysfs_int16(const char *path)
{
    int fd;
    char valstr[64];
    int val;
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
	  memset(valstr,0,64);
        read(fd, valstr, 64 - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        adec_print("unable to open file %s\n", path);
        return -1;
    }
    if (sscanf(valstr, "0x%lx", &val) < 1) {
        adec_print("unable to get pts from: %s", valstr);
        return -1;
    }
    return val;
}
static int set_tsync_enable(int enable)
{
    char *path = "/sys/class/tsync/enable";
    return amsysfs_set_sysfs_int(path, enable);
}
/**
 * \brief calc current pts
 * \param audec pointer to audec
 * \return aurrent audio pts
 */
unsigned long adec_calc_pts(aml_audio_dec_t *audec)
{
    unsigned long pts, delay_pts;
    audio_out_operations_t *out_ops;
    dsp_operations_t *dsp_ops;

    out_ops = &audec->aout_ops;
    dsp_ops = &audec->adsp_ops;

    pts = dsp_ops->get_cur_pts(dsp_ops);
    if (pts == -1) {
        adec_print("get get_cur_pts failed\n");
        return -1;
    }
    dsp_ops->kernel_audio_pts = pts;

    if (out_ops == NULL || out_ops->latency == NULL) {
        adec_print("cur_out is NULL!\n ");
        return -1;
    }
    delay_pts = out_ops->latency(audec) * 90;

    if(!audec->apts_start_flag)
        return pts;

    int diff = abs(delay_pts-pts);
    // in some case, audio latency got from audiotrack is bigger than pts<hw not enabled>, so when delay_pts-pts < 100ms ,so set apts to 0.
    //because 100ms pcr-apts diff cause  apts reset .
    if (delay_pts/*+audec->first_apts*/ < pts) {
        pts -= delay_pts;
    } else if(diff < 9000) {
        pts = 0;
    }
    return pts;
}

/**
 * \brief start pts manager
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
int adec_pts_start(aml_audio_dec_t *audec)
{
    unsigned long pts = 0;
    char *file;
    char buf[64];
    dsp_operations_t *dsp_ops;
	char value[PROPERTY_VALUE_MAX]={0};
    int tsync_mode;
    unsigned long first_apts = 0;

    adec_print("[%s %d]", __func__, __LINE__);
    dsp_ops = &audec->adsp_ops;
    memset(buf, 0, sizeof(buf));

    if (audec->avsync_threshold <= 0) {
        if (am_getconfig_bool("media.libplayer.wfd")) {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD * 2 / 3;
            adec_print("use 2/3 default av sync threshold!\n");
        } else {
            audec->avsync_threshold = SYSTIME_CORRECTION_THRESHOLD;
            adec_print("use default av sync threshold!\n");
        }
    }

    first_apts = adec_calc_pts(audec);
    if (sysfs_get_int(TSYNC_FIRSTAPTS, &audec->first_apts) == -1) {
        adec_print("## [%s::%d] unable to get first_apts! \n",__FUNCTION__,__LINE__);
        return -1;
    }

    adec_print("av sync threshold is %d , no_first_apts=%d,first_apts = 0x%x, audec->first_apts = 0x%x \n", audec->avsync_threshold, audec->no_first_apts,first_apts, audec->first_apts);
    dsp_ops->last_pts_valid = 0;

#if 0
    //default enable drop pcm
    int enable_drop_pcm = 1;
    if(property_get("sys.amplayer.drop_pcm",value,NULL) > 0)
    {
        enable_drop_pcm = atoi(value);
    }
    adec_print("[%s:%d] enable_drop_pcm :%d \n",__FUNCTION__,__LINE__, enable_drop_pcm);
    if(enable_drop_pcm)
        adec_pts_droppcm(audec);
#endif
    // before audio start or pts start
    if(amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PRE_START") == -1)
    {
        return -1;
    }

    usleep(1000);

	if (audec->no_first_apts) {
		if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
			adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
			return -1;
		}

		if (sscanf(buf, "0x%lx", &pts) < 1) {
			adec_print("unable to get vpts from: %s", buf);
			return -1;
		}

	} else {
	    pts = adec_calc_pts(audec);
	    if (pts == -1) {

	        adec_print("pts==-1");

    		if (amsysfs_get_sysfs_str(TSYNC_APTS, buf, sizeof(buf)) == -1) {
    			adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
    			return -1;
    		}

	        if (sscanf(buf, "0x%lx", &pts) < 1) {
	            adec_print("unable to get apts from: %s", buf);
	            return -1;
	        }
	    }
	}

    return 0;
}

/**
 * \brief pause pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_pause(void)
{
    adec_print("adec_pts_pause\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_PAUSE");
}

/**
 * \brief resume pts manager
 * \return 0 on success otherwise -1
 */
int adec_pts_resume(void)
{
    adec_print("adec_pts_resume\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "AUDIO_RESUME");

}

/**
 * \brief refresh current audio pts
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 */
 static int apts_interrupt=0;
int adec_refresh_pts(aml_audio_dec_t *audec)
{
    unsigned long pts;
    unsigned long systime;
    unsigned long last_pts = audec->adsp_ops.last_audio_pts;
    unsigned long last_kernel_pts = audec->adsp_ops.kernel_audio_pts;
    int apts_start_flag = 0;//store the last flag
    int samplerate = 48000;
    int channels = 2;
    char buf[64];
    char ret_val = -1;
    if (audec->auto_mute == 1) {
        return 0;
    }
    apts_start_flag = audec->apts_start_flag;
//if the audio start has not been triggered to tsync,calculate the audio  pcm data which writen to audiotrack
    if(!audec->apts_start_flag){
	    if(audec->g_bst){
	    	samplerate = audec->g_bst->samplerate;
	    	channels = audec->g_bst->channels;
	    }
	    //DSP decoder must have got audio info when feeder_init
	    else{
	    	samplerate = audec->samplerate;
	    	channels = audec->channels;
	    }
	    // 100 ms  audio hal have  triggered the output hw.
    	    if(audec->pcm_bytes_readed*1000/(samplerate*channels*2) >= 100 ){
    	    	audec->apts_start_flag =  1;
    	    }
    }
    memset(buf, 0, sizeof(buf));

    systime = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    if (systime == -1) {
        adec_print("unable to getsystime");
//        audec->pcrscr64 = 0;
        return -1;
    }

    /* get audio time stamp */
    pts = adec_calc_pts(audec);
    if(pts != -1 && (apts_start_flag != audec->apts_start_flag)){
	    adec_print("audio pts start from 0x%lx\n", pts);
	    sprintf(buf, "AUDIO_START:0x%lx", pts);
	    if(amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1)
	    {
	        return -1;
	    }
    }
    if (pts == -1 || last_pts == pts) {
        //close(fd);
        //if (pts == -1) {
//        audec->pcrscr64 = (int64_t)systime;
        return -1;
        //}
    }

    if ((abs(pts - last_pts) > APTS_DISCONTINUE_THRESHOLD) && (audec->adsp_ops.last_pts_valid)) {
        /* report audio time interruption */
        adec_print("pts = %lx, last pts = %lx\n", pts, last_pts);

        adec_print("audio time interrupt: 0x%lx->0x%lx, 0x%lx\n", last_pts, pts, abs(pts - last_pts));

        sprintf(buf, "AUDIO_TSTAMP_DISCONTINUITY:0x%lx", pts);

        if(amsysfs_set_sysfs_str(TSYNC_EVENT, buf) == -1)
        {
            adec_print("unable to open file %s,err: %s", TSYNC_EVENT, strerror(errno));
            return -1;
        }

        audec->adsp_ops.last_audio_pts = pts;
        audec->adsp_ops.last_pts_valid = 1;
        adec_print("[%s:%d]set automute orig %d!\n", __FUNCTION__, __LINE__, audec->auto_mute);
        audec->auto_mute = 0;
        apts_interrupt=5;
        return 0;
    }

    if (last_kernel_pts == audec->adsp_ops.kernel_audio_pts) {
        return 0;
    }

    //adec_print("last_kernel_pts = %d", last_kernel_pts/90);
    if ((((int)(audec->adsp_ops.kernel_audio_pts - last_kernel_pts)) > 500 * 90) && (audec->adsp_ops.last_pts_valid)) {
        // if there is APTS interrupt (audio gap possible in Netflix NTS case)
        adec_print("Audio gap, %d->%d", last_kernel_pts/90, audec->adsp_ops.kernel_audio_pts/90);
        avsync_en(0);
        adec_pts_pause();
        audec->state = GAPPING;
        return 0;
    }

    if (audec->state == GAPPING)
        return 0;

    audec->adsp_ops.last_audio_pts = pts;
    audec->adsp_ops.last_pts_valid = 1;

    if (abs(pts - systime) < audec->avsync_threshold) {
        //adec_print("pts=0x%x, systime=0x%x, diff = %d ms", pts, systime,  ((int)(pts-systime))/90);
        apts_interrupt=5;
        return 0;
    }
    else if(apts_interrupt>0){
        //adec_print("apts_interrupt=%d, pts=0x%x, systime=0x%x, diff = %d ms", apts_interrupt, pts, systime, (pts-systime)/90);
        apts_interrupt --;
        return 0;
        }

    /* report apts-system time difference */
    if(!apts_start_flag)
      return 0;

    if(audec->adsp_ops.set_cur_apts){
        ret_val = audec->adsp_ops.set_cur_apts(&audec->adsp_ops,pts);
    }
    else{
	 sprintf(buf, "0x%lx", pts);
	 ret_val = amsysfs_set_sysfs_str(TSYNC_APTS, buf);
    }
    if(ret_val == -1)
    {
        adec_print("unable to open file %s,err: %s", TSYNC_APTS, strerror(errno));
        return -1;
    }
    //adec_print("report apts as %ld,system pts=%ld, difference= %ld\n", pts, systime, (pts - systime));
    return 0;
}

/**
 * \brief Disable or Enable av sync
 * \param e 1 =  enable, 0 = disable
 * \return 0 on success otherwise -1
 */
int avsync_en(int e)
{
    return amsysfs_set_sysfs_int(TSYNC_ENABLE, e);
}

/**
 * \brief calc pts when switch audio track
 * \param audec pointer to audec
 * \return 0 on success otherwise -1
 *
 * When audio track switch occurred, use this function to judge audio should
 * be played or not. If system time fall behind audio pts , and their difference
 * is greater than SYSTIME_CORRECTION_THRESHOLD, auido should wait for
 * video. Otherwise audio can be played.
 */
int track_switch_pts(aml_audio_dec_t *audec)
{
    unsigned long vpts;
    unsigned long apts;
    unsigned long pcr;
    char buf[32];

    memset(buf, 0, sizeof(buf));

    pcr = audec->adsp_ops.get_cur_pcrscr(&audec->adsp_ops);
    if (pcr == -1) {
        adec_print("track_switch_pts unable to get pcr");
        return 1;
    }

    apts = adec_calc_pts(audec);
    if (apts == -1) {
        adec_print("track_switch_pts unable to get apts");
        return 1;
    }

    // adec_calc_pts returns apts with fixed track latency correct,
    // which is not correctly reflects the delay when output has not
    // been started at switching case.
    // accurate timeing control for when to start feeding audio data
    // is important to make audio switching smooth and avoid resetting
    // system time from APTS of the new audio track. We need start audio
    // track feeding to make the time stamp of the first audio sample
    // output from HW is exactly the system time set up before switching.
    // Unfurtunately, we never know how long it will take at Android
    // AudioFlinger. adec_calc_pts has a fixed latency applied to APTS,
    // when audio buffers in the AudioFlinger/Output pipeline is full,
    // this offset is almost right. However, when initially the pipeline
    // is all empty, use fixed latency is not correct.
    // The number given below is a correction on top of the fixed latency.
    // e.g., assuming system time is 10000ms, first apts is 12000ms, latency
    // is 100ms, then adec_calc_pts returns 12000-100 = 11900ms. The external
    // loop will wait 100ms to finish switching wait. When system time is 11900ms,
    // audio data with 12000ms is fed to AudioFlinger, but it does not take 100ms
    // to get output. With 100ms fixed latency, the data feeding is too earlier
    // and this will cause apts reset system time soon.
    // the number below of 50ms is from experiment. It assumes the time between
    // first data feeding to output is 100-50=50ms.

    apts += 50*90;

    //adec_print("track_switch_pts apts=%d, pcr=%d, kernel_pts=%d", apts/90, pcr/90, audec->adsp_ops.kernel_audio_pts/90);

    if((apts > pcr) && (apts - pcr > 0x100000))
        return 0;

    if (abs(apts - pcr) < audec->avsync_threshold || (apts <= pcr)) {
        return 0;
    } else {
        return 1;
    }

}
static int vdec_pts_pause(void)
{
    //char *path = "/sys/class/video_pause";
    //return amsysfs_set_sysfs_int(path, 1);
    adec_print("vdec_pts_pause\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x1");

}
int vdec_pts_resume(void)
{
    adec_print("vdec_pts_resume\n");
    return amsysfs_set_sysfs_str(TSYNC_EVENT, "VIDEO_PAUSE:0x0");
}
