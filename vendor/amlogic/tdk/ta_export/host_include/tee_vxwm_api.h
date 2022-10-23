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

#ifndef _TEE_VXWM_API_H_
#define _TEE_VXWM_API_H_

#include <stdlib.h>

typedef enum VM_HW_Err {
	VM_HW_OK = 0,
	VM_HW_PARAM_ERROR,
	VM_HW_FAIL,
} VM_HW_Err_t;

// WaterMark Core Parameters
typedef struct VMX_CORE_PARAMETERS_REND {
//versioning
	uint8_t		version_major;
	uint8_t		version_minor;
//embedding part
	uint8_t		watermark_on;
	uint8_t		frequency_distance[3][3];
	uint8_t		background_embedding_on;
	uint16_t	embedding_strength_threshold_8[3];
	uint16_t	embedding_strength_threshold_bg_8[3];
	uint16_t	embedding_strength_threshold_10[12];
	uint16_t	embedding_strength_threshold_bg_10[12];
	uint16_t	embedding_strength_threshold_12[48];
	uint16_t	embedding_strength_threshold_bg_12[48];
	uint16_t	direction_max;
	int8_t		strength_multiply;
//rendering part
	uint8_t		payload_symbols[1920];
	uint8_t		symbols_rows;
	uint8_t		symbols_cols;
	uint8_t		symbols_xpos;
	uint8_t		symbols_ypos;
	uint8_t		symbol_size;
	uint16_t	spacing_vert;
	uint16_t	spacing_horz;
	uint8_t		symbol_scale_control;
} vmx_hw_soc_rend_t;

VM_HW_Err_t VM_HW_Init(void);
VM_HW_Err_t VM_HW_Term(void);

VM_HW_Err_t VM_HW_OpenSession(uint8_t bServiceIdx);
VM_HW_Err_t VM_HW_CloseSession(uint8_t bServiceIdx);

VM_HW_Err_t VM_HW_SetParameters_Rend(uint8_t bServiceIdx,
				vmx_hw_soc_rend_t* hwParameters);
VM_HW_Err_t VM_HW_SetParameters_Last(uint8_t bServiceIdx, uint8_t *arg,
		unsigned int len);

VM_HW_Err_t VM_HW_SetCWC(uint8_t bServiceIdx, uint8_t* Kwmcwc, size_t len,
		uint8_t klStage);

#endif // _TEE_VXWM_API_H_
