#include "noise_reduction.h"
#define LOG_TAG "NOISE_REDUCTION"
#include <utils/Log.h>
#include <stdlib.h>
#include <cutils/properties.h>

int noise_redution_init(PRM_NR *pPrm_Nr, int nImgW, int nImgH)
{
    char prop[PROPERTY_VALUE_MAX];
    int value = 0;
    memset(prop,0,sizeof(prop));
    if (property_get("hw.encoder.nr.enable", prop, NULL) < 0) {
        return -1;
    }

    sscanf(prop,"%d",&value);
    value = value>2? 2: value;
    pPrm_Nr->mode = value;
    ALOGD("%s, %d, noise value =%d\n", __func__, __LINE__, value);
    if (value == 0)
        return -1;

    // data of noise reduction output
    if (value == 2) {
        pPrm_Nr->nr_buf = (int16_t *)calloc(nImgW*nImgH, sizeof(int16_t));
        pPrm_Nr->pNrY = NULL;
    } else {
        pPrm_Nr->pNrY = (unsigned char *)calloc(nImgW*nImgH*1.5, sizeof(unsigned char));
        pPrm_Nr->nr_buf = NULL;
    }

    pPrm_Nr->prm_is_nv21   = 1; // 0: 420p, 1: nv21
    pPrm_Nr->prm_snr_en    = 1; // 0: disable snr, 1: enable snr
    pPrm_Nr->prm_snr_ymode = 1; // snr mode for Y, 0: disable, 1: gaussian, 2: bilateral
    pPrm_Nr->prm_snr_cmode = 0; // snr mode for UV, 0: disable, 1: gaussian, 2: bilateral
    pPrm_Nr->prm_ywin	  = 1; // window size for Y, 0: 3x3, 1: 5x5
    pPrm_Nr->prm_cwin      = 0; // window size for UV, 0: 3x3, 1: 5x5
    pPrm_Nr->prm_bld_yalp   = 128; // blend coefficent alpha for Y, higher with lower QP, 0~255
    pPrm_Nr->prm_bld_calp   = pPrm_Nr->prm_bld_yalp; // blend coefficent alpha for UV, higher with lower QP,, 0~255

    if (property_get("hw.encoder.nr.alp", prop, NULL) > 0) {
        value = 128;
        sscanf(prop,"%d",&value);
        pPrm_Nr->prm_bld_yalp   = value&0xff;
    }
    ALOGI("prm_bld_yalp=%d\n", pPrm_Nr->prm_bld_yalp );

    pPrm_Nr->width = nImgW;
    pPrm_Nr->height = nImgH;

    return 0;
}

void calculate_vacc_row0_c(unsigned char *pOrg, uint16_t *pVAcc, int nImgW, int nWinSz)
{
    int j = 0;
    for ( j = 0; j < nImgW; j++ ) {
        if (nWinSz == 0)
            pVAcc[j] = 2*pOrg[j] + pOrg[nImgW + j];
        else
            pVAcc[j] = 3*pOrg[j] + pOrg[nImgW + j] + pOrg[2*nImgW + j];
    }
}

void calculate_vacc_row_c(unsigned char *pOrg, uint16_t *pVAcc, int nImgW, int ip1, int im2)
{
    int j;
    for (j = 0; j < nImgW; j++) {
        pVAcc[j] = pVAcc[j] + pOrg[ip1*nImgW + j] - pOrg[im2*nImgW + j];
    }
}

#if 0
void calculate_vacc_row0(unsigned char *pOrg, uint16_t *pVAcc, int nImgW, int nWinSz)
{
    unsigned char *pOrg2 = pOrg + nImgW;
    unsigned char *pOrg3 = pOrg + nImgW;

    asm volatile(
        "mov r5, #0				\n"
        //"mov r8, #8				\n"
        //"add %[pOrg2], %[pOrg], %[nImgW]        \n"
        //"add %[pOrg3], %[pOrg2],%[nImgW]        \n"
        "1: @begin:					\n"
        "cmp r5, %[nImgW]			\n"
        "bge 3f @end				\n"
        "cmp %[nWinSz], #0				\n"
        "beq 2f @winsize3				\n"
        //pVAcc[j] = 3*pOrg[j] + pOrg[nImgW + j] + pOrg[2*nImgW + j];
        //"add r6, %[pOrg], r5				\n"
        //3pOrg[j]
        "vld1.8 {d0}, [%[pOrg]]!		\n"
        "vshll.u8 q1, d0, #1		 	\n"
        "vmovl.u8 q0, d0			\n"
        "vadd.u16 q0, q1, q0			\n"

        //"add r6, r6, %[nImgW]				\n"
        //pOrg[nImgW + j]
        "vld1.8 {d2}, [%[pOrg2]]!			\n"
        "vmovl.u8 q1, d2			\n"
        "vadd.u16 q0, q1, q0			\n"
        //"add r6, r6, %[nImgW]				\n"
        //pOrg[nImgW*2 +j]
        "vld1.8 {d4}, [%[pOrg3]]!		\n"
        "vmovl.u8 q2, d4			\n"
        "vadd.u16 q0, q2, q0			\n"
        //"add r7, %[pVAcc], r5				\n"
        // pVAcc[j]
        "vst1.16 {d0,d1},[%[pVAcc]]!			\n"
        "add r5, r5, #8				\n"
        "b 1b @begin				\n"

        "2: @winsize3: 				\n"
        //pVAcc[j] = 2*pOrg[j] + pOrg[nImgW + j];
        //2*pOrg[j]
        "vld1.8 {d0}, [%[pOrg]]!			\n"
        "vshll.u8 q0, d0, #1		 	\n"
        //pOrg[nImgW + j]
        "vld1.8 {d2}, [%[pOrg2]]!			\n"
        "vmovl.u8 q1, d2			\n"
        "vadd.u16 q1, q1, q0			\n"
        //"add r7, %[pVAcc], r5				\n"
        //pVAcc[j]
        "vst1.16 {d2,d3},[%[pVAcc]]!			\n"
        "add r5, r5, #8				\n"
        "b 1b @begin				\n"
        "3: @end:					\n"
        : [pOrg] "+r" (pOrg),   //%0
          [pVAcc] "+r" (pVAcc), //%1
          [nImgW] "+r" (nImgW), //%2
          [nWinSz] "+r" (nWinSz),//%3
          [pOrg2] "+r" (pOrg2), //%4
          [pOrg3] "+r" (pOrg3) //%5
        :
        : "cc", "memory", "r5", "q0", "q1", "q2"
    );
}

void calculate_vacc_row(unsigned char *pOrg, uint16_t *pVAcc, int nImgW, int coeff1, int coeff2)
{
    unsigned char *pOrg2 = pOrg + coeff2*nImgW;
    pOrg = pOrg + coeff1*nImgW;
    asm volatile(
        "mov r5, #0				\n"
        //"mul %[coeff2], %[coeff2], %[nImgW]     \n"
        //"add %[pOrg2],   %[pOrg], %[coeff1]        \n"

        //"mul %[coeff1], %[coeff1], %[nImgW]     \n"
        //"add %[pOrg], %[pOrg], %[coeff1]        \n"

        "1:					\n"
        "cmp r5, %[nImgW]				\n"
        "bge 	2f		\n"

        //"little:				\n"
        //"add r9, %[pVAcc], r5				\n"
        //pVAcc[j]
        "vld1.16 {d0,d1}, [%[pVAcc]]		        \n"
        //"add r6, %[pOrg], r5				\n"
        //"mla r7, %[nImgW], %[coeff1], r6			\n"
        //pOrg[ip1*nImgW + j]
        "vld1.8 {d2}, [%[pOrg]]!			\n"
        "vmovl.u8 q1, d2			\n"
        "vadd.u16 q0, q1, q0			\n"
        //"mla r8, %[nImgW], %[coeff2], r6			\n"
        "vld1.8 {d4}, [%[pOrg2]]!			\n"
        "vmovl.u8 q2, d4			\n"
        "vsub.s16 q0, q0, q2			\n"

        "vst1.16 {d0,d1},[%[pVAcc]]!			\n"
        "add r5, r5, #8			\n"
        "b  1b				\n"
        "2:				\n"
        : [pOrg] "+r" (pOrg),     //%0
          [pVAcc] "+r" (pVAcc),   //%1
          [nImgW] "+r" (nImgW),   //%2
          [coeff1] "+r" (coeff1), //%3
          [coeff2] "+r" (coeff2),  //%4
          [pOrg2] "+r" (pOrg2)   //%5
        :
        : "cc", "memory", "r5", "r6", "r7","r8", "r9", "q0", "q1", "q2"
    );
}
#endif

int fast_gau_filter(int *pGauOut,// output
                                 unsigned char *pOrg,   // input buf
                                 uint16_t *pVAcc,  // vertical accumulator
                                 uint16_t *pHAcc,  // horizontal accumulator
                                 int nCol,
                                 int nRow,
                                 int nWinSz,  // 0:3x3, 1:5x5
                                 int nIntMode,// 0:normal, 1: interlaced (for NV21)
                                 int nImgW,
                                 int nImgH,
                                 int mode)
{
    int i, j, k;
    int ip1, ip2, im1, im2, im3;
    int jp1, jp2, jm1, jm2, jm3;
    int ii, jj;
    int nHalfWin;
    int nGau;
    int nSum;
    int nNrm;
    int nWin;
    int nIsU;
    int nLft, nRgt;

    nNrm = nWinSz == 0 ? 9 : 25;

    // update accumulator each line at the first pixel
    if (nCol == 0 ) {
        if ( nRow == 0 ) {
            calculate_vacc_row0_c(pOrg, pVAcc, nImgW, nWinSz);
        } else {
            if (nWinSz == 0) {
                im2 = nRow-2 < 0 ? 0 : nRow-2;
                ip1 = nRow+1 > nImgH-1 ? nImgH-1 : nRow+1;
            }else{
                im2 = nRow-3 < 0 ? 0 : nRow-3;
                ip1 = nRow+2 > nImgH-1 ? nImgH-1 : nRow+2;
            }
            calculate_vacc_row_c(pOrg, pVAcc, nImgW, ip1, im2);
        }
    }

    // special at each row start
    if (nCol == 0 && nIntMode == 0) {
        if (nWinSz == 0)
            pHAcc[0] = 2*pVAcc[0] + pVAcc[1];
        else
            pHAcc[0] = 3*pVAcc[0] + pVAcc[1] + pVAcc[2];
        *pGauOut = pHAcc[0]/nNrm;
    } else if (nCol/2 == 0 && nIntMode == 1) {//0 or 1
        if (nWinSz == 0)
            pHAcc[nCol] = 2*pVAcc[nCol] + pVAcc[nCol+2];
        else
            pHAcc[nCol] = 3*pVAcc[nCol] + pVAcc[nCol+2] + pVAcc[nCol+4];
        *pGauOut = pHAcc[nCol]/nNrm;
    } else if (nIntMode == 0) {
        if (nWinSz == 0) {
            jm2 = nCol-2 < 0 ? 0 : nCol-2;
            jp1 = nCol+1 > nImgW-1 ? nImgW-1 : nCol+1;
            pHAcc[0] = pHAcc[0] + pVAcc[jp1] - pVAcc[jm2];
        } else {
            jm3 = nCol-3 < 0 ? 0 : nCol-3;
            jp2 = nCol+2 > nImgW-1 ? nImgW-1 : nCol+2;
            pHAcc[0] = pHAcc[0] + pVAcc[jp2] - pVAcc[jm3];
        }
        *pGauOut = pHAcc[0]/nNrm;
    } else if (nIntMode == 1) {//NV21
        nLft = nCol%2 == 0 ? 0 : 1;
        nRgt = nCol%2 == 0 ? nImgW-2 : nImgW-1;
        if (nWinSz == 0) {
            jm2 = nCol-4 < nLft ? nLft : nCol-4;
            jp1 = nCol+2 > nRgt ? nRgt : nCol+2;
            pHAcc[nCol%2] = pHAcc[nCol%2] + pVAcc[jp1] - pVAcc[jm2];
        } else {
            jm3 = nCol-6 < nLft ? nLft : nCol-6;
            jp2 = nCol+4 > nLft ? nLft : nCol+4;
            pHAcc[nCol%2] = pHAcc[nCol%2] + pVAcc[jp2] - pVAcc[jm3];
        }
        *pGauOut = pHAcc[nCol%2]/nNrm;
    }
    return 0;
}

// interface function
int noise_reduction(unsigned char *pCurY, // current frame Y input
					int nImgW,  // image width
					int nImgH,  // image height
					PRM_NR *pPrm_Nr) // NR parameters
{
    int i, j, k;
    int ii, jj;
    int nRow, nCol;
    unsigned char *pNrY = NULL;
    unsigned char *pNrU = NULL;
    unsigned char *pNrV = NULL;
    unsigned char *pNrUV = NULL;
    unsigned char *pCurU = NULL;
    unsigned char *pCurV = NULL;
    unsigned char *pCurUV = NULL;
    int nGau[3], TNR[3], Out[3];

    // horizontal accumulator
    uint16_t nHAccY, nHAccU, nHAccV, nHAccUV[2];

    // below is programmable setting parameters
    int prm_is_nv21   = pPrm_Nr->prm_is_nv21;
    int prm_snr_en    = pPrm_Nr->prm_snr_en;
    //int prm_tnr_en  = pPrm_Nr->prm_tnr_en;
    int prm_snr_ymode = pPrm_Nr->prm_snr_ymode;
    int prm_snr_cmode = pPrm_Nr->prm_snr_cmode;
    int prm_ywin      = pPrm_Nr->prm_ywin;
    int prm_cwin      = pPrm_Nr->prm_cwin;
    int prm_bld_yalp  = pPrm_Nr->prm_bld_yalp;
    int prm_bld_calp  = pPrm_Nr->prm_bld_calp;

    int offset = nImgW * nImgH;

    pNrY = pPrm_Nr->pNrY;
    if (prm_is_nv21 == 0) {
        pNrU = pNrY + offset;
        pNrV = pNrU + (offset>>2);
        pCurU = pCurY + offset;
        pCurV = pCurU + (offset>>2);
    } else {
        pNrUV = pNrY + offset;
        pCurUV = pCurY + offset;
    }
    // buf for vertical accumulator
    uint16_t *pVAccY   = (uint16_t *)calloc(nImgW, sizeof(uint16_t));
    uint16_t *pVAccU   = (uint16_t *)calloc(nImgW/2, sizeof(uint16_t));
    uint16_t *pVAccV   = (uint16_t *)calloc(nImgW/2, sizeof(uint16_t));
    uint16_t *pVAccUV  = (uint16_t *)calloc(nImgW, sizeof(uint16_t));

    prm_bld_yalp = prm_bld_yalp == 255 ? 256 : prm_bld_yalp;
    prm_bld_calp = prm_bld_calp == 255 ? 256 : prm_bld_calp;

    // main loop
    for (nRow = 0; nRow < nImgH; nRow++) {
        for ( nCol = 0; nCol < nImgW; nCol++ ) {
        // gaussian lpf, moving window based, SNR
            if (prm_snr_ymode == 0) {
                pNrY[nRow*nImgW + nCol] = pCurY[nRow*nImgW + nCol];
            } else if (prm_snr_ymode == 1) {
                fast_gau_filter(&nGau[0], pCurY, pVAccY, &nHAccY, nCol, nRow, prm_ywin, 0, nImgW, nImgH,pPrm_Nr->mode);
                pNrY[nRow*nImgW + nCol] = prm_snr_en == 0 ? pCurY[nRow*nImgW + nCol]
                                                             : (prm_bld_yalp * nGau[0] + (256 - prm_bld_yalp) * pCurY[nRow*nImgW + nCol] + 128) / 256;
            }
            if (prm_snr_cmode == 0) {
                if (prm_is_nv21 == 0) {// 420p format
                    if ( nRow%2 == 0 && nCol%2 == 0) {
                        pNrU[nRow/2*nImgW/2 + nCol/2] = pCurU[nRow/2*nImgW/2 + nCol/2];
                        pNrV[nRow/2*nImgW/2 + nCol/2] = pCurV[nRow/2*nImgW/2 + nCol/2];
                    }
                } else {// For NV21 input (UV in one buffer), call gaussian_filter(...) only once:
                    if ( nRow%2 == 0 )
                        pNrUV[nRow/2*nImgW + nCol] = pCurUV[nRow/2*nImgW + nCol];
                }
            } else if (prm_snr_cmode == 1) {
                if (prm_is_nv21 == 0) {// 420p format
                    if (nRow%2 == 0 && nCol%2 == 0) {
                        fast_gau_filter(&nGau[1], pCurU, pVAccU, &nHAccU, nCol/2, nRow/2, prm_cwin, 0, nImgW/2, nImgH/2,pPrm_Nr->mode);
                        pNrU[nRow/2*nImgW/2 + nCol/2] = prm_snr_en == 0 ? pCurU[nRow/2*nImgW/2 + nCol/2]
                                                                                : (prm_bld_calp * nGau[1] + (256 - prm_bld_calp) * pCurU[nRow/2*nImgW/2 + nCol/2] + 128) / 256;

                        fast_gau_filter(&nGau[2], pCurV, pVAccV, &nHAccV, nCol/2, nRow/2, prm_cwin, 0, nImgW/2, nImgH/2,pPrm_Nr->mode);
                        pNrV[nRow/2*nImgW/2 + nCol/2] = prm_snr_en == 0 ? pCurV[nRow/2*nImgW/2 + nCol/2]
                                                                                : (prm_bld_calp * nGau[2] + (256 - prm_bld_calp) * pCurV[nRow/2*nImgW/2 + nCol/2] + 128) / 256;
                    }
                } else {// For NV21 input (UV in one buffer), call gaussian_filter(...) only once:
                    if (nRow%2 == 0) {
                        fast_gau_filter(&nGau[1], pCurUV,  pVAccUV, nHAccUV, nCol, nRow/2, prm_cwin, 1, nImgW, nImgH/2,pPrm_Nr->mode);
                        pNrUV[nRow/2*nImgW + nCol] = prm_snr_en == 0 ? pCurUV[nRow/2*nImgW + nCol]
                                                                           : (prm_bld_calp * nGau[1] + (256 - prm_bld_calp) * pCurUV[nRow/2*nImgW + nCol] + 128) / 256;
                    }
                }
            }
        }//j
    }//i

    free(pVAccY);
    free(pVAccU);
    free(pVAccV);
    free(pVAccUV);
    return 0;
}

void noise_reduction_release(PRM_NR *pPrm_Nr)
{
    if (pPrm_Nr->pNrY) {
        free(pPrm_Nr->pNrY);
        pPrm_Nr->pNrY = NULL;
    }
    if (pPrm_Nr->nr_buf) {
        free(pPrm_Nr->nr_buf);
        pPrm_Nr->nr_buf = NULL;
    }
    pPrm_Nr->mode = -1;
    return;
}

inline void cal_vacc_win5x5_neon(uint8_t *pOrg, int16_t *pOut, PRM_NR *pPrm_Nr)
{
    uint8_t *psrc = pOrg;
    int16_t  *pdst = pOut;
    int32_t iw = 0, jh = 0;

    int32_t nImgW = pPrm_Nr->width;
    int32_t nImgH = pPrm_Nr->height;

    int32_t heightadd2 = nImgH + 2;
    asm volatile(
        "@initialize\n\t"
        "lsl r7, %[nImgW], #1			\n\t"
        "mov    %[iw], #0                       \n\t"
        "1: @vertical prepare:			\n\t"
        "@@@@@@@@@%[jh]=0                       \n\t"
        "@eor %[jh], %[jh]			\n\t"
        "mov %[jh],  #0		                \n\t"
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@//3*pOrg[j]\n\t"
        "vld1.8 {d0}, [%[pOrg]],%[nImgW]	\n\t"
        "add %[jh], %[jh], #1                   \n\t"
        "@@@@@@@@@@@@@@@@@//v(-2) = v(-1) = v(0)\n\t"
        "@@@@@@@@@@//q0-v(-2), q1-v(-1), q2-v(0)\n\t"
        "vshll.u8 q1, d0, #1                    \n\t"
        "vmovl.u8 q0, d0                        \n\t"
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@3*v(-2)  \n\t"
        "vadd.s16 q6, q1, q0                    \n\t"
        "@@@@@@@@@@@@@@@@@//v(-1) = v(0) = v(-2)\n\t"
        "vmov.u16 q1, q0			\n\t"
        "vmov.u16 q2, q0			\n\t"
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@v(1)  \n\t"
        "vld1.8 {d6}, [%[pOrg]],%[nImgW]	\n\t"
        "add %[jh], %[jh], #1                   \n\t"
        "vmovl.u8 q3, d6                        \n\t"
        "vadd.s16 q6, q6, q3                    \n\t"
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@v(2)  \n\t"
        "vld1.8 {d8}, [%[pOrg]],%[nImgW]	\n\t"
        "add %[jh], %[jh], #1                   \n\t"
        "vmovl.u8 q4, d8                        \n\t"
        "vadd.s16 q6, q6, q4                    \n\t"
        "vst1.16 {d12, d13},[%[pOut]], r7       \n\t"
        "2: @@@@@@@@@@load new line             \n\t"
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@v(3)  \n\t"
        "vld1.8 {d10}, [%[pOrg]],%[nImgW]	\n\t"
        "vmovl.u8 q5, d10                       \n\t"
        "3: @@@@@@begin                         \n\t"
        "vadd.s16 q6, q6, q5                    \n\t"
        "vsub.s16 q6, q6, q0                    \n\t"
        "vst1.16 {d12, d13},[%[pOut]], r7       \n\t"
        "vmov.u16 q0, q1                        \n\t"
        "vmov.u16 q1, q2                        \n\t"
        "vmov.u16 q2, q3                        \n\t"
        "vmov.u16 q3, q4                        \n\t"
        "vmov.u16 q4, q5                        \n\t"
        "@@@@@@if r5(h) < height-2 load new line\n\t"
        "add %[jh], %[jh], #1                   \n\t"
        "cmp %[jh], %[nImgH]               \n\t"
        "blt 2b                                 \n\t"
        "@(height -2) < r5 < height stop load   \n\t"
        "cmp %[jh], %[heightadd2]                    \n\t"
        "blt 3b                                 \n\t"
        "@horizontal prepare                    \n\t"
        "4: @					\n\t"
        "add %[iw], %[iw], #8                   \n\t"
        "add %[pOrg], %[psrc], %[iw]	        \n\t"
        "add %[pOut], %[pdst], %[iw], lsl #1    \n\t"
        "cmp %[iw], %[nImgW]                    \n\t"
        "blt 1b                                 \n\t"
        "5: @end                                \n\t"
        : [pOrg] "+r" (pOrg),   //%0
          [pOut] "+r" (pOut), //%1
          [nImgW] "+r" (nImgW), //%2
          [nImgH] "+r" (nImgH), //%3
          [psrc] "+r" (psrc), //%4
          [pdst] "+r" (pdst), //%4
          [iw] "+r" (iw), //%2
          [jh] "+r" (jh) //%3
        : [heightadd2] "r" (heightadd2) //%3
        : "cc", "memory",  "r7",
          "q0", "q1", "q2", "q3", "q4", "q5", "q6"
    );
}

inline void cal_hacc_win5x5_neon(int16_t *pVAcc, uint8_t *pOut, PRM_NR *pPrm_Nr)
{
    int16_t *psrc = pVAcc;
    uint8_t  *pdst = pOut;
    int32_t iw = 0, jh = 0;

    int16_t alp = pPrm_Nr->prm_bld_yalp;
    int32_t nImgW = pPrm_Nr->width;
    int32_t nImgH = pPrm_Nr->height;
    int32_t widthadd2 = nImgH + 2;

    asm volatile(
        "mov %[jh], #0                          \n\t"
        "1: @horizontal prepare                 \n\t"
        "mov %[iw], #0                          \n\t"
        "sub r5, %[nImgW], #10                  \n\t"
        "vld1.16 {d0[0]}, [%[pVAcc]]            \n\t"
        "vld1.16 {d0[1]}, [%[pVAcc]]            \n\t"
        "vld1.16 {d0[2]}, [%[pVAcc]]!           \n\t"
        "vld1.16 {d0[3]}, [%[pVAcc]]!           \n\t"
        "vld1.16 {d1}, [%[pVAcc]]!              \n\t"
        "add    %[iw], %[iw], #6                \n\t"
        "vld1.16 {d2}, [%[pVAcc]]!              \n\t"
        "vld1.16 {d3}, [%[pVAcc]]!              \n\t"

        "2:@begin                               \n\t"
        "add    %[iw], %[iw], #8                \n\t"
        "vext.s16 q2, q0, q1, #1                \n\t"
        "vext.s16 q3, q0, q1, #2                \n\t"
        "vext.s16 q4, q0, q1, #3                \n\t"
        "vext.s16 q5, q0, q1, #4                \n\t"
        "vadd.s16 q2, q0, q2                    \n\t"
        "vadd.s16 q2, q2, q3                    \n\t"
        "vadd.s16 q2, q2, q4                    \n\t"
        "vadd.s16 q2, q2, q5                    \n\t"
        "vmov.s16 q5, #13                       \n\t"
        "vadd.s16 q2, q2, q5                    \n\t"
        "vrshr.u16 q0, q2, #5                   \n\t"
        "vrsra.u16 q0, q2, #7                   \n\t"
        "vrsra.u16 q0, q2, #11                  \n\t"
        "vrsra.u16 q0, q2, #12                  \n\t"
        "vrsra.u16 q0, q2, #13                  \n\t"
        "vrsra.u16 q0, q2, #14                  \n\t"
        "vrsra.u16 q0, q2, #16                  \n\t"
        "vmovn.i16 d0, q0                       \n\t"
#if 1
        "vld1.8 {d4}, [%[pOut]]                \n\t"
        "vqsub.u8 d0, d0, d4                    \n\t"
        "vdup.u8  d1, %[alp]                   \n\t"
        "vmull.u8 q0, d0, d1                    \n\t"
        "vrshr.u16 q0, q0, #16                  \n\t"
        "vmovn.i16 d0, q0                       \n\t"
        "vqadd.u8  d0, d0, d4                   \n\t"
#endif
        "vst1.8 {d0}, [%[pOut]]!                \n\t"
        "3:@update                              \n\t"
        "vmov.s16  q0, q1                       \n\t"
        "vld1.16 {d2}, [%[pVAcc]]!              \n\t"
        "vld1.16 {d3}, [%[pVAcc]]!              \n\t"
        "cmp    %[iw], r5                       \n\t"
        "blt    2b                              \n\t"

        "add %[pVAcc], %[psrc], r5, lsl #1      \n\t"
        "add %[pOut],  %[pdst], r5              \n\t"
        "add %[pOut],  %[pOut], #2              \n\t"
        "vld1.16 {d0}, [%[pVAcc]]!              \n\t"
        "vld1.16 {d1}, [%[pVAcc]]!              \n\t"
        "vld1.16 {d2}, [%[pVAcc]]!              \n\t"
        "vmov.s16 r5, d2[1]                     \n\t"
        "orr r5, r5, r5, lsl #16                \n\t"
        "vmov.s32 d2[1], r5                     \n\t"
        "vext.s16 q2, q0, q1, #1                \n\t"
        "vext.s16 q3, q0, q1, #2                \n\t"
        "vext.s16 q4, q0, q1, #3                \n\t"
        "vext.s16 q5, q0, q1, #4                \n\t"
        "vadd.s16 q2, q0, q2                    \n\t"
        "vadd.s16 q2, q2, q3                    \n\t"
        "vadd.s16 q2, q2, q4                    \n\t"
        "vadd.s16 q2, q2, q5                    \n\t"
        "vmov.s16 q5, #13                       \n\t"
        "vadd.s16 q2, q2, q5                    \n\t"
        "vrshr.u16 q0, q2, #5                   \n\t"
        "vrsra.u16 q0, q2, #7                   \n\t"
        "vrsra.u16 q0, q2, #11                  \n\t"
        "vrsra.u16 q0, q2, #12                  \n\t"
        "vrsra.u16 q0, q2, #13                  \n\t"
        "vrsra.u16 q0, q2, #14                  \n\t"
        "vrsra.u16 q0, q2, #16                  \n\t"
        "vmovn.i16 d0, q0                       \n\t"
        "vst1.8 {d0}, [%[pOut]]!                \n\t"

        "4:@vetical update                      \n\t"
        "add %[psrc], %[psrc], %[nImgW], lsl #1 \n\t"
        "mov %[pVAcc], %[psrc]                  \n\t"
        "add %[pdst],  %[pdst], %[nImgW]        \n\t"
        "mov %[pOut], %[pdst]                   \n\t"
        "add %[jh], %[jh], #1                   \n\t"
        "cmp %[jh], %[nImgH]                    \n\t"
        "blt    1b                              \n\t"
        "5:@end                                 \n\t"
        : [pVAcc] "+r" (pVAcc),//%0
          [pOut] "+r" (pOut), //%1
          [nImgW] "+r" (nImgW), //%2
          [nImgH] "+r" (nImgH), //%3
          [psrc] "+r" (psrc), //%4
          [pdst] "+r" (pdst), //%4
          [alp] "+r" (alp), //%3
          [iw] "+r" (iw), //%2
          [jh] "+r" (jh) //%3
        : //[ghtadd2] "r" (heightadd2) //%3
                        : "cc", "memory", "r5",
          "q0", "q1", "q2", "q3", "q4", "q5"
    );
}

int noise_reduction_g2(unsigned char *pCurY, // current frame Y input
					int nImgW,  // image width
					int nImgH,  // image height
					PRM_NR *pPrm_Nr) // NR parameters
{
    cal_vacc_win5x5_neon(pCurY, pPrm_Nr->nr_buf, pPrm_Nr);
    cal_hacc_win5x5_neon(pPrm_Nr->nr_buf, pCurY, pPrm_Nr);
    return 0;
}
