/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AML_GE2D_H_
#define AML_GE2D_H_

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct aml_ge2d {
    aml_ge2d_info_t *pge2d_info;
    char *src_data;
    char *src2_data;
    char *dst_data;
    unsigned int src_size;
    unsigned int src2_size;
    unsigned int dst_size;
} aml_ge2d_t;

int aml_ge2d_init(void);
void aml_ge2d_exit(void);

int aml_ge2d_mem_alloc(aml_ge2d_info_t *pge2dinfo);
void aml_ge2d_mem_free(aml_ge2d_info_t *pge2dinfo);
int aml_ge2d_process(aml_ge2d_info_t *pge2dinfo);
void aml_ge2d_get_cap(void);
int  aml_ge2d_invalid_cache(aml_ge2d_info_t *pge2dinfo);

#if defined (__cplusplus)
}
#endif


#endif

