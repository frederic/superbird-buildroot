/*****************************************************************************
**
**  Name:           HFActions.cpp
**
**  Description:    Hands free actions / state machine
**
**  Copyright (c) 2010-2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <qprocess.h>
#include <qdir.h>
#include "bsa_api.h"
#include "cmnhf.h"
#include "app_hs.h"
#include "hfactions.h"
#include "bsa.h"

// Character set values
#define UTF1 "UTF-8"
#define UTF2 "UTF8"
#define GSM1 "GSM"
#define IRA1 "IRA"

//Phonebook memory storage types
#define SM1 "SM"
#define ME1 "ME"
#define DC1 "DC"
#define RC1 "RC"
#define MC1 "MC"

#define MT1 "MT"
#define SN1 "SN"
#define FD1 "FD"
#define ON1 "ON"
#define EN1 "EN"
#define LD1 "LD"

extern void PBReadCallback();
extern void CLCCReadCallback();
extern void COPSReadCallback();
extern void CNUMReadCallback();

eVndATCmdState m_eVndCurrATCmdState,m_eVndPrevATCmdState;
HFAct *pThis=NULL;

/*******************************************************************************
**
** Function         HandleUNATResponse
**
** Description      Function that is called from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on sending the unknown AT command
*******************************************************************************/
int HandleUNATResponse(char *cResp)
{
    if(NULL!=pThis)
        return pThis->HandleATUNResponse(cResp);
    return 0;
}

/*******************************************************************************
**
** Function         HandleATCLCCResponse
**
** Description      Function that is called from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on handling the AT CLCC Command response
*******************************************************************************/
int HandleATCLCCResponse(bool bSentForTotal, char *cResp)
{
    if(NULL!=pThis)
       return pThis->HandleATCLCCResponse(bSentForTotal, cResp);
    return 0;
}

/*******************************************************************************
**
** Function         HandleATCNUMResponse
**
** Description      Function that is called from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on handling the AT CNUM Command response
*******************************************************************************/
int HandleATCNUMResponse(char *cResp)
{
    if(NULL!=pThis)
       return pThis->HandleATCNUMResponse(cResp);
    return 0;
}

/*******************************************************************************
**
** Function         HandleATCOPSResponse
**
** Description      Function that is called from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on handling the AT COPS Command response
*******************************************************************************/
int HandleATCOPSResponse(char *cResp)
{
    if(NULL!=pThis)
       return pThis->HandleATCOPSResponse(cResp);
    return 0;
}

/*******************************************************************************
**
** Function         HandleATOKResponse
**
** Description      Function that is called from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on handling the AT Command OK response
*******************************************************************************/
int HandleATOKResponse()
{
    if(NULL!=pThis)
       return pThis->HandleATOKResponse();
    return 0;
}

/*******************************************************************************
**
** Function         app_hs_send_CSCS_cmd
**
** Description      Function used for sending CSCS AT Command from app_hs.c
**
** Returns          Returns SUCCESS or FAILURE on sending the CSCS AT Command
*******************************************************************************/
int app_hs_send_CSCS_cmd(ePBMemType eRecType)
{
    if(NULL!=pThis)
        return pThis->app_hs_send_CSCS_cmd(eRecType);
    return 0;
}

/*******************************************************************************
**
** Function         HFAct()
**
** Description      HFActions constructor
**
** Returns          None
*******************************************************************************/
HFAct::HFAct()
{
    pThis=this;
    m_bReadPBCmdSent = false;
    memset(m_strUNATResp,'\0',sizeof(m_strUNATResp));
}

/*******************************************************************************
**
** Function         ~HFAct()
**
** Description      HFActions destructor
**
** Returns          None
*******************************************************************************/
HFAct::~HFAct()
{
    DeleteAllPBItemsInList();
    DeleteAllCLCCItemsInList();
    DeleteAllCNUMItemsInList();
    DeleteAllCOPSItemsInList();
    pThis=NULL;
}

/*******************************************************************************
**
** Function         app_hs_send_CPBS_cmd
**
** Description      Send CPBS AT Command. This CPBS AT Command allows the phonebook
**                  memory storage to be selected
**
** Returns          Returns SUCCESS or FAILURE on sending the CPBS AT command
*******************************************************************************/
int HFAct::app_hs_send_CPBS_cmd(BOOLEAN bInitial)
{
    char cCmd[10];
    int nRetVal=0;

    if(bInitial)
        strcpy(cCmd,"+CPBS=?");
    else
        strcpy(cCmd,"+CPBS?");

    nRetVal=app_hs_send_unat(cCmd);

    if(0==nRetVal)
    {
        if(bInitial)
        {
            switch(m_LastPBRecReadType)
            {
                case eMissCallPBType:
                {
                    TransitionState(eWaitingforCPBSInitMissedResp);
                    break;
                }

                case eLastDialCombPBType:
                {
                    TransitionState(eWaitingforCPBSInitDCResp);
                    break;
                }

                case eRCPBType:
                {
                    TransitionState(eWaitingforCPBSInitRecdCallResp);
                    break;
                }

                case eCombPBType:
                default:
                {
                    TransitionState(eWaitingforCPBSInitAllPBResp);
                    break;
                }
            }
        }
        else
            TransitionState(eWaitingforCPBSFinalResp);
    }

    return nRetVal;
}

/*******************************************************************************
**
** Function         app_hs_send_CPBS_Select_cmd
**
** Description      Send CPBS Select Command. This CPBS AT Command selects the
**                  particular phonebook storage as needed
**
** Returns          Returns SUCCESS or FAILURE on sending the phonebook select AT command
*******************************************************************************/
int HFAct::app_hs_send_CPBS_Select_cmd(char *cMemType)
{
    char cCmd[10];
    int nRetVal=0;

    strcpy(cCmd,"+CPBS=\"");
    strcat(cCmd,cMemType);
    strcat(cCmd,"\"");
    nRetVal=app_hs_send_unat(cCmd);
    if(0==nRetVal)
        TransitionState(eWaitingforCPBSSelResp);
    return nRetVal;
}

/*******************************************************************************
**
** Function         app_hs_send_CSCS_cmd
**
** Description      Send CSCS AT Command. This CSCS AT Command informs the
**                  sender of the particular character set being used on target
**
** Returns          Returns SUCCESS or FAILURE on sending the CSCS supported
**                  character set AT Command
*******************************************************************************/
int HFAct::app_hs_send_CSCS_cmd(ePBMemType eRecType)
{
    char cCmd[10];
    int nRetVal=0;

    strcpy(cCmd,"+CSCS=?");

    m_LastPBRecReadType=eRecType;
    nRetVal=app_hs_send_unat(cCmd);
    if(0==nRetVal)
        TransitionState(eWaitingforCSCSResp);
    return nRetVal;
}

/*******************************************************************************
**
** Function         app_hs_send_CSCS_Select_cmd
**
** Description      Send CSCS AT select Command. This CSCS AT Select Command informs the
**                  receiver of the particular character set choosen to be used
**                  by the sender
**
** Returns          Returns SUCCESS or FAILURE on sending the CSCS select
**                  character set AT Command
*******************************************************************************/
int HFAct::app_hs_send_CSCS_Select_cmd(char *cCharSet)
{
    char cCmd[15];
    int nRetVal=0;

    strcpy(cCmd,"+CSCS=\"");
    strcat(cCmd,cCharSet);
    strcat(cCmd,"\"");
    fprintf(stdout,"app_hs_send_cscs_select_cmd: %s\n",cCmd);
    nRetVal=app_hs_send_unat(cCmd);
    if(0==nRetVal)
       TransitionState(eWaitingforCSCSSelResp);
    return nRetVal;
}

/*******************************************************************************
**
** Function         app_hs_send_CPBR_Select_cmd
**
** Description      Send CPBR AT Command. This CPBR AT Command returns the
**                  phonebook entries for a range of locations from the
**                  current phonebook storage as selected with +CPBS command
**
** Returns          Returns SUCCESS or FAILURE on sending the CPBR AT Command
**
*******************************************************************************/
int HFAct::app_hs_send_CPBR_Select_cmd(char* StartIndex, char* EndIndex)
{
    char cCmd[15];
    int nRetVal=0;

    fprintf(stdout,"app_hs_send_cpbr_select_cmd: StartIndex:%s,EndIndex:%s\n",StartIndex,EndIndex);

    strcpy(cCmd,"+CPBR=");
    strcat(cCmd,StartIndex);
    strcat(cCmd,",");
    strcat(cCmd,EndIndex);

    nRetVal=app_hs_send_unat(cCmd);
    if(0==nRetVal)
       TransitionState(eWaitingforCPBRResp);
    return nRetVal;
}

/*******************************************************************************
**
** Function         HandleATCLCCResponse
**
** Description      Handle AT CLCC command response
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command response
**
*******************************************************************************/
int HFAct::HandleATCLCCResponse(bool bSentForTotal, char *cResp)
{
    QString strTemp=cResp;
    int nFirstLoc=0,nSecondLoc=0;
    strTemp.toUpper();
    strTemp.trimmed();
    CLCCItem *pCLCCRec = NULL;

    nFirstLoc=strTemp.indexOf("+CLCC:");
    if(nFirstLoc >=0)
       strTemp=strTemp.mid(nFirstLoc+strlen("+CLCC:")-1);

    nFirstLoc = strTemp.indexOf('"');
    if(nFirstLoc >=0)
    {
        if(bSentForTotal)
            m_nCLCCRespValue++;
        else
        {
            pCLCCRec=new CLCCItem();
            if(NULL==pCLCCRec)
                return -1;

            pCLCCRec->m_MultDetails = strTemp.left(nFirstLoc-1);
            pCLCCRec->m_MultDetails.trimmed();
            pCLCCRec->m_MultDetails.remove('"');
        }

        strTemp=strTemp.mid(nFirstLoc+1);
        nSecondLoc = strTemp.indexOf(",");
        if(nSecondLoc >=0)
        {
            QString strNum = strTemp.left(nSecondLoc);
            strNum.trimmed();
            strNum.remove('"');

            if(m_strCallers.isEmpty())
            {
                m_strCallers = "Caller ID: ";
                m_strCallers += strNum;
            }
            else if(!m_strCallers.contains(strNum))
            {
                m_strCallers += ", ";
                m_strCallers += strNum;
            }

            if(!bSentForTotal && NULL!=pCLCCRec)
            {
                pCLCCRec->m_Number = strNum;
                strTemp=strTemp.mid(nSecondLoc+1);
                pCLCCRec->m_RecType = strTemp;
                AddCLCCItemToList(pCLCCRec);
            }

            TransitionState(eWaitingforCLCCOK);
            return 0;
        }
    }

    return -1;
}

/*******************************************************************************
**
** Function         HandleATCNUMResponse
**
** Description      Handle AT CNUM command response
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command response
**
*******************************************************************************/
int HFAct::HandleATCNUMResponse(char *cResp)
{
    QString strTemp = cResp;

    //parse the response here
    int nFirstLoc=0, nSecondLoc=0;
    strTemp.toUpper();
    strTemp.trimmed();

    nFirstLoc=strTemp.indexOf("+CNUM:");
    if(nFirstLoc >=0)
       strTemp=strTemp.mid(nFirstLoc+strlen("+CNUM:")-1);

    nFirstLoc = strTemp.indexOf(",");
    if(nFirstLoc >=0)
    {
        CNUMItem *pCNUMRec=new CNUMItem();
        if(NULL==pCNUMRec)
            return -1;

        pCNUMRec->m_Descr = strTemp.left(nFirstLoc);
        pCNUMRec->m_Descr.trimmed();
        pCNUMRec->m_Descr.remove('"');

        strTemp=strTemp.mid(nFirstLoc+1);
        nSecondLoc = strTemp.indexOf(",");
        if(nSecondLoc >=0)
        {
            pCNUMRec->m_Number = strTemp.left(nSecondLoc);
            pCNUMRec->m_Number.trimmed();
            pCNUMRec->m_Number.remove('"');
            strTemp=strTemp.mid(nSecondLoc+1);
            pCNUMRec->m_RecType = strTemp;
            AddCNUMItemToList(pCNUMRec);
            TransitionState(eWaitingforCNUMOK);
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
**
** Function         HandleATCOPSResponse
**
** Description      Handle AT COPS command response
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command response
**
*******************************************************************************/
int HFAct::HandleATCOPSResponse(char *cResp)
{
    QString strTemp=cResp;
    int nFirstLoc=0,nSecondLoc=0;
    strTemp.toUpper();
    strTemp.trimmed();

    nFirstLoc=strTemp.indexOf("+COPS:");
    if(nFirstLoc >=0)
       strTemp=strTemp.mid(nFirstLoc+strlen("+COPS:")-1);

    nFirstLoc = strTemp.indexOf(",");
    if(nFirstLoc >=0)
    {
        COPSItem *pCOPSRec=new COPSItem();
        if(NULL==pCOPSRec)
            return -1;

        pCOPSRec->m_PLMNMode = strTemp.left(nFirstLoc);
        pCOPSRec->m_PLMNMode.trimmed();
        pCOPSRec->m_PLMNMode.remove('"');

        strTemp=strTemp.mid(nFirstLoc+1);
        nSecondLoc = strTemp.indexOf(",");
        if(nSecondLoc >=0)
        {
            pCOPSRec->m_Format = strTemp.left(nSecondLoc);
            pCOPSRec->m_Format.trimmed();
            pCOPSRec->m_Format.remove('"');
            strTemp=strTemp.mid(nSecondLoc+1);
            pCOPSRec->m_Status = strTemp;
            AddCOPSItemToList(pCOPSRec);
            TransitionState(eWaitingforCOPSOK);
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
**
** Function         HandleATUNResponse
**
** Description      Handle unknown AT command response
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command response
**
*******************************************************************************/
int HFAct::HandleATUNResponse(char *cResp)
{
    QString strTemp=cResp;
    int nFirstLoc=0,nSecondLoc=0;
    strTemp.toUpper();
    strTemp.trimmed();

    // Handle CPBR response here
    if(strTemp.contains("+CPBR") == TRUE)
    {
        nFirstLoc=strTemp.indexOf(",");
        if(nFirstLoc >=0)
        {
            nSecondLoc=strTemp.indexOf(",",nFirstLoc+1);

            if(nSecondLoc < 0)
                return -1;

            PBItem *PBRec=new PBItem();
            if(NULL==PBRec)
                return -1;

            PBRec->m_ContactNum=strTemp.mid(nFirstLoc+1,nSecondLoc-nFirstLoc-1);
            PBRec->m_ContactNum.trimmed();
            PBRec->m_ContactNum.remove('"');

            nFirstLoc=nSecondLoc;
            nSecondLoc=strTemp.indexOf(",",nFirstLoc+1);

            if(nSecondLoc < 0)
                return -1;

            PBRec->m_RecType=strTemp.mid(nFirstLoc+1,nSecondLoc-nFirstLoc-1);

            nFirstLoc=nSecondLoc;
            PBRec->m_ContactName=strTemp.mid(nFirstLoc+1,nSecondLoc-nFirstLoc-1);
            PBRec->m_ContactName.remove('"');
            nFirstLoc=PBRec->m_ContactName.indexOf("/");

            if(nFirstLoc >=0)
                PBRec->m_ContactName=PBRec->m_ContactName.left(nFirstLoc);

            fprintf(stdout,"Contact #, type and name:%s,%s,%s\n",PBRec->m_ContactNum.toStdString().c_str(),
                PBRec->m_RecType.toStdString().c_str(),PBRec->m_ContactName.toStdString().c_str());
            AddPBItemToList(PBRec);
        }
    }
    else
    // Handle other unknown AT Commands responses here
    if(NULL!=cResp && strlen(cResp) < sizeof(m_strUNATResp))
    {
        strcpy(m_strUNATResp,cResp);
        TransitionState(eWaitingforUNATOK);
    }
    return 0;
}

/*******************************************************************************
**
** Function         HandleATOKResponse
**
** Description      Handle OK responses for AT Commands sent and transition state
**                  machine as needed
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command OK response
**
*******************************************************************************/
int HFAct::HandleATOKResponse()
{
    QString strTemp=m_strUNATResp;

    fprintf(stdout, "Curr State: %d, Prev State: %d\n", m_eVndCurrATCmdState,m_eVndPrevATCmdState);

    strTemp.toUpper();

    // Handle OK response here after CSCS command response has been obtained
    if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCSCSResp==m_eVndPrevATCmdState)
    {
        return HandleCSCSOKResp(strTemp);
    }
    else
    // Handle OK response here after CSCS select response has been obtained
    if(eWaitingforCSCSSelResp==m_eVndCurrATCmdState)
    {
        return app_hs_send_CPBS_cmd(TRUE);
    }
    else
    // Handle OK response here after CPBS Init response has been obtained
     if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCPBSInitAllPBResp==m_eVndPrevATCmdState)
     {
         return HandleCPBSInitOKPBResp(strTemp);
     }
     else
     // Handle response here for CPBS Missed calls details
     if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCPBSInitMissedResp==m_eVndPrevATCmdState)
     {
         memset(m_cLastPBType,'\0',sizeof(m_cLastPBType));
         if(strTemp.contains(MC1) == TRUE)
             strcpy(m_cLastPBType,MC1);

         if(strlen(m_cLastPBType) > 0)
             return app_hs_send_CPBS_Select_cmd(m_cLastPBType);
         else
             ResetState();
     }
     else
     // Handle response here for CPBS Dial combo PB details
     if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCPBSInitDCResp==m_eVndPrevATCmdState)
     {
          memset(m_cLastPBType,'\0',sizeof(m_cLastPBType));
          if(strTemp.contains(DC1) == TRUE)
              strcpy(m_cLastPBType,DC1);

          if(strlen(m_cLastPBType) > 0)
              return app_hs_send_CPBS_Select_cmd(m_cLastPBType);
          else
              ResetState();
      }
      else
      // Handle response here for CPBS Received calls details
      if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCPBSInitRecdCallResp==m_eVndPrevATCmdState)
      {
          memset(m_cLastPBType,'\0',sizeof(m_cLastPBType));
          if(strTemp.contains(RC1) == TRUE)
             strcpy(m_cLastPBType,RC1);

          if(strlen(m_cLastPBType) > 0)
             return app_hs_send_CPBS_Select_cmd(m_cLastPBType);
          else
             ResetState();
      }
      else
      // Handle response here for CPBS Select details
      if(eWaitingforCPBSSelResp==m_eVndCurrATCmdState)
      {
          return app_hs_send_CPBS_cmd(FALSE);
      }
      else
      // Handle here for CPBS Final response details
      if(eWaitingforUNATOK==m_eVndCurrATCmdState && eWaitingforCPBSFinalResp==m_eVndPrevATCmdState)
      {
          return HandleCPBSFinalResp(strTemp);
      }
      else
      if(eWaitingforCPBRResp==m_eVndCurrATCmdState)
      {
          TransitionState(eWaitingforNone);
          PBReadCallback();
          return 0;
      }
      else
      if(eWaitingforCNUMOK==m_eVndCurrATCmdState)
      {
          TransitionState(eWaitingforNone);
          CNUMReadCallback();
          return 0;
      }
      else
      if(eWaitingforCOPSOK==m_eVndCurrATCmdState)
      {
          TransitionState(eWaitingforNone);
          COPSReadCallback();
          return 0;
      }
      else
      if(eWaitingforCLCCOK==m_eVndCurrATCmdState)
      {
          TransitionState(eWaitingforNone);
          CLCCReadCallback();
          return 0;
      }
      return -1;
}

/*******************************************************************************
**
** Function         HandleCSCSOKResp
**
** Description      Helper function - Handle CSCS OK responses for AT Commands sent
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command OK response
**
*******************************************************************************/
int HFAct::HandleCSCSOKResp(QString &strTemp)
{
    char cCharset[10];
    if(strTemp.contains(UTF1) == TRUE)
        strcpy(cCharset,UTF1);
    else
    if(strTemp.contains(UTF2) == TRUE)
       strcpy(cCharset,UTF2);
    else
    if(strTemp.contains(GSM1) == TRUE)
       strcpy(cCharset,GSM1);
    else
    if(strTemp.contains(IRA1) == TRUE)
       strcpy(cCharset,IRA1);

    if(strlen(cCharset) >0)
        return app_hs_send_CSCS_Select_cmd(cCharset);
    else
        ResetState();
    return 0;
}

/*******************************************************************************
**
** Function         HandleCPBSInitOKPBResp
**
** Description      Helper function - Handle CPBS Init OK responses for AT Commands sent
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command OK response
**
*******************************************************************************/
int HFAct::HandleCPBSInitOKPBResp(QString &strTemp)
{
    memset(m_cLastPBType,'\0',sizeof(m_cLastPBType));
    if(strTemp.contains(MT1) == TRUE)
        strcpy(m_cLastPBType,MT1);
    else
    if(strTemp.contains(ME1) == TRUE)
       strcpy(m_cLastPBType,ME1);
    else
    if(strTemp.contains(SM1) == TRUE)
       strcpy(m_cLastPBType,SM1);
    else
    if(strTemp.contains(ON1) == TRUE)
       strcpy(m_cLastPBType,ON1);
    else
    if(strTemp.contains(SN1) == TRUE)
       strcpy(m_cLastPBType,SN1);
    else
    if(strTemp.contains(FD1) == TRUE)
        strcpy(m_cLastPBType,FD1);
    else
    if(strTemp.contains(LD1) == TRUE)
       strcpy(m_cLastPBType,LD1);
    else
    if(strTemp.contains(EN1) == TRUE)
       strcpy(m_cLastPBType,EN1);
    else
    if(strTemp.contains(DC1) == TRUE)
       strcpy(m_cLastPBType,DC1);

    if(strlen(m_cLastPBType) > 0)
       return app_hs_send_CPBS_Select_cmd(m_cLastPBType);
    else
       ResetState();
    return 0;
}

/*******************************************************************************
**
** Function         HandleCPBSFinalResp
**
** Description      Helper function - Handle CPBS Final OK responses for AT Commands sent
**
** Returns          Returns SUCCESS or FAILURE on receiving the AT Command OK response
**
*******************************************************************************/
int HFAct::HandleCPBSFinalResp(QString &strTemp)
{
    char cStartIndex[3],cEndIndex[8];
    int nIndex=0,nTempIndex=0, nRetVal=0;

    if(strTemp.contains(m_cLastPBType) == FALSE)
        return -1;

    nIndex=strTemp.indexOf(",");
    if(nIndex >=0)
        nTempIndex=strTemp.indexOf(",",nIndex+1);

    if(nTempIndex > 0)
    {
        strTemp=strTemp.mid(nIndex+1,(nTempIndex-1)-nIndex);
        if(strTemp.length() > 0)
        {
            strcpy(cEndIndex,strTemp.toStdString().c_str());
            fprintf(stdout, "Start,end index,Index Value,Temp Index value:%s,%s,%d,%d\n",cStartIndex,cEndIndex,nIndex,nTempIndex-1);
            if(atoi(cEndIndex) > 0)
            {
                strcpy(cStartIndex,"1");
                DeleteAllPBItemsInList();
                nRetVal=app_hs_send_CPBR_Select_cmd(cStartIndex,cEndIndex);
                if(0==nRetVal)
                    m_bReadPBCmdSent=true;
            }
            else
            {
                TransitionState(eWaitingforNone);
                PBReadCallback();
                return 0;
            }
        }
    }
    return 0;
}

/*******************************************************************************
**
** Function         DeleteAllPBItemsInList
**
** Description      Delete all phonebook items in the list
**
** Returns          None
*******************************************************************************/
void HFAct::DeleteAllPBItemsInList()
{
    int nCount=0;
    nCount=m_PBList.count();
    PBItem *pItem=NULL;

    m_PBListLock.lockForWrite();
    for (int i=0; i < nCount; i++)
    {
        pItem=NULL;
        pItem=m_PBList.at(i);
        delete pItem;
    }

    m_PBList.clear();
    m_PBListLock.unlock();
}

/*******************************************************************************
**
** Function         DeleteAllCNUMItemsInList
**
** Description      Delete all CNUM items in the list
**
** Returns          None
*******************************************************************************/
void HFAct::DeleteAllCNUMItemsInList()
{
    int nCount=0;
    nCount=m_CNUMList.count();
    CNUMItem *pItem=NULL;

    m_CNUMListLock.lockForWrite();
    for (int i=0; i < nCount; i++)
    {
        pItem=NULL;
        pItem=m_CNUMList.at(i);
        delete pItem;
    }

    m_CNUMList.clear();
    m_CNUMListLock.unlock();
}

/*******************************************************************************
**
** Function         DeleteAllCOPSItemsInList
**
** Description      Delete all COPS items in the list
**
** Returns          None
*******************************************************************************/
void HFAct::DeleteAllCOPSItemsInList()
{
    int nCount=0;
    nCount=m_COPSList.count();
    COPSItem *pItem=NULL;

    m_COPSListLock.lockForWrite();
    for (int i=0; i < nCount; i++)
    {
        pItem=NULL;
        pItem=m_COPSList.at(i);
        delete pItem;
    }

    m_COPSList.clear();
    m_COPSListLock.unlock();
}

/*******************************************************************************
**
** Function         DeleteAllCLCCItemsInList
**
** Description      Delete all CLCC items in the list
**
** Returns          None
*******************************************************************************/
void HFAct::DeleteAllCLCCItemsInList()
{
    int nCount=0;
    nCount=m_CLCCList.count();
    CLCCItem *pItem=NULL;

    m_CLCCListLock.lockForWrite();
    for (int i=0; i < nCount; i++)
    {
        pItem=NULL;
        pItem=m_CLCCList.at(i);
        delete pItem;
    }

    m_CLCCList.clear();
    m_CLCCListLock.unlock();
}

/*******************************************************************************
**
** Function         AddPBItemToList
**
** Description      Add phonebook item into the list
**
** Returns          None
*******************************************************************************/
void HFAct::AddPBItemToList(PBItem *pItem)
{
    if(NULL!=pItem)
    {
        m_PBListLock.lockForWrite();
        m_PBList.append(pItem);
        m_PBListLock.unlock();
    }
}

/*******************************************************************************
**
** Function         AddCLCCItemToList
**
** Description      Add CLCC item into the list
**
** Returns          None
*******************************************************************************/
void HFAct::AddCLCCItemToList(CLCCItem *pItem)
{
    if(NULL!=pItem)
    {
        m_CLCCListLock.lockForWrite();
        m_CLCCList.append(pItem);
        m_CLCCListLock.unlock();
    }
}

/*******************************************************************************
**
** Function         AddCNUMItemToList
**
** Description      Add CNUM item into the list
**
** Returns          None
*******************************************************************************/
void HFAct::AddCNUMItemToList(CNUMItem *pItem)
{
    if(NULL!=pItem)
    {
        m_CNUMListLock.lockForWrite();
        m_CNUMList.append(pItem);
        m_CNUMListLock.unlock();
    }
}

/*******************************************************************************
**
** Function         AddCOPSItemToList
**
** Description      Add COPS item into the list
**
** Returns          None
*******************************************************************************/
void HFAct::AddCOPSItemToList(COPSItem *pItem)
{
    if(NULL!=pItem)
    {
        m_COPSListLock.lockForWrite();
        m_COPSList.append(pItem);
        m_COPSListLock.unlock();
    }
}

/*******************************************************************************
**
** Function         ObtainPBItemCount
**
** Description      Obtain phone book item count in the list
**
** Returns          None
*******************************************************************************/

int HFAct::ObtainPBItemCount()
{
    int nCount=0;
    m_PBListLock.lockForWrite();
    nCount=m_PBList.count();
    m_PBListLock.unlock();
    return nCount;
}

/*******************************************************************************
**
** Function         ObtainPBItemfromList
**
** Description      Obtain phone book item from list based on item index
**
** Returns          None
*******************************************************************************/
void HFAct::ObtainPBItemfromList(int nItemIndex, PBItem **pItem)
{
    *pItem=NULL;

    m_PBListLock.lockForWrite();
    if(nItemIndex <  m_PBList.count())
        *pItem=m_PBList.at(nItemIndex);
    m_PBListLock.unlock();
}

/*******************************************************************************
**
** Function         ObtainCNUMItemCount
**
** Description      Obtain CNUM item count in the list
**
** Returns          None
*******************************************************************************/
int HFAct::ObtainCNUMItemCount()
{
    int nCount=0;
    m_CNUMListLock.lockForWrite();
    nCount=m_CNUMList.count();
    m_CNUMListLock.unlock();
    return nCount;
}

/*******************************************************************************
**
** Function         ObtainCNUMItemfromList
**
** Description      Obtain CNUM item from list based on item index
**
** Returns          None
*******************************************************************************/
void HFAct::ObtainCNUMItemfromList(int nItemIndex, CNUMItem **pItem)
{
    *pItem=NULL;

    m_CNUMListLock.lockForWrite();
    if(nItemIndex <  m_CNUMList.count())
        *pItem=m_CNUMList.at(nItemIndex);
    m_CNUMListLock.unlock();
}

/*******************************************************************************
**
** Function         ObtainCLCCItemCount
**
** Description      Obtain CLCC item count in the list
**
** Returns          None
*******************************************************************************/
int HFAct::ObtainCLCCItemCount()
{
    int nCount=0;
    m_CLCCListLock.lockForWrite();
    nCount=m_CLCCList.count();
    m_CLCCListLock.unlock();
    return nCount;
}

/*******************************************************************************
**
** Function         ObtainCLCCItemfromList
**
** Description      Obtain CLCC item from list based on item index
**
** Returns          None
*******************************************************************************/
void HFAct::ObtainCLCCItemfromList(int nItemIndex, CLCCItem **pItem)
{
    *pItem=NULL;

    m_CLCCListLock.lockForWrite();
    if(nItemIndex <  m_CLCCList.count())
        *pItem=m_CLCCList.at(nItemIndex);
    m_CLCCListLock.unlock();
}

/*******************************************************************************
**
** Function         ObtainCOPSItemCount
**
** Description      Obtain COPS item count in the list
**
** Returns          None
*******************************************************************************/
int HFAct::ObtainCOPSItemCount()
{
    int nCount=0;
    m_COPSListLock.lockForWrite();
    nCount=m_COPSList.count();
    m_COPSListLock.unlock();
    return nCount;
}

/*******************************************************************************
**
** Function         ObtainCOPSItemfromList
**
** Description      Obtain COPS item from list based on item index
**
** Returns          None
*******************************************************************************/
void HFAct::ObtainCOPSItemfromList(int nItemIndex, COPSItem **pItem)
{
    *pItem=NULL;

    m_COPSListLock.lockForWrite();
    if(nItemIndex <  m_COPSList.count())
        *pItem=m_COPSList.at(nItemIndex);
    m_COPSListLock.unlock();
}

/*******************************************************************************
**
** Function         TransitionState
**
** Description      Transition state
**
** Returns          None
*******************************************************************************/
void HFAct::TransitionState(eVndATCmdState eNewState)
{
    m_eVndPrevATCmdState=m_eVndCurrATCmdState;
    m_eVndCurrATCmdState=eNewState;
}

/*******************************************************************************
**
** Function         ResetState
**
** Description      Reset current state
**
** Returns          None
*******************************************************************************/
void HFAct::ResetState()
{
    m_eVndCurrATCmdState=eWaitingforNone;
    m_eVndPrevATCmdState=m_eVndCurrATCmdState;
}
