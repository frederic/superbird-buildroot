#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <syslog.h>

#include "DisplayMode.h"
#include "FrameRateAutoAdaption.h"
#define LOG_TAG "systemcontrol"

FrameRateAutoAdaption::FrameRateAutoAdaption(Callbak *cb): mCallback(cb){
}

FrameRateAutoAdaption::~FrameRateAutoAdaption() {
}

void FrameRateAutoAdaption::readSinkEdid(char *edid) {
    int count = 0;
    while (true) {
        mSysWrite.readSysfsOriginal(DISPLAY_HDMI_EDID, edid);
        if (strlen(edid) > 0)
            break;

        if (count >= 5) {
            strcpy(edid, "null edid");
            break;
        }
        count++;
        usleep(500000);
    }
}

//Get best match display mode and if need pull down 1/1000 as video frame duration
void FrameRateAutoAdaption::getMatchDurOutputMode (int dur, char *curMode, char *frameRateMode, char *newMode, bool *pulldown) {
    syslog(LOG_DEBUG, "get match duration outputmode duration: %d\n", dur);

    if (strstr(curMode, "smpte")) {
        return;
    }
    //frame rate adapter mode 1:only need pull down 1/1000
    if (!strcmp(frameRateMode, FRAME_RATE_HDMI_CLK_PULLDOWN)) {
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_5992)
            || (dur == FRAME_RATE_DURATION_5994)) {

            if (strstr(curMode, "24hz")
                || strstr(curMode, "30hz")
                || strstr(curMode, "60hz")) {
                *pulldown = true;
            }
        }
    }
    //frame rate adapter mode 2:need change display mode and pull down 1/1000
    else if (!strcmp(frameRateMode, FRAME_RATE_HDMI_SWITCH_FORCE)) {
        char resolution[10] = {0};
        char firstMode[MODE_LEN] = {0};
        char secondMode[MODE_LEN] = {0};
        bool needPulldown = false;
        char sinkEdid[MAX_STR_LEN] = {0};

        readSinkEdid(sinkEdid);
        char *pos = strstr(curMode, "hz");
        if (NULL == pos) {
            syslog(LOG_DEBUG, "get match duration outputmode, current display mode: %s do not have hz keyword\n", curMode);
            return;
        }

        pos -= 2;//filter 24,30,50,60...
        strncpy(resolution, curMode, int(pos - curMode));
        // if need 1/1000 pull down
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_5992)
            || (dur == FRAME_RATE_DURATION_5994)) {
            needPulldown = true;
        }

        // get first and second mode for this duration
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_24)) {
            sprintf(firstMode, "%s%s", resolution, "24hz");
            sprintf(secondMode, "%s%s", resolution, "60hz");
        }
        else if ((dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_30)) {
            sprintf(firstMode, "%s%s", resolution, "30hz");
            sprintf(secondMode, "%s%s", resolution, "60hz");
        }
        else if ((dur == FRAME_RATE_DURATION_5992)
            ||(dur == FRAME_RATE_DURATION_5994)
            || (dur == FRAME_RATE_DURATION_60)) {
            sprintf(firstMode, "%s%s", resolution, "60hz");
            sprintf(secondMode, "%s%s", resolution, "30hz");
        }
        else if (dur == FRAME_RATE_DURATION_25) {
            sprintf(firstMode, "%s%s", resolution, "25hz");
            sprintf(secondMode, "%s%s", resolution, "50hz");
        }
        else if (dur == FRAME_RATE_DURATION_50) {
            sprintf(firstMode, "%s%s", resolution, "50hz");
            sprintf(secondMode, "%s%s", resolution, "25hz");
        }

        if (!strcmp(firstMode, curMode)) {
            *pulldown = needPulldown;
        }
        else if (strstr(sinkEdid, firstMode)) {
            strncpy(newMode, firstMode, strlen(firstMode));
            *pulldown = needPulldown;
        }
        else if (!strcmp(secondMode, curMode)) {
            *pulldown = needPulldown;
        }
        else if (strstr(sinkEdid, secondMode)) {
            strncpy(newMode, secondMode, strlen(firstMode));
            *pulldown = needPulldown;
        }
        else if (strstr(curMode, "24hz")
            ||strstr(curMode, "30hz")
            ||strstr(curMode, "60hz"))
            *pulldown = needPulldown;
        else
            *pulldown = false;

        syslog(LOG_DEBUG, "get match duration outputmode, firstMode: %s secondMode:%s, new disaplay mode:%s\n", firstMode, secondMode, newMode);
    }
}

void FrameRateAutoAdaption::onTxUeventReceived(UEventData* ueventData){
    char framerateMode[8] ={0};
    char curDisplayMode[MODE_LEN] = {0};

    mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, framerateMode);

    syslog(LOG_DEBUG, "Video framerate switchName: %s, switchState: %s, current display mode:%s, frame rate status:%s[0:off 1:pulldown 2:force switch]\n",
        ueventData->switchName, ueventData->switchState, curDisplayMode, framerateMode);

    if (!strcmp(ueventData->switchName, "end_hint")) {
        autoSwitchFlag = false;
    }
    if (NULL != strstr(curDisplayMode, "cvbs")) {
        syslog(LOG_DEBUG, "CVBS mode do not need auto frame rate\n");
        return;
    }

    if (!strcmp(framerateMode, FRAME_RATE_HDMI_OFF)) {
        syslog(LOG_DEBUG, "hdmi frame rate is off\n");
        return;
    }

    //frame rate is in pulldown or switch force mode
    if (!strcmp(ueventData->switchName, "end_hint")) {
        syslog(LOG_DEBUG, "Video framerate switch end hint last mode: %s\n", mLastVideoMode);
        if (strlen(mLastVideoMode) > 0) {
            mCallback->onDispModeSyncEvent(mLastVideoMode, OUTPUT_MODE_STATE_ADAPTER_END);
            memset(mLastVideoMode, 0, sizeof(mLastVideoMode));
        }
    }
    else{
        char newDisplayMode[MODE_LEN] = {0};
        bool pulldow = false;
        int duration = 0;

        sscanf(ueventData->switchName, "%d", &duration);
        if (duration > 0)
            getMatchDurOutputMode(duration, curDisplayMode, framerateMode, newDisplayMode, &pulldow);

        syslog(LOG_DEBUG, "Video framerate switch new display mode: %s, pulldown:%d\n", newDisplayMode, pulldow?1:0);
        if (pulldow) {
            autoSwitchFlag = true;
            strcpy(mLastVideoMode, curDisplayMode);
            mCallback->onDispModeSyncEvent((strlen(newDisplayMode) != 0)?newDisplayMode:curDisplayMode, OUTPUT_MODE_STATE_SWITCH_ADAPTER);
        }
        else if (strlen(newDisplayMode) != 0) {
            strcpy(mLastVideoMode, curDisplayMode);
            mCallback->onDispModeSyncEvent(newDisplayMode, OUTPUT_MODE_STATE_SWITCH);
        }
    }
}
