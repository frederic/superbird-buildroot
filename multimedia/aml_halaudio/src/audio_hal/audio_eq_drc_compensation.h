#ifndef _AUDIO_EQ_DRC_COMPENSATION_H_
#define _AUDIO_EQ_DRC_COMPENSATION_H_

#include "audio_eq_drc_parser.h"

struct eq_drc_data {
    int     card;
    int     dev_num;
    struct  mixer *mixer;
    struct  audio_eq_drc_info_s *p_attr;
    struct  audio_source_gain_s s_gain;
    struct  audio_post_gain_s p_gain;
};
#ifdef __cplusplus
extern "C" {
#endif

int eq_mode_set(struct eq_drc_data *pdata, int eq_mode);
int eq_drc_init(struct eq_drc_data *pdata);
int eq_drc_release(struct eq_drc_data *pdata);

#ifdef __cplusplus
}
#endif
#endif
