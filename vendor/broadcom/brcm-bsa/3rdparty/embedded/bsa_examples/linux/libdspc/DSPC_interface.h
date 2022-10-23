#ifdef __cplusplus
extern "C"{
#endif

int dspc_init(void);

int alloc_dspc_data_buffer(int16_t *ring_buffer,int *p_wp_alsa, int *p_rp_alsa);

int thread_run_start();

int dspc_pcm_read_start();

int dspc_pcm_read_stop();
#ifdef __cplusplus
}
#endif
