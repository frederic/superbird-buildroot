#ifndef CMNHF_H
#define CMNHF_H

//HF Action functions declared

#ifdef  __cplusplus
extern "C"
{
#endif
    int HandleUNATResponse(char *cResp);
    int HandleATCLCCResponse(bool bSentForTotal, char *cResp);
    int HandleATCNUMResponse(char *cResp);
    int HandleATCOPSResponse(char *cResp);
    int HandleATOKResponse();

#ifdef  __cplusplus
}
#endif


#endif // CMNHF_H
