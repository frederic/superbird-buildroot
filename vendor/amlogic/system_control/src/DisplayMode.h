#ifndef DISPLAY_MODE_H
#define DISPLAY_MODE_H

#include "common.h"
#include "SysWrite.h"
#include "HDCPTxAuth.h"
#include "FrameRateAutoAdaption.h"
#include "TxUEventCallback.h"
#include "FormatColorDepth.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DEVICE_STR_MBOX                 "MBOX"
#define SYSFS_DISPLAY_MODE              "/sys/class/display/mode"
#define SYS_DISABLE_VIDEO               "/sys/class/video/disable_video"
#define SYSFS_DISPLAY_AXIS              "/sys/class/display/axis"
#define SYSFS_VIDEO_AXIS                "/sys/class/video/axis"
#define VIDEO_LAYER_ENABLE              "0"
#define VIDEO_LAYER_DISABLE             "1"
#define VIDEO_LAYER_AUTO_ENABLE         "2"

#define DEFAULT_OUTPUT_MODE             "480p60hz"
#define DISPLAY_CFG_FILE                "/etc/mesondisplay.cfg"
#define DISPLAY_HPD_STATE               "/sys/class/amhdmitx/amhdmitx0/hpd_state"
#define DISPLAY_FB0_BLANK               "/sys/class/graphics/fb0/blank"
#define DISPLAY_FB1_BLANK               "/sys/class/graphics/fb1/blank"
#define DISPLAY_FB0_FREESCALE           "/sys/class/graphics/fb0/free_scale"
#define DISPLAY_FB1_FREESCALE           "/sys/class/graphics/fb1/free_scale"

#define DISPLAY_EDID_VALUE              "/sys/class/amhdmitx/amhdmitx0/edid"
#define DISPLAY_EDID_STATUS             "/sys/class/amhdmitx/amhdmitx0/edid_parsing"
#define DISPLAY_EDID_RAW                "/sys/class/amhdmitx/amhdmitx0/rawedid"

#define DISPLAY_HDMI_EDID               "/sys/class/amhdmitx/amhdmitx0/disp_cap"//RX support display mode
#define DISPLAY_HDMI_DEEP_COLOR         "/sys/class/amhdmitx/amhdmitx0/dc_cap"//RX supoort deep color
#define DISPLAY_HDMI_SINK_TYPE          "/sys/class/amhdmitx/amhdmitx0/sink_type"
#define DISPLAY_HDMI_HDCP_MODE          "/sys/class/amhdmitx/amhdmitx0/hdcp_mode"//set HDCP mode
#define DISPLAY_HDMI_PHY                "/sys/class/amhdmitx/amhdmitx0/phy"
#define DISPLAY_HDMI_HDR                "/sys/class/amhdmitx/amhdmitx0/hdr_cap"
#define DISPLAY_HDMI_HDR_MODE           "/sys/module/am_vecm/parameters/hdr_mode"
#define DISPLAY_HDMI_SDR_MODE           "/sys/module/am_vecm/parameters/sdr_mode"
#define DISPLAY_HDMI_AVMUTE             "/sys/devices/virtual/amhdmitx/amhdmitx0/avmute"
#define DISPLAY_HDMI_AUDIO_MUTE         "/sys/class/amhdmitx/amhdmitx0/aud_mute"
#define DISPLAY_HDMI_VIDEO_MUTE         "/sys/class/amhdmitx/amhdmitx0/vid_mute"
#define DISPLAY_HDMI_AUDIO              "/sys/class/amhdmitx/amhdmitx0/aud_cap"
#define DISPLAY_HDMI_HDCP14_STOP        "stop14" //stop HDCP1.4 authenticate
#define DISPLAY_HDMI_HDCP22_STOP        "stop22" //stop HDCP2.2 authenticate
#define DISPLAY_HDMI_HDCP_14            "1"
#define DISPLAY_HDMI_HDCP_22            "2"
#define DISPLAY_HDMI_HDCP_INIT          "3"
#define DISPLAY_HDMI_HDCP_VER           "/sys/class/amhdmitx/amhdmitx0/hdcp_ver"//RX support HDCP version
#define DISPLAY_HDMI_HDCP_AUTH          "/sys/module/hdmitx20/parameters/hdmi_authenticated"//HDCP Authentication
#define DISPLAY_HDMI_HDCP_CONF          "/sys/class/amhdmitx/amhdmitx0/hdcp_ctrl" //HDCP config
#define DISPLAY_HDMI_HDCP_KEY           "/sys/class/amhdmitx/amhdmitx0/hdcp_lstore"//TX have 22 or 14 or none key
#define DISPLAY_HDMI_HDCP_POWER         "/sys/class/amhdmitx/amhdmitx0/hdcp_pwr"//write to 1, force hdcp_tx22 quit safely
#define DISPLAY_FB0_FREESCALE_AXIS      "/sys/class/graphics/fb0/free_scale_axis"
#define DISPLAY_FB0_WINDOW_AXIS         "/sys/class/graphics/fb0/window_axis"
#define DISPLAY_FB0_SCALE_AXIS          "/sys/class/graphics/fb0/scale_axis"
#define DISPLAY_FB1_SCALE_AXIS          "/sys/class/graphics/fb1/scale_axis"
#define DISPLAY_FB1_SCALE               "/sys/class/graphics/fb1/scale"

#define AUDIO_DSP_DIGITAL_RAW           "/sys/class/audiodsp/digital_raw"
#define AV_HDMI_CONFIG                  "/sys/class/amhdmitx/amhdmitx0/config"

#define HDMI_TX_HDCP_UEVENT             "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdcp"
#define HDMI_TX_PLUG_UEVENT             "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi"
#define HDMI_TX_AUDIO_UEVENT            "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_audio"
#define HDMI_TX_PLUG_STATE              "sys/class/extcon/hdmi/state"
#define HDMI_TX_POWER_UEVENT            "DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_power"
#define HDMI_TX_HDR_UEVENT              "DEVPATH=/devices/virtual/switch/hdmi_hdr"
#define HDMI_TX_SWITCH_HDR              "/sys/class/switch/hdmi_hdr/state"
#define HDMI_TX_SUSPEND                 "0"
#define HDMI_TX_RESUME                  "1"
//HDCP RX
#define HDMI_RX_PLUG_UEVENT             "DEVPATH=/devices/virtual/switch/hdmirx_hpd"//1:plugin 0:plug out
#define HDMI_RX_AUTH_UEVENT             "DEVPATH=/devices/virtual/switch/hdmirx_hdcp_auth"//0:FAIL 1:HDCP14 2:HDCP22

#define HDMI_RX_PLUG_OUT                "0"
#define HDMI_RX_PLUG_IN                 "1"
#define HDMI_RX_AUTH_FAIL               "0"
#define HDMI_RX_AUTH_HDCP14             "1"
#define HDMI_RX_AUTH_HDCP22             "2"

#define HDMI_UEVENT_HDCP                "hdmitx_extcon_hdcp"
#define HDMI_UEVENT_HDMI                "hdmitx_extcon_hdmi"
#define HDMI_UEVENT_HDMI_POWER          "hdmitx_extcon_power"
#define HDMI_UEVENT_HDMI_AUDIO          "hdmitx_extcon_audio"

#define H265_DOUBLE_WRITE_MODE          "/sys/module/vh265/parameters/double_write_mode"

#define FULL_WIDTH_480                  720
#define FULL_HEIGHT_480                 480
#define FULL_WIDTH_576                  720
#define FULL_HEIGHT_576                 576
#define FULL_WIDTH_720                  1280
#define FULL_HEIGHT_720                 720
#define FULL_WIDTH_768                  1366
#define FULL_HEIGHT_768                 768
#define FULL_WIDTH_1080                 1920
#define FULL_HEIGHT_1080                1080
#define FULL_WIDTH_4K2K                 3840
#define FULL_HEIGHT_4K2K                2160
#define FULL_WIDTH_4K2KSMPTE            4096
#define FULL_HEIGHT_4K2KSMPTE           2160

#define MODE_480I                       "480i60hz"
#define MODE_480P                       "480p60hz"
#define MODE_480CVBS                    "480cvbs"
#define MODE_576I                       "576i50hz"
#define MODE_576P                       "576p50hz"
#define MODE_576CVBS                    "576cvbs"
#define MODE_720P50HZ                   "720p50hz"
#define MODE_720P                       "720p60hz"
#define MODE_768P                       "768p60hz"
#define MODE_1080P24HZ                  "1080p24hz"
#define MODE_1080I50HZ                  "1080i50hz"
#define MODE_1080P50HZ                  "1080p50hz"
#define MODE_1080I                      "1080i60hz"
#define MODE_1080P                      "1080p60hz"
#define MODE_4K2K24HZ                   "2160p24hz"
#define MODE_4K2K25HZ                   "2160p25hz"
#define MODE_4K2K30HZ                   "2160p30hz"
#define MODE_4K2K50HZ                   "2160p50hz"
#define MODE_4K2K60HZ                   "2160p60hz"
#define MODE_4K2KSMPTE                  "smpte24hz"
#define MODE_4K2KSMPTE30HZ              "smpte30hz"
#define MODE_4K2KSMPTE50HZ              "smpte50hz"
#define MODE_4K2KSMPTE60HZ              "smpte60hz"

#define MODE_480I_PREFIX                "480i"
#define MODE_480P_PREFIX                "480p"
#define MODE_576I_PREFIX                "576i"
#define MODE_576P_PREFIX                "576p"
#define MODE_720P_PREFIX                "720p"
#define MODE_768P_PREFIX                "768p"
#define MODE_1080I_PREFIX               "1080i"
#define MODE_1080P_PREFIX               "1080p"
#define MODE_4K2K_PREFIX                "2160p"
#define MODE_4K2KSMPTE_PREFIX           "smpte"

#define HDR_MODE_OFF                    "0"
#define HDR_MODE_ON                     "1"
#define HDR_MODE_AUTO                   "2"

#define PROP_SUPPORT_4K                 "HDMI_4K_SUPPORT"
#define SDR_MODE_OFF                    "0"
#define SDR_MODE_AUTO                   "2"

#define UBOOTENV_DIGITAUDIO             "ubootenv.var.digitaudiooutput"
#define UBOOTENV_HDCPRXVER              "ubootenv.var.hdcprxver"
#define UBOOTENV_HDMIMODE               "ubootenv.var.hdmimode"
#define UBOOTENV_HDCPMODE               "ubootenv.var.hdcpmode"
#define UBOOTENV_TESTMODE               "ubootenv.var.testmode"
#define UBOOTENV_CVBSMODE               "ubootenv.var.cvbsmode"
#define UBOOTENV_OUTPUTMODE             "ubootenv.var.outputmode"
#define UBOOTENV_ISBESTMODE             "ubootenv.var.is.bestmode"
#define UBOOTENV_EDIDCRCVALUE           "ubootenv.var.edid.crcvalue"
#define UBOOTENV_DISPLAYPERCENT         "ubootenv.var.displaypercent"

enum {
    DISPLAY_MODE_480I                   = 0,
    DISPLAY_MODE_480P                   = 1,
    DISPLAY_MODE_480CVBS                = 2,
    DISPLAY_MODE_576I                   = 3,
    DISPLAY_MODE_576P                   = 4,
    DISPLAY_MODE_576CVBS                = 5,
    DISPLAY_MODE_720P50HZ               = 6,
    DISPLAY_MODE_720P                   = 7,
    DISPLAY_MODE_1080P24HZ              = 8,
    DISPLAY_MODE_1080I50HZ              = 9,
    DISPLAY_MODE_1080P50HZ              = 10,
    DISPLAY_MODE_1080I                  = 11,
    DISPLAY_MODE_1080P                  = 12,
    DISPLAY_MODE_4K2K24HZ               = 13,
    DISPLAY_MODE_4K2K25HZ               = 14,
    DISPLAY_MODE_4K2K30HZ               = 15,
    DISPLAY_MODE_4K2K50HZ               = 16,
    DISPLAY_MODE_4K2K60HZ               = 17,
    DISPLAY_MODE_4K2KSMPTE              = 18,
    DISPLAY_MODE_4K2KSMPTE30HZ          = 19,
    DISPLAY_MODE_4K2KSMPTE50HZ          = 20,
    DISPLAY_MODE_4K2KSMPTE60HZ          = 21,
    DISPLAY_MODE_TOTAL                  = 22
};

enum output_mode_state {
    OUTPUT_MODE_STATE_INIT              = 0,
    OUTPUT_MODE_STATE_POWER             = 1,//hot plug
    OUTPUT_MODE_STATE_SWITCH            = 2,//user switch the mode
    OUTPUT_MODE_STATE_SWITCH_ADAPTER    = 3,//video auto switch the mode
    OUTPUT_MODE_STATE_RESERVE           = 4,
    OUTPUT_MODE_STATE_ADAPTER_END       = 5 //end hint video auto switch the mode
};

enum hdmi_sink_type {
    HDMI_SINK_TYPE_NONE                 = 0,
    HDMI_SINK_TYPE_SINK                 = 1,
    HDMI_SINK_TYPE_REPEATER             = 2,
    HDMI_SINK_TYPE_RESERVE              = 3
};

typedef struct hdmi_data {
    char edid[MAX_STR_LEN];
    int sink_type;
    char current_mode[MODE_LEN];
    char ubootenv_hdmimode[MODE_LEN];
}hdmi_data_t;

typedef struct resolution {
//       resolution       standard frequency deepcolor
//          2160             p       50hz      420   //2160p50hz420
//          1080             p       60hz        0   //1080p60hz
//0x00 0000 0000 0000 0000 | 0 | 0000 0000 | 0 0000 0000 //resolution_num
    int resolution;
    char standard;
    int frequency;
    int deepcolor;
    int64_t resolution_num;
} resolution_t;

class DisplayMode : public TxUEventCallback,
                                private FrameRateAutoAdaption::Callbak {
public:
    DisplayMode(const char *path);
    ~DisplayMode();

    virtual void onTxEvent(char *hpdState, int outputState);
    virtual void onDispModeSyncEvent (const char* outputmode, int state);

    void init();

    int64_t resolveResolutionValue(const char *mode);
    void setSourceColorFormat(const char* colormode);
    void setSourceHdcpMode(const char* hdcpmode);
    void setSourceHdcpMode(const char* hdcpmode, output_mode_state state);
    void setSourceOutputMode(const char* outputmode);
    void setSourceOutputMode(const char* outputmode, output_mode_state state);
    void setSourceDisplay(output_mode_state state);
    void getHdmiData(hdmi_data_t *data);
    void setDigitalMode(const char* mode);
    void setOsdMouse(const char* curMode);
    void setOsdMouse(int x, int y, int w, int h);
    void setRecWindowVideoAxis();
    void getPosition(const char* curMode, int *position);
    void setPosition(int left, int top, int width, int height);
    void zoomByPercent(int percent);
    void zoomIn();
    void zoomOut();
    bool playVideoDetect();
    bool isHDCPTxAuthSuccess();
private:
    bool getBootEnv(const char *key, char *value);
    void setBootEnv(const char *key, const char *value);
    int getBootenvInt(const char* key, int defaultVal);
    int parseConfigFile();
    void getHighestHdmiMode(char* mode, hdmi_data_t* data);
    void filterHdmiMode(char * mode, hdmi_data_t* data);
    void getHdmiOutputMode(char *mode, hdmi_data_t* data);
    bool isBestOutputmode();
    void updateDeepColor(bool cvbsMode, output_mode_state state, const char* outputmode);
    void saveDeepColorAttr(const char* mode, const char* dcValue);
    //void getDeepColorAttr(const char* mode, char *value);
    void resolveResolution(const char *mode, resolution_t* resol_t);
    void updateDefaultUI();
    void setAutoSwitchFrameRate(int state);
    void updateFreeScaleAxis();
    void updateWindowAxis(const char* outputmode);
    void dumpCap(const char * path, const char * hint, char *result);
    void dumpCaps(char *result=NULL);
    void zoom(int step);

    const char* pConfigPath;
    char mSocType[64];
    char mDefaultUI[64];//this used for mbox
    int mDisplayWidth;
    int mDisplayHeight;
    SysWrite *pSysWrite = NULL;
    FrameRateAutoAdaption *pFrameRateAutoAdaption = NULL;

    HDCPTxAuth *pTxAuth = NULL;
};

#if defined(AML_OSD_USE_DRM)
#include <stdio.h>

static void DisplaySetWindowAxis(int x, int y, int w, int h) {
    char drm_client_cmd[128] = {0};
    snprintf(drm_client_cmd, 128, "drm-helper-client -S ui_rect %d %d %d %d", x, y, w, h);
    system(drm_client_cmd);
}

static void DisplaySetDisplayMode(const char* mode) {
    char drm_client_cmd[128] = {0};
    snprintf(drm_client_cmd, 128, "drm-helper-client -S display_mode %s", mode);
    system(drm_client_cmd);
}
static bool DisplayGetDisplayMode(char* mode, int len) {
    char drm_client_cmd[128] = {0};
    char buffer[128] = {0};
    FILE* f;
    bool ret = false;
    snprintf(drm_client_cmd, 128, "drm-helper-client -G display_mode" );
    f = popen(drm_client_cmd, "r");
    if (f != NULL) {
         fread(buffer, sizeof(char), 128, f);
         if (feof(f)) {
             char* delim = strchr(buffer, ',');
             if (delim != NULL) {
                 *delim = 0;
             }
             strncpy(mode, buffer, len);
             ret = true;
         }
         fclose(f);
    }
    return ret;
}
#endif


#endif
