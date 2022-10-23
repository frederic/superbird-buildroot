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

#if !defined( __ACAMERA_CONTROL_CONFIG_H__ )
#define __ACAMERA_CONTROL_CONFIG_H__

#include "acamera_firmware_config.h"

//#if ISP_HAS_CONNECTION_SOCKET
/* please call these routines from the platform specific code to setup socket routines */
//struct acamera_socket_f *acamera_socket_f_impl(void);
//void acamera_socket_set_f_impl(struct acamera_socket_f *f);
//#endif // ISP_HAS_CONNECTION_SOCKET
#define ISP_HAS_STREAM_CONNECTION ( ISP_HAS_CONNECTION_SOCKET || ISP_HAS_CONNECTION_BUFFER || ISP_HAS_CONNECTION_UART || ISP_HAS_CONNECTION_CHARDEV )

#endif
