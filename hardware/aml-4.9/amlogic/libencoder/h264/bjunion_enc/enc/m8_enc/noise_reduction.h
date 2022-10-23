#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H
#include <stdint.h>
typedef struct{
    int prm_is_nv21;   // 0: 420p, 1: nv21
    int prm_snr_en;    // 0: disable snr, 1: enable snr
    int prm_snr_ymode; // snr mode for Y, 0: disable, 1: gaussian, 2: bilateral
    int prm_snr_cmode;// snr mode for UV, 0: disable, 1: gaussian, 2: bilateral
    int prm_ywin	 ; // window size for Y, 0: 3x3, 1: 5x5
    int prm_cwin     ; // window size for UV, 0: 3x3, 1: 5x5
    int prm_bld_yalp ; // blend coefficent alpha for Y, 0~255
    int prm_bld_calp ; // blend coefficent alpha for UV, 0~255
    unsigned char *pNrY;
    int16_t *nr_buf;
    int32_t width;
    int32_t height;
    int mode;
}PRM_NR;

int noise_redution_init(PRM_NR *Prm_Nr, int nImgW, int nImgH);

int noise_reduction(unsigned char *pCurY, // current frame Y input
					int nImgW,  // image width
					int nImgH,  // image height
					PRM_NR *pPrm_Nr); // NR parameters

int noise_reduction_g2(unsigned char *pCurY, // current frame Y input
					int nImgW,  // image width
					int nImgH,  // image height
					PRM_NR *pPrm_Nr); // NR parameters
void noise_reduction_release(PRM_NR *pPrm_Nr);
#endif
