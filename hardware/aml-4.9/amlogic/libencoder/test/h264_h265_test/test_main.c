#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "h264/bjunion_enc/vpcodec_1_0.h"
#include "h265/test.h"

#define SLT_AVC_TEST 0
#define SLT_HEVC_TEST 0

#define RET_LEN 1024

int my_system(const char *cmd, char *result, int len)
{
    assert(cmd != NULL && result != NULL
        && len >= RET_LEN);

    FILE *fp = NULL;
    int i = 0;

    if ((fp = popen(cmd, "r")) == NULL ) {
        printf("popen error!\n");
        return -1;
    }

    while (fgets(result, len, fp) != NULL) {
        printf("result: %s\n", result);
    }

    while (result[i] != ' ')
        i++;

    result[i] = '\0';

    pclose(fp);

}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: [encoder_test] [codec_id], \
                codec_id:    1: h.264, 2 : h.265, 3 : h.264 + h.265\n");
        return -1;
    }

    int ret = -1;
    char* result_avc_2 = NULL;
    char* result_avc_1 = NULL;
    char* result_hevc_1 = NULL;
    char* result_hevc_2 = NULL;

    if ((atoi(argv[1]) & 1) == 1) {
        result_avc_1 = (char *) malloc(sizeof(char) * RET_LEN);
        result_avc_2 = (char *) malloc(sizeof(char) * RET_LEN);

        avc_encoder_params_t avc_params_1 = {
            "nv21_480p.yuv",
            "es.h264.1",
            640,
            480,
            120,
            30,
            800000,
            1,
            2,
            0,
            2,
            false
        };
        avc_encoder_params_t avc_params_2 = {
            "nv21_480p.yuv",
            "es.h264.2",
            640,
            480,
            120,
            30,
            800000,
            1,
            2,
            0,
            2,
            false
        };

         //h.264 test
         for (int i = 0; i < 20; i++) {
             memset(result_avc_1, 0, sizeof(char) * RET_LEN);
             memset(result_avc_2, 0, sizeof(char) * RET_LEN);

             avc_encode(&avc_params_1);

             avc_encode(&avc_params_2);

             my_system("md5sum es.h264.1", result_avc_1, RET_LEN);
             my_system("md5sum es.h264.2", result_avc_2, RET_LEN);

             ret = strcmp(result_avc_1, result_avc_2);
             if (ret == 0) {
                 break;
             }
        }

        printf("\n\n{\"result\": %s, \"item\": avc_encoder}\n\n\n", \
             (ret == 0 && avc_params_1.is_avc_work && avc_params_2.is_avc_work) \
                 ? "true" : "false");

    }

    if ((atoi(argv[1]) & 2) == 2) {
        result_hevc_1 = (char *) malloc(sizeof(char) * RET_LEN);
        result_hevc_2 = (char *) malloc(sizeof(char) * RET_LEN);

        hevc_encoder_params_t hevc_params_1 = {
            "nv21_480p.yuv",
            "es.h265.1",
            640,
            480,
            120,
            30,
            800000,
            1,
            2,
            0,
            2,
            false
        };
        hevc_encoder_params_t hevc_params_2 = {
            "nv21_480p.yuv",
            "es.h265.2",
            640,
            480,
            120,
            30,
            800000,
            1,
            2,
            0,
            2,
            false
        };

        //h.265 test
        hevc_encode(&hevc_params_1);

        hevc_encode(&hevc_params_2);

        my_system("md5sum es.h265.1", result_hevc_1, RET_LEN);
        my_system("md5sum es.h265.2", result_hevc_2, RET_LEN);

        ret = strcmp(result_hevc_1, result_hevc_2);
        printf("\n\n{\"result\": %s, \"item\": hevc_encoder}\n\n\n", \
            (ret==0 && hevc_params_1.is_hevc_work && hevc_params_2.is_hevc_work)
                ? "true" : "false");

    }

    if (result_hevc_1 != NULL) {
        free(result_hevc_1);
        result_hevc_1 = NULL;
    }

    if (result_hevc_2 != NULL) {
        free(result_hevc_2);
        result_hevc_2 = NULL;
    }

    if (result_avc_1 != NULL) {
        free(result_avc_1);
        result_avc_1 = NULL;
    }

    if (result_avc_2 != NULL) {
        free(result_avc_2);
        result_avc_2 = NULL;
    }

    system("rm -f es* ");

    return 0;
}

