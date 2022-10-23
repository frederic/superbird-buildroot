/** @file moal_cfgvendor.c
  *
  * @brief This file contains the functions for CFG80211 vendor.
  *
  * Copyright (C) 2011-2017, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */

#include    "moal_cfgvendor.h"
#include    "moal_cfg80211.h"

/********************************************************
				Local Variables
********************************************************/

/********************************************************
				Global Variables
********************************************************/
#if CFG80211_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
extern int dfs_offload;
#endif
/********************************************************
				Local Functions
********************************************************/

/********************************************************
				Global Functions
********************************************************/

#if CFG80211_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/**marvell vendor command and event*/
#define MRVL_VENDOR_ID  0x005043
/** vendor events */
const struct nl80211_vendor_cmd_info vendor_events[] = {
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_hang,},	/*event_id 0 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_set_key_mgmt_offload,},	/*event_id 0x10001 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = fw_roam_success,},	/*event_id 0x10002 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_dfs_radar_detected,},	/*event_id 0x10004 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_dfs_cac_started,},	/*event_id 0x10005 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_dfs_cac_finished,},	/*event_id 0x10006 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_dfs_cac_aborted,},	/*event_id 0x10007 */
	{.vendor_id = MRVL_VENDOR_ID,.subcmd = event_dfs_nop_finished,},	/*event_id 0x10008 */
	/**add vendor event here*/
};

/**
 * @brief get the event id of the events array
 *
 * @param event     vendor event
 *
 * @return    index of events array
 */
int
woal_get_event_id(int event)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(vendor_events); i++) {
		if (vendor_events[i].subcmd == event)
			return i;
	}

	return event_max;
}

/**
 * @brief send vendor event to kernel
 *
 * @param priv       A pointer to moal_private
 * @param event    vendor event
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
int
woal_cfg80211_vendor_event(IN moal_private *priv,
			   IN int event, IN t_u8 *data, IN int len)
{
	struct wiphy *wiphy = NULL;
	struct sk_buff *skb = NULL;
	int event_id = 0;
	t_u8 *pos = NULL;
	int ret = 0;

	ENTER();

	if (!priv || !priv->wdev || !priv->wdev->wiphy) {
		LEAVE();
		return ret;
	}
	wiphy = priv->wdev->wiphy;
	PRINTM(MEVENT, "vendor event :0x%x\n", event);
	event_id = woal_get_event_id(event);
	if (event_max == event_id) {
		PRINTM(MERROR, "Not find this event %d \n", event_id);
		ret = 1;
		LEAVE();
		return ret;
	}

	/**allocate skb*/
#if CFG80211_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, NULL, len, event_id,
					  GFP_ATOMIC);
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len, event_id, GFP_ATOMIC);
#endif

	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor event\n");
		ret = 1;
		LEAVE();
		return ret;
	}
	pos = skb_put(skb, len);
	memcpy(pos, data, len);
	/**send event*/
	cfg80211_vendor_event(skb, GFP_ATOMIC);

	LEAVE();
	return ret;
}

#if CFG80211_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/**
 * @brief send dfs vendor event to kernel
 *
 * @param priv       A pointer to moal_private
 * @param event      dfs vendor event
 * @param chandef    a pointer to struct cfg80211_chan_def
 *
 * @return      N/A
 */
void
woal_cfg80211_dfs_vendor_event(moal_private *priv, int event,
			       struct cfg80211_chan_def *chandef)
{
	dfs_event evt;
	ENTER();
	if (!chandef) {
		LEAVE();
		return;
	}
	memset(&evt, 0, sizeof(dfs_event));
	evt.freq = chandef->chan->center_freq;
	evt.chan_width = chandef->width;
	evt.cf1 = chandef->center_freq1;
	evt.cf2 = chandef->center_freq2;
	switch (chandef->width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
		evt.ht_enabled = 0;
		break;
	case NL80211_CHAN_WIDTH_20:
		evt.ht_enabled = 1;
		break;
	case NL80211_CHAN_WIDTH_40:
		evt.ht_enabled = 1;
		if (chandef->center_freq1 < chandef->chan->center_freq)
			evt.chan_offset = -1;
		else
			evt.chan_offset = 1;
		break;
	case NL80211_CHAN_WIDTH_80:
	case NL80211_CHAN_WIDTH_80P80:
	case NL80211_CHAN_WIDTH_160:
		evt.ht_enabled = 1;
		break;
	default:
		break;
	}
	woal_cfg80211_vendor_event(priv, event, (t_u8 *)&evt,
				   sizeof(dfs_event));
	LEAVE();
	return;
}
#endif

/**
 * @brief vendor command to set drvdbg
 *
 * @param wiphy       A pointer to wiphy struct
 * @param wdev     A pointer to wireless_dev struct
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
static int
woal_cfg80211_subcmd_set_drvdbg(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
#ifdef DEBUG_LEVEL1
	struct net_device *dev = wdev->netdev;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	struct sk_buff *skb = NULL;
	t_u8 *pos = NULL;
#endif
	int ret = 1;

	ENTER();
#ifdef DEBUG_LEVEL1
	/**handle this sub command*/
	DBG_HEXDUMP(MCMD_D, "Vendor drvdbg", (t_u8 *)data, data_len);

	if (data_len) {
		/* Get the driver debug bit masks from user */
		drvdbg = *((t_u32 *)data);
		PRINTM(MIOCTL, "new drvdbg %x\n", drvdbg);
		/* Set the driver debug bit masks into mlan */
		if (woal_set_drvdbg(priv, drvdbg)) {
			PRINTM(MERROR, "Set drvdbg failed!\n");
			ret = 1;
		}
	}
	/** Allocate skb for cmd reply*/
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(drvdbg));
	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor cmd\n");
		ret = 1;
		LEAVE();
		return ret;
	}
	pos = skb_put(skb, sizeof(drvdbg));
	memcpy(pos, &drvdbg, sizeof(drvdbg));
	ret = cfg80211_vendor_cmd_reply(skb);
#endif
	LEAVE();
	return ret;
}

/**
 * @brief vendor command to get correlated HW and System time
 *
 * @param wiphy    A pointer to wiphy struct
 * @param wdev     A pointer to wireless_dev struct
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  -1: fail
 */
static int
woal_cfg80211_subcmd_get_correlated_time(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int len)
{
	struct net_device *dev = wdev->netdev;
	moal_private *priv = (moal_private *)woal_get_netdev_priv(dev);
	struct sk_buff *skb = NULL;
	mlan_ioctl_req *req = NULL;
	mlan_ds_get_correlated_time *info = NULL;
	mlan_status status = MLAN_STATUS_SUCCESS;
	int err = -1;
	int length = 0;

	/* Allocate an IOCTL request buffer */
	req = woal_alloc_mlan_ioctl_req(sizeof(t_u32) +
					sizeof(mlan_ds_get_correlated_time));
	if (req == NULL) {
		PRINTM(MERROR, "Could not allocate mlan ioctl request!\n");
		return -ENOMEM;
	}

	/* Fill request buffer */
	info = (mlan_ds_get_correlated_time *) req->pbuf;
	req->req_id = MLAN_IOCTL_GET_CORRELATED_TIME;
	req->action = MLAN_ACT_GET;

	/* Send IOCTL request to MLAN */
	status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
	if (status != MLAN_STATUS_SUCCESS) {
		PRINTM(MERROR, "get correleted time fail\n");
		goto done;
	}

	length = sizeof(mlan_ds_get_correlated_time);

	DBG_HEXDUMP(MCMD_D, "get_correlated_time", (t_u8 *)info, length);

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, length);
	if (unlikely(!skb)) {
		PRINTM(MERROR, "skb alloc failed\n");
		goto done;
	}

	/* Push the data to the skb */
	nla_put_nohdr(skb, length, info);

	err = cfg80211_vendor_cmd_reply(skb);
	if (unlikely(err)) {
		PRINTM(MERROR, "Vendor Command reply failed ret:%d \n", err);
	}

done:
	if (status != MLAN_STATUS_PENDING)
		kfree(req);
	return err;
}

/**
 * @brief vendor command to key_mgmt_set_key
 *
 * @param wiphy    A pointer to wiphy struct
 * @param wdev     A pointer to wireless_dev struct
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  fail otherwise
 */
static int
woal_cfg80211_subcmd_set_roaming_offload_key(struct wiphy *wiphy,
					     struct wireless_dev *wdev,
					     const void *data, int data_len)
{
	moal_private *priv;
	struct net_device *dev;
	struct sk_buff *skb = NULL;
	t_u8 *pos = (t_u8 *)data;
	int ret = MLAN_STATUS_SUCCESS;

	ENTER();

	if (data)
		DBG_HEXDUMP(MCMD_D, "Vendor pmk", (t_u8 *)data, data_len);

	if (!wdev || !wdev->netdev) {
		LEAVE();
		return -EFAULT;
	}

	dev = wdev->netdev;
	priv = (moal_private *)woal_get_netdev_priv(dev);
	if (!priv) {
		LEAVE();
		return -EFAULT;
	}

	if (data_len > MLAN_MAX_KEY_LENGTH) {
		memcpy(&priv->pmk.pmk_r0, pos, MLAN_MAX_KEY_LENGTH);
		pos += MLAN_MAX_KEY_LENGTH;
		memcpy(&priv->pmk.pmk_r0_name, pos,
		       MIN(MLAN_MAX_PMKR0_NAME_LENGTH,
			   data_len - MLAN_MAX_KEY_LENGTH));
	} else {
		memcpy(&priv->pmk.pmk, data,
		       MIN(MLAN_MAX_KEY_LENGTH, data_len));
	}
	priv->pmk_saved = MTRUE;

    /** Allocate skb for cmd reply*/
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, data_len);
	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor cmd\n");
		LEAVE();
		return -EFAULT;
	}
	pos = skb_put(skb, data_len);
	memcpy(pos, data, data_len);
	ret = cfg80211_vendor_cmd_reply(skb);

	LEAVE();
	return ret;
}

/**
 * @brief vendor command to supplicant to update AP info
 *
 * @param priv     A pointer to moal_private
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
int
woal_roam_ap_info(IN moal_private *priv, IN t_u8 *data, IN int len)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	struct sk_buff *skb = NULL;
	int ret = MLAN_STATUS_SUCCESS;
	key_info *pkey = NULL;
	apinfo *pinfo = NULL;
	MrvlIEtypesHeader_t *tlv = NULL;
	t_u16 tlv_type = 0, tlv_len = 0, tlv_buf_left = 0;
	int event_id = 0;
	t_u8 authorized = 1;

	ENTER();

	event_id = woal_get_event_id(fw_roam_success);
	if (event_max == event_id) {
		PRINTM(MERROR, "Not find this event %d \n", event_id);
		ret = 1;
		LEAVE();
		return ret;
	}
	/**allocate skb*/
#if CFG80211_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, NULL, len + 50,
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len + 50,
#endif
					  event_id, GFP_ATOMIC);

	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor event\n");
		ret = 1;
		LEAVE();
		return ret;
	}

	nla_put(skb, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_BSSID,
		MLAN_MAC_ADDR_LENGTH, (t_u8 *)data);
	nla_put(skb, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_AUTHORIZED,
		sizeof(authorized), &authorized);
	tlv = (MrvlIEtypesHeader_t *)(data + MLAN_MAC_ADDR_LENGTH);
	tlv_buf_left = len - MLAN_MAC_ADDR_LENGTH;
	while (tlv_buf_left >= sizeof(MrvlIEtypesHeader_t)) {
		tlv_type = woal_le16_to_cpu(tlv->type);
		tlv_len = woal_le16_to_cpu(tlv->len);

		if (tlv_buf_left < (tlv_len + sizeof(MrvlIEtypesHeader_t))) {
			PRINTM(MERROR,
			       "Error processing firmware roam success TLVs, bytes left < TLV length\n");
			break;
		}

		switch (tlv_type) {
		case TLV_TYPE_APINFO:
			pinfo = (apinfo *) tlv;
			nla_put(skb, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_RESP_IE,
				pinfo->header.len, pinfo->rsp_ie);
			break;

		case TLV_TYPE_KEYINFO:
			pkey = (key_info *) tlv;
			nla_put(skb,
				MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_KEY_REPLAY_CTR,
				MLAN_REPLAY_CTR_LEN, pkey->key.replay_ctr);
			nla_put(skb, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KCK,
				MLAN_KCK_LEN, pkey->key.kck);
			nla_put(skb, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KEK,
				MLAN_KEK_LEN, pkey->key.kek);
			break;
		default:
			break;
		}
		tlv_buf_left -= tlv_len + sizeof(MrvlIEtypesHeader_t);
		tlv = (MrvlIEtypesHeader_t *)((t_u8 *)tlv + tlv_len +
					      sizeof(MrvlIEtypesHeader_t));
	}

	/**send event*/
	cfg80211_vendor_event(skb, GFP_ATOMIC);

	LEAVE();
	return ret;
}

/**
 * @brief vendor command to set enable/disable dfs offload
 *
 * @param wiphy       A pointer to wiphy struct
 * @param wdev     A pointer to wireless_dev struct
 * @param data     a pointer to data
 * @param  len     data length
 *
 * @return      0: success  1: fail
 */
static int
woal_cfg80211_subcmd_set_dfs_offload(struct wiphy *wiphy,
				     struct wireless_dev *wdev,
				     const void *data, int data_len)
{
	struct sk_buff *skb = NULL;
	int ret = 1;

	ENTER();

	/** Allocate skb for cmd reply*/
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(dfs_offload));
	if (!skb) {
		PRINTM(MERROR, "allocate memory fail for vendor cmd\n");
		ret = 1;
		LEAVE();
		return ret;
	}
	nla_put(skb, MRVL_WLAN_VENDOR_ATTR_DFS, sizeof(t_u32), &dfs_offload);
	ret = cfg80211_vendor_cmd_reply(skb);

	LEAVE();
	return ret;
}

const struct wiphy_vendor_command vendor_commands[] = {
	{
	 .info = {.vendor_id = MRVL_VENDOR_ID,.subcmd = sub_cmd_set_drvdbg,},
	 .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
	 .doit = woal_cfg80211_subcmd_set_drvdbg,
	 },
	{
	 .info = {.vendor_id = MRVL_VENDOR_ID,.subcmd =
		  sub_cmd_set_roaming_offload_key,},
	 .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
	 .doit = woal_cfg80211_subcmd_set_roaming_offload_key,
	 },

	{
	 .info = {.vendor_id = MRVL_VENDOR_ID,.subcmd =
		  sub_cmd_dfs_capability,},
	 .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
	 .doit = woal_cfg80211_subcmd_set_dfs_offload,
	 },

	{
	 .info = {.vendor_id = MRVL_VENDOR_ID,.subcmd =
		  sub_cmd_get_correlated_time,},
	 .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
	 .doit = woal_cfg80211_subcmd_get_correlated_time,
	 },
};

/**
 * @brief register vendor commands and events
 *
 * @param wiphy       A pointer to wiphy struct
 *
 * @return
 */
void
woal_register_cfg80211_vendor_command(struct wiphy *wiphy)
{
	ENTER();
	wiphy->vendor_commands = vendor_commands;
	wiphy->n_vendor_commands = ARRAY_SIZE(vendor_commands);
	wiphy->vendor_events = vendor_events;
	wiphy->n_vendor_events = ARRAY_SIZE(vendor_events);
	LEAVE();
}
#endif
