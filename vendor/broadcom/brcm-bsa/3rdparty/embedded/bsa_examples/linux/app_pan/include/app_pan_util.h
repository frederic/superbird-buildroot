/*****************************************************************************
 **
 **  Name:           app_pan_util.h
 **
 **  Description:    This file contains the PAN utility function
 **                  implementation.
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef __APP_PAN_UTIL_H__
#define __APP_PAN_UTIL_H__

#include "gki.h"
#include "uipc.h"

/*
 * Local Functions
 */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 **
 ** Function         app_pan_util_dup
 **
 ** Description      This function duplicates GKI buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_dup(BT_HDR *p_buf, UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_realloc
 **
 ** Description      This function reallocates GKI buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_realloc(BT_HDR **pp_buf, UINT16 min_offset,
        UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_get_header
 **
 ** Description      This function gets header from buffer.
 **
 ** Returns          header.
 **
 ******************************************************************************/
void *app_pan_util_get_header(BT_HDR *p_buf, void *p_header, UINT16 len);

/******************************************************************************
 **
 ** Function         app_pan_util_add_header
 **
 ** Description      This function add header to top of buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_add_header(BT_HDR **pp_buf, void *p_header, UINT16 len,
        UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_get_trailer
 **
 ** Description      This function gets trailer from buffer.
 **
 ** Returns          trailer.
 **
 ******************************************************************************/
void *app_pan_util_get_trailer(BT_HDR *p_buf, void *p_trailer, UINT16 len);

/******************************************************************************
 **
 ** Function         app_pan_util_add_trailer
 **
 ** Description      This function add trailer to end of buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_add_trailer(BT_HDR **pp_buf, void *p_trailer, UINT16 len,
        UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_move_data
 **
 ** Description      This function moves data from top of p_buf2 to end of
 **                  p_buf1.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_move_data(BT_HDR **pp_buf1, BT_HDR *p_buf2, UINT16 len,
        UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_split_buffer
 **
 ** Description      This function splits buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_split_buffer(BT_HDR **pp_buf, UINT16 len,
        UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_merge_buffer
 **
 ** Description      This function merges buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_merge_buffer(BT_HDR **pp_buf, BT_HDR **pp_buf2,
        UINT16 min_offset, UINT16 max_len);

/******************************************************************************
 **
 ** Function         app_pan_util_uipc_get_msg
 **
 ** Description      This function get one UIPC packet from queue.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_uipc_get_msg(BUFFER_Q *q);

#ifdef __cplusplus
}
#endif

#endif
