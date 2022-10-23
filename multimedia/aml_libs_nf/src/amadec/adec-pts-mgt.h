/**
 * \file adec-pts-mgt.h
 * \brief  Function prototypes of Pts manage.
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_PTS_H
#define ADEC_PTS_H

#include <audio-dec.h>
#define TIME_UNIT90K 90000
ADEC_BEGIN_DECLS

typedef enum {
    TSYNC_MODE_VMASTER,
    TSYNC_MODE_AMASTER,
    TSYNC_MODE_PCRMASTER,
} tsync_mode_t;

#define AMSTREAM_IOC_MAGIC 'S'
#define AMSTREAM_IOC_GET_LAST_CHECKIN_APTS   _IOR(AMSTREAM_IOC_MAGIC, 0xa9, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKIN_VPTS   _IOR(AMSTREAM_IOC_MAGIC, 0xaa, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_APTS  _IOR(AMSTREAM_IOC_MAGIC, 0xab, unsigned long)
#define AMSTREAM_IOC_GET_LAST_CHECKOUT_VPTS  _IOR(AMSTREAM_IOC_MAGIC, 0xac, unsigned long)
#define AMSTREAM_IOC_AB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x09, unsigned long)

#define TSYNC_PCR_DISPOINT  "/sys/class/tsync_pcr/tsync_pcr_discontinue_point"
#define TSYNC_PCRSCR    "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT     "/sys/class/tsync/event"
#define TSYNC_APTS      "/sys/class/tsync/pts_audio"
#define TSYNC_VPTS      "/sys/class/tsync/pts_video"
#define TSYNC_ENABLE  "/sys/class/tsync/enable"
#define TSYNC_LAST_CHECKIN_APTS "/sys/class/tsync/last_checkin_apts"
#define TSYNC_MODE   "/sys/class/tsync/mode"
#define TSYNC_FIRSTVPTS "/sys/class/tsync/firstvpts"
#define TSYNC_FIRSTAPTS "/sys/class/tsync/firstapts"
#define TSYNC_CHECKIN_FIRSTVPTS "/sys/class/tsync/checkin_firstvpts"

#define SYSTIME_CORRECTION_THRESHOLD        (90000*6/100)//modified for amlogic-pd-91949
#define APTS_DISCONTINUE_THRESHOLD          (90000*3)
#define REFRESH_PTS_TIME_MS                 (1000/10)

#define abs(x) ({                               \
                long __x = (x);                 \
                (__x < 0) ? -__x : __x;         \
                })


/**********************************************************************/
int sysfs_get_int(char *path, unsigned long *val);
unsigned long adec_calc_pts(aml_audio_dec_t *audec);
int adec_pts_start(aml_audio_dec_t *audec);
int adec_pts_pause(void);
int adec_pts_resume(void);
int adec_refresh_pts(aml_audio_dec_t *audec);
int avsync_en(int e);
int track_switch_pts(aml_audio_dec_t *audec);
int adec_get_tsync_info(int *tsync_mode);

struct buf_status {
	int size;
	int data_len;
	int free_len;
	unsigned int read_pointer;
	unsigned int write_pointer;
};

struct am_io_param {
	int data;
	int len; //buffer size;
	struct buf_status status;
};
ADEC_END_DECLS

#endif
