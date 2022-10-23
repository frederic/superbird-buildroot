/*
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Fundamental constants relating to DHCP Protocol
 *
 * $Id: bcmdhcp.h 382883 2013-02-04 23:26:09Z $
 */

#ifndef _bcmdhcp_h_
#define _bcmdhcp_h_


#define DHCP_TYPE_OFFSET	0	
#define DHCP_TID_OFFSET		4	
#define DHCP_FLAGS_OFFSET	10	
#define DHCP_CIADDR_OFFSET	12	
#define DHCP_YIADDR_OFFSET	16	
#define DHCP_GIADDR_OFFSET	24	
#define DHCP_CHADDR_OFFSET	28	
#define DHCP_OPT_OFFSET		236	

#define DHCP_OPT_MSGTYPE	53	
#define DHCP_OPT_MSGTYPE_REQ	3
#define DHCP_OPT_MSGTYPE_ACK	5	

#define DHCP_OPT_CODE_OFFSET	0	
#define DHCP_OPT_LEN_OFFSET	1	
#define DHCP_OPT_DATA_OFFSET	2	

#define DHCP_OPT_CODE_CLIENTID	61	

#define DHCP_TYPE_REQUEST	1	
#define DHCP_TYPE_REPLY		2	

#define DHCP_PORT_SERVER	67	
#define DHCP_PORT_CLIENT	68	

#define DHCP_FLAG_BCAST	0x8000	

#define DHCP_FLAGS_LEN	2	

#define DHCP6_TYPE_SOLICIT	1	
#define DHCP6_TYPE_ADVERTISE	2	
#define DHCP6_TYPE_REQUEST	3	
#define DHCP6_TYPE_CONFIRM	4	
#define DHCP6_TYPE_RENEW	5	
#define DHCP6_TYPE_REBIND	6	
#define DHCP6_TYPE_REPLY	7	
#define DHCP6_TYPE_RELEASE	8	
#define DHCP6_TYPE_DECLINE	9	
#define DHCP6_TYPE_RECONFIGURE	10	
#define DHCP6_TYPE_INFOREQ	11	
#define DHCP6_TYPE_RELAYFWD	12	
#define DHCP6_TYPE_RELAYREPLY	13	

#define DHCP6_TYPE_OFFSET	0	

#define	DHCP6_MSG_OPT_OFFSET	4	
#define	DHCP6_RELAY_OPT_OFFSET	34	

#define	DHCP6_OPT_CODE_OFFSET	0	
#define	DHCP6_OPT_LEN_OFFSET	2	
#define	DHCP6_OPT_DATA_OFFSET	4	

#define	DHCP6_OPT_CODE_CLIENTID	1	
#define	DHCP6_OPT_CODE_SERVERID	2	

#define DHCP6_PORT_SERVER	547	
#define DHCP6_PORT_CLIENT	546	

#endif	
