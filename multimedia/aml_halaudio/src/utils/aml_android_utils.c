/*
 * Copyright (C) 2017 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 ** aml_android_utils.c
 **
 ** This program is APIs for get/set android property, get/set sys fs point.
 ** author: shen pengru
 **
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "log.h"
#include <fcntl.h>
//#include <cutils/str_parms.h>
//#include <cutils/properties.h>
#include <aml_android_utils.h>

#undef  LOG_TAG
#define LOG_TAG "audio_android_utils"
#define PROPERTY_VALUE_MAX 124

/*
 * Android Property
 */
int aml_getprop_bool(const char * path)
{
	char buf[PROPERTY_VALUE_MAX];
	int ret = -1;

	//ret = property_get(path, buf, NULL);
	if (ret > 0) {
		if (strcasecmp(buf, "true") == 0 || strcmp(buf, "1") == 0)
			return eAML_BOOL_TRUE;
	}

	return eAML_BOOL_FALSE;
}

int aml_getprop_int(const char *path)
{
	char buf[PROPERTY_VALUE_MAX];
	int ret = -1;
	int value = 0;

	//ret = property_get(path, buf, NULL);
	if (ret > 0) {
		sscanf(buf, "%d", &value);
	}

	return value;
}

/*
* Linux Sys Fs Set/Get Interface
*/
int aml_sysfs_get_int (const char *path)
{
	int val = 0;
	int fd = open (path, O_RDONLY);
	if (fd >= 0) {
		char bcmd[16];
		read (fd, bcmd, sizeof (bcmd));
		val = strtol (bcmd, NULL, 10);
		close (fd);
	} else {
		ALOGE("%s: open %s node failed! return 0, err: %s\n", __func__, path, strerror(errno));
	}

	return val;
}

int aml_sysfs_get_int16(const char *path,unsigned *value)
{
	int fd;
	char valstr[64];
	unsigned  val = 0;

	fd = open(path, O_RDONLY);
	if (fd >= 0) {
		memset(valstr, 0, 64);
		read(fd, valstr, 64 - 1);
		valstr[strlen(valstr)] = '\0';
		close(fd);
	} else {
		ALOGE("%s: unable to open file %s, err: %s\n", __func__, path, strerror(errno));
		return -1;
	}
	if (sscanf(valstr, "0x%x", &val) < 1) {
		ALOGE("%s: unable to get pts from: %s, err: %s\n", __func__, valstr, strerror(errno));
		return -1;
	}
	*value = val;

	return 0;
}

int aml_sysfs_get_str(const char *path, char *buf, int count)
{
	int fd, len;
	int i , j;

	if ( NULL == buf ) {
		ALOGE("buf is NULL");
		return -1;
	}

	if ((fd = open(path, O_RDONLY)) < 0) {
		ALOGE("readSys, open %s error(%s)", path, strerror (errno));
		return -1;
	}

	len = read(fd, buf, count);
	if (len < 0) {
		ALOGE("read %s error, %s\n", path, strerror(errno));
		goto exit;
	}

	for (i = 0, j = 0; i <= len -1; i++) {
		//change '\0' to 0x20(spacing), otherwise the string buffer will be cut off ,if the last char is '\0' should not replace it
		if (0x0 == buf[i] && i < len - 1) {
			buf[i] = 0x20;
		}
		/* delete all the character of '\n' */
		if (0x0a != buf[i]) {
			buf[j++] = buf[i];
		}
	}

	buf[j] = 0x0;

exit:
	close(fd);

	return len;
}

int aml_sysfs_set_str(const char *path, const char *val)
{
	int fd;
	int bytes;

	fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0) {
		bytes = write(fd, val, strlen(val));
		close(fd);
		return 0;
	} else {
		ALOGE("%s: unable to open file %s,err: %s\n", __func__, path, strerror(errno));
	}

	return -1;
}

/*
* Aml private strstr
*/
int aml_strstr(char *mystr,char *substr) {
	int i=0;
	int j=0;
	int score = 0;
	int substrlen = 0;
	int ok = 0;

	substrlen = strlen(substr);
	for (i =0;i < 1024 - substrlen;i++) {
		for (j = 0;j < substrlen;j++) {
			score += (substr[j] == mystr[i+j])?1:0;
		}
		if (score == substrlen) {
			ok = 1;
			break;
		}
		score = 0;
	}

	return ok;
}
