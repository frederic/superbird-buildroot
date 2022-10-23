#ifndef _NOISE_GATE_H_
#define _NOISE_GATE_H_

#ifdef __cplusplus
extern "C"  {
#endif

enum ng_status {
    NG_ERROR = -1,
    NG_UNMUTE = 0,
    NG_MUTE,
};

void* init_noise_gate(float noise_level, int attrack_time, int release_time);
void release_noise_gate(void *ng_handle);
int noise_evaluation(void *ng_handle, void *buffer, int sample_counter);

#ifdef __cplusplus
}
#endif

#endif
