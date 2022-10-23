#include "hevc_enc/vp_hevc_codec_1_0.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, const char *argv[]){
    int width, height, gop, framerate, bitrate, num;
    int outfd = -1;
    FILE *fp = NULL;
    int datalen = 0;
    int fmt = 0;
    vl_codec_handle_t handle_enc;
    if (argc < 9)
    {
        printf("Amlogic AVC Encode API \n");
        printf(" usage: output [srcfile][outfile][width][height][gop][framerate][bitrate][num]\n");
        printf("  options  :\n");
        printf("  srcfile  : yuv data url in your root fs\n");
        printf("  outfile  : stream url in your root fs\n");
        printf("  width    : width\n");
        printf("  height   : height\n");
        printf("  gop      : I frame refresh interval\n");
        printf("  framerate: framerate \n ");
        printf("  bitrate  : bit rate \n ");
        printf("  num      : encode frame count \n ");
        printf("  fmt      : encode input fmt 0:nv21, 1:nv12, 2:RGB888\n ");
        return -1;
    }
    else
    {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }
    width =  atoi(argv[3]);
    if ((width < 1) || (width > 3840/*1920*/))
    {
        printf("invalid width \n");
        return -1;
    }
    height = atoi(argv[4]);
    if ((height < 1) || (height > 2160/*1080*/))
    {
        printf("invalid height \n");
        return -1;
    }
    gop = atoi(argv[5]);
    framerate = atoi(argv[6]);
    bitrate = atoi(argv[7]);
    num = atoi(argv[8]);
    fmt = atoi(argv[9]);
    if ((framerate < 0) || (framerate > 30))
    {
        printf("invalid framerate \n");
        return -1;
    }
    if (bitrate <= 0)
    {
        printf("invalid bitrate \n");
        return -1;
    }
    if (num < 0)
    {
        printf("invalid num \n");
        return -1;
    }
    printf("src_url is: %s ;\n", argv[1]);
    printf("out_url is: %s ;\n", argv[2]);
    printf("width   is: %d ;\n", width);
    printf("height  is: %d ;\n", height);
    printf("gop     is: %d ;\n", gop);
    printf("frmrate is: %d ;\n", framerate);
    printf("bitrate is: %d ;\n", bitrate);
    printf("frm_num is: %d ;\n", num);

    unsigned int frameSize  = width * height * 3 / 2;
    if (fmt == 2) {
        frameSize = width * height * 3;
    }
    unsigned int outputBufferLen = 1024 * 1024 * sizeof(char);
    unsigned char *inputBuffer = (unsigned char *)malloc(frameSize);
    unsigned char *outputBuffer = (unsigned char *)malloc(outputBufferLen);

    fp = fopen((char *)argv[1], "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        goto exit;
    }
    outfd = open((char *)argv[2], O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dist file error!\n");
        goto exit;
    }
    handle_enc = vl_video_encoder_init(CODEC_ID_H265, width, height, framerate, bitrate, gop);
    while (num > 0) {
        if (fread(inputBuffer, 1, frameSize, fp) != frameSize) {
            printf("read input file error!\n");
            break;
        }
        memset(outputBuffer, 0, outputBufferLen);
        datalen = vl_video_encoder_encode(handle_enc, FRAME_TYPE_AUTO, inputBuffer, outputBufferLen, outputBuffer, fmt);
        if (datalen >= 0)
            write(outfd, (unsigned char *)outputBuffer, datalen);
        else {
            printf("encode error %d! continue ?\n",datalen);
            //break;
        }
        num--;
    }
    vl_video_encoder_destory(handle_enc);
    close(outfd);
    fclose(fp);
    free(inputBuffer);
    free(outputBuffer);
    return 0;
exit:
    if (inputBuffer)
        free(inputBuffer);
    if (outputBuffer)
        free(outputBuffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    return -1;
}