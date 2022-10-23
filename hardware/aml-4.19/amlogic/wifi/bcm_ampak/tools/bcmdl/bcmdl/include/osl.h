/*
 * OS Abstraction Layer
 *
 * Copyright (C) 2014, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: osl.h 474639 2014-05-01 23:52:31Z $
 */

#ifndef _osl_h_
#define _osl_h_

#include <osl_decl.h>

#define OSL_PKTTAG_SZ	32 


typedef void (*pktfree_cb_fn_t)(void *ctx, void *pkt, unsigned int status);


typedef unsigned int (*osl_rreg_fn_t)(void *ctx, volatile void *reg, unsigned int size);
typedef void  (*osl_wreg_fn_t)(void *ctx, volatile void *reg, unsigned int val, unsigned int size);


#ifdef __mips__
#define PREF_LOAD		0
#define PREF_STORE		1
#define PREF_LOAD_STREAMED	4
#define PREF_STORE_STREAMED	5
#define PREF_LOAD_RETAINED	6
#define PREF_STORE_RETAINED	7
#define PREF_WBACK_INV		25
#define PREF_PREPARE4STORE	30

#define MAKE_PREFETCH_FN(hint) \
static inline void prefetch_##hint(const void *addr) \
{ \
	__asm__ __volatile__(\
	"       .set    mips4           \n" \
	"       pref    %0, (%1)        \n" \
	"       .set    mips0           \n" \
	: \
	: "i" (hint), "r" (addr)); \
}

#define MAKE_PREFETCH_RANGE_FN(hint) \
static inline void prefetch_range_##hint(const void *addr, int len) \
{ \
	int size = len; \
	while (size > 0) { \
		prefetch_##hint(addr); \
		size -= 32; \
	} \
}

MAKE_PREFETCH_FN(PREF_LOAD)
MAKE_PREFETCH_RANGE_FN(PREF_LOAD)
MAKE_PREFETCH_FN(PREF_STORE)
MAKE_PREFETCH_RANGE_FN(PREF_STORE)
MAKE_PREFETCH_FN(PREF_LOAD_STREAMED)
MAKE_PREFETCH_RANGE_FN(PREF_LOAD_STREAMED)
MAKE_PREFETCH_FN(PREF_STORE_STREAMED)
MAKE_PREFETCH_RANGE_FN(PREF_STORE_STREAMED)
MAKE_PREFETCH_FN(PREF_LOAD_RETAINED)
MAKE_PREFETCH_RANGE_FN(PREF_LOAD_RETAINED)
MAKE_PREFETCH_FN(PREF_STORE_RETAINED)
MAKE_PREFETCH_RANGE_FN(PREF_STORE_RETAINED)
#endif 

#include <linux_osl.h>

#ifndef PKTDBG_TRACE
#define PKTDBG_TRACE(osh, pkt, bit)	BCM_REFERENCE(osh)
#endif

#ifndef PKTCTFMAP
#define PKTCTFMAP(osh, p)		BCM_REFERENCE(osh)
#endif 



#define	SET_REG(osh, r, mask, val)	W_REG((osh), (r), ((R_REG((osh), r) & ~(mask)) | (val)))

#ifndef AND_REG
#define AND_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) & (v))
#endif   

#ifndef OR_REG
#define OR_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) | (v))
#endif   

#if !defined(OSL_SYSUPTIME)
#define OSL_SYSUPTIME() (0)
#define OSL_SYSUPTIME_SUPPORT FALSE
#else
#define OSL_SYSUPTIME_SUPPORT TRUE
#endif 

#if !defined(PKTC) && !defined(PKTC_DONGLE)
#define	PKTCGETATTR(skb)	(0)
#define	PKTCSETATTR(skb, f, p, b) BCM_REFERENCE(skb)
#define	PKTCCLRATTR(skb)	BCM_REFERENCE(skb)
#define	PKTCCNT(skb)		(1)
#define	PKTCLEN(skb)		PKTLEN(NULL, skb)
#define	PKTCGETFLAGS(skb)	(0)
#define	PKTCSETFLAGS(skb, f)	BCM_REFERENCE(skb)
#define	PKTCCLRFLAGS(skb)	BCM_REFERENCE(skb)
#define	PKTCFLAGS(skb)		(0)
#define	PKTCSETCNT(skb, c)	BCM_REFERENCE(skb)
#define	PKTCINCRCNT(skb)	BCM_REFERENCE(skb)
#define	PKTCADDCNT(skb, c)	BCM_REFERENCE(skb)
#define	PKTCSETLEN(skb, l)	BCM_REFERENCE(skb)
#define	PKTCADDLEN(skb, l)	BCM_REFERENCE(skb)
#define	PKTCSETFLAG(skb, fb)	BCM_REFERENCE(skb)
#define	PKTCCLRFLAG(skb, fb)	BCM_REFERENCE(skb)
#define	PKTCLINK(skb)		NULL
#define	PKTSETCLINK(skb, x)	BCM_REFERENCE(skb)
#define FOREACH_CHAINED_PKT(skb, nskb) \
	for ((nskb) = NULL; (skb) != NULL; (skb) = (nskb))
#define	PKTCFREE		PKTFREE
#define PKTCENQTAIL(h, t, p) \
do { \
	if ((t) == NULL) { \
		(h) = (t) = (p); \
	} \
} while (0)
#endif 

#if !defined(HNDCTF) && !defined(PKTC_TX_DONGLE)
#define PKTSETCHAINED(osh, skb)		BCM_REFERENCE(osh)
#define PKTCLRCHAINED(osh, skb)		BCM_REFERENCE(osh)
#define PKTISCHAINED(skb)		FALSE
#endif


#define PKTFRAGPKTID(osh, lb)		(0)
#define PKTSETFRAGPKTID(osh, lb, id)	BCM_REFERENCE(osh)
#define PKTFRAGTOTNUM(osh, lb)		(0)
#define PKTSETFRAGTOTNUM(osh, lb, tot)	BCM_REFERENCE(osh)
#define PKTFRAGTOTLEN(osh, lb)		(0)
#define PKTSETFRAGTOTLEN(osh, lb, len)	BCM_REFERENCE(osh)
#define PKTIFINDEX(osh, lb)		(0)
#define PKTSETIFINDEX(osh, lb, idx)	BCM_REFERENCE(osh)
#define	PKTGETLF(osh, len, send, lbuf_type)	(0)


#define PKTFRAGUSEDLEN(osh, lb)			(0)
#define PKTSETFRAGUSEDLEN(osh, lb, len)		BCM_REFERENCE(osh)

#define PKTFRAGLEN(osh, lb, ix)			(0)
#define PKTSETFRAGLEN(osh, lb, ix, len)		BCM_REFERENCE(osh)
#define PKTFRAGDATA_LO(osh, lb, ix)		(0)
#define PKTSETFRAGDATA_LO(osh, lb, ix, addr)	BCM_REFERENCE(osh)
#define PKTFRAGDATA_HI(osh, lb, ix)		(0)
#define PKTSETFRAGDATA_HI(osh, lb, ix, addr)	BCM_REFERENCE(osh)


#define PKTISRXFRAG(osh, lb)    	(0)
#define PKTSETRXFRAG(osh, lb)		BCM_REFERENCE(osh)
#define PKTRESETRXFRAG(osh, lb)		BCM_REFERENCE(osh)


#define PKTISTXFRAG(osh, lb)		(0)
#define PKTSETTXFRAG(osh, lb)		BCM_REFERENCE(osh)


#define PKTNEEDRXCPL(osh, lb)           (TRUE)
#define PKTSETNORXCPL(osh, lb)          BCM_REFERENCE(osh)
#define PKTRESETNORXCPL(osh, lb)        BCM_REFERENCE(osh)

#define PKTISFRAG(osh, lb)		(0)
#define PKTFRAGISCHAINED(osh, i)	(0)

#define PKTFRAG_TRIM_TAILBYTES(osh, p, len)	PKTSETLEN(osh, p, PKTLEN(osh, p) - len)

#endif	
