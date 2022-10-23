#ifndef HFACTIONS_H
#define HFACTIONS_H

#include <QReadWriteLock>
#include <QObject>

typedef enum eVndATCmdSt
{
    eWaitingforNone=0,
    eWaitingforCSCSResp=1,
    eWaitingforCSCSSelResp=2,
    eWaitingforCPBSInitAllPBResp=3,
    eWaitingforCPBSInitMissedResp=4,
    eWaitingforCPBSInitDCResp=5,
    eWaitingforCPBSInitRecdCallResp=6,
    eWaitingforCPBSSelResp=7,
    eWaitingforCPBSFinalResp=8,
    eWaitingforCPBRResp=9,
    eWaitingforCNUMOK=10,
    eWaitingforCOPSOK=12,
    eWaitingforCLCCOK=13,
    eWaitingforUNATOK=14
}eVndATCmdState;

typedef enum ePBReadType
{
    eInvalidPBType=-1,
    eCombPBType=0,
    eSIMPBType=1,
    eMEPBType=2,
    eSIMOwnPBType=3,
    eServDialPBType=4,
    eSIMFixDialPBType=5,
    eLastDialCombPBType=6,
    eSIMEmerPBType=7,
    eRCPBType=8,
    eMissCallPBType=9
}ePBMemType;

class PBItem
{
public:
    QString m_ContactName;
    QString m_ContactNum;
    QString m_RecType;
};

class CNUMItem
{
public:
    QString m_Descr;
    QString m_Number;
    QString m_RecType;
};

class CLCCItem
{
public:
    QString m_MultDetails;
    QString m_Number;
    QString m_RecType;
};

class COPSItem
{
public:
    QString m_PLMNMode;
    QString m_Format;
    QString m_Status;
    QString m_Operator;
};

class HFAct
{
public:
    explicit HFAct();
    ~HFAct();

    int HandleATOKResponse();

    int HandleATUNResponse(char *cResp);
    int HandleATCLCCResponse(bool bFromHold,char *cResp);
    int HandleATCNUMResponse(char *cResp);
    int HandleATCOPSResponse(char *cResp);

    int app_hs_send_CSCS_cmd(ePBMemType eRecType);

    int ObtainPBItemCount();
    void ObtainPBItemfromList(int nItemIndex,PBItem **pItem);

    int ObtainCNUMItemCount();
    void ObtainCNUMItemfromList(int nItemIndex,CNUMItem **pItem);

    int ObtainCLCCItemCount();
    void ObtainCLCCItemfromList(int nItemIndex,CLCCItem **pItem);

    int ObtainCOPSItemCount();
    void ObtainCOPSItemfromList(int nItemIndex, COPSItem **pItem);

    void DeleteAllCLCCItemsInList();
    void DeleteAllCNUMItemsInList();
    void DeleteAllCOPSItemsInList();

public:
    int m_nCLCCRespValue;
    bool m_bReadPBCmdSent;
    QString m_strCallers;

protected:
    int app_hs_send_CSCS_Select_cmd(char *cCharSet);
    int app_hs_send_CPBS_cmd(BOOLEAN bInitial);
    int app_hs_send_CPBS_Select_cmd(char *cCmd);
    int app_hs_send_CPBR_Select_cmd(char* StartIndex, char* nEndIndex);

    void ResetState();
    void TransitionState(eVndATCmdState eNewState);

    void DeleteAllPBItemsInList();
    void AddPBItemToList(PBItem *pItem);
    void AddCLCCItemToList(CLCCItem *pItem);
    void AddCNUMItemToList(CNUMItem *pItem);
    void AddCOPSItemToList(COPSItem *pItem);

    int  HandleCSCSOKResp(QString &strTemp);
    int  HandleCPBSInitOKPBResp(QString &strTemp);
    int  HandleCPBSFinalResp(QString &strTemp);

private:
    char m_strUNATResp[512];
    char m_cLastPBType[5];

    QReadWriteLock m_CLCCListLock;
    QList<CLCCItem*> m_CLCCList;

    QReadWriteLock m_CNUMListLock;
    QList<CNUMItem*> m_CNUMList;

    QReadWriteLock m_COPSListLock;
    QList<COPSItem*> m_COPSList;

    QReadWriteLock m_PBListLock;
    QList<PBItem*> m_PBList;

    ePBMemType m_LastPBRecReadType;
};

int app_hs_send_CSCS_cmd(ePBMemType eRecType);

#endif // HFACTIONS_H
