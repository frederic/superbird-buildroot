/**
 * \file adec-internal-mgt.h
 * \brief  Definitiond Of Audiodsp Types And Structures
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_INTERNAL_MGT_H
#define ADEC_INTERNAL_MGT_H

#include <adec-macros.h>
#include <adec-types.h>

/*********************************************************************************************/
typedef struct dsp_operations dsp_operations_t;

struct dsp_operations {
    int dsp_on;
    unsigned long kernel_audio_pts;
    unsigned long last_audio_pts;
    unsigned long last_pts_valid;
    int (*dsp_read)(dsp_operations_t *dsp_ops, char *buffer, int len);                                        /* read pcm stream from dsp */
    int (*dsp_read_raw)(dsp_operations_t *dsp_ops, char *buffer, int len); /* read raw stream from dsp */
    unsigned long(*get_cur_pts)(dsp_operations_t *);
    unsigned long(*get_cur_pcrscr)(dsp_operations_t *);
    int (*set_cur_apts)(dsp_operations_t *dsp_ops,unsigned long apts);
    int amstream_fd;
    void *audec;
};

typedef struct {
    int cmd;
    int fmt;
    int data_len;
    char *data;
} audiodsp_cmd_t;

typedef struct {
    int     id;
    int     fmt;
    char    name[64];
} firmware_s_t;

#endif //ADEC_INTERNAL_MGT_H
