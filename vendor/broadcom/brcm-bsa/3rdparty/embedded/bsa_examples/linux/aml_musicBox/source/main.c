/*****************************************************************************
 **
 **  Name:           app_manager_main.c
 **
 **  Description:    Bluetooth Manager menu application
 **
 **  Copyright (c) 2010-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "gki.h"
#include "uipc.h"

#include "bsa_api.h"

#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_utils.h"
#include "app_dm.h"

#include "app_services.h"
#include "app_mgt.h"

#include "app_manager.h"
#include "app_avk.h"
#include "app_hs.h"
#include "app_ble.h"
#include "app_ble_server.h"
#include "app_socket.h"

#ifdef ENABLE_AUDIOSERVICE
#include "audioservice.h"
#include "as_client.h"
#include "aml_syslog.h"
static Audioservice *as_proxy = NULL;
#endif

/*
 * Extern variables
 */
extern BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */
extern tAPP_MGR_CB app_mgr_cb;
extern tAPP_XML_CONFIG         app_xml_config;
extern char socket_rev[256];
extern int socket_rev_len;
int ble_mode = 0;
tAPP_SOCKET sk_handle;
extern int ble_sk_fd = 0;
int main_sk_fd = 0;
char socket_path[] = "/etc/bsa/config/aml_musicBox_socket";

/*
 * Local functions
 */

/*******************************************************************************
 **
 ** Function         app_ble_setup
 **
 ** Description      Handle the BLE menu
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ble_setup(void)
{
	int choice, type, i;
	UINT16 ble_scan_interval, ble_scan_window;
	tBSA_DM_BLE_CONN_PARAM conn_param;
	tBSA_DM_BLE_ADV_CONFIG adv_conf;
	tBSA_DM_BLE_ADV_PARAM adv_param;
	UINT16 number_of_services;
	UINT8 app_ble_adv_value[APP_BLE_ADV_VALUE_LEN] = {0x2b, 0x1a, 0xaa, 0xbb, 0xcc, 0xdd}; /*First 2 byte is Company Identifier Eg: 0x1a2b refers to Bluetooth ORG, followed by 4bytes of data*/

	APP_INFO0("app ble server register");
	app_ble_server_register(APP_BLE_MAIN_INVALID_UUID, NULL);
	APP_INFO0("app ble server create service");
	app_ble_server_create_service();
	sleep(1); //must
	APP_INFO0("app ble server add char");
	app_ble_server_add_char(0);
	APP_INFO0("app ble server add char");
	app_ble_server_add_char(1);
	APP_INFO0("app ble server start service");
	app_ble_server_start_service();
	APP_INFO0("app ble server display");
	app_ble_server_display();
	APP_INFO0("Configure BLE Advertisement data");
	/*******************Configure BLE Advertisement data begin*************/
	memset(&adv_conf, 0, sizeof(tBSA_DM_BLE_ADV_CONFIG));
	/* start advertising */
	adv_conf.len = APP_BLE_ADV_VALUE_LEN;
#ifdef DUEROS_SDK
	adv_conf.flag = 0x01;
#else
	adv_conf.flag = BSA_DM_BLE_ADV_FLAG_MASK;
#endif
	memcpy(adv_conf.p_val, app_ble_adv_value, APP_BLE_ADV_VALUE_LEN);
	/* All the masks/fields that are set will be advertised*/
#ifdef DUEROS_SDK
	adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_SERVICE | BSA_DM_BLE_AD_BIT_DEV_NAME;
#else
	adv_conf.adv_data_mask = BSA_DM_BLE_AD_BIT_FLAGS | BSA_DM_BLE_AD_BIT_SERVICE | BSA_DM_BLE_AD_BIT_APPEARANCE | BSA_DM_BLE_AD_BIT_MANU;
#endif
	adv_conf.appearance_data = 0x1122;
	APP_INFO1("Enter appearance value Eg:0x1122:0x%x", adv_conf.appearance_data);
	number_of_services = 1;
	APP_INFO1("Enter number of services between <1-6> Eg:2: %d", number_of_services);
	adv_conf.num_service = number_of_services;
	for (i = 0; i < adv_conf.num_service; i++)
	{
#ifdef DUEROS_SDK
		adv_conf.uuid_val[i] = 0x1111;
#else
		adv_conf.uuid_val[i] = 0x180A;
#endif
		APP_INFO1("Enter service UUID :0x%x", adv_conf.uuid_val[i]);
	}
	adv_conf.is_scan_rsp = 0;
	APP_INFO1("Is this scan response? (0:FALSE, 1:TRUE): %d", adv_conf.is_scan_rsp);
	app_dm_set_ble_adv_data(&adv_conf);
	/*******************Configure BLE Advertisement data end*************/
}

/*******************************************************************************
 **
 ** Function         app_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
	switch (event)
	{
	case BSA_MGT_STATUS_EVT:
		APP_DEBUG0("BSA_MGT_STATUS_EVT");
		if (p_data->status.enable)
		{
			APP_DEBUG0("Bluetooth restarted => re-initialize the application");
			app_mgr_config(TRUE);/*TURE: BLE MODE, FALSE: NO BLE MODE*/
			app_avk_init(NULL);
			app_hs_init();
			if (ble_mode == 1)
				app_ble_start();
		}
		break;

	case BSA_MGT_DISCONNECT_EVT:
		APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
		exit(-1);
		break;

	default:
		break;
	}
	return FALSE;
}

/*******************************************************************************
 **
 ** Function         signal_handler
 **
 ** Description      an hanlder to interrupt and termination signals
 **
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/

static void signal_handler(int sig)
{
	APP_ERROR0("signal_handler enter");

	if (ble_mode == 1) {
		/* Exit BLE mode */
		app_ble_exit();
	} else {
		app_hs_stop();
#ifdef PCM_ALSA_DSPC
		libdspc_stop();
#endif
		app_avk_end();
		app_mgt_close();

	}
	teardown_socket_server(&sk_handle);
	signal(sig, SIG_DFL);
	exit(0);
}


#ifdef ENABLE_AUDIOSERVICE
void asclient_callback(AML_AS_NOTIFYID_e type, ASClientNotifyParam_t *param) {
	BD_ADDR bddr = {0};
	static int device_connected = 0;
	// APP_INFO1("asclient callback type = %d\n", type);
	switch (type) {
		case AML_AS_NOTIFY_SOURCE_BEFORE_CHANGE:
			if (device_connected == 1)
				break;
			// input source will be switched to other source from BT
			if (param->param.source.source_id == AML_AS_INPUT_BT) {
				// Stop BT
				if (app_avk_num_connections() != 0) {
					bdcpy(bddr, app_avk_find_connection_by_index(0)->bda_connected);
					app_avk_close_all();
					app_hs_close();
					// Resume BT
					app_avk_open(bddr);
					app_hs_open(bddr);
				}
			}
			break;
		case AML_AS_NOTIFY_POWER_SUSPEND:
			if (app_avk_num_connections() != 0) {
				bdcpy(bddr, app_avk_find_connection_by_index(0)->bda_connected);
				app_avk_close_all();
				app_hs_close();
				device_connected = 1;
			}
			break;
		case AML_AS_NOTIFY_POWER_ON:
			if (device_connected == 1) {
				// Resume BT
				app_avk_open(bddr);
				app_hs_open(bddr);
				device_connected = 0;
			}
			break;
		default:
			// APP_INFO0("Not handle this callback\n");
			break;
	}
}
#endif
/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      This is the main function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
	int mode, sk, bytes, device_connected = 0;
	int status;
	char msg[64];
	BD_ADDR bddr = {0};

	if (argc == 2) {
		if (!strncmp(argv[1], "ble_mode", 8)) {
			ble_mode = 1;
			APP_INFO0("enter ble mode");
		} else {
			ble_mode = 0;
		}
	} else
		ble_mode = 0;

	/*app_manager init*/
	app_mgt_init();
	if (app_mgt_open(NULL, app_mgt_callback) < 0)
	{
		APP_ERROR0("Unable to connect to server");
		return -1;
	}

	/* Init XML state machine */
	app_xml_init();

	if (app_mgr_config(TRUE))/*TURE: BLE MODE, FALSE: NO BLE MODE*/
	{
		APP_ERROR0("Couldn't configure successfully, exiting");
		return -1;
	}

	/* Display FW versions */
	app_mgr_read_version();
	/* Get the current Stack mode */
	mode = app_dm_get_dual_stack_mode();
	if (mode < 0)
	{
		APP_ERROR0("app_dm_get_dual_stack_mode failed");
		return -1;
	}
	else
	{
		/* Save the current DualStack mode */
		app_mgr_cb.dual_stack_mode = mode;
		APP_INFO1("Current DualStack mode:%s", app_mgr_get_dual_stack_mode_desc());
	}

	if (ble_mode == 1) {
		/***********set app ble begin*****************/
		/* Initialize BLE application */
		status = app_ble_init();
		if (status < 0)
		{
			APP_ERROR0("Couldn't Initialize BLE app, exiting");
			exit(-1);
		}

		/* Start BLE application */
		status = app_ble_start();
		if (status < 0)
		{
			APP_ERROR0("Couldn't Start BLE app, exiting");
			return -1;
		}

		/* register one application */
		app_ble_client_register(APP_BLE_MAIN_DEFAULT_APPL_UUID);

		/* The BLE setup */
		app_ble_setup();

		APP_INFO0("\tEnable Set BLE Visibiltity");
		app_dm_set_ble_visibility(TRUE, TRUE);
		app_dm_set_visibility(FALSE, FALSE);
		/***********set app ble end*****************/
	} else {
		/* Init Ad2p Sink Application */
		app_avk_get_alsa_device_conf();
		app_avk_init(NULL);
		app_avk_register();
		/* Init Headset Application */
		app_hs_init();
		app_hs_start(NULL);
		/* Init libdspc Application */
#ifdef PCM_ALSA_DSPC
		libdspc_init();
#endif
#ifdef ENABLE_AUDIOSERVICE
		APP_DEBUG0("Enable audioservice client\n");
		AML_SYSLOG_INIT(LOG_LOCAL2);
		/* Init AudioService Client */
		if (AML_AS_SUCCESS != AS_Client_Init(NULL, &as_proxy)) {
			APP_ERROR0("Cannot initialize audioservie client\n");
			return -1;
		}
		AS_Client_MainLoop(asclient_callback, 1);
#endif

	}
	/*suppose the program is only terminated by interrupt or termintion singal*/
	memcpy(sk_handle.sock_path, socket_path, strlen(socket_path));
	setup_socket_server(&sk_handle);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	while (1) {
begin:
		sk = accpet_client(&sk_handle);
		if (sk < 0) {
			APP_ERROR0("accept client fail\n");
			sleep(1);
			continue;
		}
		memset(msg, 0, sizeof(msg));
		bytes = socket_recieve(sk, msg, sizeof(msg));
		if (bytes == 0 ) {
			APP_DEBUG0("client leaved, waiting for reconnect");
			goto begin;
		}
		APP_DEBUG1("bytes = %d, msg: %s", bytes, msg);

		if (strncmp(msg, "aml_ble_setup_wifi", 18) == 0) {
			if (ble_mode == 1) {
				APP_INFO0("aml_ble_setup_wifi connected");
				int i;
				ble_sk_fd = sk;
				while (1) {
#ifdef DUEROS_SDK
					socket_rev_len = socket_recieve(ble_sk_fd, socket_rev, 256);
					if (socket_rev_len > 0) {
						if (socket_rev_len > 20) {
							for (i = 0 ; i < socket_rev_len / 20 ; i++ )
								app_ble_server_send_indication(20, socket_rev + i * 20);
							if (socket_rev_len % 20)
								app_ble_server_send_indication(socket_rev_len - 20 * i , socket_rev + 20 * i);
						} else
							app_ble_server_send_indication(socket_rev_len, socket_rev);
					}

#else
					socket_rev_len = socket_recieve(ble_sk_fd, socket_rev, 1);
					if (socket_rev_len == 1)
						APP_INFO1("\tget ble return wifi status value: %d", socket_rev[0]);
					sleep(10);
#endif
				}
			}
		} else {
			main_sk_fd = sk;
			while (1) {
				memset(msg, 0, sizeof(msg));
				bytes = socket_recieve(main_sk_fd, msg, sizeof(msg));
				if (bytes == 0 ) {
					APP_DEBUG0("client leaved, waiting for reconnect");
					goto begin;
				}
				APP_DEBUG1("msg: %s", msg);
				if (strcmp(msg, "suspend") == 0 ) {
					if (app_avk_num_connections() != 0) {
						bdcpy(bddr, app_avk_find_connection_by_index(0)->bda_connected);
						device_connected = 1;
						app_avk_close_all();
						app_hs_close();
					}

				} else if (strcmp(msg, "resume") == 0 ) {
					if (device_connected == 1) {
						app_avk_open(bddr);
						app_hs_open(bddr);
						device_connected = 0;
					}
				}
			}
		}
	}
	return 0;
}
