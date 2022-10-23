
#include <typedefs.h>
#include <osl.h>

#include <bcmutils.h>

#include <dhd_config.h>
#include <dhd_dbg.h>

/* message levels */
#define CONFIG_ERROR_LEVEL	0x0001
#define CONFIG_TRACE_LEVEL	0x0002

uint config_msg_level = CONFIG_ERROR_LEVEL;

#define CONFIG_ERROR(x) \
	do { \
		if (config_msg_level & CONFIG_ERROR_LEVEL) { \
			printk(KERN_ERR "CONFIG-ERROR) ");	\
			printk x; \
		} \
	} while (0)
#define CONFIG_TRACE(x) \
	do { \
		if (config_msg_level & CONFIG_TRACE_LEVEL) { \
			printk(KERN_ERR "CONFIG-TRACE) ");	\
			printk x; \
		} \
	} while (0)

#define MAXSZ_BUF		1000
#define	MAXSZ_CONFIG	4096

#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i
#define htodchanspec(i) i
#define dtohchanspec(i) i

void
dhd_conf_set_conf_path_by_fw_path(dhd_pub_t *dhd, char *conf_path, char *fw_path)
{
	int i;

	if (fw_path[0] == '\0') {
#ifdef CONFIG_BCMDHD_FW_PATH
		bcm_strncpy_s(conf_path, MOD_PARAM_PATHLEN-1, CONFIG_BCMDHD_FW_PATH, MOD_PARAM_PATHLEN-1);
		if (fw_path[0] == '\0')
#endif
		{
			printf("fw path is null\n");
			return;
		}
	} else
		strcpy(conf_path, fw_path);

	/* find out the last '/' */
	i = strlen(conf_path);
	while (i > 0) {
		if (conf_path[i] == '/') break;
		i--;
	}
	strcpy(&conf_path[i+1], "config.txt");

	printf("%s: config_path=%s\n", __FUNCTION__, conf_path);
}

int
dhd_conf_set_fw_int_cmd(dhd_pub_t *dhd, char *name, uint cmd, int val,
	int def, bool down)
{
	int bcmerror = -1;

	if (val >= def) {
		if (down) {
			if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_DOWN, NULL, 0, TRUE, 0)) < 0)
				CONFIG_ERROR(("%s: WLC_DOWN setting failed %d\n", __FUNCTION__, bcmerror));
		}
		printf("%s: set %s %d %d\n", __FUNCTION__, name, cmd, val);
		if ((bcmerror = dhd_wl_ioctl_cmd(dhd, cmd, &val, sizeof(val), TRUE, 0)) < 0)
			CONFIG_ERROR(("%s: %s setting failed %d\n", __FUNCTION__, name, bcmerror));
	}
	return bcmerror;
}

int
dhd_conf_set_fw_int_struct_cmd(dhd_pub_t *dhd, char *name, uint cmd,
	int *val, int len, bool down)
{
	int bcmerror = -1;

	if (down) {
		if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_DOWN, NULL, 0, TRUE, 0)) < 0)
			CONFIG_ERROR(("%s: WLC_DOWN setting failed %d\n", __FUNCTION__, bcmerror));
	}
	if ((bcmerror = dhd_wl_ioctl_cmd(dhd, cmd, val, len, TRUE, 0)) < 0)
		CONFIG_ERROR(("%s: %s setting failed %d\n", __FUNCTION__, name, bcmerror));

	return bcmerror;
}

int
dhd_conf_set_fw_string_cmd(dhd_pub_t *dhd, char *cmd, int val, int def,
	bool down)
{
	int bcmerror = -1;
	char iovbuf[WL_EVENTING_MASK_LEN + 12];	/*  Room for "event_msgs" + '\0' + bitvec  */

	if (val >= def) {
		if (down) {
			if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_DOWN, NULL, 0, TRUE, 0)) < 0)
				CONFIG_ERROR(("%s: WLC_DOWN setting failed %d\n", __FUNCTION__, bcmerror));
		}
		printf("%s: set %s %d\n", __FUNCTION__, cmd, val);
		bcm_mkiovar(cmd, (char *)&val, 4, iovbuf, sizeof(iovbuf));
		if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0)
			CONFIG_ERROR(("%s: %s setting failed %d\n", __FUNCTION__, cmd, bcmerror));
	}
	return bcmerror;
}

int
dhd_conf_set_fw_string_struct_cmd(dhd_pub_t *dhd, char *cmd, char *val,
	int len, bool down)
{
	int bcmerror = -1;
	char iovbuf[WLC_IOCTL_SMLEN];
	
	if (down) {
		if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_DOWN, NULL, 0, TRUE, 0)) < 0)
			CONFIG_ERROR(("%s: WLC_DOWN setting failed %d\n", __FUNCTION__, bcmerror));
	}
	printf("%s: set %s\n", __FUNCTION__, cmd);
	bcm_mkiovar(cmd, val, len, iovbuf, sizeof(iovbuf));
	if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0)
		printf("%s: %s setting failed %d\n", __FUNCTION__, cmd, bcmerror);

	return bcmerror;
}

uint
dhd_conf_get_band(dhd_pub_t *dhd)
{
	uint band = WLC_BAND_AUTO;

	if (dhd && dhd->conf)
		band = dhd->conf->band;
	else
		CONFIG_ERROR(("%s: dhd or conf is NULL\n", __FUNCTION__));

	return band;
}

int
dhd_conf_set_country(dhd_pub_t *dhd)
{
	int bcmerror = -1;

	memset(&dhd->dhd_cspec, 0, sizeof(wl_country_t));
	printf("%s: set country %s, revision %d\n", __FUNCTION__,
		dhd->conf->cspec.ccode, dhd->conf->cspec.rev);
	dhd_conf_set_fw_string_struct_cmd(dhd, "country", (char *)&dhd->conf->cspec, sizeof(wl_country_t), FALSE);

	return bcmerror;
}

int
dhd_conf_get_country(dhd_pub_t *dhd, wl_country_t *cspec)
{
	int bcmerror = -1;

	memset(cspec, 0, sizeof(wl_country_t));
	bcm_mkiovar("country", NULL, 0, (char*)cspec, sizeof(wl_country_t));
	if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, cspec, sizeof(wl_country_t), FALSE, 0)) < 0)
		printf("%s: country code getting failed %d\n", __FUNCTION__, bcmerror);
	else
		printf("Country code: %s (%s/%d)\n", cspec->country_abbrev, cspec->ccode, cspec->rev);

	return bcmerror;
}

int
dhd_conf_get_country_from_config(dhd_pub_t *dhd, wl_country_t *cspec)
{
	int bcmerror = -1, i;
	struct dhd_conf *conf = dhd->conf;

	for (i = 0; i < conf->country_list.count; i++) {
		if (strcmp(cspec->country_abbrev, conf->country_list.cspec[i].country_abbrev) == 0) {
			memcpy(cspec->ccode,
				conf->country_list.cspec[i].ccode, WLC_CNTRY_BUF_SZ);
			cspec->rev = conf->country_list.cspec[i].rev;
			printf("%s: %s/%d\n", __FUNCTION__, cspec->ccode, cspec->rev);
			return 0;
		}
	}

	return bcmerror;
}

int
dhd_conf_fix_country(dhd_pub_t *dhd)
{
	int bcmerror = -1;
	uint band;
	wl_uint32_list_t *list;
	u8 valid_chan_list[sizeof(u32)*(WL_NUMCHANNELS + 1)];

	if (!(dhd && dhd->conf)) {
		return bcmerror;
	}

	memset(valid_chan_list, 0, sizeof(valid_chan_list));
	list = (wl_uint32_list_t *)(void *) valid_chan_list;
	list->count = htod32(WL_NUMCHANNELS);
	if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_GET_VALID_CHANNELS, valid_chan_list, sizeof(valid_chan_list), FALSE, 0)) < 0) {
		CONFIG_ERROR(("%s: get channels failed with %d\n", __FUNCTION__, bcmerror));
	}

	band = dhd_conf_get_band(dhd);

	if (bcmerror || ((band==WLC_BAND_AUTO || band==WLC_BAND_2G) &&
			dtoh32(list->count)<11)) {
		CONFIG_ERROR(("%s: bcmerror=%d, # of channels %d\n",
			__FUNCTION__, bcmerror, dtoh32(list->count)));
		if ((bcmerror = dhd_conf_set_country(dhd)) < 0) {
			strcpy(dhd->conf->cspec.country_abbrev, "US");
			dhd->conf->cspec.rev = 0;
			strcpy(dhd->conf->cspec.ccode, "US");
			dhd_conf_set_country(dhd);
		}
	}

	return bcmerror;
}

bool
dhd_conf_match_channel(dhd_pub_t *dhd, uint32 channel)
{
	int i;
	bool match = false;

	if (dhd && dhd->conf) {
		if (dhd->conf->channels.count == 0)
			return true;
		for (i=0; i<dhd->conf->channels.count; i++) {
			if (channel == dhd->conf->channels.channel[i])
				match = true;
		}
	} else {
		match = true;
		CONFIG_ERROR(("%s: dhd or conf is NULL\n", __FUNCTION__));
	}

	return match;
}

int
dhd_conf_set_roam(dhd_pub_t *dhd)
{
	int bcmerror = -1;
	struct dhd_conf *conf = dhd->conf;

	dhd_roam_disable = conf->roam_off;
	dhd_conf_set_fw_string_cmd(dhd, "roam_off", dhd->conf->roam_off, 0, FALSE);

	if (!conf->roam_off || !conf->roam_off_suspend) {
		printf("%s: set roam_trigger %d\n", __FUNCTION__, conf->roam_trigger[0]);
		dhd_conf_set_fw_int_struct_cmd(dhd, "WLC_SET_ROAM_TRIGGER", WLC_SET_ROAM_TRIGGER,
				conf->roam_trigger, sizeof(conf->roam_trigger), FALSE);

		printf("%s: set roam_scan_period %d\n", __FUNCTION__, conf->roam_scan_period[0]);
		dhd_conf_set_fw_int_struct_cmd(dhd, "WLC_SET_ROAM_SCAN_PERIOD", WLC_SET_ROAM_SCAN_PERIOD,
				conf->roam_scan_period, sizeof(conf->roam_scan_period), FALSE);

		printf("%s: set roam_delta %d\n", __FUNCTION__, conf->roam_delta[0]);
		dhd_conf_set_fw_int_struct_cmd(dhd, "WLC_SET_ROAM_DELTA", WLC_SET_ROAM_DELTA,
				conf->roam_delta, sizeof(conf->roam_delta), FALSE);
		
		dhd_conf_set_fw_string_cmd(dhd, "fullroamperiod", dhd->conf->fullroamperiod, 1, FALSE);
	}

	return bcmerror;
}

void
dhd_conf_get_wme(dhd_pub_t *dhd, edcf_acparam_t *acp)
{
	int bcmerror = -1;
	char iovbuf[WLC_IOCTL_SMLEN];
	edcf_acparam_t *acparam;

	bzero(iovbuf, sizeof(iovbuf));

	/*
	 * Get current acparams, using buf as an input buffer.
	 * Return data is array of 4 ACs of wme params.
	 */
	bcm_mkiovar("wme_ac_sta", NULL, 0, iovbuf, sizeof(iovbuf));
	if ((bcmerror = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iovbuf, sizeof(iovbuf), FALSE, 0)) < 0) {
		CONFIG_ERROR(("%s: wme_ac_sta getting failed %d\n", __FUNCTION__, bcmerror));
		return;
	}
	memcpy((char*)acp, iovbuf, sizeof(edcf_acparam_t)*AC_COUNT);

	acparam = &acp[AC_BK];
	CONFIG_TRACE(("%s: BK: aci %d aifsn %d ecwmin %d ecwmax %d size %d\n", __FUNCTION__,
		acparam->ACI, acparam->ACI&EDCF_AIFSN_MASK,
		acparam->ECW&EDCF_ECWMIN_MASK, (acparam->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT,
		(int)sizeof(acp)));
	acparam = &acp[AC_BE];
	CONFIG_TRACE(("%s: BE: aci %d aifsn %d ecwmin %d ecwmax %d size %d\n", __FUNCTION__,
		acparam->ACI, acparam->ACI&EDCF_AIFSN_MASK,
		acparam->ECW&EDCF_ECWMIN_MASK, (acparam->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT,
		(int)sizeof(acp)));
	acparam = &acp[AC_VI];
	CONFIG_TRACE(("%s: VI: aci %d aifsn %d ecwmin %d ecwmax %d size %d\n", __FUNCTION__,
		acparam->ACI, acparam->ACI&EDCF_AIFSN_MASK,
		acparam->ECW&EDCF_ECWMIN_MASK, (acparam->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT,
		(int)sizeof(acp)));
	acparam = &acp[AC_VO];
	CONFIG_TRACE(("%s: VO: aci %d aifsn %d ecwmin %d ecwmax %d size %d\n", __FUNCTION__,
		acparam->ACI, acparam->ACI&EDCF_AIFSN_MASK,
		acparam->ECW&EDCF_ECWMIN_MASK, (acparam->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT,
		(int)sizeof(acp)));

	return;
}

void
dhd_conf_update_wme(dhd_pub_t *dhd, edcf_acparam_t *acparam_cur, int aci)
{
	int aifsn, ecwmin, ecwmax;
	edcf_acparam_t *acp;
	struct dhd_conf *conf = dhd->conf;

	/* Default value */
	aifsn = acparam_cur->ACI&EDCF_AIFSN_MASK;
	ecwmin = acparam_cur->ECW&EDCF_ECWMIN_MASK;
	ecwmax = (acparam_cur->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT;

	/* Modified value */
	if (conf->wme.aifsn[aci] > 0)
		aifsn = conf->wme.aifsn[aci];
	if (conf->wme.cwmin[aci] > 0)
		ecwmin = conf->wme.cwmin[aci];
	if (conf->wme.cwmax[aci] > 0)
		ecwmax = conf->wme.cwmax[aci];

	/* Update */
	acp = acparam_cur;
	acp->ACI = (acp->ACI & ~EDCF_AIFSN_MASK) | (aifsn & EDCF_AIFSN_MASK);
	acp->ECW = ((ecwmax << EDCF_ECWMAX_SHIFT) & EDCF_ECWMAX_MASK) | (acp->ECW & EDCF_ECWMIN_MASK);
	acp->ECW = ((acp->ECW & EDCF_ECWMAX_MASK) | (ecwmin & EDCF_ECWMIN_MASK));

	CONFIG_TRACE(("%s: mod aci %d aifsn %d ecwmin %d ecwmax %d size %d\n", __FUNCTION__,
		acp->ACI, acp->ACI&EDCF_AIFSN_MASK,
		acp->ECW&EDCF_ECWMIN_MASK, (acp->ECW&EDCF_ECWMAX_MASK)>>EDCF_ECWMAX_SHIFT,
		(int)sizeof(edcf_acparam_t)));

	/*
	* Now use buf as an output buffer.
	* Put WME acparams after "wme_ac\0" in buf.
	* NOTE: only one of the four ACs can be set at a time.
	*/
	dhd_conf_set_fw_string_struct_cmd(dhd, "wme_ac_sta", (char *)acp, sizeof(edcf_acparam_t), FALSE);

}

void
dhd_conf_set_wme(dhd_pub_t *dhd)
{
	edcf_acparam_t acparam_cur[AC_COUNT];

	if (dhd && dhd->conf) {
		if (!dhd->conf->force_wme_ac) {
			CONFIG_TRACE(("%s: force_wme_ac is not enabled %d\n",
				__FUNCTION__, dhd->conf->force_wme_ac));
			return;
		}

		CONFIG_TRACE(("%s: Before change:\n", __FUNCTION__));
		dhd_conf_get_wme(dhd, acparam_cur);

		dhd_conf_update_wme(dhd, &acparam_cur[AC_BK], AC_BK);
		dhd_conf_update_wme(dhd, &acparam_cur[AC_BE], AC_BE);
		dhd_conf_update_wme(dhd, &acparam_cur[AC_VI], AC_VI);
		dhd_conf_update_wme(dhd, &acparam_cur[AC_VO], AC_VO);

		CONFIG_TRACE(("%s: After change:\n", __FUNCTION__));
		dhd_conf_get_wme(dhd, acparam_cur);
	} else {
		CONFIG_ERROR(("%s: dhd or conf is NULL\n", __FUNCTION__));
	}

	return;
}

#ifdef PKT_FILTER_SUPPORT
void
dhd_conf_add_pkt_filter(dhd_pub_t *dhd)
{
	int i;

	/*
	 * All pkt: pkt_filter_add=99 0 0 0 0x000000000000 0x000000000000
	 * Netbios pkt: 120 0 0 12 0xFFFF000000000000000000FF000000000000000000000000FFFF 0x0800000000000000000000110000000000000000000000000089
	 */
	for(i=0; i<dhd->conf->pkt_filter_add.count; i++) {
		dhd->pktfilter[i+dhd->pktfilter_count] = dhd->conf->pkt_filter_add.filter[i];
		printf("%s: %s\n", __FUNCTION__, dhd->pktfilter[i+dhd->pktfilter_count]);
	}
	dhd->pktfilter_count += i;
}

bool
dhd_conf_del_pkt_filter(dhd_pub_t *dhd, uint32 id)
{
	int i;

	if (dhd && dhd->conf) {
		for (i=0; i<dhd->conf->pkt_filter_del.count; i++) {
			if (id == dhd->conf->pkt_filter_del.id[i]) {
				printf("%s: %d\n", __FUNCTION__, dhd->conf->pkt_filter_del.id[i]);
				return true;
			}
		}
		return false;
	}
	return false;
}

void
dhd_conf_discard_pkt_filter(dhd_pub_t *dhd)
{
	dhd->pktfilter[DHD_UNICAST_FILTER_NUM] = NULL;
	dhd->pktfilter[DHD_BROADCAST_FILTER_NUM] = "101 0 0 0 0xFFFFFFFFFFFF 0xFFFFFFFFFFFF";
	dhd->pktfilter[DHD_MULTICAST4_FILTER_NUM] = "102 0 0 0 0xFFFFFF 0x01005E";
	dhd->pktfilter[DHD_MULTICAST6_FILTER_NUM] = "103 0 0 0 0xFFFF 0x3333";
	dhd->pktfilter[DHD_MDNS_FILTER_NUM] = NULL;
	/* Do not enable ARP to pkt filter if dhd_master_mode is false.*/
	dhd->pktfilter[DHD_ARP_FILTER_NUM] = NULL;

	/* IPv4 broadcast address XXX.XXX.XXX.255 */
	dhd->pktfilter[dhd->pktfilter_count] = "110 0 0 12 0xFFFF00000000000000000000000000000000000000FF 0x080000000000000000000000000000000000000000FF";
	dhd->pktfilter_count++;
	/* discard IPv4 multicast address 224.0.0.0/4 */
	dhd->pktfilter[dhd->pktfilter_count] = "111 0 0 12 0xFFFF00000000000000000000000000000000F0 0x080000000000000000000000000000000000E0";
	dhd->pktfilter_count++;
	/* discard IPv6 multicast address FF00::/8 */
	dhd->pktfilter[dhd->pktfilter_count] = "112 0 0 12 0xFFFF000000000000000000000000000000000000000000000000FF 0x86DD000000000000000000000000000000000000000000000000FF";
	dhd->pktfilter_count++;
	/* discard Netbios pkt */
	dhd->pktfilter[dhd->pktfilter_count] = "120 0 0 12 0xFFFF000000000000000000FF000000000000000000000000FFFF 0x0800000000000000000000110000000000000000000000000089";
	dhd->pktfilter_count++;

}
#endif /* PKT_FILTER_SUPPORT */

int
dhd_conf_get_pm(dhd_pub_t *dhd)
{
	if (dhd && dhd->conf)
		return dhd->conf->pm;
	return -1;
}

unsigned int
process_config_vars(char *varbuf, unsigned int len, char *pickbuf, char *param)
{
	bool findNewline, changenewline=FALSE, pick=FALSE;
	int column;
	unsigned int n, pick_column=0;

	findNewline = FALSE;
	column = 0;

	for (n = 0; n < len; n++) {
		if (varbuf[n] == '\r')
			continue;
		if ((findNewline || changenewline) && varbuf[n] != '\n')
			continue;
		findNewline = FALSE;
		if (varbuf[n] == '#') {
			findNewline = TRUE;
			continue;
		}
		if (varbuf[n] == '\\') {
			changenewline = TRUE;
			continue;
		}
		if (!changenewline && varbuf[n] == '\n') {
			if (column == 0)
				continue;
			column = 0;
			continue;
		}
		if (changenewline && varbuf[n] == '\n') {
			changenewline = FALSE;
			continue;
		}
		if (!memcmp(&varbuf[n], param, strlen(param)) && column==0) {
			pick = TRUE;
			column = strlen(param);
			n += column;
			pick_column = 0;
		} else {
			if (pick && column==0)
				pick = FALSE;
			else
				column++;
		}
		if (pick) {
			if (varbuf[n] == 0x9)
				continue;
			if (pick_column>0 && pickbuf[pick_column-1]==' ' && varbuf[n]==' ')
				continue;
			pickbuf[pick_column] = varbuf[n];
			pick_column++;
		}
	}

	return pick_column;
}

void
dhd_conf_read_log_level(dhd_pub_t *dhd, char *bufp, uint len)
{
	uint len_val;
	char *pick;

	pick = MALLOC(dhd->osh, MAXSZ_BUF);
	if (!pick) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_BUF));
		return;
	}

	/* Process dhd_msglevel */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "msglevel=");
	if (len_val) {
		dhd_msg_level = (int)simple_strtol(pick, NULL, 0);
		printf("%s: dhd_msg_level = 0x%X\n", __FUNCTION__, dhd_msg_level);
	}
	/* Process dbus_msglevel */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "dbus_msglevel=");
	if (len_val) {
		dbus_msglevel = (int)simple_strtol(pick, NULL, 0);
		printf("%s: dbus_msglevel = 0x%X\n", __FUNCTION__, dbus_msglevel);
	}
	/* Process android_msg_level */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "android_msg_level=");
	if (len_val) {
		android_msg_level = (int)simple_strtol(pick, NULL, 0);
		printf("%s: android_msg_level = 0x%X\n", __FUNCTION__, android_msg_level);
	}
	/* Process config_msg_level */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "config_msg_level=");
	if (len_val) {
		config_msg_level = (int)simple_strtol(pick, NULL, 0);
		printf("%s: config_msg_level = 0x%X\n", __FUNCTION__, config_msg_level);
	}
#ifdef WL_CFG80211
	/* Process wl_dbg_level */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "wl_dbg_level=");
	if (len_val) {
		wl_dbg_level = (int)simple_strtol(pick, NULL, 0);
		printf("%s: wl_dbg_level = 0x%X\n", __FUNCTION__, wl_dbg_level);
	}
#endif
#if defined(WL_WIRELESS_EXT)
	/* Process iw_msg_level */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "iw_msg_level=");
	if (len_val) {
		iw_msg_level = (int)simple_strtol(pick, NULL, 0);
		printf("%s: iw_msg_level = 0x%X\n", __FUNCTION__, iw_msg_level);
	}
#endif

#if defined(DHD_DEBUG)
	/* Process dhd_console_ms */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "dhd_console_ms=");
	if (len_val) {
		dhd_console_ms = (int)simple_strtol(pick, NULL, 0);
		printf("%s: dhd_console_ms = 0x%X\n", __FUNCTION__, dhd_console_ms);
	}
#endif

	if (pick)
		MFREE(dhd->osh, pick, MAXSZ_BUF);
}

void
dhd_conf_read_wme_ac_params(dhd_pub_t *dhd, char *bufp, uint len)
{
	uint len_val;
	char *pick;
	struct dhd_conf *conf = dhd->conf;

	pick = MALLOC(dhd->osh, MAXSZ_BUF);
	if (!pick) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_BUF));
		return;
	}

	/* Process WMM parameters */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "force_wme_ac=");
	if (len_val) {
		conf->force_wme_ac = (int)simple_strtol(pick, NULL, 10);
		printf("%s: force_wme_ac = %d\n", __FUNCTION__, conf->force_wme_ac);
	}

	if (conf->force_wme_ac) {
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "bk_aifsn=");
		if (len_val) {
			conf->wme.aifsn[AC_BK] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BK aifsn = %d\n", __FUNCTION__, conf->wme.aifsn[AC_BK]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "bk_cwmin=");
		if (len_val) {
			conf->wme.cwmin[AC_BK] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BK cwmin = %d\n", __FUNCTION__, conf->wme.cwmin[AC_BK]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "bk_cwmax=");
		if (len_val) {
			conf->wme.cwmax[AC_BK] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BK cwmax = %d\n", __FUNCTION__, conf->wme.cwmax[AC_BK]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "be_aifsn=");
		if (len_val) {
			conf->wme.aifsn[AC_BE] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BE aifsn = %d\n", __FUNCTION__, conf->wme.aifsn[AC_BE]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "be_cwmin=");
		if (len_val) {
			conf->wme.cwmin[AC_BE] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BE cwmin = %d\n", __FUNCTION__, conf->wme.cwmin[AC_BE]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "be_cwmax=");
		if (len_val) {
			conf->wme.cwmax[AC_BE] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_BE cwmax = %d\n", __FUNCTION__, conf->wme.cwmax[AC_BE]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vi_aifsn=");
		if (len_val) {
			conf->wme.aifsn[AC_VI] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VI aifsn = %d\n", __FUNCTION__, conf->wme.aifsn[AC_VI]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vi_cwmin=");
		if (len_val) {
			conf->wme.cwmin[AC_VI] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VI cwmin = %d\n", __FUNCTION__, conf->wme.cwmin[AC_VI]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vi_cwmax=");
		if (len_val) {
			conf->wme.cwmax[AC_VI] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VI cwmax = %d\n", __FUNCTION__, conf->wme.cwmax[AC_VI]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vo_aifsn=");
		if (len_val) {
			conf->wme.aifsn[AC_VO] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VO aifsn = %d\n", __FUNCTION__, conf->wme.aifsn[AC_VO]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vo_cwmin=");
		if (len_val) {
			conf->wme.cwmin[AC_VO] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VO cwmin = %d\n", __FUNCTION__, conf->wme.cwmin[AC_VO]);
		}

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "vo_cwmax=");
		if (len_val) {
			conf->wme.cwmax[AC_VO] = (int)simple_strtol(pick, NULL, 10);
			printf("%s: AC_VO cwmax = %d\n", __FUNCTION__, conf->wme.cwmax[AC_VO]);
		}
	}

	if (pick)
		MFREE(dhd->osh, pick, MAXSZ_BUF);

}

void
dhd_conf_read_roam_params(dhd_pub_t *dhd, char *bufp, uint len)
{
	uint len_val;
	char *pick;
	struct dhd_conf *conf = dhd->conf;

	pick = MALLOC(dhd->osh, MAXSZ_BUF);
	if (!pick) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_BUF));
		return;
	}

	/* Process roam */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "roam_off=");
	if (len_val) {
		if (!strncmp(pick, "0", len_val))
			conf->roam_off = 0;
		else
			conf->roam_off = 1;
		printf("%s: roam_off = %d\n", __FUNCTION__, conf->roam_off);
	}

	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "roam_off_suspend=");
	if (len_val) {
		if (!strncmp(pick, "0", len_val))
			conf->roam_off_suspend = 0;
		else
			conf->roam_off_suspend = 1;
		printf("%s: roam_off_suspend = %d\n", __FUNCTION__,
			conf->roam_off_suspend);
	}

	if (!conf->roam_off || !conf->roam_off_suspend) {
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "roam_trigger=");
		if (len_val)
			conf->roam_trigger[0] = (int)simple_strtol(pick, NULL, 10);
		printf("%s: roam_trigger = %d\n", __FUNCTION__,
			conf->roam_trigger[0]);

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "roam_scan_period=");
		if (len_val)
			conf->roam_scan_period[0] = (int)simple_strtol(pick, NULL, 10);
		printf("%s: roam_scan_period = %d\n", __FUNCTION__,
			conf->roam_scan_period[0]);

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "roam_delta=");
		if (len_val)
			conf->roam_delta[0] = (int)simple_strtol(pick, NULL, 10);
		printf("%s: roam_delta = %d\n", __FUNCTION__, conf->roam_delta[0]);

		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "fullroamperiod=");
		if (len_val)
			conf->fullroamperiod = (int)simple_strtol(pick, NULL, 10);
		printf("%s: fullroamperiod = %d\n", __FUNCTION__,
			conf->fullroamperiod);
	}

	if (pick)
		MFREE(dhd->osh, pick, MAXSZ_BUF);

}

void
dhd_conf_read_country_list(dhd_pub_t *dhd, char *bufp, uint len)
{
	uint len_val;
	int i;
	char *pick, *pch, *pick_tmp;
	struct dhd_conf *conf = dhd->conf;

	pick = MALLOC(dhd->osh, MAXSZ_BUF);
	if (!pick) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_BUF));
		return;
	}

	/* Process country_list:
	 * country_list=[country1]:[ccode1]/[regrev1],
	 * [country2]:[ccode2]/[regrev2] \
	 * Ex: country_list=US:US/0, TW:TW/1
	 */
	memset(pick, 0, MAXSZ_BUF);
	len_val = process_config_vars(bufp, len, pick, "country_list=");
	if (len_val) {
		pick_tmp = pick;
		for (i=0; i<CONFIG_COUNTRY_LIST_SIZE; i++) {
			/* Process country code */
			pch = bcmstrtok(&pick_tmp, ":", 0);
			if (!pch)
				break;
			strcpy(conf->country_list.cspec[i].country_abbrev, pch);
			pch = bcmstrtok(&pick_tmp, "/", 0);
			if (!pch)
				break;
			memcpy(conf->country_list.cspec[i].ccode, pch, 2);
			pch = bcmstrtok(&pick_tmp, ", ", 0);
			if (!pch)
				break;
			conf->country_list.cspec[i].rev = (int32)simple_strtol(pch, NULL, 10);
			conf->country_list.count ++;
			CONFIG_TRACE(("%s: country_list abbrev=%s, ccode=%s, regrev=%d\n", __FUNCTION__,
				conf->country_list.cspec[i].country_abbrev,
				conf->country_list.cspec[i].ccode,
				conf->country_list.cspec[i].rev));
		}
		printf("%s: %d country in list\n", __FUNCTION__, conf->country_list.count);
	}

	if (pick)
		MFREE(dhd->osh, pick, MAXSZ_BUF);
}

int
dhd_conf_read_config(dhd_pub_t *dhd, char *conf_path)
{
	int bcmerror = -1, i;
	uint len, len_val;
	void * image = NULL;
	char * memblock = NULL;
	char *bufp, *pick = NULL, *pch, *pick_tmp;
	bool conf_file_exists;
	struct dhd_conf *conf = dhd->conf;

	conf_file_exists = ((conf_path != NULL) && (conf_path[0] != '\0'));
	if (!conf_file_exists) {
		printf("%s: config path %s\n", __FUNCTION__, conf_path);
		return (0);
	}

	if (conf_file_exists) {
		image = dhd_os_open_image(conf_path);
		if (image == NULL) {
			printf("%s: Ignore config file %s\n", __FUNCTION__, conf_path);
			goto err;
		}
	}

	memblock = MALLOC(dhd->osh, MAXSZ_CONFIG);
	if (memblock == NULL) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_CONFIG));
		goto err;
	}

	pick = MALLOC(dhd->osh, MAXSZ_BUF);
	if (!pick) {
		CONFIG_ERROR(("%s: Failed to allocate memory %d bytes\n",
			__FUNCTION__, MAXSZ_BUF));
		goto err;
	}

	/* Read variables */
	if (conf_file_exists) {
		len = dhd_os_get_image_block(memblock, MAXSZ_CONFIG, image);
	}
	if (len > 0 && len < MAXSZ_CONFIG) {
		bufp = (char *)memblock;
		bufp[len] = 0;

		/* Process log_level */
		dhd_conf_read_log_level(dhd, bufp, len);
		dhd_conf_read_roam_params(dhd, bufp, len);
		dhd_conf_read_wme_ac_params(dhd, bufp, len);
		dhd_conf_read_country_list(dhd, bufp, len);

		/* Process band:
		 * band=a for 5GHz only and band=b for 2.4GHz only
		 */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "band=");
		if (len_val) {
			if (!strncmp(pick, "b", len_val))
				conf->band = WLC_BAND_2G;
			else if (!strncmp(pick, "a", len_val))
				conf->band = WLC_BAND_5G;
			else
				conf->band = WLC_BAND_AUTO;
			printf("%s: band = %d\n", __FUNCTION__, conf->band);
		}

		/* Process mimo_bw_cap */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "mimo_bw_cap=");
		if (len_val) {
			conf->mimo_bw_cap = (uint)simple_strtol(pick, NULL, 10);
			printf("%s: mimo_bw_cap = %d\n", __FUNCTION__, conf->mimo_bw_cap);
		}

		/* Process country code */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "ccode=");
		if (len_val) {
			memset(&conf->cspec, 0, sizeof(wl_country_t));
			memcpy(conf->cspec.country_abbrev, pick, len_val);
			memcpy(conf->cspec.ccode, pick, len_val);
			memset(pick, 0, MAXSZ_BUF);
			len_val = process_config_vars(bufp, len, pick, "regrev=");
			if (len_val)
				conf->cspec.rev = (int32)simple_strtol(pick, NULL, 10);
		}

		/* Process channels */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "channels=");
		pick_tmp = pick;
		if (len_val) {
			pch = bcmstrtok(&pick_tmp, " ,.-", 0);
			i=0;
			while (pch != NULL && i<WL_NUMCHANNELS) {
				conf->channels.channel[i] = (uint32)simple_strtol(pch, NULL, 10);
				pch = bcmstrtok(&pick_tmp, " ,.-", 0);
				i++;
			}
			conf->channels.count = i;
			printf("%s: channels = ", __FUNCTION__);
			for (i=0; i<conf->channels.count; i++)
				printf("%d ", conf->channels.channel[i]);
			printf("\n");
		}

		/* Process keep alive period */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "keep_alive_period=");
		if (len_val) {
			conf->keep_alive_period = (uint)simple_strtol(pick, NULL, 10);
			printf("%s: keep_alive_period = %d\n", __FUNCTION__,
				conf->keep_alive_period);
		}

		/* Process STBC parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "stbc=");
		if (len_val) {
			conf->stbc = (int)simple_strtol(pick, NULL, 10);
			printf("%s: stbc = %d\n", __FUNCTION__, conf->stbc);
		}

		/* Process dhd_master_mode parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "dhd_master_mode=");
		if (len_val) {
			if (!strncmp(pick, "0", len_val))
				dhd_master_mode = FALSE;
			else
				dhd_master_mode = TRUE;
			printf("%s: dhd_master_mode = %d\n", __FUNCTION__, dhd_master_mode);
		}

#ifdef PKT_FILTER_SUPPORT
		/* Process pkt_filter_add:
		 * All pkt: pkt_filter_add=99 0 0 0 0x000000000000 0x000000000000
		 */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "pkt_filter_add=");
		pick_tmp = pick;
		if (len_val) {
			pch = bcmstrtok(&pick_tmp, ",.-", 0);
			i=0;
			while (pch != NULL && i<DHD_CONF_FILTER_MAX) {
				strcpy(&conf->pkt_filter_add.filter[i][0], pch);
				printf("%s: pkt_filter_add[%d][] = %s\n", __FUNCTION__, i, &conf->pkt_filter_add.filter[i][0]);
				pch = bcmstrtok(&pick_tmp, ",.-", 0);
				i++;
			}
			conf->pkt_filter_add.count = i;
		}

		/* Process pkt_filter_del */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "pkt_filter_del=");
		pick_tmp = pick;
		if (len_val) {
			pch = bcmstrtok(&pick_tmp, " ,.-", 0);
			i=0;
			while (pch != NULL && i<DHD_CONF_FILTER_MAX) {
				conf->pkt_filter_del.id[i] = (uint32)simple_strtol(pch, NULL, 10);
				pch = bcmstrtok(&pick_tmp, " ,.-", 0);
				i++;
			}
			conf->pkt_filter_del.count = i;
			printf("%s: pkt_filter_del id = ", __FUNCTION__);
			for (i=0; i<conf->pkt_filter_del.count; i++)
				printf("%d ", conf->pkt_filter_del.id[i]);
			printf("\n");
		}
#endif

		/* Process srl parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "srl=");
		if (len_val) {
			conf->srl = (int)simple_strtol(pick, NULL, 10);
			printf("%s: srl = %d\n", __FUNCTION__, conf->srl);
		}

		/* Process lrl parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "lrl=");
		if (len_val) {
			conf->lrl = (int)simple_strtol(pick, NULL, 10);
			printf("%s: lrl = %d\n", __FUNCTION__, conf->lrl);
		}

		/* Process beacon timeout parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "bcn_timeout=");
		if (len_val) {
			conf->bcn_timeout= (uint)simple_strtol(pick, NULL, 10);
			printf("%s: bcn_timeout = %d\n", __FUNCTION__, conf->bcn_timeout);
		}

		/* Process ampdu_ba_wsize parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "ampdu_ba_wsize=");
		if (len_val) {
			conf->ampdu_ba_wsize = (int)simple_strtol(pick, NULL, 10);
			printf("%s: ampdu_ba_wsize = %d\n", __FUNCTION__, conf->ampdu_ba_wsize);
		}

		/* Process spect parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "spect=");
		if (len_val) {
			conf->spect = (int)simple_strtol(pick, NULL, 10);
			printf("%s: spect = %d\n", __FUNCTION__, conf->spect);
		}

		/* Process txbf parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "txbf=");
		if (len_val) {
			conf->txbf = (int)simple_strtol(pick, NULL, 10);
			printf("%s: txbf = %d\n", __FUNCTION__, conf->txbf);
		}

		/* Process frameburst parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "frameburst=");
		if (len_val) {
			conf->frameburst = (int)simple_strtol(pick, NULL, 10);
			printf("%s: frameburst = %d\n", __FUNCTION__, conf->frameburst);
		}

		/* Process lpc parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "lpc=");
		if (len_val) {
			conf->lpc = (int)simple_strtol(pick, NULL, 10);
			printf("%s: lpc = %d\n", __FUNCTION__, conf->lpc);
		}

		/* Process dpc_cpucore parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "dpc_cpucore=");
		if (len_val) {
			conf->dpc_cpucore = (int)simple_strtol(pick, NULL, 10);
			printf("%s: dpc_cpucore = %d\n", __FUNCTION__, conf->dpc_cpucore);
		}

		/* Process deepsleep parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "deepsleep=");
		if (len_val) {
			if (!strncmp(pick, "1", len_val))
				conf->deepsleep = TRUE;
			else
				conf->deepsleep = FALSE;
			printf("%s: deepsleep = %d\n", __FUNCTION__, conf->deepsleep);
		}

		/* Process PM parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "PM=");
		if (len_val) {
			conf->pm = (int)simple_strtol(pick, NULL, 10);
			printf("%s: PM = %d\n", __FUNCTION__, conf->pm);
		}

		/* Process pktprio8021x parameters */
		memset(pick, 0, MAXSZ_BUF);
		len_val = process_config_vars(bufp, len, pick, "pktprio8021x=");
		if (len_val) {
			conf->pktprio8021x = (int)simple_strtol(pick, NULL, 10);
			printf("%s: pktprio8021x = %d\n", __FUNCTION__, conf->pktprio8021x);
		}

		bcmerror = 0;
	} else {
		CONFIG_ERROR(("%s: error reading config file: %d\n", __FUNCTION__, len));
		bcmerror = BCME_SDIO_ERROR;
	}

err:
	if (pick)
		MFREE(dhd->osh, pick, MAXSZ_BUF);

	if (memblock)
		MFREE(dhd->osh, memblock, MAXSZ_CONFIG);

	if (image)
		dhd_os_close_image(image);

	return bcmerror;
}

int
dhd_conf_set_chiprev(dhd_pub_t *dhd, uint chip, uint chiprev)
{
	printf("%s: chip=0x%x, chiprev=%d\n", __FUNCTION__, chip, chiprev);
	dhd->conf->chip = chip;
	dhd->conf->chiprev = chiprev;
	return 0;
}

uint
dhd_conf_get_chiprev(void *context)
{
	dhd_pub_t *dhd = context;

	if (dhd && dhd->conf)
		return dhd->conf->chiprev;
	return 0;
}

int
dhd_conf_preinit(dhd_pub_t *dhd)
{
	struct dhd_conf *conf = dhd->conf;

	CONFIG_TRACE(("%s: Enter\n", __FUNCTION__));

	if(!conf) {
		CONFIG_ERROR(("%s: conf not attached\n", __FUNCTION__));
	}

	conf->band = WLC_BAND_AUTO;
	conf->mimo_bw_cap = -1;
	if (conf->chip == BCM43569_CHIP_ID) {
		strcpy(conf->cspec.country_abbrev, "CN");
		strcpy(conf->cspec.ccode, "CN");
		conf->cspec.rev = 38;
	} else {
		strcpy(conf->cspec.country_abbrev, "CN");
		strcpy(conf->cspec.ccode, "CN");
		conf->cspec.rev = 0;
	}
	memset(&conf->channels, 0, sizeof(wl_channel_list_t));
	conf->roam_off = 1;
	conf->roam_off_suspend = 1;
#ifdef CUSTOM_ROAM_TRIGGER_SETTING
	conf->roam_trigger[0] = CUSTOM_ROAM_TRIGGER_SETTING;
#else
	conf->roam_trigger[0] = -65;
#endif
	conf->roam_trigger[1] = WLC_BAND_ALL;
	conf->roam_scan_period[0] = 10;
	conf->roam_scan_period[1] = WLC_BAND_ALL;
#ifdef CUSTOM_ROAM_DELTA_SETTING
	conf->roam_delta[0] = CUSTOM_ROAM_DELTA_SETTING;
#else
	conf->roam_delta[0] = 15;
#endif
	conf->roam_delta[1] = WLC_BAND_ALL;
#ifdef FULL_ROAMING_SCAN_PERIOD_60_SEC
	conf->fullroamperiod = 60;
#else /* FULL_ROAMING_SCAN_PERIOD_60_SEC */
	conf->fullroamperiod = 120;
#endif /* FULL_ROAMING_SCAN_PERIOD_60_SEC */
#ifdef CUSTOM_KEEP_ALIVE_SETTING
	conf->keep_alive_period = CUSTOM_KEEP_ALIVE_SETTING;
#else
	conf->keep_alive_period = 28000;
#endif
	conf->force_wme_ac = 0;
	conf->stbc = -1;
#ifdef PKT_FILTER_SUPPORT
	memset(&conf->pkt_filter_add, 0, sizeof(conf_pkt_filter_add_t));
	memset(&conf->pkt_filter_del, 0, sizeof(conf_pkt_filter_del_t));
#endif
	conf->srl = -1;
	conf->lrl = -1;
	conf->bcn_timeout = 15;
	conf->spect = -1;
	conf->txbf = -1;
	conf->lpc = -1;
	conf->ampdu_ba_wsize = 0;
	conf->dpc_cpucore = 0;
	conf->frameburst = -1;
	conf->deepsleep = FALSE;
	conf->pm = -1;
	conf->pktprio8021x = -1;

	return 0;
}

int
dhd_conf_reset(dhd_pub_t *dhd)
{
	memset(dhd->conf, 0, sizeof(dhd_conf_t));
	return 0;
}

int
dhd_conf_attach(dhd_pub_t *dhd)
{
	dhd_conf_t *conf;

	printf("%s: Enter\n", __FUNCTION__);

	if (dhd->conf != NULL) {
		printf("%s: config is attached before!\n", __FUNCTION__);
		return 0;
	}
	/* Allocate private bus interface state */
	if (!(conf = MALLOC(dhd->osh, sizeof(dhd_conf_t)))) {
		CONFIG_ERROR(("%s: MALLOC failed\n", __FUNCTION__));
		goto fail;
	}
	memset(conf, 0, sizeof(dhd_conf_t));

	dhd->conf = conf;

	return 0;

fail:
	if (conf != NULL)
		MFREE(dhd->osh, conf, sizeof(dhd_conf_t));
	return BCME_NOMEM;
}

void
dhd_conf_detach(dhd_pub_t *dhd)
{
	printf("%s: Enter\n", __FUNCTION__);

	if (dhd->conf) {
		MFREE(dhd->osh, dhd->conf, sizeof(dhd_conf_t));
	}
	dhd->conf = NULL;
}
