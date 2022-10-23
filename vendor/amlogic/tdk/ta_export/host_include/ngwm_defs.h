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

#ifndef _NGWM_DEFS_H_
#define _NGWM_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NGWM_QUOTE(x)                 #x
#define NGWM_QUOTE_EXPAND(x)          NGWM_QUOTE(x)
#define NGWM_INTERFACE_VERSION_STRING( head, major, medium, minor ) \
            #head NGWM_QUOTE_EXPAND(major) "." NGWM_QUOTE_EXPAND(medium) \
	    "." NGWM_QUOTE_EXPAND(minor)
#define NGWM_INTERFACE_VERSION_INT( major, medium, minor ) \
                           ( (uint32_t)( (major)<<16 | (medium)<<8 | (minor) ) )

typedef void* TNgwmEmbedder;

#ifdef __cplusplus
}
#endif

#endif /* defined _NGWM_DEFS_H_ */
