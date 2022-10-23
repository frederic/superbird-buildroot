#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h> 
#include <libgen.h>
//#include <stdbool.h>
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

	return 0;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: [encoder_test] [codec_id], \
                codec_id:    1: h.264, 2 : h.265, 3 : h.264 + h.265\n");
        return -1;
    }
#ifdef __ANDROID__ 
    char work_dir[]="/data";
#elif __linux__
    char work_dir[]="/tmp"
#endif

    char input_file[256]={0};
    char exe_path[256]={0};
    char *exe_dir;
    readlink("/proc/self/exe", exe_path, 256);
    exe_dir = dirname(exe_path);
    //printf("exe_dir=%s\n", exe_dir);

    sprintf(input_file, "%s/nv21_480p.yuv", exe_dir);

    char avc_outfile1[256]={0};
    char avc_outfile2[256]={0};

    char hevc_outfile1[256]={0};
    char hevc_outfile2[256]={0};

    int ret = -1;
    char* result_avc_2 = NULL;
    char* result_avc_1 = NULL;
    char* result_hevc_1 = NULL;
    char* result_hevc_2 = NULL;

    sprintf(avc_outfile1, "%s/es.h264.1", work_dir);
    sprintf(avc_outfile2, "%s/es.h264.2", work_dir);

    sprintf(hevc_outfile1, "%s/es.h265.1", work_dir);
    sprintf(hevc_outfile2, "%s/es.h265.2", work_dir);

    remove(avc_outfile1);
    remove(avc_outfile2);
    remove(hevc_outfile1);
    remove(hevc_outfile2);

    //printf("input_file= %s\n  %s\n  %s\n  %s\n  %s\n",
    //    input_file, avc_outfile1, avc_outfile2, hevc_outfile1, hevc_outfile2);

    if ((atoi(argv[1]) & 1) == 1) {
        result_avc_1 = (char *) malloc(sizeof(char) * RET_LEN);
        result_avc_2 = (char *) malloc(sizeof(char) * RET_LEN);

        avc_encoder_params_t avc_params_1 = {
            .width=640,
            .height=480,
            .gop=120,
            .framerate=30,
            .bitrate=800000,
            .num=1,
            .format=2,
            .buf_type=0,
            .num_planes=2,
            .is_avc_work=false
        };
        strcpy(avc_params_1.src_file, input_file);
        strcpy(avc_params_1.es_file, avc_outfile1);

        avc_encoder_params_t avc_params_2 = {
            .width=640,
            .height=480,
            .gop=120,
            .framerate=30,
            .bitrate=800000,
            .num=1,
            .format=2,
            .buf_type=0,
            .num_planes=2,
            .is_avc_work=false
        };
        strcpy(avc_params_2.src_file, input_file);
        strcpy(avc_params_2.es_file, avc_outfile2);
         //h.264 test
         for (int i = 0; i < 20; i++) {
             memset(result_avc_1, 0, sizeof(char) * RET_LEN);
             memset(result_avc_2, 0, sizeof(char) * RET_LEN);

             avc_encode(&avc_params_1);

             avc_encode(&avc_params_2);

             char command[256]={0};
             memset(command, 0, 256);

             sprintf(command, "md5sum %s", avc_outfile1);
             my_system(command, result_avc_1, RET_LEN);

             memset(command, 0, 256);

             sprintf(command, "md5sum %s", avc_outfile2);
             my_system(command, result_avc_2, RET_LEN);

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
            .width=640,
            .height=480,
            .gop=120,
            .framerate=30,
            .bitrate=800000,
            .num=1,
            .format=2,
            .buf_type=0,
            .num_planes=2,
            .is_hevc_work=false
        };
        strcpy(hevc_params_1.src_file, input_file);
        strcpy(hevc_params_1.es_file, hevc_outfile1);

        hevc_encoder_params_t hevc_params_2 = {
            .width=640,
            .height=480,
            .gop=120,
            .framerate=30,
            .bitrate=800000,
            .num=1,
            .format=2,
            .buf_type=0,
            .num_planes=2,
            .is_hevc_work=false
        };
        strcpy(hevc_params_2.src_file, input_file);
        strcpy(hevc_params_2.es_file, hevc_outfile2);

        //h.265 test
        hevc_encode(&hevc_params_1);

        hevc_encode(&hevc_params_2);

        char command[256]={0};
        memset(command, 0, 256);

        sprintf(command, "md5sum %s", hevc_outfile1);
        my_system(command, result_hevc_1, RET_LEN);

        sprintf(command, "md5sum %s", hevc_outfile2);
        my_system(command, result_hevc_2, RET_LEN);

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

    remove(avc_outfile1);
    remove(avc_outfile2);
    remove(hevc_outfile1);
    remove(hevc_outfile2);
    return 0;
}

