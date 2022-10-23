/*
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Fundamental constants relating to UDP Protocol
 *
 * $Id: bcmudp.h 382882 2013-02-04 23:24:31Z $
 */

#ifndef _bcmudp_h_
#define _bcmudp_h_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif


#include <packed_section_start.h>



#define UDP_DEST_PORT_OFFSET	2	
#define UDP_LEN_OFFSET		4	
#define UDP_CHKSUM_OFFSET	6	

#define UDP_HDR_LEN	8	
#define UDP_PORT_LEN	2	


BWL_PRE_PACKED_STRUCT struct bcmudp_hdr
{
	uint16	src_port;	
	uint16	dst_port;	
	uint16	len;		
	uint16	chksum;		
} BWL_POST_PACKED_STRUCT;


#include <packed_section_end.h>

#endif	
