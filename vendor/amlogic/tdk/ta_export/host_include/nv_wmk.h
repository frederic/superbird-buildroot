/*
 * Copyright (C) 2014-2017 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NV_WMK_H_
#define _NV_WMK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  @brief
    Defines an invalid transport session identifier.
*/
#define NV_WMK_TRANSPORT_SESSION_INVALID      ((uint32_t)(-1))

/**
  @ingroup g_watermark
  @brief
  Result returned by Nagra watermark interface functions.
*/
typedef enum
{
	NV_WMK_SUCCESS, /**< The operation has completed successfully. */
	NV_WMK_ERROR, /**< An unexpected error has occurred
			while processing the operation. */
	NV_WMK_NOT_APPLICABLE, /**< The operation is not applicable */
	NV_WMK_DENIED, /**< The operation is denied because of privilege
			  violation */
	NV_WMK_LAST_STATUS /**< Not used. */
} TNvWmkResult;

/**
  @ingroup g_watermark
  @brief
  Watermark embedder state
*/
typedef enum
{
	NV_WMK_STATE_UNKNOWN, /**< The state is unknown */
	NV_WMK_STATE_STOPPED, /**< The watermark embedder is stopped
				(i.e. content is not watermarked) */
	NV_WMK_STATE_RUNNING, /**< The watermark embedder is running
				(i.e. content is watermarked) */
	NV_WMK_LAST_STATE /**< Not used. */
} TNvWmkState;

typedef struct {
	uint32_t version;
	/**<
	  @brief
	  Nagra watermark interface version number.
	  @details
	  Assign it to the ::WMKAPI_VERSION_INT result.
	*/

	TNvWmkResult (*configure)(const uint8_t* pxConfig, uint32_t xConfigSize);
	/**<
	  @brief
	  Configures watermark IP cores on all media pipelines.
	  @details
	  This function configures all watermark IP core instances with the given
	  configuration.
	  @param[in] pxConfig
	  Buffer containing the watermark configuration.
	  @param[in] xConfigSize
	  Size in bytes of the configuration.
	  @retval ::NV_WMK_SUCCESS
	  The operation completed successfully
	  @retval ::NV_WMK_ERROR
	  The operation failed
	*/

	TNvWmkResult (*configureByPipe)(uint32_t xTransportSessionId,
					uint16_t xKeySlotId,
					const uint8_t* pxConfig,
					uint32_t xConfigSize);
	/**<
	  @brief
	  Configures watermark IP cores related to a given pipeline.
	  @details
	  This function configures watermark IP core instances associated to a
	  given pipeline. The pipeline is identified by the transport session
	  ID and key slot ID.
	  @param[in] xTransportSessionId
	  Identifier of the media pipeline associated to the decrypted stream
	  to be watermarked.
	  @param[in] xKeySlotId
	  Identifier of the key table slot associated to the decrypted stream
	  to be watermarked.
	  @param[in] pxConfig
	  Buffer containing the watermark configuration.
	  @param[in] xConfigSize
	  Size in bytes of the configuration.
	  @retval ::NV_WMK_SUCCESS
	  The operation completed successfully
	  @retval ::NV_WMK_ERROR
	  The operation failed
	  @retval ::NV_WMK_NOT_APPLICABLE
	  The operation is not applicable. For instance, the operation is
	  called for a pipe without any IP core embedder.
	*/

	TNvWmkResult (*getState)(uint32_t xTransportSessionId,
					uint16_t xKeySlotId, uint32_t* pxState);
	/**<
	  @brief
	  Returns the state of the watermark embedder related to a
	  given pipeline.
	  @details
	  This function returns the state of the watermark IP core embedder
	  related to a given pipeline to inform whether it is running or stopped.
	  @param[in] xTransportSessionId
	  Identifier of the media pipeline related to the IP core embedder for
	  which the watermark state is requested
	  @param[in] xKeySlotId
	  Identifier of the key table slot related to the IP core embedder for
	  which the watermark state is requested
	  @param[out] pxState
	  State indicating whether an IP core is running (::NV_WMK_STATE_RUNNING)
	  or stopped (::NV_WMK_STATE_STOPPED). If the state is unknown, it is
	  set to ::NV_WMK_STATE_UNKNOWN. See also ::TNvWmkState.
	  @retval ::NV_WMK_SUCCESS
	  The operation completed successfully
	  @retval ::NV_WMK_ERROR
	  The operation failed
	  @retval ::NV_WMK_NOT_APPLICABLE
	  The operation is not applicable. For instance, the operation is
	  called for a pipe without any IP core embedder.
	*/
} INvWatermark;

/**
  @pre
  Interface functions must have been defined.

  @post
  The memory allocated to the Nagra watermark interface structure must remain
  accessible as long as the Nagra client is running.

  @details
  This function is used by the Nagra client to retrieve the Nagra watermark
  interface structure.

  This function should be called once during Nagra client initialization but
  it cannot be definitively assumed. Therefore the address of the structure
  and the memory allocated to it must be valid as long as the Nagra client
  is loaded and running.

  @return
  A constant pointer to the Nagra watermark interface structure.

  @see ::INvWatermark.
*/
const INvWatermark* nvGetWatermarkInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* _NV_WMK_H_ */
