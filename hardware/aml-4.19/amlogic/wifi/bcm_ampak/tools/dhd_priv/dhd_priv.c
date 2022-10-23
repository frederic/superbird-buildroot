
#include <unistd.h>
#include <stdio.h>
#include<stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/socket.h>
#include <netpacket/packet.h>
#include <net/if.h>

#define IFNAME "wlan0"

/* message levels */
#define LOG_MSG_LEVEL	0x0
#define LOG_ERROR_LEVEL	0x1
#define LOG_INFO_LEVEL	0x2
int log_level = LOG_MSG_LEVEL;

#define TAG "dhd_priv(2.3)"
#if defined(ANDROID)
#include <android/log.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "cutils/misc.h"
#include "cutils/log.h"
#endif

#if defined(ANDROID)
static int dhd_to_android_level(int level)
{
	if (level == LOG_ERROR_LEVEL)
		return ANDROID_LOG_ERROR;
	if (level == LOG_INFO_LEVEL)
		return ANDROID_LOG_INFO;
	return ANDROID_LOG_DEBUG;
}
#endif

void dhd_printf(int level, const char *fmt, ...)
{
	va_list ap;
	char buf[512];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);

#if defined(ANDROID)
	if ((level == LOG_ERROR_LEVEL) || (log_level > LOG_ERROR_LEVEL && level & log_level))
		__android_log_vprint(dhd_to_android_level(level), TAG, buf, ap);
#endif
	if (level == LOG_MSG_LEVEL)
		printf("%s", buf);
	else if (level <= LOG_ERROR_LEVEL || level & log_level)
		printf("%s: %s", TAG, buf);

	va_end(ap);
}

typedef struct dhd_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} dhd_priv_cmd;

int
main(int argc, char **argv)
{
	struct ifreq ifr;
	dhd_priv_cmd priv_cmd;
	int ret = 0;
	int ioctl_sock; /* socket for ioctl() use */
	char buf[512]="";
	char log_buf[512], *pos;
	char *end = log_buf + sizeof(log_buf);
	char ifname[IFNAMSIZ+1];
	char **argvp = argv;

	argc++;
	if (!argv[1]) {
		dhd_printf(LOG_ERROR_LEVEL, "Please input right cmd\n");
		return 0;
	}

	while (*++argvp) {
		if (!strcmp(*argvp, "-d")) {
			++argvp;
			log_level = atoi(*argvp);
			continue;
		}
	}

	strcpy(ifname, IFNAME);
	memset(log_buf, 0, sizeof(log_buf));
	pos = log_buf;
	while (*++argv) {
		if (!strcmp(*argv, "-i")) {
			++argv;
			strcpy(ifname, *argv);
			continue;
		}
		if (!strcmp(*argv, "-d")) {
			++argv;
			continue;
		}
		pos += snprintf(pos, end - pos, "%s ", *argv);
		strcat(buf, *argv);
		if (*(argv+1))
			strcat(buf, " ");
	}
	dhd_printf(LOG_INFO_LEVEL, "%s\n", log_buf);

	ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (ioctl_sock < 0) {
		dhd_printf(LOG_ERROR_LEVEL, "socket(PF_INET,SOCK_DGRAM)\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&priv_cmd, 0, sizeof(priv_cmd));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	priv_cmd.buf = buf;
	priv_cmd.used_len = 500;
	priv_cmd.total_len = 500;
	ifr.ifr_data = (char *)&priv_cmd;

	if ((ret = ioctl(ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
		dhd_printf(LOG_ERROR_LEVEL, "failed to send cmd at %s with error %d\n",
			ifname, ret);
	} else {
		if (log_level & LOG_INFO_LEVEL)
			dhd_printf(LOG_INFO_LEVEL, "%s (len = %zu, ret = %d)\n", buf, strlen(buf), ret);
		else
			dhd_printf(LOG_MSG_LEVEL, "%s\n", buf);
	}

	close(ioctl_sock);
	return ret;
}

