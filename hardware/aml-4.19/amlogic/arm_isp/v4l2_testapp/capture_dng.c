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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "lib/tiffio.h"

#include "common.h"
#include "isp_metadata.h"
#include "capture.h"
#include "capture_dng.h"
#include "capture_dng_params.h"
#include "logs.h"

#define RGB_SAMPLES_PER_PIXEL   4
#define DNG_SAMPLES_PER_PIXEL   1

/*
 * Opcode2 function
 *
 * > Header structure
 *   4 bytes : (unsigned int) Opcode count
 *
 * > Opcode structure
 *   4 bytes : (unsigned int) Opcode ID
 *   4 bytes : (unsigned int) DNG version for Opcode
 *   4 bytes : (unsigned int) flags (bit-0: optional opcode, bit-1: skip for preview quality)
 *   4 bytes : (unsigned int) size
 *
 * > Value
 *     10 * 4 bytes (long)      = 40    (T/L/B/R, plane/planes, col/row-picth, MapPoint-V/H)
 *      4 * 8 bytes (double)    = 32    (MapSpacing-V/H, MapOrigin-V/H)
 *      1 * 4 byte  (long)      = 4     (MapPlanes)
 *   1024 * 4 bytes (float)     = 4096  (32 * 32 Map)
 *
 * > Total size in bytes
 *   4 + 4 * (16 + 76 + 4096) = 16756
 *
 */

#define TIFFTAG_OPCODELIST2     51009

static const TIFFFieldInfo xtiffFieldInfo[] = {
    { TIFFTAG_OPCODELIST2,  -1, -1, TIFF_UNDEFINED, FIELD_CUSTOM,   1,  1,  "OpCodeList2" }
};

static TIFFExtendProc parent_extender = NULL;

static void registerCustomTIFFTags(TIFF *tif)
{
    /* Install the extended Tag field info */
    int error = TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo)/sizeof(xtiffFieldInfo[0]));

    if (parent_extender)
        (*parent_extender)(tif);
}

#define GAIN_MAP_SIZE   1024
#define OP_CODE_NUM     4

typedef struct _dng_opcode2_gainmap_value {
    uint32_t top;
    uint32_t left;
    uint32_t bottom;
    uint32_t right;
    uint32_t plane;
    uint32_t planes;
    uint32_t row_picth;
    uint32_t col_picth;
    uint32_t map_point_v;
    uint32_t map_point_h;
    double   map_spacing_v;
    double   map_spacing_h;
    double   map_origin_v;
    double   map_origin_h;
    uint32_t map_planes;
    float gainmap_f[GAIN_MAP_SIZE];
} __attribute__((packed)) dng_opcode2_gainmap_value_t;

typedef struct _dng_opcode2_gainmap {
    uint32_t opcode_id;
    uint32_t dng_version;
    uint32_t flags;
    uint32_t size;
    dng_opcode2_gainmap_value_t value;
} __attribute__((packed)) dng_opcode2_gainmap_t;

typedef struct _dng_opcode2 {
    uint32_t pHeader;
    dng_opcode2_gainmap_t gainmap[OP_CODE_NUM];
} __attribute__((packed)) dng_opcode2_t;

static void get_dng_opcode2_lsc_gain_map_4ch(uint8_t * dst,
                                            uint32_t width, uint32_t height,
                                            const uint8_t *tbl_r,
                                            const uint8_t *tbl_g,
                                            const uint8_t *tbl_b,
                                            cfa_order_t cfa_order)
{
    dng_opcode2_t *opcode2;
    const uint8_t *gainmap_i[OP_CODE_NUM];
    uint32_t *ptr32_src;
    uint32_t *ptr32_dst;
    int i, j;

    /* allocate opcode2 memory */
    opcode2 = (dng_opcode2_t *)malloc(sizeof(dng_opcode2_t));
    memset(opcode2, 0x0, sizeof(dng_opcode2_t));

    /* put integer table in an order */
    switch(cfa_order) {
    case V4L2_TEST_CFA_ORDER_RGGB:
        gainmap_i[0] = tbl_r;
        gainmap_i[1] = tbl_g;
        gainmap_i[2] = tbl_g;
        gainmap_i[3] = tbl_b;
        break;
    case V4L2_TEST_CFA_ORDER_BGGR:
        gainmap_i[0] = tbl_b;
        gainmap_i[1] = tbl_g;
        gainmap_i[2] = tbl_g;
        gainmap_i[3] = tbl_r;
        break;
    case V4L2_TEST_CFA_ORDER_GRBG:
        gainmap_i[0] = tbl_g;
        gainmap_i[1] = tbl_r;
        gainmap_i[2] = tbl_b;
        gainmap_i[3] = tbl_g;
        break;
    case V4L2_TEST_CFA_ORDER_GBRG:
        gainmap_i[0] = tbl_g;
        gainmap_i[1] = tbl_b;
        gainmap_i[2] = tbl_r;
        gainmap_i[3] = tbl_g;
        break;
    default:
        printf("Invalid CFA order ! (0x%x)", cfa_order);
        exit(1);
        break;
    }

    /* fill opcode 2 structure */
    opcode2->pHeader = OP_CODE_NUM;
    for(i = 0; i < OP_CODE_NUM; i++) {
        // OpCode header
        opcode2->gainmap[i].opcode_id = 9;
        opcode2->gainmap[i].dng_version = 0x01030000;
        opcode2->gainmap[i].flags = 0x0;
        opcode2->gainmap[i].size
                = sizeof(dng_opcode2_gainmap_value_t);

        // OpCode value
        opcode2->gainmap[i].value.top = 0; // need to be changed dynamically
        opcode2->gainmap[i].value.left = 0; // need to be changed dynamically
        opcode2->gainmap[i].value.bottom = height;
        opcode2->gainmap[i].value.right = width;
        opcode2->gainmap[i].value.plane = 0;
        opcode2->gainmap[i].value.planes = 1;
        opcode2->gainmap[i].value.row_picth = 2;
        opcode2->gainmap[i].value.col_picth = 2;
        opcode2->gainmap[i].value.map_point_v = 32;
        opcode2->gainmap[i].value.map_point_h = 32;
        opcode2->gainmap[i].value.map_spacing_v = ((double)1.0)/32;
        opcode2->gainmap[i].value.map_spacing_h = ((double)1.0)/32;
        opcode2->gainmap[i].value.map_origin_v = (double)0;
        opcode2->gainmap[i].value.map_origin_h = (double)0;
        opcode2->gainmap[i].value.map_planes = 1;
        for(j = 0; j < GAIN_MAP_SIZE; j++) {
            opcode2->gainmap[i].value.gainmap_f[j] = ((float)gainmap_i[i][j]) / 64;
        }
    }

    /* set top/left coordination */
    opcode2->gainmap[0].value.top = 0;
    opcode2->gainmap[0].value.left = 0;
    opcode2->gainmap[1].value.top = 1;
    opcode2->gainmap[1].value.left = 0;
    opcode2->gainmap[2].value.top = 0;
    opcode2->gainmap[2].value.left = 1;
    opcode2->gainmap[3].value.top = 1;
    opcode2->gainmap[3].value.left = 1;

    /* Copy header to dst memory in big endian byte order */
    ptr32_src = (uint32_t *)opcode2;
    ptr32_dst = (uint32_t *)dst;
    for(i = 0; i < sizeof(dng_opcode2_t); i+=4) {
        uint32_t val = ptr32_src[i>>2];
        ptr32_dst[i>>2] = ((val & 0xff000000) >> 24)
                        | ((val & 0x00ff0000) >> 8)
                        | ((val & 0x0000ff00) << 8)
                        | ((val & 0x000000ff) << 24);
    }
    for(i = 0; i < OP_CODE_NUM; i++) {
        uint64_t *dst_pos = (uint64_t *)&(ptr32_dst[(4 + sizeof(dng_opcode2_gainmap_t) * i + 56)>>2]);

        for(j = 0; j < 4; j++) {
            uint64_t value = dst_pos[j];
            dst_pos[j] =  ((value & 0xffffffff00000000) >> 32)
                        | ((value & 0x00000000ffffffff) << 32);
        }
    }

    /* release resources */
    free(opcode2);
}

void process_capture_dng(frame_pack_t * pframep) {
    int             frame_id = pframep->frame_id;
    char            filename[FILE_NAME_LENGTH];
    TIFF            * tiffout;
    frame_t         * pframe;
    uint8_t         * buf = NULL;

    time_t          t = time(NULL);
    struct tm       tm = *localtime(&t);

    int             i, j, k,l;

    /**************************************************
     * do Tiff capture for FR
     **************************************************/
    pframe = &pframep->frame_data[ARM_V4L2_TEST_STREAM_FR];
    for(l=0;l<pframe->num_planes;l++){
		buf = pframe->paddr[l];

		sprintf(filename, "IMG%06d_%04d%02d%02d_%02d%02d%02d_rgb-%d.tif\0",
				frame_id, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,l);

		MSG("\nDoing TIF capture for FR output.\n");
		MSG("- frame id   : %d\n", frame_id);
		MSG("- filename   : %s\n", filename);
		MSG("- resolution : %d x %d.\n", pframe->width[l], pframe->height[l]);
		MSG("- format     : RGBA 32-bit\n");

		tiffout = NULL;
		tiffout = TIFFOpen(filename, "w");

		TIFFSetField(tiffout, TIFFTAG_IMAGEWIDTH, pframe->width[l]);
		TIFFSetField(tiffout, TIFFTAG_IMAGELENGTH, pframe->height[l]);
		TIFFSetField(tiffout, TIFFTAG_SAMPLESPERPIXEL, RGB_SAMPLES_PER_PIXEL);
		TIFFSetField(tiffout, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_UNSPECIFIED);
		TIFFSetField(tiffout, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(tiffout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tiffout, TIFFTAG_ROWSPERSTRIP, pframe->height[l]);
		TIFFSetField(tiffout, TIFFTAG_XRESOLUTION, 88.0);
		TIFFSetField(tiffout, TIFFTAG_YRESOLUTION, 88.0);
		TIFFSetField(tiffout, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
		TIFFSetField(tiffout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(tiffout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		TIFFSetField(tiffout, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		TIFFSetField(tiffout, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);

		for (i = 0; i < pframe->height[l]; i++)
		{
			uint8_t buffer[pframe->width[l]][RGB_SAMPLES_PER_PIXEL];
			for (j = 0; j < pframe->width[l]; j++) {
				uint8_t val = j & 0xFF;
				for(k = 0; k < RGB_SAMPLES_PER_PIXEL-1; k++)
					buffer[j][k] = buf[(i * pframe->width[l] + j) * RGB_SAMPLES_PER_PIXEL + k];
				buffer[j][RGB_SAMPLES_PER_PIXEL-1] = 0xFF;
			}

			TIFFWriteScanline(tiffout, buffer, i, 0);
		}
    }
    TIFFClose(tiffout);
    MSG("TIF capture for FR output is done ...\n");


#if ARM_V4L2_TEST_HAS_META && ARM_V4L2_TEST_HAS_RAW
    /**************************************************
     * do DNG capture for RAW
     **************************************************/

    /* register custom TIFF tag */
    static uint8_t tag_registered = 0;
    if (tag_registered == 0) {
        parent_extender = TIFFSetTagExtender(registerCustomTIFFTags);
        tag_registered = 1;
    }


    /* buffer ptr resources */
    frame_t * pmeta;
    firmware_metadata_t *buf_meta;

    pframe = &pframep->frame_data[ARM_V4L2_TEST_STREAM_RAW];
    pmeta = &pframep->frame_data[ARM_V4L2_TEST_STREAM_META];

    for(l=0;l<pframe->num_planes;l++){
		buf = pframe->paddr[l];
		buf_meta = pmeta->paddr[0];


		/* filename */
		sprintf(filename, "IMG%06d_%04d%02d%02d_%02d%02d%02d_raw-%d.dng\0",
				frame_id, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,l);


		/* Variables for tag values */
		double   aperture_value = 2.8;
		double   focal_length = 4.628;
		uint32_t dng_bytes_per_sample = (buf_meta->sensor_bits + 7) / 8;
		uint32_t dng_bits_per_sample = dng_bytes_per_sample * 8;
		uint32_t dng_pixel_depth = DNG_SAMPLES_PER_PIXEL * dng_bytes_per_sample;
		uint32_t dynamic_range_max = (uint32_t)pow(2, buf_meta->sensor_bits) - 1;
		float wb_gain[3];
		switch(buf_meta->rggb_start) {
		case V4L2_TEST_CFA_ORDER_RGGB:
			wb_gain[0] = buf_meta->gain_00 / 256.0;
			wb_gain[1] = buf_meta->gain_01 / 256.0;
			wb_gain[2] = buf_meta->gain_11 / 256.0;
			break;
		case V4L2_TEST_CFA_ORDER_GRBG:
			wb_gain[0] = buf_meta->gain_01 / 256.0;
			wb_gain[1] = buf_meta->gain_00 / 256.0;
			wb_gain[2] = buf_meta->gain_10 / 256.0;
			break;
		case V4L2_TEST_CFA_ORDER_GBRG:
			wb_gain[0] = buf_meta->gain_10 / 256.0;
			wb_gain[1] = buf_meta->gain_00 / 256.0;
			wb_gain[2] = buf_meta->gain_01 / 256.0;
			break;
		case V4L2_TEST_CFA_ORDER_BGGR:
			wb_gain[0] = buf_meta->gain_11 / 256.0;
			wb_gain[1] = buf_meta->gain_01 / 256.0;
			wb_gain[2] = buf_meta->gain_00 / 256.0;
			break;
		default:
			printf("Invalid rggb order ! (0x%x)", buf_meta->rggb_start);
			exit(1);
			break;
		}

		/* tag values */
		char     Datetime[20];
		sprintf(Datetime, "%04d:%02d:%02d %02d:%02d:%02d\0",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		int16_t     CFARepeatPatternDim[] = { CFA_WIDTH, CFA_HEIGHT };
		cfa_order_t CFAPatternID = buf_meta->rggb_start;
		uint16_t Curve[dynamic_range_max + 1];                                  // TBD
		for (i = 0; i < dynamic_range_max + 1; i++)
			Curve[i] = i;
		int16_t  BlackLevelRepeatDim[] = { BLACK_LEVEL_WIDTH, BLACK_LEVEL_HEIGHT };
		float    BlackLevel[] = { ( buf_meta->black_level_00 + buf_meta->black_level_01
								  + buf_meta->black_level_10 + buf_meta->black_level_11 ) / 4 };
		uint32_t WhiteLevel[] = { dynamic_range_max };
		float    DefaultScale[] = { 1.0, 1.0 };                                 // TBD
		float    CropOrigin[] = { 2.0, 2.0 };                                   // TBD
		float    CropSize[] = { pframe->width[l] - 4.0, pframe->height[l] - 4.0 };    // TBD
		float    ColorMatrix1[] =                                               // TBD
		{
			 3.2407078520, -1.5372589857, -0.4985696418,
			-0.9692573409,  1.8759952844,  0.0415547482,
			 0.0556364457, -0.2039964639,  1.0570694467
		};
		float    ColorMatrix2[] =                                               // TBD
		{
			 3.2407078520, -1.5372589857, -0.4985696418,
			-0.9692573409,  1.8759952844,  0.0415547482,
			 0.0556364457, -0.2039964639,  1.0570694467
		};
		float    AnalogBalance[] = { 1.0, 1.0, 1.0 };                           // TBD
		float    AsShotNeutral[] = { wb_gain[0], wb_gain[1], wb_gain[2] };
		double   BaselineExposure = (-2.0)/3.0;                                 // TBD
		double   BaselineNoise = 2.0;                                           // TBD
		double   BaselineSharpness = 1.0/2.0;                                   // TBD
		double   LinearResponseLimit = 1.0 ;                                    // TBD
		float    LensInfo[] = { focal_length, focal_length,
								aperture_value, aperture_value };
		double   AntiAliasStrength = 1.0;                                       // TBD
		double   ShadowScale = 1.0;                                             // TBD
		double   BestQualityScale = 1.0;                                        // TBD
		uint8_t  RawDataUniqueID[] =
		{
			0xA, 0x10, 0x1, 0xC, 0xA, 0x1, 0x0, 0x0,
			0xA, 0x12, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0
		};

		/* OpCodeList2 tag values */
		uint8_t *opcode2_data = (uint8_t *)malloc(sizeof(dng_opcode2_t));
		uint32_t opcode2_size = sizeof(dng_opcode2_t);
		memset(opcode2_data, 0x0, sizeof(dng_opcode2_t));
		get_dng_opcode2_lsc_gain_map_4ch(
								opcode2_data,
								pframe->width[l], pframe->height[l],
								calibration_shading_ls_d65_r,
								calibration_shading_ls_d65_g,
								calibration_shading_ls_d65_b,
								CFAPatternID);

		/* Exif tag values */
		double   ExposureTime = 0.25;
		double   FNumber = aperture_value;
		uint16_t ExposureProgram = EXIF_EXPOSURE_PROGRAM_NOT_DEFINED;
		uint16_t IsoSpeedRatings[] = { 100 };
		double   ShutterSpeedValue = 1.0/60.0;
		double   ApertureValue = log2(aperture_value) * 2;
		double   ExposureBiasValue = 0.0;
		double   MaxApertureValue = log2(aperture_value) * 2;
		uint16_t MeteringMode = EXIF_METERING_MODE_AVERAGE;
		uint16_t Flash = EXIF_FLASH_FUNCTION_NOT_PRESENT;
		double   FocalLength = focal_length;

		uint64_t ExifIFD = 0;

		MSG("\nDoing DNG capture for RAW output ...\n");
		MSG("- frame id   : %d\n", frame_id);
		MSG("- filename   : %s\n", filename);
		MSG("- resolution : %d x %d\n", pframe->width[l], pframe->height[l]);
		MSG("- format     : RAW%d bayer RGGB\n", dng_bits_per_sample);

		/* Start DNG file creation */
		tiffout = NULL;
		tiffout = TIFFOpen(filename, "w");

		/* TIF tags for RAW image */
		DBG("- Adding general TIF tags ...\n");
		TIFFSetField(tiffout, TIFFTAG_SUBFILETYPE, 0);                                  // ID:254
		TIFFSetField(tiffout, TIFFTAG_IMAGEWIDTH, pframe->width[l]);                       // ID:256
		TIFFSetField(tiffout, TIFFTAG_IMAGELENGTH, pframe->height[l]);                     // ID:257
		TIFFSetField(tiffout, TIFFTAG_BITSPERSAMPLE, dng_bits_per_sample);              // ID:258
		TIFFSetField(tiffout, TIFFTAG_COMPRESSION, COMPRESSION_NONE);                   // ID:259
		TIFFSetField(tiffout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_CFA);                    // ID:262
		TIFFSetField(tiffout, TIFFTAG_MAKE, "ARM");                                     // ID:271
		TIFFSetField(tiffout, TIFFTAG_MODEL, "Juno-DNG");                               // ID:272
		TIFFSetField(tiffout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);                // ID:274
		TIFFSetField(tiffout, TIFFTAG_SAMPLESPERPIXEL, DNG_SAMPLES_PER_PIXEL);          // ID:277
		TIFFSetField(tiffout, TIFFTAG_ROWSPERSTRIP, pframe->height[l]);                    // ID:278
		TIFFSetField(tiffout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);               // ID:284
		TIFFSetField(tiffout, TIFFTAG_SOFTWARE, "v0.1");                                // ID:305
		TIFFSetField(tiffout, TIFFTAG_DATETIME, Datetime);                              // ID:306

		/* Private tags for CFA pattern */
		DBG("- Adding DNG general tags ...\n");
		TIFFSetField(tiffout, TIFFTAG_CFAREPEATPATTERNDIM, CFARepeatPatternDim);        // ID:33421
		switch(CFAPatternID) {
		case V4L2_TEST_CFA_ORDER_RGGB:
			TIFFSetField(tiffout, TIFFTAG_CFAPATTERN, "\00\01\01\02");                  // ID:33422
			break;
		case V4L2_TEST_CFA_ORDER_BGGR:
			TIFFSetField(tiffout, TIFFTAG_CFAPATTERN, "\02\01\01\00");                  // ID:33422
			break;
		case V4L2_TEST_CFA_ORDER_GRBG:
			TIFFSetField(tiffout, TIFFTAG_CFAPATTERN, "\01\00\02\01");                  // ID:33422
			break;
		case V4L2_TEST_CFA_ORDER_GBRG:
			TIFFSetField(tiffout, TIFFTAG_CFAPATTERN, "\01\02\00\01");                  // ID:33422
			break;
		default:
			printf("Invalid CFA pattern ID ! (0x%x)", CFAPatternID);
			exit(1);
			break;
		}

		/* Private tags for DNG upto version 1.2 */
		TIFFSetField(tiffout, TIFFTAG_DNGVERSION, "\1\3\0\0");                          // ID:50706
		TIFFSetField(tiffout, TIFFTAG_DNGBACKWARDVERSION, "\1\3\0\0");                  // ID:50707
		TIFFSetField(tiffout, TIFFTAG_UNIQUECAMERAMODEL, "ARM-ISP-Juno");               // ID:50708
		TIFFSetField(tiffout, TIFFTAG_CFAPLANECOLOR, 3, "\0\1\2");                      // ID:50710
		TIFFSetField(tiffout, TIFFTAG_CFALAYOUT, 0x1);                                  // ID:50711
		TIFFSetField(tiffout, TIFFTAG_LINEARIZATIONTABLE, dynamic_range_max, Curve);    // ID:50712
		TIFFSetField(tiffout, TIFFTAG_BLACKLEVELREPEATDIM, BlackLevelRepeatDim);        // ID:50713
		TIFFSetField(tiffout, TIFFTAG_BLACKLEVEL, 1, BlackLevel);                       // ID:50714
		TIFFSetField(tiffout, TIFFTAG_WHITELEVEL, 1, WhiteLevel);                       // ID:50717
		TIFFSetField(tiffout, TIFFTAG_DEFAULTSCALE, DefaultScale);                      // ID:50718
		TIFFSetField(tiffout, TIFFTAG_DEFAULTCROPORIGIN, CropOrigin);                   // ID:50719
		TIFFSetField(tiffout, TIFFTAG_DEFAULTCROPSIZE, CropSize);                       // ID:50720
		TIFFSetField(tiffout, TIFFTAG_COLORMATRIX1, 9, ColorMatrix1);                   // ID:50721
		TIFFSetField(tiffout, TIFFTAG_COLORMATRIX2, 9, ColorMatrix2);                   // ID:50722
		TIFFSetField(tiffout, TIFFTAG_ANALOGBALANCE, 3, AnalogBalance);                 // ID:50727
		TIFFSetField(tiffout, TIFFTAG_ASSHOTNEUTRAL, 3, AsShotNeutral);                 // ID:50728
		TIFFSetField(tiffout, TIFFTAG_BASELINEEXPOSURE, BaselineExposure);              // ID:50730
		TIFFSetField(tiffout, TIFFTAG_BASELINENOISE, BaselineNoise);                    // ID:50731
		TIFFSetField(tiffout, TIFFTAG_BASELINESHARPNESS, BaselineSharpness);            // ID:50732
		TIFFSetField(tiffout, TIFFTAG_BAYERGREENSPLIT, 1);                              // ID:50733
		TIFFSetField(tiffout, TIFFTAG_LINEARRESPONSELIMIT, LinearResponseLimit);        // ID:50734
		TIFFSetField(tiffout, TIFFTAG_LENSINFO, LensInfo);                              // ID:50736
		TIFFSetField(tiffout, TIFFTAG_ANTIALIASSTRENGTH, AntiAliasStrength);            // ID:50738
		TIFFSetField(tiffout, TIFFTAG_SHADOWSCALE, ShadowScale);                        // ID:50739
		TIFFSetField(tiffout, TIFFTAG_CALIBRATIONILLUMINANT1, 17);                      // ID:50778
		TIFFSetField(tiffout, TIFFTAG_CALIBRATIONILLUMINANT2, 21);                      // ID:50779
		TIFFSetField(tiffout, TIFFTAG_BESTQUALITYSCALE, BestQualityScale);              // ID:50780
		TIFFSetField(tiffout, TIFFTAG_RAWDATAUNIQUEID, RawDataUniqueID);                // ID:50781
		TIFFSetField(tiffout, TIFFTAG_ORIGINALRAWFILENAME, 12, "Not avaiable");         // ID:50827
		TIFFSetField(tiffout, TIFFTAG_OPCODELIST2, opcode2_size, opcode2_data);         // ID:51009

		TIFFCheckpointDirectory(tiffout);

	#if TIFF_USE_EXIF
		/* Exif tags */
		TIFFCreateEXIFDirectory(tiffout);
		TIFFSetField(tiffout, EXIFTAG_EXPOSURETIME, ExposureTime);                      // ID:33434
		TIFFSetField(tiffout, EXIFTAG_FNUMBER, FNumber);                                // ID:33437
		TIFFSetField(tiffout, EXIFTAG_EXPOSUREPROGRAM, ExposureProgram);                // ID:34850
		TIFFSetField(tiffout, EXIFTAG_ISOSPEEDRATINGS, 1, IsoSpeedRatings);             // ID:34855
		TIFFSetField(tiffout, EXIFTAG_DATETIMEORIGINAL, Datetime);                      // ID:36867
		TIFFSetField(tiffout, EXIFTAG_SHUTTERSPEEDVALUE, ShutterSpeedValue);            // ID:37377
		TIFFSetField(tiffout, EXIFTAG_APERTUREVALUE, ApertureValue);                    // ID:37378
		TIFFSetField(tiffout, EXIFTAG_EXPOSUREBIASVALUE, ExposureBiasValue);            // ID:37380
		TIFFSetField(tiffout, EXIFTAG_MAXAPERTUREVALUE, MaxApertureValue);              // ID:37381
		TIFFSetField(tiffout, EXIFTAG_METERINGMODE, MeteringMode);                      // ID:37383
		TIFFSetField(tiffout, EXIFTAG_FLASH, Flash);                                    // ID:37385
		TIFFSetField(tiffout, EXIFTAG_FOCALLENGTH, FocalLength);                        // ID:37386

		TIFFWriteCustomDirectory( tiffout, &ExifIFD );
		DBG("ExifIFD = %lld\n", ExifIFD);

		/* Back to IFD0 and update exif location */
		TIFFSetDirectory(tiffout, 0);
		TIFFSetField(tiffout, TIFFTAG_EXIFIFD, ExifIFD);                                // ID:34665
	#endif

		DBG("- Start to write scanlines ...\n");

		/* DNG image data */
		int linesize = pframe->width[l] * dng_pixel_depth;
		uint8_t *scanline = (uint8_t *)malloc(linesize);
		for (i = 0; i < pframe->height[l]; i++)
		{
	#if CONVERT_RAW_4_TO_2 // Hardcoded to convert 2byte raw to 4byte apical-raw
			uint32_t *src_pos = (uint32_t *)(buf + (i * pframe->bytes_per_line[l]));
	#else
			uint32_t *src_pos = (uint32_t *)(buf + (i * linesize * 2));
	#endif
			uint32_t *dst_pos = (uint32_t *)scanline;

			/* bit shiftto remove 0 padding on LSB from Apical RAW12 */
			for (j = 0; j < linesize / 4; j++) {
				dst_pos[j] = (src_pos[j] >> 4) & 0x0FFF0FFF;
			}
			TIFFWriteScanline(tiffout, scanline, i, 0);
		}
		free(scanline);

		/* Write DNG directory */
		TIFFWriteDirectory(tiffout);

		TIFFClose(tiffout);

		MSG("DNG capture for RAW output is done ...\n");

    }
#endif
}
