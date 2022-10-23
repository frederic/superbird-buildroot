#include <stdio.h>
#include <stdlib.h>

#include "bootloader_message.h"

void usage() {
        printf("eg:urlmisc write http://xx.28.xx.53:8080/otaupdate/swupdate/software.swu\n");
        printf("     urlmisc read\n");
        printf("     urlmisc clean\n");
}

int main(int argc, char **argv) {

    int ret = 0;
    char path_buf[256] = {0};
    if ((argc != 2) && (argc != 3)) {
        usage();
        return -1;
    }

    mtd_scan_partitions();

    if (argc == 3) {
        if (!strcmp(argv[1], "write")) {
            ret = set_recovery_otapath(argv[2]);
            if (ret != 0) {
                printf("set otapath : %s failed!\n", argv[1]);
                return -1;
            }
        } else {
            usage();
            return -1;
        }
    } else {
        if (!strcmp(argv[1], "read")) {
            ret = get_recovery_otapath(path_buf);
            if (ret != 0) {
                printf("get otapath failed!\n");
                return -1;
            }
        } else if (!strcmp(argv[1], "clean")) {
            ret = clean_recovery_otapath();
            if (ret != 0) {
                printf("clean otapath failed!\n");
                return -1;
            }
        } else {
            usage();
            return -1;
        }
    }

    return 0;
}
