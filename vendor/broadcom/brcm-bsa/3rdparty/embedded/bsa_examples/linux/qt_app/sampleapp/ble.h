/*
 * ble.h
 */

#ifndef BLE_H
#define BLE_H

#include <QObject>
#include "blerwinfodlg.h"
#include "qtreewidget.h"

class BSA;
class QTreeWidgetItem;
class Service;

// tree control item types
#define SERVICE QTreeWidgetItem::UserType+2
#define SERVER  QTreeWidgetItem::UserType+1
#define CHARACTERISTIC QTreeWidgetItem::UserType+3
#define VALUES QTreeWidgetItem::UserType+4

// holds client characteristic data
class ClientChar : public QTreeWidgetItem
{
public:
    ClientChar(int handle, int inst, uchar properties);
    int m_handle;
    int m_inst;
    uchar m_properties;
};

// class to hold client side characteristic data
class ClientService : public QTreeWidgetItem
{
public:
    ClientService(int handle, int inst, uchar properties, int is_prim);
    int m_handle;
    int m_inst;
    uchar m_properties;
    int m_is_prim;
};

class BLE : public QObject
{
    Q_OBJECT

public:
    explicit BLE();
    void connectEdits(QWidget * pParent);
    void Init(BSA * pBSA);
    void shutdown();
    BSA * mBsa;
    bool mDiscActive;
    void frameActive();
    bool mFirst;
    int RegSrv();
    int RegClient();
    bool CreateService(Service * pServ);
    int mServerId;
    void clearResults();
    void setClientUIState(bool enabled);
    int getCurClientId();
    void disconnectAll();

public slots:
    void on_btnStartDisc();
    void on_btnBleInfo();
    void on_btnConnect();
    void on_tab_changed(int i);
    void on_btnSendInd();
    void on_edHexWord(int old, int n);
    void on_bleSrItemChanged(QTreeWidgetItem*,int);
    void on_serverDblClick(QTreeWidgetItem*,int);
    void on_btnNewService();
    void on_btnStartSrvc();
    void on_btnBleAddChar();
    void on_trBleSrvcChanged();
    void on_trDiscChanged();
    void on_btnConnType();
    void on_btnSetScan();
    void on_btnSetBleConnParams();
    void on_btnSetBleAdv();
    void on_cbBleWake();
    void on_btnCreateAdv();
};

#endif
