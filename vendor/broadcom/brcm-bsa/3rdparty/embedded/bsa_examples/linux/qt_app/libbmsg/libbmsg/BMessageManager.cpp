/************************************************************************************
 *
 *  Copyright (C) 2013 Broadcom Corporation
 *
 *
 *
 ************************************************************************************/

#include "BMessageManager.h"
#include "bta_ma_api.h"
#include "bta_ma_co.h"

#include <QString>

extern "C"
{
#include "app_utils.h"
}

#define BROADCOM_BLUETOOTH_BMSG_MANAGER_CLASS_PATH "com/broadcom/bt/util/bmsg/BMessageManager"


typedef QString String;


const char* GetStringUTFChars(String str, void* pt)
{
    return str.toUtf8().constData();
}


String NewStringUTF(const char* pStr)
{
    QString str;

    if(pStr)
        str = pStr;
    else
        str = "";

    return str;
}

/*
 * Class:     BMessageManager
 * Method:    parseBMsgFile

 */
void* BMessageManager::parseBMsgFile(String filePath)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status ;
  tBTA_MA_BMSG * p_msg = BTA_MaBmsgCreate();

  if (p_msg == NULL)
  {
      APP_DEBUG1("%s: Unable to parse BMessage. Memory allocation for BMessage object failed", __FUNCTION__);
    return NULL;
  }

  if (filePath == NULL)
  {
    APP_DEBUG1("%s: Unable to parse BMessage. Invalid file name", __FUNCTION__);
    return NULL;
  }

  const char* filePathStr = GetStringUTFChars(filePath,NULL);
  //Open file stream
  int fd = bta_ma_co_open(filePathStr, BTA_FS_O_RDONLY);


  if (fd == BTA_FS_INVALID_FD)
  {
    APP_DEBUG1("%s: Unable to parse BMessage. Unable to open file %s", __FUNCTION__, filePathStr);
    return NULL;
  }

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaParseMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    APP_DEBUG1("%s: Error parsing BMessage. Status = %d", __FUNCTION__, status);
    if (p_msg != NULL)
    {
      BTA_MaBmsgFree(p_msg);
    }
    return NULL;
  }
  return (void*) p_msg;
}

/*
 * Class:     BMessageManager
 * Method:    parseBMsgFile

 */
void*  BMessageManager::parseBMsgFileFD(int fd)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status ;
  tBTA_MA_BMSG * p_msg = BTA_MaBmsgCreate();

  if (p_msg == NULL)
  {
      APP_DEBUG1("%s: Unable to parse BMessage. Memory allocation for BMessage object failed", __FUNCTION__);
    return NULL;
  }

  if (fd <=0)
  {
    APP_DEBUG1("%s: Unable to parse BMessage. Invalid file descriptor", __FUNCTION__);
    return NULL;
  }

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaParseMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    APP_DEBUG1("%s: Error parsing BMessage. Status = %d", __FUNCTION__, status);
    if (p_msg != NULL)
    {
      BTA_MaBmsgFree(p_msg);
    }
    return NULL;
  }
  return (void*) p_msg;
}

bool BMessageManager::writeBMsgFileFD(void* pObj, int fd)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status;
  tBTA_MA_BMSG * p_msg;

  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to write BMessage:invalid native object!", __FUNCTION__);
    return FALSE;
  }

  if (fd <= 0)
  {
    APP_DEBUG1("%s: Unable to write BMessage. Invalid file descriptor", __FUNCTION__);
    return FALSE;
  }

  p_msg = (tBTA_MA_BMSG*) pObj;

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaBuildMapBmsgObj(p_msg,&p_stream);

  if (status != BTA_MA_STATUS_OK)
  {
    APP_DEBUG1("%s: Error creating BMessage. Status = %d", __FUNCTION__, status);
    return FALSE;
  }
  return TRUE;
}


/*
 * Class:     BMessageManager
 * Method:    writeBMsgFile

 */
bool  BMessageManager::writeBMsgFile(void* pObj, String filePath)
{
  tBTA_MA_STREAM p_stream;
  tBTA_MA_STATUS status;
  tBTA_MA_BMSG * p_msg;

  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to write BMessage:invalid native object!", __FUNCTION__);
    return FALSE;
  }

  if (filePath == NULL)
  {
    APP_DEBUG1("%s: Unable to write BMessage. Invalid file name", __FUNCTION__);
    return NULL;
  }

  p_msg = (tBTA_MA_BMSG*) pObj;

  const char* filePathStr = GetStringUTFChars(filePath,NULL);
  //Open file stream
  int fd = bta_ma_co_open(filePathStr, BTA_FS_O_CREAT | BTA_FS_O_WRONLY);

  if (fd == BTA_FS_INVALID_FD)
  {
    APP_DEBUG1("%s: Unable to write BMessage. Unable to open file %s", __FUNCTION__, filePathStr);

    return FALSE;
  }

  p_stream.type=STRM_TYPE_FILE;
  p_stream.u.file.fd=fd;
  status= BTA_MaBuildMapBmsgObj(p_msg,&p_stream);
  bta_ma_co_close(fd);

  if (status != BTA_MA_STATUS_OK)
  {
    APP_DEBUG1("%s: Error creating BMessage. Status = %d", __FUNCTION__, status);
    return FALSE;
  }
  return TRUE;
}

/*
 * Class:     BMessageManager
 * Method:    createBMsg

 */
void*  BMessageManager::createBMsg()
{
  tBTA_MA_BMSG* msg = BTA_MaBmsgCreate();
  return (void*) msg;
}

/*
 * Class:     BMessageManager
 * Method:    deleteBMsg
 */
void  BMessageManager::deleteBMsg(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to delete BMessage: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgFree((tBTA_MA_BMSG*)pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBMsgMType
 */
void  BMessageManager::setBMsgMType(void* pObj, byte msgType)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage Message Type: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetMsgType( (tBTA_MA_BMSG*)pObj, (tBTA_MA_MSG_TYPE)msgType);
}

/*
 * Class:     BMessageManager
 * Method:    getBMsgMType
 */
byte  BMessageManager::getBMsgMType(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage Message Type: invalid native object!", __FUNCTION__);
    return (byte) 0;
  }

  return (byte)BTA_MaBmsgGetMsgType((tBTA_MA_BMSG*)pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBMsgOrig

 */
void*  BMessageManager::addBMsgOrig(void* pObj)
{
    if (pObj ==0) {
      APP_DEBUG1("%s: Unable to add BMessage Message Originator: invalid native object!", __FUNCTION__);
      return NULL;
    }

    return (void*) BTA_MaBmsgAddOrigToBmsg((tBTA_MA_BMSG*)pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBMsgOrig

 */
void*  BMessageManager::getBMsgOrig(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage Message Originator: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetOrigFromBmsg((tBTA_MA_BMSG*)pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBMsgEnv

 */
void*  BMessageManager::addBMsgEnv(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage Message Envelope: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgAddEnvToBmsg((tBTA_MA_BMSG*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBMsgEnv

 */
void*  BMessageManager::getBMsgEnv(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage Message Envelope: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetEnv((tBTA_MA_BMSG*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBMsgRd

 */
void  BMessageManager::setBMsgRd(void* pObj, bool isRead)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage Message Status: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetReadSts((tBTA_MA_BMSG*)pObj, (isRead == TRUE? TRUE: FALSE));
}

/*
 * Class:     BMessageManager
 * Method:    isBMsgRd
 */
bool  BMessageManager::isBMsgRd(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage Message Status: invalid native object!", __FUNCTION__);
    return FALSE;
  }

  return (TRUE == BTA_MaBmsgGetReadSts((tBTA_MA_BMSG*) pObj)? TRUE:FALSE);
}

/*
 * Class:     BMessageManager
 * Method:    setBMsgFldr
 */
void  BMessageManager::setBMsgFldr(void* pObj, String folder)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage folder: invalid native object!", __FUNCTION__);
    return;
  }

  if (folder == NULL) {
    APP_DEBUG1("%s: Unable to set BMessage folder: invalid folder!", __FUNCTION__);
    return;
  }

  const char* folderStr = GetStringUTFChars(folder,NULL);
  BTA_MaBmsgSetFolder((tBTA_MA_BMSG*)pObj, (char*)folderStr);

}

/*
 * Class:     BMessageManager
 * Method:    getBMsgFldr
 */
String  BMessageManager::getBMsgFldr(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage folder: invalid native object!", __FUNCTION__);
    return NULL;
  }
  const char* folderStr= BTA_MaBmsgGetFolder((tBTA_MA_BMSG*)pObj);
  return (folderStr==NULL? NULL: NewStringUTF(folderStr));
}

/*
 * Class:     BMessageManager
 * Method:    addBEnvChld


 */
void*  BMessageManager::addBEnvChld(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage child envelope: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgAddEnvToEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBEnvChld

 */
void*  BMessageManager::getBEnvChld(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage child envelope: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetNextEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBEnvRecip

 */
void*  BMessageManager::addBEnvRecip(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage recipient: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgAddRecipToEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBEnvRecip

 */
void*  BMessageManager::getBEnvRecip(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage recipient: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetRecipFromEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBEnvBody

 */
void*  BMessageManager::addBEnvBody(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage env body: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgAddBodyToEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBEnvBody

 */
void*  BMessageManager::getBEnvBody(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage env body: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetBodyFromEnv((tBTA_MA_BMSG_ENVELOPE*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBBodyEnc

 */
void  BMessageManager::setBBodyEnc(void* pObj, byte encoding)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body encoding: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetBodyEncoding((tBTA_MA_BMSG_BODY*)pObj, (tBTA_MA_BMSG_ENCODING) encoding);
}

/*
 * Class:     BMessageManager
 * Method:    getBBodyEnc
 */
byte  BMessageManager::getBBodyEnc(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body encoding: invalid native object!", __FUNCTION__);
    return (byte)BTA_MA_BMSG_ENC_UNKNOWN;
  }
  return (byte) BTA_MaBmsgGetBodyEncoding((tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBBodyPartId
 */
void  BMessageManager::setBBodyPartId(void* pObj, int partId)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage body part id: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyPartid( (tBTA_MA_BMSG_BODY*) pObj, (UINT16) partId);
}

/*
 * Class:     BMessageManager
 * Method:    getBBodyPartId

 */
int BMessageManager::getBBodyPartId(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body part id: invalid native object!", __FUNCTION__);
    return (int) -1;
  }

  return (int )BTA_MaBmsgGetBodyPartid( (tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    isBBodyMultiP
 */
bool  BMessageManager::isBBodyMultiP(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage is multipart attribute: invalid native object!", __FUNCTION__);
    return FALSE;
  }

  return BTA_MaBmsgIsBodyMultiPart((tBTA_MA_BMSG_BODY*)pObj)==TRUE? TRUE:FALSE;
}

/*
 * Class:     BMessageManager
 * Method:    setBBodyCharset

 */
void  BMessageManager::setBBodyCharset(void* pObj, byte charset)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage body charset: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyCharset((tBTA_MA_BMSG_BODY*) pObj, (tBTA_MA_CHARSET) charset);
}

/*
 * Class:     BMessageManager
 * Method:    getBBodyCharset
 */
byte  BMessageManager::getBBodyCharset(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body charset: invalid native object!", __FUNCTION__);
    return (byte) BTA_MA_CHARSET_UNKNOWN;
  }

  return BTA_MaBmsgGetBodyCharset((tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBBodyLang

 */
void  BMessageManager::setBBodyLang(void* pObj, byte langId)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to s BMessage body lang: invalid native object!", __FUNCTION__);
    return;
  }

  BTA_MaBmsgSetBodyLanguage((tBTA_MA_BMSG_BODY*) pObj, (tBTA_MA_BMSG_LANGUAGE)langId);
}

/*
 * Class:     BMessageManager
 * Method:    getBBodyLang
 */
byte  BMessageManager::getBBodyLang(void* pObj)
{

  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body lang: invalid native object!", __FUNCTION__);
    return (byte) BTA_MA_BMSG_LANG_UNSPECIFIED;
  }

  return (byte) BTA_MaBmsgGetBodyLanguage((tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBBodyCont

 */
void*  BMessageManager::addBBodyCont(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage body content: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgAddContentToBody((tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBBodyCont

 */
void*  BMessageManager::getBBodyCont(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage body content: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetContentFromBody((tBTA_MA_BMSG_BODY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBContNext

 */
void*  BMessageManager::getBContNext(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage next body content: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*)BTA_MaBmsgGetNextContent((tBTA_MA_BMSG_CONTENT*) pObj);

}

/*
 * Class:     BMessageManager
 * Method:    addBContMsg
 */
void  BMessageManager::addBContMsg(void* pObj, String msg)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage message to body content: invalid native object!", __FUNCTION__);
    return;
  }

  if (msg == NULL) {
    APP_DEBUG1("%s: Unable to add message: NULL message!", __FUNCTION__);
    return;
  }

  const char* msgStr = GetStringUTFChars(msg,NULL);
  BTA_MaBmsgAddMsgContent((tBTA_MA_BMSG_CONTENT*) pObj,  (char*)msgStr);


}

/*
 * Class:     BMessageManager
 * Method:    getBCont1stMsg

 */
String  BMessageManager::getBCont1stMsg(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage first message in body content: invalid native object!", __FUNCTION__);
    return NULL;
  }

  const char* msgStr =BTA_MaBmsgGetMsgContent( (tBTA_MA_BMSG_CONTENT*) pObj);
  return (msgStr == NULL? NULL: NewStringUTF(msgStr));
}

/*
 * Class:     BMessageManager
 * Method:    getBContNextMsg

 */
String  BMessageManager::getBContNextMsg(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage next message in body content: invalid native object!", __FUNCTION__);
    return NULL;
  }

  const char* msgStr =BTA_MaBmsgGetNextMsgContent( (tBTA_MA_BMSG_CONTENT*) pObj);
  return (msgStr == NULL? NULL: NewStringUTF(msgStr));
}

/*
 * Class:     BMessageManager
 * Method:    getBvCardNext

 */
void*  BMessageManager::getBvCardNext(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage next vcard: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetNextVcard((tBTA_MA_BMSG_VCARD*)pObj);
}

/*
 * Class:     BMessageManager
 * Method:    setBvCardVer

 */
void  BMessageManager::setBvCardVer(void* pObj, byte version)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage vcard version: invalid native object!", __FUNCTION__);
    return;
  }
  BTA_MaBmsgSetVcardVersion((tBTA_MA_BMSG_VCARD*) pObj, (tBTA_MA_VCARD_VERSION) version);

}

/*
 * Class:     BMessageManager
 * Method:    getBvCardVer
 */
int  BMessageManager::getBvCardVer(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to set BMessage vcard version: invalid native object!", __FUNCTION__);
    return (int) -1;
  }
  return BTA_MaBmsgGetVcardVersion((tBTA_MA_BMSG_VCARD*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    addBvCardProp
 */
void*  BMessageManager::addBvCardProp(void* pObj, byte propId, String val, String param)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to add BMessage vcard property: invalid native object!", __FUNCTION__);
    return NULL;
  }
  const char* valStr=NULL;
  const char* paramStr=NULL;

  if (val != NULL)
  {
    valStr= GetStringUTFChars(val,NULL);
  }

  if (param != NULL)
  {
    paramStr= GetStringUTFChars(param,NULL);
  }

  void* obj = (void*) BTA_MaBmsgAddVcardProp( (tBTA_MA_BMSG_VCARD*)pObj,
                        (tBTA_MA_VCARD_PROP)propId,  (char*)valStr,  (char*) paramStr);


  return obj;
}

/*
 * Class:     BMessageManager
 * Method:    getBvCardProp
 */
void*  BMessageManager::getBvCardProp(void* pObj, byte propId)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage vcard property: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetVcardProp((tBTA_MA_BMSG_VCARD*) pObj ,(tBTA_MA_VCARD_PROP) propId);
}

/*
 * Class:     BMessageManager
 * Method:    getBvCardPropNext

 */
void*  BMessageManager::getBvCardPropNext(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage vcard property: invalid native object!", __FUNCTION__);
    return NULL;
  }
  return (void*) BTA_MaBmsgGetNextVcardProp((tBTA_MA_VCARD_PROPERTY*) pObj);
}

/*
 * Class:     BMessageManager
 * Method:    getBvCardPropVal

 */
String  BMessageManager::getBvCardPropVal(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage vcard property value: invalid native object!", __FUNCTION__);
    return NULL;
  }

  const char* propParam = BTA_MaBmsgGetVcardPropValue((tBTA_MA_VCARD_PROPERTY*)pObj);
  return (propParam == NULL? NULL: NewStringUTF(propParam));
}

/*
 * Class:     BMessageManager
 * Method:    getBvCardPropParam

 */
String  BMessageManager::getBvCardPropParam(void* pObj)
{
  if (pObj ==0) {
    APP_DEBUG1("%s: Unable to get BMessage vcard property param: invalid native object!", __FUNCTION__);
    return NULL;
  }

  char* propParam = BTA_MaBmsgGetVcardPropParam((tBTA_MA_VCARD_PROPERTY*)pObj);
  return (propParam == NULL? NULL: NewStringUTF(propParam));
}


//Conversion from native charset to UTF-8 and vice versa functions

// static String decodeSMSSubmitPDU(String pdu)
// {
    // ALOGV("%s", __FUNCTION__);
    // String ret = NULL;
    // if ( NULL == pdu )
    // {
        // APP_DEBUG1("%s: NULL pdu passed!", __FUNCTION__);
        // return ret;
    // }
    // char* pdustr= (char *)GetStringUTFChars(pdu,NULL);

    // CSmsTpdu SMSDecoder(CSmsTpdu::SMS_SUBMIT);
    // char * pResult = (char *)SMSDecoder.Decode((unsigned char *)pdustr, (unsigned char *)pdustr + strlen(pdustr));

    // if ( pResult != pdustr )
    // {
        // ret  = NewStringUTF(SMSDecoder.UserData());
    // }
    // if (pdustr != NULL)
    // {
      // ReleaseStringUTFChars(pdu,pdustr);
    // }

    // return ret;


// }

// static String encodeSMSDeliverPDU(String content, String recipient, String sender, String dateTime)
// {
    // ALOGV("%s", __FUNCTION__);
    // String ret = NULL;

    // if ( NULL == content || NULL == recipient || NULL == sender || NULL == dateTime)
    // {
        // APP_DEBUG1("%s: null content or recipient or sender passed", __FUNCTION__);
        // return ret;
    // }
    // char * contentStr = (char *)GetStringUTFChars(content, NULL);
    // char * recipientStr = (char *)GetStringUTFChars(recipient, NULL);
    // char * senderStr = (char *)GetStringUTFChars(sender,NULL);
    // char * dateTimeStr = (char *)GetStringUTFChars(dateTime,NULL);

    // CSMSCAddress saSMSC;
    // char* pszEnd = NULL;
    // CSMSAddress saDestination;
    // saDestination.Address(recipientStr);

    // CSMSAddress saOriginator;
    // saOriginator.Address(senderStr);

    // int year= 0, month=0, day=0,hour=0,minute=0,second=0;
    // sscanf(dateTimeStr,"%4d%2d%2d%2d%2d%2d", &year, &month,&day, &hour,&minute,&second);

    // CSMSTime st;

    //parse date time string to setup the data structure
    // st.year(year);
    // st.month(month);
    // st.day(day);
    // st.hour(hour);
    // st.minute(minute);
    // st.second(second);

    // CSmsTpdu SMSEncoder(CSmsTpdu::SMS_DELIVER);
    // SMSEncoder.ServiceCenterAddress(saSMSC);
    // SMSEncoder.DestinationAddress(saDestination);
    // SMSEncoder.OriginatingAddress(saOriginator);

    // SMSEncoder.ServiceCenterTimeStamp(st);

    // bool bResult = SMSEncoder.Encode((const char*)contentStr, &pszEnd);
    // ret  = NewStringUTF((const char *)SMSEncoder.GetEncodedString());

    // /*
    // while ( pszEnd && *pszEnd )
    // {
        //TODO : This is multipart behavior. check to see how to handle it for SMS
        // bResult = SMSEncoder.Encode(pszEnd, &pszEnd);
    // }
    // */


    // if ( NULL != contentStr )
    // {
        // ReleaseStringUTFChars(content, contentStr);
    // }
    // if ( NULL != recipientStr )
    // {
        // ReleaseStringUTFChars(recipient, recipientStr);
    // }
    // if ( NULL != senderStr )
    // {
        // ReleaseStringUTFChars(sender, senderStr );
    // }
    // if ( NULL != dateTimeStr )
    // {
        // ReleaseStringUTFChars(dateTime, dateTimeStr );
    // }
    // return ret;
// }

