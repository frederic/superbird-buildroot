//#define LOG_NDEBUG 0
#define LOG_TAG "GXVENC_PARSER"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif

#include "gxvenclib_fast.h"
#include "enc_define.h"
#include "parser.h"
//#include <cutils/properties.h>
#include <stdlib.h>
#include <math.h>

//For Intra4x4/Intra16x16/Inter16x16/Inter16x8/Inter8x16
#define get_mb_x(addr) *((unsigned char *)(addr+7))
#define get_mb_y(addr) *((unsigned char *)(addr+6))
#define get_mb_type(addr) *((unsigned char *)(addr+5))
#define get_mb_CPred(addr) *((unsigned char *)(addr+4))

#define SAD_INTERVAL_STATISTIC
#define FRAME_INFO_STATISTIC

#define get_mb_LPred_I4(addr, mode) \
    mode[0] = *((unsigned char *)(addr+12)) & 0xf; \
    mode[1] = (*((unsigned char *)(addr+12)) >> 4) & 0xf; \
    mode[2] = *((unsigned char *)(addr+13)) & 0xf; \
    mode[3] = (*((unsigned char *)(addr+13)) >> 4) & 0xf; \
    mode[4] = *((unsigned char *)(addr+14)) & 0xf; \
    mode[5] = (*((unsigned char *)(addr+14)) >> 4) & 0xf; \
    mode[6] = *((unsigned char *)(addr+15)) & 0xf; \
    mode[7] = (*((unsigned char *)(addr+15)) >> 4) & 0xf; \
    mode[8] = *((unsigned char *)(addr+0)) & 0xf; \
    mode[9] = (*((unsigned char *)(addr+0)) >> 4) & 0xf; \
    mode[10] = *((unsigned char *)(addr+1)) & 0xf; \
    mode[11] = (*((unsigned char *)(addr+1)) >> 4) & 0xf; \
    mode[12] = *((unsigned char *)(addr+2)) & 0xf; \
    mode[13] = (*((unsigned char *)(addr+2)) >> 4) & 0xf; \
    mode[14] = *((unsigned char *)(addr+3)) & 0xf; \
    mode[15] = (*((unsigned char *)(addr+3)) >> 4) & 0xf;

#define get_mb_mv_P16x16(addr, mv) \
    { \
        int k = 0; \
        while (k<16) { \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            k++; \
        } \
    }

#define get_mb_mv_P16x8(addr, mv) \
    { \
        int k = 0; \
        while (k<8) { \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            mv[k+8].mvx = *((short *)(addr+12)); \
            mv[k+8].mvy = *((short *)(addr+14)); \
            k++; \
        } \
    }

#define get_mb_mv_P8x16(addr, mv) \
    { \
        int k = 0; \
        while (k<4) { \
            mv[k].mvx = *((short *)(addr+0)); \
            mv[k].mvy = *((short *)(addr+2)); \
            mv[k+8].mvx = *((short *)(addr+0)); \
            mv[k+8].mvy = *((short *)(addr+2)); \
            mv[k+4].mvx = *((short *)(addr+12)); \
            mv[k+4].mvy = *((short *)(addr+14)); \
            mv[k+12].mvx = *((short *)(addr+12)); \
            mv[k+12].mvy = *((short *)(addr+14)); \
            k++; \
        } \
    }

#define get_mb_LPred_I16(addr) *((unsigned char *)(addr+12)) & 0xf
#define get_mb_quant(addr) *((unsigned char *)(addr+11))
#define get_mb_cbp(addr) *((unsigned char *)(addr+10))
#define get_mb_IntraSAD(addr) *((unsigned short *)(addr+8))
#define get_mb_InterSAD(addr) *((unsigned short *)(addr+22))
#define get_mb_bits(addr) *((unsigned short *)(addr+16))

#define get_mb_quant_ex(addr) *((unsigned char *)(addr+67))
#define get_mb_cbp_ex(addr) *((unsigned char *)(addr+66))
#define get_mb_IntraSAD_ex(addr) *((unsigned short *)(addr+64))
#define get_mb_InterSAD_ex(addr) *((unsigned short *)(addr+78))
#define get_mb_bits_ex(addr) *((unsigned short *)(addr+72))

#define get_mb_mv_P8x8(addr, mv) \
    { \
        int k = 0, j, offset; \
        while (k<8) { \
            j = k << 1; \
            offset = k << 3; \
            mv[j].mvx = *((short *)(addr+0+offset)); \
            mv[j].mvy = *((short *)(addr+2+offset)); \
            mv[j+1].mvx = *((short *)(addr+12+offset)); \
            mv[j+1].mvy = *((short *)(addr+14+offset)); \
            k++; \
        } \
    }

#define QP_TABLE_INDEX_SHIFT (1024 / QP_TAB_NUM)

const double QP2QSTEP[52] = {
        0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125,
        1.25, 1.375, 1.625, 1.75, 2, 2.25,
        2.5, 2.75, 3.25, 3.5, 4, 4.5,
        5, 5.5, 6.5, 7, 8, 9,
        10, 11, 13, 14, 16, 18,
        20, 22, 26, 28, 32, 36,
        40, 44, 52, 56, 64, 72,
        80, 88, 104, 112, 128,
        144, 160, 176, 208, 224 }; //{ 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

#define QP2Qstep(qp) QP2QSTEP[qp]

static int Qstep2QP(double Qstep) {
    int q_per = 0, q_rem = 0;

    if (Qstep < 0.625)
        return 0;
    else if ((uint32_t) Qstep > 224)
        return 51;

    while (Qstep > 1.125) {
        Qstep /= 2;
        q_per += 1;
    }

    if (Qstep <= (0.625 + 0.6875) / 2) {
        q_rem = 0;
    } else if (Qstep <= (0.6875 + 0.8125) / 2) {
        q_rem = 1;
    } else if (Qstep <= (0.8125 + 0.875) / 2) {
        q_rem = 2;
    } else if (Qstep <= (0.875 + 1.0) / 2) {
        q_rem = 3;
    } else if (Qstep <= (1.0 + 1.125) / 2) {
        q_rem = 4;
    } else {
        q_rem = 5;
    }
    return (q_per * 6 + q_rem);
}

double q_step_adjust(double q_step_aj) {
    if (q_step_aj < 0) {
        return q_step_aj;
    } else {
        if (q_step_aj > 80)
            return 0.1 * q_step_aj;
        else if (q_step_aj > 40)
            return 0.15 * q_step_aj;
        else if (q_step_aj > 20)
            return 0.2 * q_step_aj;
        else if (q_step_aj > 10)
            return 0.25 * q_step_aj;
        else if (q_step_aj > 5)
            return 0.3 * q_step_aj;
        else
            return 0.4 * q_step_aj;
    }
}

#define I_QP_ADJ_RANGE 4
#define P_QP_ADJ_RANGE 4

void adjust_qp_table(gx_fast_enc_drv_t* p)
{
    double q_step_aj;
    uint8_t cal_qp = p->quant;
    uint8_t i4_pre_qp = p->quant;
    uint8_t i16_pre_qp = p->quant;
    uint8_t p16_pre_qp = p->quant;
    uint32_t bits_per_mb = p->target / p->src.mb_width / p->src.mb_height;

    const double p_q_step_cur = QP2Qstep(p->quant);
    const double i4_q_step_cur = QP2Qstep(p->quant);
    const double i16_q_step_cur = QP2Qstep(p->quant);
    if (bits_per_mb == 0)
        bits_per_mb = 1;
    for (int i = 0; i < QP_TAB_NUM; i++) {
        if (p->qp_stic.p16_count[i] != 0) {
            p->qp_stic.p16_avr_bits[i] = p->qp_stic.p16_bits[i] / p->qp_stic.p16_count[i];
            if (p->re_encode == false) {
                q_step_aj = ((double) (p->qp_stic.p16_avr_bits[i] + 7) / bits_per_mb - 1) * p_q_step_cur;
                cal_qp = Qstep2QP(p_q_step_cur + q_step_adjust(q_step_aj));
                if ((cal_qp > p->quant + 4) && (p->quant > 28))
                    cal_qp = p->quant + 4;
                else if (cal_qp < p->quant - 4)
                    cal_qp = p->quant - 4;
                p->qp_stic.qp_table.p16_qp[i] = cal_qp;
                p16_pre_qp = p->qp_stic.qp_table.p16_qp[i];
                LOGAPI("p16 i:%d : aj:%lf avr_bits:%d count:%d lstqs:%lf",
                        i,
                        q_step_aj,
                        p->qp_stic.p16_avr_bits[i],
                        p->qp_stic.p16_count[i],
                        p_q_step_cur + q_step_adjust(q_step_aj));
            } else {
                LOGAPI("p16 i:%d avr_bits:\t%d \tcount:\t%d", i, p->qp_stic.p16_avr_bits[i], p->qp_stic.p16_count[i]);
            }
        } else {
            p->qp_stic.qp_table.p16_qp[i] = p16_pre_qp;
        }

        if (p->qp_stic.i4_count[i] != 0) {
            p->qp_stic.i4_avr_bits[i] = p->qp_stic.i4_bits[i] / p->qp_stic.i4_count[i];
            if (p->re_encode == false) {
                if (p->IDRframe) {
                    q_step_aj = ((double) (p->qp_stic.i4_avr_bits[i] + 40) / (bits_per_mb * 4) - 1) * i4_q_step_cur;
                    cal_qp = Qstep2QP(i4_q_step_cur + q_step_adjust(q_step_aj));
                } else {
                    q_step_aj = ((double) (p->qp_stic.i4_avr_bits[i] + 40) / (bits_per_mb) - 1) * i4_q_step_cur;
                    cal_qp = Qstep2QP(i4_q_step_cur + q_step_adjust(q_step_aj /** (1 +  0.3 * p->qp_stic.i_count / (p->src.mb_width * p->src.mb_height))*/));
                }
                if ((cal_qp > p->quant + 4) && (p->quant > 28))
                    cal_qp = p->quant + 4;
                else if (cal_qp < p->quant - 4)
                    cal_qp = p->quant - 4;
                p->qp_stic.qp_table.i4_qp[i] = cal_qp;
                i4_pre_qp = p->qp_stic.qp_table.i4_qp[i];
                LOGAPI("i4 i:%d aj:%lf avr_bits:%d count:%d lstqs:%lf",
                        i,
                        q_step_aj,
                        p->qp_stic.i4_avr_bits[i],
                        p->qp_stic.i4_count[i],
                        i4_q_step_cur + q_step_adjust(q_step_aj /* * (1 +  0.3 * p->qp_stic.i_count / (p->src.mb_width * p->src.mb_height)) */));
            } else {
                LOGAPI("i4 i:%d avr_bits:\t%d \tcount:\t%d", i, p->qp_stic.i4_avr_bits[i], p->qp_stic.i4_count[i]);
            }
        } else {
            p->qp_stic.qp_table.i4_qp[i] = i4_pre_qp;
        }
        if (p->qp_stic.i16_count[i] != 0) {
            p->qp_stic.i16_avr_bits[i] = p->qp_stic.i16_bits[i] / p->qp_stic.i16_count[i];
            if (p->re_encode == false) {
                q_step_aj = ((double) (p->qp_stic.i16_avr_bits[i] + 7) / bits_per_mb - 1) * i16_q_step_cur;
                if (p->IDRframe)
                    cal_qp = Qstep2QP(i16_q_step_cur + q_step_adjust(q_step_aj));
                else
                    cal_qp = Qstep2QP(i16_q_step_cur + q_step_adjust(q_step_aj /** (1 + 0.3 * p->qp_stic.i_count / (p->src.mb_width * p->src.mb_height))*/));

                if (cal_qp > p->quant + 4 && (p->quant > 28))
                    cal_qp = p->quant + 4;
                else if (cal_qp < p->quant - 4)
                    cal_qp = p->quant - 4;
                p->qp_stic.qp_table.i16_qp[i] = cal_qp;
                i16_pre_qp = p->qp_stic.qp_table.i16_qp[i];
                LOGAPI("i16 i:%d aj:%lf avr_bits:%d count:%d lstqs:%lf",
                        i,
                        q_step_aj,
                        p->qp_stic.i16_avr_bits[i],
                        p->qp_stic.i16_count[i],
                        i16_q_step_cur + q_step_adjust(q_step_aj /* * (1 +  0.3 * p->qp_stic.i_count / (p->src.mb_width * p->src.mb_height))*/));
            } else {
                LOGAPI("i16 i:%d :avr_bits:\t%d \tcount:\t%d", i, p->qp_stic.i16_avr_bits[i], p->qp_stic.i16_count[i]);
            }
        } else {
            p->qp_stic.qp_table.i16_qp[i] = i16_pre_qp;
        }
    }

#ifdef DEBUG_QP_TABLE
    char str[3][1000];
    memset(str, 0, sizeof(str));
    for (int i = 0; i < QP_TAB_NUM; i++) {
        char tmp_str[20];
        sprintf(tmp_str, "%d |", p->qp_stic.qp_table.i4_qp[i]/*, p->qp_stic.i4_bits[i],
                 p->qp_stic.i4_avr_bits[i], p->qp_stic.i4_count[i]*/);
        strcat(str[0], tmp_str);

        sprintf(tmp_str, "%d |", p->qp_stic.qp_table.i16_qp[i]/*, p->qp_stic.i16_bits[i],
                 p->qp_stic.i16_avr_bits[i], p->qp_stic.i16_count[i]*/);
        strcat(str[1], tmp_str);

        sprintf(tmp_str, "%d |", p->qp_stic.qp_table.p16_qp[i]/*, p->qp_stic.p16_bits[i],
                 p->qp_stic.p16_avr_bits[i], p->qp_stic.p16_count[i]*/);
        strcat(str[2], tmp_str);
    }
    LOGAPI("i4:%s\n", str[0]);
    LOGAPI("i16:%s\n", str[1]);
    LOGAPI("p16:%s\n", str[2]);
    LOGAPI("bits_per_mb:%d", bits_per_mb);
#endif
}

int Parser_DumpInfo(gx_fast_enc_drv_t* p) {
    int x, y;
    mb_t* info = p->mb_info;
    unsigned char* cur_mb = p->dump_buf.addr;
    unsigned char* next_mb = cur_mb;
    uint8_t index = 0;

    if (p->cbr_hw)
        memset(p->block_sad_size, 0,sizeof(p->block_sad_size));
    memset(&p->qp_stic, 0, sizeof(p->qp_stic));

    if (p->re_encode == false && !p->cbr_hw) {
        memset(&p->qp_stic.qp_table.i4_qp, p->quant, sizeof(p->qp_stic.qp_table.i4_qp));
        memset(&p->qp_stic.qp_table.i16_qp, p->quant, sizeof(p->qp_stic.qp_table.i16_qp));
        memset(&p->qp_stic.qp_table.p16_qp, p->quant, sizeof(p->qp_stic.qp_table.p16_qp));

#ifdef DEBUG_FIX_QP_TABLE
        for (int i = 0; i < 8; i++) {
            p->qp_stic.qp_table.i4_qp[i * 4] = 21 + i;
            p->qp_stic.qp_table.i4_qp[i * 4 + 1] = 21 + i;
            p->qp_stic.qp_table.i4_qp[i * 4 + 2] = 21 + i;
            p->qp_stic.qp_table.i4_qp[i * 4 + 3] = 21 + i;

            p->qp_stic.qp_table.i16_qp[i * 4] = 21 + i;
            p->qp_stic.qp_table.i16_qp[i * 4 + 1] = 21 + i;
            p->qp_stic.qp_table.i16_qp[i * 4 + 2] = 21 + i;
            p->qp_stic.qp_table.i16_qp[i * 4 + 3] = 21 + i;

            p->qp_stic.qp_table.p16_qp[i * 4] = 21 + i;
            p->qp_stic.qp_table.p16_qp[i * 4 + 1] = 21 + i;
            p->qp_stic.qp_table.p16_qp[i * 4 + 2] = 21 + i;
            p->qp_stic.qp_table.p16_qp[i * 4 + 3] = 21 + i;
        }
#endif
    }

#ifdef SAD_INTERVAL_STATISTIC
    int i_4_count = 0;
    int i_4_count_0 = 0;
    int i_4_count_128 = 0;
    int i_4_count_256 = 0;
    int i_4_count_512 = 0;
    int i_4_count_1024 = 0;
    int i_4_count_2048 = 0;

    int i_16_count =0;
    int i_16_count_0 =0;
    int i_16_count_128 = 0;
    int i_16_count_256 = 0;
    int i_16_count_512 = 0;
    int i_16_count_1024 = 0;
    int i_16_count_2048 = 0;

#ifdef FRAME_INFO_STATISTIC
    int qp_static = 0;
    int qp_avg = 0;
    int qp_min = 51;
    int qp_max = 0;
    int qp_sum = 0;
    int num_p_skip = 0;
    int bias = p->bias;
    memset(p->frame_stat_info->qp_histogram, 0, sizeof(int16_t) * 52);
    memset(&(p->frame_stat_info->mv_histogram), 0, sizeof(mv_hist_frame_t));
#endif

    int count = 0;
    int count_0 = 0;
    int count_128 = 0;
    int count_256 = 0;
    int count_512 = 0;
    int count_1024 = 0;
    int count_2048 = 0;
    static int frame_count;
    int value = 3;
    if (value > 20 || value < 0) {
        LOGAPI("error hw.encoder.mb.dump: value: %d", value);
    }
    frame_count++;
#endif

    for (y = 0; y < p->src.mb_height; y++) {
#ifdef SAD_INTERVAL_STATISTIC
        //if (value == frame_count) {
            //LOGAPI("-----------line-----------:%d\n", y);
        //}
#endif
        for (x = 0; x < p->src.mb_width; x++) {
#ifdef SAD_INTERVAL_STATISTIC
            char *str_line = (char *) malloc (1000);
            if (str_line == NULL) {
                LOGAPI("malloc buffer error");
                return -1;
            }
            memset(str_line, 0, 1000);
#endif
            cur_mb = next_mb;
            info->mbx = get_mb_x(cur_mb);
            info->mby = get_mb_y(cur_mb);
            if (((uint32_t)x != info->mbx) || ((uint32_t)y != info->mby)) {
                LOGAPI("parser mb poistion error: actual: %dx%d, info:%dx%d", x, y, info->mbx, info->mby);
                return -1;
            }
            info->mb_type = get_mb_type(cur_mb);
            if ((info->mb_type == HENC_MB_Type_I4MB) ||
                (info->mb_type == HENC_MB_Type_I16MB)) {
                info->intra.CPred = get_mb_CPred(cur_mb);
                if (info->mb_type == HENC_MB_Type_I16MB) {
                    info->intra.LPred[0] = get_mb_LPred_I16(cur_mb);
                } else {
                    get_mb_LPred_I4(cur_mb, info->intra.LPred);
                }
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant = get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                if (info->mb_type == HENC_MB_Type_I16MB) {
                    info->final_sad = info->intra.sad - p->i16_weight;
                    index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                    if (index >= QP_TAB_NUM)
                        index = QP_TAB_NUM - 1;
                    p->qp_stic.i16_count[index]++;
                    p->qp_stic.i_count++;
                    p->qp_stic.i16_bits[index] += info->bits;

                    if (info->final_sad < p->qp_stic.i16_min[index])
                        p->qp_stic.i16_min[index] = info->final_sad;
                    else if (info->final_sad > p->qp_stic.i16_max[index])
                        p->qp_stic.i16_max[index] = info->final_sad;
                } else {
                    info->final_sad = info->intra.sad - p->i4_weight;
                    index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                    if (index >= QP_TAB_NUM)
                        index = QP_TAB_NUM - 1;
                    p->qp_stic.i4_count[index]++;
                    p->qp_stic.i_count++;
                    p->qp_stic.i4_bits[index] += info->bits;
                    if (info->final_sad < p->qp_stic.i4_min[index])
                        p->qp_stic.i4_min[index] = info->final_sad;
                    else if (info->final_sad > p->qp_stic.i4_max[index])
                        p->qp_stic.i4_max[index] = info->final_sad;
                }
            } else if ((info->mb_type == HENC_MB_Type_P16x16)
                    || (info->mb_type == HENC_MB_Type_P16x8)
                    || (info->mb_type == HENC_MB_Type_P8x16)
                    || (info->mb_type == HENC_MB_Type_PSKIP)) {
                if ((info->mb_type == HENC_MB_Type_P16x16) || (info->mb_type == HENC_MB_Type_PSKIP)) {
                    get_mb_mv_P16x16(cur_mb, info->inter.mv);
                } else if (info->mb_type == HENC_MB_Type_P16x8) {
                    get_mb_mv_P16x8(cur_mb, info->inter.mv);
                } else {
                    get_mb_mv_P8x16(cur_mb, info->inter.mv);
                }
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant = get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                /* pskip is 0 */
                if (info->inter.sad != 0)
                    info->final_sad = info->inter.sad - p->me_weight;

                index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                if (index >= QP_TAB_NUM)
                    index = QP_TAB_NUM - 1;
                p->qp_stic.p16_count[index]++;
                p->qp_stic.p16_bits[index] += info->bits;

                // store mb coordinate
                p->frame_stat_info->mv_perblock[y * p->src.mb_width + x].mb_coord.mb_x =
                    x;
                p->frame_stat_info->mv_perblock[y * p->src.mb_width + x].mb_coord.mb_y =
                    y;
                // get mv hist
                if (info->mb_type == HENC_MB_Type_P16x16) {
                  (p->frame_stat_info->mv_histogram
                       .mv_x_hist[info->inter.mv[0].mvx + bias])++;
                  (p->frame_stat_info->mv_histogram
                       .mv_y_hist[info->inter.mv[0].mvy + bias])++;
                } else {
                  // 16x8 and 8x16 should statistics two different sub-marcoblock
                  (p->frame_stat_info->mv_histogram
                       .mv_x_hist[info->inter.mv[0].mvx + bias])++;
                  (p->frame_stat_info->mv_histogram
                       .mv_y_hist[info->inter.mv[0].mvy + bias])++;
                  (p->frame_stat_info->mv_histogram
                       .mv_x_hist[info->inter.mv[15].mvx + bias])++;
                  (p->frame_stat_info->mv_histogram
                       .mv_y_hist[info->inter.mv[15].mvy + bias])++;
                }
                for (int i = 0; i < 16; i++) {
                    // store mv per sub-mb(4x4)
                    p->frame_stat_info->mv_perblock[y * p->src.mb_width + x].mv[i].mvx =
                        info->inter.mv[i].mvx;
                    p->frame_stat_info->mv_perblock[y * p->src.mb_width + x].mv[i].mvy =
                        info->inter.mv[i].mvy;
                    if (info->inter.mv[i].mvx > 0)
                        p->qp_stic.abs_mv_sum += info->inter.mv[i].mvx;
                    else
                        p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvx;

                    if (info->inter.mv[i].mvy > 0)
                        p->qp_stic.abs_mv_sum += info->inter.mv[i].mvy;
                    else
                        p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvy;
                }

                if (info->final_sad < p->qp_stic.p16_min[index])
                    p->qp_stic.p16_min[index] = info->final_sad;
                else if (info->final_sad > p->qp_stic.p16_max[index])
                    p->qp_stic.p16_max[index] = info->final_sad;
            } else if (info->mb_type == HENC_MB_Type_P8x8) {
               (p->frame_stat_info->mv_histogram
                    .mv_x_hist[info->inter.mv[0].mvx + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_y_hist[info->inter.mv[0].mvy + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_x_hist[info->inter.mv[3].mvx + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_y_hist[info->inter.mv[3].mvy + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_x_hist[info->inter.mv[12].mvx + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_y_hist[info->inter.mv[12].mvy + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_x_hist[info->inter.mv[15].mvx + bias])++;
               (p->frame_stat_info->mv_histogram
                    .mv_y_hist[info->inter.mv[15].mvy + bias])++;

                get_mb_mv_P8x8(cur_mb, info->inter.mv);
                info->intra.sad = get_mb_IntraSAD_ex(cur_mb);
                info->inter.sad = get_mb_InterSAD_ex(cur_mb);
                info->quant = get_mb_quant_ex(cur_mb);
                info->cbp = get_mb_cbp_ex(cur_mb);
                info->bits = get_mb_bits_ex(cur_mb);
                next_mb = cur_mb + 80;
                info->final_sad = info->inter.sad - p->me_weight;

                for (int i = 0; i < 16; i++) {
                    if (info->inter.mv[i].mvx > 0)
                        p->qp_stic.abs_mv_sum += info->inter.mv[i].mvx;
                    else
                        p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvx;

                    if (info->inter.mv[i].mvy > 0)
                        p->qp_stic.abs_mv_sum += info->inter.mv[i].mvy;
                    else
                        p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvy;
                }

                index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                if (index >= QP_TAB_NUM)
                    index = QP_TAB_NUM - 1;
                p->qp_stic.p16_count[index]++;
                p->qp_stic.p16_bits[index] += info->bits;
            } else if (info->mb_type == HENC_MB_Type_AUTO) { // work around
                info->intra.sad = get_mb_IntraSAD(cur_mb);
                info->inter.sad = get_mb_InterSAD(cur_mb);
                info->quant = get_mb_quant(cur_mb);
                info->cbp = get_mb_cbp(cur_mb);
                info->bits = get_mb_bits(cur_mb);
                next_mb = cur_mb + 24;
                if (info->inter.sad < info->intra.sad) {
                    get_mb_mv_P16x16(cur_mb, info->inter.mv);
                    info->final_sad = info->inter.sad - p->me_weight;
                    LOGAPI("frame:%d, parser mb (%dx%d) type %d warning--set as inter 16x16, cur_mb:0x%x, inter sad:%d, intra sad:%d",
                            p->total_encode_frame + 1,
                            x,
                            y,
                            info->mb_type,
                            (ulong) (cur_mb - p->dump_buf.addr),
                            info->inter.sad,
                            info->intra.sad);
                    info->mb_type = HENC_MB_Type_P16x16;

                    index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                    if (index >= QP_TAB_NUM)
                        index = QP_TAB_NUM - 1;
                    p->qp_stic.p16_count[index]++;
                    p->qp_stic.p16_bits[index] += info->bits;

                    for (int i = 0; i < 16; i++) {
                        if (info->inter.mv[i].mvx > 0)
                            p->qp_stic.abs_mv_sum += info->inter.mv[i].mvx;
                        else
                            p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvx;

                        if (info->inter.mv[i].mvy > 0)
                            p->qp_stic.abs_mv_sum += info->inter.mv[i].mvy;
                        else
                            p->qp_stic.abs_mv_sum -= info->inter.mv[i].mvy;
                    }
                } else {
                    info->intra.CPred = get_mb_CPred(cur_mb);
                    info->intra.LPred[0] = get_mb_LPred_I16(cur_mb);
                    info->final_sad = info->intra.sad - p->i16_weight;
                    LOGAPI("frame:%d, parser mb (%dx%d) type %d warning--set as I16(mode %d), cur_mb:0x%x, inter sad:%d, intra sad:%d",
                            p->total_encode_frame + 1,
                            x,
                            y,
                            info->mb_type,
                            info->intra.LPred[0],
                            (ulong) (cur_mb - p->dump_buf.addr),
                            info->inter.sad,
                            info->intra.sad);
                    info->mb_type == HENC_MB_Type_I16MB;
                    index = info->final_sad / QP_TABLE_INDEX_SHIFT;
                    if (index >= QP_TAB_NUM)
                        index = QP_TAB_NUM - 1;
                    p->qp_stic.i16_count[index]++;
                    p->qp_stic.i_count++;
                    p->qp_stic.i16_bits[index] += info->bits;
                }
            } else {
                LOGAPI("parser mb (%dx%d) type %d error, cur_mb:0x%x", x, y, info->mb_type, (ulong) (cur_mb - p->dump_buf.addr));
                return -1;
            }
            if (info->final_sad < 0)
                info->final_sad = 0;
            p->qp_stic.f_sad_count += info->final_sad;

            if (p->cbr_hw)
                p->block_sad_size[y / p->block_height * p->block_width_n + x / p->block_width] += info->final_sad;

#ifdef SAD_INTERVAL_STATISTIC
            if ((info->mb_type == HENC_MB_Type_P16x16) ||
                (info->mb_type == HENC_MB_Type_P16x8) ||
                (info->mb_type == HENC_MB_Type_P8x16) ||
                (info->mb_type == HENC_MB_Type_P8x8) ||
                (info->mb_type == HENC_MB_Type_PSKIP)) {
                      if (info->final_sad > 2048) {
                          count_2048++;
                      } else if (info->final_sad > 1024) {
                          count_1024++;
                      } else if (info->final_sad > 512) {
                          count_512++;
                      } else if (info->final_sad > 256) {
                          count_256++;
                      } else if (info->final_sad > 128) {
                          count_128++;
                      } else {
                          count_0++;
                      }
                      count++;
                  } else if(info->mb_type == HENC_MB_Type_I4MB) {
                      if (info->final_sad > 2048) {
                          i_4_count_2048++;
                      } else if (info->final_sad > 1024) {
                          i_4_count_1024++;
                      } else if (info->final_sad > 512) {
                          i_4_count_512++;
                      } else if (info->final_sad > 256) {
                          i_4_count_256++;
                      } else if (info->final_sad > 128) {
                          i_4_count_128++;
                      } else {
                          i_4_count_0++;
                      }
                      i_4_count++;
                  } else if(info->mb_type == HENC_MB_Type_I16MB) {
                      if (info->final_sad > 2048) {
                          i_16_count_2048++;
                      } else if (info->final_sad > 1024) {
                          i_16_count_1024++;
                      } else if (info->final_sad > 512) {
                          i_16_count_512++;
                      } else if (info->final_sad > 256) {
                          i_16_count_256++;
                      } else if (info->final_sad > 128) {
                          i_16_count_128++;
                      } else {
                          i_16_count_0++;
                      }
                      i_16_count++;
            } else {
              LOGAPI("uncounted block type %X\n", info->mb_type);
            }

#ifdef FRAME_INFO_STATISTIC
            if (info->bits == 0 && ((info->mb_type == HENC_MB_Type_P16x16) ||
                                    (info->mb_type == HENC_MB_Type_P16x8) ||
                                    (info->mb_type == HENC_MB_Type_P8x16) ||
                                    (info->mb_type == HENC_MB_Type_P8x8) ||
                                    (info->mb_type == HENC_MB_Type_PSKIP))) {
              num_p_skip++;
            } else if (!(info->quant == 0 && x == 0 && y == 0)) {
              // Sometimes first P block reports quant = 0, it is wrong, skip it.
              p->frame_stat_info->qp_histogram[info->quant]++;
            }
#endif
            /*if (info->bits > 4 * p->target / p->src.mb_width / p->src.mb_height) {
                LOGAPI("frame_count%d y:%d x:%d", frame_count, y, x);
                char str[10];
                memset(str, 0, sizeof(str));
                strcat(str_line, "t:");
                sprintf(str, "%d", info->mb_type);
                strcat(str_line, str);
                strcat(str_line, "ad:");
                sprintf(str, "%d", info->intra.sad);
                strcat(str_line, str);
                strcat(str_line, "rd:");
                sprintf(str, "%d", info->inter.sad);
                strcat(str_line, str);
                strcat(str_line, "qp:");
                sprintf(str, "%d", info->quant);
                strcat(str_line, str);
                strcat(str_line, "cbp:");
                sprintf(str, "%d", info->cbp);
                strcat(str_line, str);
                strcat(str_line, "bits:");
                sprintf(str, "%d", info->bits);
                strcat(str_line, str);
                strcat(str_line, "fd:");
                sprintf(str, "%d\n", info->final_sad);
                strcat(str_line, str);
                LOGAPI("%s", str_line);
            }*/

            if (str_line != NULL) {
                free(str_line);
            }
#endif
        }
#ifdef SAD_INTERVAL_STATISTIC
        //if (value == frame_count) {
            //LOGAPI("*******");
        //}
#endif
    }
    if (p->qp_stic.f_sad_count == 0)
        p->qp_stic.f_sad_count = 1;
    if (!p->cbr_hw)
        adjust_qp_table(p);

#ifdef FRAME_INFO_STATISTIC
  // statistic frame info
  p->frame_stat_info->num_intra_prediction_blocks = i_4_count + i_16_count;
  p->frame_stat_info->num_inter_prediction_blocks = count;
  p->frame_stat_info->num_total_macro_blocks = i_4_count + i_16_count + count;
  p->frame_stat_info->num_p_skip_blocks = num_p_skip;
  p->frame_stat_info->qp_avg = 0;
  p->frame_stat_info->qp_min = 0;
  p->frame_stat_info->qp_max = 0;
  int j = 0;
  for (int i = 0; i < 52; i++) {
    qp_static = p->frame_stat_info->qp_histogram[i];
    if (qp_static > 0) {
      j += qp_static;
      qp_sum += qp_static * i;

      if (i < qp_min)
        qp_min = i;
      if (i > qp_max)
        qp_max = i;
    }
  }
  p->frame_stat_info->qp_min = qp_min;
  p->frame_stat_info->qp_max = qp_max;
  if (j > 0)
    p->frame_stat_info->qp_avg = qp_sum / j;

  LOGAPI("num_p_skip_blocks %d, skip block ratio %f, qp avg %d, qp min %d, qp max %d",
  num_p_skip, (float)(num_p_skip)/p->frame_stat_info->num_total_macro_blocks,
  p->frame_stat_info->qp_avg, p->frame_stat_info->qp_min,
  p->frame_stat_info->qp_max);

#endif
#ifdef SAD_INTERVAL_STATISTIC
    //LOGAPI("p mb: count:\t%d \t0:\t%d \t128:\t%d \t256:\t%d \t512:\t%d \t1024:\t%d \t2048:\t%d",
            //count, count_0, count_128, count_256, count_512, count_1024, count_2048);
    //LOGAPI("i4 mb: count:\t%d \t0:\t%d \t128:\t%d \t256:\t%d \t512:\t%d \t1024:\t%d \t2048:\t%d",
            //i_4_count, i_4_count_0, i_4_count_128, i_4_count_256, i_4_count_512, i_4_count_1024, i_4_count_2048);
    //LOGAPI("i16 mb: count:\t%d \t0:\t%d \t128:\t%d \t256:\t%d \t512:\t%d \t1024:\t%d \t2048:\t%d",
            //i_16_count, i_16_count_0, i_16_count_128, i_16_count_256, i_16_count_512, i_16_count_1024, i_16_count_2048);
#endif

    return 0;
}

