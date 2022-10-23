#include <fcntl.h>
#include <stdio.h>
#include "vpcodec_1_0.h"

static char *filename_in = "/system/bin/1.h264";
static char *filename_out = "/data/1.yuv";
static FILE* dumpfd = NULL;
static int out_len = 1280*720*3/2;

void main(int argc,char **argv) {
    vl_codec_handle_t handle = vl_video_decoder_init(CODEC_ID_H264);
    FILE *file_in = fopen(filename_in,"r");
    if (file_in == NULL) {
        printf("open file %s failed\n",filename_in);
        return 0;
    }

    dumpfd = open(filename_out, O_CREAT | O_RDWR, 0666);
    if (!dumpfd) {
        printf("-- dump file create fail. %d----\n",errno);
    }

    char *in = (char *)malloc(64*1000);
    char *out = (char *)malloc(out_len);
    int inputcount = 0;
    while (!feof(file_in)) {
        uint32_t  in_size = 0;
        int ret = fread(in, 1024*32, 1, file_in);
        if (ret > 0)
            in_size = ret*1024*32;
        else {
            printf("read data failed %d\n",ret);
            break;
        }
        inputcount++;
        ret = vl_video_decoder_decode(handle, in, in_size, &out);
        if (ret > 0) {
            write(dumpfd, out, ret);
        }
        usleep(100*1000);
    }

    free(in);
    free(out);
    int ret = vl_video_decoder_destory(handle);
    fclose(file_in);
    if (dumpfd != NULL) {
        close(dumpfd);
        dumpfd = NULL;
    }
}

