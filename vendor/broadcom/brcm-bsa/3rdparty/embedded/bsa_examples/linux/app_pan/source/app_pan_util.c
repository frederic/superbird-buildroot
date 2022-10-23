/*****************************************************************************
 **
 **  Name:           app_pan_util.c
 **
 **  Description:    This file contains the PAN utility function
 **                  implementation.
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <string.h>

#include "gki.h"
#include "uipc.h"
#include "bsa_sec_api.h"
#include "bsa_dm_api.h"
#include "bsa_pan_api.h"

#include "app_pan_util.h"

#include "bsa_trace.h"

/* #define APP_PAN_UTIL_DEBUG 1 */

/******************************************************************************
 **
 ** Function         app_pan_util_dup
 **
 ** Description      This function duplicates GKI buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_dup(BT_HDR *p_buf, UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf2;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG2("app_pan_util_dup offset:%04x len:%04x",
            min_offset, max_len);
#endif

    if (max_len < p_buf->len)
    {
        max_len = p_buf->len;
    }

    p_buf2 = GKI_getbuf(sizeof(BT_HDR) + min_offset + max_len);
    if (p_buf2 == NULL)
    {
        APPL_TRACE_ERROR0("app_pan_util_dup: out of memory");

        return NULL;
    }

    memcpy(p_buf2, p_buf, sizeof(BT_HDR));

    p_buf2->offset = min_offset;

    memcpy((UINT8 *)(p_buf2 + 1) + p_buf2->offset,
            (UINT8 *)(p_buf + 1) + p_buf->offset,
            p_buf->len);

    return p_buf2;
}

/******************************************************************************
 **
 ** Function         app_pan_util_realloc
 **
 ** Description      This function reallocates GKI buffer.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_realloc(BT_HDR **pp_buf, UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf2;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG2("app_pan_util_realloc offset:%04x len:%04x",
            min_offset, max_len);
#endif

    if (max_len < (*pp_buf)->len)
    {
        max_len = (*pp_buf)->len;
    }

    if (GKI_get_buf_size(*pp_buf) < sizeof(BT_HDR) + min_offset + max_len)
    {
        /* buffer is too short. allocate new buffer */
        p_buf2 = GKI_getbuf(sizeof(BT_HDR) + min_offset + max_len);
        if (p_buf2 == NULL)
        {
            APPL_TRACE_ERROR0("app_pan_util_realloc: out of memory");

            return NULL;
        }

        memcpy(p_buf2, *pp_buf, sizeof(BT_HDR));

        p_buf2->offset = min_offset;

        memcpy((UINT8 *)(p_buf2 + 1) + p_buf2->offset,
                (UINT8 *)(*pp_buf + 1) + (*pp_buf)->offset,
                p_buf2->len);

        GKI_freebuf(*pp_buf);
        *pp_buf = p_buf2;
    }
    else
    {
        if (((*pp_buf)->offset < min_offset) ||
                (GKI_get_buf_size(*pp_buf) < (*pp_buf)->offset + max_len))
        {
            memmove((UINT8 *)(*pp_buf + 1) + min_offset,
                    (UINT8 *)(*pp_buf + 1) + (*pp_buf)->offset,
                    (*pp_buf)->len);

            (*pp_buf)->offset = min_offset;
        }
    }

    return *pp_buf;
}

/******************************************************************************
 **
 ** Function         app_pan_util_get_header
 **
 ** Description      This function gets header from buffer.
 **
 ** Returns          header.
 **
 ******************************************************************************/
void *app_pan_util_get_header(BT_HDR *p_buf, void *p_header, UINT16 len)
{
#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG1("app_pan_util_get_header len:%04x", len);
#endif

    if (p_buf->len <= len)
    {
        return NULL;
    }

    memcpy(p_header, (UINT8 *)(p_buf + 1) + p_buf->offset, len);

    p_buf->offset += len;
    p_buf->len -= len;

    return p_header;
}

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
        UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf2;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG1("app_pan_util_add_header len:%04x", len);
#endif

    p_buf2 = app_pan_util_realloc(pp_buf, min_offset + len, max_len);
    if (p_buf2 == NULL)
    {
        return NULL;
    }

    p_buf2->offset -= len;
    p_buf2->len += len;

    memcpy((UINT8 *)(p_buf2 + 1) + p_buf2->offset, p_header, len);

    return p_buf2;
}

/******************************************************************************
 **
 ** Function         app_pan_util_get_trailer
 **
 ** Description      This function gets trailer from buffer.
 **
 ** Returns          trailer.
 **
 ******************************************************************************/
void *app_pan_util_get_trailer(BT_HDR *p_buf, void *p_trailer, UINT16 len)
{
#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG1("app_pan_util_get_trailer len:%04x", len);
#endif

    if (p_buf->len <= len)
    {
        return NULL;
    }

    memcpy(p_trailer,
            (UINT8 *)(p_buf + 1) + p_buf->offset + p_buf->len - len,
            len);

    p_buf->len -= len;

    return p_trailer;
}

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
        UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf2;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG1("app_pan_util_add_trailer len:%04x", len);
#endif

    if (max_len < (*pp_buf)->len + len)
    {
        max_len = (*pp_buf)->len + len;
    }

    p_buf2 = app_pan_util_realloc(pp_buf, min_offset, max_len);
    if (p_buf2 == NULL)
    {
        return NULL;
    }

    memcpy((UINT8 *)(p_buf2 + 1) + p_buf2->offset + p_buf2->len,
            p_trailer, len);

    p_buf2->len += len;

    return p_buf2;
}

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
        UINT16 min_offset, UINT16 max_len)
{
#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG1("app_pan_util_move_data len:%04x", len);
#endif

    if (p_buf2->len < len)
    {
        return NULL;
    }

    if (max_len < (*pp_buf1)->len + len)
    {
        max_len = (*pp_buf1)->len + len;
    }

    if (app_pan_util_realloc(pp_buf1, min_offset, max_len) == NULL)
    {
        return NULL;
    }

    memcpy((UINT8 *)((*pp_buf1) + 1) + (*pp_buf1)->offset + (*pp_buf1)->len,
            (UINT8 *)(p_buf2 + 1) + p_buf2->offset, len);

    (*pp_buf1)->len += len;

    p_buf2->offset += len;
    p_buf2->len -= len;

    return *pp_buf1;
}

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
        UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf1;
    BT_HDR *p_buf2;
    UINT16 len2;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG2("app_pan_util_split_buffer len:%04x %04x",
            (*pp_buf)->len, len);
#endif

    len2 = (*pp_buf)->len - len;

    if (len < len2)
    {
        if (max_len < len)
        {
            max_len = len;
        }

        p_buf1 = GKI_getbuf(sizeof(BT_HDR) + min_offset + max_len);
        if (p_buf1 == NULL)
        {
            return NULL;
        }

        p_buf2 = *pp_buf;

        memcpy(p_buf1, p_buf2, sizeof(BT_HDR));

        p_buf1->offset = min_offset;
        p_buf1->len = len;

        memcpy((UINT8 *)(p_buf1 + 1) + p_buf1->offset,
                (UINT8 *)(p_buf2 + 1) + p_buf2->offset,
                len);

        p_buf2->offset += len;
        p_buf2->len -= len;
    }
    else
    {
        if (max_len < len2)
        {
            max_len = len2;
        }

        p_buf2 = GKI_getbuf(sizeof(BT_HDR) + min_offset + max_len);
        if (p_buf2 == NULL)
        {
            return NULL;
        }

        p_buf1 = *pp_buf;

        memcpy(p_buf2, p_buf1, sizeof(BT_HDR));

        p_buf2->offset = min_offset;
        p_buf2->len = len2;

        memcpy((UINT8 *)(p_buf2 + 1) + p_buf2->offset,
                (UINT8 *)(p_buf1 + 1) + p_buf1->offset + len,
                len2);

        p_buf1->len -= len2;
    }

    *pp_buf = p_buf1;

#if defined(APP_PAN_UTIL_DEBUG) && (APP_PAN_UTIL_DEBUG > 1)
    scru_dump_hex((UINT8 *)(p_buf1 + 1) + p_buf1->offset, "MSG1:", 16, 0, 0);
    scru_dump_hex((UINT8 *)(p_buf2 + 1) + p_buf2->offset, "MSG2:", 16, 0, 0);
#endif

    return p_buf2;
}

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
        UINT16 min_offset, UINT16 max_len)
{
    BT_HDR *p_buf;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG2("app_pan_util_marge_buffer len: %04x %04x",
            (*pp_buf)->len, (*pp_buf2)->len);
#endif

    if ((*pp_buf)->len < (*pp_buf2)->len)
    {
        p_buf = app_pan_util_add_header(pp_buf2,
                (UINT8 *)((*pp_buf) + 1) + (*pp_buf)->offset,
                (*pp_buf)->len, min_offset, max_len);
        if (p_buf == NULL)
        {
            return NULL;
        }

        GKI_freebuf(*pp_buf);
        *pp_buf = p_buf;
        *pp_buf2 = NULL;
    }
    else
    {
        p_buf = app_pan_util_add_trailer(pp_buf,
                (UINT8 *)((*pp_buf2) + 1) + (*pp_buf2)->offset,
                (*pp_buf2)->len, min_offset, max_len);
        if (p_buf == NULL)
        {
            return NULL;
        }

        GKI_freebuf(*pp_buf2);
        *pp_buf2 = NULL;
    }

    return p_buf;
}

/******************************************************************************
 **
 ** Function         app_pan_util_uipc_get_msg
 **
 ** Description      This function get one UIPC packet from queue.
 **
 ** Returns          new GKI buffer.
 **
 ******************************************************************************/
BT_HDR *app_pan_util_uipc_get_msg(BUFFER_Q *q)
{
    BT_HDR *p_buf1;
    BT_HDR *p_buf2;
    tBSA_PAN_UIPC_HDR hdr;

#ifdef APP_PAN_UTIL_DEBUG
    APPL_TRACE_DEBUG0("app_pan_util_uipc_get_msg");
#endif

    p_buf1 = GKI_dequeue(q);
    if (p_buf1 == NULL)
    {
        return NULL;
    }

    while (p_buf1->len < sizeof(hdr))
    {
        p_buf2 = GKI_dequeue(q);
        if (p_buf2 == NULL)
        {
            GKI_enqueue_head(q, p_buf1);

            return NULL;
        }

        if (app_pan_util_merge_buffer(&p_buf1, &p_buf2, 0, 0) == NULL)
        {
            GKI_enqueue_head(q, p_buf2);
            GKI_enqueue_head(q, p_buf1);

            return NULL;
        }
    }

    memcpy(&hdr, (UINT8 *)(p_buf1 + 1) + p_buf1->offset, sizeof(hdr));

    while (p_buf1->len < sizeof(hdr) + hdr.len)
    {
        p_buf2 = GKI_dequeue(q);
        if (p_buf2 == NULL)
        {
            GKI_enqueue_head(q, p_buf1);

            return NULL;
        }

        if (p_buf1->len + p_buf2->len > sizeof(hdr) + hdr.len)
        {
            if (app_pan_util_move_data(&p_buf1, p_buf2, sizeof(hdr) + hdr.len -
                    p_buf1->len, 0, 0) == NULL)
            {
                GKI_enqueue_head(q, p_buf2);
                GKI_enqueue_head(q, p_buf1);

                return NULL;
            }

            GKI_enqueue_head(q, p_buf2);

            break;
        }

        if (app_pan_util_merge_buffer(&p_buf1, &p_buf2, 0, 0) == NULL)
        {
            GKI_enqueue_head(q, p_buf2);
            GKI_enqueue_head(q, p_buf1);

            return NULL;
        }
    }

    if (p_buf1->len > sizeof(hdr) + hdr.len)
    {
        p_buf2 = app_pan_util_split_buffer(&p_buf1, sizeof(hdr) + hdr.len,
                0, 0);
        if (p_buf2 == NULL)
        {
            GKI_enqueue_head(q, p_buf1);

            return NULL;
        }

        GKI_enqueue_head(q, p_buf2);
    }

    p_buf1->event = hdr.evt;
    p_buf1->offset += sizeof(hdr);
    p_buf1->len = hdr.len;

#if defined(APP_PAN_UTIL_DEBUG) && (APP_PAN_UTIL_DEBUG > 1)
#define MIN(_x, _y) (((_x) < (_y)) ? (_x) : (_y))
    APPL_TRACE_DEBUG2("UIPC ch:-- evt:%04x len:%d", p_buf1->event,
            p_buf1->len);
    scru_dump_hex((UINT8 *)(p_buf1 + 1) + p_buf1->offset, "UIPC_RCV",
            MIN(64, p_buf1->len), 0, 0);
#endif

    return p_buf1;
}
