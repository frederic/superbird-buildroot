/*
 * Trace log blocks sent over HBUS
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
 * $Id: logtrace.h 333856 2012-05-17 23:43:07Z $
 */

#ifndef	_LOGTRACE_H
#define	_LOGTRACE_H

#include <msgtrace.h>
#include <osl_decl.h>
extern void logtrace_start(void);
extern void logtrace_stop(void);
extern int logtrace_sent(void);
extern void logtrace_trigger(void);
extern void logtrace_init(void *hdl1, void *hdl2, msgtrace_func_send_t func_send);
extern bool logtrace_event_enabled(void);

#endif	/* _LOGTRACE_H */
