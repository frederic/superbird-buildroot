#ifndef BSA_H
#define BSA_H

#include <QWidget>
#include <QtWidgets/QTableWidget>
#include <QFile>
#include <QTimer>
#include <QListWidgetItem>
#include <QReadWriteLock>

extern "C"
{
#include "bt_types.h"
#include "data_types.h"
#include "bsa_disc_api.h"
#include "bsa_pbc_api.h"
#include "bsa_mce_api.h"
#include "bta_av_api.h"
#include "uipc.h"
#include "bsa_avk_api.h"
#include "app_avk.h"

}

class PBItem;
class CLCCItem;
class CNUMItem;
class COPSItem;

namespace Ui {
    class BSA;
    class Bsa_Device;
}

enum HFState
{
    eHF_INVALID_STATE=0,
    eHF_DISCONNECTED=1,
    eHF_CONNECTED=2,
    eHF_FIRST_INCOMING_CALL=3,
    eHF_FIRST_CALL_SETUP=4,
    eHF_ON_CALL=5,
    eHF_ON_CALL_ANOTHER_CALL_SETUP=6,
    eHF_ON_CALL_INCOMING_CALL=7
};

// Device class
class Bsa_Device
{
public:
    tBSA_DISC_NEW_MSG msg;
};

typedef struct
{
    QString handleStr;
    QString msginfo;
} tBSA_MSG_LIST_ITEM;


class AppAttrVal
{
public:
    tAVRC_LIST_APP_VALUES_RSP val;
};

// Media element
class AppAttr
{
public:
    UINT8       attr_id;
    QList<AppAttrVal> m_vals;
};

class AvrcItem
{
public:
    AvrcItem() {}
    ~AvrcItem() {}
    tBSA_AVRC_ITEM item;
};

class MediaPlayer
{
public:
    MediaPlayer() {m_bIsBrowsed = FALSE;}
    ~MediaPlayer() {}
    QList<AvrcItem> m_listFolderBrowseList;
    tBSA_AVRC_ITEM_PLAYER m_Player;
    BOOLEAN m_bIsBrowsed;
};

class MapInstance
{
public:
    int m_iInstId;
    int m_iMsgType;
    bool m_bMceRegNotif;
};

// US signal class
class UISignal : public QObject
{
    Q_OBJECT

public:
    UISignal() {}
    ~UISignal() {}

signals:
    void SignalPairingRequest(QString st);
    void SignalDiscovery();
    void SignalPBRead();
    void SignalCLCCRead();
    void SignalCNUMRead();
    void SignalCOPSRead();
    void UpdatePBAPDataDisplay(QString st, QVariant var);
    void ShowPhonebookData();
    void UpdateMceDataDisplay(QVariant event, QString st, QVariant var);
    void ShowMceData(QString st);
    void DisplayStatus(QString st);
    void UpdateMediaAttr();
    void UpdateProgress(QVariant len, QVariant pos);
    void DisplayMessageBox(QString st);
    void AvrcpStateChanged(QVariant bConnected);
    void HSStateConnected(QVariant bConnected);
    void PhoneBookDownload();
    void MceUserStateChanged(QVariant bConnected);
    void AvkClose(QVariant var);
    void AvkVolumeChanged();
};

// App class
class BSA : public QWidget
{
    Q_OBJECT

public:
    explicit BSA(QWidget *parent = 0);
    ~BSA();

    Ui::BSA *ui;

    UISignal *m_uiSignal;

    /*************************************************************************************/
    /*************** Device Mgr UI ************************************************************/
    /*************************************************************************************/
public:
    void DefaultVisibility();
    int app_mgr_sec_bond(BD_ADDR bd_addr);
    int remove_device(BD_ADDR bd_addr);
    void GetPairedDevices();
    void PopulateTables(Bsa_Device *device, QTableWidget *table, QString table_type);
    int isInRemovedList(UINT8 * addr);

    QList<Bsa_Device> m_disc_device_list;
    int m_disc_row_number;
    QString strBsaVersion;

    QList<Bsa_Device> m_paired_device_list;
    int m_paired_row_number;

    QList<Bsa_Device> m_removed_device_list;

    BOOLEAN IsAnyDeviceConnected(int profile);

    enum eRole
    {
        eNone,
        eCarkit,
        ePhone,
        eDual
    };

    eRole m_eRole;

    void SetRole(eRole role);
    void SetPhoneRole(BOOLEAN bOn);
    void SetCarkitRole(BOOLEAN bOn);

    UINT8* GetPairedDeviceName(BD_ADDR bd_addr);

private slots:
    void on_addDevice_clicked();

    void on_btnSearch_clicked();

    void on_btnPair_clicked();

    void on_btnPhone_clicked();

    void on_btnMusic_clicked();

    void on_btnBle_clicked();

    void on_btnPhonebook_clicked();

    void on_btnRemove_clicked();

    void on_btnReject_clicked();

    void on_tblPairedDevices_clicked(const QModelIndex &index);

    void on_BSA_pair_request(QString st);

    void on_BSA_Discovery();


    void on_radioButtonCarkit_clicked();
    void on_radioButtonPhone_clicked();
    void on_radioButtonDual_clicked();

    void on_timer_connectAVK();
    void on_timer_connectPbap();

    void on_btnTrace_clicked();

    signals:
        void RunInUiThread(void * fp, void * data,unsigned int evt);

    /*************************************************************************************/
    /*************** Music UI ************************************************************/
    /*************************************************************************************/
public:
    void on_BSA_show_message_box(QString st);

    void AvkStateChanged(BOOLEAN bConnected, BD_ADDR *pBda = NULL);
    void AvkPlayStateChanged(UINT8 state);
    void AudioStateChanged(bool bConnected);
    void ShowBrowseUI();
    void ShowMetaPlayerUI();
    BOOLEAN GetBrowsedMediaPlayer(int &iIndexPlayer, tAPP_AVK_CONNECTION * conn);

    tAPP_AVK_CONNECTION * IsActiveAconnection(UINT8 rc_handle);
    tAPP_AVK_CONNECTION * GetActiveConnection();

    tBSA_AVK_GET_ELEMENT_ATTR_MSG m_listMediaElements;
    QList<AppAttr> m_listAppAttr;
    UINT8         m_indexAppAttr;
    QList<MediaPlayer> m_listMediaPlayers;
    // Current UI browse content
    QList<tBSA_AVRC_ITEM> m_listFileFolder;

    UINT8 m_uPlayState;
    BOOLEAN m_bPausedByApp;
    int m_iIndexActiveConnection;

public slots:
        // Music
        void on_Play_clicked();

        void on_PauseButton_clicked();

        void on_stopButton_clicked();

        void on_nextButton_clicked();

        void on_PrevButton_clicked();

        void on_muteButton_clicked();

        void on_connectAVKButton_clicked();

        void on_disconnectButton_clicked();

        void on_volupButton_clicked();

        void on_voldownButton_clicked();

        void on_update_MediaAttr();

        void on_cbEqualizer_currentIndexChanged(int index);

        void on_cbRepeat_currentIndexChanged(int index);

        void on_cbShuffle_currentIndexChanged(int index);

        void on_cbScan_currentIndexChanged(int index);

        void on_BSA_update_progress(QVariant len, QVariant pos);

        void on_AvrcpStateChanged(QVariant bConnected);

        void on_AvkClose(QVariant var);

        void on_avk_volume_changed();

        void on_btnBack_clicked();

        void on_btnNowPlaying_clicked();

        void on_tblFileFolders_doubleClicked(const QModelIndex &index);

        void on_DisconnectAVRCP_clicked();

        void on_cbAVKDevices_currentIndexChanged(int index);

        void onRunInUiThread(void * fp, void * data, unsigned int event);
    /*************************************************************************************/
    /*************** HF UI ************************************************************/
    /*************************************************************************************/
public:
    //HF related functions
    void SendCLCCCommand();

    // Button and state changes

    void HsStateChanged(bool bConnected);
    HFState GetCurrentHFState();
    void SetCurrentHFState(HFState CurVal);
    void SetupButtonsAndState(HFState eState);

    void EnableDisableDTMFKPButtons(bool bEnable);
    void EnableDisableReadATCmdsButton(bool bEnable);
    void EnableDisableCallButtons(bool bEnable);
    void EnableDisablePickupRejButton(bool bEnable);
    void EnableDisableSliders(bool bEnable);
    void EnableDisableVRButtons(bool bEnable);
    void EnableDisableHoldRelButtons(bool bEnable);
    void EnableDisableHFPrivacyButtons(bool bEnable);

    void LockUnlockSetCLCCRespVar(bool bValue);
    bool LockUnlockGetCLCCRespVar();
    void LockUnlockSetCLCCTotCmdVar(bool bValue);
    bool LockUnlockGetCLCCTotCmdVar();

    void DefaultVars();

    // Incoming events
    void HandleRingnCallWaitEvent(bool bCallWaiting, QString strNum);

    //Completed events for Commands sent
    void HandleDialComplEvent(char *number);
    void HandleRedialComplEvent(char *number);

    void HandlePickupnHangupComplEvent(bool bPickup);
    void HandleVRStateChangeEvent(bool bState);
    void HandleReleasenHoldComplEvent(bool bHold);

    // Set button states
    void SetConnDiscHSButtons(bool bToggle, bool bEnableOne, bool bEnableConn);
    void SetHFPrivacyButtons(bool bToggle, bool bEnableOne, bool bEnableHF);
    void SetStartStopVRButtons(bool bToggle, bool bEnableOne, bool bEnableStart);
    void SetHoldRelButtons(bool bToggle, bool bEnableOne, bool bEnableHold);
    void SetPickupHangupRejectButtons(bool bToggle, bool bEnableOne, bool bUseCLCC, bool bEnablePickup);

    UINT8* GetConnectedHFDevice(BD_ADDR &bd_addr, BOOLEAN &bConnected);

protected:
    void PopulatePBTables(int nTblIndex, PBItem *pItem, QString table_type);
    void PopulateCLCCTables(int nTblIndex, CLCCItem *pItem, QString table_type);
    void PopulateCNUMTables(int nTblIndex, CNUMItem *pItem, QString table_type);
    void PopulateCOPSTables(int nTblIndex, COPSItem *pItem, QString table_type);

    void HoldUnHold(bool bSwap);
    void HandleCLCCResponse(char *cResp);
    void HandleCOPSResponse(char *cResp);
    void HandleCNUMResponse(char *cResp);

    void HFButtondefaults();
    void ResetPhoneRingLblState();

public:
    bool m_bDialCmdSent;
    bool m_bLastNumDialCmdSent;

    bool m_bAnswerCmdSent;
    bool m_bCHUPCmdSent;

    bool m_bMuteCmdSent;
    bool m_bUnmuteCmdSent;

    bool m_bStartVRCmdSent;
    bool m_bStopVRCmdSent;

    bool m_bCLCCCmdSentForTotal;
    bool m_bCLCCCmdTotRespObt;

    int m_nCallHeldIndValue;

    // Timer for reading indicator details
    QTimer *m_pHFStartTimer;
    QTimer *m_pSendIndTimer;
    QTimer *m_pReadIndTimer;

    bool m_bAutoConnect;
    QTimer *m_pAvkConnectTimer;
    QTimer *m_pPbapConnectTimer;

    QTimer *m_pAvkRcRetryConnectTimer;
    int m_iRetryCnt;

private:
    QReadWriteLock m_CLCCRespLock;
    QReadWriteLock m_CLCCTotCmdLock;
    QReadWriteLock m_HFStateLock;

    //Slider signals
signals:
       void valueChanged(int value);
       void sliderReleased();

private:


//Slider for setting up volume settings (Speaker and Microphone)
        QSlider *m_pSpkSlider;
        QSlider *m_pMicSlider;

// Internal storage values
        int m_nLastSpkVol;
        int m_nLastMicVol;
        HFState m_eHFStateValue;

private slots:
        // Phone UI
        void on_btnStartVR_clicked();

        void on_btnStopVR_clicked();

        void on_btnReadPB_clicked();

        void on_btnConnectHS_clicked();

        void on_btnDisconnectHS_clicked();

        void on_btnPickup_clicked();

        void on_btnHangup_clicked();

        void on_btnHold_clicked();

        void on_btnReleaseHold_clicked();

        void on_btnLastNumDial_clicked();

        void on_btnCall_clicked();

        void on_btnHF_clicked();

        void on_btnCLCC_clicked();

        void on_btnPrivacy_clicked();

        void on_btnCOPS_clicked();

        void on_btnCNUM_clicked();

        void on_btnKeypress_clicked();

        void on_btnDTMF_clicked();

        void on_SliderSpkReleased();

        void on_SliderMicReleased();

        void on_SliderSpkVolPosition_changed(int volume);

        void on_SliderMicVolPosition_changed(int volume);

        void on_HandlePBRead();

        void on_HandleCLCCRead();

        void on_HandleCNUMRead();

        void on_HandleCOPSRead();

        void on_HFStartTimer();

        void on_Indicator_CmdSend();

        void on_Indicator_Update();

        void on_Timer_Timeout();

        void on_btnSwapCalls_clicked();

        void on_HSStateChange(QVariant bUp);

        void on_cbAutoConnect_clicked();


    /*************************************************************************************/
    /*************** Phone book UI ************************************************************/
    /*************************************************************************************/
public:
    void PbcStateChanged(BOOLEAN bConnected);

    void ProcessPBAPListData(const char* filename);

    void GetPBCVCardList(int idxListType);

    void SetNextPath();

    void InitializePBAPSettings();

    QStringList m_listPath;

private slots:
        void on_btnGetPhonebook_clicked();

        void on_btnListVCards_clicked();

        void on_btnSetPath_clicked();

        void on_btnPBAPConnectDisconnect_clicked();

        void on_BSA_error(QString st);

        void on_BSA_pbap_data_display(QString st, QVariant var);

        void on_BSA_pbap_show_pb_data();

        void on_btnPbcAbort_clicked();

    /*************************************************************************************/
    /*************** Message UI ************************************************************/
    /*************************************************************************************/


public:

    void MceStateChanged(BOOLEAN bConnected);
    void ProcessMceXMLData(const char* filename);
    void InitializeMceSettings();
    void ProcessMCEXMLData(tBSA_MCE_EVT event, const char* filename);
    void SetupListHeader(QStringList list);
    void InitializeTempFile(QString fName);
    void ResetMceFolderList();
    void MceNotifRegStateChanged(BOOLEAN bReg);
    void MceMnStateChanged(BOOLEAN bConnected);
    char * GetCurrentWorkingDirectory();
    void InitializeMCETempFile();
    void MceInstanceChanged();
    void McePushMessage(QString strTo, QString strSubject, QString strMsg);

    void MceUserResetUI(bool bFullReset);
    void MceUserAdjustUI(bool bIsEmail);
    void MceUserStartBrowsing();
    void MceSetNextFolder();
    QString MceGetEmailHeader(QString strEmail, QString strHeader);
    QString MceGetEventAttribute(QString notif, QString attr);
    QString MceNotifToString(QString notif);

    enum eMode
    {
        eUserMode,
        eTestMode
    };
    eMode m_eMceMode;

    QFile* g_TempFile;
    QString g_TempFileName;
    QList<tBSA_MSG_LIST_ITEM> msgList;
    bool bMnsStarted;
    QList<MapInstance> m_listMapInst;
    int m_iMapIndex;
    bool m_bDevSelected;
    BD_ADDR m_SelectedDevBda;
    int m_iSelectedInst;

    QStringList m_listFolders;

    enum eMceAction
    {
        eMceActionNone,
        eMceActionListFolder,
        eMceActionListMessage,
        eMceActionPushMessage,
        eMceActionNoOp
    };
    eMceAction m_eMceNextAction;

    enum eMsgType
    {
        eMsgTypeIncoming,
        eMsgTypeOutgoing
    };
    eMsgType m_eMsgType;
    void MceUserSetMsgType(eMsgType type);

public slots:

        void on_BSA_mce_data_display(QVariant event, QString st, QVariant var);

        void on_BSA_mce_show_data(QString strMsgFileName);



        void on_btnGetContacts_clicked();

        void on_btnListICHVCards_clicked();

        void on_btnListOCHVCards_clicked();

        void on_btnListMCHVCards_clicked();

        void on_btnMceGetMessage_clicked();

        void on_btnMceStartStopMNS_clicked();

        void on_btnMceRegUnRegNotification_clicked();

        void on_btnMceUpdateInbox_clicked();

        void on_btnMceGetMASInfo_clicked();

        void on_btnMcePushMessage_clicked();

        void on_btnMceListFolders_clicked();

        void on_btnMceListMessages_clicked();

        void on_btnMceSetPath_clicked();

        void on_btnUpdateProg_clicked();

        void on_btnMceSetMessageStatus_clicked();

        void on_btnMceConnectDisconnect_clicked();

        void on_btnMapClient_clicked();

        void on_btnMceGetMASInstances_clicked();

        void on_lstMceMessages_itemClicked(QListWidgetItem *item);

        void on_lstMceMessages_itemDoubleClicked(QListWidgetItem *item);

        void on_lstMceFolders_itemDoubleClicked(QListWidgetItem *item);

        void on_btnMceAbort_clicked();

        void on_cmbMceInstances_currentIndexChanged(int index);

        void on_btnMceUserMode_clicked();


        void on_cmbMceUserInsts_currentIndexChanged(int index);

        void on_btnMceUserConnDisc_clicked();

        void on_cmbMceUserFolders_currentIndexChanged(int index);

        void on_lstMceUserMsgs_itemClicked(QListWidgetItem *item);

        void on_btnMceUserRefresh_clicked();

        void on_btnMceUserReply_clicked();

        void on_btnMceUserSend_clicked();

        void on_btnMceUserCancel_clicked();

        void on_btnMceTestMode_clicked();

        void on_MceUserStateChanged(QVariant var);


        /*************************************************************************************/
        /*************** AV source UI ************************************************************/
        /*************************************************************************************/


        void on_btnConnectAV_clicked();
        void on_btnDisconnectAV_clicked();
        void on_btnPlayTone_clicked();
        void on_btnStartStopAudioRelay_clicked();
        void on_btnStopAV_clicked();
        void on_btnStereo_clicked();
        void on_pushButtonNotifn_clicked();
        void on_PauseButton_2_clicked();

        /*************************************************************************************/
        /*************** HF AG UI ************************************************************/
        /*************************************************************************************/

        void on_btnOpenAudio_clicked();
        void on_btnCloseAudio_clicked();
        void on_btnHFAG_clicked();
        void on_btnConnectAG_clicked();
        void on_btnDisconnectAG_clicked();
public:
        void ConfigAvkAvRelayState();

public:
        BOOLEAN m_bInit;
        bool m_bEnableAudioRelay;
        bool m_bAvOpen;
};

Q_DECLARE_METATYPE(MediaPlayer);
Q_DECLARE_METATYPE(tBSA_AVRC_ITEM_FOLDER);

#endif // BSA_H

//void EnableDisablePickupHangupRejectButtons(bool bEnable, int nAll);
