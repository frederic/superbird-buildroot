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

/*
  @file tee_ngwm_api.h.h

  @brief
  This file defines the NexGuard Watermark Hardware Abstraction
  Layer (HAL) Interface.

  @details
  It defines functions required to write NexGuard IP Core registers
*/

#ifndef _TEE_NGWM_API_H_
#define _TEE_NGWM_API_H_

#include <stdlib.h>
#include <ngwm_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  @brief
  Result returned by NexGuard watermark HAL interface functions.
*/
typedef enum {
	NGWM_HAL_SUCCESS,
	NGWM_HAL_ERROR,
	NGWM_HAL_LAST_STATUS //Not used
} TNgwmHalResult;

/*
  @ingroup g_wm_hal
  @brief
  This structure defines the NexGuard watermark HAL interface content.
  @details
  It is a collection of function pointers composing the interface.
*/
typedef struct {
	uint32_t version;
	/*
	   @brief
	   NexGuard watermark HAL interface version number.
	   @details
	   Assign it to the ::NGWMHALAPI_VERSION_INT result.
	*/

	TNgwmHalResult (*setSeed)(TNgwmEmbedder pxEmbedder, uint32_t xSeed);
	/*
	   @brief
	   Set the seed to be used for watermarking

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xSeed
	   Seed to be to configure in the hardware embedder

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*setOperatorId)(TNgwmEmbedder pxEmbedder,
			uint8_t xOperatorId);
	/*
	   @brief
	   Set the operator ID to be used for watermarking

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xOperatorId
	   Operator ID to configure in the hardware embedder

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*setSettings)(TNgwmEmbedder pxEmbedder,
			const uint8_t* pxSettings, uint32_t xSize);
	/*
	   @brief
	   Set the settings to be used for watermarking

	   Data in pxSettings parameter is encapsulated in a TLV descriptor
	   complying with the format below and is actually conveying an array
	   of settings for potentially several output types.

	   @code
	   tag                   8 uimsbf  h44
	   length                8 uimsbf
	   technology_type       8 uimsbf
	   technology_version   16 uimsbf
	   settings_nb           8 uimsbf
	   for(i=0; i<N; i++) {
		setting_type        8 uimsbf
		setting_data     21*8 uimsbf
	   }

	   @endcode
	   Settings conveyed in the array, and also the number of entries in
	   the array, are SoC specific and depend on the IP core integration
	   model.If the SoC is able to configure IP cores depending on the type
	   of output (e.g. SDR8, HDR10, etc), the settings array contains an
	   entry for each supported output type. Otherwise, the settings array
	   contains one single entry applicable to any output.

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] pxSettings
	   Buffer containing the settings to configure in the hardware embedder.

	   @param[in] xSize
	   Size in bytes of the pxSettings array

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*setDeviceId)(TNgwmEmbedder pxEmbedder,
			const uint8_t* pxDeviceId, uint8_t xSizeInBits);
	/*
	   @brief
	   Set the device ID to be used for watermarking

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] pxDeviceId
	   Device ID to configure in the hardware embedder

	   @param[in] xSizeInBits
	   Size in bits of the device ID (e.g. typically 32 bits)

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*setTimeCode)(TNgwmEmbedder pxEmbedder,
			uint16_t xTimeCode);
	/*
	   @brief
	   Set the timecode to be used for watermarking.

	   The value of the timecode provided to this function is already a
	   16-bit value using a 15-minute resolution over a year.

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xTimeCode
	   Timecode to configure in the hardware embedder

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*enableService)(TNgwmEmbedder pxEmbedder,
			bool xIsEnabled);
	/*
	   @brief
	   Activate/deactivate watermark embedding.

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xIsEnabled
	   When this boolean is set to TRUE, watermark embedding is activated.
	   Otherwise it is deactivated.

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*setStubEmbedding)(TNgwmEmbedder pxEmbedder,
			bool xIsEnabled);
	/*
	   @brief
	   Enables or disables the stub debug test pattern.

	   The stub pattern is a visible mark used for test purposes. It is
	   easier to check the integration when embedding a stub pattern than
	   using the real imperceptible watermark.

	   Note that the stub is only visible when the watermark embedding
	   is turned on.

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xIsEnabled
	   When this boolean is set to TRUE, the stub test pattern is activated.
	   Otherwise it is deactivated.

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	TNgwmHalResult (*set24bitMode)(TNgwmEmbedder pxEmbedder,
					bool xIsEnabled);
	/*
	   @brief
	   Enables or disables the 24-bit payload mode.

	   By default, the 24-bit payload mode is disabled and the IP core
	   works in 56-bit payload mode.

	   @param[in] pxEmbedder
	   Embedder object handle related to IP core to be configured

	   @param[in] xIsEnabled
	   When this boolean is set to TRUE, the 24-bit payload mode is enabled.
	   Otherwise it is disabled and the IP core works in 56-bit mode.

	   @retval ::NGWM_HAL_SUCCESS
	   The operation completed successfully

	   @retval ::NGWM_HAL_ERROR
	   The operation failed
	*/

	void* (*allocate)(size_t xSize);
	/**<
	  @brief
	  Allocates @c xSize bytes of memory.

	  @param[in]      xSize
	  Number of bytes to allocate

	  @return
	  Returns a pointer to the allocated buffer when the operation succeeds.
	  Returns @c NULL otherwise.
	 */

	void (*free)(void* pxMem);
	/**<
	  @brief
	  Frees the allocated memory referenced by @c pxMem.

	  @param[in]      pxMem
	  A pointer to the memory to be freed.
	 */

	void (*log)(const char* pxMessage);
	/**<
	  @brief
	  This function logs a NULL-terminated string message.

	  @param[in] pxMessage
	  Null-terminated message string to be logged.
	 */

} INgwmHal;

/*
  @ingroup g_wm_hal
  @brief
  Provide the watermark HAL interface structure.

  @pre
  Interface functions must have been defined.

  @post
  The memory allocated to the NexGuard watermark HAL interface structure must
  remain accessible as long as the NexGuard TA is running.

  @details
  This function is used by the NexGuard TA to retrieve the NexGuard watermark
  HAL interface structure.

  @return
  A constant pointer to the NexGuard watermark HAL interface structure.

  @see ::INgwmHal
*/
const INgwmHal* ngwmGetHalInterface(void);

#ifdef __cplusplus
}
#endif

#endif /*_TEE_NGWM_API_H_*/
