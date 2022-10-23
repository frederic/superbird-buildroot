/*
 * PHY firmware debug interface.
 *
 * Definitions subject to change without notice.
 *
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
 *
 * $Id$
 */

#ifndef _phyioctl_defs_h_
#define _phyioctl_defs_h_

/* Debug message levels */
#define PHYHAL_ERROR		0x0001
#define PHYHAL_TRACE		0x0002
#define PHYHAL_INFORM		0x0004
#define PHYHAL_TMP		0x0008
#define PHYHAL_TXPWR		0x0010
#define PHYHAL_CAL		0x0020
#define PHYHAL_ACI		0x0040
#define PHYHAL_RADAR		0x0080
#define PHYHAL_THERMAL		0x0100
#define PHYHAL_PAPD		0x0200
#define PHYHAL_FCBS		0x0400
#define PHYHAL_RXIQ		0x0800
#define PHYHAL_WD		0x1000
#define PHYHAL_CHANLOG		0x2000

#define PHYHAL_TIMESTAMP	0x8000

#endif /* _phyioctl_defs_h_ */
