#ifndef  _HFP_CTL_
#define  _HFP_CTL_
int hfp_ctl_init(void);
void hfp_ctl_delinit(void);
int answer_call(void);
int reject_call(void);
int VGS_up(void);
int VGS_down(void);
int VGM_up(void);
int VGM_down(void);
int set_VGS(int value);
int set_VGM(int value);
#endif
