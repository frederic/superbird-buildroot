/* * BlueALSA - rfcomm.c
 * Copyright (c) 2016-2017 Arkadiusz Bokowy
 *               2017 Juha Kuikka
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "rfcomm.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>

#include "bluealsa.h"
#include "ctl.h"
#include "utils.h"
#include "shared/log.h"
#include "shared/ctl-socket.h"


#define CTL_SOCKET_PATH "/etc/bluetooth/hfp_ctl_sk"
static pthread_t phfp_ctl_id;
static tAPP_SOCKET *hfp_ctl_sk = NULL;
static int callStatus = 0;
static int callSetupStatus = 0;
void hfp_ctl_send(int cmd, int value);
#define HFP_EVENT_CONNECTION 1
#define HFP_EVENT_CALL       2
#define HFP_EVENT_CALLSETUP  3
#define HFP_EVENT_VGS        4
#define HFP_EVENT_VGM        5
struct hfp_ctl_data {
	int fd;
	struct ba_transport *t;
};
static sem_t sem;

/**
 * Structure used for buffered reading from the RFCOMM. */
struct at_reader {
	struct bt_at at;
	char buffer[256];
	/* pointer to the next message within the buffer */
	char *next;
};

/**
 * Read AT message.
 *
 * Upon error it is required to set the next pointer of the reader structure
 * to NULL. Otherwise, this function might fail indefinitely.
 *
 * @param fd RFCOMM socket file descriptor.
 * @param reader Pointer to initialized reader structure.
 * @return On success this function returns 0. Otherwise, -1 is returned and
 *   errno is set to indicate the error. */
static int rfcomm_read_at(int fd, struct at_reader *reader) {

	char *buffer = reader->buffer;
	char *msg = reader->next;
	char *tmp;

	/* In case of reading more than one message from the RFCOMM, we have to
	 * parse all of them before we can read from the socket once more. */
	if (msg == NULL) {

		ssize_t len;

retry:
		if ((len = read(fd, buffer, sizeof(reader->buffer) -1)) == -1) {
			if (errno == EINTR)
				goto retry;
			return -1;
		}

		buffer[len] = '\0';
		msg = buffer;
	}

	/* parse AT message received from the RFCOMM */
	if ((tmp = at_parse(msg, &reader->at)) == NULL) {
		reader->next = msg;
		errno = EBADMSG;
		return -1;
	}

	reader->next = tmp[0] != '\0' ? tmp : NULL;
	return 0;
}

/**
 * Write AT message.
 *
 * @param fd RFCOMM socket file descriptor.
 * @param type Type of the AT message.
 * @param command AT command or response code.
 * @param value AT value or NULL if not applicable.
 * @return On success this function returns 0. Otherwise, -1 is returned and
 *   errno is set to indicate the error. */
static int rfcomm_write_at(int fd, enum bt_at_type type, const char *command,
		const char *value) {

	char msg[256];
	size_t len;

	debug("Sending AT message: %s: command:%s, value:%s",
			at_type2str(type), command, value);

	at_build(msg, type, command, value);
	len = strlen(msg);

retry:
	if (write(fd, msg, len) == -1) {
		if (errno == EINTR)
			goto retry;
		return -1;
	}

	return 0;
}

/**
 * HFP set state wrapper for debugging purposes. */
static void rfcomm_set_hfp_state(struct rfcomm_conn *c, enum hfp_state state) {
	debug("HFP state transition: %d -> %d", c->state, state);
	c->state = state;
}

/**
 * Handle AT command response code. */
static int rfcomm_handler_resp_ok_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	/* advance service level connection state */
	if (strcmp(at->value, "OK") == 0) {
		rfcomm_set_hfp_state(c, c->state + 1);
		return 0;
	}
	/* indicate caller that error has occurred */
	if (strcmp(at->value, "ERROR") == 0) {
		errno = ENOTSUP;
		return -1;
	}

	return 0;
}

/**
 * Handle AT command response code. */
static int rfcomm_handler_resp_vgs_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	hfp_ctl_send(HFP_EVENT_VGS, atoi(at->value));

	return 0;
}


/**
 * TEST: Standard indicator update AT command */
static int rfcomm_handler_cind_test_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	const int fd = c->t->bt_fd;

	if (rfcomm_write_at(fd, AT_TYPE_RESP, "+CIND",
				"(\"call\",(0,1))"
				",(\"callsetup\",(0-3))"
				",(\"callheld\",(0-2))") == -1)
		return -1;
	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_SLC_CIND_TEST_OK)
		rfcomm_set_hfp_state(c, HFP_SLC_CIND_TEST_OK);

	return 0;
}

/**
 * GET: Standard indicator update AT command */
static int rfcomm_handler_cind_get_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	const int fd = c->t->bt_fd;

	if (rfcomm_write_at(fd, AT_TYPE_RESP, "+CIND", "0,0,0") == -1)
		return -1;
	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_SLC_CIND_GET_OK)
		rfcomm_set_hfp_state(c, HFP_SLC_CIND_GET_OK);

	return 0;
}

/**
 * RESP: Standard indicator update AT command */
static int rfcomm_handler_cind_resp_test_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	/* parse response for the +CIND TEST command */
	if (at_parse_cind(at->value, c->hfp_ind_map) == -1)
		warn("Couldn't parse AG indicators");
	if (c->state < HFP_SLC_CIND_TEST)
		rfcomm_set_hfp_state(c, HFP_SLC_CIND_TEST);
	return 0;
}

/**
 * RESP: Standard indicator update AT command */
static int rfcomm_handler_cind_resp_get_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	char *tmp = at->value;
	size_t i;

	/* parse response for the +CIND GET command */
	for (i = 0; i < sizeof(c->hfp_ind_map) / sizeof(*c->hfp_ind_map); i++) {
		t->rfcomm.hfp_inds[c->hfp_ind_map[i]] = atoi(tmp);
		if (c->hfp_ind_map[i] == HFP_IND_BATTCHG)
			device_set_battery_level(t->device, atoi(tmp) * 100 / 5);

		if (c->hfp_ind_map[i] == HFP_IND_CALL)
			callStatus = atoi(tmp);

		if (c->hfp_ind_map[i] == HFP_IND_CALLSETUP)
			callSetupStatus = atoi(tmp);

		if ((tmp = strchr(tmp, ',')) == NULL)
			break;
		tmp += 1;
	}

	if (c->state < HFP_SLC_CIND_GET)
		rfcomm_set_hfp_state(c, HFP_SLC_CIND_GET);

	return 0;
}

/**
 * SET: Standard event reporting activation/deactivation AT command */
static int rfcomm_handler_cmer_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	const int fd = c->t->bt_fd;

	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_SLC_CMER_SET_OK)
		rfcomm_set_hfp_state(c, HFP_SLC_CMER_SET_OK);

	return 0;
}

/**
 * RESP: Standard indicator events reporting unsolicited result code */
static int rfcomm_handler_ciev_resp_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	unsigned int index;
	unsigned int value;

	if (sscanf(at->value, "%u,%u", &index, &value) == 2) {
		t->rfcomm.hfp_inds[c->hfp_ind_map[index - 1]] = value;
		switch (c->hfp_ind_map[index - 1]) {
		case HFP_IND_CALL:
			eventfd_write(t->rfcomm.sco->event_fd, 1);
			hfp_ctl_send(HFP_EVENT_CALL, value);
			callStatus = value;
			break;
		case HFP_IND_CALLSETUP:
			eventfd_write(t->rfcomm.sco->event_fd, 1);
			hfp_ctl_send(HFP_EVENT_CALLSETUP, value);
			callSetupStatus = value;
			break;
		case HFP_IND_BATTCHG:
			device_set_battery_level(t->device, value * 100 / 5);
			break;
		default:
			break;
		}
	}

	return 0;
}

/**
 * SET: Bluetooth Indicators Activation */
static int rfcomm_handler_bia_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	/* We are not sending any indicators to the HF, however support for the
	 * +BIA command is mandatory for the AG, so acknowledge the message. */
	if (rfcomm_write_at(c->t->bt_fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;
	return 0;
}

/**
 * SET: Bluetooth Retrieve Supported Features */
static int rfcomm_handler_brsf_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	const int fd = t->bt_fd;
	char tmp[16];

	t->rfcomm.hfp_features = atoi(at->value);

	/* Codec negotiation is not supported in the HF, hence no
	 * wideband audio support. AT+BAC will not be sent. */
	if (!(t->rfcomm.hfp_features & HFP_HF_FEAT_CODEC))
		t->rfcomm.sco->codec = HFP_CODEC_CVSD;

	sprintf(tmp, "%u", BA_HFP_AG_FEATURES);
	if (rfcomm_write_at(fd, AT_TYPE_RESP, "+BRSF", tmp) == -1)
		return -1;
	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_SLC_BRSF_SET_OK)
		rfcomm_set_hfp_state(c, HFP_SLC_BRSF_SET_OK);

	return 0;
}

/**
 * RESP: Bluetooth Retrieve Supported Features */
static int rfcomm_handler_brsf_resp_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	t->rfcomm.hfp_features = atoi(at->value);

	/* codec negotiation is not supported in the AG */
	if (!(t->rfcomm.hfp_features & HFP_AG_FEAT_CODEC))
		t->rfcomm.sco->codec = HFP_CODEC_CVSD;

	if (c->state < HFP_SLC_BRSF_SET)
		rfcomm_set_hfp_state(c, HFP_SLC_BRSF_SET);

	return 0;
}

/**
 * SET: Gain of Microphone */
static int rfcomm_handler_vgm_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	struct ba_transport * const t = c->t;
	const int fd = t->bt_fd;

	t->rfcomm.sco->sco.mic_gain = c->mic_gain = atoi(at->value);
	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	bluealsa_ctl_event(EVENT_UPDATE_VOLUME);
	return 0;
}

/**
 * SET: Gain of Speaker */
static int rfcomm_handler_vgs_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	struct ba_transport * const t = c->t;
	const int fd = t->bt_fd;

	t->rfcomm.sco->sco.spk_gain = c->spk_gain = atoi(at->value);
	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	bluealsa_ctl_event(EVENT_UPDATE_VOLUME);
	return 0;
}

/**
 * SET: Bluetooth Response and Hold Feature */
static int rfcomm_handler_btrh_get_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	/* Currently, we are not supporting Respond & Hold feature, so just
	 * acknowledge this GET request without reporting +BTRH status. */
	if (rfcomm_write_at(c->t->bt_fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;
	return 0;
}

/**
 * SET: Bluetooth Codec Selection */
static int rfcomm_handler_bcs_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	const int fd = t->bt_fd;

	if (t->rfcomm.sco->codec != atoi(at->value)) {
		warn("Codec not acknowledged: %d != %s", t->rfcomm.sco->codec, at->value);
		if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "ERROR") == -1)
			return -1;
		return 0;
	}

	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_CC_BCS_SET_OK)
		rfcomm_set_hfp_state(c, HFP_CC_BCS_SET_OK);

	return 0;
}

static int rfcomm_handler_resp_bcs_ok_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	if (rfcomm_handler_resp_ok_cb(c, at) != 0)
		return -1;
	/* When codec selection is completed, notify connected clients, that
	 * transport has been changed. Note, that this event might be emitted
	 * for an active transport - codec switching. */
	bluealsa_ctl_event(EVENT_TRANSPORT_CHANGED);
	return 0;
}

/**
 * RESP: Bluetooth Codec Selection */
static int rfcomm_handler_bcs_resp_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	static const struct rfcomm_handler handler = {
		AT_TYPE_RESP, "", rfcomm_handler_resp_bcs_ok_cb };
	struct ba_transport * const t = c->t;
	const int fd = t->bt_fd;

	t->rfcomm.sco->codec = atoi(at->value);
	if (rfcomm_write_at(fd, AT_TYPE_CMD_SET, "+BCS", at->value) == -1)
		return -1;

	c->handler = &handler;

	if (c->state < HFP_CC_BCS_SET)
		rfcomm_set_hfp_state(c, HFP_CC_BCS_SET);

	return 0;
}

/**
 * SET: Bluetooth Available Codecs */
static int rfcomm_handler_bac_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {
	(void)at;
	const int fd = c->t->bt_fd;

	/* In case some headsets send BAC even if we don't advertise
	 * support for it. In such case, just OK and ignore. */

	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;

	if (c->state < HFP_SLC_BAC_SET_OK)
		rfcomm_set_hfp_state(c, HFP_SLC_BAC_SET_OK);

	return 0;
}

/**
 * SET: Apple Ext: Report a headset state change */
static int rfcomm_handler_iphoneaccev_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	struct ba_device * const d = t->device;
	const int fd = t->bt_fd;

	char *ptr = at->value;
	size_t count = atoi(strsep(&ptr, ","));
	char tmp;

	while (count-- && ptr != NULL)
		switch (tmp = *strsep(&ptr, ",")) {
		case '1':
			if (ptr != NULL)
				device_set_battery_level(d, atoi(strsep(&ptr, ",")) * 100 / 9);
			break;
		case '2':
			if (ptr != NULL)
				d->xapl.accev_docked = atoi(strsep(&ptr, ","));
			break;
		default:
			warn("Unsupported IPHONEACCEV key: %c", tmp);
			strsep(&ptr, ",");
		}

	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, "OK") == -1)
		return -1;
	return 0;
}

/**
 * SET: Apple Ext: Enable custom AT commands from an accessory */
static int rfcomm_handler_xapl_set_cb(struct rfcomm_conn *c, const struct bt_at *at) {

	struct ba_transport * const t = c->t;
	struct ba_device * const d = t->device;
	const int fd = t->bt_fd;

	const char *resp = "+XAPL=BlueALSA,0";
	unsigned int vendor, product;
	unsigned int version, features;

	if (sscanf(at->value, "%x-%x-%u,%u", &vendor, &product, &version, &features) == 4) {
		d->xapl.vendor_id = vendor;
		d->xapl.product_id = product;
		d->xapl.version = version;
		d->xapl.features = features;
	}
	else {
		warn("Invalid XAPL value: %s", at->value);
		resp = "ERROR";
	}

	if (rfcomm_write_at(fd, AT_TYPE_RESP, NULL, resp) == -1)
		return -1;
	return 0;
}

static const struct rfcomm_handler rfcomm_handler_resp_ok = {
	AT_TYPE_RESP, "", rfcomm_handler_resp_ok_cb };
static const struct rfcomm_handler rfcomm_handler_cind_test = {
	AT_TYPE_CMD_TEST, "+CIND", rfcomm_handler_cind_test_cb };
static const struct rfcomm_handler rfcomm_handler_cind_get = {
	AT_TYPE_CMD_GET, "+CIND", rfcomm_handler_cind_get_cb };
static const struct rfcomm_handler rfcomm_handler_cind_resp_test = {
	AT_TYPE_RESP, "+CIND", rfcomm_handler_cind_resp_test_cb };
static const struct rfcomm_handler rfcomm_handler_cind_resp_get = {
	AT_TYPE_RESP, "+CIND", rfcomm_handler_cind_resp_get_cb };
static const struct rfcomm_handler rfcomm_handler_cmer_set = {
	AT_TYPE_CMD_SET, "+CMER", rfcomm_handler_cmer_set_cb };
static const struct rfcomm_handler rfcomm_handler_ciev_resp = {
	AT_TYPE_RESP, "+CIEV", rfcomm_handler_ciev_resp_cb };
static const struct rfcomm_handler rfcomm_handler_bia_set = {
	AT_TYPE_CMD_SET, "+BIA", rfcomm_handler_bia_set_cb };
static const struct rfcomm_handler rfcomm_handler_brsf_set = {
	AT_TYPE_CMD_SET, "+BRSF", rfcomm_handler_brsf_set_cb };
static const struct rfcomm_handler rfcomm_handler_brsf_resp = {
	AT_TYPE_RESP, "+BRSF", rfcomm_handler_brsf_resp_cb };
static const struct rfcomm_handler rfcomm_handler_vgm_set = {
	AT_TYPE_CMD_SET, "+VGM", rfcomm_handler_vgm_set_cb };
static const struct rfcomm_handler rfcomm_handler_vgs_set = {
	AT_TYPE_CMD_SET, "+VGS", rfcomm_handler_vgs_set_cb };
static const struct rfcomm_handler rfcomm_handler_resp_vgs = {
	AT_TYPE_RESP, "+VGS", rfcomm_handler_resp_vgs_cb };
static const struct rfcomm_handler rfcomm_handler_btrh_get = {
	AT_TYPE_CMD_GET, "+BTRH", rfcomm_handler_btrh_get_cb };
static const struct rfcomm_handler rfcomm_handler_bcs_set = {
	AT_TYPE_CMD_SET, "+BCS", rfcomm_handler_bcs_set_cb };
static const struct rfcomm_handler rfcomm_handler_bcs_resp = {
	AT_TYPE_RESP, "+BCS", rfcomm_handler_bcs_resp_cb };
static const struct rfcomm_handler rfcomm_handler_bac_set = {
	AT_TYPE_CMD_SET, "+BAC", rfcomm_handler_bac_set_cb };
static const struct rfcomm_handler rfcomm_handler_iphoneaccev_set = {
	AT_TYPE_CMD_SET, "+IPHONEACCEV", rfcomm_handler_iphoneaccev_set_cb };
static const struct rfcomm_handler rfcomm_handler_xapl_set = {
	AT_TYPE_CMD_SET, "+XAPL", rfcomm_handler_xapl_set_cb };

/**
 * Get callback (if available) for given AT message. */
static rfcomm_callback *rfcomm_get_callback(const struct bt_at *at) {

	static const struct rfcomm_handler *handlers[] = {
		&rfcomm_handler_cind_test,
		&rfcomm_handler_cind_get,
		&rfcomm_handler_cmer_set,
		&rfcomm_handler_ciev_resp,
		&rfcomm_handler_bia_set,
		&rfcomm_handler_brsf_set,
		&rfcomm_handler_vgm_set,
		&rfcomm_handler_vgs_set,
		&rfcomm_handler_resp_vgs,
		&rfcomm_handler_btrh_get,
		&rfcomm_handler_bcs_set,
		&rfcomm_handler_bcs_resp,
		&rfcomm_handler_bac_set,
		&rfcomm_handler_iphoneaccev_set,
		&rfcomm_handler_xapl_set,
	};

	size_t i;

	for (i = 0; i < sizeof(handlers) / sizeof(*handlers); i++) {
		if (handlers[i]->type != at->type)
			continue;
		if (strcmp(handlers[i]->command, at->command) != 0)
			continue;
		return handlers[i]->callback;
	}

	return NULL;
}


/*hfp_ctl_send return 0 if success*/
void hfp_ctl_send(int cmd, int value)
{
	char msg[2];
	msg[0] = cmd;
	msg[1] = value;

	debug("hfp_ctl_server: sending msg: %d %d", cmd, value);
	send_all_client(hfp_ctl_sk, msg, sizeof(msg));

}

static void *hfp_ctl_client_handler(void *arg)
{
	struct hfp_ctl_data *phcd = (struct hfp_ctl_data *)arg;
	struct ba_transport *t = phcd->t;
	int fd = phcd->fd;
	char msg[64];
	char hello[2] = {HFP_EVENT_CONNECTION, 1};
	int bytes;

	/*once hcd data copied, increase the sem*/
	sem_post(&sem);

	send(fd, hello, 2, 0);

	while (1) {
		memset(msg, 0, sizeof(msg));
		bytes = recv(fd, msg, sizeof(msg), 0);
		if (bytes < 0) {
			perror("hfp_ctl_client_handler");
			continue;
		}
		if (bytes == 0) {
			debug("hfp_ctl_server: client fd%d off line", fd);
			break;
		}
		debug("hfp_ctl_server recv: %s from fd%d", msg, fd);
		rfcomm_write_at(t->bt_fd, AT_TYPE_CMD, msg, NULL);
	}

	//release resource and exit thread
	close(fd);
	remove_one_client(hfp_ctl_sk, fd);
	pthread_detach(pthread_self());
	return NULL;
}

static void *hfp_ctl_cb(void *arg){
	struct hfp_ctl_data hcd;
	pthread_t pid = 0;
	int  fd;


	sem_wait(&sem);

	hfp_ctl_sk = setup_socket_server(CTL_SOCKET_PATH);
	if (hfp_ctl_sk == 0) {
		error("%s", __func__);
		sem_post(&sem);
		return NULL;
	}

	while (1) {

init:
		fd = accept_client(hfp_ctl_sk);
		/*some errors happen, try again*/
		if (fd == -1) {
			/*if accept fail and err invalid arg, jump out and stop the thread*/
			if (errno == EINVAL)
				break;

			goto init;
		}

		debug("%s: new connected client: fd%d", __func__, fd);

		hcd.fd = fd;
		hcd.t  = (struct ba_transport *)arg;

		if (pthread_create(&pid, NULL, hfp_ctl_client_handler, (void *)&hcd)) {
			sem_post(&sem);
			perror("hfp_ctl_cb thread:");
		} else
			pthread_setname_np(pid, "hfp_ctl_sub");

		/*incase hcd would be recover!!*/
		sem_wait(&sem);

		pid = 0;
		fd = 0;
	}
	pthread_detach(pthread_self());
	//sleep(2);
	teardown_socket_server(hfp_ctl_sk);
	sem_post(&sem);

	return NULL;
}


void hfp_ctl_init(void *arg)
{
	static int firstrun = 1;
	debug("%s", __func__);

	if (firstrun) {
		/*semaphore, share with threads, initial value 1*/
		if (sem_init(&sem, 0, 1)) {
			error("%s", strerror(errno));
			return;
		}
		firstrun = 0;
	}

#if 0
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
#endif

	if (pthread_create(&phfp_ctl_id, NULL, hfp_ctl_cb, arg))
		perror("hfp_ctl_thread");
	else
		pthread_setname_np(phfp_ctl_id, "hfp_ctl_server");
}

void hfp_ctl_delinit(void)
{
	debug("%s", __func__);
	hfp_ctl_send(HFP_EVENT_CONNECTION, 0);
	shutdown(hfp_ctl_sk->server_sockfd, SHUT_RD);
	shutdown_all_client(hfp_ctl_sk);
}


void *rfcomm_thread(void *arg) {
	struct ba_transport *t = (struct ba_transport *)arg;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_push(transport_pthread_cleanup, t);

	/* initialize structure used for synchronization */
	struct rfcomm_conn conn = {
		.state = HFP_DISCONNECTED,
		.state_prev = HFP_DISCONNECTED,
		.mic_gain = t->rfcomm.sco->sco.mic_gain,
		.spk_gain = t->rfcomm.sco->sco.spk_gain,
		.t = t,
	};

	struct at_reader reader = { .next = NULL };
	struct pollfd pfds[] = {
		{ t->event_fd, POLLIN, 0 },
		{ t->bt_fd, POLLIN, 0 },
	};

	debug("Starting RFCOMM loop: %s",
			bluetooth_profile_to_string(t->profile, t->codec));
	for (;;) {

		/* During normal operation, RFCOMM should block indefinitely. However,
		 * in the HFP-HF mode, service level connection has to be initialized
		 * by ourself. In order to do this reliably, we have to assume, that
		 * AG might not receive our message and will not send proper response.
		 * Hence, we will incorporate timeout, after which we will send our
		 * AT command once more. */
		int timeout = -1;

		rfcomm_callback *callback;
		char tmp[16];

		if (conn.state != HFP_CONNECTED) {

			/* If some progress has been made in the SLC procedure, reset the
			 * retries counter. */
			if (conn.state != conn.state_prev) {
				conn.state_prev = conn.state;
				conn.retries = 0;
			}

			/* If the maximal number of retries has been reached, terminate the
			 * connection. Trying indefinitely will only use up our resources. */
			if (conn.retries > RFCOMM_SLC_RETRIES) {
				errno = ETIMEDOUT;
				goto ioerror;
			}

			if (t->profile == BLUETOOTH_PROFILE_HFP_HF)
				switch (conn.state) {
				case HFP_DISCONNECTED:
					sprintf(tmp, "%u", BA_HFP_HF_FEATURES);
					if (rfcomm_write_at(pfds[1].fd, AT_TYPE_CMD_SET, "+BRSF", tmp) == -1)
						goto ioerror;
					conn.handler = &rfcomm_handler_brsf_resp;
					break;
				case HFP_SLC_BRSF_SET:
					conn.handler = &rfcomm_handler_resp_ok;
					break;
				case HFP_SLC_BRSF_SET_OK:
					if (t->rfcomm.hfp_features & HFP_AG_FEAT_CODEC) {
						if (rfcomm_write_at(pfds[1].fd, AT_TYPE_CMD_SET, "+BAC", "1") == -1)
							goto ioerror;
						conn.handler = &rfcomm_handler_resp_ok;
						break;
					}
				case HFP_SLC_BAC_SET_OK:
					if (rfcomm_write_at(pfds[1].fd, AT_TYPE_CMD_TEST, "+CIND", NULL) == -1)
						goto ioerror;
					conn.handler = &rfcomm_handler_cind_resp_test;
					break;
				case HFP_SLC_CIND_TEST:
					conn.handler = &rfcomm_handler_resp_ok;
					break;
				case HFP_SLC_CIND_TEST_OK:
					if (rfcomm_write_at(pfds[1].fd, AT_TYPE_CMD_GET, "+CIND", NULL) == -1)
						goto ioerror;
					conn.handler = &rfcomm_handler_cind_resp_get;
					break;
				case HFP_SLC_CIND_GET:
					conn.handler = &rfcomm_handler_resp_ok;
					break;
				case HFP_SLC_CIND_GET_OK:
					/* Activate indicator events reporting. The +CMER specification is
					 * as follows: AT+CMER=[<mode>[,<keyp>[,<disp>[,<ind>[,<bfr>]]]]] */
					if (rfcomm_write_at(pfds[1].fd, AT_TYPE_CMD_SET, "+CMER", "3,0,0,1,0") == -1)
						goto ioerror;
					conn.handler = &rfcomm_handler_resp_ok;
					break;
				case HFP_SLC_CMER_SET_OK:
					rfcomm_set_hfp_state(&conn, HFP_SLC_CONNECTED);
				case HFP_SLC_CONNECTED:
					/*only fully connected can we report call status & callsetup*/
					if (callStatus)
						hfp_ctl_send(HFP_EVENT_CALL, callStatus);

					if (callSetupStatus)
						hfp_ctl_send(HFP_EVENT_CALLSETUP, callSetupStatus);

					callStatus = 0;
					callSetupStatus = 0;

					if (t->rfcomm.hfp_features & HFP_AG_FEAT_CODEC)
						break;
				case HFP_CC_BCS_SET:
				case HFP_CC_BCS_SET_OK:
				case HFP_CC_CONNECTED:
					rfcomm_set_hfp_state(&conn, HFP_CONNECTED);
				case HFP_CONNECTED:
					bluealsa_ctl_event(EVENT_TRANSPORT_ADDED);
				}

			if (t->profile == BLUETOOTH_PROFILE_HFP_AG)
				switch (conn.state) {
				case HFP_DISCONNECTED:
				case HFP_SLC_BRSF_SET:
				case HFP_SLC_BRSF_SET_OK:
				case HFP_SLC_BAC_SET_OK:
				case HFP_SLC_CIND_TEST:
				case HFP_SLC_CIND_TEST_OK:
				case HFP_SLC_CIND_GET:
				case HFP_SLC_CIND_GET_OK:
					break;
				case HFP_SLC_CMER_SET_OK:
					rfcomm_set_hfp_state(&conn, HFP_SLC_CONNECTED);
				case HFP_SLC_CONNECTED:
					if (t->rfcomm.hfp_features & HFP_HF_FEAT_CODEC) {
						if (rfcomm_write_at(pfds[1].fd, AT_TYPE_RESP, "+BCS", "1") == -1)
							goto ioerror;
						t->rfcomm.sco->codec = HFP_CODEC_CVSD;
						conn.handler = &rfcomm_handler_bcs_set;
						break;
					}
				case HFP_CC_BCS_SET:
				case HFP_CC_BCS_SET_OK:
				case HFP_CC_CONNECTED:
					rfcomm_set_hfp_state(&conn, HFP_CONNECTED);
				case HFP_CONNECTED:
					bluealsa_ctl_event(EVENT_TRANSPORT_ADDED);
				}

			if (conn.handler != NULL) {
				timeout = RFCOMM_SLC_TIMEOUT;
				conn.retries++;
			}

		}

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		switch (poll(pfds, sizeof(pfds) / sizeof(*pfds), timeout)) {
		case 0:
			debug("RFCOMM poll timeout");
			continue;
		case -1:
			error("RFCOMM poll error: %s", strerror(errno));
			goto fail;
		}

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		if (pfds[0].revents & POLLIN) {
			/* dispatch incoming event */

			eventfd_t event;
			eventfd_read(pfds[0].fd, &event);

			char tmp[16];

			if (conn.mic_gain != t->rfcomm.sco->sco.mic_gain) {
				int gain = conn.mic_gain = t->rfcomm.sco->sco.mic_gain;
				debug("Setting microphone gain: %d", gain);
				sprintf(tmp, "+VGM=%d", gain);
				if (rfcomm_write_at(pfds[1].fd, AT_TYPE_RESP, NULL, tmp) == -1)
					goto ioerror;
			}
			if (conn.spk_gain != t->rfcomm.sco->sco.spk_gain) {
				int gain = conn.spk_gain = t->rfcomm.sco->sco.spk_gain;
				debug("Setting speaker gain: %d", gain);
				sprintf(tmp, "+VGS=%d", gain);
				if (rfcomm_write_at(pfds[1].fd, AT_TYPE_RESP, NULL, tmp) == -1)
					goto ioerror;
			}

		}

		if (pfds[1].revents & POLLIN) {
			/* read data from the RFCOMM */

read:
			if (rfcomm_read_at(pfds[1].fd, &reader) == -1)
				switch (errno) {
				case EBADMSG:
					warn("Invalid AT message: %s", reader.next);
					reader.next = NULL;
					continue;
				default:
					goto ioerror;
				}

			/* use predefined callback, otherwise get generic one */
			if (conn.handler != NULL && conn.handler->type == reader.at.type &&
					strcmp(conn.handler->command, reader.at.command) == 0) {
				callback = conn.handler->callback;
				conn.handler = NULL;
			}
			else
				callback = rfcomm_get_callback(&reader.at);

			if (callback != NULL) {
				if (callback(&conn, &reader.at) == -1)
					goto ioerror;
			}
			else {
				warn("Unsupported AT message: %s: command:%s, value:%s",
						at_type2str(reader.at.type), reader.at.command, reader.at.value);
				if (reader.at.type != AT_TYPE_RESP)
					if (rfcomm_write_at(pfds[1].fd, AT_TYPE_RESP, NULL, "ERROR") == -1)
						goto ioerror;
			}

			if (reader.next != NULL)
				goto read;

		}
		else if (pfds[1].revents & (POLLERR | POLLHUP)) {
			errno = ECONNRESET;
			goto ioerror;
		}

		continue;

ioerror:
		switch (errno) {
		case ECONNABORTED:
		case ECONNRESET:
		case ENOTCONN:
		case ENOTSUP:
		case ETIMEDOUT:
			/* exit the thread upon socket disconnection */
			debug("RFCOMM disconnected: %s", strerror(errno));
			goto fail;
		default:
			error("RFCOMM IO error: %s", strerror(errno));
		}
	}

fail:
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_cleanup_pop(1);
	return NULL;
}
