/** @file moal_cfgvendor.h
  *
  * @brief This file contains the CFG80211 vendor specific defines.
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

#ifndef _MOAL_CFGVENDOR_H_
#define _MOAL_CFGVENDOR_H_

#include    "moal_main.h"

#if CFG80211_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/**vendor event*/
enum vendor_event { event_hang = 0, event_set_key_mgmt_offload =
		0x10001, fw_roam_success =
		0x10002, event_dfs_radar_detected =
		0x10004, event_dfs_cac_started =
		0x10005, event_dfs_cac_finished =
		0x10006, event_dfs_cac_aborted =
		0x10007, event_dfs_nop_finished = 0x10008, event_max,
};

#if CFG80211_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
/** struct dfs_event */
	typedef struct _dfs_event {
	int freq;
	int ht_enabled;
	int chan_offset;
	enum nl80211_chan_width chan_width;
	int cf1;
	int cf2;
} dfs_event;
 void woal_cfg80211_dfs_vendor_event(moal_private *priv, int event,
				       struct cfg80211_chan_def *chandef);

#endif	/*  */

/**vendor sub command*/
enum vendor_sub_command { sub_cmd_set_drvdbg =
		0, sub_cmd_set_roaming_offload_key =
		0x0002, sub_cmd_dfs_capability =
		0x0005, sub_cmd_get_correlated_time = 0x0006, sub_cmd_max,
};
void woal_register_cfg80211_vendor_command(struct wiphy *wiphy );
int woal_cfg80211_vendor_event(IN moal_private *priv, IN int event,
				IN t_u8 *data, IN int len);
 enum mrvl_wlan_vendor_attr { MRVL_WLAN_VENDOR_ATTR_INVALID = 0,
		/* Used by MRVL_NL80211_VENDOR_SUBCMD_DFS_CAPABILITY */
	MRVL_WLAN_VENDOR_ATTR_DFS =
		1, MRVL_WLAN_VENDOR_ATTR_AFTER_LAST,
		 MRVL_WLAN_VENDOR_ATTR_MAX =
		MRVL_WLAN_VENDOR_ATTR_AFTER_LAST - 1,
};
  int woal_roam_ap_info(IN moal_private *priv, IN t_u8 *data, IN int len);

#endif	/*  */
	enum mrvl_wlan_vendor_attr_roam_auth {
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_INVALID =
		0, MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_BSSID,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_REQ_IE,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_RESP_IE,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_AUTHORIZED,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_KEY_REPLAY_CTR,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KCK,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_PTK_KEK,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_SUBNET_STATUS,
		/* keep last */
	MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_AFTER_LAST,
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_MAX =
		MRVL_WLAN_VENDOR_ATTR_ROAM_AUTH_AFTER_LAST - 1
};

#define PROPRIETARY_TLV_BASE_ID 0x100
#define TLV_TYPE_APINFO (PROPRIETARY_TLV_BASE_ID + 249)
#define TLV_TYPE_KEYINFO (PROPRIETARY_TLV_BASE_ID + 250)
#define TLV_TYPE_CHANNELBANDLIST    (PROPRIETARY_TLV_BASE_ID + 0x2a) /* 0x012a */

/** MrvlIEtypesHeader_t */
	typedef struct MrvlIEtypesHeader {

	/** Header type */
	t_u16 type;

	/** Header length */
	t_u16 len;
} __ATTRIB_PACK__ MrvlIEtypesHeader_t;
 typedef struct _key_info_tlv {

	/** Header */
	MrvlIEtypesHeader_t header;

	/** kck, kek, key_replay*/
	 mlan_ds_misc_gtk_rekey_data key;
} key_info;
 typedef struct _apinfo_tlv {

	/** Header */
	MrvlIEtypesHeader_t header;

	/** Assoc response buffer */
	t_u8 rsp_ie[1];
} apinfo;

/** ChanBandParamSet_t */
	typedef struct _ChanBandParamSet_t {

    /** Channel scan parameter : Radio type */
	t_u8 radio_type;

    /** Channel number */
	t_u8 chan_number;
} ChanBandParamSet_t;

/** MrvlIEtypes_ChanBandListParamSet_t */
	typedef MLAN_PACK_START struct _MrvlIEtypes_ChanBandListParamSet_t {

    /** Header */
	MrvlIEtypesHeader_t header;

    /** Channel Band parameters */
	ChanBandParamSet_t chan_band_param[1];
} MLAN_PACK_END MrvlIEtypes_ChanBandListParamSet_t;

#endif	/* _MOAL_CFGVENDOR_H_ */
