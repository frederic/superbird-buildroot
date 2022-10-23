#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
//#include "cutils/misc.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include<stdlib.h>
#include <errno.h>
#include<unistd.h>

//#include "cutils/log.h"
//#include "cutils/memory.h"

//#include "cutils/properties.h"
//#include "private/android_filesystem_config.h"

//#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
//#include <sys/_system_properties.h>



//#define RDA_5996_91H

//#define WIFI_DRIVER_MODULE_ARG	""
//#define WIFI_FIRMWARE_LOADER	""
#define WIFI_DRIVER_FW_PATH "sta"

#define WIFI_DRIVER_MODULE_NAME "rdawfmac"
#define WIFI_DRIVER_MODULE_PATH "/vendor/modules/rdawfmac.ko"
#define WIFI_DRIVER_FW_PATH_PARAM	"/sys/module/rdawfmac/parameters/firmware_path"
#define WIFI_DRIVER_LOADER_DELAY    1000000
#define MAX_DRV_CMD_SIZE 1536
#define IFNAMELEN								(sizeof(IFNAME) - 1)
#define RDA_BT_IOCTL_MAGIC 'u'
//#define RDA_WIFI_POWER_ON_IOCTL                  _IO(RDA_BT_IOCTL_MAGIC ,0x10)
//#define RDA_WIFI_POWER_OFF_IOCTL                 _IO(RDA_BT_IOCTL_MAGIC ,0x11)
#define SET_IFACE_DELAY							(1000*1000)
#define SET_IFACE_POLLING_LOOP					1
#define TXRX_PARA								SIOCDEVPRIVATE+5

//#define RDAWLAN_DRV_NAME "/dev/rda_wlan"
#define max_args (10)

static const char IFACE_DIR[]           = "";
static const char DRIVER_MODULE_NAME[]  = "rdawfmac";
static const char DRIVER_MODULE_TAG[]   = WIFI_DRIVER_MODULE_NAME " ";
static const char DRIVER_MODULE_PATH[]  = WIFI_DRIVER_MODULE_PATH;
static const char DRIVER_MODULE_ARG[]   = "";
static const char FIRMWARE_LOADER[]     = "";
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";
//static const char MODULE_FILE[]         = "/proc/modules";
//static const char IFNAME[]              = "IFNAME=";
//static char primary_iface[PROPERTY_VALUE_MAX];

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))
#endif



typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

int wifi_send_cmd_to_net_interface(const char* if_name, int argC, char *argV[])
{
	int sock;
	struct ifreq ifr;
	int ret = 0;
	int i = 0;
	char buf[MAX_DRV_CMD_SIZE];
	struct android_wifi_priv_cmd priv_cmd;

	//printf("%s Enter.\n", __func__);

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("bad sock!\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, if_name);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		printf("%s Could not read interface %s flags: %s",__func__, if_name, strerror(errno));
		return -1;
	}

	if (!(ifr.ifr_flags & IFF_UP)) {
		printf("%s is not up!\n",if_name);
		return -1;
	}

	memset(&priv_cmd, 0, sizeof(priv_cmd));
	memset(buf, 0, sizeof(buf));

	for(i=2; i<argC; i++){
		strcat(buf, argV[i]);
		strcat(buf, " ");
	}

	priv_cmd.buf = buf;
	priv_cmd.used_len = strlen(buf);
	priv_cmd.total_len = sizeof(buf);
	ifr.ifr_data = (void*)&priv_cmd;

	if (strcasecmp(argV[2], "RX_RESULT")==0 ||
		strcasecmp(argV[2], "GET_MACADDR")==0 ||
		strcasecmp(argV[2], "GET_VENID")==0 ||
		strcasecmp(argV[2], "READ_F_CAL_VAL")==0 ||
		strcasecmp(argV[2], "READ_TXP")==0 ||
		strcasecmp(argV[2], "GET_EFUSE")==0 ||
		strcasecmp(argV[2], "GET_PARAM")==0 ||
		strcasecmp(argV[2], "GET_REG_CHAN")==0) {
		//printf("%s get rx results.\n", __func__);
		if ((ret = ioctl(sock, TXRX_PARA, &ifr)) < 0) {
				printf("%s: error ioctl[TX_PARA] ret= %d\n", __func__, ret);
				return ret;
		}
		memcpy(&priv_cmd, ifr.ifr_data, sizeof(struct android_wifi_priv_cmd));

		printf("%s\n", priv_cmd.buf);

	} else {
		printf(" send cmd: %s.\n", buf);
		if ((ret = ioctl(sock, TXRX_PARA, &ifr)) < 0) {
				printf("%s: error ioctl[TX_PARA] ret= %d\n", __func__, ret);
				return ret;
		}
		printf("Done\n");
	}
	return ret;
}

int wifi_change_fw_path(const char *fwpath)
{
    int len;
    int fd;
    int ret = 0;

    printf("%s Enter. fwpath:%s\n", __func__, fwpath);

    if (!fwpath)
        return ret;

    fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_FW_PATH_PARAM, O_WRONLY));
    if (fd < 0) {
        printf("Failed to open wlan fw path param (%s)", strerror(errno));
        return -1;
    }
    len = strlen(fwpath) + 1;
    if (TEMP_FAILURE_RETRY(write(fd, fwpath, len)) != len) {
        printf("Failed to write wlan fw path param (%s)", strerror(errno));
        ret = -1;
    }
    close(fd);
    return ret;
}
#if 0

int linux_set_iface_flags(int sock, const char *ifname, int dev_up)
{
	struct ifreq ifr;
	int ret = 0;

	if (sock < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
		printf("%s Could not read interface %s flags: %s",__func__,
			   ifname, strerror(errno));
		return ret;
	}

	if (dev_up) {
		if (ifr.ifr_flags & IFF_UP)
			return 0;
		ifr.ifr_flags |= IFF_UP;
	} else {
		if (!(ifr.ifr_flags & IFF_UP))
			return 0;
		ifr.ifr_flags &= ~IFF_UP;
	}

	if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0) {
		printf("%s Could not set interface %s flags (%s): %s", __func__,
			   ifname, dev_up ? "UP" : "DOWN", strerror(errno));
		return ret;
	}

	return 0;
}

#endif

int main(int argC, char *argV[])
{

	char* ins = "insmod";
	char* rm = "rmmod";
	char* ko = "rdawfmac.ko";

	if(argC >= 3)
		wifi_send_cmd_to_net_interface(argV[1], argC, argV);
	else
		printf("Bad parameter! %d\n",argC);

	return 0;
}
