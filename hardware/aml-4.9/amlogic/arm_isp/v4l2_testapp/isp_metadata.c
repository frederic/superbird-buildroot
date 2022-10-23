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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "isp_metadata.h"

#define LOG2_GAIN_SHIFT 18                  // copied from apical_firmware_config.h
#define WDR_MODE_COUNT  0x00000004          // copied from apical_api_command.h
#define APICAL_ABS(a)   ((a)>=0?(a):-(a))   // copied from apical_math.h

// copied from apical_math.c
const unsigned int _pow2_lut[33]={
        1073741824,1097253708,1121280436,1145833280,1170923762,1196563654,1222764986,1249540052,
        1276901417,1304861917,1333434672,1362633090,1392470869,1422962010,1454120821,1485961921,
        1518500250,1551751076,1585730000,1620452965,1655936265,1692196547,1729250827,1767116489,
        1805811301,1845353420,1885761398,1927054196,1969251188,2012372174,2056437387,2101467502,
        2147483648U};

// copied from apical_math.c
static uint32_t math_exp2(uint32_t val, const uint8_t shift_in, const uint8_t shift_out)
{
    unsigned int fract_part = (val & ((1<<shift_in)-1));
    unsigned int int_part = val >> shift_in;
    if (shift_in <= 5)
    {
        unsigned int lut_index = fract_part << (5-shift_in);
//      if ((lut_index>=33)||(lut_index<0))
//          LOG(LOG_DEBUG,"error: math_exp2 out of range\n");
        return _pow2_lut[lut_index] >> (30 - shift_out - int_part);
    }
    else
    {
        unsigned int lut_index = fract_part >> (shift_in - 5);
        unsigned int lut_fract = fract_part & ((1<<(shift_in-5))-1);
        unsigned int a = _pow2_lut[lut_index];
        unsigned int b =  _pow2_lut[lut_index+1];
        unsigned int res = ((unsigned long long)(b - a)*lut_fract) >> (shift_in-5);
        res = (res + a) >> (30-shift_out-int_part);
//      if ((lut_index>=32)||(lut_index<0))
//          LOG(LOG_DEBUG,"error: math_exp2 out of range\n");


        return res;
    }
}

static void dump_gain(const char* name, int32_t gain_log2)
{
    int32_t gain_log10 = gain_log2*6; // 1% error
    uint16_t gain_dec = (uint16_t)(((APICAL_ABS(gain_log10)&((1<<LOG2_GAIN_SHIFT)-1))*1000)>>LOG2_GAIN_SHIFT);
    int32_t gain_ones = math_exp2(gain_log2, LOG2_GAIN_SHIFT, LOG2_GAIN_SHIFT); // 1% error
    uint16_t gain_ones_dec = (uint16_t)(((APICAL_ABS(gain_ones)&((1<<LOG2_GAIN_SHIFT)-1))*1000)>>LOG2_GAIN_SHIFT);
    printf("%s gain %d.%03d = %d.%03d dB\n", name, gain_ones>>LOG2_GAIN_SHIFT, gain_ones_dec, gain_log10>>LOG2_GAIN_SHIFT, gain_dec);
}

void metadata_dump(const firmware_metadata_t *md)
{
    printf("Format: %d\n", md->image_format);
    printf("Sensor bits: %d\n", md->sensor_bits);
    printf("RGGB start: %d\n", md->rggb_start);
    const char* modes[WDR_MODE_COUNT] = {"Linear","FS HDR","Native HDR", "FS Linear"};
    printf("ISP mode: %s\n", modes[md->isp_mode]);
    printf("FPS: %d.%02d\n", md->fps>>8,((md->fps&0xFF)*100)>>8);

    printf("Integration time: %d lines %ld.%02ld ms\n", md->int_time, md->int_time_ms/100, md->int_time_ms%100);
    printf("Integration time medium: %d\n", md->int_time_medium);
    printf("Integration time long: %d\n", md->int_time_long);
    dump_gain("A",md->again);
    dump_gain("D",md->dgain);
    dump_gain("ISP",md->isp_dgain);
    printf("Equivalent Exposure: %d lines\n", math_exp2(md->exposure, LOG2_GAIN_SHIFT, 0));
    printf("Exposure_log2: %d\n", md->exposure);
    printf("Gain_log2: %d\n", md->gain_log2);

    printf("Lens Position: %d\n", md->lens_pos);

    printf("Antiflicker: %s\n", md->anti_flicker?"on":"off");

    printf("Gain 00: %d\n", md->gain_00);
    printf("Gain 01: %d\n", md->gain_01);
    printf("Gain 10: %d\n", md->gain_10);
    printf("Gain 11: %d\n", md->gain_11);
    printf("Black Level 00: %d\n", md->black_level_00);
    printf("Black Level 01: %d\n", md->black_level_01);
    printf("Black Level 10: %d\n", md->black_level_10);
    printf("Black Level 11: %d\n", md->black_level_11);
    
    printf("LSC table: %d\n", md->lsc_table);
    printf("LSC blend: %d\n", md->lsc_blend);
    printf("LSC Mesh strength: %d\n", md->lsc_mesh_strength);

    printf("AWB rg: %ld\n", md->awb_rgain);
    printf("AWB bg: %ld\n", md->awb_bgain);
    printf("AWB temperature: %ld\n", md->awb_cct);

    printf("Sinter strength: %d\n", md->sinter_strength);
    printf("Sinter strength1: %d\n", md->sinter_strength1);
    printf("Sinter strength4: %d\n", md->sinter_strength4);
    printf("Sinter thresh1h: %d\n", md->sinter_thresh_1h);
    printf("Sinter thresh4h: %d\n", md->sinter_thresh_4h);
    printf("Sinter SAD: %d\n", md->sinter_sad);

    printf("Temper strength: %d\n", md->temper_strength);

    printf("Iridix strength: %d\n", md->iridix_strength);

    printf("Dp threshold1: %d\n", md->dp_threash1);
    printf("Dp slope1: %d\n", md->dp_slope1);
    printf("Dp threshold2: %d\n", md->dp_threash2);
    printf("Dp slope2: %d\n", md->dp_slope2);

    printf("Sharpening directional: %d\n", md->sharpening_directional);
    printf("Sharpening unidirectional: %d\n", md->sharpening_unidirectional);
    printf("Demosaic NP offset: %d\n", md->demosaic_np_offset);

    printf("FR sharpen strength: %d\n", md->fr_sharpern_strength);
    printf("DS1 sharpen strength: %d\n", md->ds1_sharpen_strength);
    printf("DS2 sharpen strength: %d\n", md->ds2_sharpen_strength);

    printf("CCM R_R: 0x%X\n", md->ccm[CCM_R][CCM_R]);
    printf("CCM R_G: 0x%X\n", md->ccm[CCM_R][CCM_G]);
    printf("CCM R_B: 0x%X\n", md->ccm[CCM_R][CCM_B]);
    printf("CCM G_R: 0x%X\n", md->ccm[CCM_G][CCM_R]);
    printf("CCM G_G: 0x%X\n", md->ccm[CCM_G][CCM_G]);
    printf("CCM G_B: 0x%X\n", md->ccm[CCM_G][CCM_B]);
    printf("CCM B_R: 0x%X\n", md->ccm[CCM_B][CCM_R]);
    printf("CCM B_G: 0x%X\n", md->ccm[CCM_B][CCM_G]);
    printf("CCM B_B: 0x%X\n", md->ccm[CCM_B][CCM_B]);
}

static void format_gain(char* buf, const char* name, int32_t gain_log2)
{
    int32_t gain_log10 = gain_log2*6; // 1% error
    uint16_t gain_dec = (uint16_t)(((APICAL_ABS(gain_log10)&((1<<LOG2_GAIN_SHIFT)-1))*1000)>>LOG2_GAIN_SHIFT);
    int32_t gain_ones = math_exp2(gain_log2, LOG2_GAIN_SHIFT, LOG2_GAIN_SHIFT); // 1% error
    uint16_t gain_ones_dec = (uint16_t)(((APICAL_ABS(gain_ones)&((1<<LOG2_GAIN_SHIFT)-1))*1000)>>LOG2_GAIN_SHIFT);
    sprintf(buf,"%s gain %d.%03d = %d.%03d dB\n",name,gain_ones>>LOG2_GAIN_SHIFT,gain_ones_dec,gain_log10>>LOG2_GAIN_SHIFT,gain_dec);
}

int32_t fill_meta_buf(char *metadata_buf, firmware_metadata_t *md) {
    char       * buf = metadata_buf;
    const char * modes[] = {"Linear","FS HDR","Native HDR", "FS Linear"};

    /* clear buffer */
    buf[0] = 0;

    sprintf(buf, "Format: %d\n", md->image_format);                         buf += strlen(buf);
    sprintf(buf, "Sensor bits: %d\n", md->sensor_bits);                     buf += strlen(buf);
    sprintf(buf, "RGGB start: %d\n", md->rggb_start);                       buf += strlen(buf);
    sprintf(buf, "ISP mode: %s\n", modes[md->isp_mode]);                    buf += strlen(buf);
    sprintf(buf, "FPS: %d.%02d\n", md->fps>>8, ((md->fps&0xFF)*100)>>8);    buf += strlen(buf);

    sprintf(buf, "Integration time: %d lines %ld.%02ld ms\n",
                md->int_time, md->int_time_ms/100, md->int_time_ms%100);    buf += strlen(buf);
    sprintf(buf, "Integration time medium: %d\n", md->int_time_medium);    buf += strlen(buf);
    sprintf(buf, "Integration time long: %d\n", md->int_time_long);        buf += strlen(buf);
    format_gain(buf, "A",md->again);                                        buf += strlen(buf);
    format_gain(buf, "D",md->dgain);                                        buf += strlen(buf);
    format_gain(buf, "ISP",md->isp_dgain);                                  buf += strlen(buf);
    sprintf(buf, "Equivalent Exposure: %d lines\n",
                math_exp2(md->exposure, LOG2_GAIN_SHIFT, 0));               buf += strlen(buf);
    sprintf(buf, "Exposure_log2: %d\n", md->exposure);                     buf += strlen(buf);
    sprintf(buf, "Gain_log2: %d\n", md->gain_log2);                        buf += strlen(buf);

    sprintf(buf, "Lens Position: %d\n", md->lens_pos);                      buf += strlen(buf);

    sprintf(buf, "Antiflicker: %s\n", md->anti_flicker?"on":"off");         buf += strlen(buf);

    sprintf(buf, "Gain 00: %d\n", md->gain_00);                             buf += strlen(buf);
    sprintf(buf, "Gain 01: %d\n", md->gain_01);                             buf += strlen(buf);
    sprintf(buf, "Gain 10: %d\n", md->gain_10);                             buf += strlen(buf);
    sprintf(buf, "Gain 11: %d\n", md->gain_11);                             buf += strlen(buf);
    sprintf(buf, "Black Level 00: %d\n", md->black_level_00);               buf += strlen(buf);
    sprintf(buf, "Black Level 01: %d\n", md->black_level_01);               buf += strlen(buf);
    sprintf(buf, "Black Level 10: %d\n", md->black_level_10);               buf += strlen(buf);
    sprintf(buf, "Black Level 11: %d\n", md->black_level_11);               buf += strlen(buf);

    sprintf(buf, "LSC table: %d\n", md->lsc_table);                         buf += strlen(buf);
    sprintf(buf, "LSC blend: %d\n", md->lsc_blend);                         buf += strlen(buf);
    sprintf(buf, "LSC Mesh strength: %d\n", md->lsc_mesh_strength);         buf += strlen(buf);

    sprintf(buf, "AWB rg: %ld\n", md->awb_rgain);                           buf += strlen(buf);
    sprintf(buf, "AWB bg: %ld\n", md->awb_bgain);                           buf += strlen(buf);
    sprintf(buf, "AWB temperature: %ld\n", md->awb_cct);                    buf += strlen(buf);

    sprintf(buf, "Sinter strength: %d\n", md->sinter_strength);             buf += strlen(buf);
    sprintf(buf, "Sinter strength1: %d\n", md->sinter_strength1);           buf += strlen(buf);
    sprintf(buf, "Sinter strength4: %d\n", md->sinter_strength4);           buf += strlen(buf);
    sprintf(buf, "Sinter thresh1h: %d\n", md->sinter_thresh_1h);            buf += strlen(buf);
    sprintf(buf, "Sinter thresh4h: %d\n", md->sinter_thresh_4h);            buf += strlen(buf);
    sprintf(buf, "Sinter SAD: %d\n", md->sinter_sad);                       buf += strlen(buf);

    sprintf(buf, "Temper strength: %d\n", md->temper_strength);             buf += strlen(buf);

    sprintf(buf, "Iridix strength: %d\n", md->iridix_strength);             buf += strlen(buf);

    sprintf(buf, "Dp threshold1: %d\n", md->dp_threash1);                   buf += strlen(buf);
    sprintf(buf, "Dp slope1: %d\n", md->dp_slope1);                         buf += strlen(buf);
    sprintf(buf, "Dp threshold2: %d\n", md->dp_threash2);                   buf += strlen(buf);
    sprintf(buf, "Dp slope2: %d\n", md->dp_slope2);                         buf += strlen(buf);

    sprintf(buf, "Sharpening directional: %d\n", md->sharpening_directional);       buf += strlen(buf);
    sprintf(buf, "Sharpening unidirectional: %d\n", md->sharpening_unidirectional); buf += strlen(buf);
    sprintf(buf, "Demosaic NP offset: %d\n", md->demosaic_np_offset);               buf += strlen(buf);

    sprintf(buf, "FR sharpen strength: %d\n", md->fr_sharpern_strength);    buf += strlen(buf);
    sprintf(buf, "DS1 sharpen strength: %d\n", md->ds1_sharpen_strength);   buf += strlen(buf);
    sprintf(buf, "DS2 sharpen strength: %d\n", md->ds2_sharpen_strength);   buf += strlen(buf);

    sprintf(buf, "CCM R_R: 0x%X\n", md->ccm[CCM_R][CCM_R]); buf += strlen(buf);
    sprintf(buf, "CCM R_G: 0x%X\n", md->ccm[CCM_R][CCM_G]); buf += strlen(buf);
    sprintf(buf, "CCM R_B: 0x%X\n", md->ccm[CCM_R][CCM_B]); buf += strlen(buf);
    sprintf(buf, "CCM G_R: 0x%X\n", md->ccm[CCM_G][CCM_R]); buf += strlen(buf);
    sprintf(buf, "CCM G_G: 0x%X\n", md->ccm[CCM_G][CCM_G]); buf += strlen(buf);
    sprintf(buf, "CCM G_B: 0x%X\n", md->ccm[CCM_G][CCM_B]); buf += strlen(buf);
    sprintf(buf, "CCM B_R: 0x%X\n", md->ccm[CCM_B][CCM_R]); buf += strlen(buf);
    sprintf(buf, "CCM B_G: 0x%X\n", md->ccm[CCM_B][CCM_G]); buf += strlen(buf);
    sprintf(buf, "CCM B_B: 0x%X\n", md->ccm[CCM_B][CCM_B]); buf += strlen(buf);

    return (int32_t)(buf - metadata_buf);
}
