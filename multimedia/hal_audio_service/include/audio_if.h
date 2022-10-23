#ifndef __AUDIO_IF_H_
#define __AUDIO_IF_H_

#include <hardware/hardware.h>
#include <hardware/audio.h>

#ifdef __cplusplus
extern "C"
{
#endif

int audio_hw_load_interface(audio_hw_device_t **dev);
void audio_hw_unload_interface(audio_hw_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_IF_H_ */
