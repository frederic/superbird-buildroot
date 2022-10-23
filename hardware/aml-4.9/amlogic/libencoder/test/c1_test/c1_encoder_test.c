#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RET_LEN 1024

int my_system(const char *cmd, char *result, int len)
{
    assert(cmd != NULL && result != NULL
        && len >= RET_LEN);

    FILE * fp = NULL;
    int i = 0;

    if ((fp = popen(cmd, "r")) == NULL ) {
        printf("popen error!\n");
        return -1;
    }

    if (fgets(result, len, fp) != NULL) {
        printf("result: %s\n", result);
    } else {
        printf("fgets failed\n");
	return -1;
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
        printf("Usage: [c1_encoder_test] [codec_id]\n");
        printf("  codec_id:   1: h.264, 2 : h.265, 3 : h.264 + h.265\n");

        return -1;
    }

    int ret = -1;
    char* result_avc_2 = NULL;
    char* result_avc_1 = NULL;
    char* result_hevc_1 = NULL;
    char* result_hevc_2 = NULL;

    //h.264 test
    if ((atoi(argv[1]) & 1) == 1) {
        result_avc_1 = (char *) calloc(RET_LEN, sizeof(char));
        result_avc_2 = (char *) calloc(RET_LEN, sizeof(char));

	if (result_avc_1 == NULL || result_avc_2 == NULL) {
	    printf("avc calloc failed\n");
	    exit(1);
	}
        //h.264 test 1
        system("aml_enc_test nv21_480p.yuv  c1_es.h264.1 640 480 120 30 800000 1 2 0 2 4");

	sleep(1);
        //h.264 test 2
        system("aml_enc_test nv21_480p.yuv  c1_es.h264.2 640 480 120 30 800000 1 2 0 2 4");

        if (my_system("md5sum c1_es.h264.1", result_avc_1, RET_LEN) != 0) {
            printf("md5sum c1_es.h264.1 failed\n");
            goto EXIT;
	}
        if (my_system("md5sum c1_es.h264.2", result_avc_2, RET_LEN) != 0) {
            printf("md5sum c1_es.h264.2 failed\n");
            goto EXIT;
	}

        if (strlen(result_avc_1) != 0 && strlen(result_avc_2) != 0) {
	    ret = strcmp(result_avc_1, result_avc_2);
	}

        printf("\n\n{\"result\": %s, \"item\": c1_h264_encoder}\n\n\n", \
             (ret == 0) ? "true" : "false");
    }

    //h.265 test
    ret = -1;
    if ((atoi(argv[1]) & 2) == 2) {
        result_hevc_1 = (char *) calloc(RET_LEN, sizeof(char));
        result_hevc_2 = (char *) calloc(RET_LEN, sizeof(char));

	if (result_hevc_1 == NULL || result_hevc_2 == NULL) {
	    printf("hevc calloc failed\n");
	    exit(1);
	}

        //h.265 test 1
        system("aml_enc_test nv21_480p.yuv  c1_es.h265.1 640 480 120 30 800000 1 2 0 2 5");

	sleep(1);

        //h.265 test 2
        system("aml_enc_test nv21_480p.yuv  c1_es.h265.2 640 480 120 30 800000 1 2 0 2 5");

        if (my_system("md5sum c1_es.h265.1", result_hevc_1, RET_LEN) != 0) {
            printf("md5sum c1_es.h265.1 failed\n");
            goto EXIT;
	}
        if (my_system("md5sum c1_es.h265.2", result_hevc_2, RET_LEN) != 0) {
            printf("md5sum c1_es.h265.2 failed\n");
            goto EXIT;
	}

        if (strlen(result_hevc_1) != 0 && strlen(result_hevc_2) != 0) {
            ret = strcmp(result_hevc_1, result_hevc_2);
        }

        printf("\n\n{\"result\": %s, \"item\": c1_h265_encoder}\n\n\n", \
             (ret == 0) ? "true" : "false");
    }


EXIT:

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

    system("rm -f  c1_es* ");

    return 0;
}


