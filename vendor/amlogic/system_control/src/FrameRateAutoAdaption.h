#ifndef FRAMERATE_ADAPTER_H
#define FRAMERATE_ADAPTER_H

#include "SysWrite.h"
#include "common.h"
#include "UEventObserver.h"

/*
Duration time base is 1/96000 second.
So FRAME_RATE_DURATION is a frame display
duration time use this time base as unit.
For example frame rate is 23.976 fps
FRAME_RATE_DURATION_23.97 = 96000/23.976 = 4004
*/
#define HDMI_TX_FRAMRATE_POLICY                 "/sys/class/amhdmitx/amhdmitx0/frac_rate_policy"

//Frame rate switch
#define HDMI_TVOUT_FRAME_RATE_UEVENT            "DEVPATH=/devices/virtual/tv/tv"
#define HDMI_FRAME_RATE_AUTO                    "/sys/class/tv/policy_fr_auto"
#define FRAME_RATE_DURATION_2397                4004
#define FRAME_RATE_DURATION_2398                4003
#define FRAME_RATE_DURATION_24                  4000
#define FRAME_RATE_DURATION_25                  3840
#define FRAME_RATE_DURATION_2997                3203
#define FRAME_RATE_DURATION_30                  3200
#define FRAME_RATE_DURATION_50                  1920
#define FRAME_RATE_DURATION_5994                1601
#define FRAME_RATE_DURATION_5992                1602
#define FRAME_RATE_DURATION_60                  1600
#define FRAME_RATE_HDMI_OFF                     "0"
#define FRAME_RATE_HDMI_CLK_PULLDOWN            "1"
#define FRAME_RATE_HDMI_SWITCH_FORCE            "2"

class FrameRateAutoAdaption
{
public:
    class Callbak {
    public:
        Callbak() {};
        virtual ~Callbak() {};
        virtual void onDispModeSyncEvent (const char* outputmode, int state) = 0;
    };

    FrameRateAutoAdaption(Callbak *cb);
    ~FrameRateAutoAdaption();

    void onTxUeventReceived(UEventData* ueventData);
    bool autoSwitchFlag = false;

private:
    void readSinkEdid(char *edid);
    void getMatchDurOutputMode (int dur, char *curMode, char *frameRateMode, char *newMode, bool *pulldown);

    Callbak *mCallback;
    SysWrite mSysWrite;
    char mLastVideoMode[MODE_LEN] = {0};
};
#endif // FRAMERATE_ADAPTER_H
