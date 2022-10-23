#include <syslog.h>
#include <string.h>
#include <pthread.h>

#include "SystemControl.h"
#include "DisplayMode.h"
#include "ubootenv.h"

#define LOG_TAG "systemcontrol"

SystemControl::SystemControl(const char *path) {
    bootenv_init();
    pDisplayMode = new DisplayMode(path);
    syslog(LOG_INFO, "SystemControl::SystemControl");
    pDisplayMode->init();

    const char *firstBoot = bootenv_get("ubootenv.var.firstboot");
    if (firstBoot && !strcmp(firstBoot, "1")) {
        syslog(LOG_INFO, "SystemControl clear first_boot(%s)", firstBoot);
        if (bootenv_update("ubootenv.var.firstboot", "0") < 0) {
            syslog(LOG_ERR, "SystemControl set firstboot fail");
        }
    }
}

SystemControl::~SystemControl() {
    syslog(LOG_INFO, "SystemControl::~SystemControl");
    delete pDisplayMode;
}

int SystemControl::getBootEnv(const char *key, char *value) {
    const char *val = bootenv_get(key);
    if (val) {
        strncpy(value, val, strlen(val));
        return 1;
    }
    return 0;
}

int SystemControl::setBootEnv(const char *key, const char *value) {
    bootenv_update(key, value);
}

void SystemControl::setTvOutputMode(const char* mode) {
    if (!strcmp(mode, "auto")) {
        setBootEnv(UBOOTENV_ISBESTMODE, "true");
    } else {
        setBootEnv(UBOOTENV_ISBESTMODE, "false");
    }
    pDisplayMode->setSourceOutputMode(mode);
}

void SystemControl::setTvColorFormat(const char* mode) {
    pDisplayMode->setSourceColorFormat(mode);
}

void SystemControl::setTvHdcpMode(const char* mode) {
    pDisplayMode->setSourceHdcpMode(mode);
}

void SystemControl::setZoomOut() {
    pDisplayMode->zoomOut();
}

void SystemControl::setZoomIn() {
    pDisplayMode->zoomIn();
}
