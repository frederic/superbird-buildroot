#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ptzimpl.h"

#define C1_RS485_DIR_GIPOID     495
//turn speed ps = 1s
#define DELAY_TIME              1000000
#define DELAY_PRE_TIME          200000

////////////////////////////////////////////////////////////////////////////////
static int s_fd_485 = -1;
static struct termios s_old_tio;
struct termios tio;
BOOL g_debug = FALSE;

////////////////////////////////////////////////////////////////////////////////

void InitPtzPramas(PTZPRAMAS *pptzpramas)
{
    pptzpramas->nttys = 2;
    pptzpramas->naddr = 1;
    pptzpramas->npelco = PANTILTDEVICE_PELCO_D;
    pptzpramas->nbaudrate = 2400;
    pptzpramas->ncommand = PTZ_STOP;
    pptzpramas->noffset = 0;
}

void GetPelcoType(char *ppelco, PTZPRAMAS *pptzpramas)
{
    if ( *ppelco == 'P' ) {
        pptzpramas->npelco = PANTILTDEVICE_PELCO_P;    //pelco-P
        pptzpramas->nbaudrate = 4800;
    }
}

int Init485(PTZPRAMAS *pptzpramas)
{
    char s_dev_485[16];
    NC_PORTPARAMS s_serial_param = {pptzpramas->nbaudrate, 8, PAR_NONE, 1, 0, 0};

    sprintf(s_dev_485, "/dev/ttyS%d", pptzpramas->nttys);
    if ( (s_fd_485 = open(s_dev_485, O_RDWR | O_NOCTTY | O_NDELAY) ) < 0) {
        PRINTF(g_debug, "ERROR. Open %s device...\n",s_dev_485);
        return -1;
    }
    else
        PRINTF(g_debug, "OK. Open %s device...\n",s_dev_485);

    if ( InitPTZPort(s_fd_485, &s_serial_param) < 0 ) {
        PRINTF(g_debug, "Error. Init RS-485 device...\n");
        Deinit485();
        return -1;
    }
    else
        PRINTF(g_debug, "OK. Init RS-485 device...\n");

    return 0;
}

void Deinit485(void)
{
    if ( s_fd_485 > 0 ) {
        if ( tcsetattr(s_fd_485, TCSANOW, &s_old_tio) < 0 ) {
            PRINTF(g_debug, "ERROR. Restore tio value...\n");
        }
    }

    close( s_fd_485 );
    s_fd_485 = -1;
}

int InitPTZPort(int fd, const NC_PORTPARAMS *param)
{
    int ret = -1;
    int speed;

    bzero(&tio, sizeof(tio));  //set tio all word=0
    if ( tcgetattr(fd, &tio) < 0 ) {
        PRINTF(g_debug, "tcgetattr fail\n");
        goto done;
    }
    memcpy(&s_old_tio, &tio, sizeof(tio));

    cfmakeraw(&tio);

    if ( param->nBaudRate > 0 ) { //set in/out boudrate
        switch ( param->nBaudRate ) {
        case 1200:
            speed = B1200;
            break;
        case 1800:
            speed = B1800;
            break;
        case 2400:
            speed = B2400;
            break;
        case 4800:
            speed = B4800;
            break;
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        case 230400:
            speed = B230400;
            break;
        default:
            PRINTF(g_debug, "Invalid BaudRate Value : %d\n", param->nBaudRate);
            return ret;
        }
    }
    else {
        PRINTF(g_debug, "Invalid BaudRate Value : %d\n", param->nBaudRate);
        return ret;
    }
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);
    PRINTF(g_debug, "set baudrate=%d, %d\n", speed, param->nBaudRate);

    tio.c_cflag |= (CLOCAL | CREAD);
    // Character size
    tio.c_cflag &= ~CSIZE;
    switch ( param->nCharSize ) {
    case 5 :
        tio.c_cflag |= CS5;
        break;
    case 6 :
        tio.c_cflag |= CS6;
        break;
    case 7 :
        tio.c_cflag |= CS7;
        break;
    case 8 :
    default:
        tio.c_cflag |= CS8;
        break;
    }

    // Parity bit
    switch ( param->nParityBit ) {
    case PAR_NONE :
        tio.c_cflag &= ~PARENB;
        break;
    case PAR_EVEN :
        tio.c_cflag |= PARENB;
        tio.c_cflag &= ~PARODD;
        break;
    case PAR_ODD :
        tio.c_cflag |= PARENB;
        tio.c_cflag |= PARODD;
        break;
    }

    // Stop bit
    if ( param->nStopBit == 2 )
        tio.c_cflag |= CSTOPB;     // 2 Stop Bit
    else
        tio.c_cflag &= ~CSTOPB;    // 1 Stop Bit

    //Other
    tio.c_cflag &= ~CRTSCTS;
    tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tio.c_lflag |= FLUSHO;   // Output flush
    tio.c_oflag &= ~OPOST;
    tio.c_oflag &= ~(ONLCR | OCRNL);
    tio.c_iflag &= ~(ICRNL | INLCR);
    tio.c_iflag &= ~(IXON | IXOFF | IXANY);

    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    tio.c_iflag = IGNBRK | IGNPAR;
    tcflush(fd, TCIFLUSH);
    tcflush(fd, TCOFLUSH);
    ret = tcsetattr(fd, TCSANOW, &tio);  //reset ttys* attr
    if ( ret != 0 ) {
        PRINTF(g_debug, "tcsetattr fail\n");
        return -1;
    }

done:
    return ret;
}

void OpenGpioPin(BOOL bsend)
{
    int fd;
    char buf[100];
    char cmd[256];

    fd = open("/etc/board_info", O_RDONLY);
    if ( fd < 0 ) {
        return;
    }
    read(fd, buf, 100);
    close(fd);
    if ( !strstr(buf, "C1") ) {
        return;
    }
    sprintf(buf, "/sys/class/gpio/gpio%d", C1_RS485_DIR_GIPOID);
    if ( access(buf, 0) != 0 ) {
        sprintf(cmd, "echo %d > /sys/class/gpio/export", C1_RS485_DIR_GIPOID);
        system(cmd);
    }
    if ( bsend ) {
        sprintf(buf, "/sys/class/gpio/gpio%d/direction", C1_RS485_DIR_GIPOID);
        if ( access(buf, 0) != 0 ) {
            sprintf(cmd, "echo out > %s", buf);
            system(cmd);
        }
        fd = open(buf, O_RDONLY);
        if ( fd < 0 ) {
            return;
        }
        read(fd, buf, 3);
        close(fd);
        if ( strncmp(buf, "out", 3) != 0 ) {
            sprintf(cmd, "echo out > /sys/class/gpio/gpio%d/direction", C1_RS485_DIR_GIPOID);
            system(cmd);
        }
        sprintf(buf, "/sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
        if ( access(buf, 0) != 0 ) {
            sprintf(cmd, "echo 1 > /sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
            system(cmd);
        }
        else {
            fd = open(buf, O_RDONLY);
            if ( fd < 0 ) {
                return;
            }
            read(fd, buf, 1);
            close(fd);
            if ( buf[0] != '1' ) {
                sprintf(cmd, "echo 1 > /sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
                system(cmd);
            }
        }
    }
    else {
        sprintf(buf, "/sys/class/gpio/gpio%d/direction", C1_RS485_DIR_GIPOID);
        if ( access(buf, 0) != 0 ) {
            sprintf(cmd, "echo in > %s", buf);
            system(cmd);
        }
        fd = open(buf, O_RDONLY);
        if ( fd < 0 ) {
            return;
        }
        read(fd, buf, 2);
        close(fd);
        if ( strncmp(buf, "in", 2) != 0 ) {
            sprintf(cmd, "echo in > /sys/class/gpio/gpio%d/direction", C1_RS485_DIR_GIPOID);
            system(cmd);
        }
        sprintf(buf, "/sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
        if ( access(buf, 0) != 0 ) {
            sprintf(cmd, "echo 0 > /sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
            system(cmd);
        }
        else {
            fd = open(buf, O_RDONLY);
            if ( fd < 0 ) {
                return;
            }
            read(fd, buf, 1);
            close(fd);
            if ( buf[0] != '0' ) {
                sprintf(cmd, "echo 0 > /sys/class/gpio/gpio%d/value", C1_RS485_DIR_GIPOID);
                system(cmd);
            }
        }
    }
}

void ReadFrom485()
{
    BYTE buff[256] = {0};
    int i, nread;
    int retval;
    fd_set rfds;
    struct timeval tv ;

    FD_ZERO(&rfds);
    FD_SET(s_fd_485,&rfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    while ( 1 ) {
        FD_ZERO(&rfds);
        FD_SET(s_fd_485,&rfds);
        retval = select(s_fd_485+1,&rfds,NULL,NULL,&tv);
        if ( retval == -1 ) {
            PRINTF(g_debug, "select error\n");
            break;
        }
        else if ( retval == 0 ){
            continue;
        }
        else if ( FD_ISSET(s_fd_485,& rfds) ){
            if ( (nread = read(s_fd_485,buff,256) ) >0 ) {
                PRINTF(g_debug, "recv: ");
                for ( i=0; i<nread; i++ ) {
                    PRINTF(g_debug, " %02X", buff[i]);
                }
                PRINTF(g_debug, "\n");
            }
        }
    }
}

BOOL WriteTo485( BYTE *pdata, int nsize )
{
    int nc = 0;

    if ( pdata && nsize > 0 )
    {
        while ( nc < nsize )
        {
            if ( write(s_fd_485, pdata+nc, 1) != 1 )
            {
                PRINTF(g_debug, "ERROR. write data to 485 \n");
                return FALSE;
            }
            nc++;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL DeviceControl(PTZPRAMAS *pptzpramas)
{
    switch ( pptzpramas->npelco ) {
        case PANTILTDEVICE_PELCO_D :
            return ControlPELCO_D(pptzpramas);
            break;
        case PANTILTDEVICE_PELCO_P :
            return ControlPELCO_P(pptzpramas);
            break;
    }

    return FALSE;
}

BOOL PELCO_D_SendStop(PTZPRAMAS *pptzpramas)
{
    COMMPELCOD Stop_Command;

    Stop_Command.Synch = 0xFF;
    Stop_Command.Addr  = (BYTE)pptzpramas->naddr;
    Stop_Command.Cmd1 = 0x00;
    Stop_Command.Cmd2 = 0x00;
    Stop_Command.Data1 = 0x00;
    Stop_Command.Data2 = 0x00;
    Stop_Command.CheckSum = (Stop_Command.Addr + Stop_Command.Cmd1 + Stop_Command.Cmd2 + Stop_Command.Data1 + Stop_Command.Data2) % 256;

    PRINTF(g_debug, "write data to 485:");
    int stop_command_size = sizeof(COMMPELCOD);
    for ( int i = 0 ; i < stop_command_size; i++ ) {
        PRINTF(g_debug, " %02X", ((BYTE*)&Stop_Command)[i]);
    }
    PRINTF(g_debug, "\n");

    BOOL err = WriteTo485( (BYTE*)&Stop_Command, stop_command_size );
    if ( !err ) {
        PRINTF(g_debug, "ERROR, send pelco_d stop fail\n");
    }
    return err;
}

// PELCO-D Protocol
BOOL ControlPELCO_D(PTZPRAMAS *pptzpramas)
{
    COMMPELCOD Command;
    BYTE nPTZSpeedPreset = (BYTE)pptzpramas->noffset;
    BOOL bpre_stop = FALSE;
    BOOL bstop = FALSE;
    BOOL err = FALSE;

    Command.Synch = 0xFF;
    Command.Addr  = (BYTE)pptzpramas->naddr;

    switch ( pptzpramas->ncommand )
    {
        case TILT_UP:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x08;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;  // tilt speed : 0x00 ~ 0x3f
            break;
        case TILT_DOWN:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x10;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;  // tilt speed : 0x00 ~ 0x3f
            break;
        case PAN_LEFT:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x04;
            Command.Data1 = nPTZSpeedPreset;  // pan speed : 0x00 ~ 0x3f
            Command.Data2 = 0x00;
            break;
        case PAN_RIGHT:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x02;
            Command.Data1 = nPTZSpeedPreset;  // pan speed : 0x00 ~ 0x3f
            Command.Data2 = 0x00;
            break;
        case PANTILT_LEFT_UP:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x0C;
            Command.Data1 = nPTZSpeedPreset;
            Command.Data2 = nPTZSpeedPreset;
            break;
        case PANTILT_LEFT_DOWN:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x14;
            Command.Data1 = nPTZSpeedPreset;
            Command.Data2 = nPTZSpeedPreset;
            break;
        case PANTILT_RIGHT_UP:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x0A;
            Command.Data1 = nPTZSpeedPreset;
            Command.Data2 = nPTZSpeedPreset;
            break;
        case PANTILT_RIGHT_DOWN:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x12;
            Command.Data1 = nPTZSpeedPreset;
            Command.Data2 = nPTZSpeedPreset;
            break;
        case ZOOM_IN:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x20;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case ZOOM_OUT:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x40;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case FOCUS_FAR:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x80;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case FOCUS_NEAR:
            Command.Cmd1 = 0x01;
            Command.Cmd2 = 0x00;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case IRIS_OPEN:
            Command.Cmd1 = 0x02;
            Command.Cmd2 = 0x00;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case IRIS_CLOSE:
            Command.Cmd1 = 0x04;
            Command.Cmd2 = 0x00;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            break;
        case SET_PRESET:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x03;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case GOTO_PRESET:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x07;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case CLE_PRESET:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x05;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case PAN_AUTO:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x07;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;  // preset : 0x00 ~ 0x3f
            bpre_stop = TRUE;
            break;
        case PAN_AUTO_STOP:
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x03;
            Command.Data1 = 0x00;
            Command.Data2 = nPTZSpeedPreset;  // preset : 0x00 ~ 0x3f
            bpre_stop = TRUE;
            break;
        default:   //PTZ_STOP
            Command.Cmd1 = 0x00;
            Command.Cmd2 = 0x00;
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            bstop = TRUE;
            break;
    }

    Command.CheckSum = (Command.Addr + Command.Cmd1 + Command.Cmd2 + Command.Data1 + Command.Data2) % 256;

    if ( bpre_stop ) {
        err = PELCO_D_SendStop(pptzpramas);
        if ( !err ) {
            return err;
        }
        usleep(DELAY_PRE_TIME);
    }

    PRINTF(g_debug, "write data to 485:");
    int command_size = sizeof(COMMPELCOD);
    for ( int i = 0 ; i < command_size; i++ ) {
        PRINTF(g_debug, " %02X", ((BYTE*)&Command)[i]);
    }
    PRINTF(g_debug, "\n");

    err = WriteTo485( (BYTE*)&Command, command_size );
    if ( !err ) {
        PRINTF(g_debug, "ERROR, send pelco-d fail\n");
        return err;
    }

    if ( !bpre_stop && !bstop ) {
        usleep(DELAY_TIME);
        err = PELCO_D_SendStop(pptzpramas);
        if ( !err ) {
            return err;
        }
    }

    PRINTF(g_debug, "OK, send pelco-d success\n");
    return err;
}

BOOL PELCO_P_SendStop(PTZPRAMAS *pptzpramas)
{
    COMMPELCOP Stop_Command;

    Stop_Command.Stx = 0xA0;
    Stop_Command.Addr  = (BYTE)pptzpramas->naddr;
    Stop_Command.Data1 = 0x00;
    Stop_Command.Data2 = 0x00;
    Stop_Command.Data3 = 0x00;
    Stop_Command.Data4 = 0x00;
    Stop_Command.Etx = 0xAF;
    Stop_Command.CheckSum = Stop_Command.Stx ^ Stop_Command.Addr ^ Stop_Command.Data1 ^ Stop_Command.Data2 ^ Stop_Command.Data3 ^ Stop_Command.Data4 ^ Stop_Command.Etx;

    PRINTF(g_debug, "write data to 485:");
    int stop_command_size = sizeof(COMMPELCOP);
    for ( int i = 0 ; i < stop_command_size; i++ ) {
        PRINTF(g_debug, " %02X", ((BYTE*)&Stop_Command)[i]);
    }
    PRINTF(g_debug, "\n");

    BOOL err = WriteTo485( (BYTE*)&Stop_Command, stop_command_size );
    if ( !err ) {
        PRINTF(g_debug, "ERROR, send pelco_p stop fail\n");
    }
    return err;
}

// PELCO P Protocol
BOOL ControlPELCO_P(PTZPRAMAS *pptzpramas)
{
    COMMPELCOP Command;
    BYTE nPTZSpeedPreset = (BYTE)pptzpramas->noffset;
    BOOL bpre_stop = FALSE;
    BOOL bstop = FALSE;
    BOOL err = FALSE;

    Command.Stx = 0xA0;
    Command.Addr = (BYTE)pptzpramas->naddr;

    switch ( pptzpramas->ncommand )
    {
        case TILT_UP:
            Command.Data1 = 0x00;
            Command.Data2 = 0x08;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;  // tilt speed : 0x00 ~ 0x3f
            break;
        case TILT_DOWN:
            Command.Data1 = 0x00;
            Command.Data2 = 0x10;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;  // tilt speed : 0x00 ~ 0x3f
            break;
        case PAN_LEFT:
            Command.Data1 = 0x00;
            Command.Data2 = 0x04;
            Command.Data3 = nPTZSpeedPreset;  // pan speed : 0x00 ~ 0x3f
            Command.Data4 = 0x00;
            break;
        case PAN_RIGHT:
            Command.Data1 = 0x00;
            Command.Data2 = 0x02;
            Command.Data3 = nPTZSpeedPreset;  // pan speed : 0x00 ~ 0x3f
            Command.Data4 = 0x00;
            break;
        case PANTILT_LEFT_UP:
            Command.Data1 = 0x00;
            Command.Data2 = 0x0C;
            Command.Data3 = nPTZSpeedPreset;
            Command.Data4 = nPTZSpeedPreset;
            break;
        case PANTILT_LEFT_DOWN:
            Command.Data1 = 0x00;
            Command.Data2 = 0x14;
            Command.Data3 = nPTZSpeedPreset;
            Command.Data4 = nPTZSpeedPreset;
            break;
        case PANTILT_RIGHT_UP:
            Command.Data1 = 0x00;
            Command.Data2 = 0x0A;
            Command.Data3 = nPTZSpeedPreset;
            Command.Data4 = nPTZSpeedPreset;
            break;
        case PANTILT_RIGHT_DOWN:
            Command.Data1 = 0x00;
            Command.Data2 = 0x12;
            Command.Data3 = nPTZSpeedPreset;
            Command.Data4 = nPTZSpeedPreset;
            break;
        case ZOOM_IN:
            Command.Data1 = 0x00;
            Command.Data2 = 0x20;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case ZOOM_OUT:
            Command.Data1 = 0x00;
            Command.Data2 = 0x40;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case FOCUS_FAR:
            Command.Data1 = 0x01;
            Command.Data2 = 0x00;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case FOCUS_NEAR:
            Command.Data1 = 0x02;
            Command.Data2 = 0x00;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case IRIS_OPEN:
            Command.Data1 = 0x04;
            Command.Data2 = 0x00;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case IRIS_CLOSE:
            Command.Data1 = 0x08;
            Command.Data2 = 0x00;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            break;
        case SET_PRESET:
            Command.Data1 = 0x00;
            Command.Data2 = 0x03;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case GOTO_PRESET:
            Command.Data1 = 0x00;
            Command.Data2 = 0x07;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case CLE_PRESET:
            Command.Data1 = 0x00;
            Command.Data2 = 0x05;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case PAN_AUTO:
            Command.Data1 = 0x00;
            Command.Data2 = 0x07;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        case PAN_AUTO_STOP:
            Command.Data1 = 0x00;
            Command.Data2 = 0x03;
            Command.Data3 = 0x00;
            Command.Data4 = nPTZSpeedPreset;   //preset 0x00~0xff
            bpre_stop = TRUE;
            break;
        default:   //PTZ_STOP
            Command.Data1 = 0x00;
            Command.Data2 = 0x00;
            Command.Data3 = 0x00;
            Command.Data4 = 0x00;
            bstop = TRUE;
            break;
    }

    Command.Etx = 0xAF;
    Command.CheckSum = Command.Stx ^ Command.Addr ^ Command.Data1 ^ Command.Data2 ^ Command.Data3 ^ Command.Data4 ^ Command.Etx;

    if ( bpre_stop ) {
        err = PELCO_P_SendStop(pptzpramas);
        if ( !err ) {
            return err;
        }
        usleep(DELAY_PRE_TIME);
    }

    PRINTF(g_debug, "write data to 485:");
    int command_size = sizeof(COMMPELCOP);
    for ( int i = 0 ; i < command_size; i++ ) {
        PRINTF(g_debug, " %02X", ((BYTE*)&Command)[i]);
    }
    PRINTF(g_debug, "\n");

    err = WriteTo485( (BYTE*)&Command, command_size );
    if ( !err ) {
        PRINTF(g_debug, "ERROR, send pelco-p fail\n");
        return err;
    }

    if ( !bpre_stop && !bstop ) {
        usleep(DELAY_TIME);
        err = PELCO_P_SendStop(pptzpramas);
        if ( !err ) {
            return err;
        }
    }

    PRINTF(g_debug, "OK, send pelco-p success\n");
    return err;
}
