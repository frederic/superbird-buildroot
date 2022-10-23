/*
 * RPC OSL
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
 * $Id: rpc_osl.h 306461 2012-01-06 00:11:03Z $
 */

#ifndef _rpcosl_h_
#define _rpcosl_h_

#if (defined BCM_FD_AGGR)
typedef struct rpc_osl rpc_osl_t;
extern rpc_osl_t *rpc_osl_attach(osl_t *osh);
extern void rpc_osl_detach(rpc_osl_t *rpc_osh);

#define RPC_OSL_LOCK(rpc_osh) rpc_osl_lock((rpc_osh))
#define RPC_OSL_UNLOCK(rpc_osh) rpc_osl_unlock((rpc_osh))
#define RPC_OSL_WAIT(rpc_osh, to, ptimedout)	rpc_osl_wait((rpc_osh), (to), (ptimedout))
#define RPC_OSL_WAKE(rpc_osh)			rpc_osl_wake((rpc_osh))
extern void rpc_osl_lock(rpc_osl_t *rpc_osh);
extern void rpc_osl_unlock(rpc_osl_t *rpc_osh);
extern int rpc_osl_wait(rpc_osl_t *rpc_osh, uint ms, bool *ptimedout);
extern void rpc_osl_wake(rpc_osl_t *rpc_osh);

#else
typedef void rpc_osl_t;
#define rpc_osl_attach(a)	(rpc_osl_t *)0x0dadbeef
#define rpc_osl_detach(a)	do { }	while (0)

#define RPC_OSL_LOCK(a)		do { }	while (0)
#define RPC_OSL_UNLOCK(a)	do { }	while (0)
#define RPC_OSL_WAIT(a, b, c)	(TRUE)
#define RPC_OSL_WAKE(a, b)	do { }	while (0)

#endif 
#endif	/* _rpcosl_h_ */
