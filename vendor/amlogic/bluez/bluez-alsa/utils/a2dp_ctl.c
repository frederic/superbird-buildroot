#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <pthread.h>
#include "a2dp_ctl.h"

#define COD_SERVICE_INFORMATION                       (1<<23)
#define COD_SERVICE_TELEPHONY                         (1<<22)
#define COD_SERVICE_AUDIO                             (1<<21)
#define COD_SERVICE_OBJECT_TRANSFER                   (1<<20)
#define COD_SERVICE_CAPTUREING                        (1<<19)
#define COD_SERVICE_RENDERING                         (1<<18)
#define COD_SERVICE_NETWORKING                        (1<<17)
#define COD_SERVICE_POSITIONING                       (1<<16)
#define COD_SERVICE_LIMITED_DISCOVERABLE_MODE         (1<<13)

#define GENERIC_AUDIO_UUID	"00001203-0000-1000-8000-00805f9b34fb"
#define HSP_HS_UUID		"00001108-0000-1000-8000-00805f9b34fb"
#define HSP_AG_UUID		"00001112-0000-1000-8000-00805f9b34fb"
#define HFP_HS_UUID		"0000111e-0000-1000-8000-00805f9b34fb"
#define HFP_AG_UUID		"0000111f-0000-1000-8000-00805f9b34fb"
#define ADVANCED_AUDIO_UUID	"0000110d-0000-1000-8000-00805f9b34fb"
#define A2DP_SOURCE_UUID	"0000110a-0000-1000-8000-00805f9b34fb"
#define A2DP_SINK_UUID		"0000110b-0000-1000-8000-00805f9b34fb"
#define AVRCP_REMOTE_UUID	"0000110e-0000-1000-8000-00805f9b34fb"
#define AVRCP_TARGET_UUID	"0000110c-0000-1000-8000-00805f9b34fb"
#define SCAN_FILTER

#define INFO(fmt, args...) \
	printf("[A2DP_CTL][%s] " fmt, __func__, ##args)

#define A2DP_CTL_DEBUG 0
#if A2DP_CTL_DEBUG
#define DEBUG(fmt, args...) \
	printf("[A2DP_CTL][DEBUG][%s] " fmt, __func__, ##args)
#else
#define DEBUG(fmt, args...)
#endif

#define TRANSPORT_INTERFACE "org.bluez.MediaTransport1"
static char TRANSPORT_OBJECT[128] = {0};

#define PLAYER_INTERFACE "org.bluez.MediaPlayer1"
static char PLAYER_OBJECT[128] = {0};

#define CONTROL_INTERFACE "org.bluez.MediaControl1"
//static char CONTROL_OBJECT[128] = {0};

#define ADAPTER_INTERFACE "org.bluez.Adapter1"
static char ADAPTER_OBJECT[128] = {0};

#define DEVICE_INTERFACE "org.bluez.Device1"

static GMainLoop *main_loop;
static pthread_t thread_id;
static GDBusConnection *conn;
static gboolean A2dpConnected = FALSE;

static int call_player_method(char *method);
static GVariant *get_property(char *obj, char *inf, char *prop);
static int modify_tansport_volume_property(gboolean up);
static void *dbus_thread(void *user_data);
static int call_objManager_method(void);
static void subscribe_signals(void);
static void unsubscribe_signals(void);
static GList *dev_list;
static char device_mode[] = "central";

gint ifa_signal_handle = 0;
gint ifr_signal_handle = 0;
gint pch_signal_handle = 0;

int player_init(void)
{
	gchar *address;
	GError *err = NULL;

	INFO("\n");
	address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	if (( conn = g_dbus_connection_new_for_address_sync(address,
					G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
					G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
					NULL, NULL, &err)) == NULL) {
		INFO("Couldn't obtain D-Bus connection: %s", err->message);
		return -1;
	}

	//check status
	call_objManager_method();
	//monitor
	subscribe_signals();

	if (pthread_create(&thread_id, NULL, dbus_thread, NULL)) {
		INFO("thread create failed\n");
		unsubscribe_signals();
		return -1;
	}

	return 0;
}

void player_delinit(void)
{
	INFO("\n");
	unsubscribe_signals();
	conn = NULL;

	if (NULL != main_loop)
		g_main_loop_quit(main_loop);

}

int start_play(void)
{
	return call_player_method("Play");
}

int stop_play(void)
{
	return call_player_method("Stop");
}

int pause_play(void)
{
	return call_player_method("Pause");
}

int next(void)
{
	return call_player_method("Next");
}

int previous(void)
{
	return call_player_method("Previous");
}

int volume_up()
{
	INFO("\n");
	return modify_tansport_volume_property(TRUE);
}

int volume_down()
{
	INFO("\n");
	return modify_tansport_volume_property(FALSE);
}

void connect_call_back(gboolean connected)
{
	if (TRUE == connected) {
		INFO("A2dp Connected\n");
		/*works when a2dp connected*/

	} else {
		INFO("A2dp Disconnected\n");
		/*works when a2dp disconnected*/
	}

}

void play_call_back(char *status)
{

	/*Possible status: "playing", "stopped", "paused"*/
	if (strcmp("playing", status) == 0) {
		INFO("Media_Player is now playing\n");
		/*works when playing*/

	} else if (strcmp("stopped", status) == 0) {
		INFO("Media_Player stopped\n");
		/*works when stopped*/

	} else if (strcmp("paused", status) == 0) {
		INFO("Media_Player paused\n");
		/*works when paused*/

	}
}

static void unref_variant(GVariant *v)
{
	if (v != NULL)
		g_variant_unref(v);
}

static int call_player_method(char *method)
{

	GVariant *result;
	GError *error = NULL;
	int ret = -1;
	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

	if (NULL == method) {
		INFO("Invalid args!!\n") ;
		return ret;
	}

	if (FALSE == A2dpConnected) {
		INFO("A2dp not connected yet!\n") ;
		return ret;
	}

	if (NULL == method) {
		INFO("Invalid args!!\n") ;
		return ret;
	}

	INFO("args: %s\n", method);

	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			PLAYER_OBJECT,
			PLAYER_INTERFACE,
			method,
			NULL,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL) {
		INFO("Error: %s\n", error->message);
		g_error_free (error);
		return ret;
	} else
		ret = 0;


	g_variant_unref(result);

	return ret;
}

static void connect_status(void)
{

	char dev_obj[256] = {0};

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return;
	}

	if (strncmp("/org/bluez", TRANSPORT_OBJECT, 10) != 0) {
		INFO("\n\n****Connected Device Information******\n      No Connected Device!!\n\n");
	} else {
		/*copy obj_path from trasport, example: /org/bluez/hci0/dev_22_22_B0_69_CE_60/fd4
		 only the front 37 bytes copied */
		strncpy(dev_obj, TRANSPORT_OBJECT, 37);

		INFO("\n\n****Connected Device Information******\nName   : %s\nAddress: %s\nState  : %s\n\n",
			g_variant_get_string(get_property((char *)dev_obj, DEVICE_INTERFACE, "Name"), NULL),
			g_variant_get_string(get_property((char *)dev_obj, DEVICE_INTERFACE, "Address"), NULL),
			g_variant_get_string(get_property((char *)TRANSPORT_OBJECT, TRANSPORT_INTERFACE, "State"), NULL));
	}

	GList *temp = g_list_first(dev_list);
	while (temp) {
		if (g_variant_get_boolean(get_property((char *)temp->data, DEVICE_INTERFACE, "Paired")))
			printf("Paired  Device : %s   %s \n",
				g_variant_get_string(get_property((char *)temp->data, DEVICE_INTERFACE, "Address"), NULL),
				g_variant_get_string(get_property((char *)temp->data, DEVICE_INTERFACE, "Name"), NULL));
		temp = temp->next;
	}
}
#if 0
static void remove_dev(void)
{

	GVariant *result;
	GError *error = NULL;

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return;
	}

	GList *temp = g_list_first(dev_list);
	while (temp) {
		INFO("len %d %s\n", g_list_length(dev_list), temp->data);

		if (!g_variant_get_boolean(get_property((char *)temp->data, DEVICE_INTERFACE, "Paired"))) {
				result = g_dbus_connection_call_sync(conn,
						"org.bluez",
						ADAPTER_OBJECT,
						ADAPTER_INTERFACE,
						"RemoveDevice",
						g_variant_new("(o)", (char *)temp->data),
						NULL,
						G_DBUS_CALL_FLAGS_NONE,
						-1,
						NULL,
						&error);

				if (result == NULL) {
					INFO("Error: %s\n", error->message);
					g_error_free (error);
				} else
					g_variant_unref(result);
		}

		temp = temp->next;
	}


}
#endif

static void disconnect_dev(void)
{

	GVariant *result;
	GError *error = NULL;
	char obj[256] = {0};

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return;
	}

	if (strncmp("/org/bluez", TRANSPORT_OBJECT, 10) != 0) {
		INFO("a2dp not connected\n");
		return;
	}

	/*copy obj_path from trasport, example: /org/bluez/hci0/dev_22_22_B0_69_CE_60/fd4
	 only the front 37 bytes copied */
	strncpy(obj, TRANSPORT_OBJECT, 37);

	INFO("Target obj: %s\n", obj);

	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			obj,
			DEVICE_INTERFACE,
			"Disconnect",
			NULL,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL) {
		INFO("Error: %s\n", error->message);
		g_error_free (error);
	} else
		g_variant_unref(result);
}

static int connect_dev(const char* bddr)
{

	GVariant *result;
	GError *error = NULL;
	int ret = 1;
	char obj[256] = {0};

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

	sprintf(obj, "/org/bluez/hci0/dev_%s", bddr);

	INFO("Target obj: %s\n", obj);

	if (strncmp("/org/bluez", TRANSPORT_OBJECT, 10) == 0) {
		INFO("There is Connected Device\n");

		if (strncmp(obj, TRANSPORT_OBJECT, 37) != 0) {
			INFO("Disconnect previous device\n");
			disconnect_dev();
			sleep(1);
		}
	}

	GVariant *param;
	/*if we are central, we should connect target's sink uuid*/
	if (strcmp(device_mode, "central") == 0)
		param = g_variant_new("(s)", A2DP_SINK_UUID);
	else
		param = g_variant_new("(s)", A2DP_SOURCE_UUID);


	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			obj,
			DEVICE_INTERFACE,
			"ConnectProfile",
			param,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL) {
		INFO("Error: %s\n", error->message);
		g_error_free (error);
		return ret;
	} else
		ret = 0;


	g_variant_unref(result);

	return ret;
}

static int set_scan_filter(void)
{

	GVariant *result;
	GError *error = NULL;
	int ret = 0;

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

#ifdef SCAN_FILTER
	GVariantBuilder builder;
	GVariant *dict;
	GVariant *array;
	g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));


	/*if we are central, we should discover sink device*/
	if (strcmp(device_mode, "central") == 0)
		g_variant_builder_add (&builder, "s", A2DP_SINK_UUID);
	else
		g_variant_builder_add (&builder, "s", A2DP_SOURCE_UUID);

	array = g_variant_new("as", &builder);
	g_variant_builder_clear(&builder);

	g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
	//g_variant_builder_add (&builder, "{sv}", "UUIDs", array);
	g_variant_builder_add (&builder, "{sv}", "Transport", g_variant_new_string ("bredr"));
	//g_variant_builder_add (&builder, "{sv}", "RSSI", g_variant_new_int16(0x7fff));
	//g_variant_builder_add (&builder, "{sv}", "Pathloss", g_variant_new_uint16(0x7fff));
	//g_variant_builder_add (&builder, "{sv}", "DuplicateData", g_variant_new_boolean(TRUE));

	dict = g_variant_new("(a{sv})", &builder);

	INFO("parameters: %s \n",g_variant_print(dict, TRUE));

	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			ADAPTER_OBJECT,
			ADAPTER_INTERFACE,
			"SetDiscoveryFilter",
			dict,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL) {
		INFO("Error: %s\n", error->message);
		g_error_free (error);
		ret = 1;
	} else
		g_variant_unref(result);

	g_variant_builder_clear(&builder);
#else
	INFO("skip!!\n");
#endif
	return ret;
}


static int adapter_scan(int onoff)
{

	GVariant *result;
	GError *error = NULL;
	int ret = -1;
	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

	INFO("%s onoff: %d\n", __func__, onoff);

	if (onoff) {
		set_scan_filter();
		result = g_dbus_connection_call_sync(conn,
				"org.bluez",
				ADAPTER_OBJECT,
				ADAPTER_INTERFACE,
				"StartDiscovery",
				NULL,
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				G_MAXINT,
				NULL,
				&error);
	} else {
		result = g_dbus_connection_call_sync(conn,
				"org.bluez",
				ADAPTER_OBJECT,
				ADAPTER_INTERFACE,
				"StopDiscovery",
				NULL,
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				&error);
	}

	if (result == NULL) {
		INFO("Error: %s\n", error->message);
		g_error_free (error);
		return ret;
	} else
		ret = 0;


	g_variant_unref(result);

	return ret;
}

static GVariant *get_property(char *obj, char *inf, char *prop)
{

	GVariant *result;
	GError *error = NULL;
	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return NULL;
	}

	if (strncmp("/org/bluez", obj, 10) != 0) {
		return NULL;
	}

	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			obj,
			"org.freedesktop.DBus.Properties",
			"Get",
			g_variant_new("(ss)", inf, prop),
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL) {
		DEBUG("Error: %s\n", error->message);
		g_error_free (error);
		return NULL;
	}

	GVariant *temp = NULL;
	g_variant_get(result, "(v)", &temp);
	g_variant_unref(result);

	DEBUG("prop = %s: %s\n", prop, g_variant_print (temp, TRUE));

	return temp;
}

static int modify_tansport_volume_property(gboolean up)
{

	GVariant *result = NULL, *child = NULL, *parameters = NULL;
	int value = 0, temp = 0, ret = -1;
	GError *error = NULL;

	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

	if (FALSE == A2dpConnected) {
		INFO("A2dp not connected yet!\n");
		return ret;
	}

	/*------------------read volume-----------------------------------------------*/
	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			TRANSPORT_OBJECT,
			"org.freedesktop.DBus.Properties",
			"Get",
			g_variant_new("(ss)", TRANSPORT_INTERFACE, "Volume"),
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL)
	{
		INFO("Error: %s\n", error->message);
		INFO("volume read failed\n");
		g_error_free (error);
		return ret;
	}

	DEBUG("result: %s\n", g_variant_print(result, TRUE));
	DEBUG("result type : %s\n", g_variant_get_type_string(result));
	g_variant_get(result, "(v)", &child);
	g_variant_get(child, "q", &value);

	/*-------------------modify value--------------------------------------------*/
	temp = value;

	if (TRUE == up)
		value += 10;
	else
		value -= 10;

	//volume rang from 0~127
	value = value > 127 ? 127 : value;
	value = value > 0   ? value : 0;

	INFO("volume set: %u->%u\n", temp, value);

	g_variant_unref(child);
	child = g_variant_new_uint16(value);
	parameters = g_variant_new("(ssv)",
			TRANSPORT_INTERFACE,
			"Volume",
			child);

	/*------------------set volume-----------------------------------------------*/
	g_variant_unref(result);
	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			TRANSPORT_OBJECT,
			"org.freedesktop.DBus.Properties",
			"Set",
			parameters,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL)
	{
		INFO("Error: %s\n", error->message);
		INFO("volume set failed\n");
		g_error_free (error);
		return ret;
	} else
		ret = 0;

	g_variant_unref(result);

	return ret;
}

static void signal_interfaces_added(GDBusConnection *conn,
		const gchar *sender,
		const gchar *path,
		const gchar *interface,
		const gchar *signal,
		GVariant *params,
		void *userdata)
{
	GVariantIter *interfaces, *interface_content;
	char *object, *interface_name;


	g_variant_get(params, "(oa{sa{sv}})", &object, &interfaces);

	while (g_variant_iter_next(interfaces, "{sa{sv}}", &interface_name, &interface_content)) {
		if (strcmp(interface_name, TRANSPORT_INTERFACE) == 0) {
			memset(TRANSPORT_OBJECT, 0, sizeof(TRANSPORT_OBJECT));
			INFO("Media_Transport registerd: %s\n", object);
			strncpy(TRANSPORT_OBJECT, object, sizeof(TRANSPORT_OBJECT) -1);
		} else if (strcmp(interface_name, PLAYER_INTERFACE) == 0) {
			memset(PLAYER_OBJECT, 0, sizeof(PLAYER_OBJECT));
			INFO("Media_Player registerd: %s\n", object);
			strncpy(PLAYER_OBJECT, object, sizeof(PLAYER_OBJECT) - 1);
		} else if (strcmp(interface_name, CONTROL_INTERFACE) == 0) {
			INFO("Media_Control registerd: %s\n", object);
		} else if (strcmp(interface_name, DEVICE_INTERFACE) == 0) {
			char *temp = malloc(strlen(object) + 1);
			strcpy(temp, object);
			dev_list = g_list_append(dev_list, temp);
		}

		g_free(interface_name);
		g_variant_iter_free(interface_content);
	}

	g_variant_iter_free(interfaces);
	g_free(object);


}

static void signal_interfaces_removed(GDBusConnection *conn,
		const gchar *sender,
		const gchar *path,
		const gchar *interface,
		const gchar *signal,
		GVariant *params,
		void *userdata)
{
	GVariantIter *interfaces;
	char *object, *interface_name;

	//	INFO("params type : %s\n", g_variant_get_type_string(params));
	//	INFO("params: %s\n", g_variant_print(params, TRUE));

	g_variant_get(params, "(oas)", &object, &interfaces);

	while (g_variant_iter_next(interfaces, "s", &interface_name)) {
		if (strcmp(interface_name, TRANSPORT_INTERFACE) == 0) {
			INFO("Media_Transport unregisterd\n");
			//memset(TRANSPORT_OBJECT, 0, sizeof(TRANSPORT_OBJECT));
		} else if (strcmp(interface_name, PLAYER_INTERFACE) == 0) {
			INFO("Media_Player unregisterd\n");
			//memset(PLAYER_OBJECT, 0, sizeof(PLAYER_OBJECT));
		} else if (strcmp(interface_name, CONTROL_INTERFACE) == 0) {
			//INFO("Media_Control unregisterd\n");
		} else if (strcmp(interface_name, DEVICE_INTERFACE) == 0) {
			GList *temp = g_list_first(dev_list);
			while (temp) {
				if (strcmp((char *)temp->data, object) == 0) {
					free((char *)temp->data);
					dev_list = g_list_remove(dev_list, temp->data);
					break;
				}
				temp = temp->next;
			}
		}

		g_free(interface_name);
	}

	g_variant_iter_free(interfaces);
	g_free(object);

}

static void signal_properties_changed(GDBusConnection *conn,
		const gchar *sender,
		const gchar *path,
		const gchar *interface,
		const gchar *signal,
		GVariant *params,
		void *userdata)
{
	GVariantIter *properties;
	GVariant *value;
	char *property, *interface_name, *status;

	g_variant_get(params, "(sa{sv}as)", &interface_name, &properties);

	//Media_Control properies handler
	if (strcmp(interface_name, CONTROL_INTERFACE) == 0) {
		while (g_variant_iter_next(properties, "{sv}", &property, &value)) {
			if (strcmp(property, "Connected") == 0) {
				g_variant_get(value, "b", &A2dpConnected);
				/*Possible value: TRUE, FALSE*/
				connect_call_back(A2dpConnected);
			}
			g_free(property);
			g_variant_unref(value);
		}
	}

	//Media_Player properies handler
	if (strcmp(interface_name, PLAYER_INTERFACE) == 0) {
		while (g_variant_iter_next(properties, "{sv}", &property, &value)) {
			if (strcmp(property, "Status") == 0) {
				g_variant_get(value, "s", &status);
				play_call_back(status);
			}
			g_free(property);
			g_variant_unref(value);
		}
	}

	g_free(interface_name);
	g_variant_iter_free(properties);
}

static int call_objManager_method(void)
{

	GVariant *result, *value;
	GError *error = NULL;

	GVariantIter *iter1, *iter2, *iter3;
	char *object, *interface_name, *property;
	gboolean status = FALSE;

	int ret = -1;
	if (NULL == conn) {
		INFO("No connection!! Please init first\n");
		return ret;
	}

	result = g_dbus_connection_call_sync(conn,
			"org.bluez",
			"/",
			"org.freedesktop.DBus.ObjectManager",
			"GetManagedObjects",
			NULL,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	if (result == NULL)
	{
		INFO("Error: %s\n", error->message);
		g_error_free (error);
		return ret;
	} else
		ret = 0;

	g_variant_get(result, "(a{oa{sa{sv}}})", &iter1);
	//g_variant_iter_init(result, &iter1);
	while (g_variant_iter_next(iter1, "{oa{sa{sv}}}", &object, &iter2)) {
		while (g_variant_iter_next(iter2, "{sa{sv}}", &interface_name, &iter3)) {
			//get connected status
			if (strcmp(interface_name, CONTROL_INTERFACE) == 0) {
				while (g_variant_iter_next(iter3, "{sv}", &property, &value)) {
					if (strcmp(property, "Connected") == 0) {
						g_variant_get(value, "b", &status);
						//this is the beginng check, only 'TRUE' would be reported
						if (TRUE == status) {
							connect_call_back(TRUE);
							A2dpConnected = TRUE;
						}
					}
					g_free(property);
					g_variant_unref(value);
				}
				//get object path
			} else if (strcmp(interface_name, TRANSPORT_INTERFACE) == 0) {
				memset(TRANSPORT_OBJECT, 0, sizeof(TRANSPORT_OBJECT));
				strncpy(TRANSPORT_OBJECT, object, sizeof(TRANSPORT_OBJECT) -1);
				INFO("Media_Transport registerd: %s\n", object);
			} else if (strcmp(interface_name, PLAYER_INTERFACE) == 0) {
				memset(PLAYER_OBJECT, 0, sizeof(PLAYER_OBJECT));
				strncpy(PLAYER_OBJECT, object, sizeof(PLAYER_OBJECT) - 1);
				INFO("Media_Player registerd: %s\n", object);
			} else if (strcmp(interface_name, ADAPTER_INTERFACE) == 0) {
				memset(ADAPTER_OBJECT, 0, sizeof(ADAPTER_OBJECT));
				strncpy(ADAPTER_OBJECT, object, strlen(object));
				INFO("Adapter registerd: %s\n", object);
			} else if (strcmp(interface_name, DEVICE_INTERFACE) == 0) {
				/*cache device from last scan*/
				char *temp = malloc(strlen(object) + 1);
				strcpy(temp, object);
				dev_list = g_list_append(dev_list, temp);
			}

			g_free(interface_name);
			g_variant_iter_free(iter3);
		}
		g_free(object);
		g_variant_iter_free(iter2);
	}

	g_variant_iter_free(iter1);
	g_variant_unref(result);
	return ret;
}

static void subscribe_signals(void)
{
	//we monitor interface added here
	ifa_signal_handle = g_dbus_connection_signal_subscribe(conn, "org.bluez", "org.freedesktop.DBus.ObjectManager",
			"InterfacesAdded", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
			signal_interfaces_added, NULL, NULL);

	//we monitor interface remove here
	ifr_signal_handle = g_dbus_connection_signal_subscribe(conn, "org.bluez", "org.freedesktop.DBus.ObjectManager",
			"InterfacesRemoved", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
			signal_interfaces_removed, NULL, NULL);

	//we monitor propertes changed here
	pch_signal_handle = g_dbus_connection_signal_subscribe(conn, "org.bluez", "org.freedesktop.DBus.Properties",
			"PropertiesChanged", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
			signal_properties_changed, NULL, NULL);

}

static void unsubscribe_signals(void)
{
	g_dbus_connection_signal_unsubscribe(conn, ifa_signal_handle);
	g_dbus_connection_signal_unsubscribe(conn, ifr_signal_handle);
	g_dbus_connection_signal_unsubscribe(conn, pch_signal_handle);

}

static void *dbus_thread(void *user_data)
{
	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

	return NULL;
}


int main(int argc, void **argv)
{
	int i, ret = 0;
	int timeout = 10;
	char *bddr = NULL;
	if (player_init())
		return 1;


	while (timeout > 0) {
		if (strncmp("/org/bluez", ADAPTER_OBJECT, 10) == 0)
			break;
		sleep(1);
		timeout--;
	}

	if (timeout == 0) {
		INFO("adapter not found!!, exit!!");
		return 1;
	}

	if (argc > 1) {

		if (argc > 3) {
			strcpy(device_mode, argv[3]);
			if (strcmp(device_mode, "central") != 0)
					strcpy(device_mode, "peripheral");
		}

		INFO("device_mode = %s\n", device_mode);

		if (strcmp(argv[1], "scan") == 0) {
			sleep(1);

			//remove_dev();

			adapter_scan(1);
			INFO("scanning....\n");

			timeout = 5;
			if (argc > 2)
				timeout = atoi(argv[2]);

			INFO("scan timeout = %ds\n", timeout);
			sleep(timeout);

			adapter_scan(0);
			GList *temp = g_list_first(dev_list);

			GVariant *g1, *g2, *g3, *g4;
			const gchar *addr, *name;
			gchar *uuids = NULL;
			guint class = 0;
			gsize uuid_len = 0;
			guint device_type = 0;
			while (temp) {
				g1 = get_property((char *)temp->data, DEVICE_INTERFACE, "Address");
				if (g1 != NULL)
					addr = g_variant_get_string(g1, NULL);
				else
					addr = "(null)";
				g2 = get_property((char *)temp->data, DEVICE_INTERFACE, "Name");
				if (g2 != NULL)
					name = g_variant_get_string(g2, NULL);
				else
					name = "(null)";
				g3 = get_property((char *)temp->data, DEVICE_INTERFACE, "Class");
				if (g3 != NULL)
					g_variant_get(g3, "u", &class);
				else
					class = 0;

				g4 = get_property((char *)temp->data, DEVICE_INTERFACE, "UUIDs");
				if (g4 != NULL) {
					const gchar **astr = g_variant_get_strv (g4, &uuid_len);
					uuids = malloc(uuid_len * (strlen(A2DP_SINK_UUID) + 3));
					memset(uuids, 0,  uuid_len * (strlen(A2DP_SINK_UUID) + 3));
					for (i = 0 ; i < uuid_len; i++)
						sprintf(uuids, "%s %s\n", uuids, astr[i]);
				} else {
					uuids = "(null)";
					uuid_len = 0;
				}

				DEBUG("Device Props: %s %s 0x%x\n%s\n", addr, name, class, uuids);

				if (strcmp(device_mode, "central") == 0) {
					if (strstr(uuids, A2DP_SINK_UUID) || ((class & COD_SERVICE_AUDIO) && (class & COD_SERVICE_RENDERING)))
						INFO("A2DP-SINK Device found: %s %s\n", addr, name);
				} else {
					if (strstr(uuids, A2DP_SOURCE_UUID) || (class & COD_SERVICE_CAPTUREING))
						INFO("A2DP-SOURCE Device found: %s %s\n", addr, name);
				}

				unref_variant(g1);
				unref_variant(g2);
				unref_variant(g3);
				unref_variant(g4);
				if (uuids != NULL) {
					free(uuids);
					uuids = NULL;
				}

				temp = temp->next;
			}
		} else if (strcmp(argv[1], "scanoff") == 0) {
			adapter_scan(0);
		} else if (strcmp(argv[1], "connect") == 0) {
			if (argc > 2) {
				bddr = argv[2];
				INFO("connec target : %s\n", bddr);
				bddr[2] = bddr[5] = bddr[8] = bddr[11] = bddr[14] = '_';

				ret = connect_dev(bddr);
				if (!ret)
					INFO("\n*********************\n* Device Connected! *\n*********************\n");

			} else {
				INFO("please set bddr!!");
				ret = 1;
			}
		} else if (strcmp(argv[1], "disconnect") == 0) {
			disconnect_dev();
		} else if (strcmp(argv[1], "status") == 0) {
			connect_status();
		}
	}

	player_delinit();

	return ret;

}
#if 0
int main(int argc, void *argv)
{
	int i;
	if (player_init())
		return 0;

	while (A2dpConnected == FALSE) {
		sleep(1);
		INFO("waiting for connection\n");
	}

	INFO("Device connected, start playing\n");
	start_play();
	sleep(10);
#if 0
	pause_play();
	sleep(2);
	next();
	sleep(2);
	previous();
	sleep(2);
	for (i = 0 ; i < 5; i++) {
		volume_down();
		sleep(1);
	}

	for (i = 0 ; i < 5; i++) {
		volume_up();
		sleep(1);
	}
#endif

	stop_play();
	sleep(2);

	player_delinit();

	return 0;

}
#endif
