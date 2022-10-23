/*******************************************************************************
 **
 **Name:  app_main.c
 **
 **Descripition:bt audio source main
 **
 **Copyright(c) Amlogic
 **
 ******************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>

#include "app_av.h"
#ifdef APP_AV_BCST_INCLUDED
#include "app_av_bcst.h"
#endif
#include "app_utils.h"
#include "app_mgt.h"
#include "app_disc.h"
#include "app_xml_param.h"


struct msg_st{
    long int msg_type;
    int index;
};

UINT8 bddr[6];
static BOOLEAN app_av_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    UINT8 cnt;

    switch (event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        if (p_data->status.enable)
        {
            APP_DEBUG0("Bluetooth restarted => re-initialize the application");
            app_av_init(FALSE);

            for (cnt = 0; cnt < APP_AV_MAX_CONNECTIONS; cnt++)
            {
                app_av_register();
            }
        }
        break;

    case BSA_MGT_DISCONNECT_EVT:
        APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        /* Connection with the Server lost => Just exit the application */
        exit(-1);
        break;

    default:
        break;
    }
    return FALSE;
}
int read_by_file(){
    APP_DEBUG0("geting file begin");
	int bddr_file = -1;
	if ((bddr_file = open("/etc/bddr.conf", O_RDONLY)) >= 0) {
        read(bddr_file, &bddr[0] , sizeof(UINT8) * 6);
        close(bddr_file);
        APP_DEBUG0("geting file sucecss");
        return 0;
    }
	APP_DEBUG0("geting file fail");
    return -1;
}

int read_by_web(){
	int dsc_fd, index;
	struct msg_st data;
	long int msg_type = 0;
	int msgid = -1;
    msgid = msgget((key_t)1219, 0666 | IPC_CREAT);

    if (msgid == -1) {
        return -1;
    }
    if (msgrcv(msgid, (void*)&data, sizeof(int), msg_type, 0) == -1) {
        return -1;
    }

    index = data.index;
    APP_DEBUG1("geting index %d", index);
    if (msgctl(msgid, IPC_RMID, 0) == -1) {
        return -1;
    }

    dsc_fd = open("/tmp/bddr_fd", O_RDONLY);
    if (dsc_fd < 0)
        return -1;
    lseek(dsc_fd,  sizeof(UINT8) * 6 * index , SEEK_SET);
    read(dsc_fd, &bddr[0], sizeof(UINT8) * 6);
    APP_DEBUG1("geting web SUCCESS %02x:%02x:%02x:%02x:%02x:%02x",bddr[0],bddr[1],bddr[2],bddr[3],bddr[4],bddr[5]);
	return 0;
}

int send_to_web(int stat){
    int msg_id = -1;
    struct msg_st snd_data;
    msg_id = msgget((key_t)1220, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        APP_DEBUG0("get msg id fail!");
    }
    snd_data.index = stat;
    if (msgsnd(msg_id, (void*)&snd_data, sizeof(int), 0) == -1) {
        APP_DEBUG0("snd web error !!");
    }
    return 0;

}

int main(int argc, char **argv)
{
    int choice, volume;
	int s_bddr = -1;
	if (read_by_file() < 0 ) {
        APP_DEBUG0("get file fail  geting web");
		read_by_web();
    }
    UINT16 tone_sampling_freq = 44100;
    tBSA_AVK_RELAY_AUDIO relay_param;
    app_mgt_init();
    if (app_mgt_open(NULL, app_av_mgt_callback) < 0)
    {
        APP_DEBUG0("Unable to connect to server");
        return -1;
    }
    APP_DEBUG0("GET BDDR SUCCESS");
    app_xml_init();

    app_av_init(TRUE);

    for (choice = 0; choice < APP_AV_MAX_CONNECTIONS; choice++)
    {
        app_av_register();
    }
	s_bddr = open("/etc/bddr.conf", O_WRONLY|O_CREAT, 0755);
    if (app_av_open(bddr) < 0) {
	while (1) {
		if (read_by_web() == 0)
			if (app_av_open(bddr) >= 0)
				break;
	}
	}
    write(s_bddr, &bddr[0], sizeof(UINT8) * 6);
	close(s_bddr);
    send_to_web(0);
    while (1) {
        sleep(10);
    }
    app_av_end();
    app_mgt_close();
    return 0;
}

