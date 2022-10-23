#ifndef _AML_AUDIO_PARSER_
#define _AML_AUDIO_PARSER_

struct aml_audio_parser {
    struct audio_hw_device *dev;
    ring_buffer_t aml_ringbuffer;
    pthread_t audio_parse_threadID;
    pthread_mutex_t mutex;
    int parse_thread_exit;
    void *audio_parse_para;
    audio_format_t aformat;
    pthread_t decode_ThreadID;
    pthread_mutex_t *decode_dev_op_mutex;
    int decode_ThreadExitFlag;
    int decode_enabled;
    struct pcm *aml_pcm;
    int in_sample_rate;
    int out_sample_rate;
    struct resample_para aml_resample;
    int data_ready;
};

#endif

