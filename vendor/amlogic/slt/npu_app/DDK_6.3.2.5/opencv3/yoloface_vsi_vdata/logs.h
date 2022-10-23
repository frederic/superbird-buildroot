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


#ifndef __LOGS_H__
#define __LOGS_H__
/*
 * Apical(ARM) V4L2 test application 2016
 *
 * This is ARM internal development purpose SW tool running on JUNO.
 */

#if 1
#define ERR printf
#else
#define ERR //
#endif

#if 1
#define MSG printf
#else
#define MSG //
#endif

#if 1
#define INFO printf
#else
#define INFO //
#endif

#if 1
#define DBG printf
#else
#define DBG //
#endif

#endif // __METADATA_API_H__
