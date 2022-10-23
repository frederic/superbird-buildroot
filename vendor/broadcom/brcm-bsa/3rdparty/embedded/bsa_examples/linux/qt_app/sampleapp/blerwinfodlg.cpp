/*
 * BleRwInforDlg.cpp
 *
 * Dialog box class for client read/write operations.
 */

#include "bsa.h"
#include "ble.h"
#include "ui_bsa.h"

#include <QDomDocument>
#include <QThread>
#include "blerwinfodlg.h"
#include "ui_blerwinfodlg.h"

void display_info(char * fmt, va_list ap);

extern "C"
{
#include "app_ble.h"
#include "app_ble_qt.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_ble_client_db.h"
#include "app_ble_client.h"
#include "app_dm.h"
#include "bsa_ble_api.h"

// display routine for incoming attr values
void qt_disp_info(char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format,ap);
    display_info(format,ap);
    va_end(ap);
}

}

extern BLE gBle;        // main BLE object
extern BSA * gThis;     // main BSA object
BleRWInfoDlg * gpRWDlg = NULL;  // this dlg object


BleRWInfoDlg::BleRWInfoDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BleRWInfoDlg)
{
    ui->setupUi(this);
    gpRWDlg = this;
    connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(on_close()));
    connect(ui->btnBlwWrInfo, SIGNAL(clicked()), this, SLOT(onBtnWr()));
    connect(ui->btnBleRdAttr, SIGNAL(clicked()), this, SLOT(onBtnRd()));
    connect(ui->btnBleDeRegNot, SIGNAL(clicked()), this, SLOT(onDeRegNot()));
    connect(ui->btnBleClReg, SIGNAL(clicked()), this, SLOT(onRegNot()));
    connect(ui->btnDeRegAllNots, SIGNAL(clicked()), this, SLOT(onDeRegNotAll()));

    gBle.connectEdits(ui->edBleWrChar->parentWidget());

    setFixedSize(this->width(),this->height());
    updateUI();
    ui->cbBleWrPrime->setChecked(true);
}

BleRWInfoDlg::~BleRWInfoDlg()
{
    gpRWDlg = NULL;
    delete ui;
}

// called when user changes selection in client tree control
// populate dialog fields with item selected in tree control
void BleRWInfoDlg::updateUI()
{
    QTreeWidgetItem * pDev = gThis->ui->trScanResults->currentItem();
    QString str;
    if (pDev)
    {
        if (pDev->type() == CHARACTERISTIC)
        {
            UINT16 uuid = pDev->data(0, Qt::UserRole).toUInt();
            str.sprintf("%04x", uuid);
            ui->edBleWrChar->setText(str);

            uuid = pDev->parent()->data(0, Qt::UserRole).toUInt();
            str.sprintf("%04x", uuid);
            ui->edBleWriteUUiD->setText(str);

            ClientService * pService = (ClientService*)(pDev->parent());
            ui->cbBleWrPrime->setChecked(pService->m_is_prim == 1);
            ui->edBleWrUuidInst->setText(QString::number(pService->m_inst));

            ClientChar * pChar = (ClientChar*)pDev;
            ui->edBleWrCharInst->setText(QString::number(pChar->m_inst));
        }
        else if (pDev->type() == SERVICE)
        {
            ui->edBleWrChar->setText("");
            ui->edBleWrCharInst->setText("");
            UINT16 uuid = pDev->data(0, Qt::UserRole).toUInt();
            str.sprintf("%04x", uuid);
            ui->edBleWriteUUiD->setText(str);
            ClientService * pService = (ClientService*)pDev;
            ui->cbBleWrPrime->setChecked(pService->m_is_prim == 1);
            ui->edBleWrUuidInst->setText(QString::number(pService->m_inst));

        }
    }
    mClientId = gBle.getCurClientId();
}

// de-register all notifications
void BleRWInfoDlg::onDeRegNotAll()
{
    dereg(1);
}

// de-register notifications
void BleRWInfoDlg::onDeRegNot()
{
    dereg(0);
}

// de-register notifications
void  BleRWInfoDlg::dereg(int all)
{
    if (mClientId == -1)
        return;

    bool ok;

    UINT16 character_id=ui->edBleWrChar->text().toUShort(&ok,16);;
    int ser_inst_id= ui->edBleWrUuidInst->text().toInt();
    int is_primary=ui->cbBleWrPrime->isChecked();
    int char_inst_id = ui->edBleWrCharInst->text().toInt();
    UINT16 service_id = ui->edBleWriteUUiD->text().toUShort(&ok,16);

    app_ble_client_deregister_notification_qt(mClientId ,all, service_id, character_id,ser_inst_id, char_inst_id, is_primary);
}

// register for notifications
void BleRWInfoDlg::onRegNot()
{
    if (mClientId == -1)
        return;

    bool ok;

    UINT16 character_id=ui->edBleWrChar->text().toUShort(&ok,16);;
    int ser_inst_id= ui->edBleWrUuidInst->text().toInt();
    int is_primary=ui->cbBleWrPrime->isChecked();
    int char_inst_id = ui->edBleWrCharInst->text().toInt();
    UINT16 service_id = ui->edBleWriteUUiD->text().toUShort(&ok,16);

    app_ble_client_register_notification_qt(mClientId , service_id, character_id,ser_inst_id, char_inst_id, is_primary);
}


// parsed incoming data from read/notify event
static QStringList sParsedData;

// save strings to display in dialog for values read-in
void display_info(char * fmt, va_list ap)
{
    QString str;
    str.vsprintf(fmt,ap);
    if (str.right(1) == "\n")
        str.remove(str.length()-1,1);

    sParsedData.append(str);
}

// read attribute value
tBSA_STATUS bleReadAttr(int mClientId, int is_primary, UINT16 service_uuid, int ser_inst_id, UINT16 char_uuid,
                 int char_inst_id, int is_descript, UINT16 descr_uuid)
{
    tBSA_STATUS status ;
    tBSA_BLE_CL_READ p_read;
    BSA_BleClReadInit(&p_read);
    if (mClientId < 0 || mClientId >= BSA_BLE_CLIENT_MAX)
        return BSA_ERROR_SRV_BAD_CLIENT;

    p_read.conn_id = app_ble_cb.ble_client[mClientId].conn_id;

    p_read.descr_id.char_id.char_id.uuid.uu.uuid16 = char_uuid;
    p_read.descr_id.char_id.char_id.uuid.len = 2;
    p_read.descr_id.char_id.char_id.inst_id = char_inst_id;
    p_read.descr_id.char_id.srvc_id.is_primary = is_primary;
    p_read.descr_id.char_id.srvc_id.id.inst_id = ser_inst_id;
    p_read.descr_id.char_id.srvc_id.id.uuid.len = 2;
    p_read.descr_id.char_id.srvc_id.id.uuid.uu.uuid16 = service_uuid;
    p_read.descr_id.descr_id.inst_id = 0;
    p_read.descr_id.descr_id.uuid.uu.uuid16 = descr_uuid;
    p_read.descr_id.descr_id.uuid.len = 2;

    p_read.char_id.srvc_id.id.uuid.uu.uuid16 = service_uuid;
    p_read.char_id.srvc_id.id.uuid.len = 2;
    p_read.char_id.srvc_id.id.inst_id = ser_inst_id;
    p_read.char_id.srvc_id.is_primary = is_primary;
    p_read.char_id.char_id.uuid.uu.uuid16 = char_uuid;
    p_read.char_id.char_id.uuid.len = 2;
    p_read.char_id.char_id.inst_id = char_inst_id;

    p_read.auth_req = 0;
    p_read.descr = is_descript;

    if (BSA_SUCCESS != (status = BSA_BleClRead(&p_read)))
    {
        APP_ERROR1("app_ble_client_read failed status = %d", status);
    }
    return status;
}

// user pressed read button to get values for attribute
void BleRWInfoDlg::onBtnRd()
{
    tBSA_STATUS status ;
    bool ok;

    if (-1 == mClientId)
        return;

    ui->edRawData->clear();
    ui->edParsedData->clear();
    sParsedData.clear();

    ui->lbBleRdStat->setText("");

    int ser_inst_id = ui->edBleWrUuidInst->text().toInt();
    int char_inst_id = ui->edBleWrCharInst->text().toInt();
    int is_primary = ui->cbBleWrPrime->isChecked();
    int is_descript=ui->cbBleWrUseDesc->isChecked();
    UINT16 service_uuid = ui->edBleWriteUUiD->text().toUShort(&ok,16);
    UINT16 char_uuid = ui->edBleWrChar->text().toUShort(&ok, 16);
    UINT16 descr_uuid = ui->edBleWrDesc->text().toInt(&ok,16);

    bleReadAttr(mClientId,is_primary,service_uuid,ser_inst_id,char_uuid,char_inst_id,is_descript,descr_uuid);
}

// user pressed write button to set attr values on BLE server
void BleRWInfoDlg::onBtnWr()
{
    if (-1 == mClientId)
        return;
    ui->lbBleRdStat->setText("");
    bool ok;
    int ser_inst_id = ui->edBleWrUuidInst->text().toInt();
    int char_inst_id = ui->edBleWrCharInst->text().toInt();
    int is_primary = ui->cbBleWrPrime->isChecked();
    int is_descript=ui->cbBleWrUseDesc->isChecked();
    UINT16 service = ui->edBleWriteUUiD->text().toUShort(&ok,16);
    UINT16 char_id = ui->edBleWrChar->text().toUShort(&ok, 16);
    UINT16 desc_id = ui->edBleWrDesc->text().toInt(&ok,16);

    int num_bytes=2;
    UINT8 data1 = ui->edBleWrB1->text().toUShort(&ok,16);
    UINT8 data2 = ui->edBleWrB2->text().toUShort(&ok,16);
    if (ui->edBleWrB2->text().isEmpty())
        num_bytes = 1;
    UINT16 desc_val = ui->edBleWrDescVal->text().toUShort(&ok,16);

    int write_type = (ui->rbBleWrNoResp->isChecked()) ? 1 : 2;  //[i.e 1-GATT_WRITE_NO_RSP 2-GATT_WRITE]");

    app_ble_client_write_qt(mClientId,ser_inst_id, char_inst_id, is_primary, is_descript,
                     service, char_id, desc_id, num_bytes, data1, data2, write_type,desc_val);
}

// close dialog
void BleRWInfoDlg::on_close()
{
    setVisible(false);
}

// display incoming raw data from read or notify event
void BleRWInfoDlg::displayData(UINT16 uuid, UINT8 * val_data, UINT16 desc, int len)
{
    QString data;

    for (int i = 0; i < sParsedData.length(); i++)
    {
        ui->edParsedData->appendPlainText(sParsedData[i]);
    }

    data.sprintf("%d bytes read",len);
    ui->lbBleRdStat->setText(data);
    data = "00: ";
    QString strByte;
    for (int i = 0; i < len; i++)
    {
        if (i && ((i % 8)==0))
        {
            ui->edRawData->appendPlainText(data);
            data.sprintf("%02d: ", i);
        }

        strByte.sprintf("%02x ", (uint)val_data[i]);
        data += strByte;

    }

    if (len)
        ui->edRawData->appendPlainText(data);

}

// called when we receive incoming notification event
void BleRWInfoDlg::onNotify(tBSA_BLE_MSG *p_data)
{
    QString str;
    ui->edRawData->clear();
    ui->edParsedData->clear();
    sParsedData.clear();

    str.sprintf("Notification for %04x",p_data->cli_notif.char_id.char_id.uuid.uu.uuid16);
    sParsedData.append(str);

    app_ble_client_handle_notification_ex(&(p_data->cli_notif),qt_disp_info);
    displayData(p_data->cli_notif.char_id.char_id.uuid.uu.uuid16, p_data->cli_notif.value,p_data->cli_notif.descr_type.uuid.uu.uuid16,p_data->cli_notif.len);
}

// called when we receive incoming read event
void BleRWInfoDlg::onRead(tBSA_BLE_MSG *p_data)
{
    QString data;

    if(p_data->cli_read.status != BSA_SUCCESS)
    {
        data.sprintf("Read failed: %d (0x%x)", p_data->cli_read.status, p_data->cli_read.status);
        ui->lbBleRdStat->setText(data);
        return;
    }

    app_ble_client_handle_read_evt_ex(&p_data->cli_read, qt_disp_info);
    displayData(p_data->cli_read.char_id.uuid.uu.uuid16, p_data->cli_read.value,p_data->cli_read.descr_type.uuid.uu.uuid16,p_data->cli_read.len);
}
