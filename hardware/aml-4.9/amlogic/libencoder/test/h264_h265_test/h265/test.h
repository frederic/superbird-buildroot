#ifndef __INCLUDE_TEST_H_
#define __INCLUDE_TEST_H_

typedef struct hevc_encoder_params {
    char src_file[100];
    char es_file[100];
    int width;
    int height;
    int gop;
    int framerate;
    int bitrate;
    int num;
    int format;
    int buf_type;
    int num_planes;
    bool is_hevc_work;
} hevc_encoder_params_t;

int hevc_encode(hevc_encoder_params_t * param);

#endif
