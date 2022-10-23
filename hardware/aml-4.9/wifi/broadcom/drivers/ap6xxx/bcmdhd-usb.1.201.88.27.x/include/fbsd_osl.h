/*
 * FreeBSD OS Independent Layer
 *
 * Copyright (C) 2015, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: fbsd_osl.h 542557 2015-03-20 05:23:54Z $
 */

#ifndef _bsd_osl_h_
#define _bsd_osl_h_

#include <typedefs.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <sys/resource.h>
#include <machine/param.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#if !defined(__FreeBSD__) /* FreeBSD doesn't seem to use this file anymore */
#include <dev/pci/pcidevs.h>
#endif /* !defined ( __FreeBSD__) */

#include <sys/mbuf.h>

#if defined(__FreeBSD__)
#include <sys/kernel.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#endif

#define OSL_ERR(args)
#define OSL_TRACE(args)
#define OSL_INFO(args)
#define OSL_DBGLOCK(args)

#if defined(__FreeBSD__)
/* Normally... We would expect the following to come from <sys/mbuf.h> but it's not present
 * in the FreeBSD version of this header...  Try this for now.
 */
#define M_GETCTX(m, t)          ((t) (m)->m_pkthdr.rcvif + 0)
#define MCLISREFERENCED(m)      ((m)->m_ext.ext_nextref != (m))

#define unlikely(x)      (x)
#define likely(x)        (x)
#define	FREEBSD_PKTTAG   (0x0114)
#define	FREEBSD_PKTPRIO  (0x0115)
#define OSL_PKTPRIO_SZ   4
#endif /* defined (__FreeBSD__) */

#define FBSD_MTX_DEBUG   0

#if FBSD_MTX_DEBUG
#define MTX_INIT(mutex, name, type, opts)   ({ \
	printf("init mtx from %s:%d\n", __FUNCTION__, __LINE__); \
	mtx_init(mutex, name, type, opts); \
})
#define MTX_DESTROY(mutex)                  ({ \
	printf("destroy mtx from %s:%d\n", __FUNCTION__, __LINE__); \
	mtx_destroy(mutex); \
})
#define MTX_LOCK(mutex)                     ({ \
	printf("lock mtx from %s:%d\n", __FUNCTION__, __LINE__); \
	mtx_lock(mutex); \
})
#define MTX_UNLOCK(mutex)                   ({ \
	printf("unlock mtx from %s:%d\n", __FUNCTION__, __LINE__); \
	mtx_unlock(mutex); \
})
#define MTX_ASSERT(mutex, what)             ({ \
	printf("assert mtx from %s:%d\n", __FUNCTION__, __LINE__); \
	mtx_assert(mutex, what); \
})
#else
#define MTX_INIT          mtx_init
#define MTX_DESTROY       mtx_destroy
#define MTX_LOCK          mtx_lock
#define MTX_UNLOCK        mtx_unlock
#define MTX_ASSERT        mtx_assert
#endif /* FBSD_MTX_DEBUG */

/* The magic cookie */
#define OS_HANDLE_MAGIC		0x1234abcd /* Magic number  for osl_t */

/* Assert */
extern void osl_assert(const char *exp, const char *file, int line);
#define	ASSERT(exp)		do {} while (0)

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint size, uint offset);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCMCIA attribute space access macros, not suppotred */
#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) 	\
	({ \
	 BCM_REFERENCE(osh); \
	 BCM_REFERENCE(buf); \
	 ASSERT(0); \
	 })
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) 	\
	({ \
	 BCM_REFERENCE(osh); \
	 BCM_REFERENCE(buf); \
	 ASSERT(0); \
	 })

/* OSL initialization */
extern osl_t *osl_attach(void *pdev, const char *dev_name, bus_space_tag_t space,
                         bus_space_handle_t handle, uint8 *vaddr);
extern void osl_detach(osl_t *osh);

extern void osl_pktfree_cb_set(osl_t *osh, pktfree_cb_fn_t tx_fn, void *tx_ctx);
#define PKTFREESETCB(osh, tx_fn, tx_ctx) osl_pktfree_cb_set(osh, tx_fn, tx_ctx)

/* Host/bus architecture-specific byte swap */
#ifndef IL_BIGENDIAN
#define BUS_SWAP32(v)		(v)
#else
#define BUS_SWAP32(v)		htol32(v)
#endif

#ifndef bsd_osl_c
/* Undefine the generic BSD kernel MALLOC and MFREE macros to avoid clash
 *
 * Do this only if we are not in bsd_osl.c itself.
 */
#undef MALLOC
#undef MFREE


#define	MALLOC(osh, size)       osl_malloc((osh), (size))
#define MALLOCZ(osh, size)      osl_mallocz((osh), (size))
#define	MFREE(osh, addr, size)  osl_mfree((osh), (addr), (size))
#define MALLOCED(osh)           osl_malloced((osh))
#endif /* bsd_osl_c */

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))

extern void *osl_debug_malloc(osl_t *osh, uint size, int line, const char* file);
extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, const char* file);
struct bcmstrbuf;
extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);
extern void *osl_malloc(osl_t *osh, uint size);
extern void *osl_mallocz(osl_t *osh, uint size);
extern void osl_mfree(osl_t *osh, void *addr, uint size);
extern uint osl_malloced(osl_t *osh);
extern uint osl_malloc_failed(osl_t *osh);

/* Allocate/free shared (dma-able) consistent memory */

#define	DMA_CONSISTENT_ALIGN	PAGE_SIZE

#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap), (dmah))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa), (dmah))

extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align, uint *tot,
	ulong *pap, osldma_t **dmah);

/* Map/unmap direction */
#define	DMA_TX	1 	/* DMA TX flag */
#define	DMA_RX	2	/* DMA RX flag */

/* Map/unmap shared (dma-able) memory */

#define	DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map((osh), (va), (size), (direction), (dmah))
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	osl_dma_unmap((osh), (pa), (size), (direction), (dmah))

extern uint osl_dma_map(osl_t *osh, void *va, uint size, int direction, osldma_t  **dmah);
extern void osl_dma_unmap(osl_t *osh, uint pa, uint size, int direction, osldma_t  **dmah);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth)	BCM_REFERENCE(osh)

/* map/unmap physical to virtual, not supported */

#define	REG_MAP(pa, size)	((void *)NULL)
#define	REG_UNMAP(va)		ASSERT(0)

/* FreeBSD needs 2 handles the bus_space_tag at attach time
 * and the bus_space_handle
 */
/* Pkttag flag should be part of public information */
struct osl_pubinfo {
	bus_space_tag_t 	space;
	bus_space_handle_t	handle;
	bool pkttag;
	bool mmbus;		/* Bus supports memory-mapped register accesses */
	pktfree_cb_fn_t tx_fn;  /* Callback function for PKTFREE */
	void *tx_ctx;		/* Context to the callback function */
	uint8 *vaddr;
};

#define OSL_PUB(osh) ((struct osl_pubinfo *)(osh))

/* IO bus mapping routines */
#define rreg32(osh, r)	bus_space_read_4(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))
#define rreg16(osh, r)	bus_space_read_2(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))
#define rreg8(osh, r)	bus_space_read_1(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				(bus_size_t)(((uintptr)(r)) - \
				((uintptr)(OSL_PUB(osh)->vaddr))))

#define wreg32(osh, r, v)	bus_space_write_4(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint32)(v))
#define wreg16(osh, r, v)	bus_space_write_2(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint16)(v))
#define wreg8(osh, r, v)	bus_space_write_1(OSL_PUB(osh)->space, OSL_PUB(osh)->handle, \
				  (bus_size_t)((uintptr)(r) - \
					((uintptr)(OSL_PUB(osh)->vaddr))), (uint8)(v))

#define	R_REG(osh, r)	((sizeof *(r) == sizeof (uint32))? rreg32((osh), (r)):\
			(uint32)((sizeof(*(r)) == sizeof(uint16))? rreg16((osh), (r)):\
			rreg8((osh), (r))))

#define W_REG(osh, r, v)     do {\
			if (sizeof *(r) == sizeof (uint32)) \
				wreg32((osh), (r), (v)); \
			else if (sizeof *(r) == sizeof (uint16))\
				wreg16((osh), (r), (v)); \
			else \
				wreg8((osh), (r), (v)); \
			} while (0)


#define	AND_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)	W_REG(osh, (r), R_REG(osh, r) | (v))

/* Shared memory access macros */
#define	R_SM(a)		*(a)
#define	W_SM(a, v)	(*(a) = (v))
#define	BZERO_SM(a, len)	bzero((char*)a, len)

/* Uncached/cached virtual address */
#define OSL_UNCACHED(va)	({BCM_REFERENCE(va); ASSERT(0);})
#define OSL_CACHED(va)		({BCM_REFERENCE(va); ASSERT(0);})

#define OSL_PREF_RANGE_LD(va, sz) ({BCM_REFERENCE(va); BCM_REFERENCE(sz);})
#define OSL_PREF_RANGE_ST(va, sz) ({BCM_REFERENCE(va); BCM_REFERENCE(sz);})

/* Get processor cycle count */
#ifdef __i386__
#define	OSL_GETCYCLES(x)	__asm__ __volatile__("rdtsc" : "=a" (x) : : "edx")
#endif /* #ifdef __i386__ */

/* dereference an address that may target abort */
#define	BUSPROBE(val, addr)	osl_busprobe(&(val), (addr))
extern int osl_busprobe(uint32 *val, uint32 addr);

/* Microsecond delay */
#define	OSL_DELAY(usec)		DELAY((usec))

static INLINE void *
osl_pktlink(void *m)
{
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	return ((struct mbuf *)(m))->m_nextpkt;
}

static INLINE void
osl_pktsetlink(void *m, void *x)
{
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	((struct mbuf *)(m))->m_nextpkt = (struct mbuf *)(x);
}

static INLINE void *
osl_pkttag(void *m)
{
#ifdef __FreeBSD__
	struct m_tag	*mtag;
	mtag = m_tag_find((struct mbuf *)m, FREEBSD_PKTTAG, (struct m_tag *)NULL);
	if (mtag == NULL) {
		printf("Did not find FREEBSD_PKTTAG for mbuf %p?\n", m);
		return NULL;
	}
	return ((void*)(mtag + 1));
#else
	ASSERT(((struct mbuf *)(m))->m_flags & M_PKTHDR);
	return (void *)M_GETCTX((struct mbuf *) m, struct mbuf *);
#endif
}

#define OSH_NULL   NULL

#define	MBUF_CHAINLEN(osh, m)   ({BCM_REFERENCE(osh); m_length(m, NULL);})
#define PKTSUMNEEDED(skb)       ({BCM_REFERENCE(skb); 1;})
#define PKTSETSUMGOOD(skb, x)  ({ \
	if (x) \
		((struct mbuf *)(skb))->m_pkthdr.csum_flags |= (CSUM_IP_CHECKED | CSUM_IP_VALID); \
	else \
		((struct mbuf *)(skb))->m_pkthdr.csum_flags &= ~(CSUM_IP_CHECKED | CSUM_IP_VALID); \
})

/* Packet primitives */
#ifdef BCM_OBJECT_TRACE
#define	PKTGET(osh, len, send)	osl_pktget((osh), __LINE__, __FUNCTION__, (len), (send))
#define	PKTFREE(osh, m, send)   osl_pktfree((osh), __LINE__, __FUNCTION__, (m), (send))
#define	PKTDUP(osh, m)          osl_pktdup((osh), __LINE__, __FUNCTION__, (m))
#else
#define	PKTGET(osh, len, send)	osl_pktget((osh), (len), (send))
#define	PKTFREE(osh, m, send)   osl_pktfree((osh), (m), (send))
#define	PKTDUP(osh, m)          osl_pktdup((osh), (m))
#endif /* BCM_OBJECT_TRACE */
#ifdef __FreeBSD__
#define	PKTDATA(osh, m)         ({BCM_REFERENCE(osh); mtod((struct mbuf *)(m), char *);})
#else
#define	PKTDATA(osh, m)         ({BCM_REFERENCE(osh); ((struct mbuf *)(m))->m_data;})
#endif
#define	PKTHEADROOM(osh, m)     ({BCM_REFERENCE(osh); M_LEADINGSPACE((struct mbuf *)(m));})
#define	PKTTAILROOM(osh, m)     ({BCM_REFERENCE(osh); M_TRAILINGSPACE((struct mbuf *)(m));})
#define	PKTSETNEXT(osh, m, x)   osl_pktsetnext((osh), (m), (x))
#define	PKTNEXT(osh, m)         ({BCM_REFERENCE(osh); (((struct mbuf *)(m))->m_next);})
#ifdef __FreeBSD__
#define	PKTPUSH(osh, m, bytes)  osl_pktpush((osh), &(m), (bytes))
#define	PKTSETLEN(osh, m, len)  osl_pktsetlen((osh), (m), (len))
#define	PKTPULL(osh, m, bytes)  m_adj((struct mbuf*)(m), (bytes))
#define	PKTLEN(osh, m)          ({BCM_REFERENCE(osh); (((struct mbuf *)(m))->m_pkthdr.len);})
#else
#define	PKTPUSH(osh, m, bytes)  osl_pktpush((osh), (m), (bytes))
#define	PKTSETLEN(osh, m, len)  ({BCM_REFERENCE(osh); ((struct mbuf *)((m)))->m_len = (len);})
#define	PKTPULL(osh, m, bytes)  osl_pktpull((osh), (m), (bytes))
#define	PKTLEN(osh, m)          ({BCM_REFERENCE(osh); (((struct mbuf *)(m))->m_len);})
#endif
#define	PKTTAG(m)               osl_pkttag((m))
#define	PKTLINK(m)              osl_pktlink((m))
#define	PKTSETLINK(m, x)	    osl_pktsetlink((m), (x))
#ifdef __FreeBSD__
#define PKTFRMNATIVE(osh, m)	BCM_REFERENCE(osh)
#define PKTTONATIVE(osh, pkt)   ({BCM_REFERENCE(osh); pkt;})
#else
#define PKTFRMNATIVE(osh, m)    osl_pkt_frmnative((osh), (struct mbuf *)(m))
#define PKTTONATIVE(osh, p)     osl_pkt_tonative((osh), (p))
#endif
#define PKTSHARED(p)            MCLISREFERENCED((struct mbuf *)(p))
#define PKTALLOCED(osh)         ({BCM_REFERENCE(osh); 0;})
#define PKTSETPOOL(osh, m, x, y) BCM_REFERENCE(osh)
#define PKTPOOL(osh, m)         ({BCM_REFERENCE(osh); FALSE;})
#define PKTFREELIST(m)          PKTLINK(m)
#define PKTSETFREELIST(m, x)    PKTSETLINK((m), (x))
#define PKTPTR(m)               (m)
#define PKTID(m)                ({BCM_REFERENCE(m); 0;})
#define PKTSETID(m, id)         ({BCM_REFERENCE(m); BCM_REFERENCE(id);})
#define PKTLIST_DUMP(osh, buf)	BCM_REFERENCE(osh)
#define PKTSHRINK(osh, m)       ({BCM_REFERENCE(osh); (m);})
#define PKTORPHAN(pkt)          (pkt)


static INLINE uint
osl_pktprio(void *mbuf)
{
	struct m_tag	*mtag;
	uint32_t *v;

	mtag = m_tag_find((struct mbuf *)mbuf, FREEBSD_PKTPRIO, (struct m_tag *)NULL);
	if (mtag == NULL) {
		printf("Did not find FREEBSD_PKTPRIO tag to get for mbuf %p?\n", mbuf);
		return 0;
	}
	v = (uint32_t *)(mtag + 1);

	return *v;
}

static INLINE void
osl_pktsetprio(void *mbuf, uint x)
{
	struct m_tag	*mtag;
	uint32_t *v;

	mtag = m_tag_find((struct mbuf *)mbuf, FREEBSD_PKTPRIO, (struct m_tag *)NULL);
	if (mtag == NULL) {
		printf("Did not find FREEBSD_PKTPRIO tag to set for mbuf %p?\n", mbuf);
		return;
	}
	v = (uint32_t *)(mtag + 1);
	*v = x;

	return;
}

#define	PKTPRIO(m)           osl_pktprio((m))
#define	PKTSETPRIO(m, x)     osl_pktsetprio((m), (x))
#define bcopy(src, dst, len) memcpy((void *) (dst), (void *) (src), (len))


extern void osl_delay(uint usec);
/* extern uint32_t osl_pktprio(void *p); */
extern void osl_setpktprio(void *p, uint32_t x);
extern void osl_pktsetlen(osl_t *osh, void *m, uint len);

/* OSL packet primitive functions  */
extern void *osl_pktget(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	uint len, bool send);
extern void osl_pktfree(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	void *m, bool send);
extern void *osl_pktdup(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	void *m);

extern void *osl_pktpush(osl_t *osh, void **m, int bytes);
extern void *osl_pktpull(osl_t *osh, void *m, int bytes);
extern struct mbuf *osl_pkt_tonative(osl_t *osh, void *p);
extern void *osl_pkt_frmnative(osl_t *osh, struct mbuf *m);
#if !defined(__FreeBSD__) /* Not sure what this is doing here... It's defined as inline \
	above? */
extern void osl_pktsetlink(void *m, void *x);
#endif
extern void osl_pktsetnext(osl_t *osh, void *m, void *x);

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#ifdef __FreeBSD__
#define	PKTBUFSZ	2048   /* packet size */
#else
#define	PKTBUFSZ	MCLBYTES /* packet size */
#endif

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);
#define OSL_PCIE_DOMAIN(osh)	({BCM_REFERENCE(osh); 0;})
#define OSL_PCIE_BUS(osh)	({BCM_REFERENCE(osh); 0;})

/* Translate bcmerrors into FreeBSD errors */
#define OSL_ERROR(bcmerror)	osl_error(bcmerror)
extern int osl_error(int bcmerror);

extern uint32 g_assert_type;

/* get system up time in miliseconds */
#define OSL_SYSUPTIME()		osl_sysuptime()
extern uint32 osl_sysuptime(void);

#endif	/* _bsd_osl_h_ */
