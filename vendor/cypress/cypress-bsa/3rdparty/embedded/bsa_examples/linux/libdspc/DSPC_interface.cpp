#include "PortAudioMicrophoneWrapper.h"
#include "DSPC_interface.h"

#ifdef __cplusplus
extern "C" {
#endif
std::shared_ptr<PortAudioMicrophoneWrapper> Paw; 

int dspc_init(void)
{
        Paw = PortAudioMicrophoneWrapper::create();
	if (!Paw) {
		printf("PortAudioMicrophoneWrapper create error!!!\n");
		return false;
	}
	return 0;
}

int alloc_dspc_data_buffer(int16_t *ring_buffer,int *p_wp_alsa, int *p_rp_alsa)
{	
	int ret = 0;
        ret = Paw->do_dspc_data_buffer_alloc(ring_buffer,p_wp_alsa,p_rp_alsa);
	if(!ret)
		return 0;
	else
		return -1;
}

int thread_run_start()
{
        Paw->thread_run();
	return 0;
}

int dspc_pcm_read_start()
{
        Paw->pcm_read_start();
	return 0;
}

int dspc_pcm_read_stop()
{
        Paw->pcm_read_stop();
	return 0;
}
#ifdef __cplusplus
}
#endif
