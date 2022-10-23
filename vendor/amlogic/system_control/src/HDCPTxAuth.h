#ifndef HDCP_TX_AUTH_H
#define HDCP_TX_AUTH_H

#include <pthread.h>
#include <semaphore.h>
#include "TxUEventCallback.h"
#include "SysWrite.h"
#include "FrameRateAutoAdaption.h"

enum {
    REPEATER_RX_VERSION_NONE        = 0,
    REPEATER_RX_VERSION_14          = 1,
    REPEATER_RX_VERSION_22          = 2
};

class HDCPTxAuth {
public:
    HDCPTxAuth();
    ~HDCPTxAuth();

    void setRepeaterRxVersion(int ver);
    void setUEventCallback(TxUEventCallback *cb);
    void setFRAutoAdpt (FrameRateAutoAdaption *mFRAutoAdpt);
    int start();
    int stop();
    void stopVerAll();
    void startVer22();
    void startVer14();

private:
    bool getBootEnv(const char *key, char *value);
    void setBootEnv(const char *key, const char *value);
    bool authInit(bool *pHdcp22, bool *pHdcp14);
    bool authLoop(bool useHdcp22, bool useHdcp14);
    void stopVer22();
    void mute(bool mute);

    static void *TxUEventThread(void *data);
    static void* authThread(void* data);

    TxUEventCallback *mUEventCallback;
    FrameRateAutoAdaption *mFRAutoAdpt;
    sem_t pthreadTxSem;
    SysWrite mSysWrite;
    int mRepeaterRxVer;
    bool mMute;

    pthread_mutex_t mTxMutex;
    pthread_t pthreadIdHdcpTx;
    bool mExitHdcpTxThread;
};

#endif
