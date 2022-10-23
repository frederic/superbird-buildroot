
/*****************************************************************************
**
**  Name:           bta_gp_api.h
**
**  Description:    This file contains the common API functions used
**                  for the Generic PIM Access Profiles (GP).
**
**  Copyright (c) 20114, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_GP_API_H
#define BTA_GP_API_H

#include "bta_gse_api.h"        /* For tBTA_GP_MSG_TYPE */
#include <stdio.h>

typedef struct
{
    UINT16              size;       /* Size of the buffer */
    UINT8 *             p_buffer;   /* Pointer to buffer */
    UINT8 *             p_next;     /* Pointer to next byte to use in buffer */

}tBTA_GP_MEM_STREAM;

typedef struct
{
    int   fd;

}tBTA_GP_FILE_STREAM;

/* Structure used for streaming data */
typedef struct
{
#define STRM_TYPE_MEMORY        0
#define STRM_TYPE_FILE          1

    UINT8               type;

#define STRM_SUCCESS            0
#define STRM_ERROR_OVERFLOW     1
#define STRM_ERROR_FILE         2

    UINT8               status;

    union
    {
        tBTA_GP_MEM_STREAM  mem;
        tBTA_GP_FILE_STREAM file;
    } u;

} tBTA_GP_STREAM;

typedef struct
{
    UINT8       *p_data;            /* Pointer to property data. */
    UINT32      parameters;         /* Property parameters. */
    UINT16      name;               /* Property name. */
    UINT16      len;                /* Length of data. */
    UINT8       *p_param;           /* Pointer to the Parameters */
    UINT16      param_len;          /* Param Len */
} tBTA_GP_PROP;

/* Status */
#define BTA_GP_CO_OK                   0       /* Operation successful. */
#define BTA_GP_CO_FAIL                 1       /* Operation failed. */
#define BTA_GP_CO_MEM                  2       /* Not enough memory to complete operation. */

typedef UINT8 tBTA_GP_CO_STATUS;

/*******************************************************************************
**
** bMessage functions
**
** Description      The following API functions are generic in a sense that
**                  they do not imply how the data is stored (to memory or
**                  to file, etc.).
**
**                  They operate on a generic set of structure types.  Though
**                  the internal structure of those types are implementation
**                  specific.
**
*******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
**
** Function         BTA_GpInitMemStream
**
** Description      Initializes a memory based stream
**
** Parameters       p_stream - pointer to stream information.
**                  p_buffer - pointer to buffer to be manipulated.
**                  size - size of buffer pointed to by 'p_buffer'.
**
** Returns          TRUE if stream is successfully initialized
**
*******************************************************************************/
BTA_API extern BOOLEAN BTA_GpInitMemStream(tBTA_GP_STREAM *p_stream, UINT8 *p_buffer, UINT16 size);

#ifdef __cplusplus
}
#endif

#endif /* BTA_GP_API_H */
