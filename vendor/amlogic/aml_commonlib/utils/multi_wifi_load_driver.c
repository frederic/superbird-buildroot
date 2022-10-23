#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libusb-1.0/libusb.h"

#define USB_POWER_UP        _IO('m', 1)
#define USB_POWER_DOWN      _IO('m', 2)
#define SDIO_POWER_UP       _IO('m', 3)
#define SDIO_POWER_DOWN     _IO('m', 4)
#define SDIO_GET_DEV_TYPE   _IO('m', 5)

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))

#define XSTR(s)     STR(s)
#define STR(s)      #s

#define DEFAULT_KO_PATH     "/vendor/lib/modules"

#ifdef BROADCOM_MODULES_PATH
#define BROADCOM_KO_PATH     XSTR(BROADCOM_MODULES_PATH)
#else
#define BROADCOM_KO_PATH     DEFAULT_KO_PATH
#endif

#ifdef CYPRESS_MODULES_PATH
#define CYPRESS_KO_PATH     XSTR(CYPRESS_MODULES_PATH)
#else
#define CYPRESS_KO_PATH     DEFAULT_KO_PATH
#endif

#ifdef REALTEK_MODULES_PATH
#define REALTEK_KO_PATH     XSTR(REALTEK_MODULES_PATH)
#else
#define REALTEK_KO_PATH     DEFAULT_KO_PATH
#endif

#define DEFAULT_CONFIG_PATH     "/vendor/etc/wifi"

#ifdef BROADCOM_CONFIG_PATH
#define BROADCOM_FIRMWARE_PATH     XSTR(BROADCOM_CONFIG_PATH)
#else
#define BROADCOM_FIRMWARE_PATH     DEFAULT_CONFIG_PATH
#endif

#ifdef CYPRESS_CONFIG_PATH
#define CYPRESS_FIRMWARE_PATH     XSTR(CYPRESS_CONFIG_PATH)
#else
#define CYPRESS_FIRMWARE_PATH     DEFAULT_CONFIG_PATH
#endif

#ifdef MRVL_MODULES_PATH
#define MRVL_KO_PATH     XSTR(MRVL_MODULES_PATH)
#else
#define MRVL_KO_PATH     DEFAULT_CONFIG_PATH
#endif

#ifdef QCA_MODULES_PATH
#define QCA_KO_PATH     XSTR(QCA_MODULES_PATH)
#else
#define QCA_KO_PATH     DEFAULT_CONFIG_PATH
#endif

#define MODULE_ARG_FIRMWARE     0
#define MODULE_ARG_IFNAME       1
#define MODULE_ARG_STACFG       2
#define MODULE_ARG_OTHER        3

#if defined(__NR_finit_module)
# define finit_module(fd, uargs, flags) syscall(__NR_finit_module, fd, uargs, flags)
#endif

typedef struct config_arg {
	const int arg_type;
	const char *firmware_path;
	const char *firmware_ap_path;
	const char *nvram_path;
	const char *ifname;
	const char *if2name;
	const char *stacfgpath;
	const char *arg;
} module_arg;

typedef struct load_info {
	const char *chip_id;
	const char *wifi_module_name;
	const char *wifi_module_filename;
	const char *wifi_module_path;

	const module_arg wifi_module_arg;
	const char *wifi_name;
	const int wifi_pid;

	const char *wifi_module_name2;
	const char *wifi_module_filename2;
	const char *wifi_module_arg2;
} dongle_info;

#define TYPE_AP         0
#define TYPE_STATION    1

static int type = TYPE_AP;

static const dongle_info dongle_registerd[] = {
	{
		"a962",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "40181/fw_bcm40181a2.bin",
			.firmware_ap_path  = "40181/fw_bcm40181a2_apsta.bin",
			.nvram_path     = "40181/nvram.txt",
		},
		"bcm6210",
		0x0
	},
	{
		"4359",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "AP6398/fw_bcm4359c0_ag.bin",
			.firmware_ap_path  = "AP6398/fw_bcm4359c0_ag_apsta.bin",
			.nvram_path     = "AP6398/nvram.txt",
		},
		"ap6398",
		0x0
	},
	{
		"a804",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "AP6201/fw_bcm43013b0.bin",
			.firmware_ap_path  = "AP6201/fw_bcm43013b0_apsta.bin",
			.nvram_path     = "AP6201/nvram_ap6201.txt",
		},
		"ap6201",
		0x0
	},
	{
		"4354",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "4354/fw_bcm4354a1_ag.bin",
			.firmware_ap_path  = "4354/fw_bcm4354a1_ag_apsta.bin",
			.nvram_path     = "4354/nvram.txt",
		},
		"bcm4354",
		0x0
	},
	{
		"4335",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "6335/fw_bcm4339a0_ag.bin",
			.firmware_ap_path  = "6335/fw_bcm4339a0_ag_apsta.bin",
			.nvram_path     = "6335/nvram.txt",
		},
		"bcm6335",
		0x0
	},
	{
		"a94d",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "6234/fw_bcm43341b0_ag.bin",
			.firmware_ap_path  = "6234/fw_bcm43341b0_ag_apsta.bin",
			.nvram_path     = "6234/nvram.txt",
		},
		"bcm6234",
		0x0
	},
#ifdef CYPRESS_WIFI_MODULE
{
	"a9bf",
	"cywdhd",
	"cywdhd.ko",
	CYPRESS_KO_PATH,
	.wifi_module_arg = {
		.arg_type		= MODULE_ARG_FIRMWARE,
		.firmware_path	= "cyw43455/cyw43455-7.45.100.13.bin",
		.firmware_ap_path  = "cyw43455/cyw43455-7.45.100.13.bin",
		.nvram_path 	= "cyw43455/bcm943457wlsagb_4L_170607_customer.txt",
	},
	"cyw43455",
	0x0
},
{
	"a9a6",
	"cywdhd",
	"cywdhd.ko",
	CYPRESS_KO_PATH,
	.wifi_module_arg = {
		.arg_type		= MODULE_ARG_FIRMWARE,
		.firmware_path	= "cyw43438/cyw43438-7.46.58.15.bin",
		.firmware_ap_path  = "cyw43438/cyw43438-7.46.58.15.bin",
		.nvram_path 	= "cyw43438/NB197SM.nvram_20180419_AZ.txt",
	},
	"cyw43438",
	0x0
},
#else
	{
		"a9bf",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "6255/fw_bcm43455c0_ag.bin",
			.firmware_ap_path  = "6255/fw_bcm43455c0_ag_apsta.bin",
			.nvram_path     = "6255/nvram.txt",
		},
		"bcm6255",
		0x0
	},
	{
		"a9a6",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "6212/fw_bcm43438a0.bin",
			.firmware_ap_path  = "6212/fw_bcm43438a0_apsta.bin",
			.nvram_path     = "6212/nvram.txt",
		},
		"bcm6212",
		0x0
	},
#endif
	{
		"4356",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "4356/fw_bcm4356a2_ag.bin",
			.firmware_ap_path  = "4356/fw_bcm4356a2_ag_apsta.bin",
			.nvram_path     = "4356/nvram.txt",
		},
		"bcm6356",
		0x0
	},
	{
		"aa31",
		"dhd",
		"dhd.ko",
		BROADCOM_KO_PATH,
		.wifi_module_arg = {
			.arg_type       = MODULE_ARG_FIRMWARE,
			.firmware_path  = "4358/fw_bcm4358_ag.bin",
			.firmware_ap_path  = "4358/fw_bcm4358_ag_apsta.bin",
			.nvram_path     = "4358/nvram.txt",
		},
		"bcm4358",
		0x0
	},
	{
		"8179",
		"8189es",
		"8189es.ko",
		REALTEK_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_IFNAME,
			.ifname     = "wlan0",
			.if2name    = "p2p0",
		},
		"rtl8189es",
		0x0
	},
	{
		"d723",
		"8723ds",
		"8723ds.ko",
		REALTEK_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_IFNAME,
			.ifname     = "wlan0",
			.if2name    = "p2p0",
		},
		"rtl8723ds",
		0x0
	},
	{
		"b822",
		"8822bs",
		"8822bs.ko",
		REALTEK_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_IFNAME,
			.ifname     = "wlan0",
			.if2name    = "p2p0",
		},
		"rtl8822bs",
		0x0
	},
	{
		"invalid",
		"8822bu",
		"8822bu.ko",
		REALTEK_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_IFNAME,
			.ifname     = "wlan0",
			.if2name    = "p2p0",
		},
		"rtl8822bu",
		0xb82c
	},
	{
		"3030",
		"ssv6051",
		"ssv6051.ko",
		DEFAULT_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_STACFG,
			.stacfgpath = "/system/vendor/etc/wifi/ssv6051/ssv6051-wifi.cfg",
		},
		"ssv6051",
		0x0
	},
	{
		"9145",
		"mlan",
		"mlan8977.ko",
		MRVL_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = 0xFF
		},
		"sd8977",
		0x0,
		"sd8xxx",
		"sd8xxx_8977.ko",
		"cal_data_cfg=none"
	},
	{
		"9149",
		"mlan",
		"mlan8987.ko",
		MRVL_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = 0xFF
		},
		"sd8987",
		0x0,
		"sd8xxx",
		"sd8xxx_8987.ko",
		"cal_data_cfg=none"
	},
	{
		"0701",
		"wlan",
		"wlan.ko",
		QCA_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_OTHER,
			.arg        = "country_code=CN",
		},
		"qca9377",
		0x0,
	},
	{
		"050a",
		"wlan",
		"wlan.ko",
		QCA_KO_PATH,
		.wifi_module_arg = {
			.arg_type   = MODULE_ARG_OTHER,
			.arg        = "country_code=CN",
		},
		"qca6174",
		0x0,
	}
};

static void get_module_arg(const module_arg *arg, char *str, int type)
{
	switch (arg->arg_type) {
	case MODULE_ARG_FIRMWARE: {
#ifdef CYPRESS_WIFI_MODULE
		if (type == TYPE_AP)
			sprintf(str, "firmware_path=%s/%s nvram_path=%s/%s", CYPRESS_FIRMWARE_PATH, arg->firmware_ap_path, CYPRESS_FIRMWARE_PATH, arg->nvram_path);
		else
			sprintf(str, "firmware_path=%s/%s nvram_path=%s/%s", CYPRESS_FIRMWARE_PATH, arg->firmware_path, CYPRESS_FIRMWARE_PATH, arg->nvram_path);
#else
		if (type == TYPE_AP)
			sprintf(str, "firmware_path=%s/%s nvram_path=%s/%s", BROADCOM_FIRMWARE_PATH, arg->firmware_ap_path, BROADCOM_FIRMWARE_PATH, arg->nvram_path);
		else
			sprintf(str, "firmware_path=%s/%s nvram_path=%s/%s", BROADCOM_FIRMWARE_PATH, arg->firmware_path, BROADCOM_FIRMWARE_PATH, arg->nvram_path);
#endif
		break;
	}
	case MODULE_ARG_IFNAME: {
		sprintf(str, "ifname=%s if2name=%s", arg->ifname, arg->if2name);
		break;
	}
	case MODULE_ARG_STACFG: {
		sprintf(str, "stacfgpath=%s", arg->stacfgpath);
		break;
	}
	case MODULE_ARG_OTHER: {
		sprintf(str, "%s", arg->arg);
		break;
	}
	default:
		break;
	}
}

static char file_name[100] = {'\0'};
static int load_dongle_index = -1;

static int insmod(const char *filename, const char *options)
{
	int rc = 0;
#ifdef __NR_finit_module
	int fd = open(filename, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);

	fprintf(stderr, "[%s:%d]filename(%s) options(%s)\n", __func__, __LINE__, filename, options);
	if (fd == -1) {
		fprintf(stderr, "insmod: open(%s) failed\n", filename);
		return -1;
	}

	rc = finit_module(fd, options, 0);
	close(fd);
	if (rc == 0)
		return rc;
#endif
	char wifi_insmod[200];

	rc = sprintf(wifi_insmod, "insmod ");
	rc += sprintf(wifi_insmod + rc, filename);
	rc += sprintf(wifi_insmod + rc, " ");
	rc += sprintf(wifi_insmod + rc, options);
	fprintf(stderr, "[%s:%d] wifi_insmod command:%s\n", __func__, __LINE__, wifi_insmod);

	rc = system(wifi_insmod);

	return rc;
}

static int rmmod(const char *modname)
{
	int ret = -1;
	int maxtry = 10;

	fprintf(stderr, "[%s:%d]module(%s)\n", __func__, __LINE__, modname);
	while (maxtry-- > 0) {
		ret = syscall(__NR_delete_module, modname, O_NONBLOCK | O_EXCL);
		if (ret < 0 && errno == EAGAIN)
			usleep(500000);
		else
			break;
	}

	if (ret != 0)
		fprintf(stderr, "rmmod: unload driver module (%s) failed\n", modname);

	return ret;
}

static void set_wifi_power(int power)
{
	int fd;

	fd = open("/dev/wifi_power", O_RDWR);
	if (fd != -1) {
		switch (power) {
		case USB_POWER_UP: {
			if (ioctl(fd, USB_POWER_UP) < 0) {
				fprintf(stderr, "Set usb Wi-Fi power up error!\n");
			}
			break;
		}
		case USB_POWER_DOWN: {
			if (ioctl(fd, USB_POWER_DOWN) < 0) {
				fprintf(stderr, "Set usb Wi-Fi power down error!\n");
			}
			break;
		}
		case SDIO_POWER_UP: {
			if (ioctl(fd, SDIO_POWER_UP) < 0) {
				fprintf(stderr, "Set SDIO Wi-Fi power up error!\n");
			}
			break;
		}
		case SDIO_POWER_DOWN: {
			if (ioctl(fd, SDIO_POWER_DOWN) < 0) {
				fprintf(stderr, "Set SDIO Wi-Fi power down error!\n");
			}
			break;
		}
		default:
			break;
		}
	} else {
		fprintf(stderr, "device /dev/wifi_power open failed\n");
	}
	close(fd);

	return;
}

static int get_wifi_dev_type(char *dev_type)
{
	int fd;

	fd = open("/dev/wifi_power", O_RDWR);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, SDIO_GET_DEV_TYPE, dev_type) < 0) {
		close(fd);

		return -1;
	}
	close(fd);

	return 0;
}

static int print_devs(libusb_device **devs, int type)
{
	libusb_device *dev;
	int i = 0, j;
	char module_path[128] = {'\0'};
	char module_arg[256] = {'\0'};

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);

		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor\n");

			return -1;
		}

		for (j = 0; j < (int)(ARRAY_SIZE(dongle_registerd)); j++) {
			if (dongle_registerd[j].wifi_pid == desc.idProduct) {
				load_dongle_index = j;
				fprintf(stderr, "found the match wifi is: %s", dongle_registerd[j].wifi_name);
				memset(module_path, 0, sizeof(module_path));
				memset(module_arg, 0, sizeof(module_arg));
				sprintf(module_path, "%s/%s", dongle_registerd[i].wifi_module_path, dongle_registerd[i].wifi_module_filename);
				get_module_arg(&dongle_registerd[i].wifi_module_arg, module_arg, type);
				fprintf(stderr, "[%s:%d]module_path(%s)\n", __func__, __LINE__, module_path);
				fprintf(stderr, "[%s:%d]module_arg(%s)\n", __func__, __LINE__, module_arg);
				insmod(module_path, module_arg);

				return 0;
			}
		}
	}

	return -1;
}

static int usb_wifi_load_driver(int type)
{
	libusb_device **devs;
	int r;
	ssize_t cnt;

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int)cnt;

	r = print_devs(devs, type);
	if (r < 0)
		return r;

	libusb_free_device_list(devs, 1);
	libusb_exit(NULL);

	return 0;
}

static int sdio_wifi_load_driver(int type)
{
	int i;
	char sdio_buf[128];
	char module_path[128] = {'\0'};
	char module_arg[256] = {'\0'};
	FILE *fp = fopen(file_name, "r");

	if (!fp) {
		fprintf(stderr, "open sdio wifi file failed\n");

		return -1;
	}

	memset(sdio_buf, 0, sizeof(sdio_buf));
	if (fread(sdio_buf, 1, 128, fp) < 1) {
		fclose(fp);

		return -1;
	}

	fclose(fp);
	for (i = 0; i < (int)(ARRAY_SIZE(dongle_registerd)); i++) {
		if (strstr(sdio_buf, dongle_registerd[i].chip_id)) {
			load_dongle_index = i;
			fprintf(stderr, "found the match wifi is: %s\n", dongle_registerd[i].wifi_name);
			memset(module_path, 0, sizeof(module_path));
			memset(module_arg, 0, sizeof(module_arg));
			sprintf(module_path, "%s/%s", dongle_registerd[i].wifi_module_path, dongle_registerd[i].wifi_module_filename);
			get_module_arg(&dongle_registerd[i].wifi_module_arg, module_arg, type);
			fprintf(stderr, "[%s:%d]module_path(%s)\n", __func__, __LINE__, module_path);
			fprintf(stderr, "[%s:%d]module_arg(%s)\n", __func__, __LINE__, module_arg);
#ifdef RTK_WIFI_MODULE
			char wifimac[32];
			char *wifimac_fileName = "/sys/module/kernel/parameters/wifimac";
			FILE *wifimac_fp = NULL;
			memset(wifimac, 0 ,sizeof(wifimac));
			wifimac_fp = fopen(wifimac_fileName, "r");
			if (wifimac_fp) {
				if (fread(wifimac, 1, sizeof(wifimac) - 1, wifimac_fp) > 0) {
					if (strlen(wifimac) > 0 && wifimac[strlen(wifimac) - 1] == '\n') {
						wifimac[strlen(wifimac) - 1] = '\0';
					}
					if (strlen(wifimac) > 0 && strstr(wifimac, "(null)") == NULL) {
						snprintf(module_arg + strlen(module_arg),
							sizeof(module_arg) - strlen(module_arg) - 1,
							" rtw_initmac=%s", wifimac);
					}
				}
				fclose(wifimac_fp);
			}
#endif
			insmod(module_path, module_arg);

			if (dongle_registerd[i].wifi_module_name2) {
				memset(module_path, 0, sizeof(module_path));
				memset(module_arg, 0, sizeof(module_arg));
				memcpy(module_arg, dongle_registerd[i].wifi_module_arg2, strlen(dongle_registerd[i].wifi_module_arg2));
				sprintf(module_path, "%s/%s", dongle_registerd[i].wifi_module_path, dongle_registerd[i].wifi_module_filename2);
				fprintf(stderr, "[%s:%d]module_path2(%s)\n", __func__, __LINE__, module_path);
				fprintf(stderr, "[%s:%d]module_arg2(%s)\n", __func__, __LINE__, module_arg);
				insmod(module_path, module_arg);
			}

			return 0;
		}
	}

	return -1;
}

static int multi_wifi_load_driver(int type)
{
	int wait_time = 0, ret;
	char dev_type[10] = {'\0'};

	get_wifi_dev_type(dev_type);
	sprintf(file_name, "/sys/bus/mmc/devices/%s:0001/%s:0001:1/device", dev_type, dev_type);
	if (!sdio_wifi_load_driver(type)) {
		return 0;
	}
	fprintf(stderr, "wait usb ok\n");
	do {
		ret = usb_wifi_load_driver(type);
		if (!ret)
			return 0;
		else if (ret > 0)
			break;
		else {
			wait_time++;
			usleep(50000);
		}
	} while (wait_time < 300);

	return -1;
}

static int wifi_on(int type)
{
	set_wifi_power(SDIO_POWER_UP);
	if (multi_wifi_load_driver(type) < 0) {
		set_wifi_power(SDIO_POWER_DOWN);

		return -1;
	}

	return 0;
}

static int wifi_off(void)
{
	int i;
	char dev_type[10] = {'\0'};
	char sdio_buf[128];

	get_wifi_dev_type(dev_type);
	sprintf(file_name, "/sys/bus/mmc/devices/%s:0001/%s:0001:1/device", dev_type, dev_type);
	FILE *fp = fopen(file_name, "r");

	if (!fp) {
		fprintf(stderr, "open sdio wifi file failed\n");

		return -1;
	}

	memset(sdio_buf, 0, sizeof(sdio_buf));
	if (fread(sdio_buf, 1, 128, fp) < 1) {
		fclose(fp);

		return -1;
	}

	fclose(fp);
	for (i = 0; i < (int)(ARRAY_SIZE(dongle_registerd)); i++) {
		if (strstr(sdio_buf, dongle_registerd[i].chip_id)) {
			load_dongle_index = i;
			break;
		}
	}

	usleep(200000); /* allow to finish interface down */
	rmmod(dongle_registerd[load_dongle_index].wifi_module_name);
	if (dongle_registerd[load_dongle_index].wifi_module_name2)
		rmmod(dongle_registerd[load_dongle_index].wifi_module_name2);
	usleep(500000);
	set_wifi_power(SDIO_POWER_DOWN);

	return 0;
}

int main(int argc, char *argv[])
{
	long value = 0;
	int fd;

	fd = open("/dev/console", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd != -1) {
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(fd);
	}

	if (argc != 3) {
		fprintf(stderr, "wrong number of arguments\n");
		return -1;
	}

	if (!strncmp(argv[1], "ap", 2))
		type = TYPE_AP;
	else if (!strncmp(argv[1], "sta", 3))
		type = TYPE_STATION;

	value = strtol(argv[2], NULL, 10);
	if (value == 1)
		wifi_on(type);
	else
		wifi_off();

	return 0;
}
