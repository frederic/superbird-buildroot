#ifndef __PTZIMPL_H__
#define __PTZIMPL_H__

#define PRINTF(bdebug, args...)  if ( bdebug ) {\
        printf(" " args);\
    }

typedef unsigned char BYTE;
typedef enum {
    FALSE = 0,
    TRUE = 1
} BOOL;


typedef struct PTZPramas
{
    int nttys;        // /dev/ttyS*, 0,1,2...
    int naddr;        // the addr of ptz device
    int npelco;       // pelco-D for 0, pelco-P for 1
    int nbaudrate;    // BAUDRATE
    int ncommand;     // PTZ_COMMAND_
    int noffset;      // speed for pan or lilt, or preset point
} PTZPRAMAS;

typedef struct tagPORTPARAMS
{
    int nBaudRate;
    int nCharSize;
    int nParityBit;
    int nStopBit;
    int fCRTSCTS;     // Hardware Flow Control(1/0)
    int fXONXOFF;     // Sofwware Flow Control(1/0)
} NC_PORTPARAMS;

enum PARITYBIT_ENUM
{
    PAR_NONE,
    PAR_EVEN,
    PAR_ODD
};

////////////////////////////////////////////////////////////////////////////////

enum _PTZ_COMMAND_
{
    TILT_UP,
    TILT_DOWN,
    PAN_LEFT,
    PAN_RIGHT,
    PANTILT_LEFT_UP,
    PANTILT_LEFT_DOWN,
    PANTILT_RIGHT_UP,
    PANTILT_RIGHT_DOWN,
    ZOOM_IN,
    ZOOM_OUT,
    FOCUS_FAR,
    FOCUS_NEAR,
    IRIS_OPEN,
    IRIS_CLOSE,
    SET_PRESET,
    GOTO_PRESET,
    CLE_PRESET,
    PAN_AUTO,
    PAN_AUTO_STOP,
    PTZ_STOP
};

enum _PTZ_DEVICE_ID_
{
    PANTILTDEVICE_PELCO_D,         // PELCO-D
    PANTILTDEVICE_PELCO_P         // PELCO-P
};

////////////////////////////////////////////////////////////////////////////////
typedef struct tagCOMMPELCOD
{
    BYTE Synch;    // Stx
    BYTE Addr;     // Address
    BYTE Cmd1;     // Command 1
    BYTE Cmd2;     // Command 2
    BYTE Data1;    // Data 1
    BYTE Data2;    // Data 2
    BYTE CheckSum; // CheckSum
} COMMPELCOD, *PCOMMPELCOD;

typedef struct tagCOMMPELCOP
{
    BYTE Stx;      // Stx
    BYTE Addr;     // Address
    BYTE Data1;    // Data1
    BYTE Data2;    // Data2
    BYTE Data3;    // Data3
    BYTE Data4;    // Data4
    BYTE Etx;      // Etx
    BYTE CheckSum; // CheckSum
} COMMPELCOP, *PCOMMPELCOP;

////////////////////////////////////////////////////////////////////////////////
extern BOOL g_debug;

void InitPtzPramas(PTZPRAMAS *pptzpramas);
void GetPelcoType(char *ppelco, PTZPRAMAS *pptzpramas);
int Init485(PTZPRAMAS *pptzpramas);

void Deinit485(void);
int InitPTZPort(int fd, const NC_PORTPARAMS *param);

void OpenGpioPin(BOOL bsend);

void ReadFrom485();
BOOL WriteTo485(BYTE *pdata, int nsize);

BOOL DeviceControl(PTZPRAMAS *pptzpramas);
BOOL PELCO_D_SendStop(PTZPRAMAS *pptzpramas);
BOOL ControlPELCO_D(PTZPRAMAS *pptzpramas);
BOOL PELCO_P_SendStop(PTZPRAMAS *pptzpramas);
BOOL ControlPELCO_P(PTZPRAMAS *pptzpramas);

#endif//    __PTZIMPL_H__
