/*
* Copyright (C) 1999-2015, Broadcom Corporation
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
* $Id: flring_fc.h jaganlv $
*
*/
#ifndef __flring_fc_h__
#define __flring_fc_h__

typedef struct flowring_op_data {
	uint16	flowid;
	uint8	handle;
	uint8	tid;
	uint8	ifindex;
	uint8	maxpkts;
	uint8	minpkts;
	uint8	addr[ETHER_ADDR_LEN];
} flowring_op_data_t;

#endif /* __flring_fc_h__ */
