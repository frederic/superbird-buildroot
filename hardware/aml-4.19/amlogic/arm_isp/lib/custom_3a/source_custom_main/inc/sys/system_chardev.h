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

#ifndef __SYSTEM_CHARDEV_H__
#define __SYSTEM_CHARDEV_H__

/**
 *   Spawn (misc) character device as the access channel for the Control Software.
 *   Make sense only for the Firmware running in Linux kernel space.
 *   One client at a time supported.
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int system_chardev_init( void );

/**
 *   Nonblocking read from the character device.
 *
 *   @return  number of bytes read on success
 *           -1 is returned after userspace application connected or disconnected
 */
int system_chardev_read( char *data, int size );

/**
 *   Nonblocking write to the character device.
 *
 *   @return  number of bytes written on success
 *           -1 is returned after userspace application connected or disconnected
 */
int system_chardev_write( const char *data, int size );

/**
 *   Deregister character device.
 *
 *   @return  0 - on success
 *           -1 - on error
 */
int system_chardev_destroy( void );

#endif /* __SYSTEM_CHARDEV_H__ */
