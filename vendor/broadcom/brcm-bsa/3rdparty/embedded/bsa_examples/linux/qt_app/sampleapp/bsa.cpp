#include "buildcfg.h"
#include "bsa_extern.h"
#include "ui_bsa.h"
#include "bsa.h"
#include <QMessageBox>
#include <qprocess.h>
#include <qdir.h>
#include "hfactions.h"

#include <unistd.h>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QTreeWidget>

#include "xmlparser.h"
#include "vcard.h"
#include "bsa_status.h"
#include "BMessageManager.h"
#include "BMessageConstants.h"
#include "cmnhf.h"
#include "ble.h"

#define CIND_TIMER 6000
#define CIND_DELAY_TIMER 3
#define NO_SERVICE 0
#define CS_SERVICE 1
#define VOIP_SERVICE 2
#define CS_VOIP_SERVICE 3

#define CALL_HELD_INACTIVE 0
#define CALL_HELD_ACTIVE 1
#define CALL_HELD_NOACT 2

#define ASCHASH 35
#define ASCSTAR 42
#define ASCZERO 48
#define ASCNINE 57
#define ASCA    65
#define ASCD    68

#define AVRC_ITEM_NULL 0

#define APP_AV_PLAYTYPE_AVK     4

// Global app pointer
BSA *gThis = NULL;

BOOLEAN m_bAVResumePending = FALSE;

static int g_av_prev_state = APP_AV_PLAY_STOPPED;

void HandleErrors(tBSA_HS_EVT event,tBSA_HS_MSG *p_data);

// App callbacks
void Security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data);
void DiscoveryCallback(tBSA_DISC_EVT evt, tBSA_DISC_MSG *p_data);
void AvkCallback(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data);
void HsCallback(tBSA_HS_EVT event, tBSA_HS_MSG *p_data);
void AvCallback(tBSA_AV_EVT event, tBSA_AV_MSG *p_data);
void AgCallback(tBSA_AG_EVT event, tBSA_AG_MSG *p_data);

void PBReadCallback();
void CLCCReadCallback();
void COPSReadCallback();
void CNUMReadCallback();

HFAct HFActObj;
void PbcCallback(tBSA_PBC_EVT event, tBSA_PBC_MSG *p_data);
void MceCallback(tBSA_MCE_EVT event, tBSA_MCE_MSG *p_data);

BLE gBle;

BD_ADDR gBDA;

#define PB_LIST  0
#define ICH_LIST 1
#define OCH_LIST 2
#define MCH_LIST 3
#define CCH_LIST 4

#define MSG_SELECT_DEVICE "Please select a device to connect to on Devices Screen"
#define MSG_SEARCHING   "Searching..."
#define MSG_SEARCH_COMPLETE "Search complete."
#define MSG_MSG_PAIRING_FAILURE "Pairing Unsuccessful"
#define MSG_SET_PATH_SUCCESS "Set Path succeeded"
#define MSG_SET_PATH_FAILED "Set Path failed"
#define MSG_MAP_CONNECTED "Message access is already connected with another device"

#define CONN_PROFILE_ANY    0
#define CONN_PROFILE_AVK    1
#define CONN_PROFILE_HS     2
#define CONN_PROFILE_PBC    3
#define CONN_PROFILE_MCE    4

#define BSA_HS

extern tAPP_AVK_CB app_avk_cb;

// App Ctor
BSA::BSA(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BSA)
{
    // set global
    gThis = this;
    g_TempFile = NULL;

    m_bInit = FALSE;

    m_pHFStartTimer = new QTimer(this);
    m_pSendIndTimer = new QTimer(this);
    m_pReadIndTimer = new QTimer(this);

    // auto connect AVK and PBAP on HF connection
    m_bAutoConnect = false;
    m_pAvkConnectTimer = new QTimer(this);
    m_pPbapConnectTimer = new QTimer(this);

    m_bAvOpen = FALSE;
    m_bEnableAudioRelay = FALSE;

    m_iIndexActiveConnection = -1;

    // Set UI defaults
    ui->setupUi(this);

    // Speaker volume settings slider
    m_pSpkSlider = new QSlider(Qt::Vertical, this->ui->devicePhoneFrame);
    m_pSpkSlider->setGeometry(360,130,20,50);
    m_pSpkSlider->setObjectName("SliderSpk");
    m_pSpkSlider->setRange(0,15);

    // Microphone volume settings slider
    m_pMicSlider = new QSlider(Qt::Vertical, this->ui->devicePhoneFrame);
    m_pMicSlider->setGeometry(310,130,20,50);
    m_pMicSlider->setObjectName("SliderMic");
    m_pMicSlider->setRange(0,15);

    ui->BatteryIndicator->setRange(0,100);
    ui->BatteryIndicator->setEnabled(FALSE);
    ui->BatteryIndicator->setSliderPosition(0);

    ui->lblPhoneRing->setEnabled(false);
    ui->lblPhoneRing->setVisible(false);

    ui->SignalIndicator->setRange(0,100);
    ui->SignalIndicator->setEnabled(FALSE);
    ui->SignalIndicator->setSliderPosition(0);

    DefaultVisibility();
    HFButtondefaults();

    ui->tblDeviceList->setVisible(true);
    ui->devicesFrame->setVisible(true);

    m_paired_row_number = 0;
    m_disc_row_number = 0;

    m_indexAppAttr = 0;

    m_uiSignal = NULL;

    ui->tblDeviceList->setRowCount(25);
    ui->tblDeviceList->setColumnCount(3);
    ui->tblPairedDevices->setRowCount(25);
    ui->tblPairedDevices->setColumnCount(3);

    ui->tblDispList->setRowCount(30);
    ui->tblDispList->setColumnCount(3);
    ui->tblDispList->setColumnWidth(0,120);
    ui->tblDispList->setColumnWidth(1,180);
    ui->tblDispList->setColumnWidth(2,50);
    ui->RadioBtnPhoneBook->setChecked(true);

    // Table for Music media player elements
    ui->tblMetaPlayer->setRowCount(7);
    ui->tblMetaPlayer->setColumnCount(2);
    ui->tblMetaPlayer->setColumnWidth(0,120);
    ui->tblMetaPlayer->setColumnWidth(1,330);

    QStringList list;
    list<<"Name"<<"Address"<<"Device Type";
    ui->tblDeviceList->setHorizontalHeaderLabels(list);
    ui->tblPairedDevices->setHorizontalHeaderLabels(list);

    list.clear();
    list<<"Name"<<"Phone #"<<"Type";
    SetupListHeader(list);

    list.clear();
    list<<"Media Element"<<"Value";
    ui->tblMetaPlayer->setHorizontalHeaderLabels(list);

    ui->cbEqualizer->setEnabled(false);
    ui->cbRepeat->setEnabled(false);
    ui->cbScan->setEnabled(false);
    ui->cbShuffle->setEnabled(false);
    ui->m_songProgressBar->setValue(0);

    ui->label->setText("Double click on a list item to browse or play");
    ui->btnBack->setVisible(false);

    ui->cbLastPlay->setChecked(false);

    connect(ui->btnBLE,SIGNAL(clicked()),this,SLOT(on_btnBle_clicked()));
    gBle.Init(this);

    // Connect to BSA server
    if (app_mgr_open(APP_DEFAULT_UIPC_PATH) == -1)
    {
        // if connection fails, show error message
        QString st("Could not connect to BSA server. Please make sure that bsa server is running from the same location as this app. Close this app and try again.");

        QMessageBox msg;
        msg.setText(st);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.exec();

        return;
    }

    // populate UI with previously paired devices
    GetPairedDevices();

    // Get version
    char szVersion[BSA_TM_BSA_SERVER_VERSION_LEN];
    tBSA_TM_READ_VERSION bsa_read_version;
    tBSA_STATUS bsa_status;

    bsa_status = BSA_TmReadVersionInit(&bsa_read_version);
    bsa_status = BSA_TmReadVersion(&bsa_read_version);

    strncpy(szVersion,(char *)bsa_read_version.bsa_server_version, BSA_TM_BSA_SERVER_VERSION_LEN-1);
    strBsaVersion = szVersion;

    // Configure application and server
    app_mgr_config(Security_callback);

    /* Initialize PBAP settings in settings screen */
    InitializePBAPSettings();

    /* Initialize MCE settings in settings screen */
    InitializeMceSettings();

    // Connect the signals and slots (to perform callback handling on UI thread)
    m_uiSignal = new UISignal;

    connect(m_uiSignal, SIGNAL(DisplayMessageBox(QString)), this, SLOT(on_BSA_show_message_box(QString)));
    connect(m_uiSignal, SIGNAL(SignalPairingRequest(QString)), this, SLOT(on_BSA_pair_request(QString)));
    connect(m_uiSignal, SIGNAL(SignalDiscovery()), this, SLOT(on_BSA_Discovery()));
    connect(m_uiSignal, SIGNAL(SignalPBRead()), this, SLOT(on_HandlePBRead()));
    connect(m_uiSignal, SIGNAL(SignalCLCCRead()), this, SLOT(on_HandleCLCCRead()));
    connect(m_uiSignal, SIGNAL(SignalCNUMRead()), this, SLOT(on_HandleCNUMRead()));
    connect(m_uiSignal, SIGNAL(SignalCOPSRead()), this, SLOT(on_HandleCOPSRead()));
    connect(m_uiSignal, SIGNAL(UpdatePBAPDataDisplay(QString, QVariant)), this, SLOT(on_BSA_pbap_data_display(QString, QVariant)));
    connect(m_uiSignal, SIGNAL(ShowPhonebookData()), this, SLOT(on_BSA_pbap_show_pb_data()));
    connect(m_uiSignal, SIGNAL(DisplayStatus(QString)), this, SLOT(on_BSA_error(QString)));
    connect(m_uiSignal, SIGNAL(UpdateMceDataDisplay(QVariant, QString, QVariant)), this, SLOT(on_BSA_mce_data_display(QVariant, QString, QVariant)));
    connect(m_uiSignal, SIGNAL(ShowMceData(QString)), this, SLOT(on_BSA_mce_show_data(QString)));
    connect(m_uiSignal, SIGNAL(MceUserStateChanged(QVariant)), this, SLOT(on_MceUserStateChanged(QVariant)));
    connect(m_uiSignal, SIGNAL(PhoneBookDownload()), this, SLOT(on_btnGetPhonebook_clicked()));


    // HF events connected
    connect(m_pSpkSlider, SIGNAL(valueChanged(int)), this, SLOT(on_SliderSpkVolPosition_changed(int)));
    connect(m_pSpkSlider, SIGNAL(sliderReleased()), this, SLOT(on_SliderSpkReleased()));
    connect(m_pMicSlider, SIGNAL(valueChanged(int)), this, SLOT(on_SliderMicVolPosition_changed(int)));
    connect(m_pMicSlider, SIGNAL(sliderReleased()), this, SLOT(on_SliderMicReleased()));

    connect(m_uiSignal, SIGNAL(HSStateConnected(QVariant)), this, SLOT(on_HSStateChange(QVariant)));


    // HF timer events
    connect(m_pSendIndTimer, SIGNAL(timeout()), this, SLOT(on_Indicator_CmdSend()));
    connect(m_pReadIndTimer, SIGNAL(timeout()), this, SLOT(on_Indicator_Update()));
    connect(m_pHFStartTimer, SIGNAL(timeout()), this, SLOT(on_HFStartTimer()));

    connect(m_uiSignal, SIGNAL(UpdateMediaAttr()), this, SLOT(on_update_MediaAttr()));
    connect(m_uiSignal, SIGNAL(UpdateProgress(QVariant, QVariant)), this, SLOT(on_BSA_update_progress(QVariant, QVariant)));
    connect(m_uiSignal, SIGNAL(AvrcpStateChanged(QVariant)), this, SLOT(on_AvrcpStateChanged(QVariant)));
    connect(m_uiSignal, SIGNAL(AvkClose(QVariant)), this, SLOT(on_AvkClose(QVariant)));
    connect(m_uiSignal, SIGNAL(AvkVolumeChanged()), this, SLOT(on_avk_volume_changed()));

    DefaultVars();
    AvkStateChanged(FALSE);
    on_AvrcpStateChanged(false);

    m_eRole = eNone;
    ui->btnMusic->setEnabled(false);
    ui->btnPhone->setEnabled(false);
    ui->btnPhonebook->setEnabled(false);
    ui->btnMapClient->setEnabled(false);
    ui->btnStereo->setEnabled(false);
    ui->btnHFAG->setEnabled(false);

    // Default role as sink (AVK/HF)
    ui->radioButtonPhone->setChecked(false);
    ui->radioButtonDual->setChecked(false);
    ui->radioButtonCarkit->setChecked(true);

    SetRole(eCarkit);

    ui->btnConnectAV->setEnabled(true);
    ui->btnDisconnectAV->setEnabled(false);

    ui->btnPlayTone->setEnabled(false);
    ui->btnStopAV->setEnabled(false);
    ui->pushButtonNotifn->setEnabled(false);
    ui->PauseButton_2->setEnabled(false);

    ui->btnConnectAG->setEnabled(true);
    ui->btnDisconnectAG->setEnabled(false);
    ui->btnOpenAudio->setEnabled(false);
    ui->btnCloseAudio->setEnabled(false);

    connect(m_pAvkConnectTimer, SIGNAL(timeout()), this, SLOT(on_timer_connectAVK()));
    connect(m_pPbapConnectTimer, SIGNAL(timeout()), this, SLOT(on_timer_connectPbap()));

    connect(this, SIGNAL(RunInUiThread(void *,void *, unsigned int)),SLOT(onRunInUiThread(void *,void *,unsigned int)));

    ConfigAvkAvRelayState();

    gThis->ui->lblVersion->setText(strBsaVersion);

    ui->traceText->setVisible(false);
}

// App dtor
BSA::~BSA()
{
    APP_DEBUG0("BSA::~BSA() called");

    gBle.shutdown();

    if(NULL!=m_pSpkSlider)
    {
        delete m_pSpkSlider;
        m_pSpkSlider = NULL;
    }

    if(NULL!=m_pMicSlider)
    {
        delete m_pMicSlider;
        m_pMicSlider = NULL;
    }

    if(NULL!=m_pSendIndTimer)
    {
        m_pSendIndTimer->stop();
        delete m_pSendIndTimer;
    }
    m_pSendIndTimer=NULL;

    if(NULL!=m_pReadIndTimer)
    {
       m_pReadIndTimer->stop();
       delete m_pReadIndTimer;
    }
    m_pReadIndTimer=NULL;

    if(NULL!=m_pHFStartTimer)
    {
        m_pHFStartTimer->stop();
        delete m_pHFStartTimer;
    }
    m_pHFStartTimer=NULL;

    if(NULL!=m_pAvkConnectTimer)
    {
       m_pAvkConnectTimer->stop();
       delete m_pAvkConnectTimer;
    }
    m_pAvkConnectTimer=NULL;

    if(NULL!=m_pPbapConnectTimer)
    {
       m_pPbapConnectTimer->stop();
       delete m_pPbapConnectTimer;
    }
    m_pPbapConnectTimer=NULL;


    app_avk_close_all();
    app_hs_close();
    app_pbc_close();
    app_mce_close_all();
    app_mce_mn_stop();

    /* Terminate the profile */
    app_avk_end();

    /* Stop Headset service*/
    app_hs_stop();

    /* Stop PBAP Client */
    app_pbc_disable();

    /* Stop MCE Client */
    app_mce_disable();

    /* Just wait a second to receive all disable events before closing
    * connection to app manager */
    sleep(1);

    /* Close BSA before exiting (to release resources) */
    app_mgr_close();

    delete ui;

    if(m_uiSignal)
      delete m_uiSignal;
}

void BSA::DefaultVars()
{
    m_bAnswerCmdSent = false;
    m_bDialCmdSent = false;
    m_bLastNumDialCmdSent = false;
    m_bCHUPCmdSent = false;
    m_bMuteCmdSent = false;
    m_bUnmuteCmdSent = false;
    m_bStartVRCmdSent = false;
    m_bStopVRCmdSent = false;
    m_bCLCCCmdSentForTotal = false;
    m_bCLCCCmdTotRespObt = false;
    HFActObj.m_nCLCCRespValue = 0;
    HFActObj.m_strCallers = "";
}

// Set default visibility
void BSA::DefaultVisibility()
{
    ui->devicesFrame->setVisible(false);
    ui->devicePhoneFrame->setVisible(false);
    ui->deviceMusicFrame->setVisible(false);
    ui->devicePhonebookFrame->setVisible(false);
    ui->deviceMessagesFrame->setVisible(false);
    ui->deviceMceUserFrame->setVisible(false);
    ui->deviceAVFrame->setVisible(false);
    ui->deviceHFAGFrame->setVisible(false);

    ui->tblDeviceList->setVisible(false);
    ui->lblStatus->setVisible(false);
    ui->bleFrame->setVisible(false);
}

typedef void (*FUNC_ON_UI)(unsigned int, void *data);

void BSA::onRunInUiThread(void * fp, void * data, unsigned int event)
{
    ((FUNC_ON_UI)fp)(event, data);
}


/*************************************************************************************/
/******************Add Device UI *****************************************************/
/*************************************************************************************/
void BSA::on_addDevice_clicked()
{
    DefaultVisibility();
    ui->tblDeviceList->setVisible(true);
    ui->devicesFrame->setVisible(true);

    IsAnyDeviceConnected(CONN_PROFILE_ANY);
}

// Set app role (phone, carkit, dual)
void BSA::SetRole(eRole role)
{
    eRole lastRole = m_eRole;
    m_eRole = role;
    if(m_eRole == eCarkit)
    {
        if(lastRole == ePhone || lastRole == eDual)
        {
            SetPhoneRole(FALSE);
        }

        if(lastRole != eDual)
        {
            SetCarkitRole(TRUE);
        }

    }
    else if(m_eRole == ePhone)
    {
        if(lastRole == eCarkit || lastRole == eDual)
        {
            SetCarkitRole(FALSE);
        }

        if(lastRole != eDual)
        {
            SetPhoneRole(TRUE);
        }

    }
    else if(m_eRole == eDual)
    {
        if(lastRole != eCarkit )
        {
            SetCarkitRole(TRUE);
        }

        if(lastRole != ePhone )
        {
            SetPhoneRole(TRUE);
        }
    }
}

// Set carkit role
void BSA::SetCarkitRole(BOOLEAN bOn)
{
    if(bOn)
    {
        ui->btnMusic->setEnabled(true);
        ui->btnPhonebook->setEnabled(true);
        ui->btnMapClient->setEnabled(true);

        // AVK init
        int iRet = app_avk_init(AvkCallback);

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_avk_init failed");
        }

        // AVK start
        iRet = app_avk_register();

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_avk_register failed");
        }

#ifdef BSA_HS
        ui->btnPhone->setEnabled(true);

        /* Init Headset Application */
        app_hs_init();

        /* Start Headset service*/
        iRet = app_hs_start(HsCallback);

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_hs_start failed");
        }
#endif
        /* Enable PBC Client */
        iRet = app_pbc_enable(PbcCallback);

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_pbc_enable failed");
        }

        /* Enable MCE Client */
        iRet = app_mce_enable(MceCallback);

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_mce_enable failed");
        }


    }
    else
    {
        ui->btnMusic->setEnabled(false);
        ui->btnPhone->setEnabled(false);
        ui->btnPhonebook->setEnabled(false);
        ui->btnMapClient->setEnabled(false);

        if(m_pSendIndTimer->isActive())
            m_pSendIndTimer->stop();

        if(m_pReadIndTimer->isActive())
            m_pReadIndTimer->stop();

        if(m_pHFStartTimer->isActive())
            m_pHFStartTimer->stop();

        if(m_pAvkConnectTimer->isActive())
            m_pAvkConnectTimer->stop();

        if(m_pPbapConnectTimer->isActive())
            m_pPbapConnectTimer->stop();

        app_avk_close_all();

        app_avk_deregister();

        app_hs_close();

        app_pbc_close();

        app_mce_close_all();

        sleep (1);

        app_mce_mn_stop();

        /* Terminate the profile */
        app_avk_end();

        /* Stop Headset service*/
        app_hs_stop();

        /* Stop PBAP Client */
        app_pbc_disable();

        /* Stop MCE Client */
        app_mce_disable();

    }
}

// Set phone role
void BSA::SetPhoneRole(BOOLEAN bOn)
{
    if(bOn)
    {
        ui->btnStereo->setEnabled(true);
        ui->btnHFAG->setEnabled(true);

        // source
        int iRet = app_av_init(!m_bInit);
        app_init_playlist("../../qt_app/sampleapp/test");
        app_av_play_playlist(APP_AV_START);

        m_bInit = TRUE;

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_av_init failed");
        }

        app_av_set_cback(AvCallback);

        iRet = app_av_register();

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_av_register failed");
        }

        app_ag_init();

        app_ag_set_cback(AgCallback);

        iRet = app_ag_start(BSA_SCO_ROUTE_HCI);

        if(iRet != 0)
        {
            on_BSA_show_message_box("app_ag_start failed");
        }
    }
    else
    {
        ui->btnStereo->setEnabled(false);
        ui->btnHFAG->setEnabled(false);
        // AV source close
        app_av_close();

        sleep(1);

        // AV sec end
        app_av_end();

        // HF AG end
        app_ag_end();

    }
}

// Enable carkit role
void BSA::on_radioButtonCarkit_clicked()
{
    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);

    ui->radioButtonPhone->setChecked(false);
    ui->radioButtonDual->setChecked(false);

    SetRole(eCarkit);

    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);
}

// Enable phone role
void BSA::on_radioButtonPhone_clicked()
{
    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);

    ui->radioButtonCarkit->setChecked(false);
    ui->radioButtonDual->setChecked(false);

    SetRole(ePhone);

    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);
}

// Set dual role
void BSA::on_radioButtonDual_clicked()
    {
    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);

    ui->radioButtonPhone->setChecked(false);
    ui->radioButtonCarkit->setChecked(false);

    SetRole(eDual);

    ui->radioButtonCarkit->setEnabled(false);
    ui->radioButtonPhone->setEnabled(false);
    ui->radioButtonDual->setEnabled(false);
}

// Search for devices
void BSA::on_btnSearch_clicked()
{
    ui->btnSearch->setEnabled(false);
    ui->lblStatus->setVisible(true);
    ui->lblStatus->setText(MSG_SEARCHING);
    ui->tblDeviceList->clearContents();
    m_disc_device_list.clear();
    m_disc_row_number = 0;

    app_disc_start_regular(DiscoveryCallback);
}

// DiscoveryCallback function. Used to populate the table with list of devices available to pair
// with and also store their corresponding Bluetooth Addresses and device types.
void DiscoveryCallback(tBSA_DISC_EVT evt, tBSA_DISC_MSG *p_data)
{
    if (p_data)
    {
        bool isSkip = false;

        APP_INFO1("DiscoveryCallback %s", p_data->disc_new.name);

        // Search for paired list, if present do not add

        Bsa_Device *device = new Bsa_Device();

        memcpy(&device->msg, &p_data->disc_new, sizeof(tBSA_DISC_NEW_MSG));

        for(int i = 0; i < gThis->m_paired_row_number; i++)
        {
            int result = bdcmp(device->msg.bd_addr, gThis->m_paired_device_list[i].msg.bd_addr);

            if (result == 0)
            {
                isSkip = true;
                break;
            }
        }

        if(!isSkip)
        {
            gThis->m_disc_device_list.append(*device);
        }
        else
        {
            isSkip = false;
        }
    }

    switch (evt)
    {
    case BSA_DISC_CMPL_EVT: /* Discovery complete. */
        APP_INFO0("Discovery complete");
        app_disc_cb.p_disc_cback = NULL;

        emit gThis->m_uiSignal->SignalDiscovery();
        break;

    default:
        APP_ERROR1("app_generic_disc_cback unknown event:%d", evt);
        emit gThis->m_uiSignal->SignalDiscovery();
        break;
    }
}

// Update UI when discovery is complete
void BSA::on_BSA_Discovery()
{
    ui->tblDeviceList->clearContents();
    m_disc_row_number = 0;

    for(int i = 0; i < m_disc_device_list.size(); i++)
    {
        Bsa_Device device = m_disc_device_list.at(i);
        PopulateTables(&device, ui->tblDeviceList, "Discovery");
        m_disc_row_number++;
    }

    ui->btnSearch->setEnabled(true);
    ui->lblStatus->setText(MSG_SEARCH_COMPLETE);
    ui->tblDeviceList->showNormal();
}

// User clicked on pair button
void BSA::on_btnPair_clicked()
{
    int iCount = m_disc_device_list.size();
    int iRow = ui->tblDeviceList->currentRow();

    // Check if valid device is selected, if not show message
    if ((iRow == -1) || (iRow > iCount -1))
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
    }
    else
    {
        ui->lblStatus->setVisible(false);
        // perform pairing
        int success = app_mgr_sec_bond(m_disc_device_list[iRow].msg.bd_addr);

        if (success != 0)
        {
            ui->lblStatus->setVisible(true);
            ui->lblStatus->setText(MSG_MSG_PAIRING_FAILURE);
        }
        else
        {
            ui->btnPair->setEnabled(false);
        }
    }
}

// Pair with peer device
int BSA::app_mgr_sec_bond(BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_BOND sec_bond;

    APP_INFO0("Bluetooth Bond menu:");

    BSA_SecBondInit(&sec_bond);
    bdcpy(sec_bond.bd_addr, bd_addr);
    status = BSA_SecBond(&sec_bond);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecBond failed:%d", status);
        return -1;
    }

    return 0;
}

// Security callback
void Security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
    switch(event)
    {
        // Simple pairing request, show message to user
    case BSA_SEC_SP_CFM_REQ_EVT:
        {
            char name[256];
            memset(name, 0, 256);

            if(p_data->cfm_req.just_works)
            sprintf(name, "Do you want to accept pairing from '%s'' ?", p_data->cfm_req.bd_name);
            else
                sprintf(name, "Do you want to accept pairing from '%s', PIN %d' ?", p_data->cfm_req.bd_name,  p_data->cfm_req.num_val);

            QString st(name);
            emit gThis->m_uiSignal->SignalPairingRequest(st);

            break;
        }

    case BSA_SEC_AUTH_CMPL_EVT:
        if (p_data->auth_cmpl.success != 0)
        {
            // update paired devices
            gThis->GetPairedDevices();
        }
        break;

    case BSA_SEC_UNKNOWN_LINKKEY_EVT:
    {
        for (int i = 0; i < gThis->m_removed_device_list.count(); i++)
        {
            if (memcmp(p_data->lost_link_key.bd_addr, gThis->m_removed_device_list[i].msg.bd_addr,BD_ADDR_LEN) == 0)
            {
                QString str;
                str.sprintf("Bluetooth device %s (%02X:%02X:%02X:%02X:%02X:%02X) is not paired.  Pair device and try operation again.",
                            gThis->m_removed_device_list[i].msg.name,
                            gThis->m_removed_device_list[i].msg.bd_addr[0],
                            gThis->m_removed_device_list[i].msg.bd_addr[1],
                            gThis->m_removed_device_list[i].msg.bd_addr[2],
                            gThis->m_removed_device_list[i].msg.bd_addr[3],
                            gThis->m_removed_device_list[i].msg.bd_addr[4],
                            gThis->m_removed_device_list[i].msg.bd_addr[5]
                        );
                emit gThis->m_uiSignal->DisplayMessageBox(str);
                break;
            }
        }
    }
    break;

    default:
        break;
    }

    gThis->ui->btnPair->setEnabled(true);
}

// Show message to user to accept pairing
void BSA::on_BSA_pair_request(QString st)
{
    QMessageBox msg;
    msg.setText(st);

    msg.setStandardButtons(QMessageBox::Yes|QMessageBox::No);

    int iRet = msg.exec();

    BOOLEAN bAccept = FALSE;
    if(iRet == 0x4000) // yes was clicked
        bAccept = TRUE;

    // Call the bsa stack for pairing request
    app_mgr_sp_cfm_reply(bAccept, app_sec_db_addr);
}

// Show message box
void BSA::on_BSA_show_message_box(QString st)
{
    QMessageBox msg;
    msg.setText(st);

    int iRet = msg.exec();
}

// Show error message to user
void BSA::on_BSA_error(QString st)
{
    ui->lblStatus->setVisible(TRUE);
    ui->lblStatus->setText(st);
}

// Remove paired device
void BSA::on_btnRemove_clicked()
{
    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();
    int i = 0;

    // Check if valid device is selected, if not show message
    if (iRow == -1 || iRow > iCount-1)
    {
        ui->lblStatus->setVisible(true);
    }
    else
    {
        ui->lblStatus->setVisible(false);

        // save device being removed for re-pairing
        if (-1 == isInRemovedList(m_paired_device_list[iRow].msg.bd_addr))
        {
            Bsa_Device * pDev = new Bsa_Device();
            if (pDev)
            {
                memcpy(pDev->msg.name, m_paired_device_list[iRow].msg.name,BD_NAME_LEN);
                memcpy(pDev->msg.bd_addr, m_paired_device_list[iRow].msg.bd_addr,BD_ADDR_LEN);
                m_removed_device_list.append(*pDev);
            }
        }

        int success = remove_device(m_paired_device_list[iRow].msg.bd_addr);
        ui->tblPairedDevices->removeRow(iRow);
        m_paired_row_number--;
    }
}

// Remove device from paired database
int BSA::remove_device(BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_REMOVE_DEV remove_dev;
    tAPP_XML_REM_DEVICE *p_xml_dev;

    BSA_SecRemoveDeviceInit(&remove_dev);
    bdcpy(remove_dev.bd_addr, bd_addr);
    status = BSA_SecRemoveDevice(&remove_dev);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecRemoveDevice failed:%d", status);
        return -1;
    }

    /* Remove the device from Security database (XML) */
    app_read_xml_remote_devices();

    for (int dev_index = 0 ; dev_index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; dev_index++)
    {
        p_xml_dev = &app_xml_remote_devices_db[dev_index];
        if ((p_xml_dev->in_use) && (bdcmp(p_xml_dev->bd_addr, bd_addr) == 0))
        {
            /* Mark this device as unused */
            p_xml_dev->in_use = FALSE;

            /* Update XML database */
            app_write_xml_remote_devices();
        }
    }

    return 0;
}

// Utility function to populate device table (discovered or paired devices)
void BSA::PopulateTables(Bsa_Device *device, QTableWidget *table, QString table_type)
{
    char name[256];
    char bd_addr[256];
    sprintf(name, "%s", device->msg.name);
    sprintf(bd_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
        device->msg.bd_addr[0], device->msg.bd_addr[1],
        device->msg.bd_addr[2], device->msg.bd_addr[3],
        device->msg.bd_addr[4], device->msg.bd_addr[5]);

    QStringList list(name);
    QString class_of_device = app_get_cod_string(device->msg.class_of_device);

    QTableWidgetItem *name_col = new QTableWidgetItem(list[0]);
    QTableWidgetItem *bd_addr_col = new QTableWidgetItem(bd_addr);
    QTableWidgetItem *class_of_dev_col = new QTableWidgetItem(class_of_device);

    if (table_type == "Discovery")
    {
        table->setItem(m_disc_row_number,0,name_col);
        table->setItem(m_disc_row_number,1,bd_addr_col);
        table->setItem(m_disc_row_number,2,class_of_dev_col);
    }
    else if (table_type == "Paired")
    {
        table->setItem(m_paired_row_number,0,name_col);
        table->setItem(m_paired_row_number,1,bd_addr_col);
        table->setItem(m_paired_row_number,2,class_of_dev_col);
    }
}

int BSA::isInRemovedList(UINT8 * addr)
{
    // save device being removed for re-pairing
    for (int i = 0; i < m_removed_device_list.count(); i++)
    {
        if (memcmp(m_removed_device_list[i].msg.bd_addr,addr,BD_ADDR_LEN) == 0)
            return i;
    }

    return -1;  // not found
}

// Update paired devices table
void BSA::GetPairedDevices()
{
    int i = 0;
    ui->tblPairedDevices->clearContents();
    m_paired_device_list.clear();
    app_read_xml_remote_devices();
    m_paired_row_number = 0;

    // Read paired devices from database
    int nb_device_max = APP_NUM_ELEMENTS(app_xml_remote_devices_db);

    for (int index = 0; index < nb_device_max; index++)
    {
        if (app_xml_remote_devices_db[index].in_use != false)
        {
            Bsa_Device *device = new Bsa_Device();

            memcpy(&device->msg.bd_addr, &app_xml_remote_devices_db[index].bd_addr, BD_ADDR_LEN);
            memcpy(&device->msg.name, &app_xml_remote_devices_db[index].name, BD_NAME_LEN);
            memcpy(&device->msg.class_of_device, &app_xml_remote_devices_db[index].class_of_device, DEV_CLASS_LEN);

            m_paired_device_list.append(*device);

            PopulateTables(device, ui->tblPairedDevices, "Paired");

            m_paired_row_number++;

            if (-1 != (i = isInRemovedList(device->msg.bd_addr)))
            {
                m_removed_device_list.removeAt(i);
            }
        }
    }
}

// Utility function to check if any device is connected
BOOLEAN BSA::IsAnyDeviceConnected(int profile)
{
    BD_ADDR bda_open = {0};
    BOOLEAN isOpen = FALSE;

    if (profile == CONN_PROFILE_ANY || profile == CONN_PROFILE_AVK)
    {
        if(app_avk_num_connections() == 1)
            isOpen = avk_is_any_open(bda_open);
    }

    if (profile == CONN_PROFILE_ANY || profile == CONN_PROFILE_HS)
    {
        if(!isOpen)
        {
            tBSA_HS_CONN_CB *p_conn;

            /* Check if HF connection exist, if so use that device to connect */
            if ((p_conn = app_hs_get_default_conn()) != NULL)
            {
                memcpy(&bda_open, &p_conn->connected_bd_addr, BD_ADDR_LEN);
                isOpen = TRUE;
            }
        }
    }

    if (profile == CONN_PROFILE_ANY || profile == CONN_PROFILE_PBC)
    {
        if(!isOpen)
        {
            isOpen = pbc_is_open(&bda_open);
        }
    }

    if (profile == CONN_PROFILE_ANY || profile == CONN_PROFILE_MCE)
    {
        if(!isOpen)
        {
            isOpen = mce_is_open(&bda_open);
        }
    }

    if(isOpen)
    {
        app_read_xml_remote_devices();
        int nb_device_max = APP_NUM_ELEMENTS(app_xml_remote_devices_db);

        char name[BD_NAME_LEN];
        for (int index = 0; index < nb_device_max; index++)
        {
            int result = bdcmp(bda_open, app_xml_remote_devices_db[index].bd_addr);

            if (result == 0)
            {
                memcpy(&name, &app_xml_remote_devices_db[index].name, BD_NAME_LEN);

                char title[BD_NAME_LEN*2];
                sprintf(title, "BSA Sample app (Connected:'%s')", name);
                QString strTile = title;
                gThis->setWindowTitle(strTile);

                break;
            }
        }
    }
    else
    {
        QString strTile = "BSA Sample app";
        gThis->setWindowTitle(strTile);
    }

    return isOpen;
}

UINT8* BSA::GetPairedDeviceName(BD_ADDR bd_addr)
{
    UINT8 *p_name = NULL;

    for(int i = 0; i < m_paired_row_number; i++)
    {
        int result = bdcmp(bd_addr, m_paired_device_list[i].msg.bd_addr);

        if (result == 0)
        {
            p_name = m_paired_device_list[i].msg.name;
            break;
        }
    }

    return p_name;
}

/*************************************************************************************/
/*************** BLE UI ************************************************************/
/*************************************************************************************/

// BLE UI actived
void BSA::on_btnBle_clicked()
{
    DefaultVisibility();
    ui->bleFrame->setVisible(true);
    gBle.frameActive();
}

/*************************************************************************************/
/*************** Music UI ************************************************************/
/*************************************************************************************/

// Music UI actived
void BSA::on_btnMusic_clicked()
{
    BD_ADDR bda_open;

    DefaultVisibility();

    ui->deviceMusicFrame->setVisible(true);
    ui->tblFileFolders->setVisible(false);
    ui->label->setVisible(false);
}

// Connect AVK
void BSA::on_connectAVKButton_clicked()
{
    if(avk_is_open_pending())
    {
        QString s = ui->connectAVKButton->text();
        if(s == "Canceling ...")
            return;
        ui->connectAVKButton->setText("Canceling ...");
        app_avk_cancel(app_avk_cb.open_pending_bda);
        return;
    }


    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    // check if valid device is selected, if not show message
    if (iRow == -1 || iRow > iCount-1)
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
    }
    else
    {
        ui->lblStatus->setVisible(false);
        tBSA_STATUS status;

        BD_ADDR bd_addr;

        // Get the paired device selected
        bdcpy(bd_addr, m_paired_device_list[iRow].msg.bd_addr);

        int iConnections = app_avk_num_connections();
        if(iConnections == APP_AVK_MAX_CONNECTIONS)
        {
            emit gThis->m_uiSignal->DisplayMessageBox("Reached max allowed AVK connection");
            return;
        }

        tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_bd_addr(bd_addr);

        if (connection != NULL)
        {
            emit gThis->m_uiSignal->DisplayMessageBox("This device is already connected");
            return;
        }
        else
        {
            avk_set_open_pending(TRUE);
            bdcpy(app_avk_cb.open_pending_bda, bd_addr);

            ui->connectAVKButton->setText("Cancel");

            tBSA_AVK_OPEN open_param;
            BSA_AvkOpenInit(&open_param);
            memcpy((char *) (open_param.bd_addr), bd_addr, sizeof(BD_ADDR));
            open_param.sec_mask = BSA_SEC_NONE;

            bdcpy(gBDA, bd_addr);

            status = BSA_AvkOpen(&open_param);
            if (status != BSA_SUCCESS)
            {
                APP_ERROR1("Unable to connect to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
                    open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
                    open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);

                avk_set_open_pending(FALSE);
            }
        }
    }
}

// Disconnect AVK connection
void BSA::on_disconnectButton_clicked()
{
    if(gThis->ui->cbAVKDevices->count() == 0)
    {
        return;
    }

    tAPP_AVK_CONNECTION * connection = GetActiveConnection();

    if(connection == NULL)
        return;

    APP_DEBUG0("BSA::on_disconnectButton_clicked called");
    app_avk_close(connection->bda_connected);
}



// Play
void BSA::on_Play_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
    {
        APP_DEBUG0("on_Play_clicked - *Error finding active connecion*");
        return;
    }

    if(connection->is_streaming_chl_open)
    {
    app_avk_play_start(connection->rc_handle);
}
    else
    {
        // We do this as a last resort because device did not open AVDT connection
        // It is device issue as it does not comply with AVK White paper.
        emit gThis->m_uiSignal->DisplayStatus("Reconnect and play from the peer device.");
    }
}

// Pause
void BSA::on_PauseButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    app_avk_play_pause(connection->rc_handle);
}

// Stop
void BSA::on_stopButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    app_avk_rc_send_click(BSA_AVK_RC_STOP, connection->rc_handle);


}

// Next
void BSA::on_nextButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    app_avk_play_next_track(connection->rc_handle);
}

// previous
void BSA::on_PrevButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    app_avk_play_previous_track(connection->rc_handle);
}

// Mute
void BSA::on_muteButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    app_avk_rc_send_command(BSA_AV_STATE_PRESS, BSA_AVK_RC_MUTE, connection->rc_handle);
    GKI_delay(300);
    app_avk_rc_send_command(BSA_AV_STATE_RELEASE, BSA_AVK_RC_MUTE, connection->rc_handle);
}

// Vol up
void BSA::on_volupButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    /* If abs vol is not supported, send vol pass thru command */

    if (connection->m_bAbsVolumeSupported == FALSE)
    {
        app_avk_rc_send_command(BSA_AV_STATE_PRESS, AVRC_ID_VOL_UP, connection->rc_handle);
        GKI_delay(300);
        app_avk_rc_send_command(BSA_AV_STATE_RELEASE, AVRC_ID_VOL_UP, connection->rc_handle);
    }
    else
    {
        /* increase vol by 10% */
        UINT8 vol = app_avk_cb.volume + (UINT8)(BSA_MAX_ABS_VOLUME/10);
        app_avk_cb.volume = (vol <= BSA_MAX_ABS_VOLUME) ? vol : BSA_MAX_ABS_VOLUME;

        /* send abs vol change notification */
        app_avk_reg_notfn_rsp(app_avk_cb.volume, connection->rc_handle, connection->volChangeLabel, AVRC_EVT_VOLUME_CHANGE, BSA_AVK_RSP_CHANGED);

        on_avk_volume_changed();
    }
}

// Vol down
void BSA::on_voldownButton_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    /* If abs vol is not supported, send vol pass thru command */
    if (connection->m_bAbsVolumeSupported == FALSE)
    {
        app_avk_rc_send_command(BSA_AV_STATE_PRESS, AVRC_ID_VOL_DOWN, connection->rc_handle);
        GKI_delay(300);
        app_avk_rc_send_command(BSA_AV_STATE_RELEASE, AVRC_ID_VOL_DOWN, connection->rc_handle);
    }
    else
    {
        /* increase vol by 10% */
        UINT8 voldown = (UINT8)(BSA_MAX_ABS_VOLUME/10);
        app_avk_cb.volume = (app_avk_cb.volume <= voldown) ? BSA_MIN_ABS_VOLUME : app_avk_cb.volume-voldown;

        /* send abs vol change notification */
        app_avk_reg_notfn_rsp(app_avk_cb.volume, connection->rc_handle, connection->volChangeLabel, AVRC_EVT_VOLUME_CHANGE, BSA_AVK_RSP_CHANGED);

        on_avk_volume_changed();
    }
}

// Update UI state when connected/disconnected
void BSA::AvkStateChanged(BOOLEAN bConnected, BD_ADDR *pBda)
{
    m_bPausedByApp = FALSE;
    m_uPlayState = AVRC_PLAYSTATE_STOPPED;

    ui->connectAVKButton->setText("Connect");

    if(bConnected && pBda)
    {
        /* XML Database update should be done upon reception of AV OPEN event */

        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();

        /* Add AV service for this devices in XML database */
        app_xml_add_trusted_services_db(app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db), *pBda,
            BSA_A2DP_SERVICE_MASK | BSA_AVRCP_SERVICE_MASK);

        /* Update database => write to disk */
        if (app_write_xml_remote_devices() < 0)
        {
            APP_ERROR0("Failed to store remote devices database");
        }
    }
}

// Update UI state when play/pause/stop state changes
void BSA::AvkPlayStateChanged(UINT8 uState)
{
    if(AVRC_PLAYSTATE_PLAYING == uState)
    {
        ui->Play->setEnabled(false);
        ui->stopButton->setEnabled(true);
        ui->PauseButton->setEnabled(true);
    }
    else if(AVRC_PLAYSTATE_PAUSED == uState)
    {
        ui->Play->setEnabled(true);
        ui->PauseButton->setEnabled(false);
    }
    else if(AVRC_PLAYSTATE_STOPPED == uState)
    {
        ui->Play->setEnabled(true);
        ui->stopButton->setEnabled(false);
        ui->PauseButton->setEnabled(false);
    }
    m_uPlayState = uState;
}

// Update UI of meta player
void BSA::on_update_MediaAttr()
{
    ui->tblMetaPlayer->clearContents();

    for(int i = 0; i < m_listMediaElements.num_attr; i++)
    {
        char name[256];
        sprintf(name, "%s", m_listMediaElements.attr_entry[i].name.data);
        QString mediaName(name);
        QString mediaType;

        switch(m_listMediaElements.attr_entry[i].attr_id)
        {
        case AVRC_MEDIA_ATTR_ID_TITLE:
            mediaType = "Title";
            break;
        case AVRC_MEDIA_ATTR_ID_ARTIST:
            mediaType = "Artist";
            break;
        case AVRC_MEDIA_ATTR_ID_ALBUM:
            mediaType = "Album";
            break;
        case AVRC_MEDIA_ATTR_ID_TRACK_NUM:
            mediaType = "Track Num";
            break;
        case AVRC_MEDIA_ATTR_ID_NUM_TRACKS:
            mediaType = "Total Tracks";
            break;
        case AVRC_MEDIA_ATTR_ID_GENRE:
            mediaType = "Genre";
            break;
        case AVRC_MEDIA_ATTR_ID_PLAYING_TIME:
            mediaType = "Playing Time";

            char valSec[256];

            QTime qtimeInit(1, 0, 0, 0); // initialize time with 1 hour
            int msec = mediaName.toInt();
            QTime qtime = qtimeInit.addMSecs(msec); // add the milisec
            int iHour = qtime.hour() - 1; // subtract 1 hour we used above
            int iMin = qtime.minute();
            int iSec = qtime.second();
            if(iHour > 0)
                sprintf(valSec, "%d hr, %d min, %d sec", iHour, iMin, iSec);
            else if(iMin != -1)
                sprintf(valSec, "%d min, %d sec", iMin, iSec);
            else if(iSec != -1)
                sprintf(valSec, "%d Min %d sec", iMin, iSec);
            else
                sprintf(valSec, "%s mil sec", name);

            mediaName = valSec;
            break;
        }

        QTableWidgetItem *colType = new QTableWidgetItem(mediaType);
        QTableWidgetItem *colVal = new QTableWidgetItem(mediaName);

        ui->tblMetaPlayer->setItem(i,0, colType);
        ui->tblMetaPlayer->setItem(i,1, colVal);
    }
}

/* set player attributes */
void SetPlayerValue(UINT8 id, UINT8 val)
{
    tAPP_AVK_CONNECTION * connection = gThis->GetActiveConnection();
    if(connection == NULL)
        return;

    UINT8 attr_ids [] = { id };
    UINT8 attr_vals [] = { val };
    UINT8 num_attr = 1;

    app_avk_rc_set_player_value_command(num_attr, attr_ids, attr_vals, connection->rc_handle);
}

void BSA::on_cbEqualizer_currentIndexChanged(int index)
{
    if(index < 0)
        return;

    SetPlayerValue(AVRC_PLAYER_SETTING_EQUALIZER, gThis->ui->cbEqualizer->itemData(index).toUInt());
}

void BSA::on_cbRepeat_currentIndexChanged(int index)
{
    if(index < 0)
        return;

    SetPlayerValue(AVRC_PLAYER_SETTING_REPEAT, gThis->ui->cbRepeat->itemData(index).toUInt());
}

void BSA::on_cbShuffle_currentIndexChanged(int index)
{
    if(index < 0)
        return;

    SetPlayerValue(AVRC_PLAYER_SETTING_SHUFFLE, gThis->ui->cbShuffle->itemData(index).toUInt());
}

void BSA::on_cbScan_currentIndexChanged(int index)
{
    if(index < 0)
        return;

    SetPlayerValue(AVRC_PLAYER_SETTING_SCAN, gThis->ui->cbScan->itemData(index).toUInt());
}

/* update the song progress bar */
void BSA::on_btnUpdateProg_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;
    app_avk_rc_get_play_status_command(connection->rc_handle);
}

void BSA::on_BSA_update_progress(QVariant len, QVariant pos)
{
    uint iMax = len.toUInt();
    if(iMax != 0)
        ui->m_songProgressBar->setMaximum(iMax  );
    ui->m_songProgressBar->setValue(pos.toUInt());

}

/* update media UI when AVRCP is state changes*/
void BSA::on_AvrcpStateChanged(QVariant bConnected)
{
    bool bEnable = false;
    if(bConnected.toBool())
        bEnable = true;

    if(!bEnable)
    {
        memset(&m_listMediaElements, 0 , sizeof(tBSA_AVK_GET_ELEMENT_ATTR_MSG));
        ui->tblMetaPlayer->clearContents();

        ui->cbEqualizer->setEnabled(false);
        ui->cbEqualizer->clear();
        ui->cbRepeat->setEnabled(false);
        ui->cbRepeat->clear();
        ui->cbShuffle->setEnabled(false);
        ui->cbShuffle->clear();
        ui->cbScan->setEnabled(false);
        ui->cbScan->clear();

        ui->m_songProgressBar->setValue(0);

        m_listAppAttr.clear();
        m_indexAppAttr = 0;
        m_listMediaPlayers.clear();
        m_listFileFolder.clear();
        ui->tblFileFolders->clear();
        ui->label->setText("Double click on a list item to browse or play");
        ui->cbMediaPlayer->clear();

    }

    ui->btnUpdateProg->setEnabled(bEnable);
    ui->muteButton->setEnabled(bEnable);
    ui->Play->setEnabled(bEnable);
    ui->stopButton->setEnabled(bEnable);
    ui->PauseButton->setEnabled(bEnable);
    ui->nextButton->setEnabled(bEnable);
    ui->PrevButton->setEnabled(bEnable);
    ui->voldownButton->setEnabled(bEnable);
    ui->volupButton->setEnabled(bEnable);
    ui->btnBack->setEnabled(bEnable);
    ui->btnNowPlaying->setEnabled(bEnable);
    ui->labelAbsVol->setText("");

    ShowMetaPlayerUI();
}

/* update player settings */
void SetUIAppSettings(UINT8 num_val, UINT8 *attr_ids, UINT8 * attr_val)
{
    gThis->ui->cbEqualizer->blockSignals(true);
    gThis->ui->cbRepeat->blockSignals(true);
    gThis->ui->cbShuffle->blockSignals(true);
    gThis->ui->cbScan->blockSignals(true);

    for(int i = 0; i < num_val; i++)
    {
        switch( attr_ids[i])
        {
            case AVRC_PLAYER_SETTING_EQUALIZER:
            {
                int iIndex = gThis->ui->cbEqualizer->findData(attr_val[i]);

                gThis->ui->cbEqualizer->setCurrentIndex(iIndex);
            }
            break;
            case AVRC_PLAYER_SETTING_REPEAT:
            {
                int iIndex = gThis->ui->cbRepeat->findData(attr_val[i]);

                gThis->ui->cbRepeat->setCurrentIndex(iIndex);
            }
            break;
            case AVRC_PLAYER_SETTING_SHUFFLE:
            {
                int iIndex = gThis->ui->cbShuffle->findData(attr_val[i])  ;

                gThis->ui->cbShuffle->setCurrentIndex(iIndex);
            }
            break;
            case AVRC_PLAYER_SETTING_SCAN:
            {
                int iIndex = gThis->ui->cbScan->findData(attr_val[i])  ;

                gThis->ui->cbScan->setCurrentIndex(iIndex);
            }
            break;
        }
    }

    gThis->ui->cbEqualizer->blockSignals(false);
    gThis->ui->cbRepeat->blockSignals(false);
    gThis->ui->cbShuffle->blockSignals(false);
    gThis->ui->cbScan->blockSignals(false);
}

// Get the index of browsed player
BOOLEAN BSA::GetBrowsedMediaPlayer(int &iIndexPlayer, tAPP_AVK_CONNECTION * connection)
{
    BOOLEAN bFound = FALSE;
    iIndexPlayer = 0;
    for(int i = 0; i < m_listMediaPlayers.size(); i++)
    {
        if(m_listMediaPlayers[i].m_Player.player_id == connection->m_uiBrowsedPlayer)
        {
            bFound  = TRUE;
            iIndexPlayer = i;
            break;
        }
    }

    return bFound;
}

// click on Back button
void BSA::on_btnBack_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    // get index of browsed player
    int iIndexPlayer  = 0;
    if(!GetBrowsedMediaPlayer(iIndexPlayer, connection))
        return; // should not happen

    if(m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size() == 0)
        return; // should not happen

    // clear current Browse UI contents
    gThis->m_listFileFolder.clear();
    gThis->ui->tblFileFolders->clear();

    // change up one folder and pop out last folder from list of folder hierarchy
    int iRemove = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size() -1;
    AvrcItem item = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.at(iRemove);
    m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.removeAt(iRemove);


    int iSize = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size();

    // Set the folder hierarchy label text
    QString strPath;
    for(int i = 0; i < iSize; i++)
    {
        AvrcItem itemI = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.at(i);

        char name[256];
        sprintf(name, "%s", itemI.item.u.folder.name.data);
        QString str = name;
        strPath = strPath + " \\ "+ str;
    }
    ui->label->setText(strPath);

    // If the folder list is empty, hide the Back button and reset label
    if(iSize == 0)
    {
        ui->btnBack->setVisible(false);
        ui->label->setText("Double click on a list item to browse or play");
    }
    else // show back button
    {
        ui->btnBack->setVisible(true);
    }

    // Change path to one level up
    app_avk_rc_change_path_command(connection->m_uid_counter, AVRC_DIR_UP, item.item.u.folder.uid, connection->rc_handle);


}

// Show meta player UI
void BSA::ShowMetaPlayerUI()
{
    ui->tblFileFolders->setVisible(false);
    ui->label->setVisible(false);
    ui->btnBack->setVisible(false);

    ui->tblMetaPlayer->setVisible(true);

    ui->btnNowPlaying->setText("Browse...");
}

// Show browse UI
void BSA::ShowBrowseUI()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    if(!connection->m_bDeviceSupportBrowse)
    {
        emit gThis->m_uiSignal->DisplayMessageBox("This device does not support browsing.");
        return;
    }

    if(gThis->ui->cbMediaPlayer->count() == 0)
    {
        app_avk_rc_get_folder_items(AVRC_SCOPE_PLAYER_LIST, 0, 10, connection->rc_handle);
        return;
    }

    // get selected media player
    int i = gThis->ui->cbMediaPlayer->currentIndex();

    // check if media player supports browsing
    if(AVRC_PF_BROWSE_SUPPORTED(m_listMediaPlayers[i].m_Player.features))
    {
        // If the current player is not addressed player, check if browsing is supported when player is not addressed player
        if((m_listMediaPlayers[i].m_Player.player_id  != connection->m_uiAddressedPlayer) && !AVRC_PF_BR_WH_ADDR_SUPPORTED(m_listMediaPlayers[i].m_Player.features))
        {
            char name[256];
            sprintf(name, "%s", m_listMediaPlayers[i].m_Player.name.data);

            QString str = name;
            str += " :  This player cannot be browsed as it is not active player.";

            emit gThis->m_uiSignal->DisplayMessageBox(str);
            return;
        }
    }
    else // if player does not support browsing, show message
    {
        char name[256];
        sprintf(name, "%s", m_listMediaPlayers[i].m_Player.name.data);

        QString str = name;
        str += " :  This player does not support browsing";

        emit gThis->m_uiSignal->DisplayMessageBox(str);
        return;
    }

    // If the current player is not already being browsed, start it
    if(m_listMediaPlayers[i].m_listFolderBrowseList.size() == 0)
    {
        if(m_listMediaPlayers[i].m_bIsBrowsed == FALSE)
        {
            m_listMediaPlayers[i].m_bIsBrowsed = TRUE;
            gThis->m_listFileFolder.clear();
            gThis->ui->tblFileFolders->clear();
            // start browsing (call app_avk_rc_set_browsed_player_command)
            connection->m_uiBrowsedPlayer = m_listMediaPlayers[i].m_Player.player_id;
            app_avk_rc_set_browsed_player_command(connection->m_uiBrowsedPlayer, connection->rc_handle);
        }

        ui->btnBack->setVisible(false);

    }
    else // if current player is already browsed, show back button
    {
        ui->btnBack->setVisible(true);
    }

    // show browsing UI
    ui->tblFileFolders->setVisible(true);
    ui->label->setVisible(true);
    ui->tblMetaPlayer->setVisible(false);
    ui->btnNowPlaying->setText("Now Playing");
}

// Clicked on Now Playing button, switch view : file/folder or meta player
void BSA::on_btnNowPlaying_clicked()
{
    if(ui->tblMetaPlayer->isVisible())
    {
        ShowBrowseUI();
    }
    else
    {
        ShowMetaPlayerUI();
    }
}

// Double click on list item
void BSA::on_tblFileFolders_doubleClicked(const QModelIndex &index)
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    if(connection == NULL)
        return;

    // get index of browsed player
    int iIndexPlayer = 0;
    if(!GetBrowsedMediaPlayer(iIndexPlayer, connection))
        return;

    int iRow = index.row();

    tBSA_AVRC_ITEM item = gThis->m_listFileFolder.at(iRow);

    // If folder was double clicked, browse it
    if(item.item_type == AVRC_ITEM_FOLDER)
    {
        gThis->m_listFileFolder.clear();
        gThis->ui->tblFileFolders->clear();

        AvrcItem *pItem = new AvrcItem;
        memcpy(&(pItem->item), &item, sizeof(tBSA_AVRC_ITEM));

        // Cache folder hierarchy and change path to it
        m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.append(*pItem);

        int iSize = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size();

        // Set the folder hierarchy in the label
        QString strPath;
        for(int i = 0; i < iSize; i++)
        {
            AvrcItem itemI = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.at(i);

            char name[256];
            sprintf(name, "%s", itemI.item.u.folder.name.data);
            QString str = name;
            strPath = strPath + " \\ "+ str;
        }
        ui->label->setText(strPath);

        ui->btnBack->setVisible(true);

        // Change path (inside) to the browsed folder
        app_avk_rc_change_path_command(connection->m_uid_counter, AVRC_DIR_DOWN, item.u.folder.uid, connection->rc_handle);


    }
    // If media was double clicked, play it
    else if(item.item_type == AVRC_ITEM_MEDIA)
    {
        app_avk_rc_play_item(AVRC_SCOPE_FILE_SYSTEM, item.u.folder.uid, connection->m_uid_counter, connection->rc_handle);
        on_btnNowPlaying_clicked();
    }

    // if it is fake item, browse more items
    if(item.item_type == AVRC_ITEM_NULL)
    {
        // Remove the fake items
        gThis->m_listFileFolder.removeAt(iRow);
        gThis->ui->tblFileFolders->takeItem(iRow);

        // Browse folder starting from last item
        app_avk_rc_get_folder_items(AVRC_SCOPE_FILE_SYSTEM, gThis->m_listFileFolder.size()-1, connection->m_iCurrentFolderItemCount-1, connection->rc_handle);
    }
}

// Connect/disconnect AVRCP
void BSA::on_DisconnectAVRCP_clicked()
{
    tAPP_AVK_CONNECTION * connection = GetActiveConnection();
    BD_ADDR bda;

//    if(connection == NULL)
//        return;

    if(connection)
        bdcpy(bda, connection->bda_connected);
    else
        bdcpy(bda, gBDA);

    if(avk_is_rc_open(bda))
        app_avk_close_rc(connection->rc_handle);
    else
    {
        app_avk_open_rc(bda);
    }
}


void BSA::on_cbAVKDevices_currentIndexChanged(int index)
{
    if(index < 0)
        return;

    if(m_iIndexActiveConnection >= 0) // This is last active connection before we swtich over
    {
        int iIndexPlayer  = 0;
        tAPP_AVK_CONNECTION * lastActConn = app_avk_find_connection_by_index(m_iIndexActiveConnection);
        if(lastActConn && lastActConn->is_rc_open && GetBrowsedMediaPlayer(iIndexPlayer, lastActConn))
        {
            int iSize = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size();
            if(iSize != 0)
            {
                gThis->m_listFileFolder.clear();
                gThis->ui->tblFileFolders->clear();
                ui->label->setText("Double click on a list item to browse or play");

                for(int i = 0; i < iSize; i++)
                {
                    // change up one folder and pop out last folder from list of folder hierarchy
                    int iRemove = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.size() -1;
                    AvrcItem item = m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.at(iRemove);
                    m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.removeAt(iRemove);

                    app_avk_rc_change_path_command(lastActConn->m_uid_counter, AVRC_DIR_UP, item.item.u.folder.uid, lastActConn->rc_handle);
                }

                m_listMediaPlayers[iIndexPlayer].m_listFolderBrowseList.clear();
             }
        }
    }

    int iD = gThis->ui->cbAVKDevices->itemData(index).toInt();

    m_iIndexActiveConnection = iD;
    tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_index(iD);

    if(connection == NULL)
    {
        APP_DEBUG0("on_cbAVKDevices_currentIndexChanged, connection not found");
        return;
    }

    if(!connection->is_rc_open)
    {
        APP_DEBUG0("RC connection not opended yet");
        return;
    }

    // clear the AVRCP UI
    on_AvrcpStateChanged(false);

    if(connection->peer_features & BSA_AVK_FEAT_BROWSE)
    {
        connection->m_bDeviceSupportBrowse = true;
        APP_DEBUG0("Browsing supported");
    }

    // get player setting and values
    app_avk_rc_list_player_attr_command(connection->rc_handle);

    // get current play status
    app_avk_rc_get_play_status_command(connection->rc_handle);

    // get info about current playing media
    memset(&gThis->m_listMediaElements, 0 , sizeof(tBSA_AVK_GET_ELEMENT_ATTR_MSG));
    app_avk_rc_get_element_attr_command(connection->rc_handle);

    if(connection->m_bDeviceSupportBrowse)
    {
        app_avk_rc_set_addressed_player_command(connection->m_uiAddressedPlayer, connection->rc_handle);
        app_avk_rc_set_browsed_player_command(connection->m_uiAddressedPlayer, connection->rc_handle);
    }


    //if(connection->m_bDeviceSupportBrowse)
    //    app_avk_rc_get_folder_items(AVRC_SCOPE_PLAYER_LIST, 0, 10, connection->rc_handle);

    // re-enable the AVRCP UI
    on_AvrcpStateChanged(true);


}

// Get the active connection (could be null)
tAPP_AVK_CONNECTION * BSA::GetActiveConnection()
{
    if(gThis->ui->cbAVKDevices->count() == 0)
        return NULL;

    int index = gThis->ui->cbAVKDevices->currentIndex();
    int iD = gThis->ui->cbAVKDevices->itemData(index).toInt();
    tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_index(iD);
    return connection;
}

// Using the rc_handle, check if the connection is active connection, if so return the connection pointer, else null
tAPP_AVK_CONNECTION * BSA::IsActiveAconnection(UINT8 rc_handle)
{
    tAPP_AVK_CONNECTION *connection = NULL;

    if(gThis->ui->cbAVKDevices->count() == 0)
        return NULL;

    // Get the ID of the active connection
    int index = gThis->ui->cbAVKDevices->currentIndex();
    int iD = gThis->ui->cbAVKDevices->itemData(index).toInt();

    // Get the connection for which we got notification
    connection = app_avk_find_connection_by_rc_handle(rc_handle);

    if(connection == NULL)
        return NULL;

    // If we got notification for connection that is not active, ignore it
    if(iD != connection->index)
    {
        return NULL;
    }

    return connection;

}

/* update UI when AVK is state changes*/
void BSA::on_AvkClose(QVariant var)
{
    UINT8 handle = var.toUInt();
    tAPP_AVK_CONNECTION *connection = app_avk_find_connection_by_av_handle(handle);
    if(connection != NULL)
    {
        for(int i = 0; i < gThis->ui->cbAVKDevices->count(); i++)
        {
            int iD = gThis->ui->cbAVKDevices->itemData(i).toInt();
            if(iD == connection->index)
            {
                gThis->ui->cbAVKDevices->removeItem(i);
                break;
            }

        }
    }
    gThis->AvkStateChanged(FALSE);
    gThis->ConfigAvkAvRelayState();

    if(gThis->ui->cbAVKDevices->count())
    {
        gThis->ui->cbAVKDevices->setCurrentIndex(0);

        char lbl[256];
        sprintf(lbl, "Connected devices (%d): ", gThis->ui->cbAVKDevices->count());

        QString strLbl = lbl;
        ui->labelConnedAVKDevices->setText(strLbl);

    }
    else
    {
        gThis->ui->cbAVKDevices->clear();
        m_iIndexActiveConnection = -1;

        on_AvrcpStateChanged(false);
        ui->labelConnedAVKDevices->setText("Connected devices (0): ");
    }

}

/* update on volume changed*/
void BSA::on_avk_volume_changed()
{
    /* set the UI label to show the volume percentage*/
    char s[20];
    int percent = (100.0/BSA_MAX_ABS_VOLUME) * app_avk_cb.volume;
    UINT8 u_percent = (UINT8)percent;
    sprintf(s, "Volume: %d%%", u_percent);
    QString strVol = s;
    gThis->ui->labelAbsVol->setText(strVol);

    /* change the system volume via system command */
    char szCommand[50];
    sprintf(szCommand, "amixer set Master %d%%", u_percent);
    system(szCommand);
}

// AVK callback
void AvkCallback(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data)
{
    tAPP_AVK_CONNECTION *connection = NULL;

    switch (event)
    {
    case BSA_AVK_OPEN_EVT: // open event
        APP_DEBUG1("BSA_AVK_OPEN_EVT status 0x%x", p_data->sig_chnl_open.status);

        if (p_data->sig_chnl_open.status == BSA_SUCCESS)
        {
            gThis->AvkStateChanged(TRUE, &(p_data->sig_chnl_open.bd_addr));
            connection = app_avk_add_connection(p_data->sig_chnl_open.bd_addr);
            if(connection)
            {
                UINT8 *pname = gThis->GetPairedDeviceName(p_data->sig_chnl_open.bd_addr);
                QString strName((char *)pname);
                gThis->ui->cbAVKDevices->addItem(strName, connection->index);
            }

            char lbl[256];
            sprintf(lbl, "Connected devices (%d): ", gThis->ui->cbAVKDevices->count());

            QString strLbl = lbl;
            gThis->ui->labelConnedAVKDevices->setText(strLbl);

        }
        else
        {
            gThis->AvkStateChanged(FALSE, &(p_data->sig_chnl_open.bd_addr));
            emit gThis->m_uiSignal->DisplayMessageBox("Failed to connect to audio sink service.");
        }
        gThis->ConfigAvkAvRelayState();
        break;

    case BSA_AVK_CLOSE_EVT: // close event

        APP_DEBUG0("BSA_AVK_CLOSE_EVT");

        emit gThis->m_uiSignal->AvkClose(p_data->sig_chnl_close.ccb_handle);


        break;

    case BSA_AVK_START_EVT: //start streaming event
       {
            APP_DEBUG0("BSA_AVK_START_EVT");
            tAPP_AVK_CONNECTION *stream_conn = app_avk_find_streaming_connection();

            if(stream_conn == NULL)
                break;

            tAPP_AVK_CONNECTION *act_conn = gThis->GetActiveConnection();

            if(act_conn == NULL)
                break;

            // We got a stream event where active connection was different than streaming connection
            // switch UI to streaming connection
            if(stream_conn->ccb_handle != act_conn->ccb_handle)
            {
                for(int i = 0; i < gThis->ui->cbAVKDevices->count(); i++)
                {
                    // Get the ID of the active connection
                    int iD = gThis->ui->cbAVKDevices->itemData(i).toInt();

                    if(iD == stream_conn->index)
                    {
                        gThis->ui->cbAVKDevices->setCurrentIndex(i);
                        break;
                    }
                }

            }

            /* We got START_EVT for a new device that is streaming but server discards the data
                because another stream is already active */
            if(p_data->start_streaming.discarded)
            {
                /* Check if we are supposed to play the last playing device */
                /* If so, pause the current streaming device. The BSA server */
                /* will automatically start streaming the last playing device */
                if(gThis->ui->cbLastPlay->isChecked()) /* Party mode */
                {
                    app_avk_play_pause(stream_conn->rc_handle);
                }
            }
        }
        break;

    case BSA_AVK_STR_CLOSE_EVT: // streaming channel close
        {
            APP_DEBUG0("BSA_AVK_STR_CLOSE_EVT");
            // Get the active connection
            tAPP_AVK_CONNECTION *act_conn = gThis->GetActiveConnection();
            if(act_conn == NULL)
                break;

            // Check if the event was for active connection, if not ignore it
            if(bdcmp(act_conn->bda_connected, p_data->stream_chnl_close.bd_addr) != 0)
                break;

            // Set the play state to stopped. The BSA_AVK_STOP_EVT comes after BSA_AVK_CLOSE_EVT, so
            // first check if AVK is open, if not ignore the event.
            BOOLEAN isOpen = avk_is_open(p_data->stream_chnl_close.bd_addr);
            if(isOpen)
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_STOPPED);
        }
        break;

    case BSA_AVK_STOP_EVT: //stop streaming event
        {
            APP_DEBUG0("BSA_AVK_STOP_EVT");

            // Get the active connection
            tAPP_AVK_CONNECTION *act_conn = gThis->GetActiveConnection();
            if(act_conn == NULL)
                break;

            // Check if the event was for active connection, if not ignore it
            if(bdcmp(act_conn->bda_connected, p_data->stop_streaming.bd_addr) != 0)
                break;

            // Set the play state to stopped. The BSA_AVK_STOP_EVT comes after BSA_AVK_CLOSE_EVT, so
            // first check if AVK is open, if not ignore the event.
            BOOLEAN isOpen = avk_is_open(p_data->stop_streaming.bd_addr);
            if(isOpen)
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_STOPPED);
        }
        break;

    case BSA_AVK_REMOTE_RSP_EVT: // remote pass thru command response event

        APP_DEBUG0("BSA_AVK_REMOTE_RSP_EVT");

        // Check if it is active connection
        connection = gThis->IsActiveAconnection(p_data->remote_rsp.rc_handle);

        // If we got notification for connection that is not active, ignore it
        if(connection == NULL)
        {
            break;
        }

        if(p_data->remote_rsp.key_state == BSA_AVK_STATE_PRESS)
        {
            if(p_data->remote_rsp.rc_id == BSA_AVK_RC_PLAY)
            {
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PLAYING);
            }
            else if (p_data->remote_rsp.rc_id == BSA_AVK_RC_PAUSE)
            {
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PAUSED);
            }
            else if (p_data->remote_rsp.rc_id == BSA_AVK_RC_STOP)
            {
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_STOPPED);
            }
            else if ((p_data->remote_rsp.rc_id == BSA_AVK_RC_MUTE) && (p_data->remote_rsp.len == BSA_AVK_RSP_NOT_IMPL))
            {
                gThis->ui->muteButton->setEnabled(false);
            }
            else if ((p_data->remote_rsp.rc_id == BSA_AVK_RC_VOL_UP) && (p_data->remote_rsp.len == BSA_AVK_RSP_NOT_IMPL))
            {
                gThis->ui->volupButton->setEnabled(false);
            }
            else if ((p_data->remote_rsp.rc_id == BSA_AVK_RC_VOL_DOWN) && (p_data->remote_rsp.len == BSA_AVK_RSP_NOT_IMPL))
            {
                gThis->ui->voldownButton->setEnabled(false);
            }
        }

        break;

    case BSA_AVK_RC_OPEN_EVT: // RC channel opened
        if(p_data->rc_open.status != BSA_SUCCESS)
        {
            APP_DEBUG1("BSA_AVK_RC_OPEN_EVT, failed %d", p_data->rc_open.status);
            break;
        }
        // if BSA_AVK_RC_OPEN_EVT succeeds, fall thru to the next case BSA_AVK_RC_PEER_OPEN_EVT

    case BSA_AVK_RC_PEER_OPEN_EVT:

        APP_DEBUG1("BSA_AVK_RC_OPEN_EVT, feat %x", p_data->rc_open.peer_features);
        APP_DEBUG1("BSA_AVK_RC_OPEN_EVT, version %x", p_data->rc_open.peer_version);

        /* If peer version is 0, it means the stack has not done SDP on peer. Wait for SDP, then we will get
         * BSA_AVK_RC_PEER_OPEN_EVT */
        if(p_data->rc_open.peer_version == 0)
            break;

        // If there are devices in combo box, check if this is active connection
        if(gThis->ui->cbAVKDevices->count() != 0)
        {
            // Check if it is active connection
            connection = gThis->IsActiveAconnection(p_data->rc_open.rc_handle);

            // If we got notification for connection that is not active, ignore it
            if(connection == NULL)
            {
                break;
            }
        }
        // If there are no devices in combo box yet, it means this is first device.
        else
        {
            connection = app_avk_find_connection_by_rc_handle(p_data->rc_open.rc_handle);
        }

        // If the peer version is 1.4 or greater get browse information
        if (connection->peer_version >= AVRC_REV_1_4 )
        {
            if(connection->peer_features & BSA_AVK_FEAT_BROWSE)
            {
                connection->m_bDeviceSupportBrowse = true;
                APP_DEBUG0("Browsing supported");
            }
        }

        // If the peer version is 1.3 or greater get metadata information
        if (connection->peer_version >= AVRC_REV_1_3 )
        {
            // get player setting and values when RC channel is opend
            app_avk_rc_list_player_attr_command(connection->rc_handle);

            // get current play status
            app_avk_rc_get_play_status_command(connection->rc_handle);

            // get info about current playing media
            memset(&gThis->m_listMediaElements, 0 , sizeof(tBSA_AVK_GET_ELEMENT_ATTR_MSG));
            app_avk_rc_get_element_attr_command(connection->rc_handle);
        }

        emit gThis->m_uiSignal->AvrcpStateChanged(true);

        gThis->ui->DisconnectAVRCP->setText("Disconnect AVRC");

        break;

    case BSA_AVK_RC_CLOSE_EVT:
        // clear the media elements app content and disable UI when AV_RC is closed.

        APP_DEBUG0("BSA_AVK_RC_CLOSE_EVT");

        // when count it 0, it was the last connection that was disconnected
        if(gThis->ui->cbAVKDevices->count() != 0)
        {
            // Check if it is active connection
            connection = gThis->IsActiveAconnection(p_data->rc_close.rc_handle);

            // If we got notification for connection that is not active, ignore it
            if(connection == NULL)
            {
                break;
            }
        }

        emit gThis->m_uiSignal->AvrcpStateChanged(false);
        gThis->ui->DisconnectAVRCP->setText("Connect AVRC");
        break;

    case BSA_AVK_GET_ELEM_ATTR_EVT: // response to get element attribute, populate media elements in UI
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->elem_attr.handle);
            if(conn == NULL)
            {
                break;
            }

            APP_DEBUG0("BSA_AVK_GET_ELEM_ATTR_EVT");
            memcpy(&gThis->m_listMediaElements, &p_data->elem_attr, sizeof(tBSA_AVK_GET_ELEMENT_ATTR_MSG));

            emit gThis->m_uiSignal->UpdateMediaAttr();
        }
        break;

    case BSA_AVK_REGISTER_NOTIFICATION_EVT: // Notification of upate done by peer
    {
        APP_DEBUG1("BSA_AVK_REGISTER_NOTIFICATION_EVT : handle %d", p_data->reg_notif.handle);

        tAPP_AVK_CONNECTION * conn = app_avk_find_connection_by_rc_handle(p_data->reg_notif.handle);

        // addressed player changed
        if(p_data->reg_notif.rsp.event_id == AVRC_EVT_ADDR_PLAYER_CHANGE )
        {
            conn->m_uiAddressedPlayer = p_data->reg_notif.rsp.param.addr_player.player_id;

            conn->m_uidCounterAddrPlayer = p_data->reg_notif.rsp.param.addr_player.uid_counter;

            APP_DEBUG1("AVRC_EVT_ADDR_PLAYER_CHANGE, id %d", conn->m_uiAddressedPlayer);

            app_avk_rc_get_folder_items(AVRC_SCOPE_PLAYER_LIST, 0, 10, p_data->reg_notif.handle);

        }

        conn = gThis->IsActiveAconnection(p_data->reg_notif.handle);
        if(conn == NULL)
        {
            break;
        }

        APP_DEBUG1("BSA_AVK_REGISTER_NOTIFICATION_EVT : ID %d", p_data->reg_notif.rsp.event_id);

        // On track change event, clear media elements app content and send request to get new content
        if(p_data->reg_notif.rsp.event_id == AVRC_EVT_TRACK_CHANGE)
        {
            memset(&gThis->m_listMediaElements, 0 , sizeof(tBSA_AVK_GET_ELEMENT_ATTR_MSG));

            app_avk_rc_get_element_attr_command(p_data->reg_notif.handle);

            // get current play status
            app_avk_rc_get_play_status_command(p_data->reg_notif.handle);
        }

        // Set UI for play status change notification
        else if(p_data->reg_notif.rsp.event_id == AVRC_EVT_PLAY_STATUS_CHANGE)
        {
            switch(p_data->reg_notif.rsp.param.play_status)
            {
            case AVRC_PLAYSTATE_STOPPED:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_STOPPED);
                break;
            case AVRC_PLAYSTATE_PLAYING:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PLAYING);
                break;
            case AVRC_PLAYSTATE_PAUSED:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PAUSED);
                break;
            }
        }
        // play position changed
        else if(p_data->reg_notif.rsp.event_id == AVRC_EVT_PLAY_POS_CHANGED )
        {
            QVariant qpos = p_data->reg_notif.rsp.param.play_pos;;
            QVariant qlen = 0;

            emit gThis->m_uiSignal->UpdateProgress(qlen, qpos);
        }

        // app settings changed
        else if(p_data->reg_notif.rsp.event_id == AVRC_EVT_APP_SETTING_CHANGE )
        {
            UINT8 attr_ids[8];
            UINT8 attr_vals[8];
            UINT8 num_val = p_data->reg_notif.rsp.param.player_setting.num_attr;

            for(int i = 0; i < num_val; i++)
            {
                attr_ids[i] = p_data->reg_notif.rsp.param.player_setting.attr_id[i];
                attr_vals[i] = p_data->reg_notif.rsp.param.player_setting.attr_value[i];

            }

            SetUIAppSettings(num_val, attr_ids, attr_vals);

        }



        // available players changed
        else if(p_data->reg_notif.rsp.event_id == AVRC_EVT_AVAL_PLAYERS_CHANGE )
        {
            APP_DEBUG0("AVRC_EVT_AVAL_PLAYERS_CHANGE");
        }

        // now playing changed
        else if(p_data->reg_notif.rsp.event_id == AVRC_EVT_NOW_PLAYING_CHANGE )
        {
        }
    }
        break;

    // response for list player app attr
    case BSA_AVK_LIST_PLAYER_APP_ATTR_EVT:
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->list_app_attr.handle);
            if(conn == NULL)
            {
                break;
            }
            UINT8 num = p_data->list_app_attr.rsp.num_attr;

            APP_DEBUG1("BSA_AVK_LIST_PLAYER_APP_ATTR_EVT, num %d, status %d", num, p_data->list_app_attr.rsp.status);

            if(p_data->list_app_attr.rsp.status != AVRC_STS_NO_ERROR)
            {
                APP_DEBUG0("BSA_AVK_LIST_PLAYER_APP_ATTR_EVT error");
                break;
            }

            for(UINT8 i = 0; i < num; i++ )
            {
                switch(p_data->list_app_attr.rsp.attrs[i])
                {
                case AVRC_PLAYER_SETTING_EQUALIZER:
                    gThis->ui->cbEqualizer->setEnabled(true);
                    break;
                case AVRC_PLAYER_SETTING_REPEAT:
                    gThis->ui->cbRepeat->setEnabled(true);
                    break;
                case AVRC_PLAYER_SETTING_SHUFFLE:
                    gThis->ui->cbShuffle->setEnabled(true);
                    break;
                case AVRC_PLAYER_SETTING_SCAN:
                    gThis->ui->cbScan->setEnabled(true);
                    break;
                }

                AppAttr *pAppAttr = new AppAttr;
                pAppAttr->attr_id = p_data->list_app_attr.rsp.attrs[i];

                gThis->m_listAppAttr.append(*pAppAttr);

                app_avk_rc_list_player_attr_value_command(p_data->list_app_attr.rsp.attrs[i], p_data->list_app_attr.handle);
            }

        }
        break;


    // response for list player app attr values
    case BSA_AVK_LIST_PLAYER_APP_VALUES_EVT:
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->list_app_values.handle);
            if(conn == NULL)
            {
                break;
            }

            APP_DEBUG0("BSA_AVK_LIST_PLAYER_APP_VALUES_EVT");
            AppAttrVal *pVal = new AppAttrVal;
            pVal->val.num_val = p_data->list_app_values.rsp.num_val;
            memcpy(pVal->val.vals, p_data->list_app_values.rsp.vals, sizeof(UINT8) * pVal->val.num_val);

            if(gThis->m_listAppAttr.count() <= gThis->m_indexAppAttr)
            {
                APP_ERROR1("BSA_AVK_LIST_PLAYER_APP_VALUES_EVT error, count %d, index %d", gThis->m_listAppAttr.count(), gThis->m_indexAppAttr);
                break;
            }

            AppAttr attr = gThis->m_listAppAttr.at(gThis->m_indexAppAttr);
            attr.m_vals.append(*pVal);
            gThis->m_indexAppAttr++;

            if(gThis->m_indexAppAttr == gThis->m_listAppAttr.count())
            {
                UINT8 attr_ids[8];

                for(int i = 0; i < gThis->m_listAppAttr.count(); i++)
                {
                    AppAttr attr = gThis->m_listAppAttr.at(i);
                    attr_ids[i] = attr.attr_id;
                }

                app_avk_rc_get_player_value_command(attr_ids, gThis->m_listAppAttr.count(), p_data->list_app_values.handle);
            }

            switch(attr.attr_id)
            {
            case AVRC_PLAYER_SETTING_EQUALIZER:
            {
                for(int i = 0; i < pVal->val.num_val; i++)
                {
                    QString str;
                    switch(pVal->val.vals[i])
                    {
                        case AVRC_PLAYER_VAL_OFF:
                            str = "OFF";
                            break;
                        case AVRC_PLAYER_VAL_ON:
                            str = "ON";
                            break;
                    }

                    gThis->ui->cbEqualizer->addItem(str, pVal->val.vals[i]);

                }
                break;
            }
            case AVRC_PLAYER_SETTING_REPEAT:
            {
                for(int i = 0; i < pVal->val.num_val; i++)
                {
                    QString str;
                    switch(pVal->val.vals[i])
                    {
                        case AVRC_PLAYER_VAL_OFF:
                            str = "OFF";
                            break;
                        case AVRC_PLAYER_VAL_SINGLE_REPEAT:
                            str = "Single Track";
                            break;
                        case AVRC_PLAYER_VAL_ALL_REPEAT:
                            str = "All Tracks";
                            break;
                        case AVRC_PLAYER_VAL_GROUP_REPEAT:
                            str = "Group";
                            break;
                    }

                    gThis->ui->cbRepeat->addItem(str, pVal->val.vals[i]);

                }
                break;
            }

            case AVRC_PLAYER_SETTING_SHUFFLE:
            {
                for(int i = 0; i < pVal->val.num_val; i++)
                {
                    QString str;
                    switch(pVal->val.vals[i])
                    {
                        case AVRC_PLAYER_VAL_OFF:
                            str = "OFF";
                            break;
                        case AVRC_PLAYER_VAL_ALL_SHUFFLE:
                            str = "All";
                            break;
                        case AVRC_PLAYER_VAL_GROUP_SHUFFLE:
                            str = "Group";
                            break;
                    }
                    gThis->ui->cbShuffle->addItem(str, pVal->val.vals[i]);

                }
                break;
            }

            case AVRC_PLAYER_SETTING_SCAN:
            {
                for(int i = 0; i < pVal->val.num_val; i++)
                {
                    QString str;
                    switch(pVal->val.vals[i])
                    {
                        case AVRC_PLAYER_VAL_OFF:
                            str = "OFF";
                            break;
                        case AVRC_PLAYER_VAL_ALL_SCAN:
                            str = "All";
                            break;
                        case AVRC_PLAYER_VAL_GROUP_SCAN:
                            str = "Group";
                            break;
                    }
                    gThis->ui->cbScan->addItem(str, pVal->val.vals[i]);

                }
                break;
            }

            }

        }
        break;

    // response for get player attr value
    case BSA_AVK_GET_PLAYER_APP_VALUE_EVT:
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->get_cur_app_val.handle);
            if(conn == NULL)
            {
                break;
            }

            UINT8 attr_ids[8];
            UINT8 attr_vals[8];
            UINT8 num_val = p_data->get_cur_app_val.num_val;

            APP_DEBUG1("BSA_AVK_GET_PLAYER_APP_VALUE_EVT, %d", num_val);

            for(int i = 0; i < num_val; i++)
            {
                attr_ids[i] = p_data->get_cur_app_val.vals[i].attr_id;
                attr_vals[i] = p_data->get_cur_app_val.vals[i].attr_val;

            }

            SetUIAppSettings(num_val, attr_ids, attr_vals);
        }
        break;

    // response for get play status
    case BSA_AVK_GET_PLAY_STATUS_EVT:
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->get_play_status.handle);
            if(conn == NULL)
            {
                break;
            }

            APP_DEBUG0("BSA_AVK_GET_PLAY_STATUS_EVT");

            UINT32 len = p_data->get_play_status.rsp.song_len;
            UINT32 pos = p_data->get_play_status.rsp.song_pos;
            UINT8 status = p_data->get_play_status.rsp.play_status;

            QVariant qlen = len;
            QVariant qpos = pos;
            emit gThis->m_uiSignal->UpdateProgress(qlen, qpos);

            switch(status)
            {
            case AVRC_PLAYSTATE_STOPPED:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_STOPPED);
                break;
            case AVRC_PLAYSTATE_PLAYING:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PLAYING);
                break;
            case AVRC_PLAYSTATE_PAUSED:
                gThis->AvkPlayStateChanged(AVRC_PLAYSTATE_PAUSED);
                break;
            }

        }

        break;

    // response for set browsed player
    case BSA_AVK_SET_BROWSED_PLAYER_EVT:
        {
            APP_DEBUG0("BSA_AVK_SET_BROWSED_PLAYER_EVT");
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->br_player.handle);
            if(conn == NULL)
            {
                break;
            }
            // save the UID counter
            conn->m_uid_counter = p_data->br_player.uid_counter;

            if(p_data->br_player.final)
            {
                // save the folder count
                conn->m_iCurrentFolderItemCount = p_data->br_player.num_items;

                // Set the label for browse UI to add the folder count
                QString strLabel = gThis->ui->label->text();

                char name[256];
                sprintf(name, " : %d item(s)", conn->m_iCurrentFolderItemCount);

                strLabel += name;

                gThis->ui->label->setText(strLabel);

                // Now browse folder items
                app_avk_rc_get_folder_items(AVRC_SCOPE_FILE_SYSTEM, 0, conn->m_iCurrentFolderItemCount-1, p_data->br_player.handle);
            }

        }
        break;

    // Response for get folder items
    case BSA_AVK_GET_FOLDER_ITEMS_EVT:
        {
            tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->get_items.handle);
            if(conn == NULL)
            {
                break;
            }

            APP_DEBUG1("BSA_AVK_GET_FOLDER_ITEMS_EVT, type %d", p_data->get_items.item.item_type);

            if(p_data->get_items.status != AVRC_STS_NO_ERROR)
            {
                return;
            }

            // Create a list of media players
            if(p_data->get_items.item.item_type == AVRC_ITEM_PLAYER)
            {
                BOOLEAN bFound = false;
                for(int i = 0; i < gThis->m_listMediaPlayers.size(); i++)
                {
                    char name[256];
                    sprintf(name, "%s", p_data->get_items.item.u.player.name.data);

                    char name1[256];
                    sprintf(name1, "%s", gThis->m_listMediaPlayers[i].m_Player.name.data);

                    if(strcmp(name, name1) == 0)
                    {
                        bFound = true;

                        // When addressed player changes, set addressed player as current selected
                        if(conn->m_uiAddressedPlayer == p_data->get_items.item.u.player.player_id)
                            gThis->ui->cbMediaPlayer->setCurrentIndex(i);
                        break;
                    }
                }

                if(!bFound)
                {
                    MediaPlayer *pPlayer = new MediaPlayer;
                    memcpy(&(pPlayer->m_Player), &(p_data->get_items.item.u.player), sizeof(tBSA_AVRC_ITEM_PLAYER));

                    gThis->m_listMediaPlayers.append(*pPlayer);

                    char name[256];
                    sprintf(name, "%s (ID %d)", p_data->get_items.item.u.player.name.data, p_data->get_items.item.u.player.player_id);

                    QString str = name;

                    gThis->ui->cbMediaPlayer->addItem(str, QVariant::fromValue(*pPlayer));

                    // When addressed player changes, set addressed player as current selected
                    if(conn->m_uiAddressedPlayer == p_data->get_items.item.u.player.player_id)
                    {
                        int iCnt = gThis->ui->cbMediaPlayer->count();
                        gThis->ui->cbMediaPlayer->setCurrentIndex(iCnt-1);
                    }
                }

                // When addressed player changes, show meta player UI
                gThis->ShowMetaPlayerUI();
            }

            // For folders or media items, add then to browser list UI
            else if((p_data->get_items.item.item_type == AVRC_ITEM_FOLDER) || (p_data->get_items.item.item_type == AVRC_ITEM_MEDIA))
            {
                // make a copy of item
                tBSA_AVRC_ITEM *pItem= new tBSA_AVRC_ITEM;
                memcpy(pItem, &(p_data->get_items.item), sizeof(tBSA_AVRC_ITEM));

                // Add it to folder list
                gThis->m_listFileFolder.append(*pItem);

                // Set the for UI list
                char name[256];
                sprintf(name, "%s", p_data->get_items.item.u.folder.name.data);

                QString str = name;

                // Create item of different color for folder and media
                QListWidgetItem *pWItem = new QListWidgetItem(str);
                if(p_data->get_items.item.item_type == AVRC_ITEM_FOLDER)
                    pWItem->setForeground(Qt::darkBlue);
                else
                    pWItem->setForeground(Qt::darkGreen);

                pWItem->setText(str);

                // Add item to UI list
                gThis->ui->tblFileFolders->addItem(pWItem);

                // If we did not get all the items, add a fake list item that shows the total count
                if((p_data->get_items.final) && (gThis->m_listFileFolder.size() < conn->m_iCurrentFolderItemCount))
                {
                    tBSA_AVRC_ITEM *pItem= new tBSA_AVRC_ITEM;
                    pItem->item_type = AVRC_ITEM_NULL; // fake item type
                    gThis->m_listFileFolder.append(*pItem);

                    char name[256];
                    sprintf(name, "<-- Showing %d of %d items, double click for more -->", gThis->m_listFileFolder.size(), conn->m_iCurrentFolderItemCount);
                    QString str = name;

                    gThis->ui->tblFileFolders->addItem(str);

                }
            }

        }
        break;

    // response for change path
    case BSA_AVK_CHANGE_PATH_EVT:
    {
        tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->chg_path.handle);
        if(conn == NULL)
        {
            break;
        }

        APP_DEBUG0("BSA_AVK_CHANGE_PATH_EVT");
        // When path is changed, set the browsed folder, then execute folder list
        app_avk_rc_set_browsed_player_command(conn->m_uiBrowsedPlayer, p_data->chg_path.handle);

        break;
    }

    case BSA_AVK_SET_ADDRESSED_PLAYER_EVT:
    case BSA_AVK_SET_PLAYER_APP_VALUE_EVT:
    case BSA_AVK_GET_PLAYER_ATTR_TEXT_EVT:
    case BSA_AVK_GET_PLAYER_ATTR_VALUE_EVT:
    case BSA_AVK_GET_ITEM_ATTR_EVT:
    case BSA_AVK_PLAY_ITEM_EVT:
    case BSA_AVK_ADD_TO_NOW_PLAYING_EVT:
    case BSA_AVK_REG_NOTIFICATION_CMD_EVT:
    {
        APP_ERROR1("unhandled meta event %d", event);
        break;
    }

    case BSA_AVK_SET_ABS_VOL_CMD_EVT:
    {
        APP_DEBUG1("Absolute Volume Command event %d", event);

        tAPP_AVK_CONNECTION * conn = gThis->IsActiveAconnection(p_data->abs_volume.handle);
        tAPP_AVK_CONNECTION *connection = NULL;

        connection = app_avk_find_connection_by_rc_handle(p_data->abs_volume.handle);
        if((connection != NULL) && connection->m_bAbsVolumeSupported)
        {
            /* Peer requested change in volume. Make the change and send response with new system volume. BSA is TG role in AVK */
            /* However if we are already streaming from another source and are not able to make the change send the value we are currently using */
            if (p_data->abs_volume.abs_volume_cmd.volume <= BSA_MAX_ABS_VOLUME)
            {
                if(conn)
                    app_avk_cb.volume = p_data->abs_volume.abs_volume_cmd.volume;
            }

            app_avk_set_abs_vol_rsp(app_avk_cb.volume, p_data->abs_volume.handle, p_data->abs_volume.label);
        }

        emit gThis->m_uiSignal->AvkVolumeChanged();

        break;
    }

    default:

        APP_ERROR1("unhandled event %d", event);
        break;
    }
}

/*************************************************************************************/
/********************** Phone UI *****************************************************/
/*************************************************************************************/

// Phone UI activated
// UI Button click events
void BSA::on_btnPhone_clicked()
{
    DefaultVisibility();
    ui->devicePhoneFrame->setVisible(true);

    gThis->HsStateChanged(false);

    QString str("18775772726");
    ui->TxtDialNum->setText(str);

    tBSA_HS_CONN_CB *p_conn;

    /* Check if connection exist */
    if ((p_conn = app_hs_get_default_conn()) != NULL)
    {
        gThis->HsStateChanged(true);
    }

    IsAnyDeviceConnected(CONN_PROFILE_HS);
}


UINT8* BSA::GetConnectedHFDevice(BD_ADDR &bd_addr, BOOLEAN &bConnected)
{
    UINT8 *p_name = NULL;

    tBSA_HS_CONN_CB *p_conn;

    bConnected = FALSE;
    /* Check if HF connection exist, if so use that device to connect */
    if ((p_conn = app_hs_get_default_conn()) != NULL)
    {
        bdcpy(bd_addr, p_conn->connected_bd_addr);
        bConnected = TRUE;

        for(int i = 0; i < m_paired_row_number; i++)
        {
            int result = bdcmp(bd_addr, m_paired_device_list[i].msg.bd_addr);

            if (result == 0)
            {
                p_name = m_paired_device_list[i].msg.name;
                break;
            }
        }
    }

    return p_name;
}

// Phone book records read button clicked
void BSA::on_btnReadPB_clicked()
{
    tBSA_HS_CONN_CB *p_conn=NULL;
    ePBMemType eRecType= eCombPBType;

    QStringList list;
    list.clear();
    list<<"Name"<<"Phone #"<<"Type";
    SetupListHeader(list);

    if ((p_conn = app_hs_get_default_conn()) != NULL)
    {
        if(ui->RadioBtnPhoneBook->isChecked() == TRUE)
            eRecType = eCombPBType;
        else
        if(ui->RadioBtnDLCalls->isChecked() == TRUE)
           eRecType = eLastDialCombPBType;
        else
        if(ui->RadioBtnMSCalls->isChecked() == TRUE)
           eRecType = eMissCallPBType;
        else
        if(ui->RadioBtnRecdCalls->isChecked() == TRUE)
            eRecType = eRCPBType;

        if(0 != app_hs_send_CSCS_cmd(eRecType))
           printf("Error in sending PB read command\n");
    }
    else
    {
        QString st("Please switch to Handsfree mode for downloading phonebook contents.");
        QMessageBox msg;
        msg.setText(st);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.exec();
    }
}

// Connect with peer device
void BSA::on_btnConnectHS_clicked()
{
    BD_ADDR bda_open_pending_hs;
    if(app_hs_is_open_pending(bda_open_pending_hs))
    {
        QString s = ui->btnConnectHS->text();
        if(s == "Cancelling ...")
            return;

        ui->btnConnectHS->setText("Cancelling ...");

        app_hs_cancel();

        return;
    }

    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();
    tBSA_HS_CONN_CB *p_conn = NULL;

    // Check if valid device is selected, if not show message
    if (iRow == -1 || iRow > iCount-1)
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
    }
    else
    {
        ui->lblStatus->setVisible(false);

        /* Check if connection exist */
        if ((p_conn = app_hs_get_default_conn()) != NULL)
        {
            gThis->HsStateChanged(true);
            return;
        }

        BD_ADDR bd_addr;

        // Check if any AVK is open
        BD_ADDR bda_avk;
        BOOLEAN isOpen = avk_is_any_open(bda_avk);

        // If AVK is open, connect HS to same device
        if(isOpen)
            bdcpy(bd_addr, bda_avk);

        // If not open, use selected device
        else if(iCount > 0)
        {
            bdcpy(bd_addr, m_paired_device_list[ui->tblPairedDevices->currentRow()].msg.bd_addr);
        }
        else
        {
            // no paired devices
            return;
        }

        app_hs_open(&bd_addr);

        ui->btnConnectHS->setText("Cancel");
    }
}

// Disconnect connection
void BSA::on_btnDisconnectHS_clicked()
{
    tBSA_HS_CONN_CB *p_conn = app_hs_get_default_conn();
    if(!p_conn)
        return;

    app_hs_close();

    // If autoconnect is enabled, kill the AVK and PBAP connection timer and
    // cancel AVK / PBAP is connection
    if(m_bAutoConnect)
    {
        m_pAvkConnectTimer->stop();
        m_pPbapConnectTimer->stop();


        BOOLEAN bConnected = avk_is_open(p_conn->connected_bd_addr);

        if(bConnected ||
           (avk_is_open_pending() && (bdcmp(p_conn->connected_bd_addr, app_avk_cb.open_pending_bda) == 0))
           )
        {
            app_avk_cancel(p_conn->connected_bd_addr);
        }

        BD_ADDR bda_open;
        bConnected = pbc_is_open(&bda_open);
        if (bConnected || pbc_is_open_pending())
        {
            app_pbc_cancel();
        }
    }
}

// Handsfree button UI event
void BSA::on_btnHF_clicked()
{
    if(0==app_hs_audio_open())
       AudioStateChanged(true);
}

// Privacy button UI event
void BSA::on_btnPrivacy_clicked()
{
    if(0==app_hs_audio_close())
        AudioStateChanged(false);
}

// Pickup call UI event
void BSA::on_btnPickup_clicked()
{
    if(!m_bAnswerCmdSent && !m_bCHUPCmdSent)
    {
        HFState state = GetCurrentHFState();

        if (state == eHF_ON_CALL_INCOMING_CALL)
        {
            app_hs_hold_call(BTHF_CHLD_TYPE_HOLDACTIVE_ACCEPTHELD);
        }
        else if (app_hs_answer_call() == 0)
        {
            m_bAnswerCmdSent = true;
        }
        ResetPhoneRingLblState();
    }
}

// Call hangup
void BSA::on_btnHangup_clicked()
{
    if(!m_bCHUPCmdSent && !m_bAnswerCmdSent && 0==app_hs_hangup())
    {
        m_bCHUPCmdSent = true;
    }
}

// Call reject
void BSA::on_btnReject_clicked()
{
    if(!m_bCHUPCmdSent && !m_bAnswerCmdSent && 0==app_hs_hangup())
    {
        m_bCHUPCmdSent = true;
    }
}

void BSA::on_HFStartTimer()
{
    m_pHFStartTimer->stop();

    m_pSendIndTimer->start(CIND_TIMER);
    // Allow 3 sec for the indicator values to be read out
    sleep(CIND_DELAY_TIMER);
    m_pReadIndTimer->start(CIND_TIMER);
}

void BSA::on_Indicator_CmdSend()
{
    app_hs_send_ind_cmd();
}

// Voice recognition start button UI click event
void BSA::on_btnStartVR_clicked()
{
    if(0 == app_hs_start_voice_recognition())
        m_bStartVRCmdSent = true;
}

// Voice recognition stop button UI click event
void BSA::on_btnStopVR_clicked()
{
    if(0 == app_hs_stop_voice_recognition())
        m_bStopVRCmdSent = true;
}

// Call hold clicked event
void BSA::on_btnHold_clicked()
{
    HoldUnHold(false);
}

// Release hold clicked event
void BSA::on_btnReleaseHold_clicked()
{
    gThis->ui->btnReleaseHold->setText("Release Hold");
    HoldUnHold(false);
}

// Swap calls if more than 1 caller is connected
void BSA::on_btnSwapCalls_clicked()
{
    HoldUnHold(true);
}

// Dial last number clicked event
void BSA::on_btnLastNumDial_clicked()
{
    if(0==app_hs_last_num_dial())
        m_bLastNumDialCmdSent = true;
}

// COPS button UI click event
void BSA::on_btnCOPS_clicked()
{
    QList<QTableWidgetItem*> Items;
    char cCmd[10];

    memset(cCmd,'\0', sizeof(cCmd));
    Items = ui->tblDispList->selectedItems();

    HFActObj.DeleteAllCOPSItemsInList();

    //Allow only one item to be selected in the grid
    if(0 == Items.count())
    {
        strcpy(cCmd,"=?");
        if(0==app_hs_send_cops_cmd(cCmd))
        {
            QStringList list;
            list<<"Mode+Format"<<"Status"<<"Operator";
            SetupListHeader(list);
        }
        return;
    }
    else
    if(1 == Items.count())
    {
        // The format of the command to be done here (TBD)
        strcpy(cCmd,"=?");
        if(0==app_hs_send_cops_cmd(cCmd))
        {
            QStringList list;
            list<<"Mode+Format"<<"Status"<<"Operator";
            SetupListHeader(list);
        }
        return;
    }

    QString msg= "Please select only ONE item in the grid";
    // Provide an error message to the user, that only one item can be selected
    QMessageBox::information(NULL, msg,"");
}

// CNUM button UI click event
void BSA::on_btnCNUM_clicked()
{
    HFActObj.DeleteAllCNUMItemsInList();
    if(0==app_hs_send_cnum())
    {
        QStringList list;
        list<<"Description"<<"Number"<<"Type";
        SetupListHeader(list);
    }
}

// CLCC button UI click event
void BSA::on_btnCLCC_clicked()
{
    HFActObj.DeleteAllCLCCItemsInList();

    if(0==app_hs_send_clcc_cmd())
    {
        QStringList list;
        list<<"IDX+Dir+Status+Mode+MPTY"<<"Number"<<"Type";
        SetupListHeader(list);
    }
}

// DTMF button UI click event
void BSA::on_btnDTMF_clicked()
{
    QString strDTMFCmd = ui->TxtDTMF->text();
    if(strDTMFCmd.length() != 1)
    {
        APP_DEBUG0("DTMFinvalid string");
        return;
    }

    // Send 0-9, A-D, # and * characters
    const char *cDTMFCmd = strDTMFCmd.toStdString().c_str();
    char DTMFCmd;

    if((cDTMFCmd[0] >=ASCZERO && cDTMFCmd[0] <=ASCNINE) ||
       (cDTMFCmd[0]>=ASCA && cDTMFCmd[0] <=ASCD) ||
        cDTMFCmd[0]==ASCHASH ||
        cDTMFCmd[0]==ASCSTAR )
    {
        DTMFCmd = cDTMFCmd[0];
    }
    else
    {
        APP_DEBUG1("DTMF value not sent:%x\n",DTMFCmd);
        return;
    }

    printf("DTMF Tone value being sent:%x\n",DTMFCmd);
    app_hs_send_dtmf(DTMFCmd);
}

// Keypress button UI click event
void BSA::on_btnKeypress_clicked()
{
    QString strKP = ui->TxtKeyPress->text();
    strKP = strKP.trimmed();

    char *cKPCmd = (char*)strKP.toStdString().c_str();

    printf("KP value:%s\n",cKPCmd);
    app_hs_send_keypress_evt(cKPCmd);
}

// Dial number button UI click event
void BSA::on_btnCall_clicked()
{
    QString str = ui->TxtDialNum->text();
    const char *c_str2 = str.toUtf8().constData();

    if ((app_av_get_play_state()!= APP_AV_PLAY_STOPPED) &&
        (app_av_get_play_state() != APP_AV_PLAY_STOPPING) &&
        (app_av_get_play_state() != APP_AV_PLAY_PAUSED))
    {
        app_av_pause();
        sleep(1);
        m_bAVResumePending = TRUE;
    }


    if(0 == app_hs_dial_num(c_str2))
    {
        //HandlePickupnHangupComplEvent(true);
        m_bDialCmdSent = true;
    }
}

// Handlers for UI actions after response is obtained
// Display phone book contents read out
void BSA::on_HandlePBRead()
{
    int nCount=0;
    PBItem *pItem=NULL;

    nCount=HFActObj.ObtainPBItemCount();
    ui->tblDispList->clearContents();
    for(int i = 0; i < nCount; i++)
    {
        HFActObj.ObtainPBItemfromList(i,&pItem);
        if(NULL!=pItem)
            PopulatePBTables(i, pItem, "Phone Book Items");
    }

    ui->btnReadPB->setEnabled(true);
    ui->tblDispList->showNormal();
}

// Display CLCC response read out
void BSA::on_HandleCLCCRead()
{
    int nCount=0;
    CLCCItem *pItem=NULL;

    nCount=HFActObj.ObtainCLCCItemCount();
    ui->tblDispList->clearContents();
    for(int i = 0; i < nCount; i++)
    {
        HFActObj.ObtainCLCCItemfromList(i,&pItem);
        if(NULL!=pItem)
            PopulateCLCCTables(i, pItem, "CLCC Items");
    }

    ui->tblDispList->showNormal();
}

// Display CNUM response read out
void BSA::on_HandleCNUMRead()
{
    int nCount=0;
    CNUMItem *pItem=NULL;

    nCount=HFActObj.ObtainCNUMItemCount();
    ui->tblDispList->clearContents();
    for(int i = 0; i < nCount; i++)
    {
        HFActObj.ObtainCNUMItemfromList(i,&pItem);
        if(NULL!=pItem)
           PopulateCNUMTables(i, pItem, "CNUM Response");
    }

    ui->tblDispList->showNormal();
}

// Display COPS response read out
void BSA::on_HandleCOPSRead()
{
    int nCount=0;
    COPSItem *pItem=NULL;

    nCount=HFActObj.ObtainCOPSItemCount();
    ui->tblDispList->clearContents();
    for(int i = 0; i < nCount; i++)
    {
        HFActObj.ObtainCOPSItemfromList(i,&pItem);
        if(NULL!=pItem)
           PopulateCOPSTables(i, pItem, "COPS Response");
    }

    ui->tblDispList->showNormal();
}

// Handle speaker volume change slider changes
void BSA::on_SliderSpkVolPosition_changed(int volume)
{
    m_nLastSpkVol = volume;
}

// Handle microphone volume change slider changes
void BSA::on_SliderMicVolPosition_changed(int volume)
{
    m_nLastMicVol = volume;
}

// Handle speaker slider release event
void BSA::on_SliderSpkReleased()
{
    tBSA_BTHF_VOLUME_TYPE_T type = BTHF_VOLUME_TYPE_SPK;
    app_hs_set_volume(type,m_nLastSpkVol);
}

// Handle microphone slider release event
void BSA::on_SliderMicReleased()
{
    tBSA_BTHF_VOLUME_TYPE_T type = BTHF_VOLUME_TYPE_MIC;
    app_hs_set_volume(type,m_nLastMicVol);
}

// Callback obtained when phonebook read is completed
void PBReadCallback()
{
    if(NULL!=gThis && NULL!=gThis->m_uiSignal)
        emit gThis->m_uiSignal->SignalPBRead();
}

// Callback functions
// Callback obtained when CNUM AT Command response is obtained
void CNUMReadCallback()
{
    if(NULL!=gThis && NULL!=gThis->m_uiSignal)
        emit gThis->m_uiSignal->SignalCNUMRead();
}

// Callback obtained when COPS AT Command response is obtained
void COPSReadCallback()
{
    if(NULL!=gThis && NULL!=gThis->m_uiSignal)
        emit gThis->m_uiSignal->SignalCOPSRead();
}

// Callback obtained when CLCC AT Command response is obtained
void CLCCReadCallback()
{
    if(NULL!=gThis && NULL!=gThis->m_uiSignal)
        emit gThis->m_uiSignal->SignalCLCCRead();
}

void BSA::SetupButtonsAndState(HFState eState)
{
    HFState CurStateVal = GetCurrentHFState();

    if(CurStateVal == eState)
        return;

    printf("CURRENT HF STATE : %d\n",eState);
    SetCurrentHFState(eState);

    switch(eState)
    {
        case eHF_DISCONNECTED:
        {
            HFButtondefaults();
            break;
        }

        case eHF_CONNECTED:
        {
            SetConnDiscHSButtons(false, false, false);
            SetStartStopVRButtons(false,false, true);
            EnableDisableHFPrivacyButtons(false);
            EnableDisableHoldRelButtons(false);
            EnableDisableDTMFKPButtons(false);
            EnableDisableSliders(false);
            EnableDisableCallButtons(true);
            EnableDisableReadATCmdsButton(true);
            SetPickupHangupRejectButtons(false,false,false,true);
            ui->btnPickup->setEnabled(false);
            ui->btnReject->setEnabled(false);
            ui->btnSwapCalls->setEnabled(false);
            ResetPhoneRingLblState();
            break;
        }

        case eHF_FIRST_INCOMING_CALL:
        {
            EnableDisableHFPrivacyButtons(false);
            EnableDisableVRButtons(false);
            EnableDisableHoldRelButtons(false);
            EnableDisableDTMFKPButtons(false);
            EnableDisableCallButtons(false);
            EnableDisableSliders(false);
            SetPickupHangupRejectButtons(false,false,true,true);
            EnableDisableReadATCmdsButton(true);
            ui->btnSwapCalls->setEnabled(false);
            break;
        }

        case eHF_FIRST_CALL_SETUP:
        {
            ResetPhoneRingLblState();
            EnableDisableHFPrivacyButtons(false);
            EnableDisableVRButtons(false);
            EnableDisableHoldRelButtons(false);
            EnableDisableDTMFKPButtons(false);
            EnableDisableCallButtons(false);
            EnableDisableSliders(false);
            SetPickupHangupRejectButtons(false,false,false,false);
            EnableDisableReadATCmdsButton(true);
            ui->btnSwapCalls->setEnabled(false);
            break;
        }

        case eHF_ON_CALL:
        {
            SetHFPrivacyButtons(false, true, false);
            SetHoldRelButtons(false,false,true);
            SetPickupHangupRejectButtons(false,false,false,false);
            ResetPhoneRingLblState();

            EnableDisableVRButtons(false);
            EnableDisablePickupRejButton(false);
            EnableDisableCallButtons(true);
            EnableDisableSliders(true);
            EnableDisableDTMFKPButtons(true);
            EnableDisableReadATCmdsButton(true);
            ui->btnSwapCalls->setEnabled(false);
            break;
        }

        case eHF_ON_CALL_ANOTHER_CALL_SETUP:
        {
            EnableDisableHFPrivacyButtons(false);
            EnableDisableVRButtons(false);
            EnableDisableHoldRelButtons(false);
            EnableDisableCallButtons(false);
            EnableDisableDTMFKPButtons(false);
            EnableDisableSliders(false);
            EnableDisableReadATCmdsButton(true);
            ResetPhoneRingLblState();

            SetPickupHangupRejectButtons(false,false,false,false);
            ui->btnSwapCalls->setEnabled(false);
            break;
        }

        case eHF_ON_CALL_INCOMING_CALL:
        {
            EnableDisableVRButtons(false);
            EnableDisableCallButtons(false);
            EnableDisableDTMFKPButtons(false);
            EnableDisableSliders(false);
            EnableDisableReadATCmdsButton(true);
            SetPickupHangupRejectButtons(false,false,true,true);
            // Disable reject here, as it may reject the active call and there is nothing in spec to support it either
            ui->btnReject->setEnabled(false);
            ui->btnSwapCalls->setEnabled(false);
            break;
        }

        default:
            break;
    }
}

// Update UI state
// Setup pickup, hangup, reject, hold and unhold button states based on indicator results
void SetupButtonStates(tBSA_HS_IND_VALS *pIndVals)
{
    int nLoopValue=0;
    bool bCLCCCmdSent = false;
    bool bStateNotSetup = false;

    printf("CALL WAIT VALUE: %d,CALL IND VALUE:%d,CALL HELD VALUE:%d, CALL SETUP VALUE:%d\n",
            pIndVals->curr_callwait_ind,pIndVals->curr_call_ind,gThis->m_nCallHeldIndValue,
            pIndVals->curr_call_setup_ind);

    if(BSA_HS_CALL_ACTIVE == pIndVals->curr_call_ind)
    {
        gThis->SendCLCCCommand();
        bCLCCCmdSent = true;
    }
    else
    {
        HFActObj.m_nCLCCRespValue=0;
        HFActObj.m_strCallers = "";
    }

    if(BSA_HS_CALL_INACTIVE == pIndVals->curr_call_ind && 0 == pIndVals->curr_callheld_ind
                && 0 == pIndVals->curr_callwait_ind && BSA_HS_CALLSETUP_NONE == pIndVals->curr_call_setup_ind)
    {
        HFState eState = gThis->GetCurrentHFState();
        if(eState !=eHF_DISCONNECTED)
            gThis->SetupButtonsAndState(eHF_CONNECTED);
    }
    else
    if(BSA_HS_CALL_ACTIVE == pIndVals->curr_call_ind && BSA_HS_CALLSETUP_INCOMING == pIndVals->curr_call_setup_ind
            && 0 == pIndVals->curr_callwait_ind)
    {
        // Incoming first call (does not have wait indicator)
        gThis->SetupButtonsAndState(eHF_FIRST_INCOMING_CALL);
    }
    else
    if(BSA_HS_CALL_INACTIVE == pIndVals->curr_call_ind && (BSA_HS_CALLSETUP_OUTGOING == pIndVals->curr_call_setup_ind
             || BSA_HS_CALLSETUP_ALERTING == pIndVals->curr_call_setup_ind))
    {
        // Outgoing call
        gThis->SetupButtonsAndState(eHF_FIRST_CALL_SETUP);
    }
    else
    if(BSA_HS_CALL_ACTIVE == pIndVals->curr_call_ind && 0 == pIndVals->curr_callheld_ind
            && 0 == pIndVals->curr_callwait_ind && BSA_HS_CALLSETUP_NONE == pIndVals->curr_call_setup_ind)
    {
        // On call and no call is on hold nor a call is outgoing.
        gThis->SetupButtonsAndState(eHF_ON_CALL);
        // Enable hangup, in case it got disabled due to hold button toggling
        gThis->SetPickupHangupRejectButtons(false,false,false,false);
        gThis->SetHoldRelButtons(false,false,true);
    }
    else
        bStateNotSetup = true;

    // Give 5 seconds for response
    while (bCLCCCmdSent && false == gThis->LockUnlockGetCLCCRespVar() && nLoopValue < 5)
    {
        sleep(1);
        nLoopValue++;
    }

    if(bCLCCCmdSent)
        gThis->LockUnlockSetCLCCRespVar(false);

    gThis->ui->lblCallerID->setText(HFActObj.m_strCallers);

    // Handle more than one call here
    if(bStateNotSetup && HFActObj.m_nCLCCRespValue >=1 &&
       (BSA_HS_CALLSETUP_OUTGOING == pIndVals->curr_call_setup_ind ||
        BSA_HS_CALLSETUP_ALERTING == pIndVals->curr_call_setup_ind) &&
        0 == pIndVals->curr_callwait_ind)
    {
        // On call and another call outgoing
        gThis->SetupButtonsAndState(eHF_ON_CALL_ANOTHER_CALL_SETUP);
    }
    else
    if(HFActObj.m_nCLCCRespValue >=2 && BSA_HS_CALL_ACTIVE == pIndVals->curr_call_ind
            && BSA_HS_CALLHELD_ACTIVE_HELD == pIndVals->curr_callheld_ind
            && 0 == pIndVals->curr_callwait_ind && BSA_HS_CALLSETUP_NONE == pIndVals->curr_call_setup_ind)
    {
        // On call and a call is on hold.
        gThis->ui->btnReleaseHold->setText("Conference");
        gThis->ui->btnSwapCalls->setEnabled(true);
        // Disable pickup, in case it got enabled due to hold button toggling
        gThis->SetPickupHangupRejectButtons(false,false,false,false);
        gThis->SetHoldRelButtons(false,false,false);
    }
    else
    if(1 == HFActObj.m_nCLCCRespValue && BSA_HS_CALLHELD_HELD == pIndVals->curr_callheld_ind)
    {
        gThis->SetHoldRelButtons(false,false,false);
        gThis->ui->btnReleaseHold->setText("Release Hold");
    }
}

// Indicator timer updates
void BSA::on_Indicator_Update()
{
    tBSA_HS_IND_VALS *pIndVals=new tBSA_HS_IND_VALS();
    if(NULL == pIndVals)
        return;
    app_hs_getallIndicatorValues(pIndVals);
    m_nCallHeldIndValue = pIndVals->curr_callheld_ind;
    ui->BatteryIndicator->setSliderPosition((pIndVals->curr_battery_ind)*20);
    ui->SignalIndicator->setSliderPosition((pIndVals->curr_signal_strength_ind)*20);

    if(pIndVals->curr_call_ind >0)
       ui->ChkCallInd->setChecked(true);
    else
       ui->ChkCallInd->setChecked(false);

    if(pIndVals->curr_call_setup_ind >0)
       ui->chkCallSetupInd->setChecked(true);
    else
       ui->chkCallSetupInd->setChecked(false);

    if(NO_SERVICE == pIndVals->curr_service_ind)
       ui->radCallServNone->setChecked(true);
    else
    if(CS_SERVICE == pIndVals->curr_service_ind)
       ui->radCallServCS->setChecked(true);
    else
    if(VOIP_SERVICE == pIndVals->curr_service_ind)
       ui->radCallServVOIP->setChecked(true);
    else
    if(CS_VOIP_SERVICE == pIndVals->curr_service_ind)
       ui->radCallServCSVOIP->setChecked(true);

    if(CALL_HELD_INACTIVE == m_nCallHeldIndValue)
       ui->radCallHeldInactive->setChecked(true);
    else
    if(CALL_HELD_ACTIVE == m_nCallHeldIndValue)
       ui->radCallHeldActive->setChecked(true);
    else
    if(CALL_HELD_NOACT == m_nCallHeldIndValue)
       ui->radcallHeld_noAct->setChecked(true);

    if(pIndVals->curr_roam_ind >0)
       ui->ChkCallRoam->setChecked(true);
    else
       ui->ChkCallRoam->setChecked(false);
    SetupButtonStates(pIndVals);
}

void BSA::HsStateChanged(bool bConnected)
{
    if(bConnected)
    {
        SetupButtonsAndState(eHF_CONNECTED);
    }
    else
    {
        SetupButtonsAndState(eHF_DISCONNECTED);
    }

    ui->btnConnectHS->setVisible(!bConnected);
    ui->btnDisconnectHS->setVisible(bConnected);

    ui->btnConnectHS->setEnabled(!bConnected);
    ui->btnDisconnectHS->setEnabled(bConnected);

    ResetPhoneRingLblState();
    ui->btnConnectHS->setText("Connect");
}

// Handle audio state changed event
void BSA::AudioStateChanged(bool bAudioConnected)
{
    HFState CurStateVal = GetCurrentHFState();
    if(CurStateVal >=eHF_CONNECTED)
    {
        if(bAudioConnected)
            // Audio connected and user wants HF
            // Enable privacy button
        {
            SetHFPrivacyButtons(false,false, false);
            if(m_bAvOpen && !m_bAVResumePending)
            {
                if ((app_av_get_play_state()!= APP_AV_PLAY_STOPPED) &&
                            (app_av_get_play_state() != APP_AV_PLAY_STOPPING) &&
                            (app_av_get_play_state() != APP_AV_PLAY_PAUSED))
                    {
                        app_av_pause();
                        sleep(1);
                        m_bAVResumePending = TRUE;
                    }
            }
        }
        else
        {
            SetHFPrivacyButtons(false,false, true);

            if(m_bAvOpen && m_bAVResumePending)
            {
                m_bAVResumePending = FALSE;
                if(m_bEnableAudioRelay)
                    app_av_play_from_avk();
                else if(app_av_play_playlist(APP_AV_START) != 0)
                    app_av_play_tone();
            }
        }
    }
    else
    {
        EnableDisableHFPrivacyButtons(false);
    }

    ui->btnReleaseHold->setText("Release Hold");
    ResetPhoneRingLblState();
}

HFState BSA::GetCurrentHFState()
{
    HFState lStateVal=eHF_INVALID_STATE;

    m_HFStateLock.lockForWrite();
    lStateVal=m_eHFStateValue;
    m_HFStateLock.unlock();
    return lStateVal;
}

void BSA::SetCurrentHFState(HFState CurVal)
{
    m_HFStateLock.lockForWrite();
    m_eHFStateValue=CurVal;
    m_HFStateLock.unlock();
}

// Handle phone ring and call waiting event
void BSA::HandleRingnCallWaitEvent(bool bCallWaiting, QString strNum)
{
    QString strTemp;
    int nIndex=0;

    if(!bCallWaiting)
    {
        SetupButtonsAndState(eHF_FIRST_INCOMING_CALL);
        if(strNum.isEmpty())
            strTemp = "Please pick up the call ...";
        else
        {
            strTemp = "Please pick up the call from ";
            nIndex=strNum.indexOf(",");
            if(nIndex >0)
                strNum = strNum.left(nIndex);
            strTemp += strNum;
        }
    }
    else
    {
        SetupButtonsAndState(eHF_ON_CALL_INCOMING_CALL);
        if(strNum.isEmpty())
            strTemp = "Call waiting. Please pick up the call ...";
        else
        {
            strTemp = "Another call is waiting from ";
            nIndex=strNum.indexOf(",");
            if(nIndex >0)
                strNum = strNum.left(nIndex);
            strTemp += strNum;
        }
    }

    // Add the phone rings here.... TBD
    ui->lblPhoneRing->setEnabled(true);
    ui->lblPhoneRing->setVisible(true);
    ui->lblPhoneRing->setText(strTemp);
}

// Handle dial event if needed
void BSA::HandleDialComplEvent(char *number)
{
    // Nothing to be done for the current UI
}

// Handle redial event if needed
void BSA::HandleRedialComplEvent(char *number)
{
    // Nothing to be done for the current UI
}

// Handle phone pickup and hangup completed event
void BSA::HandlePickupnHangupComplEvent(bool bPickup)
{
    SetPickupHangupRejectButtons(true, false, false, false);
}

// Handle voice recognition state change event
void BSA::HandleVRStateChangeEvent(bool bState)
{
    SetStartStopVRButtons(true, false, false);
}

// Handle release and hold event here
void BSA::HandleReleasenHoldComplEvent(bool bHold)
{
    SetHoldRelButtons(true, false, false);
}

void BSA::HFButtondefaults()
{
    ui->btnHold->setVisible(true);
    ui->btnReleaseHold->setVisible(false);

    ui->btnReject->setVisible(true);
    ui->btnSwapCalls->setVisible(true);

    ui->btnCall->setVisible(true);
    ui->btnDTMF->setVisible(true);
    ui->btnKeypress->setVisible(true);

    ui->btnHF->setVisible(true);
    ui->btnPrivacy->setVisible(false);

    SetConnDiscHSButtons(false, true, false);

    ui->btnHangup->setEnabled(false);
    ui->btnSwapCalls->setEnabled(false);

    ui->btnReleaseHold->setText("Release Hold");

    EnableDisableHFPrivacyButtons(false);
    EnableDisableHoldRelButtons(false);
    EnableDisablePickupRejButton(false);
    EnableDisableCallButtons(false);
    EnableDisableDTMFKPButtons(false);
    EnableDisableReadATCmdsButton(false);
    EnableDisableVRButtons(false);
    EnableDisableSliders(false);

    ui->lblCallerID->setText("");
}

// Set HF / Privacy buttons or enable one button if both are not enabled
void BSA::SetHFPrivacyButtons(bool bToggle, bool bEnableOne, bool bEnableHF)
{
    bool bPrivEnabled = ui->btnPrivacy->isEnabled();
    bool bHFEnabled =   ui->btnHF->isEnabled();

    if(!bToggle)
    {
       // If both buttons are not enabled, enable the Privacy button
        if(false == bPrivEnabled && false == bHFEnabled && true == bEnableOne)
        {
            ui->btnHF->setVisible(false);
            ui->btnHF->setEnabled(false);
            ui->btnPrivacy->setVisible(true);
            ui->btnPrivacy->setEnabled(true);
        }
        else
        if(false == bEnableOne)
        {
            if(true == bEnableHF)
            {
                ui->btnPrivacy->setVisible(false);
                ui->btnPrivacy->setEnabled(false);
                ui->btnHF->setVisible(true);
                ui->btnHF->setEnabled(true);
            }
            else
            {
                ui->btnHF->setVisible(false);
                ui->btnHF->setEnabled(false);
                ui->btnPrivacy->setVisible(true);
                ui->btnPrivacy->setEnabled(true);
            }
        }
    }
    else
    {
        if(bPrivEnabled)
        {
            ui->btnPrivacy->setVisible(false);
            ui->btnPrivacy->setEnabled(false);
            ui->btnHF->setVisible(true);
            ui->btnHF->setEnabled(true);
        }
        else
        if(bHFEnabled)
        {
            ui->btnHF->setVisible(false);
            ui->btnHF->setEnabled(false);
            ui->btnPrivacy->setVisible(true);
            ui->btnPrivacy->setEnabled(true);
        }
        else
        {
            // If both buttons are not enabled, enable the Privacy button
             if(false == bPrivEnabled && false == bHFEnabled && true == bEnableOne)
             {
                 ui->btnHF->setVisible(false);
                 ui->btnHF->setEnabled(false);
                 ui->btnPrivacy->setVisible(true);
                 ui->btnPrivacy->setEnabled(true);
             }
        }
    }
}

// Set Start VR / Stop VR buttons or enable one button, if both are not enabled
void BSA::SetStartStopVRButtons(bool bToggle, bool bEnableOne, bool bEnableStart)
{
    bool bStartVREnabled = ui->btnStartVR->isEnabled();
    bool bStopVREnabled =  ui->btnStopVR->isEnabled();

    if(!bToggle)
    {
       // If both buttons are not enabled, enable the Start VR button
        if(false == bStartVREnabled && false == bStopVREnabled && true == bEnableOne)
        {
            ui->btnStopVR->setVisible(false);
            ui->btnStopVR->setEnabled(false);
            ui->btnStartVR->setVisible(true);
            ui->btnStartVR->setEnabled(true);
        }
        else
        if(false == bEnableOne)
        {
            if(true == bEnableStart)
            {
                ui->btnStopVR->setVisible(false);
                ui->btnStopVR->setEnabled(false);
                ui->btnStartVR->setVisible(true);
                ui->btnStartVR->setEnabled(true);
            }
            else
            {
                ui->btnStartVR->setVisible(false);
                ui->btnStartVR->setEnabled(false);
                ui->btnStopVR->setVisible(true);
                ui->btnStopVR->setEnabled(true);
            }
        }
    }
    else
    {
        if(bStartVREnabled)
        {
            ui->btnStartVR->setVisible(false);
            ui->btnStartVR->setEnabled(false);
            ui->btnStopVR->setVisible(true);
            ui->btnStopVR->setEnabled(true);
        }
        else
        if(bStopVREnabled)
        {
            ui->btnStopVR->setVisible(false);
            ui->btnStopVR->setEnabled(false);
            ui->btnStartVR->setVisible(true);
            ui->btnStartVR->setEnabled(true);
        }
        else
        {
            // If both buttons are not enabled, enable the Start VR button
             if(false == bStartVREnabled && false == bStopVREnabled && true == bEnableOne)
             {
                 ui->btnStopVR->setVisible(false);
                 ui->btnStopVR->setEnabled(false);
                 ui->btnStartVR->setVisible(true);
                 ui->btnStartVR->setEnabled(true);
             }
        }
    }
}

// Set Connect/disconnect buttons or enable one button, if both are not enabled
void BSA::SetConnDiscHSButtons(bool bToggle, bool bEnableOne, bool bEnableConn)
{
    bool bConnHSEnabled = ui->btnConnectHS->isEnabled();
    bool bDiscHSEnabled =  ui->btnDisconnectHS->isEnabled();

    if(!bToggle)
    {
       // If both buttons are not enabled, enable the Start VR button
        if(false == bConnHSEnabled && false == bDiscHSEnabled && true == bEnableOne)
        {
            ui->btnDisconnectHS->setVisible(false);
            ui->btnDisconnectHS->setEnabled(false);
            ui->btnConnectHS->setVisible(true);
            ui->btnConnectHS->setEnabled(true);
        }
        else
        if(false == bEnableOne)
        {
            if(true == bEnableConn)
            {
                ui->btnDisconnectHS->setVisible(false);
                ui->btnDisconnectHS->setEnabled(false);
                ui->btnConnectHS->setVisible(true);
                ui->btnConnectHS->setEnabled(true);
            }
            else
            {
                ui->btnConnectHS->setVisible(false);
                ui->btnConnectHS->setEnabled(false);
                ui->btnDisconnectHS->setVisible(true);
                ui->btnDisconnectHS->setEnabled(true);
            }
        }
    }
    else
    {
        if(bConnHSEnabled)
        {
            ui->btnConnectHS->setVisible(false);
            ui->btnConnectHS->setEnabled(false);
            ui->btnDisconnectHS->setVisible(true);
            ui->btnDisconnectHS->setEnabled(true);
        }
        else
        if(bDiscHSEnabled)
        {
            ui->btnDisconnectHS->setVisible(false);
            ui->btnDisconnectHS->setEnabled(false);
            ui->btnConnectHS->setVisible(true);
            ui->btnConnectHS->setEnabled(true);
        }
        else
        {
            // If both buttons are not enabled, enable the Connect HS button
             if(false == bConnHSEnabled && false == bDiscHSEnabled && true == bEnableOne)
             {
                 ui->btnDisconnectHS->setVisible(false);
                 ui->btnDisconnectHS->setEnabled(false);
                 ui->btnConnectHS->setVisible(true);
                 ui->btnConnectHS->setEnabled(true);
             }
        }
    }
}
void BSA::SetHoldRelButtons(bool bToggle, bool bEnableOne, bool bEnableHold)
{
    bool bHoldEnabled = ui->btnHold->isEnabled();
    bool bRelEnabled =   ui->btnReleaseHold->isEnabled();

    if(!bToggle)
    {
       // If both buttons are not enabled, enable the hold button
        if(false == bHoldEnabled && false == bRelEnabled && true == bEnableOne)
        {
            ui->btnReleaseHold->setVisible(false);
            ui->btnReleaseHold->setEnabled(false);
            ui->btnHold->setVisible(true);
            ui->btnHold->setEnabled(true);
        }
        else
        if(false == bEnableOne)
        {
            if(true == bEnableHold)
            {
                ui->btnReleaseHold->setVisible(false);
                ui->btnReleaseHold->setEnabled(false);
                ui->btnHold->setVisible(true);
                ui->btnHold->setEnabled(true);
            }
            else
            {
                ui->btnHold->setVisible(false);
                ui->btnHold->setEnabled(false);
                ui->btnReleaseHold->setVisible(true);
                ui->btnReleaseHold->setEnabled(true);
            }
        }
    }
    else
    {
        if(bHoldEnabled)
        {
            ui->btnHold->setVisible(false);
            ui->btnHold->setEnabled(false);
            ui->btnReleaseHold->setVisible(true);
            ui->btnReleaseHold->setEnabled(true);
        }
        else
        if(bRelEnabled)
        {
            ui->btnReleaseHold->setVisible(false);
            ui->btnReleaseHold->setEnabled(false);
            ui->btnHold->setVisible(true);
            ui->btnHold->setEnabled(true);
        }
        else
        {
            // If both buttons are not enabled, enable the hold button
             if(false == bHoldEnabled && false == bRelEnabled && true == bEnableOne)
             {
                 ui->btnReleaseHold->setVisible(false);
                 ui->btnReleaseHold->setEnabled(false);
                 ui->btnHold->setVisible(true);
                 ui->btnHold->setEnabled(true);
             }
        }
    }
}


void BSA::SetPickupHangupRejectButtons(bool bToggle, bool bEnableOne, bool bUseCLCC, bool bEnablePickup)
{
    bool bPickupEnabled = ui->btnPickup->isEnabled();
    bool bHangupEnabled = ui->btnHangup->isEnabled();

    if(!bToggle)
    {
       // If both buttons are not enabled, enable the pickup button
        if(false == bPickupEnabled && false == bHangupEnabled && true == bEnableOne)
        {
            ui->btnHangup->setVisible(false);
            ui->btnHangup->setEnabled(false);
            ui->btnPickup->setVisible(true);
            ui->btnPickup->setEnabled(true);
            if(bUseCLCC && HFActObj.m_nCLCCRespValue < 1)
                ui->btnReject->setEnabled(true);
            else
            if(!bUseCLCC)
                ui->btnReject->setEnabled(true);
        }
        else
        if(false == bEnableOne)
        {
            if(true == bEnablePickup)
            {
                ui->btnHangup->setVisible(false);
                ui->btnHangup->setEnabled(false);
                ui->btnPickup->setVisible(true);
                ui->btnPickup->setEnabled(true);
                if(bUseCLCC && HFActObj.m_nCLCCRespValue < 1)
                   ui->btnReject->setEnabled(true);
                else
                if(!bUseCLCC)
                   ui->btnReject->setEnabled(true);
            }
            else
            {
                ui->btnPickup->setVisible(false);
                ui->btnPickup->setEnabled(false);
                ui->btnReject->setEnabled(false);
                ui->btnHangup->setVisible(true);
                ui->btnHangup->setEnabled(true);
            }
        }
    }
    else
    {
        if(bPickupEnabled)
        {
            ui->btnPickup->setVisible(false);
            ui->btnPickup->setEnabled(false);
            ui->btnReject->setEnabled(false);
            ui->btnHangup->setVisible(true);
            ui->btnHangup->setEnabled(true);
        }
        else
        if(bHangupEnabled)
        {
            ui->btnHangup->setVisible(false);
            ui->btnHangup->setEnabled(false);
            if(bUseCLCC && HFActObj.m_nCLCCRespValue < 1)
               ui->btnReject->setEnabled(true);
            else
            if(!bUseCLCC)
                ui->btnReject->setEnabled(true);
            ui->btnPickup->setVisible(true);
            ui->btnPickup->setEnabled(true);
        }
        else
        {
            // If both buttons are not enabled, enable the pickup and reject button
             if(false == bPickupEnabled && false == bHangupEnabled && true == bEnableOne)
             {
                 ui->btnHangup->setVisible(false);
                 ui->btnHangup->setEnabled(false);
                 ui->btnPickup->setVisible(true);
                 ui->btnPickup->setEnabled(true);
                 ui->btnReject->setEnabled(true);
             }
        }
    }
}

void BSA::EnableDisableHoldRelButtons(bool bEnable)
{
    ui->btnHold->setEnabled(bEnable);
    ui->btnReleaseHold->setEnabled(bEnable);
}

void BSA::EnableDisableHFPrivacyButtons(bool bEnable)
{
    ui->btnHF->setEnabled(bEnable);
    ui->btnPrivacy->setEnabled(bEnable);
}

void BSA::EnableDisablePickupRejButton(bool bEnable)
{
    ui->btnPickup->setEnabled(bEnable);
    ui->btnReject->setEnabled(bEnable);
}

void BSA::EnableDisableReadATCmdsButton(bool bEnable)
{
    ui->btnReadPB->setEnabled(bEnable);
    ui->btnCLCC->setEnabled(bEnable);
    ui->btnCNUM->setEnabled(bEnable);
    ui->btnCOPS->setEnabled(bEnable);
}

void BSA::EnableDisableCallButtons(bool bEnable)
{
    ui->btnCall->setEnabled(bEnable);
    ui->btnLastNumDial->setEnabled(bEnable);
}

void BSA::EnableDisableSliders(bool bEnable)
{
    if(NULL!=m_pMicSlider)
        m_pMicSlider->setEnabled(bEnable);
    if(NULL!=m_pSpkSlider)
        m_pSpkSlider->setEnabled(bEnable);
}

void BSA::EnableDisableDTMFKPButtons(bool bEnable)
{
    ui->btnDTMF->setEnabled(bEnable);
    ui->btnKeypress->setEnabled(bEnable);
}

void BSA::EnableDisableVRButtons(bool bEnable)
{
    ui->btnStartVR->setEnabled(bEnable);
    ui->btnStopVR->setEnabled(bEnable);
}

// reset Phone ring label state
void BSA::ResetPhoneRingLblState()
{
    ui->lblPhoneRing->setEnabled(false);
    ui->lblPhoneRing->setVisible(false);
    ui->lblPhoneRing->setText("");
}

// Helpers and HS call back
void BSA::SendCLCCCommand()
{
    // Check if the command is already sent
    // out by another thread
    if(false == LockUnlockGetCLCCTotCmdVar())
    {
        HFActObj.m_nCLCCRespValue=0;
        HFActObj.m_strCallers = "";
        if(0==app_hs_send_clcc_cmd())
            LockUnlockSetCLCCTotCmdVar(true);
    }
}

// Hold and release hold of calls
void BSA::HoldUnHold(bool bSwap)
{
    int nLoopValue =0;

    if(!bSwap)
    {
        SendCLCCCommand();
        // Give 15 seconds for response
        while (false == LockUnlockGetCLCCRespVar() && nLoopValue < 15)
        {
            sleep(1);
            nLoopValue++;
        }
        LockUnlockSetCLCCRespVar(false);
    }

    if(!bSwap && HFActObj.m_nCLCCRespValue >= 2 && (BSA_HS_CALLHELD_ACTIVE_HELD == m_nCallHeldIndValue))
    {
        // It is a conference call since multiple lines
        // are connected and one call is on hold
        if(0==app_hs_hold_call(BTHF_CHLD_TYPE_ADDHELDTOCONF))
        {
        }
    }
    else
    // Includes swapping the call also
    if(0==app_hs_hold_call(BTHF_CHLD_TYPE_HOLDACTIVE_ACCEPTHELD))
    {
    }
}

// Set up the list header
void BSA::SetupListHeader(QStringList list)
{
    ui->tblDispList->setHorizontalHeaderLabels(list);
    ui->tblDispList->clearContents();
}

// Setup the CLCC Command response variable as needed
void BSA::LockUnlockSetCLCCRespVar(bool bValue)
{
    m_CLCCRespLock.lockForWrite();
    m_bCLCCCmdTotRespObt = bValue;
    m_CLCCRespLock.unlock();
}

// Get the CLCC Command response variable as needed
bool BSA::LockUnlockGetCLCCRespVar()
{
    bool bRetVal= false;
    m_CLCCRespLock.lockForWrite();
    bRetVal = m_bCLCCCmdTotRespObt;
    m_CLCCRespLock.unlock();
    return bRetVal;
}

void BSA::LockUnlockSetCLCCTotCmdVar(bool bValue)
{
    m_CLCCTotCmdLock.lockForWrite();
    m_bCLCCCmdSentForTotal = bValue;
    m_CLCCTotCmdLock.unlock();
}

// Get the CLCC total command sent variable
bool BSA::LockUnlockGetCLCCTotCmdVar()
{
    bool bRetVal= false;
    m_CLCCTotCmdLock.lockForWrite();
    bRetVal = m_bCLCCCmdSentForTotal;
    m_CLCCTotCmdLock.unlock();
    return bRetVal;
}

// call on BSA_HS_CONN_EVT or BSA_HS_CLOSE_EVT event
void BSA::on_HSStateChange(QVariant bConnected)
{
    bool bUp = false;
    if(bConnected.toBool())
        bUp = true;

    if(bUp)
    {
        HsStateChanged(true);
        DefaultVars();

        m_pHFStartTimer->start(1);

    }
    else
    {
        m_pHFStartTimer->stop();

        m_pSendIndTimer->stop();

        m_pReadIndTimer->stop();

        HsStateChanged(false);
        DefaultVars();
    }
}

// Auto connect PBAP and AVK on HF connection
void BSA::on_cbAutoConnect_clicked()
{
   m_bAutoConnect = ui->cbAutoConnect->isChecked();
}


// HF Callback events with data
void HsCallback(tBSA_HS_EVT event, tBSA_HS_MSG *p_data)
{
    char buf[100];

    switch (event)
    {
        case BSA_HS_CONN_EVT:       /* Service level connection */
        {
            emit gThis->m_uiSignal->HSStateConnected(TRUE);

            break;
        }

        case BSA_HS_CLOSE_EVT:      /* Connection Closed (for info)*/
        {
            emit gThis->m_uiSignal->HSStateConnected(FALSE);
            break;
        }

        case BSA_HS_OPEN_EVT:
        {
            // If auto connect is enabled, start AVK connection timer on HF callback (whether or not HF connection
            // is successful.
            if(gThis->m_bAutoConnect)
                gThis->m_pAvkConnectTimer->start(4000);

            if(p_data->open.status != BSA_SUCCESS)
            {
                gThis->ui->btnConnectHS->setText("Connect");
                emit gThis->m_uiSignal->DisplayMessageBox("Failed to connect HS service.");
            }
            else
            {
                // If autoconnect is enabled, start PBAP connection when HF connection succeeds
                if(gThis->m_bAutoConnect)
                    gThis->m_pPbapConnectTimer->start(6000);
            }
            break;
        }
        case BSA_HS_AUDIO_OPEN_EVT:
        {
            gThis->AudioStateChanged(true);
            gThis->ui->btnStartVR->setEnabled(true);
            if(gThis->m_bAnswerCmdSent)
            {
                gThis->m_bAnswerCmdSent=false;
            }
            break;
        }

        case BSA_HS_AUDIO_CLOSE_EVT:
        {
            gThis->AudioStateChanged(false);
            break;
        }

        case BSA_HS_CCWA_EVT:
        {
            gThis->HandleRingnCallWaitEvent(true,p_data->val.str);
            break;
        }

        case BSA_HS_RING_EVT:
        {
            QString strNum = "";
            gThis->HandleRingnCallWaitEvent(false,strNum);
            break;
        }

        case BSA_HS_VGM_EVT:
        {
            printf("BSA_HS_VGM_EVT Microphone volume event: %d\n",p_data->val.num);
            break;
        }

        case BSA_HS_VGS_EVT:
        {
            printf("BSA_HS_VGS_EVT Speaker volume event: %d\n",p_data->val.num);
            break;
        }

        case BSA_HS_CNUM_EVT:
        {
            printf("CNUM Event: %s\n",p_data->val.str);
            HandleATCNUMResponse(p_data->val.str);
            break;
        }

        case BSA_HS_CLCC_EVT:
        {
            printf("CLCC Event: %s\n",p_data->val.str);
            if(!gThis->m_bCLCCCmdSentForTotal)
                HandleATCLCCResponse(false, p_data->val.str);
            else
                HandleATCLCCResponse(true, p_data->val.str);
            break;
        }

        case BSA_HS_UNAT_EVT:
        {
            HandleUNATResponse(p_data->val.str);
            break;
        }

        case BSA_HS_COPS_EVT:
        {
             printf("COPS Event: %s\n",p_data->val.str);
             break;
        }

        case BSA_HS_CLIP_EVT:
        {
            QString strNum = p_data->val.str;
            gThis->HandleRingnCallWaitEvent(false,strNum);
            printf("CLIP Event: %s\n",p_data->val.str);
            break;
        }

        case BSA_HS_BVRA_EVT:
        {
            // Handle the states here
            if(p_data->val.num == 0)
                gThis->HandleVRStateChangeEvent(false);
            if(p_data->val.num == 1)
                gThis->HandleVRStateChangeEvent(true);
            printf("BSA_HS_BVRA_EVT:%d\n", p_data->val.num);
            break;
        }

        case BSA_HS_CHLD_EVT:
        {
            // Handle the 3 way calling event here, if needed.
            break;
        }

        case BSA_HS_OK_EVT:
        {
            fprintf(stdout, "BSA_HS_OK_EVT: command value %d, string: %s\n",p_data->val.num, p_data->val.str);
            if(gThis->m_bAnswerCmdSent && BSA_HS_A_CMD == p_data->val.num)
            {
                gThis->m_bAnswerCmdSent=false;
            }
            else
            if(gThis->m_bDialCmdSent && BSA_HS_D_CMD == p_data->val.num)
            {
                gThis->m_bDialCmdSent=false;
                gThis->HandleDialComplEvent(p_data->val.str);
            }
            else
            if(gThis->m_bLastNumDialCmdSent && BSA_HS_BLDN_CMD == p_data->val.num)
            {
                gThis->m_bLastNumDialCmdSent=false;
                gThis->HandleRedialComplEvent(p_data->val.str);
            }
            else
            if(gThis->m_bCHUPCmdSent && BSA_HS_CHUP_CMD == p_data->val.num)
            {
                gThis->m_bCHUPCmdSent = false;
                gThis->ui->btnHangup->setEnabled(false);
                gThis->ui->btnReject->setEnabled(false);
            }
            else
            if(gThis->m_bUnmuteCmdSent && BSA_HS_UNAT_CMD == p_data->val.num)
                gThis->m_bUnmuteCmdSent = false;
            else
            if(gThis->m_bMuteCmdSent && BSA_HS_UNAT_CMD == p_data->val.num)
                gThis->m_bMuteCmdSent = false;
            else
            if(HFActObj.m_bReadPBCmdSent && BSA_HS_UNAT_CMD == p_data->val.num)
                HFActObj.m_bReadPBCmdSent = false;
            else
            if(gThis->m_bStartVRCmdSent && BSA_HS_BVRA_CMD == p_data->val.num)
            {
                gThis->m_bStartVRCmdSent = false;
                gThis->HandleVRStateChangeEvent(true);
            }
            else
            if(gThis->m_bStopVRCmdSent && BSA_HS_BVRA_CMD == p_data->val.num)
            {
                gThis->m_bStopVRCmdSent = false;
                gThis->HandleVRStateChangeEvent(false);
            }
            else
            if(true == gThis->LockUnlockGetCLCCTotCmdVar() && BSA_HS_CLCC_CMD == p_data->val.num)
            {
                gThis->LockUnlockSetCLCCRespVar(true);
                gThis->LockUnlockSetCLCCTotCmdVar(false);
            }
            HandleATOKResponse();
            break;
        }

        case BSA_HS_CMEE_EVT:
            // Handle enhanced error event here - TBD
            HandleErrors(event,p_data);
            break;

        case BSA_HS_ERROR_EVT:
            HandleErrors(event,p_data);
            break;

        case BSA_HS_CIND_EVT:                /* CIEV event */
        {
            printf("BSA_HS_CIEV_EVT\n");
            memset(buf, 0, 16);
            strncpy(buf, p_data->val.str, 15);
            buf[16] ='\0';


            // check if AVK is connected
            tAPP_AVK_CONNECTION * connection = gThis->GetActiveConnection();
            if(connection != NULL)
            {
                tBSA_HS_CONN_CB *p_conn;
                // check if HF is connected
                if ((p_conn = app_hs_get_default_conn()) != NULL)
                {
                    int result = bdcmp(p_conn->connected_bd_addr, connection->bda_connected);

                    // check if HF and AVK are connected to different devices
                    if (result != 0)
                    {
                        // If connected to different devices ..

                        // If we are in a call, and AVK is playing, pause it

                        char c2 = buf[3];
                        char c4 = buf[5];

                        if((atoi(&c2) != 0) || (atoi(&c4) != 0))
                        {
                            if(gThis->m_uPlayState == AVRC_PLAYSTATE_PLAYING)
                            {
                                gThis->m_bPausedByApp = TRUE;
                                app_avk_play_pause(connection->rc_handle);
                            }
                        }
                        // if we are not in a call and we paused AVK, play it
                        else if(gThis->m_bPausedByApp)
                        {
                            gThis->m_bPausedByApp = FALSE;
                            app_avk_play_start(connection->rc_handle);
                        }

                    }
                }

            }
        }
        break;

        default:
            break;
    }
    fflush(stdout);
}

// On AVK timer, connect AVK
// Connect the same device selected by user for HF connection is list of paired devices.
void BSA::on_timer_connectAVK()
{
    m_pAvkConnectTimer->stop();

    BD_ADDR bd_addr_selected;
    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    if (iRow == -1 || iRow > iCount-1)
    {
        APP_DEBUG0("on_timer_connectAVK, no device selected in paired device list");
        return;
    }
    else
    {
        // Get the paired device selected
        bdcpy(bd_addr_selected, m_paired_device_list[iRow].msg.bd_addr);
}

    if(avk_is_open(bd_addr_selected))
{
        APP_DEBUG0("on_timer_connectAVK, AVK already connected to selected device");
        return;
    }

    if(avk_is_open_pending())
    {
        APP_DEBUG0("on_timer_connectAVK, AVK already connecting");
        return;
    }

    on_connectAVKButton_clicked();
}

// On PBAP timer, connect PBAP
void BSA::on_timer_connectPbap()
{
    m_pPbapConnectTimer->stop();

    BD_ADDR bda_open = {0};

    if(!pbc_is_open_pending() && !pbc_is_open(&bda_open))
        on_btnPBAPConnectDisconnect_clicked();
    else
    {
        APP_DEBUG0("on_timer_connectPbap, PBAP already connected");
    }
}


// Handle HS errors here
void HandleErrors(tBSA_HS_EVT event, tBSA_HS_MSG *p_data)
{
    UINT16 ErrSource=0;

    if(BSA_HS_CMEE_EVT == event)
     // Extract the lower 8 bits
       ErrSource = (UINT16)((p_data->val.num) & 0xFF);
    else
       ErrSource = (p_data->val.num);

    printf("Error event:%d, Value:%d, %s",event,ErrSource, p_data->val.str);

    if(gThis->m_bAnswerCmdSent && BSA_HS_A_CMD == ErrSource)
        gThis->m_bAnswerCmdSent=false;
    else
    if(gThis->m_bDialCmdSent && BSA_HS_D_CMD == ErrSource)
        gThis->m_bDialCmdSent=false;
    else
    if(gThis->m_bLastNumDialCmdSent && BSA_HS_BLDN_CMD == ErrSource)
        gThis->m_bLastNumDialCmdSent=false;
    else
    if(gThis->m_bCHUPCmdSent && BSA_HS_CHUP_CMD == ErrSource)
    {
        gThis->m_bCHUPCmdSent = false;
        gThis->ui->btnHangup->setEnabled(false);
        gThis->ui->btnReject->setEnabled(false);
    }
    else
    if(gThis->m_bMuteCmdSent && BSA_HS_UNAT_CMD == ErrSource)
    {
        gThis->m_bMuteCmdSent = false;
        emit gThis->m_uiSignal->DisplayMessageBox("Failed to mute the device");
    }
    else
    if(gThis->m_bUnmuteCmdSent && BSA_HS_UNAT_CMD == ErrSource)
    {
        gThis->m_bUnmuteCmdSent = false;
        emit gThis->m_uiSignal->DisplayMessageBox("Failed to unmute the device");
    }
    if(HFActObj.m_bReadPBCmdSent && BSA_HS_UNAT_CMD == p_data->val.num)
    {
        HFActObj.m_bReadPBCmdSent = false;
        emit gThis->m_uiSignal->DisplayMessageBox("Failed to read entries from device");
    }
    else
    if(gThis->m_bStartVRCmdSent && BSA_HS_BVRA_CMD == ErrSource)
    {
        gThis->m_bStartVRCmdSent = false;
        gThis->EnableDisableVRButtons(false);
        emit gThis->m_uiSignal->DisplayMessageBox("Voice recognition start failed");
    }
    else
    if(gThis->m_bStopVRCmdSent && BSA_HS_BVRA_CMD == ErrSource)
    {
        gThis->m_bStopVRCmdSent = false;
        gThis->EnableDisableVRButtons(false);
        emit gThis->m_uiSignal->DisplayMessageBox("Voice recognition stop failed");
    }
    else
    if(true == gThis->LockUnlockGetCLCCTotCmdVar() && BSA_HS_CLCC_CMD == p_data->val.num)
    {
        gThis->LockUnlockSetCLCCTotCmdVar(false);
        gThis->LockUnlockSetCLCCRespVar(true);
    }
}

// Populate Phone book table in UI
void BSA::PopulatePBTables(int nTblIndex, PBItem *pItem, QString table_type)
{
    if(NULL==pItem || NULL==ui->tblDispList)
        return;

    QTableWidgetItem *pContactName = new QTableWidgetItem(pItem->m_ContactName);
    QTableWidgetItem *pContactNum = new QTableWidgetItem(pItem->m_ContactNum);
    QTableWidgetItem *pRecType = new QTableWidgetItem(pItem->m_RecType);

    ui->tblDispList->setWindowTitle(table_type);
    ui->tblDispList->setItem(nTblIndex,0,pContactName);
    ui->tblDispList->setItem(nTblIndex,1,pContactNum);
    ui->tblDispList->setItem(nTblIndex,2,pRecType);
}

// Populate CLCC output table in UI
void BSA::PopulateCLCCTables(int nTblIndex, CLCCItem *pItem, QString table_type)
{
    if(NULL==pItem || NULL==ui->tblDispList)
        return;

    QTableWidgetItem *pDetails = new QTableWidgetItem(pItem->m_MultDetails);
    QTableWidgetItem *pNumber = new QTableWidgetItem(pItem->m_Number);
    QTableWidgetItem *pRecType = new QTableWidgetItem(pItem->m_RecType);

    ui->tblDispList->setWindowTitle(table_type);
    ui->tblDispList->setItem(nTblIndex,0,pDetails);
    ui->tblDispList->setItem(nTblIndex,1,pNumber);
    ui->tblDispList->setItem(nTblIndex,2,pRecType);
}

// Populate CNUM output table in UI
void BSA::PopulateCNUMTables(int nTblIndex, CNUMItem *pItem, QString table_type)
{
    if(NULL==pItem || NULL==ui->tblDispList)
        return;

    QTableWidgetItem *pDescr = new QTableWidgetItem(pItem->m_Descr);
    QTableWidgetItem *pNumber = new QTableWidgetItem(pItem->m_Number);
    QTableWidgetItem *pRecType = new QTableWidgetItem(pItem->m_RecType);

    ui->tblDispList->setWindowTitle(table_type);
    ui->tblDispList->setItem(nTblIndex,0,pDescr);
    ui->tblDispList->setItem(nTblIndex,1,pNumber);
    ui->tblDispList->setItem(nTblIndex,2,pRecType);
}

// Populate COPS output table in UI
void BSA::PopulateCOPSTables(int nTblIndex, COPSItem *pItem, QString table_type)
{
    if(NULL==pItem || NULL==ui->tblDispList)
        return;

    QString strDesc= pItem->m_PLMNMode+","+ pItem->m_Format;
    QTableWidgetItem *pDescr = new QTableWidgetItem(strDesc);
    QTableWidgetItem *pStatus = new QTableWidgetItem(pItem->m_Status);
    QTableWidgetItem *pOperator = new QTableWidgetItem(pItem->m_Operator);

    ui->tblDispList->setWindowTitle(table_type);
    ui->tblDispList->setItem(nTblIndex,0,pDescr);
    ui->tblDispList->setItem(nTblIndex,1,pStatus);
    ui->tblDispList->setItem(nTblIndex,2,pOperator);
}

void BSA::on_Timer_Timeout()
{
    if(NULL!=ui->btnReadPB && !ui->btnReadPB->isEnabled())
    {
        ui->btnReadPB->setEnabled(true);
        QString st("Could not obtain response from connected device. Please disconnect the device and reconnect.");
        QMessageBox msg;
        msg.setText(st);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.exec();
    }
}


void BSA::on_tblPairedDevices_clicked(const QModelIndex &index)
{

}

/*************************************************************************************/
/********************* PBAP Client UI ************************************************/
/*************************************************************************************/

void BSA::InitializePBAPSettings()
{
    ui->cmbPath->clear();
    ui->cmbSearch->clear();
    ui->cmbSort->clear();

    QString val;

    val = "telecom/pb";
    ui->cmbPath->addItem("PB_PATH",QVariant::fromValue(val));
    val = "telecom/ich";
    ui->cmbPath->addItem("ICH_PATH",QVariant::fromValue(val));
    val = "telecom/och";
    ui->cmbPath->addItem("OCH_PATH",QVariant::fromValue(val));
    val = "telecom/mch";
    ui->cmbPath->addItem("MCH_PATH",QVariant::fromValue(val));
    val = "telecom/cch";
    ui->cmbPath->addItem("CCH_PATH",QVariant::fromValue(val));

    val = "SIM1/telecom/pb";
    ui->cmbPath->addItem("SIM_PB_PATH",QVariant::fromValue(val));
    val = "SIM1/telecom/ich";
    ui->cmbPath->addItem("SIM_ICH_PATH",QVariant::fromValue(val));
    val = "SIM1/telecom/och";
    ui->cmbPath->addItem("SIM_OCH_PATH",QVariant::fromValue(val));
    val = "SIM1/telecom/mch";
    ui->cmbPath->addItem("SIM_MCH_PATH",QVariant::fromValue(val));
    val = "SIM1/telecom/cch";
    ui->cmbPath->addItem("SIM_CCH_PATH",QVariant::fromValue(val));

    /* Add items to sort order */

    ui->cmbSort->addItem("PBC_ORDER_INDEXED",QVariant(BSA_PBC_ORDER_INDEXED));

    ui->cmbSort->addItem("PBC_ORDER_ALPHANUM",QVariant(BSA_PBC_ORDER_ALPHANUM));

    ui->cmbSort->addItem("PBC_ORDER_PHONETIC",QVariant(BSA_PBC_ORDER_PHONETIC));


    /* Add items to search */
    ui->cmbSearch->addItem("PBC_ATTR_NAME",QVariant(BSA_PBC_ATTR_NAME));

    ui->cmbSearch->addItem("PBC_ATTR_NUMBER",QVariant(BSA_PBC_ATTR_NUMBER));

    ui->cmbSearch->addItem("BSA_PBC_ATTR_SOUND",QVariant(BSA_PBC_ATTR_SOUND));
}


void BSA::on_btnPhonebook_clicked()
{
    DefaultVisibility();
    ui->devicePhonebookFrame->setVisible(true);

    BD_ADDR bda_open;
    BOOLEAN isOpen = pbc_is_open(&bda_open);

    PbcStateChanged(isOpen ?  TRUE : FALSE);

    IsAnyDeviceConnected(CONN_PROFILE_PBC);
}

/* Update UI state */
void BSA::PbcStateChanged(BOOLEAN bConnected)
{
    if(bConnected)
    {
        ui->btnPBAPConnectDisconnect->setText("Disconnect");
    }
    else
    {
        ui->btnPBAPConnectDisconnect->setText("Connect");
        ui->lblStatus->setText("");
        ui->lstObjects->clear();
    }

    bool bEnable = bConnected ? true : false;
    ui->btnGetPhonebook->setEnabled(bEnable);
    ui->btnListVCards->setEnabled(bEnable);
    ui->btnSetPath->setEnabled(bEnable);
    ui->btnGetContacts->setEnabled(bEnable);
    ui->btnListICHVCards->setEnabled(bEnable);
    ui->btnListOCHVCards->setEnabled(bEnable);
    ui->btnListMCHVCards->setEnabled(bEnable);
}

/* Connect PBC */
void BSA::on_btnPBAPConnectDisconnect_clicked()
{
    BOOLEAN bConnected = FALSE;
    BD_ADDR bda_open = {0};

    bConnected = pbc_is_open(&bda_open);

    /* Disconnect PBC connection */
    if(bConnected)
    {
        app_pbc_close();

        ui->edtSearchValue->clear();
        ui->lstObjects->clear();
        ui->lblStatus->setText("");
        ui->edtPbcResult->clear();

        return;
    }

    if (pbc_is_open_pending())
    {
        APP_ERROR0("already trying to connect");

        QString s = ui->btnPBAPConnectDisconnect->text();
        if(s == "Canceling ...")
            return;

        ui->btnPBAPConnectDisconnect->setText("Canceling ...");
        app_pbc_cancel();
        return;
    }

    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    BD_ADDR bd_addr;
    UINT8 *p_name = NULL;
    BOOLEAN bIsHfConnected = FALSE;
    p_name = GetConnectedHFDevice(bd_addr, bIsHfConnected);

    // Check if HF is connected, if so we will use this device
    // If not, check if valid device is selected, if not show message
    if (!bIsHfConnected && ((iRow == -1 || iRow > iCount-1)))
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
        return;
    }

    if(!bIsHfConnected)
    {
        bdcpy(bd_addr, m_paired_device_list[iRow].msg.bd_addr);
        p_name = m_paired_device_list[iRow].msg.name;
    }

    ui->lblStatus->setVisible(false);

    pbc_set_open_pending(TRUE);

    ui->btnPBAPConnectDisconnect->setText("Cancel");

    tBSA_STATUS status = app_pbc_open(bd_addr);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to connect to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
            bd_addr[0], bd_addr[1], bd_addr[2],
            bd_addr[3], bd_addr[4], bd_addr[5], status);

        pbc_set_open_pending(FALSE);
        ui->btnPBAPConnectDisconnect->setText("Connect");
    }

}

void BSA::ProcessPBAPListData(const char* filename)
{
    APP_DEBUG1("ProcessPBAPListData filename = %s", filename);

    QListWidget *plst =  ui->lstObjects;

    PBAPListXmlStreamReader reader(plst);
    reader.readFile(filename);
}

/*
Display new incoming listing data as it arrives.
This function is triggered in response to the signal.
*/

void BSA::on_BSA_pbap_data_display(QString st, QVariant var)
{
    APP_DEBUG1("on_BSA_pbap_data_display g_TempFile = 0x%x", g_TempFile);
    APP_DEBUG1("on_BSA_pbap_data_display string %s",st.toStdString().c_str());

    //Show RAW data as it arrives
    ui->edtPbcResult->appendPlainText(st);
    ui->edtPbcResult->show();

    //Write out to the temporary file
    QTextStream out(g_TempFile);
    out << st.toStdString().c_str();

    int final = var.toInt();

    // Final indicates that the listing is complete
    if(final)
    {
        g_TempFile->close();

        QString tempPath(GetCurrentWorkingDirectory());
        tempPath += "/temp.xml";

        ProcessPBAPListData(tempPath.toStdString().c_str());
    }
}

/*
Read contents from the downloaded phonebook object and display it in the UI.
This function is triggered in response to the signal.
*/

void BSA::on_BSA_pbap_show_pb_data()
{
    QString path;
    path = ui->cmbPath->itemData(ui->cmbPath->currentIndex()).toString();
    path += ".vcf";

    int len = path.length();
    int index = path.lastIndexOf('/');

    QString str(GetCurrentWorkingDirectory());
    //str += path.right(len - index);
    str += "/pb_data.vcf";

    ui->lstObjects->clear();

    QFile file(str);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;


    // Show RAW data
    QTextStream in(&file);
    while(!in.atEnd()){
        QString line = in.readLine();
        ui->edtPbcResult->appendPlainText(line);
        ui->edtPbcResult->show();
    }

    // Now we can parse it...
    QList<vCard> vcards = vCard::fromFile(str);

    if (!vcards.isEmpty())
    {
        for(int idx = 0; idx < vcards.size(); idx++)
        {
            vCard vcard = vcards.at(idx);
            vCardProperty name_prop = vcard.property(VC_FORMATTED_NAME);
            vCardProperty tel_prop = vcard.property(VC_TELEPHONE);

            QString output;
            QStringList values = name_prop.values();
            QString firstname = vcard.properties().at(vCardProperty::Firstname).value();
            QString lastname = vcard.properties().at(vCardProperty::Lastname).value();

            for(int pidx = 0; pidx < vcard.properties().size(); pidx++)
            {
                QString name = vcard.properties().at(pidx).name();

                if(vcard.properties().at(pidx).name() == VC_FORMATTED_NAME ||
                    vcard.properties().at(pidx).name() == VC_TELEPHONE ||
                    vcard.properties().at(pidx).name() == VC_ADDRESS  )
                {
                    vCardProperty prop = vcard.properties().at(pidx);

                    if(prop.name() == VC_TELEPHONE)
                    {
                        for(int i = 0; i < prop.params().size(); i++)
                        {
                            QString tempProp  = prop.params().at(i).value();
                            if(tempProp.length())
                            {
                                output +=  tempProp;
                                output += '\t';
                            }
                        }
                    }

                    for(int i = 0; i < prop.values().size(); i++)
                    {
                        QString tempVal = prop.values().at(i);
                        if(tempVal.length())
                        {
                            output +=  tempVal;
                            output += '\n';
                        }
                    }
                }
            }

            QListWidgetItem *newItem = new QListWidgetItem;
            newItem->setText(output);
            ui->lstObjects->addItem(newItem);
        }
    }
}

/*  PBC callback to handle events received from BSA PBAP API */
void PbcCallback(tBSA_PBC_EVT event, tBSA_PBC_MSG *p_data)
{
    switch (event)
    {
    case BSA_PBC_SET_EVT:
        if (p_data->set.status == BSA_SUCCESS)
        {
            gThis->SetNextPath();
        }
        else
        {
            gThis->ui->lblStatus->setVisible(true);
            gThis->ui->lblStatus->setText(MSG_SET_PATH_FAILED);
            gThis->m_listPath.clear();
        }
        break;

    case BSA_PBC_GET_EVT:
        {
            APP_DEBUG1("BSA_PBC_GET_EVT status 0x%x", p_data->get.status);
            switch(p_data->get.type)
            {
            case BSA_PBC_GET_PARAM_STATUS: /* status only*/
                break;

            case BSA_PBC_GET_PARAM_LIST:            /* List message */
                {
                    if(p_data->get.status == BSA_SUCCESS)
                    {
                        UINT8* p_buf  =  p_data->get.param.list.data;
                        int len = p_data->get.param.list.len;
                        APP_DEBUG1("BSA_PBC_GET_PARAM_LIST len %d", len);

                        char* pbuffer = new char[len+1];
                        memset(pbuffer, 0, len + 1);
                        memcpy(pbuffer, p_buf, len);

                        int final = p_data->get.param.list.final;
                        /* Send a signal to display data*/
                        QString st(pbuffer);

                        emit gThis->m_uiSignal->UpdatePBAPDataDisplay(st, QVariant(final));

                        delete[] pbuffer;
                    }
                    else
                    {
                        QString st;
                        st.sprintf("Status : %d",p_data->get.status );
                        emit gThis->m_uiSignal->DisplayStatus(st);
                    }
                }
                break;

            case BSA_PBC_GET_PARAM_PROGRESS:        /* Progress Message*/
                break;

            case BSA_PBC_GET_PARAM_PHONEBOOK:       /* Phonebook param*/
                APP_DEBUG1("Phonebook Transfer Status : %d",p_data->get.status);
                emit gThis->m_uiSignal->ShowPhonebookData();
                break;

            case BSA_PBC_GET_PARAM_FILE_TRANSFER_STATUS:       /* File Transfer status*/
                APP_DEBUG1("File Transfer Status : %d",p_data->get.status);
                break;

            default:
                break;
            }
        }
        break;

    case BSA_PBC_OPEN_EVT: /* open event */
        APP_DEBUG1("BSA_PBC_OPEN_EVT status 0x%x", p_data->open.status);

        if (p_data->open.status == BSA_SUCCESS)
        {
            APP_DEBUG1("BSA_PBC_OPEN_EVT gThis 0x%x", gThis);
            gThis->PbcStateChanged(TRUE);

            // If autoconnect is enabled, download phone book when PBAP is opened
            if(gThis->m_bAutoConnect)
                emit gThis->m_uiSignal->PhoneBookDownload();
        }
        else
        {
            gThis->PbcStateChanged(FALSE);
        }
        break;

    case BSA_PBC_DISABLE_EVT: /* disable event */
    case BSA_PBC_CLOSE_EVT: /*  close event */

        gThis->PbcStateChanged(FALSE);
        break;

    default:
        APP_ERROR1("Unsupported event %d", event);
        break;
    }
}

char * BSA::GetCurrentWorkingDirectory()
{
    static char buf[260];
    memset(buf, 0, sizeof(buf));

    char* pStr = getcwd(buf, 260);

    return buf;
}

/* Get Phonebook */
void BSA::on_btnGetPhonebook_clicked()
{
    QString path;
    path = ui->cmbPath->itemData(ui->cmbPath->currentIndex()).toString();
    path += ".vcf";

    app_pbc_get_phonebook(path.toStdString().c_str(), 0, 0, 0);
}

/* Get Card listing */
void BSA::GetPBCVCardList(int idxListType)
{
    int order = ui->cmbSort->itemData(ui->cmbSort->currentIndex()).toInt();
    int attr = ui->cmbSearch->itemData(ui->cmbSearch->currentIndex()).toInt();

    QString searchvalue;
    searchvalue = ui->edtSearchValue->text();

    int max_list_count = 65535; /* Use as default */
    int tempmlc = ui->edtMaxListCount->text().toInt();

    if(tempmlc > 0 && tempmlc <= 65535)
        max_list_count = tempmlc;

    ui->lblStatus->setText("");
    ui->edtPbcResult->clear();

    if(g_TempFile)
        g_TempFile->close();

    if(g_TempFile)
        delete g_TempFile;

    g_TempFile = NULL;

    QString tempPath(GetCurrentWorkingDirectory());
    tempPath += "/temp.xml";

    QFile::remove(tempPath.toStdString().c_str());

    g_TempFile = new QFile(tempPath.toStdString().c_str());

    if (g_TempFile && !g_TempFile->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        APP_DEBUG1("on_btnListVCards_clicked g_TempFile OPEN FAILED = 0x%x", g_TempFile);
        return;
    }

    app_pbc_get_list_vcards("", order, attr, searchvalue.toStdString().c_str(),
            max_list_count, 0, 0, 0);
}

/* Set next path folder */
void BSA::SetNextPath()
{
    if (!m_listPath.isEmpty())
    {
        app_pbc_set_chdir(m_listPath.first().toStdString().c_str());
        m_listPath.removeFirst();
    }
    else
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SET_PATH_SUCCESS);
    }
}

void BSA::on_btnListVCards_clicked()
{
    int idxListType = ui->cmbPath->currentIndex() ;
    GetPBCVCardList(idxListType);
}

/* Change to specified directory */
void BSA::on_btnSetPath_clicked()
{
    QString path;
    path = ui->cmbPath->itemData(ui->cmbPath->currentIndex()).toString();

    m_listPath = path.split("/", QString::SkipEmptyParts);

    /* Set to root first */
    app_pbc_set_chdir("");
}

/* Get Phonebook */
void BSA::on_btnGetContacts_clicked()
{
    QString path;
    path = ui->cmbPath->itemData(ui->cmbPath->currentIndex()).toString();
    path += ".vcf";

    app_pbc_get_phonebook(path.toStdString().c_str(), 0, 0, 0);
}

/* Get Incoming call history */
void BSA::on_btnListICHVCards_clicked()
{
    GetPBCVCardList(ICH_LIST);
}

/* Get Outgoing call history */
void BSA::on_btnListOCHVCards_clicked()
{
    GetPBCVCardList(OCH_LIST);
}

/* Get Missed call history */
void BSA::on_btnListMCHVCards_clicked()
{
    GetPBCVCardList(MCH_LIST);
}


/*************************************************************************************/
/********************* MAP Client UI *************************************************/
/*************************************************************************************/

void BSA::InitializeMceSettings()
{
    m_eMceMode = eUserMode;

    ui->cmbMceInstances->clear();
    ResetMceFolderList();
    ui->lstMceMessages->clear();
    ui->edtMceResult->clear();
    ui->edtMceMessage->clear();

    bMnsStarted = false;

    //Message Listing
    /* Add items for read status*/
    ui->cmbMceReadStatus->addItem("--",QVariant(BSA_MCE_READ_STATUS_NO_FILTERING));
    ui->cmbMceReadStatus->addItem("Read",QVariant(BSA_MCE_READ_STATUS_READ));
    ui->cmbMceReadStatus->addItem("Unread",QVariant(BSA_MCE_READ_STATUS_UNREAD));

    /* Add items for priority*/
    ui->cmbMcePriority->addItem("--",QVariant(BSA_MCE_PRI_STATUS_NO_FILTERING));
    ui->cmbMcePriority->addItem("Normal",QVariant(BSA_MCE_PRI_STATUS_NON_HIGH));
    ui->cmbMcePriority->addItem("High",QVariant(BSA_MCE_PRI_STATUS_HIGH));

    //Message Status
    /* Add items for set status indicator*/
    ui->cmbMceStatusInd->addItem("Read",QVariant(BSA_MCE_STS_INDTR_READ));
    ui->cmbMceStatusInd->addItem("Delete",QVariant(BSA_MCE_STS_INDTR_DELETE));

    /* Add items for set status value*/
    ui->cmbMceStatusValue->addItem("No",QVariant(BSA_MCE_STS_VALUE_NO));
    ui->cmbMceStatusValue->addItem("Yes",QVariant(BSA_MCE_STS_VALUE_YES));

    /* User mode */
    m_eMceNextAction = eMceActionNone;
    m_listFolders.clear();
}

void BSA::on_btnMapClient_clicked()
{
    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    /* Check if valid device is selected */
    if (iRow == -1 || iRow > iCount-1)
    {
        m_bDevSelected = false;
    }
    else
    {
        bdcpy(m_SelectedDevBda, m_paired_device_list[iRow].msg.bd_addr);
        m_bDevSelected = true;
    }

    BD_ADDR bda_open;
    BOOLEAN isOpen = mce_is_open(&bda_open);
    bool bDeviceConnected = false;

    /* Check if MAP is already connected */
    if(isOpen)
    {
        if (m_bDevSelected)
        {
            /*  check if connected device is the same as selected device */
            if(bdcmp(bda_open, m_SelectedDevBda) == 0)
            {
                bDeviceConnected = true;
            }
        }
    }

    DefaultVisibility();
    if (m_eMceMode == eTestMode)
    {
        ui->deviceMessagesFrame->setVisible(true);

        ui->edtMceMessage->setPlainText("Hello from mce client");

        if (!m_bDevSelected ||(isOpen && !bDeviceConnected))
            ui->cmbMceInstances->clear();

        on_cmbMceInstances_currentIndexChanged(ui->cmbMceInstances->currentIndex());
    }
    else
    {
        ui->deviceMceUserFrame->setVisible(true);

        MceUserResetUI(true);

        if (m_bDevSelected &&
            (!isOpen || bDeviceConnected))
        {
            app_mce_get_mas_instances(m_SelectedDevBda);
        }
    }

    IsAnyDeviceConnected(CONN_PROFILE_MCE);
}

/*  MCE callback to handle events received from BSA MCE API */
void MceCallback(tBSA_MCE_EVT event, tBSA_MCE_MSG *p_data)
{
    if (gThis->m_eMceMode == gThis->eTestMode)
    {
        switch (event)
        {
        case BSA_MCE_FOLDER_LIST_EVT:
        case BSA_MCE_MSG_LIST_EVT:
        {
            APP_DEBUG1("BSA_MCE_GET_EVT status 0x%x", p_data->list_data.status);
            if(p_data->list_data.status == BSA_SUCCESS)
            {
                UINT8* p_buf  =  p_data->list_data.data;
                int len = p_data->list_data.len;
                APP_DEBUG1("BSA_MCE_GET_PARAM_LIST len %d", len);

                char* pbuffer = new char[len+1];
                memset(pbuffer, 0, len + 1);
                memcpy(pbuffer, p_buf, len);

                int final = p_data->list_data.is_final;
                /* Send a signal to display data*/
                QString st(pbuffer);

                emit gThis->m_uiSignal->UpdateMceDataDisplay(QVariant(event), st, QVariant(final));

                delete[] pbuffer;
}
            else
            {
                QString st;
                st.sprintf("Status : %d",p_data->list_data.status );
                emit gThis->m_uiSignal->DisplayStatus(st);
            }
        }
        break;

        case BSA_MCE_NOTIF_EVT:
        {
            if(gThis->g_TempFileName != "tempeventreport.xml")
            {
                gThis->g_TempFileName = "tempeventreport.xml";
                gThis->InitializeTempFile("tempeventreport.xml");
            }

            APP_DEBUG1("BSA_MCE_GET_EVT status 0x%x", p_data->notif.status);
            if(p_data->notif.status == BSA_SUCCESS)
            {
                UINT8* p_buf  =  p_data->notif.data;
                int len = p_data->notif.len;
                APP_DEBUG1("BSA_MCE_GET_PARAM_LIST len %d", len);

                if(len < 0 || len > BSA_MCE_MN_MAX_MSG_EVT_OBECT_SIZE)
                    break;

                char* pbuffer = new char[len+1];
                memset(pbuffer, 0, len + 1);
                memcpy(pbuffer, p_buf, len);

                int final = p_data->notif.final;
                /* Send a signal to display data*/
                QString st(pbuffer);

                emit gThis->m_uiSignal->UpdateMceDataDisplay(QVariant(event), st, QVariant(final));

                delete[] pbuffer;
            }
            else
            {
                QString st;
                st.sprintf("Status : %d",p_data->notif.status );
                emit gThis->m_uiSignal->DisplayStatus(st);
            }
        }
        break;

        case BSA_MCE_SET_FOLDER_EVT:
        {
            QString st;
            if(p_data->set_folder.status == BSA_SUCCESS)
            {
                st = "Set folder succeded";
                gThis->ResetMceFolderList();
            }
            else
                st = "Set folder failed";

            emit gThis->m_uiSignal->DisplayStatus(st);

        }
        break;

        case BSA_MCE_START_EVT:
            emit gThis->m_uiSignal->DisplayStatus("Notification Server Started");
            gThis->MceMnStateChanged(true);
            break;

        case BSA_MCE_STOP_EVT:
            emit gThis->m_uiSignal->DisplayStatus("Notification Server Stopped");
            gThis->MceMnStateChanged(false);
            break;

        case BSA_MCE_MN_OPEN_EVT:
            if(p_data->mn_open.status == BSA_SUCCESS)
            {
                emit gThis->m_uiSignal->DisplayStatus("MAP Notification client connected");
                gThis->MceNotifRegStateChanged(true);
            }
            break;

        case BSA_MCE_MN_CLOSE_EVT:
            if(p_data->mn_close.status == BSA_SUCCESS)
            {
                emit gThis->m_uiSignal->DisplayStatus("MAP Notification client disconnected");
            }
            break;


        case BSA_MCE_GET_MSG_EVT:
        {
            printf("BSA_MCE_GET_MSG_EVT\n");
            //QString st(p_data->getmsg_msg.msg_name);
            //Save incoming message content in temporary file
            QString st("get_msg.txt");

            QString stpath(gThis->GetCurrentWorkingDirectory());
            stpath = stpath + "/" + st;

            emit gThis->m_uiSignal->ShowMceData(stpath);
        }
        break;

        case BSA_MCE_OPEN_EVT: /* open event */
            APP_DEBUG1("BSA_MCE_OPEN_EVT status 0x%x", p_data->open.status);

            if (p_data->open.status == BSA_SUCCESS)
            {
                APP_DEBUG1("BSA_MCE_OPEN_EVT gThis 0x%x", gThis);
                gThis->MceStateChanged(TRUE);
                gThis->IsAnyDeviceConnected(CONN_PROFILE_MCE);
            }
            else
            {
                gThis->MceStateChanged(FALSE);
            }

            break;

        case BSA_MCE_DISABLE_EVT: /* disable event */
            gThis->MceStateChanged(FALSE);
            break;

        case BSA_MCE_CLOSE_EVT: /*  close event */
            gThis->MceStateChanged(FALSE);
            if(!mce_is_open(NULL))
            {
                app_mce_mn_stop();
                gThis->IsAnyDeviceConnected(CONN_PROFILE_ANY);
            }
            break;

        case BSA_MCE_GET_MAS_INSTANCES_EVT:
        {
            printf("BSA_MCE_GET_MAS_INSTANCES_EVT\n");

            int cnt = p_data->mas_instances.count;
            int i = 0;
            printf("Number of Instances: %d \n", cnt);

            gThis->m_listMapInst.clear();
            for(i = 0; i < cnt; i++)
            {
                MapInstance *instance = new MapInstance;
                instance->m_iInstId = p_data->mas_instances.mas_ins[i].instance_id;
                instance->m_iMsgType = p_data->mas_instances.mas_ins[i].msg_type;
                instance->m_bMceRegNotif = false;
                gThis->m_listMapInst.append(*instance);

                QString val;
                val.sprintf("%d", i);

                gThis->ui->cmbMceInstances->addItem((const char*) p_data->mas_instances.mas_ins[i].instance_name, QVariant::fromValue(val));
            }
            if (gThis->m_iSelectedInst >= 0)
                gThis->ui->cmbMceInstances->setCurrentIndex(gThis->m_iSelectedInst);
            else
                gThis->ui->cmbMceInstances->setCurrentIndex(0);
        }
        break;

        default:
            APP_ERROR1("Unsupported event %d", event);
            break;
        }
    }
    else /* User mode */
    {
        switch (event)
        {
        case BSA_MCE_GET_MAS_INSTANCES_EVT:
            gThis->m_listMapInst.clear();
            if (p_data->mas_instances.status == BSA_SUCCESS)
            {
                for(int i = 0; i < p_data->mas_instances.count; i++)
                {
                    MapInstance *instance = new MapInstance;
                    instance->m_iInstId = p_data->mas_instances.mas_ins[i].instance_id;
                    instance->m_iMsgType = p_data->mas_instances.mas_ins[i].msg_type;
                    instance->m_bMceRegNotif = false;
                    gThis->m_listMapInst.append(*instance);

                    gThis->ui->cmbMceUserInsts->addItem(
                            (const char*) p_data->mas_instances.mas_ins[i].instance_name,
                            QVariant(i));
                }
                gThis->ui->cmbMceUserInsts->setCurrentIndex(0);
            }
            break;

        case BSA_MCE_OPEN_EVT: /* open event */
            if (p_data->open.status == BSA_SUCCESS)
            {
                emit gThis->m_uiSignal->MceUserStateChanged(true);
                gThis->IsAnyDeviceConnected(CONN_PROFILE_MCE);
            }
            else
            {
                emit gThis->m_uiSignal->MceUserStateChanged(false);
            }

            break;

        case BSA_MCE_CLOSE_EVT: /*  close event */
        case BSA_MCE_DISABLE_EVT: /* disable event */
            emit gThis->m_uiSignal->MceUserStateChanged(false);
            if(!mce_is_open(NULL))
            {
                app_mce_mn_stop();
                gThis->IsAnyDeviceConnected(CONN_PROFILE_ANY);
            }
            break;

        case BSA_MCE_SET_FOLDER_EVT:
            if (p_data->set_folder.status == BSA_SUCCESS)
            {
                gThis->MceSetNextFolder();
            }
            else
            {
                emit gThis->m_uiSignal->DisplayStatus(MSG_SET_PATH_FAILED);
                gThis->m_listFolders.clear();
                gThis->m_eMceNextAction = gThis->eMceActionNone;
            }
            break;

        case BSA_MCE_FOLDER_LIST_EVT:
        case BSA_MCE_MSG_LIST_EVT:
            if(p_data->list_data.status == BSA_SUCCESS)
            {
                char* pbuffer = new char[p_data->list_data.len + 1];
                memcpy(pbuffer, p_data->list_data.data, p_data->list_data.len);
                pbuffer[p_data->list_data.len] = 0;

                emit gThis->m_uiSignal->UpdateMceDataDisplay(QVariant(event), QString(pbuffer),
                        QVariant(p_data->list_data.is_final));

                delete[] pbuffer;

                if (event == BSA_MCE_FOLDER_LIST_EVT)
                {
                    gThis->m_eMceNextAction = gThis->eMceActionListMessage;
                    app_mce_set_folder(gThis->m_listMapInst[gThis->m_iMapIndex].m_iInstId,
                            "inbox", BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL);
                    gThis->MceUserSetMsgType(gThis->eMsgTypeIncoming);
                }
                else
                {
                    if(p_data->list_data.is_final)
                    {
                        gThis->ui->btnMceUserRefresh->setEnabled(true);

                        if (!mce_is_mns_started())
                            app_mce_mn_start();
                    }
                }
            }
            else
            {
                QString st;
                st.sprintf("Status : %d",p_data->list_data.status );
                emit gThis->m_uiSignal->DisplayStatus(st);
            }
            break;

        case BSA_MCE_GET_MSG_EVT:
            emit gThis->m_uiSignal->ShowMceData(QString(gThis->GetCurrentWorkingDirectory()) +
                    QString("/") + QString("get_msg.txt"));
            break;

        case BSA_MCE_PUSH_MSG_EVT:
            if(p_data->pushmsg_msg.status == BSA_SUCCESS)
            {
                emit gThis->m_uiSignal->DisplayStatus("Message has been sent");
                gThis->ui->edtMceUserTo->clear();
                gThis->ui->edtMceUserSubj2->clear();
                gThis->ui->edtMceUserPushMsg->clear();
            }
            else
                emit gThis->m_uiSignal->DisplayStatus("Failed to send message");

            gThis->m_listFolders.clear();
            gThis->m_listFolders.append(gThis->ui->cmbMceUserFolders->currentText());
            gThis->m_eMceNextAction = gThis->eMceActionNoOp;
            app_mce_set_folder(gThis->m_listMapInst[gThis->m_iMapIndex].m_iInstId,
                    "", BSA_MCE_DIR_NAV_UP_ONE_LVL);
            break;

        case BSA_MCE_START_EVT:
            if (p_data->mn_start.status == BSA_SUCCESS)
            {
                emit gThis->m_uiSignal->DisplayStatus("MAP Notification Server Started");
                app_mce_notif_reg(gThis->m_listMapInst[gThis->m_iMapIndex].m_iInstId, TRUE);
            }
            break;

        case BSA_MCE_STOP_EVT:
            emit gThis->m_uiSignal->DisplayStatus("MAP Notification Server Stopped");
            break;

        case BSA_MCE_MN_OPEN_EVT:
            emit gThis->m_uiSignal->DisplayStatus("MAP Notification client connected");
            break;

        case BSA_MCE_MN_CLOSE_EVT:
            emit gThis->m_uiSignal->DisplayStatus("MAP Notification client disconnected");
            break;

        case BSA_MCE_NOTIF_EVT:
            if (p_data->notif.status == BSA_SUCCESS)
            {
                char* pbuffer = new char[p_data->notif.len + 1];
                memcpy(pbuffer, p_data->notif.data, p_data->notif.len);
                pbuffer[p_data->notif.len] = 0;
                emit gThis->m_uiSignal->DisplayStatus(gThis->MceNotifToString(pbuffer));
                delete[] pbuffer;
            }
            break;

        default:
            APP_ERROR1("Unsupported event %d", event);
            break;
        }
    }
}

void BSA::McePushMessage(QString strTo, QString strSubject, QString strMsg)
{
    QString filepath(GetCurrentWorkingDirectory());
    filepath += "/tempoutmsg.txt";

    QFile::remove(filepath);

    bool isEmail = (m_listMapInst[m_iMapIndex].m_iMsgType & BSA_MCE_MSG_TYPE_EMAIL) ? true : false;

    QString stparam = "";

    // Create a BMSG to carry the message
    void* pbmsg = BMessageManager::createBMsg();

    // Set the bmsg type - SMS GSM for example
    if (isEmail)
        BMessageManager::setBMsgMType(pbmsg, BMessageConstants::BTA_MSG_TYPE_EMAIL);
    else
        BMessageManager::setBMsgMType(pbmsg, BMessageConstants::BTA_MSG_TYPE_SMS_GSM);

    // Add envelope to bmsg
    void* penv = BMessageManager::addBMsgEnv(pbmsg);

    // Add body to envelope
    void* pbody = BMessageManager::addBEnvBody(penv);

    // Set character set used
    BMessageManager::setBBodyCharset(pbody, BMessageConstants::BTA_CHARSET_UTF_8);

    // Add recipient to envelop
    void* precip = BMessageManager::addBEnvRecip(penv);

    // Fill out vcard for recipient
    if (isEmail)
        BMessageManager::addBvCardProp(precip, BMessageConstants::BTA_VCARD_PROP_EMAIL,
                strTo, stparam);
    else
        BMessageManager::addBvCardProp(precip, BMessageConstants::BTA_VCARD_PROP_TEL,
                strTo, stparam);

    // Add Content to body
    void* pcontent = BMessageManager::addBBodyCont(pbody);

    // Add Message to content
    if (isEmail)
    {
        QString tmp = "To: " + strTo + "\r\n";
        BMessageManager::addBContMsg(pcontent, tmp);
        tmp = "Subject: " + strSubject + "\r\n";
        BMessageManager::addBContMsg(pcontent, tmp);
        BMessageManager::addBContMsg(pcontent, "\r\n");
    }
    BMessageManager::addBContMsg(pcontent, strMsg);

    // Write out bmsg to the file
    bool retval = BMessageManager::writeBMsgFile(pbmsg,filepath);

    // Call BSA API to push the message to MAP server
    app_mce_push_msg(m_listMapInst[m_iMapIndex].m_iInstId, "");

    // Release BMessage
    if(pbmsg)
        BMessageManager::deleteBMsg(pbmsg);
}


/*************************************************************************************/
/********************* MCE Test Mode *************************************************/
/*************************************************************************************/

/* Update UI state */
void BSA::MceStateChanged(BOOLEAN bConnected)
{
    if(bConnected)
    {
        ui->btnMceConnectDisconnect->setText("Disconnect");
    }
    else
    {
        ui->btnMceConnectDisconnect->setText("Connect");
        ui->lblStatus->setText("");
    }

    ui->btnMceGetMASInstances->setEnabled(true);

    bool bEnable = bConnected ? true : false;

    ui->btnMceGetMASInfo->setEnabled(bEnable);
    ui->btnMceGetMessage->setEnabled(bEnable);
    ui->btnMceListFolders->setEnabled(bEnable);
    ui->btnMceListMessages->setEnabled(bEnable);
    ui->btnMcePushMessage->setEnabled(bEnable);
    ui->btnMceRegUnRegNotification->setEnabled(bEnable);
    ui->btnMceSetMessageStatus->setEnabled(bEnable);
    ui->btnMceSetPath->setEnabled(bEnable);
    ui->btnMceUpdateInbox->setEnabled(bEnable);
    ui->btnMceStartStopMNS->setEnabled(bEnable);
    //ui->btnMceAbort->setEnabled(bEnable);
}

/* Update button text to Start / Stop Message Notification Server */
void BSA::MceMnStateChanged(BOOLEAN bStart)
{
    bMnsStarted = bStart;

    if(bStart)
    {
        ui->btnMceStartStopMNS->setText("Stop MNS");
    }
    else
    {
        ui->btnMceStartStopMNS->setText("Start MNS");
        ui->btnMceRegUnRegNotification->setText("Reg Notif");
        for (int i = 0; i < m_listMapInst.size(); i++)
            m_listMapInst[i].m_bMceRegNotif = false;
    }
}

/* Update button text to Start / Stop Message Notification Server */
void BSA::MceNotifRegStateChanged(BOOLEAN bReg)
{
    m_listMapInst[m_iMapIndex].m_bMceRegNotif = bReg;

    if(bReg)
    {
        ui->btnMceRegUnRegNotification->setText("UnReg Notif");
    }
    else
    {
        ui->btnMceRegUnRegNotification->setText("Reg Notif");
    }
}

void BSA::MceInstanceChanged()
{
    if (m_listMapInst[m_iMapIndex].m_iMsgType & BSA_MCE_MSG_TYPE_EMAIL)
    {
        ui->lbMessage->setText("Email Text");
    }
    else
    {
        ui->lbMessage->setText("Message Text");
    }
    MceNotifRegStateChanged(m_listMapInst[m_iMapIndex].m_bMceRegNotif);
}

//Get Instances
void BSA::on_btnMceGetMASInstances_clicked()
{
    /* Check if valid device is selected, if not show message */
    if (!m_bDevSelected)
    {
        m_uiSignal->DisplayStatus(MSG_SELECT_DEVICE);
        return;
    }

    m_iSelectedInst = ui->cmbMceInstances->currentIndex();
    ui->lblStatus->setVisible(false);
    ui->cmbMceInstances->clear();

    tBSA_STATUS status;
    BD_ADDR bd_addr;
    bdcpy(bd_addr, m_SelectedDevBda);

    status = app_mce_get_mas_instances(bd_addr);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to get mas instances to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5], status);
    }
}

/* Connect MCE */
void BSA::on_btnMceConnectDisconnect_clicked()
{
    /* Check if valid device is selected, if not show message */
    if (!m_bDevSelected)
    {
        m_uiSignal->DisplayStatus(MSG_SELECT_DEVICE);
        return;
    }

    BD_ADDR bda_open;
    BOOLEAN isOpen = mce_is_open(&bda_open);

    /* Check if MAP is already connected with another device */
    if(isOpen && bdcmp(bda_open, m_SelectedDevBda) != 0)
    {
        m_uiSignal->DisplayStatus(MSG_MAP_CONNECTED);
        return;
    }

    BOOLEAN bConnected = FALSE;

    bConnected = mce_is_connected(m_SelectedDevBda, m_listMapInst[m_iMapIndex].m_iInstId);

    /* Disconnect MCE connection */
    if(bConnected)
    {
        app_mce_close(m_listMapInst[m_iMapIndex].m_iInstId);

        ResetMceFolderList();
        ui->lstMceMessages->clear();
        ui->lblStatus->setText("");
        ui->edtMceResult->clear();
        return;
    }

    ui->lblStatus->setVisible(false);

    tBSA_STATUS status;
    BD_ADDR bd_addr;

    if (mce_is_open_pending(m_listMapInst[m_iMapIndex].m_iInstId))
    {
        APP_ERROR0("already trying to connect");

        QString s = ui->btnMceConnectDisconnect->text();
        if(s == "Canceling ...")
            return;

        ui->btnMceConnectDisconnect->setText("Canceling ...");
        app_mce_cancel(m_listMapInst[m_iMapIndex].m_iInstId);
        return;
    }

    bdcpy(bd_addr, m_SelectedDevBda);

    ui->btnMceConnectDisconnect->setText("Cancel");

    status = app_mce_open(bd_addr, m_listMapInst[m_iMapIndex].m_iInstId);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to connect to device %02X:%02X:%02X:%02X:%02X:%02X with status %d",
            bd_addr[0], bd_addr[1], bd_addr[2],
            bd_addr[3], bd_addr[4], bd_addr[5], status);

        ui->btnMceConnectDisconnect->setText("Connect");
    }

}

void BSA::ProcessMCEXMLData(tBSA_MCE_EVT event, const char* filename)
{
    APP_DEBUG1("ProcessMCEXMLData filename = %s", filename);

    if(event == BSA_MCE_FOLDER_LIST_EVT)
    {
        if (m_eMceMode == eTestMode)
        {
            MCEFolderListXmlParser reader(ui->lstMceFolders);
        reader.readFile(filename);
    }
        else
        {
            MCEFolderListXmlParser reader(ui->cmbMceUserFolders);
            reader.readFile(filename);
        }
    }
    else if (event == BSA_MCE_MSG_LIST_EVT)
    {
        QListWidget *plst;
        if (m_eMceMode == eTestMode)
            plst = ui->lstMceMessages;
        else
            plst = ui->lstMceUserMsgs;
        MCEMessageListXmlParser reader(plst, &msgList);
        reader.readFile(filename);
    }
    else if (event == BSA_MCE_NOTIF_EVT)
    {
        msgList.clear();
        ui->lstMceMessages->clear();
        ui->lblStatus->setText("");
        ui->edtMceResult->clear();

        QListWidget *plst;
        plst =  ui->lstMceMessages;
        MCEEventReportXmlParser reader(plst);
        reader.readFile(filename);
    }

}

/*
Display new incoming listing data as it arrives.
This function is triggered in response to the signal.
*/
void BSA::on_BSA_mce_data_display(QVariant event, QString st, QVariant var)
{
    APP_DEBUG1("on_BSA_mce_data_display g_TempFile = 0x%x", g_TempFile);
    APP_DEBUG1("on_BSA_mce_data_display string %s",st.toStdString().c_str());

    //Show RAW data as it arrives
    ui->edtMceResult->appendPlainText(st);
    ui->edtMceResult->show();

    //Write out to the temporary file
    QTextStream out(g_TempFile);
    out << st.toStdString().c_str();

    int final = var.toInt();

    // Final indicates that the listing is complete
    if(final)
    {
        ui->edtMceResult->moveCursor(QTextCursor::Start);

        g_TempFile->close();
        QString tempPath(GetCurrentWorkingDirectory());
        tempPath += "/";
        tempPath += g_TempFileName;

        ProcessMCEXMLData((tBSA_MCE_EVT) event.toInt(), tempPath.toStdString().c_str());

        gThis->g_TempFileName = "";
    }
}

/*
Read contents from the downloaded phonebook object and display it in the UI.
This function is triggered in response to the signal.
*/
void BSA::on_BSA_mce_show_data(QString strMsgFileName)
{
    int len = strMsgFileName.length();

    if(!len)
        return;

    QFile file(strMsgFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString sterror = file.errorString();
        m_uiSignal->DisplayStatus(sterror);
        return;
    }

    // Show the entire bmessage that we received
    ui->edtMceResult->clear();

    // Show RAW data
    QTextStream in(&file);
    while(!in.atEnd()){
        QString line = in.readLine();
        ui->edtMceResult->appendPlainText(line);
        ui->edtMceResult->show();
    }

    file.close();

    bool isEmail = (m_listMapInst[m_iMapIndex].m_iMsgType & BSA_MCE_MSG_TYPE_EMAIL) ? true : false;

    QString msgStr, strFrom, strTo, msgBody;

    // Now we can parse it...

    //Read the bMessage file into memory
    void* pbmsgObj = BMessageManager::parseBMsgFile(strMsgFileName);

    //Get the originator vcard
    void* pOriVCard = BMessageManager::getBMsgOrig(pbmsgObj);

    //Get Originator name/first name / telephone number
    void* vProp = BMessageManager::getBvCardProp(pOriVCard, BMessageConstants::BTA_VCARD_PROP_FN);
    QString pParam  = BMessageManager::getBvCardPropParam(vProp);
    QString pValue = BMessageManager::getBvCardPropVal(vProp);

    vProp = BMessageManager::getBvCardProp(pOriVCard, BMessageConstants::BTA_VCARD_PROP_N);
    if(vProp)
    {
        pParam  = BMessageManager::getBvCardPropParam(vProp);
        pValue = BMessageManager::getBvCardPropVal(vProp);
        if(pValue.length())
        {
            msgStr += "\n" + pValue;
            strFrom = pValue;
        }
    }

    vProp = BMessageManager::getBvCardProp(pOriVCard, BMessageConstants::BTA_VCARD_PROP_FN);
    if(vProp)
    {
        pParam  = BMessageManager::getBvCardPropParam(vProp);
        pValue = BMessageManager::getBvCardPropVal(vProp);
        if(pValue.length())
        {
            msgStr += "\n" + pValue;
            strFrom = pValue;
        }
    }

    if (isEmail)
        vProp = BMessageManager::getBvCardProp(pOriVCard, BMessageConstants::BTA_VCARD_PROP_EMAIL);
    else
        vProp = BMessageManager::getBvCardProp(pOriVCard, BMessageConstants::BTA_VCARD_PROP_TEL);
    if(vProp)
    {
        pParam  = BMessageManager::getBvCardPropParam(vProp);
        pValue = BMessageManager::getBvCardPropVal(vProp);
        if(pValue.length())
        {
            msgStr += "\n" + pValue;
            strFrom = pValue;
        }
    }

    //Get Envelope
    void* pEnv = BMessageManager::getBMsgEnv(pbmsgObj);

    //Get Body
    void* pBody = BMessageManager::getBEnvBody(pEnv);

    //Get Recipient
    void* pRecip = BMessageManager::getBEnvRecip(pEnv);

    if(pRecip)
    {
        //Get Recipent name/first name / telephone number
        vProp = BMessageManager::getBvCardProp(pRecip, BMessageConstants::BTA_VCARD_PROP_N);
        if(vProp)
        {
            pParam  = BMessageManager::getBvCardPropParam(vProp);
            pValue = BMessageManager::getBvCardPropVal(vProp);
            if(pValue.length())
            {
                msgStr += "\n" + pValue;
                strTo = pValue;
            }
        }

        vProp = BMessageManager::getBvCardProp(pRecip, BMessageConstants::BTA_VCARD_PROP_FN);
        if(vProp)
        {
            pParam  = BMessageManager::getBvCardPropParam(vProp);
            pValue = BMessageManager::getBvCardPropVal(vProp);
            if(pValue.length())
            {
                msgStr += "\n" + pValue;
                strTo = pValue;
            }
        }

        if (isEmail)
            vProp = BMessageManager::getBvCardProp(pRecip, BMessageConstants::BTA_VCARD_PROP_EMAIL);
        else
            vProp = BMessageManager::getBvCardProp(pRecip, BMessageConstants::BTA_VCARD_PROP_TEL);
        if(vProp)
        {
            pParam  = BMessageManager::getBvCardPropParam(vProp);
            pValue = BMessageManager::getBvCardPropVal(vProp);
            if(pValue.length())
            {
                msgStr += "\n" + pValue;
                strTo = pValue;
            }
        }
    }

    // Get Content
    void* pContent = BMessageManager::getBBodyCont(pBody);

    // Get message
    msgStr += "\n";
    msgBody = BMessageManager::getBCont1stMsg(pContent);
    QString temp;
    while ((temp = BMessageManager::getBContNextMsg(pContent)) != NULL)
    {
        msgBody += temp;
    }
    msgStr += msgBody;

    //Display the message sender info and message
    if (m_eMceMode == eTestMode)
    {
        ui->edtMceMessage->setPlainText(msgStr);
    }
    else
    {
        if (m_eMsgType == eMsgTypeIncoming)
            ui->edtMceUserFromTo->setText(strFrom);
        else
            ui->edtMceUserFromTo->setText(strTo);
        ui->edtMceUserFromTo->setCursorPosition(0);
        if (isEmail)
        {
            ui->edtMceUserSubj1->setText(MceGetEmailHeader(msgBody, "Subject"));
            ui->edtMceUserSubj1->setCursorPosition(0);
            ui->edtMceUserMsg->setPlainText(msgBody.remove(0, msgBody.indexOf("\r\n\r\n") + 4));
        }
        else
            ui->edtMceUserMsg->setPlainText(msgBody);
    }

    // Release BMessage
    if(pbmsgObj)
        BMessageManager::deleteBMsg(pbmsgObj);

}

/* UI helper to reset folder list */
void BSA::ResetMceFolderList()
        {
    ui->lstMceFolders->clear();
    ui->lstMceFolders->addItem("/");
    ui->lstMceFolders->addItem("..");
}

/* Get the message content for the selected message handle */
void BSA::on_btnMceGetMessage_clicked()
{
    int index = ui->lstMceMessages->currentRow();
    if(index == -1)
    {
        m_uiSignal->DisplayStatus("Please select a folder");
        return;
    }

    // Clear UI
    ui->edtMceMessage->clear();
    ui->edtMceResult->clear();

    //We dont have valid msglist in cache so return
    //List Message control is being used for multiple purposes
    //to display notification events etc.
    if(!msgList.size())
        return;

    QString handleStr;
    handleStr = ui->lstMceMessages->currentItem()->text();

    app_mce_get_msg(m_listMapInst[m_iMapIndex].m_iInstId, handleStr.toStdString().c_str(), false);
}

/* Start / Stop Message Notification Server*/
void BSA::on_btnMceStartStopMNS_clicked()
{
    if(mce_is_open(NULL))
    {
        if(!mce_is_mns_started())
            app_mce_mn_start();
        else
            app_mce_mn_stop();
    }
}

/* Register / Unregister to receive notification from MAP server */

void BSA::on_btnMceRegUnRegNotification_clicked()
{
    if(mce_is_open(NULL) && mce_is_mns_started())
    {
        app_mce_notif_reg(m_listMapInst[m_iMapIndex].m_iInstId,
                !m_listMapInst[m_iMapIndex].m_bMceRegNotif);
        MceNotifRegStateChanged(!m_listMapInst[m_iMapIndex].m_bMceRegNotif);
    }
}

/* Update Inbox on MAP server */
void BSA::on_btnMceUpdateInbox_clicked()
{
    if(mce_is_connected(m_SelectedDevBda, m_listMapInst[m_iMapIndex].m_iInstId))
        app_mce_upd_inbox(m_listMapInst[m_iMapIndex].m_iInstId);
}

/* Get MAP Server instance information*/
void BSA::on_btnMceGetMASInfo_clicked()
{
    if(mce_is_open(NULL))
        app_mce_get_mas_ins_info(m_listMapInst[m_iMapIndex].m_iInstId);
}

void BSA::on_btnMcePushMessage_clicked()
{
    QString st = ui->edtMceMessage->toPlainText();

    QString stvalTo = ui->edtMceSendTo->text();

    if (!stvalTo.length())
    {
        m_uiSignal->DisplayStatus("Please enter recipient phone number");
        return;
    }

    McePushMessage(stvalTo, QString(""), st);

    // Clear UI
    ui->edtMceMessage->clear();
}

/* Initialize the temp file used to store the data received for PBAP and MAP servers */
void BSA::InitializeTempFile(QString fName)
{
    if(g_TempFile)
        g_TempFile->close();

    if(g_TempFile)
    {
        delete g_TempFile;
        g_TempFile = NULL;
    }

    QString tempPath(GetCurrentWorkingDirectory());
    tempPath += ("/" + fName);

    QFile::remove(tempPath.toStdString().c_str());

    g_TempFile = new QFile(tempPath.toStdString().c_str());

    if (g_TempFile && !g_TempFile->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        APP_DEBUG1("InitializeMCETempFile g_TempFile OPEN FAILED = 0x%x", g_TempFile);
        return;
    }
}

/* Get folder listing at the current folder level*/
void BSA::on_btnMceListFolders_clicked()
{
    g_TempFileName = "tempmcefolderlist.xml";
    InitializeTempFile(g_TempFileName);

    ResetMceFolderList();
    ui->lblStatus->setText("");
    ui->edtMceResult->clear();

    app_mce_get_folder_list(m_listMapInst[m_iMapIndex].m_iInstId, 100, 0);
}

/* Get Message listing for the selected folder*/
void BSA::on_btnMceListMessages_clicked()
{
    QString pFolder = "";
    tBSA_MCE_MSG_LIST_FILTER_PARAM filter_param;

    int index = ui->lstMceFolders->currentRow();
    if(index != -1)
    {
        pFolder = ui->lstMceFolders->currentItem()->text();
    }

    msgList.clear();
    ui->lstMceMessages->clear();
    ui->lblStatus->setText("");
    ui->edtMceResult->clear();

    g_TempFileName = "tempmcemsglist.xml";
    InitializeTempFile(g_TempFileName);

    memset(&filter_param, 0, sizeof(tBSA_MCE_MSG_LIST_FILTER_PARAM));
    QString stMaxCount = ui->edtMceMaxCount->text();
    if (stMaxCount.length())
        filter_param.max_list_cnt = stMaxCount.toInt();
    else
    filter_param.max_list_cnt = 65535;
    QString stStartOffset = ui->edtMceStartOffset->text();
    if (stStartOffset.length())
        filter_param.list_start_offset = stStartOffset.toInt();
    QString stRecipient = ui->edtMceRecipient->text();
    if (stRecipient.length())
        strncpy(filter_param.recipient, stRecipient.toStdString().c_str(),
                BSA_MCE_MAX_FILTER_TEXT_SIZE);
    QString stOriginator = ui->edtMceOriginator->text();
    if (stOriginator.length())
        strncpy(filter_param.originator, stOriginator.toStdString().c_str(),
                BSA_MCE_MAX_FILTER_TEXT_SIZE);
    filter_param.read_status =
        ui->cmbMceReadStatus->itemData(ui->cmbMceReadStatus->currentIndex()).toInt();
    filter_param.pri_status =
        ui->cmbMcePriority->itemData(ui->cmbMcePriority->currentIndex()).toInt();

    app_mce_get_msg_list(m_listMapInst[m_iMapIndex].m_iInstId, pFolder.toStdString().c_str(),
            &filter_param);
}

/* Set the current folder on MAP server  */
void BSA::on_btnMceSetPath_clicked()
{
    int index = ui->lstMceFolders->currentRow();
    if(index == -1)
    {
        m_uiSignal->DisplayStatus("Please select a folder");
        return;
    }

    QString folderStr = ui->lstMceFolders->currentItem()->text();

    if(folderStr == "..")
    {
        app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId, "",
                BSA_MCE_DIR_NAV_UP_ONE_LVL);

    }
    else if(folderStr == "/")
    {
        app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId, "",
                BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL);
    }
    else
    {
        app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId, folderStr.toStdString().c_str(),
                BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL);
    }
}

/* Set the current folder on MAP server on double clicking the folder item */
void BSA::on_lstMceFolders_itemDoubleClicked(QListWidgetItem *item)
{
    on_btnMceSetPath_clicked();
}

/* Set the message status Read/Unread Delete/Undelete*/
void BSA::on_btnMceSetMessageStatus_clicked()
{
    int index = ui->lstMceMessages->currentRow();
    if(index == -1)
    {
        m_uiSignal->DisplayStatus("Please select a message");
        return;
    }

    QString handleStr;
    handleStr = ui->lstMceMessages->currentItem()->text();

   int status_indicator = ui->cmbMceStatusInd->currentIndex();
    if(status_indicator < 0)
        status_indicator = 0;
    int status_value = ui->cmbMceStatusValue->currentIndex();
     if(status_value < 0)
         status_value = 0;

    app_mce_set_msg_sts(m_listMapInst[m_iMapIndex].m_iInstId, handleStr.toStdString().c_str(),
            status_indicator, status_value);
}

/* Load the message details from cached message listing data */
void BSA::on_lstMceMessages_itemClicked(QListWidgetItem *item)
{
    QString handleStr;

    if(!ui->lstMceMessages->count())
        return;

    handleStr = ui->lstMceMessages->currentItem()->text();
    ui->edtMceResult->clear();

    if(!msgList.size())
        return;

    ui->edtMceResult->appendPlainText(msgList.at(ui->lstMceMessages->currentRow()).msginfo);
    ui->edtMceResult->moveCursor(QTextCursor::Start);
}

/* Get the message for the selected handle on double click */
void BSA::on_lstMceMessages_itemDoubleClicked(QListWidgetItem *item)
{
    on_btnMceGetMessage_clicked();
}

void BSA::on_btnMceAbort_clicked()
{
    if (m_iMapIndex < 0)
        return;

    app_mce_abort(m_listMapInst[m_iMapIndex].m_iInstId);
}

void BSA::on_btnPbcAbort_clicked()
{
    pbc_set_open_pending(FALSE);
    app_pbc_abort();
}

void BSA::on_cmbMceInstances_currentIndexChanged(int index)
{
    if(index == -1)
    {
        ui->btnMceConnectDisconnect->setEnabled(FALSE);
        m_iMapIndex = -1;
        MceStateChanged(FALSE);
    }
    else
    {
        ui->btnMceConnectDisconnect->setEnabled(TRUE);
        m_iMapIndex = ui->cmbMceInstances->itemData(index).toUInt();
        MceStateChanged(mce_is_connected(m_SelectedDevBda, m_listMapInst[m_iMapIndex].m_iInstId));
        MceInstanceChanged();
    }
}

void BSA::on_btnMceUserMode_clicked()
{
    m_eMceMode = eUserMode;
    app_mce_close_all();
    sleep(1);
    on_btnMapClient_clicked();
}

/*************************************************************************************/
/********************* MCE User Mode *************************************************/
/*************************************************************************************/

void BSA::MceUserResetUI(bool bFullReset)
{
    if(bFullReset)
    {
        ui->cmbMceUserInsts->clear();
        m_iMapIndex = -1;
        ui->btnMceUserRefresh->setEnabled(false);
        ui->btnMceUserReply->setEnabled(false);
        ui->btnMceUserSend->setEnabled(false);
        ui->btnMceUserCancel->setEnabled(false);
    }

    ui->cmbMceUserFolders->clear();
    ui->lstMceUserMsgs->clear();
    ui->edtMceUserFromTo->clear();
    ui->edtMceUserSubj1->clear();
    ui->edtMceUserMsg->clear();
    ui->edtMceUserTo->clear();
    ui->edtMceUserSubj2->clear();
    ui->edtMceUserPushMsg->clear();
}

void BSA::MceUserAdjustUI(bool bIsEmail)
{
    if(bIsEmail)
    {
        ui->edtMceUserMsg->setGeometry(244, 108, 210, 240);
        ui->edtMceUserPushMsg->setGeometry(470, 108, 210, 240);
        ui->edtMceUserSubj1->setVisible(true);
        ui->edtMceUserSubj2->setVisible(true);
    }
    else
    {
        ui->edtMceUserMsg->setGeometry(244, 88, 210, 260);
        ui->edtMceUserPushMsg->setGeometry(470, 88, 210, 260);
        ui->edtMceUserSubj1->setVisible(false);
        ui->edtMceUserSubj2->setVisible(false);
    }
}

void BSA::on_MceUserStateChanged(QVariant var)
{
    bool bConnected = var.toBool();

    MceUserResetUI(false);

    if(bConnected)
    {
        ui->btnMceUserConnDisc->setText("Disconnect");
        MceUserStartBrowsing();
    }
    else
    {
        ui->btnMceUserConnDisc->setText("Connect");
    }

    ui->btnMceUserRefresh->setEnabled(bConnected);
    ui->btnMceUserReply->setEnabled(bConnected);
    ui->btnMceUserSend->setEnabled(bConnected);
    ui->btnMceUserCancel->setEnabled(bConnected);

}

void BSA::on_cmbMceUserInsts_currentIndexChanged(int index)
{
    if(index == -1)
    {
        m_iMapIndex = -1;
        on_MceUserStateChanged(false);
    }
    else
    {
        m_iMapIndex = ui->cmbMceUserInsts->itemData(index).toInt();
        on_MceUserStateChanged(mce_is_connected(m_SelectedDevBda,
                m_listMapInst[m_iMapIndex].m_iInstId));
        MceUserAdjustUI((m_listMapInst[m_iMapIndex].m_iMsgType & BSA_MCE_MSG_TYPE_EMAIL) ?
                true : false);
    }
}

void BSA::on_btnMceUserConnDisc_clicked()
{
    /* Check if valid device is selected, if not show message */
    if (!m_bDevSelected)
    {
        m_uiSignal->DisplayStatus(MSG_SELECT_DEVICE);
        return;
    }

    if (m_iMapIndex < 0)
        return;

    BD_ADDR bda_open;
    BOOLEAN isOpen = mce_is_open(&bda_open);

    /* Check if MAP is already connected with another device */
    if(isOpen && bdcmp(bda_open, m_SelectedDevBda) != 0)
    {
        m_uiSignal->DisplayStatus(MSG_MAP_CONNECTED);
        return;
    }

    BOOLEAN bConnected = FALSE;

    bConnected = mce_is_connected(m_SelectedDevBda, m_listMapInst[m_iMapIndex].m_iInstId);

    /* Disconnect MCE connection */
    if(bConnected)
    {
        app_mce_close(m_listMapInst[m_iMapIndex].m_iInstId);
        return;
    }

    ui->lblStatus->setVisible(false);

    tBSA_STATUS status;

    if (mce_is_open_pending(m_listMapInst[m_iMapIndex].m_iInstId))
    {
        APP_ERROR0("already trying to connect");

        QString s = ui->btnMceConnectDisconnect->text();
        if(s == "Canceling ...")
            return;

        ui->btnMceUserConnDisc->setText("Canceling ...");
        app_mce_cancel(m_listMapInst[m_iMapIndex].m_iInstId);
        return;
    }

    ui->btnMceUserConnDisc->setText("Cancel");

    status = app_mce_open(m_SelectedDevBda, m_listMapInst[m_iMapIndex].m_iInstId);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("MAP connect failed with status %d", status);

        ui->btnMceUserConnDisc->setText("Connect");
    }

}

void BSA::MceUserStartBrowsing()
{
    /* set path to telecom/msg */
    m_listFolders.clear();
    m_listFolders.append("telecom");
    m_listFolders.append("msg");

    /* get folder list after set path */
    m_eMceNextAction = eMceActionListFolder;

    /* start by setting to root */
    app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId,
            "", BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL);
}

void BSA::on_cmbMceUserFolders_currentIndexChanged(int index)
{
    if(index == -1)
    {
        ui->lstMceUserMsgs->clear();
    }
    else if (m_eMceNextAction != eMceActionNone)
    {
        /* Do nothing if set folder is already in progress */
    }
    else
    {
        QString folder = ui->cmbMceUserFolders->currentText();
        eMsgType type;
        if (folder == "inbox" || folder == "deleted")
            type = eMsgTypeIncoming;
        else
            type = eMsgTypeOutgoing;
        MceUserSetMsgType(type);

        m_listFolders.clear();
        m_listFolders.append(folder);
        m_eMceNextAction = eMceActionListMessage;
        app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId, "", BSA_MCE_DIR_NAV_UP_ONE_LVL);

        ui->edtMceUserFromTo->clear();
        ui->edtMceUserSubj1->clear();
        ui->edtMceUserMsg->clear();
    }
}

void BSA::on_lstMceUserMsgs_itemClicked(QListWidgetItem *item)
{
    QString handleStr = ui->lstMceUserMsgs->currentItem()->text();

    app_mce_get_msg(m_listMapInst[m_iMapIndex].m_iInstId, handleStr.toStdString().c_str(), false);
}

void BSA::on_btnMceUserRefresh_clicked()
{
    ui->btnMceUserRefresh->setEnabled(false);
    m_eMceNextAction = eMceActionListMessage;
    MceSetNextFolder();
}

void BSA::on_btnMceUserReply_clicked()
{
    ui->edtMceUserTo->setText(ui->edtMceUserFromTo->text());
    ui->edtMceUserTo->setCursorPosition(0);
    ui->edtMceUserSubj2->setText(QString("Re: ") + ui->edtMceUserSubj1->text());
    ui->edtMceUserSubj2->setCursorPosition(0);
}

void BSA::on_btnMceUserSend_clicked()
{
    if (ui->edtMceUserTo->text().length() == 0)
    {
        m_uiSignal->DisplayStatus("Please enter recipient phone number");
        return;
    }

    m_listFolders.clear();
    m_listFolders.append("outbox");
    m_eMceNextAction = eMceActionPushMessage;
    app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId, "", BSA_MCE_DIR_NAV_UP_ONE_LVL);
}

void BSA::on_btnMceUserCancel_clicked()
{
    ui->edtMceUserTo->clear();
    ui->edtMceUserSubj2->clear();
    ui->edtMceUserPushMsg->clear();
}

void BSA::on_btnMceTestMode_clicked()
{
    m_eMceMode = eTestMode;
    app_mce_close_all();
    sleep(1);
    on_btnMapClient_clicked();
}

void BSA::MceSetNextFolder()
{
    if (!m_listFolders.isEmpty())
    {
        app_mce_set_folder(m_listMapInst[m_iMapIndex].m_iInstId,
                m_listFolders.first().toStdString().c_str(),
                BSA_MCE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL);
        m_listFolders.removeFirst();
    }
    else
    {
        if (m_eMceNextAction == eMceActionListFolder)
        {
            g_TempFileName = "tempmcefolderlist.xml";
            InitializeTempFile(g_TempFileName);

            app_mce_get_folder_list(m_listMapInst[m_iMapIndex].m_iInstId, 0xFFFF, 0);
        }
        else if (m_eMceNextAction == eMceActionListMessage)
        {
            tBSA_MCE_MSG_LIST_FILTER_PARAM filter_param;

            g_TempFileName = "tempmcemsglist.xml";
            InitializeTempFile(g_TempFileName);

            memset(&filter_param, 0, sizeof(tBSA_MCE_MSG_LIST_FILTER_PARAM));
            filter_param.max_list_cnt = 0xFFFF;
            app_mce_get_msg_list(m_listMapInst[m_iMapIndex].m_iInstId, "", &filter_param);
        }
        else if (m_eMceNextAction == eMceActionPushMessage)
        {
            McePushMessage(ui->edtMceUserTo->text(), ui->edtMceUserSubj2->text(),
                    ui->edtMceUserPushMsg->toPlainText());
        }
        m_eMceNextAction = eMceActionNone;
    }
}

void BSA::MceUserSetMsgType(eMsgType type)
{
    m_eMsgType = type;

    if (type == eMsgTypeIncoming)
        ui->lbMceUserFromTo->setText("From:");
    else
        ui->lbMceUserFromTo->setText("To:");
}

QString BSA::MceGetEmailHeader(QString strEmail, QString strHeader)
{
    int index1, index2, length;

    /* first remove email body */
    strEmail = strEmail.left(strEmail.indexOf("\r\n\r\n"));

    index1 = strEmail.indexOf(strHeader);
    if (index1 >= 0)
    {
        index1 += strHeader.size() + 2;

        index2 = strEmail.indexOf("\r\n", index1);
        if (index2 >= 0)
            length = index2 - index1;
        else
            length = strEmail.size() - index1;
        return strEmail.mid(index1, length);
    }

    return NULL;
}

QString BSA::MceGetEventAttribute(QString notif, QString attr)
{
    int index1 = 0;
    int index2 = 0;

    index1 = notif.indexOf("<event ");
    if (index1 > 0)
    {
        index1 = notif.indexOf(attr, index1);
        if (index1 > 0)
        {
            index1 = notif.indexOf("\"", index1);
            if (index1 > 0)
                index2 = notif.indexOf("\"", ++index1);
        }
    }

    if (index1 > 0 && index2 > 0 && index2 > index1)
        return notif.mid(index1, index2 - index1);

    return NULL;
}

QString BSA::MceNotifToString(QString notif)
{
    QString st;
    QString type = MceGetEventAttribute(notif, "type");
    QString handle = MceGetEventAttribute(notif, "handle");
    QString msg_type = MceGetEventAttribute(notif, "msg_type");

    if (type == "NewMessage")
    {
        st = QString("New ") + msg_type + QString(" message received, handle ") + handle;
    }
    else if (type == "MessageDeleted")
    {
        st = msg_type + QString(" message ") + handle + QString(" has been deleted");
    }
    else if (type == "MessageShift")
    {
        QString folder = MceGetEventAttribute(notif, "folder");
        QString old_folder = MceGetEventAttribute(notif, "old_folder");
        folder.remove(0, folder.lastIndexOf("/"));
        old_folder.remove(0, old_folder.lastIndexOf("/"));
        st = msg_type + QString(" message ") + handle + QString(" moved from ") +
                old_folder + QString(" to ") + folder;
    }
    else if (type == "DeliverySuccess")
    {
        st = msg_type + QString(" message ") + handle + QString(" has been delivered");
    }
    else if (type == "SendingSuccess")
    {
        st = msg_type + QString(" message ") + handle + QString(" has been sent");
    }
    else if (type == "DeliveryFailure")
    {
        st = msg_type + QString(" message ") + handle + QString(" delivery failed");
    }
    else if (type == "SendingFailure")
    {
        st = msg_type + QString(" message ") + handle + QString(" sending failed");
    }
    else if (type == "MemoryFull")
    {
        st = "MAP server memory is full";
    }
    else if (type == "MemoryAvailable")
    {
        st = "MAP server memory is available again";
    }
    else if (type == "ReadStatusChanged")
    {
        st = msg_type + QString(" message ") + handle + QString(" read status changed");
    }
    else
    {
        st = QString("Notification type ") + type + QString("  received");
    }

    return st;
}

/*************************************************************************************/
/*************** AV source UI ************************************************************/
/*************************************************************************************/
void BSA::ConfigAvkAvRelayState()
{
#if (defined(BSA_AVK_AV_AUDIO_RELAY) && (BSA_AVK_AV_AUDIO_RELAY==TRUE))

    BD_ADDR bda_open = {0};
    BOOLEAN isAvkOpen = avk_is_any_open(bda_open);

    ui->btnStartStopAudioRelay->setText("Start Relay");
    if(m_bAvOpen && isAvkOpen)
    {
        ui->btnStartStopAudioRelay->setEnabled(true);
    }
    else
    {
        m_bEnableAudioRelay = FALSE;
        ui->btnStartStopAudioRelay->setEnabled(false);
    }
#endif
}

void BSA::on_btnStereo_clicked()
{
     DefaultVisibility();
     ui->deviceAVFrame->setVisible(true);
}


void AvCallback(tBSA_AV_EVT event, tBSA_AV_MSG *p_data)
{
    tAPP_AV_CONNECTION *connection;
    UINT8 command;
    UINT8 av_play_type, av_play_state;

    switch (event)
    {
    case BSA_AV_OPEN_EVT:
        connection = app_av_find_connection_by_handle(p_data->open.handle);
        if (connection != NULL)
        {
            if (p_data->open.status == BSA_SUCCESS)
            {
                bdcpy(connection->bd_addr, p_data->open.bd_addr);
                connection->is_open = TRUE;
                gThis->m_bAvOpen = TRUE;
                gThis->ui->btnConnectAV->setEnabled(false);
                gThis->ui->btnDisconnectAV->setEnabled(true);

                gThis->ui->btnPlayTone->setEnabled(true);
                gThis->ui->btnStopAV->setEnabled(true);
                gThis->ui->pushButtonNotifn->setEnabled(true);
                gThis->ui->PauseButton_2->setEnabled(true);

                gThis->ConfigAvkAvRelayState();

                // Update the UI to show current registered notifications
                gThis->ui->listNotifications->clear();
                for(UINT8 i = AVRC_EVT_PLAY_STATUS_CHANGE; i <= AVRC_EVT_VOLUME_CHANGE; i++)
                {
                    QString s = app_av_get_notification_string(i);
                    if(app_av_is_event_registered(i))
                    {
                        s += " - Registered";
                    }
                    else
                    {
                        s += " - not registered";
                    }

                    gThis->ui->listNotifications->addItem(s);
                }
            }
            else
            {
                emit gThis->m_uiSignal->DisplayMessageBox("Could not connect AV.");
            }
        }
        break;

    case BSA_AV_CLOSE_EVT:

        connection = app_av_find_connection_by_handle(p_data->close.handle);
        if (connection == NULL)
        {
            APP_ERROR1("unknown connection handle %d", p_data->close.handle);
        }
        else
        {
            connection->is_open = FALSE;
            gThis->m_bAvOpen = FALSE;
            gThis->ui->btnConnectAV->setEnabled(true);
            gThis->ui->btnDisconnectAV->setEnabled(false);
            gThis->ui->listNotifications->clear();

            gThis->ui->btnPlayTone->setEnabled(false);
            gThis->ui->btnStopAV->setEnabled(false);
            gThis->ui->PauseButton_2->setEnabled(false);
            gThis->ui->pushButtonNotifn->setEnabled(false);

            gThis->ConfigAvkAvRelayState();
        }

        break;
    case BSA_AV_RC_OPEN_EVT:
        break;

    case BSA_AV_RC_CLOSE_EVT:
        break;

#if (defined(BSA_AVK_AV_AUDIO_RELAY) && (BSA_AVK_AV_AUDIO_RELAY==TRUE))
/*Relay AVRCP commands recieved from headset (AV Sink) to Phone (AV Source)*/
    case BSA_AV_REMOTE_CMD_EVT:
        APP_DEBUG1("BSA_AV_REMOTE_CMD_EVT handle:%d", p_data->remote_cmd.rc_handle);
        av_play_type = app_av_get_play_type();
        av_play_state = app_av_get_play_state();

        if(gThis->m_bEnableAudioRelay)
        {
            switch(p_data->remote_cmd.rc_id)
            {
            case BSA_AV_RC_PLAY:
                APP_INFO0("Play key");
                command = APP_AV_START;
                break;
            case BSA_AV_RC_STOP:
                APP_INFO0("Stop key");
                command = APP_AV_STOP;
                break;
            case BSA_AV_RC_PAUSE:
                APP_INFO0("Pause key");
                command = APP_AV_PAUSE;
                break;
            case BSA_AV_RC_FORWARD:
                APP_INFO0("Forward key");
                command = APP_AV_FORWARD;
                break;
            case BSA_AV_RC_BACKWARD:
                APP_INFO0("Backward key");
                command = APP_AV_BACKWARD;
                break;
            default:
                APP_INFO1("key:0x%x", p_data->remote_cmd.rc_id);
                command = APP_AV_IDLE;
                break;
            }
            if (p_data->remote_cmd.key_state == BSA_AV_STATE_PRESS)
            {
                APP_INFO1("Key pressed -> executing command %d", command);
                switch(command)
                {
                case APP_AV_START:
                    switch (av_play_state)
                    {
                    case APP_AV_PLAY_PAUSED:
                        gThis->on_Play_clicked(); /* Start AVK Play */
                        app_av_play_from_avk();
                        break;

                    case APP_AV_PLAY_STOPPED:
                        switch (av_play_type)
                        {
                        case APP_AV_PLAYTYPE_AVK:
                            gThis->on_Play_clicked(); /* Start AVK Play */
                            app_av_play_from_avk();
                            break;
                        default:
                            APP_ERROR1("Unsupported play type (%d)", av_play_type);
                            break;
                        }
                        break;
                    case APP_AV_PLAY_STARTED:
                        /* Already started */
                        APP_DEBUG0("Stream already started, not calling start");
                        break;
                    case APP_AV_PLAY_STOPPING:
                        /* Stop in progress */
                        APP_DEBUG0("Stream stopping, not calling start");
                        break;
                    default:
                        APP_ERROR1("Unsupported play state (%d)", av_play_state);
                        break;
                    }
                    break;
                case APP_AV_STOP:
                    gThis->on_stopButton_clicked();
                    break;

                case APP_AV_PAUSE:
                    gThis->on_PauseButton_clicked();
                 break;

                case APP_AV_FORWARD:
                case APP_AV_BACKWARD:
                {
                    tAPP_AVK_CONNECTION * connection = gThis->GetActiveConnection();
                    if(connection == NULL)
                        break;

                    if(command == APP_AV_FORWARD)
                    {
                        app_avk_rc_send_click(BSA_AVK_RC_FORWARD, connection->rc_handle);
                    }
                    else if(command == APP_AV_BACKWARD)
                    {
                        app_avk_rc_send_click(BSA_AVK_RC_BACKWARD, connection->rc_handle);
                    }

                    break;
                }
                }
            }
            else  if (p_data->remote_cmd.key_state == BSA_AV_STATE_RELEASE)
            {
                APP_INFO0(" released -> just informative");
            }
            else
            {
                APP_ERROR0("Unknown key state");
            }
            break;
    }
#endif


    case BSA_AV_META_MSG_EVT:
        switch(p_data->meta_msg.pdu)
        {
            case BSA_AVRC_PDU_REGISTER_NOTIFICATION:

            // Update the UI to show current registered notifications
            gThis->ui->listNotifications->clear();
            for(UINT8 i = AVRC_EVT_PLAY_STATUS_CHANGE; i <= AVRC_EVT_VOLUME_CHANGE; i++)
            {
                QString s = app_av_get_notification_string(i);
                if(app_av_is_event_registered(i))
                {
                    s += " - Registered";
                }
                else
                {
                    s += " - not registered";
                }

                gThis->ui->listNotifications->addItem(s);
            }

            break;
        }

        break;
    }


}


// A2DP src connect
void BSA::on_btnConnectAV_clicked()
{
    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    /* Check if valid device is selected, if not show message */
    if (iRow == -1 || iRow > iCount-1)
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
        return;
    }

    BD_ADDR bd_addr = {0};

    bdcpy(bd_addr, m_paired_device_list[iRow].msg.bd_addr);

    app_av_open(&bd_addr);
}

// A2DP src disconnect
void BSA::on_btnDisconnectAV_clicked()
{
    app_av_close();
}


// play tone
void BSA::on_btnPlayTone_clicked()
{
    if ((app_av_get_play_state()!= APP_AV_PLAY_STOPPED) &&
        (app_av_get_play_state() != APP_AV_PLAY_STOPPING) &&
        (app_av_get_play_state() != APP_AV_PLAY_PAUSED))
        app_av_pause();

    if(app_av_play_playlist(APP_AV_START) != 0)
        app_av_play_tone();
}

// Stop play
void BSA::on_btnStopAV_clicked()
{
    app_av_stop();
}


//Relay AVK Audio as AV Source
void BSA::on_btnStartStopAudioRelay_clicked()
{
#if (defined(BSA_AVK_AV_AUDIO_RELAY) && (BSA_AVK_AV_AUDIO_RELAY==TRUE))
    tBSA_AVK_RELAY_AUDIO relay_param;

    if(!m_bEnableAudioRelay)
    {
        m_bEnableAudioRelay = TRUE;
        BSA_AvkRelayAudioInit(&relay_param);
        relay_param.audio_relay = TRUE;
        BSA_AvkRelayAudio(&relay_param);
        ui->btnStartStopAudioRelay->setText("Stop Relay");

        app_av_play_from_avk();
    }
    else
    {
        m_bEnableAudioRelay = FALSE;
        ui->btnStartStopAudioRelay->setText("Start Relay");
        BSA_AvkRelayAudioInit(&relay_param);
        relay_param.audio_relay = FALSE;
        BSA_AvkRelayAudio(&relay_param);
    }
#endif
}


// Send registered notifications
void BSA::on_pushButtonNotifn_clicked()
{
    // Check if anything is selected
    int iIndex = gThis->ui->listNotifications->currentRow();
    if(iIndex < 0)
        return;

    // Send the notification
    int iID = iIndex + 1;

    app_av_rc_complete_notification(iID);

    gThis->ui->listNotifications->clear();

    // update UI to show current registered notifications
    for(UINT8 i = AVRC_EVT_PLAY_STATUS_CHANGE; i <= AVRC_EVT_VOLUME_CHANGE; i++)
    {
        QString s = app_av_get_notification_string(i);
        if(app_av_is_event_registered(i))
        {
            s += " - Registered";
        }
        else
        {
            s += " - not registered";
        }

        gThis->ui->listNotifications->addItem(s);
    }

}

void BSA::on_PauseButton_2_clicked()
{
    app_av_pause();
}

/*************************************************************************************/
/*************** HF AG UI ************************************************************/
/*************************************************************************************/

void BSA::on_btnHFAG_clicked()
{
    DefaultVisibility();
    ui->deviceHFAGFrame->setVisible(true);
}

void AgCallback(tBSA_AG_EVT event, tBSA_AG_MSG *p_data)
{
    switch (event)
    {
        case BSA_AG_OPEN_EVT: /* RFCOM level connection */
            if (p_data->open.status == BSA_SUCCESS)
            {
                gThis->ui->btnConnectAG->setEnabled(false);
                gThis->ui->btnDisconnectAG->setEnabled(true);
                gThis->ui->btnOpenAudio->setEnabled(true);
                gThis->ui->btnCloseAudio->setEnabled(false);
            }
            else
            {
                emit gThis->m_uiSignal->DisplayMessageBox("Could not connect AG.");
            }
            break;

        case BSA_AG_CONN_EVT: /* Service level connection */
            break;

        case BSA_AG_CLOSE_EVT: /* Connection Closed (for info)*/
            gThis->ui->btnConnectAG->setEnabled(true);
            gThis->ui->btnDisconnectAG->setEnabled(false);
            gThis->ui->btnOpenAudio->setEnabled(false);
            gThis->ui->btnCloseAudio->setEnabled(false);
            break;

        case BSA_AG_AUDIO_OPEN_EVT: /* Audio Open Event */
            gThis->ui->btnOpenAudio->setEnabled(false);
            gThis->ui->btnCloseAudio->setEnabled(true);

            break;

        case BSA_AG_AUDIO_CLOSE_EVT: /* Audio Close event */
            gThis->ui->btnOpenAudio->setEnabled(true);
            gThis->ui->btnCloseAudio->setEnabled(false);
            break;
    }

}


// Service level connetion, connect
void BSA::on_btnConnectAG_clicked()
{
    int iRow = ui->tblPairedDevices->currentRow();
    int iCount = m_paired_device_list.size();

    /* Check if valid device is selected, if not show message */
    if (iRow == -1 || iRow > iCount-1)
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(MSG_SELECT_DEVICE);
        return;
    }

    BD_ADDR bd_addr = {0};

    bdcpy(bd_addr, m_paired_device_list[iRow].msg.bd_addr);

    app_ag_open(&bd_addr);
}

// Service level connetion, disconnect
void BSA::on_btnDisconnectAG_clicked()
{
    app_ag_close();
}

// SCO open
void BSA::on_btnOpenAudio_clicked()
{
    // If AV is streaming, pause it before opening SCO
    g_av_prev_state = app_av_get_play_state();
    app_av_pause();
    sleep(1);
    app_ag_open_audio();
}

// SCO close
void BSA::on_btnCloseAudio_clicked()
{
    app_ag_close_audio();

    if((g_av_prev_state == APP_AV_PLAY_STOPPED) || (g_av_prev_state  == APP_AV_PLAY_PAUSED))
        return;

    // audio was playing before we paused it, resume playing
    sleep(1);
    app_av_resume();
}


// Tracing utility
void BSA::on_btnTrace_clicked()
{
    // If the trace input is not visible, show it
    if(!ui->traceText->isVisible())
    {
        ui->traceText->setVisible(true);
        ui->btnTrace->setText("Done");
    }
    // If trace input is visible, send the user input text to BSA server and hide it
    else
    {
        ui->traceText->setVisible(false);
        ui->btnTrace->setText("Add Trace");
        QString strText= ui->traceText->text();

        tBSA_STATUS bsa_status;
        tBSA_TM_SET_TRACE_LEVEL trace_text;
        bsa_status = BSA_TmSetTraceTextInit(&trace_text);
        QByteArray ba = strText.toLatin1();
        strncpy((char*)trace_text.trace_text, (char*) ba.data(), BSA_TM_TRACE_LEN_MAX-1);
        bsa_status = BSA_TmSetTraceText(&trace_text);
        ui->traceText->setText("");

        APPL_TRACE_DEBUG0("**********************************************************");
        APPL_TRACE_DEBUG1("USER TRACE: %s", trace_text.trace_text);
        APPL_TRACE_DEBUG0("**********************************************************");
    }

}
