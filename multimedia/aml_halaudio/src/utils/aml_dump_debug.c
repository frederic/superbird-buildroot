/*
 * hardware/amlogic/audio/utils/aml_dump_debug.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include "log.h"
#include "properties.h"

#include <aml_dump_debug.h>

#undef  LOG_TAG
#define LOG_TAG "aml_dump_debug"

static int gDumpDataFd = -1;

void DoDumpData(const void *data_buf, int size, int aud_src_type) {
    int tmp_type = -1;
    char prop_value[PROPERTY_VALUE_MAX] = { 0 };
    char file_path[PROPERTY_VALUE_MAX] = { 0 };

    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("audio.dumpdata.en", prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0
            || strcasecmp(prop_value, "0") == 0) {
        if (gDumpDataFd >= 0) {
            close(gDumpDataFd);
            gDumpDataFd = -1;
        }
        return;
    }

    property_get("audio.dumpdata.src", prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0
            || strcasecmp(prop_value, "input") == 0
            || strcasecmp(prop_value, "0") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_INPUT;
    } else if (strcasecmp(prop_value, "output") == 0
            || strcasecmp(prop_value, "1") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_OUTPUT;
    } else if (strcasecmp(prop_value, "input_parse") == 0
            || strcasecmp(prop_value, "2") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_INPUT_PARSE;
    }

    if (tmp_type != aud_src_type) {
        return;
    }

    memset(file_path, '\0', PROPERTY_VALUE_MAX);
    property_get("audio.dumpdata.path", file_path, "null");
    if (strcasecmp(file_path, "null") == 0) {
        file_path[0] = '\0';
    }

    if (gDumpDataFd < 0 && file_path[0] != '\0') {
        if (access(file_path, 0) == 0) {
            gDumpDataFd = open(file_path, O_RDWR | O_SYNC);
            if (gDumpDataFd < 0) {
                ALOGE("%s, Open device file \"%s\" error: %s.\n",
                        __FUNCTION__, file_path, strerror(errno));
            }
        } else {
            gDumpDataFd = open(file_path, O_WRONLY | O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR);
            if (gDumpDataFd < 0) {
                ALOGE("%s, Create device file \"%s\" error: %s.\n",
                        __FUNCTION__, file_path, strerror(errno));
            }
        }
    }

    if (gDumpDataFd >= 0) {
        write(gDumpDataFd, data_buf, size);
    }
    return;
}
