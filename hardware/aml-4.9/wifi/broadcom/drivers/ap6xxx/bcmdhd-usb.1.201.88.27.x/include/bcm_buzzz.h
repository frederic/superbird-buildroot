#ifndef __bcm_buzzz_h_included__
#define __bcm_buzzz_h_included__

/*
 * +----------------------------------------------------------------------------
 *
 * BCM BUZZZ Performance tracing tool for ARM Cortex-R4, Cortex-M3
 *
 * BUZZZ_CYCLES_PER_USEC : Specify CR4 speed
 * BUZZZ_LOG_BUFSIZE     : Specify log buffer size
 * BUZZZ_TRACING_LEVEL   : Specify tracing level
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
 * $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * +----------------------------------------------------------------------------
 */

#define BUZZZ_NULL_STMT         do { /* Nothing */ } while(0)

#if defined(BCM_BUZZZ)

#define BUZZZ_COUNTERS_MAX      (8)

/* Overhead was computed by making back to back buzzz_log() calls */
#define BUZZZ_CR4_CYCLECNT_OVHD (80)
#define BUZZZ_CR4_INSTRCNT_OVHD (34)
#define BUZZZ_CR4_BRMISPRD_OVHD (1)

#define BUZZZ_CM3_CYCCNT_OVHD   (100) /* 114 - 138 */
#define BUZZZ_CM3_CPICNT_OVHD   (4)
#define BUZZZ_CM3_EXCCNT_OVHD   (0)
#define BUZZZ_CM3_SLEEPCNT_OVHD (0)
#define BUZZZ_CM3_LSUCNT_OVHD   (90)
#define BUZZZ_CM3_FOLDCNT_OVHD  (0)


#if defined(__ARM_ARCH_7R__)
#define BUZZZ_HNDRTE
#define BUZZZ_CONFIG_CPU_ARM_CR4
#define BUZZZ_CYCLES_PER_USEC   (320)
#define BUZZZ_LOG_BUFSIZE       (4 * 4 * 1024) /* min 4K, multiple of 16 */
#define BUZZZ_TRACING_LEVEL     (5)            /* Buzzz tracing level */
#define BUZZZ_COUNTERS          (3)            /* 3 performance counters */

/*
 * Event ref. value definitions from the CR4 TRM.
 * Please check the TRM for more event definitions.
 */
#define BUZZZ_ARMCR4_SWINC_EVT		(0x00)	/* Software increment */
#define BUZZZ_ARMCR4_ICACHEMISS_EVT	(0x01)	/* Intruction cache miss */
#define BUZZZ_ARMCR4_DCACHEMISS_EVT	(0x03)	/* Data cache miss */
#define BUZZZ_ARMCR4_DATAREAD_EVT	(0x06)	/* Data read executed */
#define BUZZZ_ARMCR4_DATAWRITE_EVT	(0x07)	/* Data write executed */
#define BUZZZ_ARMCR4_INSTRCNT_EVT	(0x08)	/* Instruction executed */
#define BUZZZ_ARMCR4_EXPCNT_EVT		(0x09)	/* Exception taken */
#define BUZZZ_ARMCR4_EXPRTN_EVT		(0x0a)	/* Exception return executed */
#define BUZZZ_ARMCR4_CTXIDCHG_EVT	(0x0b)	/* Change to Context ID executed */
#define BUZZZ_ARMCR4_SWCHGPC_EVT	(0x0c)	/* Software change of PC executed */
#define BUZZZ_ARMCR4_BICNT_EVT		(0x0d)	/* B/BL/BLX immediate executed */
#define BUZZZ_ARMCR4_PROCRTN_EVT	(0x0e)	/* Procedure return executed */
#define BUZZZ_ARMCR4_UNALIGNED_EVT	(0x0f)	/* Unaligned access executed */
#define BUZZZ_ARMCR4_BRMISS_EVT		(0x10)	/* Branch mispredicted or not predicted */
#define BUZZZ_ARMCR4_CYCLECNT_EVT	(0x11)	/* Cycle count */
#define BUZZZ_ARMCR4_BRHIT_EVT		(0x12)	/* Branches predicted */

#endif /* __ARM_ARCH_7R__ */

#if defined(__ARM_ARCH_7M__)
#define BUZZZ_BMOS
#define BUZZZ_CONFIG_CPU_ARM_CM3
#define BUZZZ_CYCLES_PER_USEC   (160)
#define BUZZZ_LOG_BUFSIZE       (4 * 4 * 1024) /* min 4K, multiple of 16 */
#define BUZZZ_TRACING_LEVEL     (5)            /* Buzzz tracing level */
#define BUZZZ_COUNTERS          (6)            /* 6 performance counters */
#endif /* __ARM_ARCH_7M__ */


#define BUZZZ_LOGENTRY_MAXSZ    (64)

#define BUZZZ_ERROR             (-1)
#define BUZZZ_SUCCESS           (0)
#define BUZZZ_FAILURE           BUZZZ_ERROR
#define BUZZZ_DISABLED          (0)
#define BUZZZ_ENABLED           (1)
#define BUZZZ_FALSE             (0)
#define BUZZZ_TRUE              (1)
#define BUZZZ_INVALID           (~0U)

#define BUZZZ_INLINE            inline  __attribute__ ((always_inline))
#define BUZZZ_NOINSTR_FUNC      __attribute__ ((no_instrument_function))

extern void buzzz_log0(uint32 evt_id);
extern void buzzz_log1(uint32 evt_id, uint32 arg1);

#if defined(BUZZZ_4ARGS) /* Not supported */
extern void buzzz_log2(uint32 evt_id, uint32 arg1, uint32 arg2);
extern void buzzz_log3(uint32 evt_id, uint32 arg1, uint32 arg2, uint32 arg3);
extern void buzzz_log4(uint32 evt_id, uint32 arg1, uint32 arg2,
                                      uint32 arg3, uint32 arg4);
#else  /* ! BUZZZ_4ARGS */
#define buzzz_log2(evt, arg1, arg2)             BUZZZ_NULL_STMT
#define buzzz_log3(evt, arg1, arg2, arg3)       BUZZZ_NULL_STMT
#define buzzz_log4(evt, arg1, arg2, arg3, arg4) BUZZZ_NULL_STMT
#endif /* ! BUZZZ_4ARGS */

extern void buzzz_start(void);
extern void buzzz_stop(void);
extern void buzzz_config_ctr(uint32 ctr_sel);
extern void buzzz_dump(void);
extern void buzzz_clear(void);
extern int buzzz_register(void * shared);

typedef struct buzzz
{
	uint32          log;
	uint32          cur;        /* pointer to next log entry */
	uint32          end;        /* pointer to end of log entry */

	uint16          count;      /* count of logs, wraps on 64K */
	uint8           status;     /* runtime status */
	uint8           wrap;       /* log buffer wrapped */
	uint32          buffer_sz;
	uint32          log_sz;
	uint32          counters;
	uint32          ovhd[BUZZZ_COUNTERS_MAX];
} buzzz_t;

typedef struct buzzz_klog
{
	uint8  cnt;
	uint8  args;          /* number of arguments logged */
	uint16 id;            /* index into registerd format strings */
} buzzz_klog_t;

typedef union buzzz_arg0
{
	uint32 u32;
	buzzz_klog_t klog;
} buzzz_arg0_t;


typedef enum buzzz_ctrl {
	BUZZZ_START_COMMAND = 1,
	BUZZZ_STOP_COMMAND  = 2
} buzzz_ctrl_t;

/*
 * +----------------------------------------------------------------------------
 *
 * CAUTION: impact on ROM invalidations.
 *
 * Three steps to insert an instrumentation point.
 *
 * Step #1. List event in enum buzzz_KLOG_dpid
 *          E.g. BUZZZ_KLOG(SAMPLE_EVENT_NAME)
 *
 * Step #2. Register the event string to be used in buzzz_dump()
 *          Add an entry in bcm_buzzz.h: BUZZZ_FMT_STRINGS
 *
 * Step #3. Insert instrumentationi, at a desired compile time tracing level
 *          E.g. BUZZZ_LVL#(SAMPLE_EVENT_NAME, 1, (uint32)pkt);
 *          See note below on BUZZZ_LVL#()
 *
 * +----------------------------------------------------------------------------
 */

#undef BUZZZ_KLOG
#define BUZZZ_KLOG(event)       BUZZZ_KLOG__ ## event,
typedef enum buzzz_KLOG_dpid    /* List of datapath event point ids */
{
	BUZZZ_KLOG__START_EVT = 0,

	BUZZZ_KLOG(BUZZZ_0)
	BUZZZ_KLOG(BUZZZ_1)
	BUZZZ_KLOG(BUZZZ_2)
	BUZZZ_KLOG(BUZZZ_3)
	BUZZZ_KLOG(BUZZZ_4)

	/* HNDRTE subsystem events */
	BUZZZ_KLOG(HNDRTE_TRAP_HANDLER)
	BUZZZ_KLOG(HNDRTE_TRAP_HANDLER_RTN)
	BUZZZ_KLOG(HNDRTE_ISR)
	BUZZZ_KLOG(HNDRTE_ISR_RTN)
	BUZZZ_KLOG(HNDRTE_ISR_ACTION)
	BUZZZ_KLOG(HNDRTE_ISR_ACTION_RTN)
	BUZZZ_KLOG(HNDRTE_TIMER_FN)
	BUZZZ_KLOG(HNDRTE_TIMER_FN_RTN)

	BUZZZ_KLOG__LAST_EVT

} buzzz_KLOG_dpid_t;


#define BUZZZ_START()               buzzz_start()
#define BUZZZ_STOP()                buzzz_stop()
#define BUZZZ_CCTR(ctr_sel)         buzzz_config_ctr(ctr_sel)
#define BUZZZ_DUMP()                buzzz_dump()
#define BUZZZ_REGISTER(shared)      buzzz_register(shared)
#define BUZZZ_CLEAR()               buzzz_clear()

#else	/* ! BCM_BUZZZ */

#define BUZZZ_START()               BUZZZ_NULL_STMT
#define BUZZZ_STOP()                BUZZZ_NULL_STMT
#define BUZZZ_CCTR(ctr_sel)         BUZZZ_NULL_STMT
#define BUZZZ_DUMP()                BUZZZ_NULL_STMT
#define BUZZZ_REGISTER(shared)      BUZZZ_NULL_STMT
#define BUZZZ_CLEAR()               BUZZZ_NULL_STMT


#endif	/* ! BCM_BUZZZ */


/*
 * +----------------------------------------------------------------------------
 *  Insert instrumentation in code at various tracing levels using
 *
 *   BUZZZ_LVL#(EVENT_ENUM, NUM_ARGS, ARGS_LIST)
 *
 *     #         : Compile time tracing level, BUZZZ_TRACING_LEVEL >= #
 *     EVENT_ENUM: Enum added to buzzz_KLOG_dpid_t using BUZZZ_KLOG()
 *     NUM_ARGS  : Number of arguments to log, max 4 arguments
 *     ARGS_LIST : List of arguments, comma seperated
 * +----------------------------------------------------------------------------
 */

#if defined(BUZZZ_TRACING_LEVEL) && (BUZZZ_TRACING_LEVEL >= 1)
#define BUZZZ_LVL1(ID, N, ARG...)   buzzz_log ##N(BUZZZ_KLOG__ ##ID, ##ARG)
#else   /* ! BUZZZ_TRACING_LEVEL >= 1 */
#define BUZZZ_LVL1(ID, N, ARG...)   BUZZZ_NULL_STMT
#endif  /* ! BUZZZ_TRACING_LEVEL >= 1 */


#if defined(BUZZZ_TRACING_LEVEL) && (BUZZZ_TRACING_LEVEL >= 2)
#define BUZZZ_LVL2(ID, N, ARG...)   buzzz_log ##N(BUZZZ_KLOG__ ##ID, ##ARG)
#else   /* ! BUZZZ_TRACING_LEVEL >= 2 */
#define BUZZZ_LVL2(ID, N, ARG...)   BUZZZ_NULL_STMT
#endif  /* ! BUZZZ_TRACING_LEVEL >= 2 */


#if defined(BUZZZ_TRACING_LEVEL) && (BUZZZ_TRACING_LEVEL >= 3)
#define BUZZZ_LVL3(ID, N, ARG...)   buzzz_log ##N(BUZZZ_KLOG__ ##ID, ##ARG)
#else   /* ! BUZZZ_TRACING_LEVEL >= 3 */
#define BUZZZ_LVL3(ID, N, ARG...)   BUZZZ_NULL_STMT
#endif  /* ! BUZZZ_TRACING_LEVEL >= 3 */


#if defined(BUZZZ_TRACING_LEVEL) && (BUZZZ_TRACING_LEVEL >= 4)
#define BUZZZ_LVL4(ID, N, ARG...)   buzzz_log ##N(BUZZZ_KLOG__ ##ID, ##ARG)
#else   /* ! BUZZZ_TRACING_LEVEL >= 4 */
#define BUZZZ_LVL4(ID, N, ARG...)   BUZZZ_NULL_STMT
#endif  /* ! BUZZZ_TRACING_LEVEL >= 4 */


#if defined(BUZZZ_TRACING_LEVEL) && (BUZZZ_TRACING_LEVEL >= 5)
#define BUZZZ_LVL5(ID, N, ARG...)   buzzz_log ##N(BUZZZ_KLOG__ ##ID, ##ARG)
#else   /* ! BUZZZ_TRACING_LEVEL >= 5 */
#define BUZZZ_LVL5(ID, N, ARG...)   BUZZZ_NULL_STMT
#endif  /* ! BUZZZ_TRACING_LEVEL >= 5 */


#undef  _B_
#undef  _H_
#undef  _N_
#undef  _FAIL_
#define _B_                     "\e[0;34m"
#define _H_                     "\e[0;31m;40m"
#define _N_                     "\e[0m"
#define _FAIL_                  _H_ " === FAILURE ===" _N_

#define BUZZZ_FMT_STRINGS \
{                                                                              \
	"START_EVT",                          /* START_EVT */                      \
\
	"buzzz_log0",                         /* BUZZZ_0 */                        \
	"buzzz_log1 arg<%u>",                 /* BUZZZ_1 */                        \
	"buzzz_log2 arg<%u:%u>",              /* BUZZZ_2 */                        \
	"buzzz_log2 arg<%u:%u:%u>",           /* BUZZZ_3 */                        \
	"buzzz_log2 arg<%u:%u:%u:%u>",        /* BUZZZ_4 */                        \
										\
	/* HNDRTE */                                                               \
	_B_ "hndrte_trap_handler TRAP<%08x>" _N_, /* HNDRTE_TRAP_HANDLER */        \
	_B_ "hndrte_trap_handler RTN" _N_,    /* HNDRTE_TRAP_HANDLER_RTN */        \
	_B_ "hndrte_isr" _N_,                 /* HNDRTE_ISR */                     \
	_B_ "hndrte_isr RTN" _N_,             /* HNDRTE_ISR_RTN */                 \
	_B_ "hndrte_isr ACTION<%p>" _N_,      /* HNDRTE_ISR_ACTION */              \
	_B_ "hndrte_isr action RTN" _N_,      /* HNDRTE_ISR_ACTION_RTN */          \
	_B_ "hndrte::run_timeouts FN<%p>" _N_, /* HNDRTE_TIMER_FN */               \
	_B_ "hndrte::run_timeouts RTN" _N_,   /* HNDRTE_TIMER_FN_RTN */            \
										\
	"LAST_EVENT"                                                               \
}

#endif  /* __bcm_buzzz_h_included__ */
