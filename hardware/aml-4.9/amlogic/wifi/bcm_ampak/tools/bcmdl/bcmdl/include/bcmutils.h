/*
 * Misc useful os-independent macros and functions.
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
 * $Id: bcmutils.h 484281 2014-06-12 22:42:26Z $
 */

#ifndef	_bcmutils_h_
#define	_bcmutils_h_

#define LOG_TAG "bcmdl"
#ifdef TARGETENV_android
#include <cutils/log.h>
#undef fprintf
#define fprintf(x, ...) \
  { if(x==stderr) ALOGE(__VA_ARGS__); else fprintf(x, __VA_ARGS__); printf(__VA_ARGS__); }
#endif

#define bcm_strcpy_s(dst, noOfElements, src)            strcpy((dst), (src))
#define bcm_strncpy_s(dst, noOfElements, src, count)    strncpy((dst), (src), (count))
#define bcm_strcat_s(dst, noOfElements, src)            strcat((dst), (src))

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__FreeBSD__)
#include <stdbool.h>
#endif

#ifdef PKTQ_LOG
#include <wlioctl.h>
#endif


#define _BCM_U	0x01	
#define _BCM_L	0x02	
#define _BCM_D	0x04	
#define _BCM_C	0x08	
#define _BCM_P	0x10	
#define _BCM_S	0x20	
#define _BCM_X	0x40	
#define _BCM_SP	0x80	

extern const unsigned char bcm_ctype[];
#define bcm_ismask(x)	(bcm_ctype[(int)(unsigned char)(x)])

#define bcm_isalnum(c)	((bcm_ismask(c)&(_BCM_U|_BCM_L|_BCM_D)) != 0)
#define bcm_isalpha(c)	((bcm_ismask(c)&(_BCM_U|_BCM_L)) != 0)
#define bcm_iscntrl(c)	((bcm_ismask(c)&(_BCM_C)) != 0)
#define bcm_isdigit(c)	((bcm_ismask(c)&(_BCM_D)) != 0)
#define bcm_isgraph(c)	((bcm_ismask(c)&(_BCM_P|_BCM_U|_BCM_L|_BCM_D)) != 0)
#define bcm_islower(c)	((bcm_ismask(c)&(_BCM_L)) != 0)
#define bcm_isprint(c)	((bcm_ismask(c)&(_BCM_P|_BCM_U|_BCM_L|_BCM_D|_BCM_SP)) != 0)
#define bcm_ispunct(c)	((bcm_ismask(c)&(_BCM_P)) != 0)
#define bcm_isspace(c)	((bcm_ismask(c)&(_BCM_S)) != 0)
#define bcm_isupper(c)	((bcm_ismask(c)&(_BCM_U)) != 0)
#define bcm_isxdigit(c)	((bcm_ismask(c)&(_BCM_D|_BCM_X)) != 0)
#define bcm_tolower(c)	(bcm_isupper((c)) ? ((c) + 'a' - 'A') : (c))
#define bcm_toupper(c)	(bcm_islower((c)) ? ((c) + 'A' - 'a') : (c))

#define CIRCULAR_ARRAY_FULL(rd_idx, wr_idx, max) ((wr_idx + 1)%max == rd_idx)



struct bcmstrbuf {
	char *buf;	
	unsigned int size;	
	char *origbuf;	
	unsigned int origsize;	
};


#ifdef BCMDRIVER
#include <osl.h>
#include <hnd_pktq.h>
#include <hnd_pktpool.h>

#define GPIO_PIN_NOTDEFINED 	0x20	


#ifndef SPINWAIT_POLL_PERIOD
#define SPINWAIT_POLL_PERIOD	10
#endif

#define SPINWAIT(exp, us) { \
	uint countdown = (us) + (SPINWAIT_POLL_PERIOD - 1); \
	while ((exp) && (countdown >= SPINWAIT_POLL_PERIOD)) { \
		OSL_DELAY(SPINWAIT_POLL_PERIOD); \
		countdown -= SPINWAIT_POLL_PERIOD; \
	} \
}



struct ether_addr;

extern int ether_isbcast(const void *ea);
extern int ether_isnulladdr(const void *ea);

#define BCM_MAC_RXCPL_IDX_BITS			12
#define BCM_MAX_RXCPL_IDX_INVALID		0
#define BCM_MAC_RXCPL_IFIDX_BITS		3
#define BCM_MAC_RXCPL_DOT11_BITS		1
#define BCM_MAX_RXCPL_IFIDX			((1 << BCM_MAC_RXCPL_IFIDX_BITS) - 1)
#define BCM_MAC_RXCPL_FLAG_BITS			4
#define BCM_RXCPL_FLAGS_IN_TRANSIT		0x1
#define BCM_RXCPL_FLAGS_FIRST_IN_FLUSHLIST	0x2
#define BCM_RXCPL_FLAGS_RXCPLVALID		0x4
#define BCM_RXCPL_FLAGS_RSVD			0x8

#define BCM_RXCPL_SET_IN_TRANSIT(a)	((a)->rxcpl_id.flags |= BCM_RXCPL_FLAGS_IN_TRANSIT)
#define BCM_RXCPL_CLR_IN_TRANSIT(a)	((a)->rxcpl_id.flags &= ~BCM_RXCPL_FLAGS_IN_TRANSIT)
#define BCM_RXCPL_IN_TRANSIT(a)		((a)->rxcpl_id.flags & BCM_RXCPL_FLAGS_IN_TRANSIT)

#define BCM_RXCPL_SET_FRST_IN_FLUSH(a)	((a)->rxcpl_id.flags |= BCM_RXCPL_FLAGS_FIRST_IN_FLUSHLIST)
#define BCM_RXCPL_CLR_FRST_IN_FLUSH(a)	((a)->rxcpl_id.flags &= ~BCM_RXCPL_FLAGS_FIRST_IN_FLUSHLIST)
#define BCM_RXCPL_FRST_IN_FLUSH(a)	((a)->rxcpl_id.flags & BCM_RXCPL_FLAGS_FIRST_IN_FLUSHLIST)

#define BCM_RXCPL_SET_VALID_INFO(a)	((a)->rxcpl_id.flags |= BCM_RXCPL_FLAGS_RXCPLVALID)
#define BCM_RXCPL_CLR_VALID_INFO(a)	((a)->rxcpl_id.flags &= ~BCM_RXCPL_FLAGS_RXCPLVALID)
#define BCM_RXCPL_VALID_INFO(a) (((a)->rxcpl_id.flags & BCM_RXCPL_FLAGS_RXCPLVALID) ? TRUE : FALSE)


struct reorder_rxcpl_id_list {
	uint16 head;
	uint16 tail;
	uint32 cnt;
};

typedef struct rxcpl_id {
	uint32		idx : BCM_MAC_RXCPL_IDX_BITS;
	uint32		next_idx : BCM_MAC_RXCPL_IDX_BITS;
	uint32		ifidx : BCM_MAC_RXCPL_IFIDX_BITS;
	uint32		dot11 : BCM_MAC_RXCPL_DOT11_BITS;
	uint32		flags : BCM_MAC_RXCPL_FLAG_BITS;
} rxcpl_idx_id_t;

typedef struct rxcpl_data_len {
	uint32		metadata_len_w : 6;
	uint32		dataoffset: 10;
	uint32		datalen : 16;
} rxcpl_data_len_t;

typedef struct rxcpl_info {
	rxcpl_idx_id_t		rxcpl_id;
	uint32		host_pktref;
	union {
		rxcpl_data_len_t	rxcpl_len;
		struct rxcpl_info	*free_next;
	};
} rxcpl_info_t;


typedef struct bcm_rxcplid_list {
	uint32			max;
	uint32			avail;
	rxcpl_info_t		*rxcpl_ptr;
	rxcpl_info_t		*free_list;
} bcm_rxcplid_list_t;

extern bool BCMATTACHFN(bcm_alloc_rxcplid_list)(osl_t *osh, uint32 max);
extern rxcpl_info_t * bcm_alloc_rxcplinfo(void);
extern void bcm_free_rxcplinfo(rxcpl_info_t *ptr);
extern void bcm_chain_rxcplid(uint16 first,  uint16 next);
extern rxcpl_info_t *bcm_id2rxcplinfo(uint16 id);
extern uint16 bcm_rxcplinfo2id(rxcpl_info_t *ptr);
extern rxcpl_info_t *bcm_rxcpllist_end(rxcpl_info_t *ptr, uint32 *count);



extern uint pktcopy(osl_t *osh, void *p, uint offset, int len, uchar *buf);
extern uint pktfrombuf(osl_t *osh, void *p, uint offset, int len, uchar *buf);
extern uint pkttotlen(osl_t *osh, void *p);
extern void *pktlast(osl_t *osh, void *p);
extern uint pktsegcnt(osl_t *osh, void *p);
extern uint pktsegcnt_war(osl_t *osh, void *p);
extern uint8 *pktdataoffset(osl_t *osh, void *p,  uint offset);
extern void *pktoffset(osl_t *osh, void *p,  uint offset);


#define	PKTPRIO_VDSCP	0x100		
#define	PKTPRIO_VLAN	0x200		
#define	PKTPRIO_UPD	0x400		
#define	PKTPRIO_DSCP	0x800		



#define DSCP_AF11	0x0A
#define DSCP_AF12	0x0C
#define DSCP_AF13	0x0E

#define DSCP_AF21	0x12
#define DSCP_AF22	0x14
#define DSCP_AF23	0x16

#define DSCP_AF31	0x1A
#define DSCP_AF32	0x1C
#define DSCP_AF33	0x1E

#define DSCP_EF		0x2E

extern uint pktsetprio(void *pkt, bool update_vtag);
extern bool pktgetdscp(uint8 *pktdata, uint pktlen, uint8 *dscp);


extern int bcm_atoi(const char *s);
extern ulong bcm_strtoul(const char *cp, char **endp, uint base);
extern char *bcmstrstr(const char *haystack, const char *needle);
extern char *bcmstrnstr(const char *s, uint s_len, const char *substr, uint substr_len);
extern char *bcmstrcat(char *dest, const char *src);
extern char *bcmstrncat(char *dest, const char *src, uint size);
extern ulong wchar2ascii(char *abuf, ushort *wbuf, ushort wbuflen, ulong abuflen);
char* bcmstrtok(char **string, const char *delimiters, char *tokdelim);
int bcmstricmp(const char *s1, const char *s2);
int bcmstrnicmp(const char* s1, const char* s2, int cnt);



extern char *bcm_ether_ntoa(const struct ether_addr *ea, char *buf);
extern int bcm_ether_atoe(const char *p, struct ether_addr *ea);


struct ipv4_addr;
extern char *bcm_ip_ntoa(struct ipv4_addr *ia, char *buf);
extern char *bcm_ipv6_ntoa(void *ipv6, char *buf);
extern int bcm_atoipv4(const char *p, struct ipv4_addr *ip);


extern void bcm_mdelay(uint ms);

#define NVRAM_RECLAIM_CHECK(name)

extern char *getvar(char *vars, const char *name);
extern int getintvar(char *vars, const char *name);
extern int getintvararray(char *vars, const char *name, int index);
extern int getintvararraysize(char *vars, const char *name);
extern uint getgpiopin(char *vars, char *pin_name, uint def_pin);
#define bcm_perf_enable()
#define bcmstats(fmt)
#define	bcmlog(fmt, a1, a2)
#define	bcmdumplog(buf, size)	*buf = '\0'
#define	bcmdumplogent(buf, idx)	-1

#define TSF_TICKS_PER_MS	1000
#define TS_ENTER		0xdeadbeef	
#define TS_EXIT			0xbeefcafe	

#define bcmtslog(tstamp, fmt, a1, a2)
#define bcmprinttslogs()
#define bcmprinttstamp(us)
#define bcmdumptslog(buf, size)

extern char *bcm_nvram_vars(uint *length);
extern int bcm_nvram_cache(void *sih);




typedef struct bcm_iovar {
	const char *name;	
	uint16 varid;		
	uint16 flags;		
	uint16 type;		
	uint16 minlen;		
} bcm_iovar_t;




#define IOV_GET 0 
#define IOV_SET 1 


#define IOV_GVAL(id)		((id) * 2)
#define IOV_SVAL(id)		((id) * 2 + IOV_SET)
#define IOV_ISSET(actionid)	((actionid & IOV_SET) == IOV_SET)
#define IOV_ID(actionid)	(actionid >> 1)



extern const bcm_iovar_t *bcm_iovar_lookup(const bcm_iovar_t *table, const char *name);
extern int bcm_iovar_lencheck(const bcm_iovar_t *table, void *arg, int len, bool set);
#if defined(WLTINYDUMP) || defined(WLMSG_INFORM) || defined(WLMSG_ASSOC) || \
	defined(WLMSG_PRPKT) || defined(WLMSG_WSEC)
extern int bcm_format_ssid(char* buf, const uchar ssid[], uint ssid_len);
#endif 
#endif	


#define IOVT_VOID	0	
#define IOVT_BOOL	1	
#define IOVT_INT8	2	
#define IOVT_UINT8	3	
#define IOVT_INT16	4	
#define IOVT_UINT16	5	
#define IOVT_INT32	6	
#define IOVT_UINT32	7	
#define IOVT_BUFFER	8	
#define BCM_IOVT_VALID(type) (((unsigned int)(type)) <= IOVT_BUFFER)


#define BCM_IOV_TYPE_INIT { \
	"void", \
	"bool", \
	"int8", \
	"uint8", \
	"int16", \
	"uint16", \
	"int32", \
	"uint32", \
	"buffer", \
	"" }

#define BCM_IOVT_IS_INT(type) (\
	(type == IOVT_BOOL) || \
	(type == IOVT_INT8) || \
	(type == IOVT_UINT8) || \
	(type == IOVT_INT16) || \
	(type == IOVT_UINT16) || \
	(type == IOVT_INT32) || \
	(type == IOVT_UINT32))



#define BCME_STRLEN 		64	
#define VALID_BCMERROR(e)  ((e <= 0) && (e >= BCME_LAST))




#define BCME_OK				0	
#define BCME_ERROR			-1	
#define BCME_BADARG			-2	
#define BCME_BADOPTION			-3	
#define BCME_NOTUP			-4	
#define BCME_NOTDOWN			-5	
#define BCME_NOTAP			-6	
#define BCME_NOTSTA			-7	
#define BCME_BADKEYIDX			-8	
#define BCME_RADIOOFF 			-9	
#define BCME_NOTBANDLOCKED		-10	
#define BCME_NOCLK			-11	
#define BCME_BADRATESET			-12	
#define BCME_BADBAND			-13	
#define BCME_BUFTOOSHORT		-14	
#define BCME_BUFTOOLONG			-15	
#define BCME_BUSY			-16	
#define BCME_NOTASSOCIATED		-17	
#define BCME_BADSSIDLEN			-18	
#define BCME_OUTOFRANGECHAN		-19	
#define BCME_BADCHAN			-20	
#define BCME_BADADDR			-21	
#define BCME_NORESOURCE			-22	
#define BCME_UNSUPPORTED		-23	
#define BCME_BADLEN			-24	
#define BCME_NOTREADY			-25	
#define BCME_EPERM			-26	
#define BCME_NOMEM			-27	
#define BCME_ASSOCIATED			-28	
#define BCME_RANGE			-29	
#define BCME_NOTFOUND			-30	
#define BCME_WME_NOT_ENABLED		-31	
#define BCME_TSPEC_NOTFOUND		-32	
#define BCME_ACM_NOTSUPPORTED		-33	
#define BCME_NOT_WME_ASSOCIATION	-34	
#define BCME_SDIO_ERROR			-35	
#define BCME_DONGLE_DOWN		-36	
#define BCME_VERSION			-37 	
#define BCME_TXFAIL			-38 	
#define BCME_RXFAIL			-39	
#define BCME_NODEVICE			-40 	
#define BCME_NMODE_DISABLED		-41 	
#define BCME_NONRESIDENT		-42 
#define BCME_SCANREJECT			-43 	
#define BCME_USAGE_ERROR                -44     
#define BCME_IOCTL_ERROR                -45     
#define BCME_SERIAL_PORT_ERR            -46     
#define BCME_DISABLED			-47     
#define BCME_DECERR				-48		
#define BCME_ENCERR				-49		
#define BCME_MICERR				-50		
#define BCME_REPLAY				-51		
#define BCME_IE_NOTFOUND		-52		
#define BCME_LAST			BCME_IE_NOTFOUND

#define BCME_NOTENABLED BCME_DISABLED


#define BCMERRSTRINGTABLE {		\
	"OK",				\
	"Undefined error",		\
	"Bad Argument",			\
	"Bad Option",			\
	"Not up",			\
	"Not down",			\
	"Not AP",			\
	"Not STA",			\
	"Bad Key Index",		\
	"Radio Off",			\
	"Not band locked",		\
	"No clock",			\
	"Bad Rate valueset",		\
	"Bad Band",			\
	"Buffer too short",		\
	"Buffer too long",		\
	"Busy",				\
	"Not Associated",		\
	"Bad SSID len",			\
	"Out of Range Channel",		\
	"Bad Channel",			\
	"Bad Address",			\
	"Not Enough Resources",		\
	"Unsupported",			\
	"Bad length",			\
	"Not Ready",			\
	"Not Permitted",		\
	"No Memory",			\
	"Associated",			\
	"Not In Range",			\
	"Not Found",			\
	"WME Not Enabled",		\
	"TSPEC Not Found",		\
	"ACM Not Supported",		\
	"Not WME Association",		\
	"SDIO Bus Error",		\
	"Dongle Not Accessible",	\
	"Incorrect version",		\
	"TX Failure",			\
	"RX Failure",			\
	"Device Not Present",		\
	"NMODE Disabled",		\
	"Nonresident overlay access", \
	"Scan Rejected",		\
	"WLCMD usage error",		\
	"WLCMD ioctl error",		\
	"RWL serial port error", 	\
	"Disabled",			\
	"Decrypt error", \
	"Encrypt error", \
	"MIC error", \
	"Replay", \
	"IE not found", \
}

#ifndef ABS
#define	ABS(a)			(((a) < 0) ? -(a) : (a))
#endif 

#ifndef MIN
#define	MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif 

#ifndef MAX
#define	MAX(a, b)		(((a) > (b)) ? (a) : (b))
#endif 


#ifndef LIMIT_TO_RANGE
#define LIMIT_TO_RANGE(x, min, max) \
	((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#endif 


#ifndef LIMIT_TO_MAX
#define LIMIT_TO_MAX(x, max) \
	(((x) > (max) ? (max) : (x)))
#endif 


#ifndef LIMIT_TO_MIN
#define LIMIT_TO_MIN(x, min) \
	(((x) < (min) ? (min) : (x)))
#endif 

#define DELTA(curr, prev) ((curr) > (prev) ? ((curr) - (prev)) : \
	(0xffffffff - (prev) + (curr) + 1))
#define CEIL(x, y)		(((x) + ((y) - 1)) / (y))
#define ROUNDUP(x, y)		((((x) + ((y) - 1)) / (y)) * (y))
#define ROUNDDN(p, align)	((p) & ~((align) - 1))
#define	ISALIGNED(a, x)		(((uintptr)(a) & ((x) - 1)) == 0)
#define ALIGN_ADDR(addr, boundary) (void *)(((uintptr)(addr) + (boundary) - 1) \
	                                         & ~((boundary) - 1))
#define ALIGN_SIZE(size, boundary) (((size) + (boundary) - 1) \
	                                         & ~((boundary) - 1))
#define	ISPOWEROF2(x)		((((x) - 1) & (x)) == 0)
#define VALID_MASK(mask)	!((mask) & ((mask) + 1))

#ifndef OFFSETOF
#ifdef __ARMCC_VERSION

#include <stddef.h>
#define	OFFSETOF(type, member)	offsetof(type, member)
#else
#  if ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 8))

#    define	OFFSETOF(type, member)	__builtin_offsetof(type, member)
#  else
#    define	OFFSETOF(type, member)	((uint)(uintptr)&((type *)0)->member)
#  endif 
#endif 
#endif 

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)		(sizeof(a) / sizeof(a[0]))
#endif

#ifndef ARRAYLAST 
#define ARRAYLAST(a)		(&a[ARRAYSIZE(a)-1])
#endif


extern void *_bcmutils_dummy_fn;
#define REFERENCE_FUNCTION(f)	(_bcmutils_dummy_fn = (void *)(f))


#ifndef setbit
#ifndef NBBY		
#define	NBBY	8	
#endif 
#ifdef BCMUTILS_BIT_MACROS_USE_FUNCS
extern void setbit(void *array, uint bit);
extern void clrbit(void *array, uint bit);
extern bool isset(const void *array, uint bit);
extern bool isclr(const void *array, uint bit);
#else
#define	setbit(a, i)	(((uint8 *)a)[(i) / NBBY] |= 1 << ((i) % NBBY))
#define	clrbit(a, i)	(((uint8 *)a)[(i) / NBBY] &= ~(1 << ((i) % NBBY)))
#define	isset(a, i)	(((const uint8 *)a)[(i) / NBBY] & (1 << ((i) % NBBY)))
#define	isclr(a, i)	((((const uint8 *)a)[(i) / NBBY] & (1 << ((i) % NBBY))) == 0)
#endif
#endif 
extern void set_bitrange(void *array, uint start, uint end, uint maxbit);

#define	isbitset(a, i)	(((a) & (1 << (i))) != 0)

#define	NBITS(type)	(sizeof(type) * 8)
#define NBITVAL(nbits)	(1 << (nbits))
#define MAXBITVAL(nbits)	((1 << (nbits)) - 1)
#define	NBITMASK(nbits)	MAXBITVAL(nbits)
#define MAXNBVAL(nbyte)	MAXBITVAL((nbyte) * 8)

extern void bcm_bitprint32(const uint32 u32);



#define DECLARE_MAP_API(NB, RSH, LSH, OFF, MSK)                     \
static INLINE void setbit##NB(void *ptr, uint32 ix, uint32 val)     \
{                                                                   \
	uint32 *addr = (uint32 *)ptr;                                   \
	uint32 *a = addr + (ix >> RSH);               \
	uint32 pos = (ix & OFF) << LSH;       \
	uint32 mask = (MSK << pos);                                     \
	uint32 tmp = *a & ~mask;                                        \
	*a = tmp | (val << pos);                                        \
}                                                                   \
static INLINE uint32 getbit##NB(void *ptr, uint32 ix)               \
{                                                                   \
	uint32 *addr = (uint32 *)ptr;                                   \
	uint32 *a = addr + (ix >> RSH);                                 \
	uint32 pos = (ix & OFF) << LSH;                                 \
	return ((*a >> pos) & MSK);                                     \
}

DECLARE_MAP_API(2, 4, 1, 15U, 0x0003) 
DECLARE_MAP_API(4, 3, 2, 7U, 0x000F) 
DECLARE_MAP_API(8, 2, 3, 3U, 0x00FF) 


#define MUX(pred, true, false) ((pred) ? (true) : (false))


#define MODDEC(x, bound) MUX((x) == 0, (bound) - 1, (x) - 1)
#define MODINC(x, bound) MUX((x) == (bound) - 1, 0, (x) + 1)


#define MODDEC_POW2(x, bound) (((x) - 1) & ((bound) - 1))
#define MODINC_POW2(x, bound) (((x) + 1) & ((bound) - 1))


#define MODADD(x, y, bound) \
    MUX((x) + (y) >= (bound), (x) + (y) - (bound), (x) + (y))
#define MODSUB(x, y, bound) \
    MUX(((int)(x)) - ((int)(y)) < 0, (x) - (y) + (bound), (x) - (y))


#define MODADD_POW2(x, y, bound) (((x) + (y)) & ((bound) - 1))
#define MODSUB_POW2(x, y, bound) (((x) - (y)) & ((bound) - 1))


#define CRC8_INIT_VALUE  0xff		
#define CRC8_GOOD_VALUE  0x9f		
#define CRC16_INIT_VALUE 0xffff		
#define CRC16_GOOD_VALUE 0xf0b8		
#define CRC32_INIT_VALUE 0xffffffff	
#define CRC32_GOOD_VALUE 0xdebb20e3	


#define MACF				"%02x:%02x:%02x:%02x:%02x:%02x"
#define ETHERP_TO_MACF(ea)	((struct ether_addr *) (ea))->octet[0], \
							((struct ether_addr *) (ea))->octet[1], \
							((struct ether_addr *) (ea))->octet[2], \
							((struct ether_addr *) (ea))->octet[3], \
							((struct ether_addr *) (ea))->octet[4], \
							((struct ether_addr *) (ea))->octet[5]

#define ETHER_TO_MACF(ea) 	(ea).octet[0], \
							(ea).octet[1], \
							(ea).octet[2], \
							(ea).octet[3], \
							(ea).octet[4], \
							(ea).octet[5]
#if !defined(SIMPLE_MAC_PRINT)
#define MACDBG "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STRDBG(ea) (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]
#else
#define MACDBG				"%02x:%02x:%02x"
#define MAC2STRDBG(ea) (ea)[0], (ea)[4], (ea)[5]
#endif 


typedef struct bcm_bit_desc {
	uint32	bit;
	const char* name;
} bcm_bit_desc_t;


typedef struct bcm_bit_desc_ex {
	uint32 mask;
	const bcm_bit_desc_t *bitfield;
} bcm_bit_desc_ex_t;


#define ETHER_ADDR_STR_LEN	18	



static INLINE void
xor_128bit_block(const uint8 *src1, const uint8 *src2, uint8 *dst)
{
	if (
#ifdef __i386__
	    1 ||
#endif
	    (((uintptr)src1 | (uintptr)src2 | (uintptr)dst) & 3) == 0) {
		
		
		((uint32 *)dst)[0] = ((const uint32 *)src1)[0] ^ ((const uint32 *)src2)[0];
		((uint32 *)dst)[1] = ((const uint32 *)src1)[1] ^ ((const uint32 *)src2)[1];
		((uint32 *)dst)[2] = ((const uint32 *)src1)[2] ^ ((const uint32 *)src2)[2];
		((uint32 *)dst)[3] = ((const uint32 *)src1)[3] ^ ((const uint32 *)src2)[3];
	} else {
		
		int k;
		for (k = 0; k < 16; k++)
			dst[k] = src1[k] ^ src2[k];
	}
}



extern uint8 hndcrc8(uint8 *p, uint nbytes, uint8 crc);
extern uint16 hndcrc16(uint8 *p, uint nbytes, uint16 crc);
extern uint32 hndcrc32(uint8 *p, uint nbytes, uint32 crc);


#if defined(DHD_DEBUG) || defined(WLMSG_PRHDRS) || defined(WLMSG_PRPKT) || \
	defined(WLMSG_ASSOC)

extern int bcm_format_field(const bcm_bit_desc_ex_t *bd, uint32 field, char* buf, int len);

extern int bcm_format_flags(const bcm_bit_desc_t *bd, uint32 flags, char* buf, int len);
#endif

#if defined(DHD_DEBUG) || defined(WLMSG_PRHDRS) || defined(WLMSG_PRPKT) || \
	defined(WLMSG_ASSOC) || defined(WLMEDIA_PEAKRATE)
extern int bcm_format_hex(char *str, const void *bytes, int len);
#endif

extern const char *bcm_crypto_algo_name(uint algo);
extern char *bcm_chipname(uint chipid, char *buf, uint len);
extern char *bcm_brev_str(uint32 brev, char *buf);
extern void printbig(char *buf);
extern void prhex(const char *msg, uchar *buf, uint len);




typedef struct bcm_tlv {
	uint8	id;
	uint8	len;
	uint8	data[1];
} bcm_tlv_t;


typedef struct bcm_xtlv {
	uint16	id;
	uint16	len;
	uint8	data[1];
} bcm_xtlv_t;

#define BCM_TLV_MAX_DATA_SIZE (255)
#define BCM_XTLV_MAX_DATA_SIZE (65535)
#define BCM_TLV_HDR_SIZE (OFFSETOF(bcm_tlv_t, data))

#define BCM_XTLV_HDR_SIZE (OFFSETOF(bcm_xtlv_t, data))
#define BCM_XTLV_LEN(elt) ltoh16_ua(&(elt->len))
#define BCM_XTLV_ID(elt) ltoh16_ua(&(elt->id))
#define BCM_XTLV_SIZE(elt) (BCM_XTLV_HDR_SIZE + BCM_XTLV_LEN(elt))


#define bcm_valid_tlv(elt, buflen) (\
	 ((int)(buflen) >= (int)BCM_TLV_HDR_SIZE) && \
	 ((int)(buflen) >= (int)(BCM_TLV_HDR_SIZE + (elt)->len)))

#define bcm_valid_xtlv(elt, buflen) (\
	 ((int)(buflen) >= (int)BCM_XTLV_HDR_SIZE) && \
	 ((int)(buflen) >= (int)BCM_XTLV_SIZE(elt)))

extern bcm_tlv_t *bcm_next_tlv(bcm_tlv_t *elt, int *buflen);
extern bcm_tlv_t *bcm_parse_tlvs(void *buf, int buflen, uint key);
extern bcm_tlv_t *bcm_parse_tlvs_min_bodylen(void *buf, int buflen, uint key, int min_bodylen);

extern bcm_tlv_t *bcm_parse_ordered_tlvs(void *buf, int buflen, uint key);

extern bcm_tlv_t *bcm_find_vendor_ie(void *tlvs, int tlvs_len, const char *voui, uint8 *type,
	int type_len);

extern uint8 *bcm_write_tlv(int type, const void *data, int datalen, uint8 *dst);
extern uint8 *bcm_write_tlv_safe(int type, const void *data, int datalen, uint8 *dst,
	int dst_maxlen);

extern uint8 *bcm_copy_tlv(const void *src, uint8 *dst);
extern uint8 *bcm_copy_tlv_safe(const void *src, uint8 *dst, int dst_maxlen);


extern bcm_xtlv_t *bcm_next_xtlv(bcm_xtlv_t *elt, int *buflen);


extern const char *bcmerrorstr(int bcmerror);


typedef uint32 mbool;
#define mboolset(mb, bit)		((mb) |= (bit))		
#define mboolclr(mb, bit)		((mb) &= ~(bit))	
#define mboolisset(mb, bit)		(((mb) & (bit)) != 0)	
#define	mboolmaskset(mb, mask, val)	((mb) = (((mb) & ~(mask)) | (val)))


struct fielddesc {
	const char *nameandfmt;
	uint32 	offset;
	uint32 	len;
};

extern void bcm_binit(struct bcmstrbuf *b, char *buf, uint size);
extern void bcm_bprhex(struct bcmstrbuf *b, const char *msg, bool newline, uint8 *buf, int len);

extern void bcm_inc_bytes(uchar *num, int num_bytes, uint8 amount);
extern int bcm_cmp_bytes(const uchar *arg1, const uchar *arg2, uint8 nbytes);
extern void bcm_print_bytes(const char *name, const uchar *cdata, int len);

typedef  uint32 (*bcmutl_rdreg_rtn)(void *arg0, uint arg1, uint32 offset);
extern uint bcmdumpfields(bcmutl_rdreg_rtn func_ptr, void *arg0, uint arg1, struct fielddesc *str,
                          char *buf, uint32 bufsize);
extern uint bcm_bitcount(uint8 *bitmap, uint bytelength);

extern int bcm_bprintf(struct bcmstrbuf *b, const char *fmt, ...);


extern uint16 bcm_qdbm_to_mw(uint8 qdbm);
extern uint8 bcm_mw_to_qdbm(uint16 mw);
extern uint bcm_mkiovar(char *name, char *data, uint datalen, char *buf, uint len);

unsigned int process_nvram_vars(char *varbuf, unsigned int len);


extern void bcm_uint64_multiple_add(uint32* r_high, uint32* r_low, uint32 a, uint32 b, uint32 c);

extern void bcm_uint64_divide(uint32* r, uint32 a_high, uint32 a_low, uint32 b);





static const uint8 
_CSBTBL[256] =
{
#	define B2(n)    n,     n + 1,     n + 1,     n + 2
#	define B4(n) B2(n), B2(n + 1), B2(n + 1), B2(n + 2)
#	define B6(n) B4(n), B4(n + 1), B4(n + 1), B4(n + 2)
	B6(0), B6(0 + 1), B6(0 + 1), B6(0 + 2)
};

static INLINE uint32 
bcm_cntsetbits(const uint32 u32)
{
	
	const uint8 * p = (const uint8 *)&u32;
	return (_CSBTBL[p[0]] + _CSBTBL[p[1]] + _CSBTBL[p[2]] + _CSBTBL[p[3]]);
}


static INLINE int 
C_bcm_count_leading_zeros(uint32 u32)
{
	int shifts = 0;
	while (u32) {
		shifts++; u32 >>= 1;
	}
	return (32U - shifts);
}

#ifdef BCMDRIVER

#if defined(__mips__)
#define __USE_ASM_CLZ__
#endif 

#if defined(__arm__)

#if defined(__ARM_ARCH_7M__) 
#define __USE_ASM_CLZ__
#endif 

#if defined(__ARM_ARCH_7R__) 
#define __USE_ASM_CLZ__
#endif 

#endif 

static INLINE int
bcm_count_leading_zeros(uint32 u32)
{
#if defined(__USE_ASM_CLZ__)
	int zeros;
	__asm__ volatile("clz    %0, %1 \n" : "=r" (zeros) : "r"  (u32));
	return zeros;
#else	
	return C_bcm_count_leading_zeros(u32);
#endif  
}


struct bcm_mwbmap;	

#define BCM_MWBMAP_INVALID_HDL	((struct bcm_mwbmap *)NULL)
#define BCM_MWBMAP_INVALID_IDX	((uint32)(~0U))


extern struct bcm_mwbmap * bcm_mwbmap_init(osl_t * osh, uint32 items_max);


extern void bcm_mwbmap_fini(osl_t * osh, struct bcm_mwbmap * mwbmap_hdl);


extern uint32 bcm_mwbmap_alloc(struct bcm_mwbmap * mwbmap_hdl);


extern void bcm_mwbmap_force(struct bcm_mwbmap * mwbmap_hdl, uint32 bitix);


extern void bcm_mwbmap_free(struct bcm_mwbmap * mwbmap_hdl, uint32 bitix);


extern uint32 bcm_mwbmap_free_cnt(struct bcm_mwbmap * mwbmap_hdl);


extern bool bcm_mwbmap_isfree(struct bcm_mwbmap * mwbmap_hdl, uint32 bitix);


extern void bcm_mwbmap_show(struct bcm_mwbmap * mwbmap_hdl);

extern void bcm_mwbmap_audit(struct bcm_mwbmap * mwbmap_hdl);





#define ID16_INVALID                ((uint16)(~0))


extern void * id16_map_init(osl_t *osh, uint16 total_ids, uint16 start_val16);
extern void * id16_map_fini(osl_t *osh, void * id16_map_hndl);


extern uint16 id16_map_alloc(void * id16_map_hndl);


extern void id16_map_free(void * id16_map_hndl, uint16 val16);


extern uint32 id16_map_failures(void * id16_map_hndl);


extern bool id16_map_audit(void * id16_map_hndl);


#endif 

extern void bcm_uint64_right_shift(uint32* r, uint32 a_high, uint32 a_low, uint32 b);

void bcm_add_64(uint32* r_hi, uint32* r_lo, uint32 offset);
void bcm_sub_64(uint32* r_hi, uint32* r_lo, uint32 offset);


uint16 bcm_ip_cksum(uint8 *buf, uint32 len, uint32 sum);

#ifndef _dll_t_
#define _dll_t_

typedef struct dll {
	struct dll * next_p;
	struct dll * prev_p;
} dll_t;

static INLINE void
dll_init(dll_t *node_p)
{
	node_p->next_p = node_p;
	node_p->prev_p = node_p;
}


static INLINE dll_t *
dll_head_p(dll_t *list_p)
{
	return list_p->next_p;
}


static INLINE dll_t *
dll_tail_p(dll_t *list_p)
{
	return (list_p)->prev_p;
}


static INLINE dll_t *
dll_next_p(dll_t *node_p)
{
	return (node_p)->next_p;
}


static INLINE dll_t *
dll_prev_p(dll_t *node_p)
{
	return (node_p)->next_p;
}


static INLINE bool
dll_empty(dll_t *list_p)
{
	return ((list_p)->next_p == (list_p));
}


static INLINE bool
dll_end(dll_t *list_p, dll_t * node_p)
{
	return (list_p == node_p);
}



static INLINE void
dll_insert(dll_t *new_p, dll_t * at_p)
{
	new_p->next_p = at_p->next_p;
	new_p->prev_p = at_p;
	at_p->next_p = new_p;
	(new_p->next_p)->prev_p = new_p;
}

static INLINE void
dll_append(dll_t *list_p, dll_t *node_p)
{
	dll_insert(node_p, dll_tail_p(list_p));
}

static INLINE void
dll_prepend(dll_t *list_p, dll_t *node_p)
{
	dll_insert(node_p, list_p);
}



static INLINE void
dll_delete(dll_t *node_p)
{
	node_p->prev_p->next_p = node_p->next_p;
	node_p->next_p->prev_p = node_p->prev_p;
}
#endif  



typedef struct dll_pool {
	dll_t       free_list;
	uint16      free_count;
	uint16      elems_max;
	uint16      elem_size;
	dll_t       elements[1];
} dll_pool_t;

dll_pool_t * dll_pool_init(void * osh, uint16 elems_max, uint16 elem_size);
void * dll_pool_alloc(dll_pool_t * dll_pool_p);
void dll_pool_free(dll_pool_t * dll_pool_p, void * elem_p);
void dll_pool_free_tail(dll_pool_t * dll_pool_p, void * elem_p);
typedef void (* dll_elem_dump)(void * elem_p);
void dll_pool_detach(void * osh, dll_pool_t * pool, uint16 elems_max, uint16 elem_size);

#ifdef __cplusplus
	}
#endif


#ifdef DEBUG_COUNTER
#define CNTR_TBL_MAX 10
typedef struct _counter_tbl_t {
	char name[16];				
	uint32 prev_log_print;		
	uint log_print_interval;	
	uint needed_cnt;			
	uint32 cnt[CNTR_TBL_MAX];		
	bool enabled;				
} counter_tbl_t;


void counter_printlog(counter_tbl_t *ctr_tbl);
#endif 

#endif	
