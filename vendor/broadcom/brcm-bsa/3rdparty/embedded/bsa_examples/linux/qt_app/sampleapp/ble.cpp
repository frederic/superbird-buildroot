/*****************************************************************************
**
**  Name:           ble.cpp
**
**  Description:    Handles BLE functions
**
*****************************************************************************/
#include "bsa.h"
#include "ble.h"
#include "ui_bsa.h"
#include "blerwinfodlg.h"
#include <QDomDocument>
#include <QThread>
#include <QGridLayout>

extern "C"
{
#include "app_ble.h"
#include "app_ble_client.h"
#include "app_ble_server.h"
#include "app_ble_qt.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_ble_client_db.h"
#include "app_dm.h"

extern int  app_dm_set_ble_visibility(BOOLEAN discoverable, BOOLEAN connectable);
extern void app_ble_client_display_attr(tAPP_BLE_CLIENT_DB_ELEMENT *p_blecl_db_elmt);
extern int  app_ble_wake_configure_qt(BOOLEAN enabled);
}

// style sheet for groupboxes
static char * gGrpBoxCSS = {
    "QGroupBox {"
    "  border: 1px solid gray;"
    "  border-radius: 5px;"
    "  margin-top: 0.5em;"
    "}"
    "QGroupBox::title {"
    "   subcontrol-origin: margin;"
    "   subcontrol-position: top left;"
    "}"
};

extern BSA * gThis; // global pointer to main BSA class
extern BLE gBle;    // global pointer to this BLE class
void setChildFont(QTreeWidgetItem *pChild,QColor color);
static QTreeWidgetItem* checkDups(QString str, QTreeWidgetItem * pChild);
#define APP_BLE_ADV_VALUE_LEN             6  /*This is temporary value, Total Adv data including all fields should be <31bytes*/

ushort gNextServerUuid = 1;
ushort gNextClientUuid = 9000;
char * attrDesc(UINT16 uuid);
void columnResize();
void fix_case(QString *str);
Qt::ItemFlags gEditFlags = Qt::NoItemFlags;

// class to hold server side service data
class Service : public QTreeWidgetItem
{
public:
    Service(ushort uuid) :
        QTreeWidgetItem(SERVICE), m_uuid(uuid), m_server_index(-1), m_running(false),
        m_service_created(false), m_attr_num(-1) {}
    ushort m_uuid;
    int    m_server_index;
    bool   m_running;
    bool   m_service_created;
    int    m_attr_num;
};

// class to hold server side characteristic data
class ServiceChar : public QTreeWidgetItem
{
public:
    ServiceChar(ushort uuid, uchar perm, uchar properties, QByteArray defVals)
        : QTreeWidgetItem(CHARACTERISTIC), m_uuid(uuid), m_permissions(perm),
          m_properties(properties), m_vals(defVals), m_attr_num(-1), m_attr_id(-1)
    {
    }
    int m_attr_num;
    int m_attr_id;
    ushort m_uuid;
    uchar  m_permissions;
    uchar  m_properties;
    QByteArray m_vals;
};

ClientChar::ClientChar(int handle, int inst, uchar properties)
    : QTreeWidgetItem(CHARACTERISTIC), m_handle(handle), m_inst(inst),
      m_properties(properties)
{
}

ClientService::ClientService(int handle, int inst, uchar properties, int is_prim)
    : QTreeWidgetItem(SERVICE), m_handle(handle), m_inst(inst), m_properties(properties), m_is_prim(is_prim)
{
}


BLE::BLE()
{
    mBsa = NULL;
    mDiscActive = false;
    mFirst = true;
}

void BLE::shutdown()
{
	app_ble_server_deregister_all();
    app_ble_exit();
}

// gets attribute data value from XML node
QString attrValue(QDomNode node, QString attr)
{
    QDomNamedNodeMap nnm = node.attributes();
    if (nnm.isEmpty())
        return QString();   // null string

    QDomNode attr_node = nnm.namedItem(attr);
    if (attr_node.isNull())
        return QString();   // null string

    return (attr_node.nodeValue());
}

// parse string of 2 digit hex numbers
QByteArray parseDefVals(QString attrVal)
{
    QByteArray defvals;
    bool ok;

    for (int i = 0; i < attrVal.length(); i += 3)
    {
        ok = false;
        char b = attrVal.mid(i,2).toUShort(&ok,16);
        if (ok)
            defvals.append(b);
    }

    return defvals;
}


// init BLE class and UI
void BLE::Init(BSA * pBsa)
{
    if (app_ble_init() < 0)
    {
        pBsa->on_BSA_show_message_box("Couldn't Initialize BLE app, exiting");
    }

    mServerId = -1;
    mBsa = pBsa;    
    qApp->setStyleSheet(gGrpBoxCSS);    

    // connect UI widgets with functions
    connect(mBsa->ui->btnBleRdWr, SIGNAL(clicked()), this, SLOT(on_btnBleInfo()));
    connect(mBsa->ui->btnConnectClBle, SIGNAL(clicked()), this, SLOT(on_btnConnect()));
    connect(mBsa->ui->btnStartDiscBle, SIGNAL(clicked()), this, SLOT(on_btnStartDisc()));
    connect(mBsa->ui->tabBle, SIGNAL(currentChanged(int)), this, SLOT(on_tab_changed(int)));
    connect(mBsa->ui->btnBleNewService, SIGNAL(clicked()), this, SLOT(on_btnNewService()));
    connect(mBsa->ui->btnBleStartSrvc, SIGNAL(clicked()), this, SLOT(on_btnStartSrvc()));
    connect(mBsa->ui->btnAddCharact, SIGNAL(clicked()), this, SLOT(on_btnBleAddChar()));
    connect(mBsa->ui->trBleSrvc, SIGNAL(itemSelectionChanged()), this, SLOT(on_trBleSrvcChanged()));
    connect(mBsa->ui->trScanResults, SIGNAL(itemSelectionChanged()), this, SLOT(on_trDiscChanged()));    
    connect(mBsa->ui->rbBleConnTypeAuto, SIGNAL(clicked()), this, SLOT(on_btnConnType()));
    connect(mBsa->ui->rbBleConnTypeNone, SIGNAL(clicked()), this, SLOT(on_btnConnType()));
    connect(mBsa->ui->btnSetBleScan, SIGNAL(clicked()), this, SLOT(on_btnSetScan()));
    connect(mBsa->ui->btnSetBleConnParams, SIGNAL(clicked()), this, SLOT(on_btnSetBleConnParams()));
    connect(mBsa->ui->btnSetBleAdv, SIGNAL(clicked()), this, SLOT(on_btnSetBleAdv()));
    connect(mBsa->ui->cbWakeOnBLE, SIGNAL(clicked()), this, SLOT(on_cbBleWake()));
    connect(mBsa->ui->btnBleAdv, SIGNAL(clicked()), this, SLOT(on_btnCreateAdv()));
    connect(mBsa->ui->btnBleSendInd, SIGNAL(clicked()), this, SLOT(on_btnSendInd()));
    connect(mBsa->ui->trBleSrvc, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(on_serverDblClick(QTreeWidgetItem*,int)));
    connect(mBsa->ui->trBleSrvc, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(on_bleSrItemChanged(QTreeWidgetItem*,int)));
    connectEdits(mBsa->ui->tabBle);

    mBsa->ui->rbBleScacnRspYes->setChecked(true);
    mBsa->ui->btnBleSendInd->setEnabled(false);
    mBsa->ui->btnConnectClBle->setEnabled(false);
    mBsa->ui->lbBleStat->setText("Not Connected");
    mBsa->ui->edBleAdAp->setText("0002");
    mBsa->ui->btnAddCharact->setEnabled(false);    
    mBsa->ui->btnBleStartSrvc->setEnabled(false);
    mBsa->ui->rbConnDirect->setChecked(true);
    mBsa->ui->trBleSrvc->setColumnCount(2);

    // read and parse profile XML file and setup UI for services
    QString strFn = "://xml/servicedefs.xml";
    QFile services_file(strFn);
    int nMaxColWidth = 0;
    QDomDocument doc;
    if (services_file.open(QIODevice::ReadOnly))
    {
        doc.clear();
        if (doc.setContent(&services_file))
        {
            QDomNodeList srvs = doc.elementsByTagName("SERVICE");
            QString name,col2;

            for (int i = 0; i < srvs.size(); i++)
            {
                QDomNode n = srvs.item(i);
                name = attrValue(n, "DESCRIPTION");
                name += " (";
                QString strUUID = attrValue(n, "UUID");
                name += strUUID;
                name += ")";

                // do not allow duplicates
                if (!name.isNull())
                {
                    bool ok=false;
                    ushort uuid = strUUID.toUShort(&ok, 16);

                    Service * pItem = new Service(uuid);
                    pItem->setText(0,name);
                    mBsa->ui->trBleSrvc->addTopLevelItem(pItem);
                    setChildFont(pItem,Qt::blue);
                    if (mBsa->ui->trBleSrvc->columnWidth(0) > nMaxColWidth)
                        nMaxColWidth = mBsa->ui->trBleSrvc->columnWidth(0);

                    QDomElement service_el = n.toElement();
                    if (service_el.isNull())
                        continue;
                    QDomNodeList si_list = service_el.elementsByTagName("SERVERINFO");
                    for (int j = 0; j < si_list.size(); j++)
                    {
                        QDomNode si = si_list.item(j);
                        QDomElement si_el = si.toElement();
                        if (si_el.isNull())
                            continue;
                        QDomNodeList char_list = si_el.elementsByTagName("CHARACTERISTIC");
                        for (int k = 0; k < char_list.size(); k++)
                        {
                            ushort char_uuid = 0;
                            uchar prop = 0x34;
                            uchar perm = 0x11;
                            bool ok = false;
                            QByteArray defVals;

                            QDomNode cn = char_list.item(k);
                            QString char_name="",attrVal="";
                            attrVal = attrValue(cn,"NAME");
                            if (!attrVal.isNull())
                                char_name = attrVal;

                            attrVal = attrValue(cn, "UUID");
                            if (attrVal.isNull())
                                continue;

                            char_uuid = attrVal.toUShort(&ok,16);
                            if (!ok || char_uuid == 0)
                                continue;

                            char_name += " (";
                            char_name += attrVal;
                            char_name += ")";

                            col2 = "Perm=";
                            // Permissions[Eg: Read-0x1, Write-0x10, Read|Write-0x11]
                            // XML file specifies permissions as text 'R|W'
                            // convert to mask mask value
                            attrVal = attrValue(cn, "PERMISSIONS");
                            perm = attrVal.toUInt(&ok,16);
                            if (ok)
                            {
                                if (perm == 0x11)
                                    col2 += "Rd|Wr";
                                if (perm == 0x10)
                                    col2 += "Wr";
                                if (perm == 1)
                                    col2 += "Rd";
                                if (perm == 0)
                                    col2 += "none";
                            }
                            else if (!attrVal.isNull())
                            {                  
                                if (attrVal.size() >= 1)
                                {
                                    if (attrVal[0] == 'R')
                                        perm = 1;
                                    if (attrVal[0] == 'W')
                                        perm |= 0x10;
                                    if (attrVal.size() >= 2)
                                    {
                                        if (attrVal[1] == 'R')
                                            perm |= 1;
                                        if (attrVal[1] == 'W')
                                            perm |= 0x10;
                                    }
                                }
                                if (perm == 0x11)
                                    col2 += "Rd|Wr";
                                if (perm == 0x10)
                                    col2 += "Wr";
                                if (perm == 1)
                                    col2 += 'Rd';
                                if (perm == 0)
                                    col2 += "none";
                            }

                            // Properties  WRITE-0x08, READ-0x02, Notify-0x10, Indicate-0x20
                            //      Eg: For READ|WRITE|NOTIFY|INDICATE enter 0x3A
                            // XML file specifies properties as HEX
                            // convert to text for display
                            QString strProp = "";
                            attrVal = attrValue(cn, "PROPERTIES");
                            if (!attrVal.isNull())
                            {
                                prop = attrVal.toUInt(&ok,16);

                                if (ok)
                                {
                                    strProp += " ";
                                    if ((prop & 0x02) == 0x02)
                                        strProp += "Rd";
                                    if ((prop & 0x08) == 0x08)
                                    {
                                        if (strProp == " ")
                                            strProp += "Wr";
                                        else
                                            strProp += "|Wr";
                                    }
                                    if ((prop & 0x10) == 0x10)
                                    {
                                        if (strProp == " ")
                                            strProp += "Notify";
                                        else
                                            strProp += "|Notify";
                                    }
                                    if ((prop & 0x20) == 0x20)
                                    {
                                        if (strProp == " ")
                                            strProp += "Indicate";
                                        else
                                            strProp += "|Indicate";
                                    }
                                  }
                            }
                            if (strProp != " ")
                            {
                                if (strProp.size() <= 1)
                                    col2 += " Prop=Rd|Wr|Notify|Indicate";
                                else
                                    col2 += " Prop=" + strProp;
                            }
                            attrVal = attrValue(cn, "DEFAULTVAL");
                            if (!attrVal.isNull())
                            {
                                defVals = parseDefVals(attrVal);
                            }

                            ServiceChar * pChild = new ServiceChar(char_uuid, perm, prop, defVals);

                            fix_case(&char_name);
                            pChild->setText(0,char_name);
                            pChild->setText(1,col2);

                            QTreeWidgetItem * pVals = new QTreeWidgetItem (VALUES);
                            gEditFlags = pVals->flags();
                            QString strVals = ""; // "values (default): ";
                            QString strHex;
                            for (int i = 0; i < defVals.size() && i < 5; i++)
                            {
                                strHex.sprintf("%02x ", (UINT8)defVals.at(i));
                                strVals += strHex;
                                if ((i+1) == defVals.size() && defVals.size() > 5)
                                {
                                    strVals += "...";
                                }
                            }

                            pVals->setText(0, "Values (default): ");
                            pVals->setText(1, strVals);

                            // QTreeWidgetItem* item = new QTreeWidgetItem();
                           // pVals->setFlags(pVals->flags() | Qt::ItemIsEditable);

                            pChild->addChild(pVals);

                            pItem->addChild(pChild);

                            setChildFont(pChild,Qt::darkGreen);
                            mBsa->ui->trBleSrvc->resizeColumnToContents(0);
                            if (mBsa->ui->trBleSrvc->columnWidth(0) > nMaxColWidth)
                                nMaxColWidth = mBsa->ui->trBleSrvc->columnWidth(0);
                        }
                    }
                }
            }
        }
    }
    mBsa->ui->trBleSrvc->expandAll();
    mBsa->ui->trBleSrvc->resizeColumnToContents(0);
    mBsa->ui->trBleSrvc->resizeColumnToContents(1);
    mBsa->ui->trBleSrvc->collapseAll();
    setClientUIState(false);
}

// user double clicked values for characteristic
void BLE::on_serverDblClick(QTreeWidgetItem*pItem,int col)
{
    if (col != 1)
        return;
    if (pItem->type() != VALUES)
        return;
    pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
    mBsa->ui->trBleSrvc->editItem(pItem, 1);
}

// setup edit controls for hex input vavlidation
void BLE::connectEdits(QWidget * pParent)
{
    QList<QLineEdit *>ui_widgets = pParent->findChildren<QLineEdit *>();

    for (int i = 0; i < ui_widgets.count(); i++)
    {
        QLineEdit * pEdit = ui_widgets.at(i);

        if (pEdit->inputMask() == "HHHH" || pEdit->inputMask() == "HH")
            connect(pEdit, SIGNAL(cursorPositionChanged(int,int)), this, SLOT(on_edHexWord(int,int)));
    }

}

// user pressed BLE button on main window
void BLE::frameActive()
{
    // first time initializations
    if (mFirst)
    {
        mFirst = false;
        if (app_ble_start() < 0)
        {
            mBsa->on_BSA_show_message_box("Couldn't Start BLE app, exiting");
        }

        app_dm_set_ble_visibility(TRUE, TRUE);        

        QString strBda="Ready";
        int status;
        tBSA_DM_GET_CONFIG bt_config;

        /*
        * Get bluetooth configuration
        */
        status = BSA_DmGetConfigInit(&bt_config);
        status = BSA_DmGetConfig(&bt_config);
        if (status == BSA_SUCCESS)
        {
            strBda.sprintf("%s (%02X:%02X:%02X:%02X:%02X:%02X)",bt_config.name,
                           bt_config.bd_addr[0], bt_config.bd_addr[1],
                           bt_config.bd_addr[2], bt_config.bd_addr[3],
                           bt_config.bd_addr[4], bt_config.bd_addr[5]);
        }

        mBsa->ui->lbBleBDA->setText(strBda);
        mBsa->ui->lbBleBDA->setVisible(true);
    }
}

// slot function for hex input edit controls
// called when user enters edit field
void BLE::on_edHexWord(int old, int n)
{    
    QLineEdit * pEdit = (QLineEdit *)sender();
    if (pEdit == NULL)
        return;

    printf("on_edBleAdU1, old=%d, new=%d\n",old,n);
    if (old == 0 && n != 0 && pEdit->text() == "")
    {
        // set curor location to make it easier to enter data
        pEdit->setCursorPosition(0);
    }
}

#define MAX_SERV_NAME   200

typedef struct
{
    UINT16 uuid;
    char desc[MAX_SERV_NAME];
    BD_ADDR addr;
} BLE_DISC_SERVICE_INFO;

std::vector<BLE_DISC_SERVICE_INFO*> sDiscServices;

// discovery complete, show results
void ShowDiscResults(void *,unsigned int )
{    
    gThis->setCursor(Qt::ArrowCursor);
    gBle.mDiscActive = false;
    gThis->ui->btnStartDiscBle->setText("Start Discovery");    

    QString str;
    for (int index = 0; index < APP_DISC_NB_DEVICES; index++)
    {
        if (!app_discovery_cb.devs[index].in_use)
            continue;

        if (BT_DEVICE_TYPE_BLE != app_discovery_cb.devs[index].device.device_type)
            continue;

        // device name and address
        str.sprintf("%s (%02X:%02X:%02X:%02X:%02X:%02X)",
                    (char *)app_discovery_cb.devs[index].device.name,
                    app_discovery_cb.devs[index].device.bd_addr[0],
                    app_discovery_cb.devs[index].device.bd_addr[1],
                    app_discovery_cb.devs[index].device.bd_addr[2],
                    app_discovery_cb.devs[index].device.bd_addr[3],
                    app_discovery_cb.devs[index].device.bd_addr[4],
                    app_discovery_cb.devs[index].device.bd_addr[5]);

        // add device to list
        QTreeWidgetItem * pDev = new QTreeWidgetItem(SERVER);
        setChildFont(pDev,Qt::blue);
        pDev->setText(0,str);   // 1st column is name and address
        str.sprintf("%d",app_discovery_cb.devs[index].device.rssi);
        pDev->setText(1,str);   // 2cd column is RSSI
        UINT64 lBda=0;
        memcpy((UINT8 *)&lBda, app_discovery_cb.devs[index].device.bd_addr, 6);
        pDev->setData(0, Qt::UserRole,lBda);    // save bt address of device
        pDev->setData(1, Qt::UserRole,-1);      // initialize connection id
        gThis->ui->trScanResults->addTopLevelItem(pDev);

        for (int i = 0; i < sDiscServices.size(); i++)
        {
            // add servics if any were discovered
            if (memcmp(sDiscServices[i]->addr, app_discovery_cb.devs[index].device.bd_addr, 6) == 0)
            {
                str.sprintf("%s (%04x)",sDiscServices[i]->desc, sDiscServices[i]->uuid);
                if (checkDups(str,pDev))
                    continue;

                ClientService * pChild = new ClientService(0,0,0,1);
                setChildFont(pChild,Qt::darkGreen);
                pChild->setText(0,str);
                pChild->setData(0,Qt::UserRole, sDiscServices[i]->uuid);
                pDev->addChild(pChild);
            }
        }
    }
    columnResize();
}

// called when discovery finds ble services
extern "C" void BleDiscNewService16(UINT16 uuid16,char * desc, char* bda)
{
    BLE_DISC_SERVICE_INFO *info = (BLE_DISC_SERVICE_INFO*)malloc(sizeof(BLE_DISC_SERVICE_INFO));
    memset(info, 0, sizeof(BLE_DISC_SERVICE_INFO));
    info->uuid = uuid16;
    if (desc)
        strncpy(info->desc, desc, MAX_SERV_NAME-1);
    else
        strcpy(info->desc,"Unknow");
    memcpy(info->addr, bda, 6);
    sDiscServices.push_back(info);
}

extern "C" void app_disc_parse_eir_new(tBSA_DISC_NEW_MSG *disc_new);

extern "C" void BleDiscCb(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    if (event == BSA_DISC_NEW_EVT)
    {
        /* check if this device has already been received (update) */
        for (int index = 0; index < APP_DISC_NB_DEVICES; index++)
        {
            if ((app_discovery_cb.devs[index].in_use == TRUE) &&
                (!bdcmp(app_discovery_cb.devs[index].device.bd_addr, p_data->disc_new.bd_addr)))
            {
                return;
            }
        }

        if (p_data->disc_new.eir_data[0])
        {
            app_disc_parse_eir_new(&(p_data->disc_new));
        }
    }

    if (event != BSA_DISC_CMPL_EVT)
        return;

    emit gThis->RunInUiThread((void *)ShowDiscResults,NULL,0);
}

void BLE::clearResults()
{
    disconnectAll();
    mBsa->ui->trScanResults->clear();
    gThis->ui->btnConnectClBle->setEnabled(false);    
}

// called when user starts dicovery
void BLE::on_btnStartDisc()
{
    if (mDiscActive)
    {        
        gThis->setCursor(Qt::ArrowCursor);
        mDiscActive = false;
        app_disc_abort();        
        mBsa->ui->btnStartDiscBle->setText("Start Discovery");
        return;
    }

    app_ble_client_db_clear();
    mDiscActive = true;
    clearResults();
    for (int i = 0; i < sDiscServices.size(); i++)
    {
        delete sDiscServices[i];
    }
    sDiscServices.clear();
    app_disc_start_ble_regular(BleDiscCb);    
    mBsa->ui->btnStartDiscBle->setText("Stop Discovery");
    gThis->setCursor(Qt::WaitCursor);
}

// user clicked on client functions
void BLE::on_btnBleInfo()
{
    if (NULL == gpRWDlg)
        if (NULL == (gpRWDlg = new BleRWInfoDlg()))
            return;

    // open dialog for read/write operations modeless
    gpRWDlg->show();
}

void BLE::disconnectAll()
{
    for (int i = 0; i < mBsa->ui->trScanResults->topLevelItemCount(); i++)
    {
        QTreeWidgetItem * pRoot = mBsa->ui->trScanResults->topLevelItem(i);

        int clientId = pRoot->data(1,Qt::UserRole).toInt();
        if (clientId != -1)
        {
            // disconnect
            app_ble_client_close_qt(clientId);
            setClientUIState(false);
            pRoot->setData(1,Qt::UserRole,-1);
            pRoot->setText(1,"");
            app_ble_client_deregister_qt(clientId);
        }
    }
}

// connect to BLE server
void BLE::on_btnConnect()
{
    QTreeWidgetItem * pDev = mBsa->ui->trScanResults->currentItem();
    if (pDev == NULL)
        return;

    // find root item in tree, this is the server
    QTreeWidgetItem * pRoot = pDev;
    if (pDev->parent())
    {        
        while (pRoot->parent())
            pRoot = pRoot->parent();
    }

    // get connection id
    int clientId = pRoot->data(1,Qt::UserRole).toInt();
    if (clientId != -1)
    {
        // connected to device, disconnect
        app_ble_client_close_qt(clientId);
        setClientUIState(false);
        pRoot->setData(1,Qt::UserRole,-1);
        pRoot->setText(1,"");
        app_ble_client_deregister_qt(clientId);
        return;
    }

    // register the client
    if (-1 == (clientId = app_ble_client_register_qt(gNextClientUuid++)))
    {
        mBsa->on_BSA_show_message_box("Error registering client");
        return;
    }

    tBSA_STATUS status;
    tBSA_BLE_CL_OPEN ble_open_param;
    UINT64 llBda = pDev->data(0, Qt::UserRole).toULongLong();
    BD_ADDR bd_addr;

    // connect to server device
    memcpy(bd_addr, (UINT8 *)&llBda, 6);
    status = BSA_BleClConnectInit(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        mBsa->on_BSA_show_message_box("BSA_BleClConnectInit");
        return;
    }

    ble_open_param.client_if = app_ble_cb.ble_client[clientId].client_if;
    ble_open_param.is_direct = (mBsa->ui->rbConnDirect->isChecked()) ? TRUE : FALSE;

    memcpy(app_ble_cb.ble_client[clientId].server_addr, bd_addr, 6);
    memcpy(ble_open_param.bd_addr, bd_addr, 6);

    status = BSA_BleClConnect(&ble_open_param);
    if (status != BSA_SUCCESS)
    {
        mBsa->on_BSA_show_message_box("BSA_BleClConnect failed");
        return ;
    }
}

void BLE::on_tab_changed(int i)
{    
    if (i == 1) // client tab
        return;

    // server tab or config tab
    // close client functions dlg if open
    if (gpRWDlg)
        gpRWDlg->close();
    gpRWDlg = NULL;
}

// called form server cb call back when a characteristic is added
// we need to save the id
extern "C" void bleSetCharAttrId(int index, int attr_index,int attr_id)
{
    return;
    for (int j = 0; j < gThis->ui->trBleSrvc->topLevelItemCount(); j++)
    {
        Service * pServ = (Service *)gThis->ui->trBleSrvc->topLevelItem(j);
        for (int k = 0; k < pServ->childCount(); k++)
        {
            ServiceChar * pChar = (ServiceChar * )pServ->child(k);
            if (pChar->m_attr_num == attr_index)
            {
                pChar->m_attr_id = attr_id;
                return;
            }
        }

    }
}

// called from the server cb when connected client tries to read
// the value of a chacteristic
extern "C" int bleGetAttrValue(tBSA_BLE_MSG* p_data, UINT8 attribute_value[], int max)
{
    int rval = 0;

    // find tree control item
    for (int j = 0; j < gThis->ui->trBleSrvc->topLevelItemCount(); j++)
    {
        Service * pServ = (Service *)gThis->ui->trBleSrvc->topLevelItem(j);
        if (!pServ->m_running)
            continue;

        int conn_id = app_ble_cb.ble_server[pServ->m_server_index].server_if;
        if (p_data->ser_read.conn_id != conn_id)
            continue;

        for (int k = 0; k < pServ->childCount(); k++)
        {
            ServiceChar * pChar = (ServiceChar * )pServ->child(k);
            int attr_id = app_ble_cb.ble_server[pServ->m_server_index].attr[pChar->m_attr_num].attr_id;
            if (attr_id == p_data->ser_read.handle)
            {
                rval = (pChar->m_vals.count() >= max) ? max : pChar->m_vals.count();
                memcpy(attribute_value, pChar->m_vals.data(), rval);
                return rval;
            }
        }
    }

    return rval;
}

// server callback
void ServerCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    switch (event)
    {
    case BSA_BLE_SE_OPEN_EVT:
        if (p_data->ser_open.reason == BSA_SUCCESS)
        {
            int num = app_ble_server_find_index_by_interface(p_data->ser_open.server_if);
            for (int i = 0; i < gThis->ui->trBleSrvc->topLevelItemCount(); i++)
            {
                int inx = gThis->ui->trBleSrvc->topLevelItem(i)->data(0,Qt::UserRole).toInt();
                if (num == inx)
                {
                    QString str;//<font color='red'>Some text</font>
                    str.sprintf("Connected to %02X:%02X:%02X:%02X:%02X:%02X",
                       p_data->ser_open.remote_bda[0], p_data->ser_open.remote_bda[1],
                       p_data->ser_open.remote_bda[2], p_data->ser_open.remote_bda[3],
                       p_data->ser_open.remote_bda[4], p_data->ser_open.remote_bda[5]);

                    gThis->ui->lbBleStat->setText(str);
                    break;
                }
            }
        }
        break;

    case BSA_BLE_SE_CLOSE_EVT:
        {        
            int num = app_ble_server_find_index_by_interface(p_data->ser_close.server_if);
            for (int i = 0; i < gThis->ui->trBleSrvc->topLevelItemCount(); i++)
            {
                int inx = gThis->ui->trBleSrvc->topLevelItem(i)->data(0,Qt::UserRole).toInt();
                if (num == inx)
                {
                    gThis->ui->lbBleStat->setText("Disconnected");
                    break;
                }
            }
        }
        break;

    case BSA_BLE_SE_WRITE_EVT:
        if (p_data->ser_write.status != BSA_SUCCESS)
            break;

        for (int j = 0; j < gThis->ui->trBleSrvc->topLevelItemCount(); j++)
        {
            Service * pServ = (Service *)gThis->ui->trBleSrvc->topLevelItem(j);
            if (!pServ->m_running)
                continue;

            int conn_id = app_ble_cb.ble_server[pServ->m_server_index].server_if;
            if (p_data->ser_write.conn_id != conn_id)
                continue;
            for (int k = 0; k < pServ->childCount(); k++)
            {
                ServiceChar * pChar = (ServiceChar * )pServ->child(k);
                int attr_id = app_ble_cb.ble_server[pServ->m_server_index].attr[pChar->m_attr_num].attr_id;
                if (attr_id == p_data->ser_write.handle)
                {
                    pChar->m_vals.clear();

                    QString str= "";//"Values (written): ";   //<font color='red'>Some text</font>
                    for (int j = 0; j < p_data->ser_write.len; j++)
                    {
                        if (j < 5)
                        {
                            QString hex;
                            hex.sprintf("%02X ",p_data->ser_write.value[j]);
                            str += hex;
                        }
                        pChar->m_vals.append(p_data->ser_write.value[j]);
                    }
                    gBle.disconnect(gThis->ui->trBleSrvc, SIGNAL(itemChanged(QTreeWidgetItem*,int)), &gBle, SLOT(on_bleSrItemChanged(QTreeWidgetItem*,int)));
                    if (pChar->child(0))
                    {
                        pChar->child(0)->setText(0,"Values (written):");
                        pChar->child(0)->setText(1,str);
                    }
                    gBle.connect(gThis->ui->trBleSrvc, SIGNAL(itemChanged(QTreeWidgetItem*,int)), &gBle, SLOT(on_bleSrItemChanged(QTreeWidgetItem*,int)));
                    break;
                }
            }
        }
        break;
    }
    if (p_data)
       free(p_data);
}

extern "C" char *app_disc_get_uuid16_desc(UINT16 uuid16);

// get client tree node for a connection
QTreeWidgetItem * findConnId(int target_conn_id, int * pIndx)
{
    for (int i =0 ; i < gThis->ui->trScanResults->topLevelItemCount(); i++)
    {
        QTreeWidgetItem * pItem = gThis->ui->trScanResults->topLevelItem(i);
        int cn = pItem->data(1,Qt::UserRole).toUInt();
        if (cn == -1)
            continue;
        UINT16 conn_id = app_ble_cb.ble_client[cn].conn_id;

        if (conn_id == target_conn_id)
        {
            if (pIndx)
                *pIndx = i;
            return pItem;
        }
    }
    return NULL;
}

// add tree contorl items for the services characteristics
void showChars(QTreeWidgetItem * pRoot)
{
    UINT64 llAddr = pRoot->data(0,Qt::UserRole).toLongLong();
    BD_ADDR bd_addr;
    memcpy(bd_addr, (UINT8 *)&llAddr, 6);

    // look up clients chars in cache database
    tAPP_BLE_CLIENT_DB_ELEMENT * p_db_ele = app_ble_client_db_find_by_bda(bd_addr);
    if (p_db_ele == NULL)
        return;

    UINT16 serv_id;
    QTreeWidgetItem *pChild = NULL;
    ClientService * pService = NULL;
    for (tAPP_BLE_CLIENT_DB_ATTR *p_attr = p_db_ele->p_attr; p_attr; p_attr = p_attr->next)
    {
        printf("attr = %x, id = %d, handle = %d, type=%d,\n",
               p_attr->attr_UUID.uu.uuid16, p_attr->id, p_attr->handle, p_attr->attr_type);
        if (p_attr->attr_type == 3)
        {
            // service
            serv_id = p_attr->attr_UUID.uu.uuid16;

            QString str;
            char * szServName = app_disc_get_uuid16_desc(p_attr->attr_UUID.uu.uuid16);
            str.sprintf("%s (%04x)",
                (NULL == szServName) ? "Unknown Service" : szServName,
                p_attr->attr_UUID.uu.uuid16);

            if (pChild = checkDups(str,pRoot))
                continue;

            pService = new ClientService(p_attr->handle,p_attr->id, p_attr->prop,p_attr->is_primary);
            pChild = (QTreeWidgetItem *)pService;
            pChild->setText(0,str);
            setChildFont(pChild,Qt::darkGreen);
            pChild->setData(0,Qt::UserRole, p_attr->attr_UUID.uu.uuid16);
            pRoot->addChild(pChild);
            gThis->ui->trScanResults->resizeColumnToContents(0);
        }
        else
        {
            // characteristic
            if (pChild)
            {
               QString str;
               str.sprintf("%s (%04X)",attrDesc(p_attr->attr_UUID.uu.uuid16),p_attr->attr_UUID.uu.uuid16);
               if (checkDups(str,pChild))
                   continue;

               ClientChar * pNew = new ClientChar(p_attr->handle,p_attr->id,
                    p_attr->prop);
               pNew->setText(0,str);
               pNew->setData(0,Qt::UserRole,p_attr->attr_UUID.uu.uuid16);
               pChild->addChild(pNew);
               gThis->ui->trScanResults->resizeColumnToContents(0);
            }
        }
    }

    columnResize();
}

void columnResize()
{
    gThis->ui->trScanResults->expandAll();
    gThis->ui->trScanResults->resizeColumnToContents(0);
    gThis->ui->trScanResults->resizeColumnToContents(1);
    gThis->ui->trScanResults->collapseAll();
}

void showAttrs(QTreeWidgetItem * pItem)
{
    showChars(pItem);
}

void showAttrs(BD_ADDR bda)
{
    UINT64 llConnBda=0;
    memcpy((UINT8 *)&llConnBda, bda, 6);
    for (int i =0 ; i < gThis->ui->trScanResults->topLevelItemCount(); i++)
    {
        QTreeWidgetItem * pItem = gThis->ui->trScanResults->topLevelItem(i);
        UINT64 llBda = pItem->data(0,Qt::UserRole).toULongLong();

        if (llBda == llConnBda)
        {
            showAttrs(pItem);
            break;
        }
    }
}

// client side callback
void ClientCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    switch (event)
    {
        case BSA_BLE_CL_SEARCH_RES_EVT:
        {
            int i = 0;
            QTreeWidgetItem * pItem=findConnId(p_data->cli_search_res.conn_id,&i);
            if (pItem)
            {
                QString str;
                str.sprintf("%s (%04x)",app_disc_get_uuid16_desc(p_data->cli_search_res.service_uuid.id.uuid.uu.uuid16),
                    p_data->cli_search_res.service_uuid.id.uuid.uu.uuid16);

                ClientService * pChild = new ClientService(0,0,0,1);
                pChild->setText(0,str);
                pChild->setData(0,Qt::UserRole, p_data->cli_search_res.service_uuid.id.uuid.uu.uuid16);
                pItem->addChild(pChild);
                gThis->ui->trScanResults->resizeColumnToContents(0);
            }
        }
        break;

    case BSA_BLE_CL_SEARCH_CMPL_EVT:
        gThis->ui->trScanResults->expandItem(gThis->ui->trScanResults->currentItem());
        gThis->setCursor(Qt::ArrowCursor);
        break;

    case BSA_BLE_CL_CACHE_SAVE_EVT:
        showAttrs(p_data->cli_cache_save.bd_addr);
        break;

    case BSA_BLE_CL_CACHE_LOAD_EVT:
        showAttrs(p_data->cli_cache_load.bd_addr);
        break;

    case BSA_BLE_CL_CLOSE_EVT:
    {
        UINT64 llConnBda =0;
        memcpy((UINT8 *)&llConnBda , p_data->cli_close.remote_bda, 6);

        for (int i =0 ; i < gThis->ui->trScanResults->topLevelItemCount(); i++)
        {
            QTreeWidgetItem * pItem = gThis->ui->trScanResults->topLevelItem(i);
            UINT64 llBda = pItem->data(0,Qt::UserRole).toULongLong();

            if (llBda == llConnBda)
            {
                pItem->setData(1,Qt::UserRole,-1);
                pItem->setText(1, "");                
                columnResize();
                gBle.setClientUIState(false);

                QList<QTreeWidgetItem *> children = pItem->takeChildren();
                children.clear();
                break;
            }
        }
    }
    break;

    case BSA_BLE_CL_READ_EVT:
        if (gpRWDlg )
            gpRWDlg->onRead(p_data);
        break;

    case BSA_BLE_CL_NOTIF_EVT:
        if (gpRWDlg )
            gpRWDlg->onNotify(p_data);
        break;

    case BSA_BLE_CL_OPEN_EVT:
        int client_num = app_ble_client_find_index_by_interface(p_data->cli_open.client_if);        
        if (p_data->cli_open.status != BSA_SUCCESS)
        {
            QString str;
            str.sprintf("Error connecting to server; %d (0x%x)",p_data->cli_open.status, p_data->cli_open.status );
            gThis->on_BSA_show_message_box(str);
            app_ble_client_deregister_qt(client_num);
            break;
        }

        if(client_num < 0)
        {
            gThis->on_BSA_show_message_box("Error connecting to server: No client registered.");            
            break;
        }

        BD_NAME bd_name={0};

        for (int index = 0; index < APP_NUM_ELEMENTS(app_discovery_cb.devs); index++)
        {
            if (!bdcmp(app_discovery_cb.devs[index].device.bd_addr, p_data->cli_open.bd_addr))
            {
                strcpy((char*)bd_name, (char*)app_discovery_cb.devs[index].device.name);
                break;
            }
        }

        UINT64 llConnBda =0;
        memcpy((UINT8 *)&llConnBda , p_data->cli_open.bd_addr, 6);

        for (int i =0 ; i < gThis->ui->trScanResults->topLevelItemCount(); i++)
        {
            QTreeWidgetItem * pItem = gThis->ui->trScanResults->topLevelItem(i);
            UINT64 llBda = pItem->data(0,Qt::UserRole).toULongLong();

            if (llBda == llConnBda)
            {
                pItem->setData(1,Qt::UserRole,client_num);
                pItem->setText(1, "Connected");
                columnResize();
                gBle.setClientUIState(true);
                showAttrs(pItem);
                break;
            }
        }

        break;
    }

   if (p_data)
      free(p_data);
}

// server tree control selected item changed
void BLE::on_bleSrItemChanged(QTreeWidgetItem*pItem,int col)
{
    if (col != 1)
        return;
    if (pItem->type() != VALUES)
        return;
    pItem->setFlags(gEditFlags);
    bool ok=false;
    QByteArray vals;
    QString str = pItem->text(col);
    for (int i = 0; i < str.length(); i += 3)
    {
        if ( (i + 1) > str.length() )
            break;
        UINT8 byte = (UINT8)(str.mid(i,i+2).toUShort(&ok,16));
        if (ok)
        {
            vals.append(byte);
        }
        else
            break;
    }

    ServiceChar * pChar = (ServiceChar *)pItem->parent();
    if (!ok || vals.count() == 0)
    {
        QString str = "";
        for (int j = 0; j < pChar->m_vals.count(); j++)
        {
            QString strHex;
            strHex.sprintf("%02X ", pChar->m_vals.at(j));
            str += strHex;
        }
        disconnect(mBsa->ui->trBleSrvc, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(on_bleSrItemChanged(QTreeWidgetItem*,int)));
        pItem->setText(1,str);
        connect(mBsa->ui->trBleSrvc, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(on_bleSrItemChanged(QTreeWidgetItem*,int)));
    }
    else
    {
        pChar->m_vals = vals;
        pItem->setText(0,"Values:");
    }
}

// user was to send characteristic values to connected client
void BLE::on_btnSendInd()
{
    QTreeWidgetItem *pItem=NULL ,*pDev = mBsa->ui->trBleSrvc->currentItem();

    if (pDev->type() == VALUES)
    {
        pDev = pDev->parent();
    }
    else if (pDev->type() != CHARACTERISTIC)
    {
        return;
    }
    Service *pSrv = NULL;

    for (pItem = pDev; pItem; pItem = pItem->parent())
    {
        if (pItem->parent() == NULL)
        {
            pSrv = (Service *)pItem;
            break;
        }
    }

    if (NULL == pSrv)
        return;

    ServiceChar * pChar = (ServiceChar *)pDev;
    UINT8 data[BSA_PBC_MAX_VALUE_LEN];
    int i;

    for (i=0; i < BSA_PBC_MAX_VALUE_LEN && i < pChar->m_vals.length(); i++)
    {
        data[i] = pChar->m_vals.at(i);
    }

    app_ble_server_send_indication_qt(pSrv->m_server_index,i,((ServiceChar*)pDev)->m_attr_num,data);
}

// server callback, proxy call to UI thread handler
extern "C" void BleServerCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    tBSA_BLE_MSG * pm = (tBSA_BLE_MSG *)malloc(sizeof(tBSA_BLE_MSG ));
    memcpy(pm,p_data,sizeof(tBSA_BLE_MSG ));
    emit gThis->RunInUiThread((void *)ServerCb,pm,event);
}

// client callback, proxy call to UI thread handler
extern "C" void BleClientCb(tBSA_BLE_EVT event,  tBSA_BLE_MSG *p_data)
{
    tBSA_BLE_MSG * pm = (tBSA_BLE_MSG *)malloc(sizeof(tBSA_BLE_MSG ));
    memcpy(pm,p_data,sizeof(tBSA_BLE_MSG ));
    emit gThis->RunInUiThread((void *)ClientCb,pm,event);
}

// user wanst to manually add a characteristic to a server
void BLE::on_btnBleAddChar()
{
    QTreeWidgetItem * pTwItem = mBsa->ui->trBleSrvc->currentItem();
    if (pTwItem == NULL)
        return;

    QTreeWidgetItem * pService = pTwItem;

    for (;;)
    {
        if (pService->parent() == NULL)
            break;
        pService = pService->parent();
    }

    Service * pSrvc = (Service *)pService;
    bool ok;
    ushort uuid = mBsa->ui->edAttrUuid->text().toUShort(&ok,16);;
    uchar characteristic_property=0;
    characteristic_property |= mBsa->ui->cbBleCharInd->isChecked() ? 0x20 : 0;
    characteristic_property |= mBsa->ui->cbBleCharNot->isChecked() ? 0x10 : 0;
    characteristic_property |= mBsa->ui->cbBleCharRd->isChecked() ? 0x02 : 0;
    characteristic_property |= mBsa->ui->cbBleCharWr->isChecked() ? 0x08 : 0;
    uchar  attribute_permission=0;
    attribute_permission |= mBsa->ui->cbBleAttrRd->isChecked() ? 1 : 0;
    attribute_permission |= mBsa->ui->cbBleAttrWr->isChecked() ? 0x10 : 0;

    QByteArray defVals;
    defVals.append( (char) 0);
    ServiceChar * pChar = new ServiceChar(uuid,attribute_permission ,characteristic_property, defVals );
    pChar->setText(0,mBsa->ui->edAttrUuid->text());

    QString col2 = "Perm=";
    if (attribute_permission == 0x11)
        col2 += "Rd|Wr";
    if (attribute_permission == 0x10)
        col2 += "Wr";
    if (attribute_permission == 1)
        col2 += 'Rd';

    QString strProp = "";

    if ((characteristic_property & 0x02) == 0x02)
        strProp += "Rd";
    if ((characteristic_property & 0x08) == 0x08)
        strProp += "|Wr";
    if ((characteristic_property & 0x10) == 0x10)
        strProp += "|Notify";
    if ((characteristic_property & 0x20) == 0x20)
        strProp += "|Indicate";
    col2 += " Prop=" + strProp;

    pChar->setText(1,col2);
    QTreeWidgetItem * pVals = new QTreeWidgetItem(VALUES);

    pVals->setText(0, "Values: ");
    pVals->setText(1, "00");    

    pChar->addChild(pVals);
    pSrvc->addChild(pChar);
    setChildFont(pChar,Qt::darkGreen);
    columnResize();
}

// user wants to start a service
void BLE::on_btnStartSrvc()
{
    QTreeWidgetItem * p = mBsa->ui->trBleSrvc->currentItem();
    if (p == NULL)
        return;

    if (mServerId == -1)
    {
        mServerId = RegSrv();    
    }

    QTreeWidgetItem * pServer = p;

    for (;;)
    {
        if (pServer->parent() == NULL)
            break;
        pServer = pServer->parent();
    }

    Service * pServ = (Service *)pServer;

    if (pServ->m_server_index == -1)
        pServ->m_server_index = mServerId;

    int srvc_attr_num=0;

    if (pServ->m_running)
    {
        // running, stop
        p->setText(1,"Stopped");
        mBsa->ui->btnBleStartSrvc->setText("Start Service");
        app_ble_server_stop_service_qt(pServ->m_server_index,srvc_attr_num);
    }
    else
    {
        if (!pServ->m_service_created)
            if (!(pServ->m_service_created = CreateService(pServ)))
                return;

        p->setText(1,"Running");
        mBsa->ui->btnBleStartSrvc->setText("Stop Service");
        app_ble_server_start_service_qt(pServ->m_server_index,pServ->m_attr_num);
    }
    pServ->m_running = !pServ->m_running;
    mBsa->ui->trBleSrvc->resizeColumnToContents(1);
    app_ble_server_display();
}

#include <QWaitCondition>
#include <qmutex.h>

// synccronization objects
static QWaitCondition gWait;
static QMutex gWaitMutex;

extern "C" void Wake()
{
    gWait.wakeAll();
}

// create a new service
bool BLE::CreateService(Service * pServ)
{
    int num_handle = 2 + (pServ->childCount() * 2);/* 2 for service, 2 for each chararc */

    if (pServ->m_server_index == -1)
        return false;

    if (-1 == (pServ->m_attr_num = app_ble_server_create_service_qt(pServ->m_server_index, pServ->m_uuid, num_handle, true)))
        return false;
    {
        QMutexLocker lock(&gWaitMutex);
        gWait.wait(&gWaitMutex,500);
    }
    // add characteristics
    QMutexLocker lock(&gWaitMutex);

    for (int i = 0; i < pServ->childCount(); i++)
    {
        ServiceChar * pChar = (ServiceChar *)pServ->child(i);
        pChar->m_attr_num = app_ble_server_add_char_qt(pServ->m_server_index,pServ->m_attr_num,pChar->m_uuid,pChar->m_permissions,0,pChar->m_properties);
        gWait.wait(&gWaitMutex,500);
    }
    return true;

}

// user wants to manualy create a new service
void BLE::on_btnNewService()
{

    bool ok=false;
    ushort uuid = mBsa->ui->edBleNewSrvUuid->text().toUShort(&ok, 16);
    Service * pItem = new Service(uuid);
    pItem->setText(0,mBsa->ui->edBleNewSrvUuid->text());
    mBsa->ui->trBleSrvc->addTopLevelItem(pItem);

    setChildFont(pItem,Qt::blue);
    mBsa->ui->trBleSrvc->setCurrentItem(pItem);
}

int BLE::RegClient()
{
    int inx = -1;
    if (-1 != (inx = app_ble_client_register_qt(gNextClientUuid)))
    {
        gNextClientUuid++;
    }
    return inx;

}

int BLE::RegSrv()
{
    int inx = -1;
    if (-1 != (inx = app_ble_server_register_qt(gNextServerUuid,NULL)))
    {
        gNextServerUuid++;
    }
    return inx;
}

int BLE::getCurClientId()
{
    QTreeWidgetItem * pDev = mBsa->ui->trScanResults->currentItem();
    if (pDev == NULL)
        return -1;

    for (;pDev->parent(); pDev = pDev->parent())
        ;

    return pDev->data(1,Qt::UserRole).toInt();
}

QTreeWidgetItem * findChild(QTreeWidgetItem *pParent, UINT16 uuid)
{
    for (int i = 0; i < pParent->childCount(); i++)
    {
        UINT32 u = pParent->child(i)->data(0,Qt::UserRole).toUInt();
        if (uuid == pParent->child(i)->data(0,Qt::UserRole).toUInt())
        {
            return pParent->child(i);
        }
    }
    return NULL;
}

// attribute descriptions
char * attrDescUpper(UINT16 uuid)
{
    switch(uuid)
    {
    case GATT_UUID_GAP_DEVICE_NAME       : //0x2A00
        return("DEVICE_NAME");

    case GATT_UUID_GAP_ICON://              0x2A01
        return ("ICON");

    case GATT_UUID_GAP_PREF_CONN_PARAM   : //0x2A04
        return ("PREF_CONN_PARAM");

    case GATT_UUID_GAP_CENTRAL_ADDR_RESOL    : //0x2AA6
        return ("ADDR");

    case BSA_BLE_GATT_UUID_SENSOR_LOCATION:
        return ("Sensor Location");

    case BSA_BLE_GATT_UUID_CSC_FEATURE:
        return ("CSC Feature");

    case BSA_BLE_GATT_UUID_BLOOD_PRESSURE_FEATURE:
        return ("BLOOD_PRESSURE_FEATURE");

    case BSA_BLE_GATT_UUID_BLOOD_PRESSURE_MEASUREMENT: //      0x2A35
        return ("BLOOD_PRESSURE_MEASUREMENT");

    case BSA_BLE_GATT_UUID_INTERMEDIATE_CUFF_PRESSURE: //        0x2A36
        return ("INTERMEDIATE_CUFF_PRESSURE");

    case BSA_BLE_GATT_UUID_RSC_FEATURE:
        return ("RSC_FEATURE");

    case BSA_BLE_GATT_UUID_TEMPERATURE_TYPE:
        return ("TEMPERATURE_TYPE");

    case BSA_BLE_GATT_UUID_TEMPERATURE_MEASUREMENT_INTERVAL:
        return ("TEMPERATURE_MEASUREMENT_INTERVAL");

    case BSA_BLE_GATT_UUID_BATTERY_LEVEL:
        return ("BATTERY_LEVEL");

    case BSA_BLE_GATT_UUID_MANU_NAME:
        return ("MANU_NAME");

    case BSA_BLE_GATT_UUID_MODEL_NUMBER_STR :
        return ("MODEL_NUMBER_STR");

    case BSA_BLE_GATT_UUID_SERIAL_NUMBER_STR :
        return ("Serial Number");

    case BSA_BLE_GATT_UUID_HW_VERSION_STR :
        return ("HW Version");

    case BSA_BLE_GATT_UUID_FW_VERSION_STR:
        return ("FW Version");

    case BSA_BLE_GATT_UUID_SW_VERSION_STR:
        return("SW Version");

    case BSA_BLE_GATT_UUID_SYSTEM_ID:
        return "SYSTEM ID";

    case 0x891A:
        return "HELLO CONFIG";
    case 0xF626:
        return "HELLO_NOTIFY";
    case 0x2023:
        return "HELLO SENSOR";

    default:
        return ("Unkown");
    }
}

// convert all uppercase to lower case
void fix_case(QString *str)
{
    bool bNewWord = true;
    for (int i = 0; i < str->length(); i++)
    {
        if (!bNewWord)
            str->replace(i,1,str->at(i).toLower());

        if (bNewWord = (str->at(i) == '_' || str->at(i) == ' '))
            str->replace(i,1, ' ');
    }
}


char * attrDesc(UINT16 uuid)
{
    QString qstr=attrDescUpper(uuid);
    fix_case(&qstr);
    static std::string str;
    str.assign(qstr.toStdString());
    return (char *)str.c_str();
}

// check to see if string value is already in tree
QTreeWidgetItem * checkDups(QString str, QTreeWidgetItem * pChild)
{
    for (int i = 0; i < pChild->childCount(); i++)
    {
        if (pChild->child(i)->text(0) == str)
        {
            return pChild->child(i);
        }
    }

    return (NULL);
}

void BLE::setClientUIState(bool enabled)
{
    mBsa->ui->btnConnectClBle->setText( (enabled) ? "Disconnect" : "Connect");
    mBsa->ui->btnBleRdWr->setEnabled(enabled);
}

// called when scan results tree control selection changes
void BLE::on_trDiscChanged()
{
    QTreeWidgetItem * pTwItem = mBsa->ui->trScanResults->currentItem();
    if (pTwItem == NULL)
        return;

    for (;;)
    {
        if (pTwItem->parent() == NULL)
            break;
        pTwItem = pTwItem->parent();
    }
    int conid = -1;
    int client_num = pTwItem->data(1,Qt::UserRole).toInt();
    if (client_num != -1)
    {
        conid = app_ble_cb.ble_client[client_num].conn_id;
    }

    mBsa->ui->btnConnectClBle->setEnabled(true);
    setClientUIState(conid != -1);
    if (gpRWDlg)
        gpRWDlg->updateUI();
}

// called when server tree control selection changes
void BLE::on_trBleSrvcChanged()
{
    QTreeWidgetItem * pTwItem = mBsa->ui->trBleSrvc->currentItem();
    int type = pTwItem->type();

    mBsa->ui->btnAddCharact->setEnabled(type == SERVICE);
    //mBsa->ui->btnBleNewService->setEnabled(type == SERVER);
    mBsa->ui->btnBleStartSrvc->setEnabled(type == SERVICE);
    mBsa->ui->btnBleSendInd->setEnabled(type == CHARACTERISTIC || type == VALUES);
    if (type == SERVICE)
    {
        Service* pServ = (Service *)pTwItem;
        if (pServ->m_running)
            mBsa->ui->btnBleStartSrvc->setText("Stop Service");
        else
            mBsa->ui->btnBleStartSrvc->setText("Start Service");
    }
}

// set text color for tree control item
void setChildFont(QTreeWidgetItem *pChild, QColor color)
{
    QFont font = pChild->font(0);
    pChild->setFont( 0,  font );
    QBrush b (color);
    pChild->setForeground( 0 , b );
}

void BLE::on_btnConnType()
{
    int type=0;

    if (mBsa->ui->rbBleConnTypeAuto->isChecked())
        type = 1;
    app_dm_set_ble_bg_conn_type(type);
}

void BLE::on_btnSetScan()
{
    UINT16 ble_scan_interval = mBsa->ui->edBleScanInt->text().toUShort();
    UINT16 ble_scan_window = mBsa->ui->edBleScanWindow->text().toUShort();
    app_dm_set_ble_scan_param(ble_scan_interval, ble_scan_window);
}

void BLE::on_btnSetBleConnParams()
{
    tBSA_DM_BLE_CONN_PARAM conn_param;
    conn_param.min_conn_int = mBsa->ui->edMinConInt->text().toUShort();
    conn_param.max_conn_int =mBsa->ui->edMaxConInt->text().toUShort();
    conn_param.slave_latency = mBsa->ui->edSlaveLatency->text().toUShort();
    conn_param.supervision_tout = mBsa->ui->edSupTimeout->text().toUShort();
    QString str = mBsa->ui->edBlebdAddr->text();

    if (sscanf(str.toStdString().c_str(),"%hhx.%hhx.%hhx.%hhx.%hhx.%hhx",
        &conn_param.bd_addr[0], &conn_param.bd_addr[1],
        &conn_param.bd_addr[2], &conn_param.bd_addr[3],
        &conn_param.bd_addr[4], &conn_param.bd_addr[5]) != 6)
    {
        mBsa->on_BSA_show_message_box("BD address not entered correctly");
        return;
    }
    app_dm_set_ble_conn_param(&conn_param);
}

void BLE::on_btnSetBleAdv()
{
    tBSA_DM_BLE_ADV_PARAM adv_param;

    memset(&adv_param, 0, sizeof(tBSA_DM_BLE_ADV_PARAM));
    adv_param.adv_int_min = mBsa->ui->edMinAdvInt->text().toUShort();
    adv_param.adv_int_max = mBsa->ui->edMaxAdvInt->text().toUShort();

    QString str = mBsa->ui->edBleAddAdv->text();
    if (sscanf(str.toStdString().c_str(),"%hhx.%hhx.%hhx.%hhx.%hhx.%hhx",
        &adv_param.dir_bda.bd_addr[0], &adv_param.dir_bda.bd_addr[1],
        &adv_param.dir_bda.bd_addr[2], &adv_param.dir_bda.bd_addr[3],
        &adv_param.dir_bda.bd_addr[4], &adv_param.dir_bda.bd_addr[5]) != 6)
    {
        mBsa->on_BSA_show_message_box("BD address not entered correctly");
        return;
    }
    app_dm_set_ble_adv_param(&adv_param);
}

void BLE::on_cbBleWake()
{
    app_ble_wake_configure_qt(mBsa->ui->cbWakeOnBLE->isChecked());
}

void BLE::on_btnCreateAdv()
{
    /* This is just sample code to show how BLE Adv data can be sent from application */
    /*Adv.Data should be < 31bytes including Manufacturer data,Device Name, Appearance data, Services Info,etc.. */
    /* We are not receving all fields from user to reduce the complexity */
    tBSA_DM_BLE_ADV_CONFIG adv_conf;
    UINT8 app_ble_adv_value[APP_BLE_ADV_VALUE_LEN] = {0x2b, 0x1a, 0xaa, 0xbb, 0xcc, 0xdd}; /*First 2 byte is Company Identifier Eg: 0x1a2b refers to Bluetooth ORG, followed by 4bytes of data*/

    memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
   /* start advertising */
   adv_conf.len = APP_BLE_ADV_VALUE_LEN;
   adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
   memcpy(adv_conf.p_val, app_ble_adv_value, APP_BLE_ADV_VALUE_LEN);
   /* All the masks/fields that are set will be advertised*/
   adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS|BSA_DM_BLE_AD_BIT_SERVICE|BSA_DM_BLE_AD_BIT_APPEARANCE|BSA_DM_BLE_AD_BIT_MANU;

   bool ok;
   adv_conf.appearance_data = mBsa->ui->edBleAdAp->text().toUShort(&ok,16);  //app_get_choice("Enter appearance value Eg:0x1122");

   adv_conf.uuid_val[0]= mBsa->ui->edBleAdUUID1->text().toUShort(&ok,16);
   adv_conf.uuid_val[1]= mBsa->ui->edBleAdUUID2->text().toUShort(&ok,16);
   adv_conf.uuid_val[2]= mBsa->ui->edBleAdUUID3->text().toUShort(&ok,16);
   adv_conf.uuid_val[3]= mBsa->ui->edBleAdUUID4->text().toUShort(&ok,16);
   adv_conf.uuid_val[4]= mBsa->ui->edBleAdUUID5->text().toUShort(&ok,16);
   adv_conf.uuid_val[5]= mBsa->ui->edBleAdUUID6->text().toUShort(&ok,16);

   for (adv_conf.num_service = 0; adv_conf.num_service < 6; adv_conf.num_service++)
       if (adv_conf.uuid_val[adv_conf.num_service] == 0)
           break;
   adv_conf.is_scan_rsp = mBsa->ui->rbBleScacnRspYes->isChecked() ? 1 : 0;
   app_dm_set_ble_adv_data(&adv_conf);
}
