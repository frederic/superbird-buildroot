#define LOG_TAG "systemcontrol"
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include "DisplayMode.h"
#include "FormatColorDepth.h"
#include "common.h"
#include "ubootenv.h"

//this is prior selected list  of 4k2k50hz, 4k2k60hz smpte50hz,smpte60hz
static const char* COLOR_ATTRIBUTE_LIST1[] = {
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of other display mode
static const char* COLOR_ATTRIBUTE_LIST2[] = {
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

FormatColorDepth::FormatColorDepth() {
}

FormatColorDepth::~FormatColorDepth() {
}

bool FormatColorDepth::initColorAttribute(char* supportedColorList, int len) {
    syslog(LOG_INFO, "FormatColorDepth::initColorAttribute");
    int count = 0;
    bool result = false;

    if (supportedColorList != NULL)
        memset(supportedColorList, 0, len);

    while (true) {
        mSysWrite.readSysfs(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        if (strlen(supportedColorList) > 0) {
            result = true;
            break;
        }

        if (count++ >= 5) {
            break;
        }
        usleep(500000);
    }

    return result;
}

void FormatColorDepth::getHdmiColorAttribute(const char* outputmode, char* colorAttribute, int state) {
    syslog(LOG_INFO, "FormatColorDepth::getHdmiColorAttribute");
    char supportedColorList[MAX_STR_LEN];

    //if read /sys/class/amhdmitx/amhdmitx0/dc_cap is null. return
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
              || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
            strcpy(colorAttribute, COLOR_YCBCR420_8BIT);
        } else {
            strcpy(colorAttribute, COLOR_YCBCR444_8BIT);
        }
        syslog(LOG_ERR, "FormatColorDepth::getHdmiColorAttribute: Error!!! Do not find sink color list, use default color attribute:%s\n", colorAttribute);
        return;
    }

    char curMode[MODE_LEN] = {0};
    char isBestMode[MODE_LEN] = {0};
    mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curMode);

    // if only change deepcolor in MoreSettings, getcolorAttr from ubootenv, it was set in DroidTvSettings
    if (((state == OUTPUT_MODE_STATE_SWITCH) && (!strcmp(curMode, outputmode)) || (state == OUTPUT_MODE_STATE_POWER)) && getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) && (strcmp(isBestMode, "false") == 0)) {
        //note: "outputmode" should be the second parameter of "strcmp", because it maybe prefix of "curMode".
        syslog(LOG_INFO, "FormatColorDepth::getHdmiColorAttribute: Only modify deep color mode, get colorAttr from ubootenv.var.colorattribute\n");
        getBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
        if (isModeSupportDeepColorAttr(outputmode, colorAttribute)) {
            syslog(LOG_INFO, "FormatColorDepth::getHdmiColorAttribute: current outputmode support deepcolor(%s)\n", colorAttribute);
        } else {
	    syslog(LOG_ERR, "FormatColorDepth::getHdmiColorAttribute: current outputmode(%s) not support deepcolor(%s), set the best deepcolor\n", outputmode, colorAttribute);
	    getProperHdmiColorArrtibute(outputmode,  colorAttribute);
        }
    } else {
        getProperHdmiColorArrtibute(outputmode,  colorAttribute);
    }

    syslog(LOG_INFO, "FormatColorDepth::getHdmiColorAttribute: get hdmi color attribute : [%s], outputmode is: [%s] , and support color list is: [%s]\n", colorAttribute, outputmode, supportedColorList);
}

void FormatColorDepth::getProperHdmiColorArrtibute(const char* outputmode, char* colorAttribute) {
    syslog(LOG_INFO, "FormatColorDepth::getProperHdmiColorArrtibute");
    char ubootvar[MODE_LEN] = {0};
    char tmpValue[MODE_LEN] = {0};
    char isBestMode[MODE_LEN] = {0};

    getBestHdmiDeepColorAttr(outputmode, colorAttribute);
    //if colorAttr is null above steps, will defines a initial value to it
    if (!strstr(colorAttribute, "bit")) {
        strcpy(colorAttribute, COLOR_YCBCR444_8BIT);
    }

}

void FormatColorDepth::getBestHdmiDeepColorAttr(const char *outputmode, char* colorAttribute) {
    syslog(LOG_INFO, "FormatColorDepth::getBestHdmiDeepColorAttr");
    char *pos = NULL;
    int length = 0;
    const char **colorList = NULL;
    char supportedColorList[MAX_STR_LEN];
    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        return;
    }

    //filter some color value options, aimed at some modes.
    if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)
        || !strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
        colorList = COLOR_ATTRIBUTE_LIST1;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
    } else {
        colorList = COLOR_ATTRIBUTE_LIST2;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
    }

    for (int i = 0; i < length; i++) {
        if ((pos = strstr(supportedColorList, colorList[i])) != NULL) {
            if (isModeSupportDeepColorAttr(outputmode, colorList[i])) {
                syslog(LOG_INFO, "FormatColorDepth::getBestHdmiColorAttr: support current mode:[%s], deep color:[%s]\n", outputmode, colorList[i]);

                strcpy(colorAttribute, colorList[i]);
                break;
            }
        }
    }
}

bool FormatColorDepth::isModeSupportDeepColorAttr(const char *mode, const char * color) {
    char valueStr[10] = {0};
    char outputmode[MODE_LEN] = {0};
    strcpy(outputmode, mode);
    strcat(outputmode, color);
    //try support or not
    mSysWrite.writeSysfs(DISPLAY_HDMI_VALID_MODE, outputmode);
    mSysWrite.readSysfs(DISPLAY_HDMI_VALID_MODE, valueStr);
    return atoi(valueStr) ? true : false;
}

bool FormatColorDepth::getBootEnv(const char* key, char* value) {
    const char* p_value = bootenv_get(key);
    if (p_value) {
        strcpy(value, p_value);
        return true;
    }
    return false;
}
