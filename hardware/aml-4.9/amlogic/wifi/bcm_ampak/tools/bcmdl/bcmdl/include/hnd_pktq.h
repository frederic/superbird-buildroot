/*
 * HND generic pktq operation primitives
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
 * $Id: $
 */

#ifndef _hnd_pktq_h_
#define _hnd_pktq_h_

#ifdef __cplusplus
extern "C" {
#endif


#define PKTQ_LEN_MAX            0xFFFF  
#ifndef PKTQ_LEN_DEFAULT
#define PKTQ_LEN_DEFAULT        128	
#endif
#ifndef PKTQ_MAX_PREC
#define PKTQ_MAX_PREC           16	
#endif

typedef struct pktq_prec {
	void *head;     
	void *tail;     
	uint16 len;     
	uint16 max;     
} pktq_prec_t;

#ifdef PKTQ_LOG
typedef struct {
	uint32 requested;    
	uint32 stored;	     
	uint32 saved;	     
	uint32 selfsaved;    
	uint32 full_dropped; 
	uint32 dropped;      
	uint32 sacrificed;   
	uint32 busy;         
	uint32 retry;        
	uint32 ps_retry;     
	uint32 suppress;     
	uint32 retry_drop;   
	uint32 max_avail;    
	uint32 max_used;     
	uint32 queue_capacity; 
	uint32 rtsfail;        
	uint32 acked;          
	uint32 txrate_succ;    
	uint32 txrate_main;    
	uint32 throughput;     
	uint32 airtime;        
	uint32  _logtime;      
} pktq_counters_t;

typedef struct {
	uint32                  _prec_log;
	pktq_counters_t*        _prec_cnt[PKTQ_MAX_PREC];     
} pktq_log_t;
#endif 


#define PKTQ_COMMON	\
	uint16 num_prec;        			\
	uint16 hi_prec;         	\
	uint16 max;             					\
	uint16 len;             


struct pktq {
	PKTQ_COMMON
	
	struct pktq_prec q[PKTQ_MAX_PREC];
#ifdef PKTQ_LOG
	pktq_log_t*      pktqlog;
#endif
};


struct spktq {
	PKTQ_COMMON
	
	struct pktq_prec q[1];
};

#define PKTQ_PREC_ITER(pq, prec)        for (prec = (pq)->num_prec - 1; prec >= 0; prec--)


typedef bool (*ifpkt_cb_t)(void*, int);



#define pktq_psetmax(pq, prec, _max)	((pq)->q[prec].max = (_max))
#define pktq_pmax(pq, prec)		((pq)->q[prec].max)
#define pktq_plen(pq, prec)		((pq)->q[prec].len)
#define pktq_pavail(pq, prec)		((pq)->q[prec].max - (pq)->q[prec].len)
#define pktq_pfull(pq, prec)		((pq)->q[prec].len >= (pq)->q[prec].max)
#define pktq_pempty(pq, prec)		((pq)->q[prec].len == 0)

#define pktq_ppeek(pq, prec)		((pq)->q[prec].head)
#define pktq_ppeek_tail(pq, prec)	((pq)->q[prec].tail)

extern void  pktq_append(struct pktq *pq, int prec, struct spktq *list);
extern void  pktq_prepend(struct pktq *pq, int prec, struct spktq *list);

extern void *pktq_penq(struct pktq *pq, int prec, void *p);
extern void *pktq_penq_head(struct pktq *pq, int prec, void *p);
extern void *pktq_pdeq(struct pktq *pq, int prec);
extern void *pktq_pdeq_prev(struct pktq *pq, int prec, void *prev_p);
extern void *pktq_pdeq_with_fn(struct pktq *pq, int prec, ifpkt_cb_t fn, int arg);
extern void *pktq_pdeq_tail(struct pktq *pq, int prec);

extern void pktq_pflush(osl_t *osh, struct pktq *pq, int prec, bool dir,
	ifpkt_cb_t fn, int arg);

extern bool pktq_pdel(struct pktq *pq, void *p, int prec);



extern int pktq_mlen(struct pktq *pq, uint prec_bmp);
extern void *pktq_mdeq(struct pktq *pq, uint prec_bmp, int *prec_out);
extern void *pktq_mpeek(struct pktq *pq, uint prec_bmp, int *prec_out);



#define pktq_len(pq)		((int)(pq)->len)
#define pktq_max(pq)		((int)(pq)->max)
#define pktq_avail(pq)		((int)((pq)->max - (pq)->len))
#define pktq_full(pq)		((pq)->len >= (pq)->max)
#define pktq_empty(pq)		((pq)->len == 0)


#define pktenq(pq, p)		pktq_penq(((struct pktq *)(void *)pq), 0, (p))
#define pktenq_head(pq, p)	pktq_penq_head(((struct pktq *)(void *)pq), 0, (p))
#define pktdeq(pq)		pktq_pdeq(((struct pktq *)(void *)pq), 0)
#define pktdeq_tail(pq)		pktq_pdeq_tail(((struct pktq *)(void *)pq), 0)
#define pktqflush(osh, pq)	pktq_flush(osh, ((struct pktq *)(void *)pq), TRUE, NULL, 0)
#define pktqinit(pq, len)	pktq_init(((struct pktq *)(void *)pq), 1, len)

extern void pktq_init(struct pktq *pq, int num_prec, int max_len);
extern void pktq_set_max_plen(struct pktq *pq, int prec, int max_len);


extern void *pktq_deq(struct pktq *pq, int *prec_out);
extern void *pktq_deq_tail(struct pktq *pq, int *prec_out);
extern void *pktq_peek(struct pktq *pq, int *prec_out);
extern void *pktq_peek_tail(struct pktq *pq, int *prec_out);
extern void pktq_flush(osl_t *osh, struct pktq *pq, bool dir, ifpkt_cb_t fn, int arg);

#ifdef __cplusplus
	}
#endif

#endif 
