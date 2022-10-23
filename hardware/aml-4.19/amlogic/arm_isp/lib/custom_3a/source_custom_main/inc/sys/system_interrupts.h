//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#ifndef __SYSTEM_INTERRUPTS_H__
#define __SYSTEM_INTERRUPTS_H__

#include "acamera_types.h"

#define ACAMERA_IRQ_COUNT 32

#define ACAMERA_IRQ_MASK( num ) ( 1 << num )

typedef uint32_t system_fw_interrupt_mask_t;

typedef void ( *system_interrupt_handler_t )( void *ptr, uint32_t mask );


/**
 *   Initialize system interrupts
 *
 *   This function initializes system dependent interrupt functionality
 *
 *   @return none
 */
void system_interrupts_init( void );


/**
 *   Set an interrupt handler
 *
 *   This function is used by application to set an interrupt handler for all ISP related interrupt events.
 *
 *   @param
 *          handler - a callback to handle interrupts
 *          param - pointer to a context which must be send to interrupt handler
 *
 *   @return none
 */
void system_interrupt_set_handler( system_interrupt_handler_t handler, void *param );


/**
 *   Enable system interrupts
 *
 *   This function is used by firmware to enable system interrupts in a case if they were disabled before
 *
 *   @return none
 */
void system_interrupts_enable( void );


/**
 *   Disable system interrupts
 *
 *   This function is used by firmware to disable system interrupts for a short period of time.
 *   Usually IRQ register is updated by new interrupts but main interrupt handler is not called by a system.
 *
 *   @return none
 */
void system_interrupts_disable( void );

#endif /* __SYSTEM_INTERRUPTS_H__ */
