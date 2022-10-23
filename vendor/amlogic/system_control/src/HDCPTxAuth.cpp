#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

#include "UEventObserver.h"
#include "DisplayMode.h"
#include "HDCPTxAuth.h"
#include "ubootenv.h"

#define LOG_TAG "systemcontrol"

HDCPTxAuth::HDCPTxAuth() :
    mRepeaterRxVer(REPEATER_RX_VERSION_NONE),
    mUEventCallback(NULL),
    mMute(false),
    pthreadIdHdcpTx(0) {

    syslog(LOG_INFO, "HDCPTxAuth::HDCPTxAuth");
    if (sem_init(&pthreadTxSem, 0, 0) < 0) {
        syslog(LOG_ERR, "HDCPTxAuth sem_init fail");
        exit(0);
    }

    pthread_t id;
    int ret = pthread_create(&id, NULL, TxUEventThread, this);
    if (ret != 0) {
        syslog(LOG_ERR, "create TxUeventThread failed!\n");
    }
}

HDCPTxAuth::~HDCPTxAuth() {
    syslog(LOG_INFO, "HDCPTxAuth::~HDCPTxAuth");
    sem_destroy(&pthreadTxSem);
}

void HDCPTxAuth::setRepeaterRxVersion(int ver) {
    syslog(LOG_INFO, "HDCPTxAuth setRepeaterRxVersion");
    mRepeaterRxVer = ver;
}
void HDCPTxAuth::setUEventCallback(TxUEventCallback *cb) {
    syslog(LOG_INFO, "HDCPTxAuth setUEventCallback");
    mUEventCallback = cb;
}
bool HDCPTxAuth::getBootEnv(const char* key, char* value) {
    syslog(LOG_INFO, "HDCPTxAuth::getBootEnv");
    const char* val = bootenv_get(key);

    if (val) {
        strcpy(value, val);
        return true;
    }
    return false;
}

void HDCPTxAuth::setBootEnv(const char* key, const char* value) {
    syslog(LOG_INFO, "HDCPTxAuth::setBootEnv");
    bootenv_update(key, value);
}

void HDCPTxAuth::setFRAutoAdpt(FrameRateAutoAdaption *mFRAutoAdpt) {
    this->mFRAutoAdpt = mFRAutoAdpt;
}

//start HDCP TX authenticate
int HDCPTxAuth::start() {
    int ret;
    pthread_t thread_id;

    syslog(LOG_INFO, "HDCPTxAuth start");
    if (pthread_mutex_trylock(&mTxMutex) == EDEADLK) {
        syslog(LOG_ERR, "HDCPTxAuth mutex is deadlock\n");
        return -1;
    }

    mExitHdcpTxThread = false;
    ret = pthread_create(&thread_id, NULL, authThread, this);
    if (ret != 0)
        syslog(LOG_ERR, "HDCPTxAuth create thread fail\n");

    ret = sem_wait(&pthreadTxSem);
    if (ret < 0)
        syslog(LOG_ERR, "HDCPTxAuth sem_wait fail\n");

    pthreadIdHdcpTx = thread_id;
    pthread_mutex_unlock(&mTxMutex);
    syslog(LOG_INFO, "HDCPTxAuth create hdcp thread id (%lu) done\n", thread_id);
    return 0;
}

//stop HDCP TX authenticate
int HDCPTxAuth::stop() {
    void *threadResult;
    int ret = -1;

    syslog(LOG_INFO, "HDCPTxAuth stop pthreadIdHdcpTx:%lu\n", pthreadIdHdcpTx);
    stopVerAll();
    if (0 != pthreadIdHdcpTx) {
        mExitHdcpTxThread = true;
        if (pthread_mutex_trylock(&mTxMutex) == EDEADLK) {
            syslog(LOG_ERR, "HDCPTxAuth exit thread, mutex is deadlock\n");
            return ret;
        }

        if (0 != pthread_join(pthreadIdHdcpTx, &threadResult)) {
            syslog(LOG_ERR, "HDCPTxAuth exit thread fail\n");
            return ret;
        }

        pthread_mutex_unlock(&mTxMutex);
        syslog(LOG_INFO, "HDCPTxAuth pthread exit id = (%lu) %s", pthreadIdHdcpTx, (char *)threadResult);
        pthreadIdHdcpTx = 0;
        ret = 0;
    }

    return ret;
}

void HDCPTxAuth::mute(bool mute) {
    syslog(LOG_INFO, "HDCPTxAuth mute entry");
    char hdcpTxKey[MODE_LEN] = {0};
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_KEY, hdcpTxKey);
    if ((strlen(hdcpTxKey) == 0) || !(strcmp(hdcpTxKey, "00")))
        return;

    if (mute != mMute) {
        mSysWrite.writeSysfs(DISPLAY_HDMI_VIDEO_MUTE, mute ? "1" : "0");
        mMute = mute;
        syslog(LOG_INFO, "HDCPTxAuth hdcp_tx mute %s", mute ? "on" : "off");
    }
}

void HDCPTxAuth::stopVer22() {
    syslog(LOG_INFO, "HDCPTxAuth stopVer22\n");
    int status;
    status = system("hdcpcontrol.sh stop");
    if (status < 0) {
        syslog(LOG_ERR, "HDCPTxAuth stopVer22: stop hdcp_tx 2.2 failed:%s\n", strerror(errno));
    }
}

void HDCPTxAuth::stopVerAll() {
    char hdcpRxVer[MODE_LEN] = {0};
    //stop hdcp_tx 2.2 & 1.4
    syslog(LOG_INFO, "HDCPTxAuth stopVerAll: stop 2.2 & 1.4 hdcp pwr\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
    usleep(20000);
    //mSysWrite.setProperty("ctl.stop", "hdcp_tx22");
    stopVer22();
    usleep(20000);
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_POWER, hdcpRxVer);

    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_CONF, DISPLAY_HDMI_HDCP14_STOP);
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_CONF, DISPLAY_HDMI_HDCP22_STOP);
    usleep(2000);
}

void* HDCPTxAuth::authThread(void* data) {
    bool hdcp22 = false;
    bool hdcp14 = false;
    HDCPTxAuth *pThiz = (HDCPTxAuth*)data;

    syslog(LOG_INFO, "HDCPTxAuth auththread entry\n");
    sem_post(&pThiz->pthreadTxSem);

    if (pThiz->authInit(&hdcp22, &hdcp14)) {
        if (!pThiz->authLoop(hdcp22, hdcp14)) {
            syslog(LOG_ERR, "HDCPTxAuth HDCP authenticate fail,need black screen and disable audio\n");
            //pThiz->mSysWrite.writeSysfs(AV_HDMI_CONFIG, "audio_off");
            return NULL;
        }
        pThiz->mSysWrite.writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_AUTO_ENABLE);
        pThiz->mSysWrite.writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
    } else {
        pThiz->mSysWrite.writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_AUTO_ENABLE);
    }
    return NULL;
}

bool HDCPTxAuth::authInit(bool *pHdcp22, bool *pHdcp14) {
    bool useHdcp22 = false;
    bool useHdcp14 = false;
    char hdcpRxVer[MODE_LEN] = {0};
    char hdcpTxKey[MODE_LEN] = {0};

    syslog(LOG_INFO, "HDCPTxAuth authInit entry\n");
    //in general, MBOX is TX device, need to detect its TX keys.
    //            TV   is RX device, need to detect its RX keys.
    //HDCP TX: get current MBOX[TX] device contains which TX keys. Values:[14/22, 00 is no key]
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_KEY, hdcpTxKey);
    syslog(LOG_INFO, "HDCPTxAuth hdcp_tx key:%s\n", hdcpTxKey);
    if ((strlen(hdcpTxKey) == 0) || !(strcmp(hdcpTxKey, "00")))
        return false;

    //HDCP RX: get currtent TV[RX] device contains which RX key. Values:[14/22, 00 is no key]
    //Values is the hightest key. if value is 22, means the devices supports 22 and 14.
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_VER, hdcpRxVer);
    syslog(LOG_INFO, "HDCPTxAuth hdcp_rx support hdcp version:%s\n", hdcpRxVer);
    if ((strlen(hdcpRxVer) == 0) || !(strcmp(hdcpRxVer, "00")))
        return false;

    //stop hdcp_tx
    stopVerAll();

    if (REPEATER_RX_VERSION_22 == mRepeaterRxVer) {
        syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 2.2 supported for RxSupportHdcp2.2Auth\n");
        useHdcp22 = true;
    } else if ((strstr(hdcpRxVer, (char *)"22") != NULL) && (strstr(hdcpTxKey, (char *)"22") != NULL)) {
        syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 2.2 supported\n");
        useHdcp22 = true;
    }

    if (REPEATER_RX_VERSION_14 == mRepeaterRxVer) {
        syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 1.4 supported for RxSupportHdcp1.4Auth\n");
        useHdcp14 = true;
    } else if (!useHdcp22 && (strstr(hdcpRxVer, (char *)"14") != NULL) && (strstr(hdcpTxKey, (char *)"14") != NULL)) {
        useHdcp14 = true;
        syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 1.4 supported\n");
    }

    if (!useHdcp22 && !useHdcp14) {
        //do not support hdcp1.4 and hdcp2.2
        syslog(LOG_ERR, "HDCPTxAuth device do not support hdcp1.4 or hdcp2.2\n");
        return false;
    }

    //start hdcp_tx
    if (useHdcp22) {
        startVer22();
    } else if (useHdcp14) {
        startVer14();
    }
    *pHdcp22 = useHdcp22;
    *pHdcp14 = useHdcp14;
    return true;
}

bool HDCPTxAuth::authLoop(bool useHdcp22, bool useHdcp14) {
    bool success = false;
    int count = 0;
    syslog(LOG_INFO, "HDCPTxAuth authLoop: begin to authenticate hdcp22:%d, hdcp14:%d, mExitHdcpTxThread:%d\n", useHdcp22, useHdcp14, mExitHdcpTxThread);
    while (!mExitHdcpTxThread) {
        usleep(200*1000);//sleep 200ms

        char auth[MODE_LEN] = {0};
        mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_AUTH, auth);
        if (strstr(auth, (char *)"1")) {//Authenticate is OK
            success = true;
            mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
            break;
        }

        count++;
        if (count > 40) { //max 200msx40 = 8s it will authenticate completely
            if (useHdcp22) {
                syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 2.2 authenticate fail for 8s timeout, change to hdcp_tx 1.4 authenticate\n");
                count = 0;
                useHdcp22 = false;
                useHdcp14 = true;
                stopVerAll();
                //if support hdcp22, must support hdcp14
                startVer14();
                continue;
            } else if (useHdcp14) {
                syslog(LOG_INFO, "HDCPTxAuth hdcp_tx 1.4 authenticate fail, 8s timeout\n");
                startVer14();
            }
            mSysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
            break;
        }
    }

    char hdcpmode[MODE_LEN] = {0};
    char hdcpRxVer[MODE_LEN] = {0};
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_MODE, hdcpmode);
    mSysWrite.readSysfs(DISPLAY_HDMI_HDCP_VER, hdcpRxVer);
    setBootEnv(UBOOTENV_HDCPMODE, hdcpmode);
    //if TV only support hdcp1.4
    if ((strstr(hdcpRxVer, (char *)"22") == NULL) && (strstr(hdcpRxVer, (char *)"14") != NULL)) {
        setBootEnv(UBOOTENV_HDCPRXVER, "14");
    }
    syslog(LOG_INFO, "HDCPTxAuth hdcp_tx authenticate success: %d\n", success?1:0);
    return success;
}

void HDCPTxAuth::startVer22() {
    //start hdcp_tx 2.2
    syslog(LOG_INFO, "HDCPTxAuth startVer22: start hdcp_tx 2.2\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_MODE, DISPLAY_HDMI_HDCP_22);
    usleep(50*1000);

    int status;
    status = system("hdcpcontrol.sh start");
    if (status < 0) {
        syslog(LOG_ERR, "HDCPTxAuth startVer22: start hdcp_tx 2.2 failed:%s\n", strerror(errno));
    }
}

void HDCPTxAuth::startVer14() {
    //start hdcp_tx 1.4
    syslog(LOG_INFO, "HDCPTxAuth startVer14: start hdcp_tx 1.4\n");
    mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_MODE, DISPLAY_HDMI_HDCP_14);
}

void *HDCPTxAuth::TxUEventThread(void *data) {
    syslog(LOG_INFO, "HDCPTxAuth TxUEventThread");
    HDCPTxAuth *pThiz = (HDCPTxAuth *)data;
    UEventData ueventData;
    memset(&ueventData, 0, sizeof(UEventData));

    UEventObserver ueventObserver;
    ueventObserver.addMatch(HDMI_TX_PLUG_UEVENT);
    ueventObserver.addMatch(HDMI_TX_POWER_UEVENT);
    ueventObserver.addMatch(HDMI_TX_HDCP_UEVENT);

    while (1) {
        ueventObserver.waitForNextEvent(&ueventData);
        syslog(LOG_INFO, "HDCP TX match_name(%s) switch_name(%s) switch_state(%s)\n", ueventData.matchName, ueventData.switchName, ueventData.switchState);

        if (!strcmp(ueventData.matchName, HDMI_TX_PLUG_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI) && (NULL != pThiz->mUEventCallback)) {
            pThiz->mUEventCallback->onTxEvent(ueventData.switchState, OUTPUT_MODE_STATE_POWER);
        } else if (!strcmp(ueventData.matchName, HDMI_TX_POWER_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDMI_POWER)) {
            //0: hdmi suspend  1: hdmi resume
            if (!strcmp(ueventData.switchState, HDMI_TX_RESUME) && (NULL != pThiz->mUEventCallback)) {
                pThiz->mUEventCallback->onTxEvent(ueventData.switchState, OUTPUT_MODE_STATE_POWER);
            } else if (!strcmp(ueventData.switchState, HDMI_TX_SUSPEND)) {
                syslog(LOG_INFO, "HDCP switchState = HDMI_TX_SUSUPEND\n");
                pThiz->mSysWrite.writeSysfs(DISPLAY_HDMI_HDCP_POWER, "1");
            }
        } else if (!strcmp(ueventData.matchName, HDMI_TX_HDCP_UEVENT) && !strcmp(ueventData.switchName, HDMI_UEVENT_HDCP)) {
        }
    }

    return NULL;
}
