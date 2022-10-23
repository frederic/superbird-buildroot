
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

//#include</linux/amlogic/cec_common.h>
#include "hdmi_tx_cec_20.h"

#define DEV_CEC			"/dev/cec"
static int      Cec_fd = 0;
#define HDR 0
#define OPCODE 1
#define OPERAND 2

#define CEC_MSG_HDR_BROADCAST 0x0F
#define CEC_MSG_HDR 0x05
#define INITIATE_ARC                     0XC0
#define	REPORT_ARC_INITIATED             0XC1
#define REPORT_ARC_TERMINATED            0XC2
#define	REQUEST_ARC_INITIATION           0XC3
#define	REQUEST_ARC_TERMINATION          0XC4
#define	TERMINATE_ARC                    0XC5
char CEC_MSG_ARC_INITIATED[]={CEC_MSG_HDR,REPORT_ARC_INITIATED};

static pthread_t CEC_Event_Thread;
struct pollfd polling;
static bool arc_not_initialised = true;
static char cecmsg[16] ;
static int stop =0;
static char msg_to_send[16];

void process_cec_msg(char * cec_msg,int len)
{
    int i = 0;

    switch (cec_msg[OPCODE]) {
    case INITIATE_ARC :
        printf("Got the Initiate ARC message Amplifeir requesting to start ARC\n");
        arc_not_initialised = false;	  
        write(Cec_fd,CEC_MSG_ARC_INITIATED, 2);
        printf("Sending the ARC initiated message to the Amplifier\n");
	stop =1;
	break;

    case CEC_OC_GIVE_PHYSICAL_ADDRESS :
        printf("CEC_OC_GIVE_PHYSICAL_ADDRESS\n");
        msg_to_send[0] = CEC_MSG_HDR_BROADCAST;
        msg_to_send[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
        msg_to_send[2] = 0x00;
        msg_to_send[3] = 0x00;
        msg_to_send[4] = 0x00;

        write(Cec_fd,msg_to_send,5);
        printf("Sending CEC_OC_REPORT_PHYSICAL_ADDRESS\n");
	break;
		
    default :
        {
            printf("CEC msg is ");
            for(i=0;i< len;i++) {
                printf(" 0x%x ",cec_msg[i]);
            }
        }
        printf("\n###################");
        break;
    }
}

void* CEC_event_read_fn (void *data)
{
    int timeout = -1;
    unsigned int mask = 0;
    int read_len =0;

    polling.fd= Cec_fd;
    polling.events = POLLIN;

    while(!stop) {
        mask = poll(&polling,1,timeout);
        
        if (mask & POLLIN || mask & POLLRDNORM ) {
            read_len= read(Cec_fd, &cecmsg, 1);
            process_cec_msg(cecmsg,read_len);
        }
    }
}

static void sighandler(int sig)
{
    printf("Interrupted by signal %d\n", sig);
    stop= 1;
    printf("sighandler! Done\n");
}

int main ( int argc, char *argv[] ) 
{
    char msg[16];
    int ret=0;
  
    Cec_fd = open (DEV_CEC, O_RDWR);

    if (Cec_fd < 0) {
        printf ("%s CEC_device opening returned %d",DEV_CEC);
        return -1;
    }

    ret = pthread_create(&CEC_Event_Thread, NULL, CEC_event_read_fn, NULL);
    if (ret){
        printf("Unable to create decoder event thread\n");
    }

    /* setup the signal handler for CTRL-C */
    signal(SIGINT, sighandler);
    /*kill -USR1 pid */
    signal(SIGUSR1, sighandler);

    ioctl(Cec_fd, CEC_IOC_SET_OPTION_SYS_CTRL, 0x8);
	
    ioctl(Cec_fd, CEC_IOC_ADD_LOGICAL_ADDR, 0x0);

    /* request ARC initialisation */
    msg[HDR]=CEC_MSG_HDR;
    msg[OPCODE]=REQUEST_ARC_INITIATION;
    write(Cec_fd,msg,2);
    printf("sending ARC initialisation \n");

    while (!stop) {
        sleep(1);
        if (arc_not_initialised) {
            printf("TV sending Request ARC initialisation to Amplifier \n");
            write(Cec_fd,msg,2);
        }
    }

    ret = pthread_join(CEC_Event_Thread, NULL);
    if (ret != 0) {
        printf("CEC_Event_Thread returned error\n");
    }	

    //ioctl(Cec_fd, CEC_IOC_CLR_LOGICAL_ADDR, 0x0);
    //ioctl(Cec_fd, CEC_IOC_SET_OPTION_SYS_CTRL, 0x0);
	
    close(Cec_fd);

    return 0;
}



