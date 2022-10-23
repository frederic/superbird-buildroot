#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "shared/ctl-socket.h"
#include "hfp_ctl.h"
#include "sco_handler.h"


#define INFO(fmt, args...) \
	printf("[HFP_CTL][%s] " fmt, __func__, ##args)


static int hfp_ctl_sk = 0;
static unsigned int sVol = 10;
static unsigned int mVol = 10;
static int hfp_connected = 0;
static pthread_t phfp_ctl_id;
static void *hfp_ctl_monitor(void *arg);

#define CTL_SOCKET_PATH "/etc/bluetooth/hfp_ctl_sk"
#define HFP_EVENT_CONNECTION 1
#define HFP_EVENT_CALL       2
#define HFP_EVENT_CALLSETUP  3
#define HFP_EVENT_VGS        4
#define HFP_EVENT_VGM        5

#define HFP_IND_DEVICE_DISCONNECTED 0
#define HFP_IND_DEVICE_CONNECTED    1

#define HFP_IND_CALL_NONE           0
/* at least one call is in progress */
#define HFP_IND_CALL_ACTIVE         1
/* currently not in call set up */
#define HFP_IND_CALLSETUP_NONE      0
/* an incoming call process ongoing */
#define HFP_IND_CALLSETUP_IN        1
/* an outgoing call set up is ongoing */
#define HFP_IND_CALLSETUP_OUT       2
/* remote party being alerted in an outgoing call */
#define HFP_IND_CALLSETUP_OUT_ALERT 3


int hfp_ctl_init(void)
{
	if (pthread_create(&phfp_ctl_id, NULL, hfp_ctl_monitor, NULL)) {
		perror("hfp_ctl_client");
		teardown_socket_client(hfp_ctl_sk);
		return -1;
	}
	pthread_setname_np(phfp_ctl_id, "hfp_ctl_monitor");

	INFO("Server is ready for connection...\n");
	return 0;
}

void hfp_ctl_delinit(void)
{
	/*set thread to cancel state, and free resouce
	  thread will exit once teardown_socket_server()
	  wakes thread up from recv()
	 */
	pthread_cancel(phfp_ctl_id);
	pthread_detach(phfp_ctl_id);

	hfp_connected = 0;
	teardown_socket_client(hfp_ctl_sk);
}

int answer_call(void)
{
	unsigned int byte, ret = 1;
	char msg[] = "A";
	INFO("\n");
	byte = send(hfp_ctl_sk, msg, strlen(msg), 0);

	if (byte == strlen(msg))
		ret = 0;

	return ret;
}

int reject_call(void)
{
	unsigned int byte, ret = 1;
	char msg[] = "+CHUP";
	INFO("\n");
	byte = send(hfp_ctl_sk, msg, strlen(msg), 0);
	if (byte == strlen(msg))
		ret = 0;

	return ret;
}

int VGS_up(void)
{
	sVol += 1;
	return set_VGS(sVol);
}

int VGS_down(void)
{
	sVol -= 1;
	return set_VGS(sVol);

}

int VGM_up(void)
{
	mVol += 1;
	return set_VGM(mVol);
}

int VGM_down(void)
{
	mVol -= 1;
	return set_VGM(mVol);

}

int set_VGS(int value)
{
	unsigned int byte, ret = 1;
	char msg[10] = {0};

	//value range from 0 ~ 15
	value = value < 0 ? 0 : value;
	value = value > 15 ? 15 : value;

	sprintf(msg, "+VGS=%d", value);
	INFO("\n");
	byte = send(hfp_ctl_sk, msg, strlen(msg), 0);
	if (byte == strlen(msg))
		ret = 0;

	return ret;
}

int set_VGM(int value)
{
	unsigned int byte, ret = 1;
	char msg[10] = {0};

	//value range from 0 ~ 15
	value = value < 0 ? 0 : value;
	value = value > 15 ? 15 : value;

	sprintf(msg, "+VGM=%d", value);
	INFO("\n");
	byte = send(hfp_ctl_sk, msg, strlen(msg), 0);
	if (byte == strlen(msg))
		ret = 0;

	return ret;
}

static void *hfp_ctl_monitor(void *arg)
{
	int byte, value = -1, event =-1, i;
	char msg[64];

init:
	hfp_ctl_sk = setup_socket_client(CTL_SOCKET_PATH);
	if (hfp_ctl_sk < 0) {
		sleep(3);
		goto init;
	}


	INFO("recieving....\n");
	while (1) {
		memset(msg, 0, sizeof(msg));
		byte = recv(hfp_ctl_sk, msg, sizeof(msg), 0);
		if (byte < 0) {
			perror("recv");
			continue;
		}
		if (byte == 0) {
			INFO("server off line\n");
			sleep(3);
			goto init;
		}

		if (byte == 2) {
			event = msg[0];
			value = msg[1];
			INFO("event = %d, value = %d\n", event, value);
		} else {
#if 0
			INFO("invalid msg: %s\n", msg);
			for (i = 0 ; i < byte; i++)
				INFO("msg %d = %d\n", i, msg[i]);
#endif
			continue;
		}

		switch (event) {
			case HFP_EVENT_CONNECTION:
				switch (value) {
					case HFP_IND_DEVICE_DISCONNECTED:
						INFO("HFP disconnected!!!\n");
						set_sco_enable(0);
						hfp_connected = 0;
						break;
					case HFP_IND_DEVICE_CONNECTED:
						INFO("HFP connected!!!\n");
						hfp_connected = 1;
						break;
				}
				break;

			case HFP_EVENT_CALL:
				switch (value) {
					case HFP_IND_CALL_NONE:
						INFO("Call stopped!!!\n");
						set_sco_enable(0);
						break;
					case HFP_IND_CALL_ACTIVE :
						INFO("Call active!!!\n");
						set_sco_enable(1);
						break;
				}
				break;

			case HFP_EVENT_CALLSETUP:
				switch (value) {
					case HFP_IND_CALLSETUP_NONE:
						INFO("Callsetup stopped!!!\n");
						break;
					case HFP_IND_CALLSETUP_IN :
						INFO("An incomming Callsetup!!!\n");
						break;
					case HFP_IND_CALLSETUP_OUT :
						INFO("An outgoing Callsetup!!!\n");
						break;
					case HFP_IND_CALLSETUP_OUT_ALERT :
						INFO("Remote device being altered!!!\n");
						break;
				}
				break;

			case HFP_EVENT_VGS:
				INFO("VGS EVENT!!!\n");
				sVol = value;
				break;
			case HFP_EVENT_VGM:
				INFO("VGM EVENT!!!\n");
				mVol = value;
				break;
		}
	}
	INFO("exit\n");
	return NULL;
}

static void signal_handler(int sig)
{
	INFO("INT/TERM signal detected\n");
	hfp_ctl_delinit();
	signal(sig, SIG_DFL);
	exit(0);
}


int main(int argc, char **argv)
{

	if (hfp_ctl_init())
		return -1;

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	while (!hfp_connected)
		sleep(1);

#if 0
	answer_call();
	VGS_up();
	VGM_up();
	VGS_down();
	VGM_down();
	sleep(5);
	reject_call();
#endif
	while (1)
		sleep(1);

}
