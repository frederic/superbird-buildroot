#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"

#include "app_socket.h"

char socket_path[] = "/etc/bsa/config/aml_musicBox_socket";

#define BLE_MAX_DATA_LEN 18

char rev_buf[BLE_MAX_DATA_LEN] = {0};

/*start+magic+version+cmd+ssid+psk+end*/
char start[1] = {0x01};
char magic[10] = "amlogicble";
char cmd[9] = "wifisetup";
char ssid[32] = {0};
char psk[32] = {0};
char end[1] = {0x04};
#define FRAME_BUF_MAX (1+10+9+32+32+1)
char version[8] = "20171211";
char frame_buf[FRAME_BUF_MAX] = {0};
char ssid_psk_file[] = "/var/www/cgi-bin/wifi/select.txt";
/*0: wifi set success, 1: wifi set fail*/
char wifi_status_file[] = "/etc/bsa/config/wifi_status";
char wifi_status;
char wifi_success = '1';
int main(int argc, char **argv)
{
	int rev_len = 0;
	int cnt = 10, ret = 0;
	int check_0 = 0;
	FILE *fd;
	int sk = 0;
	char id[] = "aml_ble_setup_wifi";
	while (cnt--) {
		sk = setup_socket_client(socket_path);
		if (sk > 0)
			break;
		APP_INFO1(" cnt :%d", cnt);
		sleep(1);

	}
	socket_send(sk, id, strlen(id));
	cnt = 0;

	for (;;) {
		rev_len = socket_recieve(sk, rev_buf, BLE_MAX_DATA_LEN);
		if (rev_len > 0) {
			APP_INFO1("rev_buf:%s,rev_len:%d", rev_buf, rev_len);
			memcpy(frame_buf + cnt, rev_buf, rev_len);
			memset(rev_buf, 0, BLE_MAX_DATA_LEN);
			cnt += rev_len;
			APP_INFO1("frame_buf cnt :%d", cnt);
			rev_len = 0;
			check_0 = 0;
			if (cnt >= FRAME_BUF_MAX) {
				cnt = 0;
				APP_INFO0("receive data length is FRAME_BUF_MAX");
				if ((frame_buf[0] == 0x01)
						&& (frame_buf[FRAME_BUF_MAX - 1] == 0X04)) {
					APP_INFO0("frame start and end is right");
					if (!strncmp(magic, frame_buf + 1, 10)) {
						APP_INFO1("magic : %s", magic);
						APP_INFO1("version : %s", version);
						check_0 = 1;
					} else {
						APP_INFO0("magic of frame error!!!");
						memset(frame_buf, 0, FRAME_BUF_MAX);
						cnt = 0;
						check_0 = 0;
					}
				} else {
					APP_INFO0("start or end of frame error!!!");
					memset(frame_buf, 0, FRAME_BUF_MAX);
					cnt = 0;
					check_0 = 0;
				}
				if (check_0 == 1) {
					if (!strncmp("wifisetup", frame_buf + 11, 9)) {
						strncpy(ssid, frame_buf + 20, 32);
						strncpy(psk, frame_buf + 52, 32);
						APP_INFO1("WiFi setup,ssid:%s,psk:%s", ssid, psk);
						system("rm -rf /var/www/cgi-bin/wifi/select.txt");
						system("touch /var/www/cgi-bin/wifi/select.txt");
						system("chmod 644 /var/www/cgi-bin/wifi/select.txt");
						fd = fopen(ssid_psk_file, "wb");
						ret = fwrite(ssid, strlen(ssid), 1, fd);
						if (ret != strlen(ssid)) {
							APP_INFO0("write wifi ssid error");
						}
						ret = fwrite("\n", 1, 1, fd);
						if (ret != 1) {
							APP_INFO0("write enter and feedline error");
						}
						ret = fwrite(psk, strlen(psk), 1, fd);
						if (ret != strlen(psk)) {
							APP_INFO0("write wifi password error");
						}
						fflush(fd);
						system("cd /etc/bsa/config");
						system("sh ./wifi_tool.sh");
						check_0 = 0;
						memset(frame_buf, 0, FRAME_BUF_MAX);
						cnt = 0;
						fd = fopen(wifi_status_file, "r+");
						if (fd <= 0) {
							APP_INFO0("read wifi status file error");
						}
						ret = fread(&wifi_status, 1, 1, fd);
						if (ret == 1) {
							if (!strncmp(&wifi_status, &wifi_success, 1)) {
								wifi_status = 1;
								if (socket_send(sk, &wifi_status, 1) != 1) {
									/*retry to send return value*/
									socket_send(sk, &wifi_status, 1);
								}
								APP_INFO0("wifi setup success, and then exit ble mode");
								sleep(2);
								system("killall aml_musicBox");
								system("aml_musicBox");
								return 0;
							} else {
								wifi_status = 2;
								if (socket_send(sk, &wifi_status, 1) != 1) {
									/*retry to send return value*/
									socket_send(sk, &wifi_status, 1);
								}
								APP_INFO0("wifi setup fail, please double check ssid and password,and retry!");
							}
						}
					}
				}
			}
		}
	}
	teardown_socket_client(sk);
	return 0;
}
