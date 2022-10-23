#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#include "SysWrite.h"
#include "common.h"


SysWrite::SysWrite() {
}

SysWrite::~SysWrite() {
}

bool SysWrite::readSysfs(const char *path, char *value){
	 char buf[MAX_STR_LEN+1] = {0};
	 readSys(path, (char*)buf, MAX_STR_LEN, false);
	 strcpy(value, buf);
	 return true;
}

// get the original data from sysfs without any change.
bool SysWrite::readSysfsOriginal(const char *path, char *value){
    char buf[MAX_STR_LEN+1] = {0};
    readSys(path, (char*)buf, MAX_STR_LEN, true);
    strcpy(value, buf);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value){
    writeSys(path, value);
    return true;
}

bool SysWrite::writeSysfs(const char *path, const char *value, const int size){
    writeSys(path, value, size);
    return true;
}

void SysWrite::writeSys(const char *path, const char *val){
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        syslog(LOG_ERR, "SysWrite writeSys open %s fail.", path);
        return;
    }

    write(fd, val, strlen(val));
    close(fd);
}

int SysWrite::writeSys(const char *path, const char *val, const int size){
    int fd;

    if ((fd = open(path, O_WRONLY)) < 0) {
        syslog(LOG_ERR, "SysWrite writeSys open %s fail.", path);
        return -1;
    }

    if (write(fd, val, size) != size) {
        syslog(LOG_ERR, "SysWrite writeSys write %s size:%d failed!\n", path, size);
        return -1;
    }

    close(fd);
    return 0;
}

int SysWrite::readSys(const char *path, char *buf, int count) {
    int fd;
    int len = -1;

    if (!buf) {
        syslog(LOG_ERR, "SysWrite readSys buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        syslog(LOG_ERR, "SysWrite readSys open %s fail.", path);
        return -1;
    }

    if ((len = read(fd, buf, count)) < 0) {
        syslog(LOG_ERR, "SysWrite read %s error %s.", path, strerror(errno));
    }

    close(fd);
    return len;
}


void SysWrite::readSys(const char *path, char *buf, int count, bool needOriginalData){
    int fd;
    int len;
    char *pos;

    if (!buf) {
        syslog(LOG_ERR, "SysWrite readSys buf NULL.");
        return;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        syslog(LOG_ERR, "SysWrite readSys open %s fail.", path);
        goto ret;
    }

    if ((len = read(fd, buf, count)) < 0) {
        syslog(LOG_ERR, "SysWrite readSys read %s error %s.", path, strerror(errno));
        goto ret;
    }

    if (!needOriginalData) {
        int i;
        int j;
        for (i = 0, j = 0; i <= len -1; i++) {
            /*change '\0' to 0x20(spacing), otherwise the string buffer will be cut off
             * if the last char is '\0' should not replace it
             */
            if (0x0 == buf[i] && i < len - 1) {
                buf[i] = 0x20;
            }

            /* delete all the character of '\n' */
            if (0x0a != buf[i]) {
                buf[j++] = buf[i];
            }
        }

        buf[j] = 0x0;
    }

ret:
    close(fd);
}

void SysWrite::executeCMD(const char *cmd, char *result) {
    char buf_ps[1024];
    char ps[1024] = {0};
    FILE *ptr;
    strcpy(ps, cmd);
    if ((ptr=popen(ps, "r")) != NULL) {
        while (fgets(buf_ps, 1024, ptr) != NULL) {
	    syslog(LOG_INFO, "SysWrite registerValue: %s\n", buf_ps);
            strcat(result, buf_ps);
	    if (strlen(result) > 1024)
	        break;
	}
	pclose(ptr);
	ptr = NULL;
    } else {
	syslog(LOG_ERR, "SysWrite popen %s error\n", ps);
    }
}
