/*
 * IP Packet Parser Module.
 *
 * Copyright (C) 1999-2016, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: dhd_ip.c 575241 2015-07-29 09:17:04Z $
 */
#include <typedefs.h>
#include <osl.h>

#include <proto/ethernet.h>
#include <proto/vlan.h>
#include <proto/802.3.h>
#include <proto/bcmip.h>
#include <bcmendian.h>

#include <dhd_dbg.h>

#include <dhd_ip.h>


/* special values */
/* 802.3 llc/snap header */
static const uint8 llc_snap_hdr[SNAP_HDR_LEN] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};

#ifdef __FreeBSD__
pkt_frag_t pkt_frag_info(osl_t *osh, void *p)
{
	struct mbuf *m;
	struct ether_header *eh;
	pkt_frag_t ret;
	int len;

	ASSERT(osh && p);

	m = (struct mbuf *)p;
	len = PKTLEN(osh, p);
	ret = DHD_PKT_FRAG_NONE;

	/* Smaller than  ether header */
	if (len < ETHER_HDR_LEN + ETHER_TYPE_LEN)
		return ret;

	/* Smaller than SNAP 802.3 */
	if (len < ETHER_HDR_LEN + SNAP_HDR_LEN + ETHER_TYPE_LEN)
		return ret;

	eh = mtod(m, struct ether_header *);

	/* Not IP packet */
	if (ntoh16(eh->ether_type) != ETHER_TYPE_IP)
		return ret;

	if (m->m_flags & M_FIRSTFRAG){			/* first */

		ret = DHD_PKT_FRAG_FIRST;

	} else if (m->m_flags & M_LASTFRAG){		/* last */

		ret = DHD_PKT_FRAG_LAST;

	} else if ((m->m_flags & M_FRAG) == 0){		/* no frag */

		ret = DHD_PKT_FRAG_NONE;

	} else if (m->m_flags & M_FRAG){		/* frag */

		ret = DHD_PKT_FRAG_CONT;
	}

	return ret;
}
#else
pkt_frag_t pkt_frag_info(osl_t *osh, void *p)
{
	uint8 *frame;
	int length;
	uint8 *pt;			/* Pointer to type field */
	uint16 ethertype;
	struct ipv4_hdr *iph;		/* IP frame pointer */
	int ipl;			/* IP frame length */
	uint16 iph_frag;

	ASSERT(osh && p);

	frame = PKTDATA(osh, p);
	length = PKTLEN(osh, p);

	/* Process Ethernet II or SNAP-encapsulated 802.3 frames */
	if (length < ETHER_HDR_LEN) {
		DHD_INFO(("%s: short eth frame (%d)\n", __FUNCTION__, length));
		return DHD_PKT_FRAG_NONE;
	} else if (ntoh16(*(uint16 *)(frame + ETHER_TYPE_OFFSET)) >= ETHER_TYPE_MIN) {
		/* Frame is Ethernet II */
		pt = frame + ETHER_TYPE_OFFSET;
	} else if (length >= ETHER_HDR_LEN + SNAP_HDR_LEN + ETHER_TYPE_LEN &&
	           !bcmp(llc_snap_hdr, frame + ETHER_HDR_LEN, SNAP_HDR_LEN)) {
		pt = frame + ETHER_HDR_LEN + SNAP_HDR_LEN;
	} else {
		DHD_INFO(("%s: non-SNAP 802.3 frame\n", __FUNCTION__));
		return DHD_PKT_FRAG_NONE;
	}

	ethertype = ntoh16(*(uint16 *)pt);

	/* Skip VLAN tag, if any */
	if (ethertype == ETHER_TYPE_8021Q) {
		pt += VLAN_TAG_LEN;

		if (pt + ETHER_TYPE_LEN > frame + length) {
			DHD_INFO(("%s: short VLAN frame (%d)\n", __FUNCTION__, length));
			return DHD_PKT_FRAG_NONE;
		}

		ethertype = ntoh16(*(uint16 *)pt);
	}

	if (ethertype != ETHER_TYPE_IP) {
		DHD_INFO(("%s: non-IP frame (ethertype 0x%x, length %d)\n",
			__FUNCTION__, ethertype, length));
		return DHD_PKT_FRAG_NONE;
	}

	iph = (struct ipv4_hdr *)(pt + ETHER_TYPE_LEN);
	ipl = (uint)(length - (pt + ETHER_TYPE_LEN - frame));

	/* We support IPv4 only */
	if ((ipl < IPV4_OPTIONS_OFFSET) || (IP_VER(iph) != IP_VER_4)) {
		DHD_INFO(("%s: short frame (%d) or non-IPv4\n", __FUNCTION__, ipl));
		return DHD_PKT_FRAG_NONE;
	}

	iph_frag = ntoh16(iph->frag);

	if (iph_frag & IPV4_FRAG_DONT) {
		return DHD_PKT_FRAG_NONE;
	} else if ((iph_frag & IPV4_FRAG_MORE) == 0) {
		return DHD_PKT_FRAG_LAST;
	} else {
		return (iph_frag & IPV4_FRAG_OFFSET_MASK)? DHD_PKT_FRAG_CONT : DHD_PKT_FRAG_FIRST;
	}
}
#endif /* FreeBSD */
