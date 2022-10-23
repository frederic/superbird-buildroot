#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"

#include "gdbus/gdbus.h"

#include "src/dbus-common.h"
#include "src/plugin.h"
#include "src/adapter.h"
#include "src/log.h"

/********************************************************************/
/*Dev strategy:														*/
/*0: not strategy, no limit to connected device number				*/
/*																	*/
/*1: only one connected device allowed,								*/
/*   always reject next device till prev device disconnected		*/
/*																	*/
/*2: only one connected device allowed,								*/
/*   disconnect prev device when next device request connection		*/
/*3: Smart Strategy													*/
/*   Use strategy 1 in conditions:									*/
/*   a. in the first 10s after device connected						*/
/*   b. while conncted device is playing music						*/
/*   c. while conncted device is making phonecall					*/
/*																	*/
/*   In other condisions, use strategy 2 							*/
/*																	*/
/********************************************************************/
#define DEV_STRATEGY 2

#define RESERVED_DEVICE_FILE "/etc/bluetooth/reserved_device"
static GDBusClient *client = NULL;
static GDBusProxy *adapter_proxy = NULL;
static GDBusProxy *reserved_device_proxy = NULL;
static const char *reserved_device_path = NULL;
static GList *dev_list = NULL;
static char SRT[2] = {5, 2};		//Startup reconnect timeout

static gboolean connect_dev(void* user_data);
static void connect_dev_return(DBusMessage *message, void *user_data);
static int set_discoverable(int enable);
static int set_pairable(int enable);
static int disconnect_prev_dev();
static int disconnect_next_dev();

#if (DEV_STRATEGY == 3)
static int hfp_ctl_sk;
static pthread_t phfp_ctl_id;
#define HFP_CTL_SK_PATH "/etc/bluetooth/hfp_ctl_sk"
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

/*new connected device can keep the lock for 30s*/
#define NEW_CON_TIMEOUT 10

typedef int tLOCK;
#define NEW_CON_LOCK			(tLOCK)1<<0
#define A2DP_PLAY_LOCK			(tLOCK)1<<1
#define HFP_CALL_LOCK			(tLOCK)1<<2
#define HFP_CALLSETUP_LOCK		(tLOCK)1<<3
static tLOCK connection_lock = 0;

static guint timer_id = 0;
static pthread_mutex_t dev_mutex;

static void excute_with_lock(void)
{
	pthread_mutex_lock(&dev_mutex);

	/*if connection_lock set, use strategy 1*/
	if (connection_lock)
		disconnect_next_dev();
	/*if connection_lock cleared, use strategy 2*/
	else
		disconnect_prev_dev();

	pthread_mutex_unlock(&dev_mutex);
}

static void update_lock(int set, tLOCK type)
{
	pthread_mutex_lock(&dev_mutex);
	info("%s: %s bit%d\n", __func__, (set ? "set": "clear"), type);
	if (set) {
		/*only if connection_lock not set before*/
		if (connection_lock == 0) {
			info("%s: acquire lock", __func__);
			set_discoverable(0);
			set_pairable(0);
		}

		connection_lock |= type;
	} else if (connection_lock != 0){
		connection_lock &= ~type;

		/*only if connection_lock is completely clear*/
		if (connection_lock == 0) {
			info("%s: release lock", __func__);
			set_discoverable(1);
			set_pairable(1);
		}
	}
	info("%s: connection_lock = 0x%x", __func__, connection_lock);
	pthread_mutex_unlock(&dev_mutex);
}

static gboolean new_con_cback(void* user_data)
{
	info("%s: new connection lock timeout!\n", __func__);
	update_lock(0, NEW_CON_LOCK);
	return FALSE;
}

static void *hfp_ctl_cb(void *data)
{
	struct sockaddr_un address;
	int fd, len;
	char msg[64] = {0};

init:
	memset(&address, 0, sizeof(struct sockaddr_un));

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			error("%s", strerror(errno));
			return NULL;
	}

	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, HFP_CTL_SK_PATH, sizeof(address.sun_path));

	len = sizeof(address);

	if (connect(fd, (struct sockaddr *)&address, len) == -1) {
		/*if socket is not avialable or not ready, don't report error*/
		if (errno != 111 && errno != 2)
			info("connect server: %s", strerror(errno));

		sleep(3);
		goto init;
	}

	hfp_ctl_sk  = fd;

	info("dev_strategy: socket recieving....\n");
	while (1) {
		memset(msg, 0, sizeof(msg));
		len = recv(hfp_ctl_sk, msg, sizeof(msg), 0);
		if (len < 0) {
			perror("recv");
			continue;
		}
		if (len == 0) {
			info("dev_strategy: socket off line\n");
			sleep(3);
			goto init;
		}

		if (len == 2)
			DBG("event = %d, value = %d\n", msg[0], msg[1]);
		else
			continue;

		switch (msg[0]) {
			case HFP_EVENT_CONNECTION:
				switch (msg[1]) {
					case HFP_IND_DEVICE_DISCONNECTED:
						update_lock(0, HFP_CALL_LOCK);
						update_lock(0, HFP_CALLSETUP_LOCK);
						break;
				}
				break;

			case HFP_EVENT_CALL:
				switch (msg[1]) {
					case HFP_IND_CALL_NONE:
						update_lock(0, HFP_CALL_LOCK);
						break;
					case HFP_IND_CALL_ACTIVE :
						update_lock(1, HFP_CALL_LOCK);
						break;
				}
				break;

			case HFP_EVENT_CALLSETUP:
				switch (msg[1]) {
					case HFP_IND_CALLSETUP_NONE:
						update_lock(0, HFP_CALLSETUP_LOCK);
						break;
					case HFP_IND_CALLSETUP_IN :
					case HFP_IND_CALLSETUP_OUT :
						update_lock(1, HFP_CALLSETUP_LOCK);
						break;
				}
				break;
		}
	}

	info("exit\n");
	return NULL;
}

static int setup_socket(void)
{

	info(" ");

	if (pthread_create(&phfp_ctl_id, NULL, hfp_ctl_cb, NULL)) {
		error("%s", strerror(errno));
		return -1;
	}

	pthread_setname_np(phfp_ctl_id, "hfp_ctl_monitor_dev");

}

static void teardown_socket(void)
{
	info(" ");
	if (phfp_ctl_id) {
		pthread_cancel(phfp_ctl_id);
		pthread_detach(phfp_ctl_id);

		if (hfp_ctl_sk)
			close(hfp_ctl_sk);

		phfp_ctl_id = 0;
		hfp_ctl_sk  = 0;
	}

}


#endif	//DEV_STRATEGY == 3

static void save_reserved_device()
{
	FILE *fp;
	const char *obj_path;

	if (!reserved_device_proxy) {
		info("reserved_device_proxy is NULL");
		return;
	}

	obj_path = g_dbus_proxy_get_path(reserved_device_proxy);

	info("reserved device obj_path = %s\n", obj_path);

	fp = fopen(RESERVED_DEVICE_FILE, "w");
	if (fp  == NULL) {
		error("open %s: %s", RESERVED_DEVICE_FILE, strerror(errno));
		return;
	}

	fwrite(obj_path, strlen(obj_path), 1, fp);

	fclose(fp);

}

static const char* get_reserved_device()
{
	FILE *fp;
	static char obj_path[] = "/org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx";


	fp = fopen(RESERVED_DEVICE_FILE, "r");
	if (fp  == NULL) {
		error("open %s: %s", RESERVED_DEVICE_FILE, strerror(errno));
		return obj_path;
	}

	fread(obj_path, sizeof(obj_path), 1, fp);

	fclose(fp);

	info("reserved device obj_path readed = %s\n", obj_path);

	return obj_path;

}

static int set_discoverable(int enable)
{
	dbus_bool_t value;
	DBusMessageIter iter;

	if (!adapter_proxy) {
		error("Enable/Disable disacoverable fail, org.bluez.Adapter1 not registered yet!");
		return -1;
	}

	if (enable) {
		info("Set local device discoverable");
		value = TRUE;
	} else {
		info("Set local device undiscoverable");
		value = FALSE;
	}

	g_dbus_proxy_set_property_basic(adapter_proxy,
									"Discoverable",
									DBUS_TYPE_BOOLEAN,
									&value,
									NULL,
									NULL,
									NULL);
	return 0;
}

static int set_pairable(int enable)
{
	dbus_bool_t value;
	DBusMessageIter iter;

	if (!adapter_proxy) {
		error("Enable/Disable pairable fail, org.bluez.Adapter1 not registered yet!");
		return -1;
	}

	if (enable) {
		info("Set local device pairable");
		value = TRUE;
	} else {
		info("Set local device unpairable");
		value = FALSE;
	}

	g_dbus_proxy_set_property_basic(adapter_proxy,
									"Pairable",
									DBUS_TYPE_BOOLEAN,
									&value,
									NULL,
									NULL,
									NULL);
	return 0;
}

static void connect_dev_return(DBusMessage *message, void *user_data)
{

	DBusError error;
	static int cnt = 3;

	dbus_error_init(&error);
	if (dbus_set_error_from_message(&error, message) == TRUE) {
		info("%s get msg: %s", __func__, error.name);

		if (strstr(error.name, "Failed") && (cnt > 0)) {
			info("%s reset timer", __func__);
			cnt--;
			g_timeout_add_seconds(SRT[1], connect_dev, user_data);

		}
		dbus_error_free(&error);
	}
}

static gboolean connect_dev(void* user_data)
{
	GDBusProxy *proxy = (GDBusProxy *) user_data;
	DBusMessageIter iter;

	info("Startup reconnection begin!!");

	if (!proxy) {
		error("Connecet device  fail, invalid proxy!");
		return FALSE;
	}

	if (g_list_length(dev_list) > 0) {
		error("Device connected already, return!");
		return FALSE;

	}

	info("Connect target device: %s", g_dbus_proxy_get_path(proxy));

	if (g_dbus_proxy_method_call(proxy,
								 "Connect",
								 NULL,
								 connect_dev_return,
								 user_data,
								 NULL) == FALSE) {
		error("Failed to call org.bluez.Device1.Connect");
	}

	//not matter what, stop timer
	return FALSE;
}

static int disconnect_prev_dev()
{
	DBusMessageIter iter;

	GList *temp = g_list_last(dev_list)->prev;

	info("Disconnect earlier connected device");

	while (temp) {
		GDBusProxy *proxy = (GDBusProxy *)temp->data;
		info("Disconnecting device, obj_path = %s\n", g_dbus_proxy_get_path(proxy));
		if (g_dbus_proxy_method_call(proxy,
									 "Disconnect",
									 NULL,
									 NULL,
									 NULL,
									 NULL) == FALSE) {
			error("Failed to call org.bluez.Device1.Disconnect");
		}
		temp = temp->prev;
	}

	return 0;
}

static int disconnect_next_dev()
{
	DBusMessageIter iter;
	GList *temp = g_list_first(dev_list)->next;

	info("Disconnect later connected device");

	while (temp) {
		GDBusProxy *proxy = (GDBusProxy *)temp->data;
		info("Disconnecting device, obj_path = %s\n", g_dbus_proxy_get_path(proxy));
		if (g_dbus_proxy_method_call(proxy,
									 "Disconnect",
									 NULL,
									 NULL,
									 NULL,
									 NULL) == FALSE) {
			error("Failed to call org.bluez.Device1.Disconnect");
		}
		temp = temp->next;
	}

	return 0;
}

static int strategy_excute()
{
	int dev_num = g_list_length(dev_list);

	info("Connected device number = %d", dev_num);
#if (DEV_STRATEGY == 1)
	if (dev_num > 1)
		disconnect_next_dev();

	if (dev_num > 0)
		set_discoverable(0);
	else
		set_discoverable(1);

#elif (DEV_STRATEGY == 2)
	if (dev_num > 1)
		disconnect_prev_dev();

#elif (DEV_STRATEGY == 3)

	if (dev_num == 0) {
		/*clear all lock*/
		update_lock(0, NEW_CON_LOCK | A2DP_PLAY_LOCK | HFP_CALL_LOCK | HFP_CALLSETUP_LOCK);

		/*if new_con timer still running, we should turn it off*/
		if (timer_id) {
			GSource *s = g_main_context_find_source_by_id(NULL, timer_id);
			if (s != NULL) {
				g_source_destroy(s);
				timer_id = 0;
			}
		}
	} else if (dev_num > 1) {
		excute_with_lock();
	}

#endif

	if (dev_num == 1) {
		reserved_device_proxy = (GDBusProxy *)g_list_first(dev_list)->data;
		save_reserved_device();
	}

	return 0;

}

static void property_changed(GDBusProxy *proxy, const char *name,
							 DBusMessageIter *iter, void *user_data)
{
	const char *interface, *status;
	dbus_bool_t valbool;


	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.Device1")) {
		if (!strcmp(name, "Connected")) {
			dbus_message_iter_get_basic(iter, &valbool);
			info("%s, Connectd status changed:  %s\n", g_dbus_proxy_get_path(proxy), valbool == TRUE ? "TRUE" : "FALSE");
			if (TRUE == valbool) {
				dev_list = g_list_append(dev_list, proxy);
			} else {
				dev_list = g_list_remove(dev_list, proxy);
			}
			strategy_excute();
		}
	}

#if (DEV_STRATEGY == 3)
	if (!strcmp(interface, "org.bluez.MediaControl1")) {
		if (!strcmp(name, "Connected")) {
			dbus_message_iter_get_basic(iter, &valbool);
			info("%s, A2dp Connectd status changed:  %s\n", g_dbus_proxy_get_path(proxy), valbool == TRUE ? "TRUE" : "FALSE");
			if (TRUE == valbool) {
				GSource *s = NULL;
				if (timer_id)
					s = g_main_context_find_source_by_id(NULL, timer_id);
				/*if prv-timer is still runnig, destroy it first*/
				if (s)
					g_source_destroy(s);
				/*use a timer to unlock later*/
				timer_id = g_timeout_add_seconds(NEW_CON_TIMEOUT, new_con_cback, NULL);

				update_lock(1, NEW_CON_LOCK);
			}
		}
	}

	if (!strcmp(interface, "org.bluez.MediaPlayer1")) {
		if (!strcmp(name, "Status")) {
			dbus_message_iter_get_basic(iter, &status);
			info("%s, A2dp play status changed:  %s\n", g_dbus_proxy_get_path(proxy), status);
			if (!strcmp(status, "playing"))
				update_lock(1, A2DP_PLAY_LOCK);
			else
				update_lock(0, A2DP_PLAY_LOCK);
		}
	}
#endif //DEV_STRATEGY == 3
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
	const char *interface;
	DBusMessageIter iter;
	dbus_bool_t valbool;
	static int startup = 1;

	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.Device1")) {
		const char *obj_path =  g_dbus_proxy_get_path(proxy);
		info("org.bluez.Device1 registered: %s\n", obj_path);
		if (TRUE == g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
			dbus_message_iter_get_basic(&iter, &valbool);
			if (TRUE == valbool) {
				dev_list = g_list_append(dev_list, proxy);
				strategy_excute();
			} else {
				/*If device is found in startup and is reserved_device
				  we will reconnect it*/
				if (!strcmp(obj_path, reserved_device_path) && startup) {
					info("Going to reconnect last connected device!");
					startup = 0;
					//create a timer call connect_dev later
					g_timeout_add_seconds(SRT[0], connect_dev, (void *)proxy);
				}
			}
		}

	}

	if (!strcmp(interface, "org.bluez.Adapter1"))
		adapter_proxy = proxy;

}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
	const char *interface;
	DBusMessageIter iter;

	interface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(interface, "org.bluez.Device1")) {
		info("org.bluez.Device1 removed: %s\n", g_dbus_proxy_get_path(proxy));
	}

	if (!strcmp(interface, "org.bluez.Adapter1"))
		adapter_proxy = NULL;
}

static int dev_strategy_probe(struct btd_adapter *adapter)
{
	DBG("");
	g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed, property_changed, NULL);

	return 0;
}

static void dev_strategy_remove(struct btd_adapter *adapter)
{
	DBG("");
}

static struct btd_adapter_driver dev_strategy_driver = {
	.name 	= "dev_strategy",
	.probe 	= dev_strategy_probe,
	.remove	= dev_strategy_remove,
};

static int dev_strategy_init(void)
{
	int err = 0;
#if (DEV_STRATEGY > 0)
	DBusConnection *conn = btd_get_dbus_connection();

	DBG("%s", __func__);
	info("dev_strategy%d starting work!!", DEV_STRATEGY);

	client =  g_dbus_client_new(conn, "org.bluez", "/org/bluez");

	err = btd_register_adapter_driver(&dev_strategy_driver);
	if (err < 0) {
		g_dbus_client_unref(client);
		error("Failed to register btd driver\n");
	}

	reserved_device_path = get_reserved_device();
#if (DEV_STRATEGY == 3)
	setup_socket();
#endif

#else
	info("dev_strategy won't work!!");
#endif

	return err;
}

static void dev_strategy_exit(void)
{
	DBG("%s", __func__);
#if (DEV_STRATEGY > 0)
	btd_unregister_adapter_driver(&dev_strategy_driver);

	if (client) {
		g_dbus_client_unref(client);
		client = NULL;
	}

	if (dev_list) {
		g_list_free(dev_list);
		dev_list = NULL;
	}

#if (DEV_STRATEGY == 3)
	teardown_socket();
#endif

#endif
}

BLUETOOTH_PLUGIN_DEFINE(dev_strategy, VERSION,
						BLUETOOTH_PLUGIN_PRIORITY_DEFAULT, dev_strategy_init, dev_strategy_exit)
