#ifndef BLERWINFODLG_H
#define BLERWINFODLG_H

#include <QDialog>

extern "C"
{
#include "bsa_ble_api.h"
}
//class tBSA_BLE_MSG;

namespace Ui {
class BleRWInfoDlg;
}

class BleRWInfoDlg : public QDialog
{
    Q_OBJECT

public:
    explicit BleRWInfoDlg(QWidget *parent = 0);
    ~BleRWInfoDlg();
    int mClientId;
    void onRead(tBSA_BLE_MSG *p_data);
    void onNotify(tBSA_BLE_MSG *p_data);
    void displayData(UINT16 uuid, UINT8 * val_data, UINT16 desc,int len);
    void updateUI();
    void dereg(int all);

public slots:
    void onBtnRd();
    void onBtnWr();
    void on_close();
    void onRegNot();
    void onDeRegNot();
    void onDeRegNotAll();

private:
    Ui::BleRWInfoDlg *ui;
};

extern BleRWInfoDlg * gpRWDlg ;

#endif // BLERWINFODLG_H
