#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef  unsigned char uint8_t;

#define MAX_FILE_NAME 20
#define CH1_CH2 "ch1ch2.bin"
#define CH3_CH4 "ch3ch4.bin"
#define CH5_CH6 "ch5ch6.bin"
#define CH7_CH8 "ch7ch8.bin"

#define CH1_CH2_BIT "ch1ch2_bit.bin"
#define CH3_CH4_BIT "ch3ch4_bit.bin"
#define CH5_CH6_BIT "ch5ch6_bit.bin"
#define CH7_CH8_BIT "ch7ch8_bit.bin"

enum {
	CHAN_ABNORMAL = -1,
	CHAN_NORMAL = 0
};

/* function: The data of 8 channels is converted to data of 4 two channels
 * input args:
 *    pdm_file: raw data file
 *    channel:  raw data channel number
*/
int ch8_4_2ch(char *pdm_file, int channel_count);

/* function: To calculate the raw file for 2ch
 * input args:
 *   oddeven_ch_file: 2ch raw data file
 *   channel_val: to calculate channel number
 */
int cal_ch(char *oddeven_ch_file, int channel_val);

/* function: Move one bit after the odd channel, Remove the highest bit of odd and even channel Numbers
 * input args:
 *   oddeven_ch_file: 2ch raw data file
 * output args:
 *   bit_process_out_file: 2ch raw data file
 * return: 1 byte:
 *   bit[0]: 1(channel1 void join)
 *   bit[1]: 1(channel2 void join)
 *   bit[2]: 1(channel3 void join)
 *   bit[3]: 1(channel4 void join)
 *   bit[4]: 1(channel5 void join)
 *   bit[5]: 1(channel6 void join)
 *   bit[6]: 1(channel7 void join)
 *   bit[7]: 1(channel8 void join)
 * */
int bit_main(char *oddeven_ch_file, char *bit_process_out_file);

/* function: To calculate sensitivity
 * input args:
 *   oddeven_pdm_file: raw data file
 *   channel_count: raw data  channel number
 * return: 1 byte:
 *   bit[0]: 1(channel1 block)
 *   bit[1]: 1(channel2 block)
 *   bit[2]: 1(channel3 block)
 *   bit[3]: 1(channel4 block)
 *   bit[4]: 1(channel5 block)
 *   bit[5]: 1(channel6 block)
 *   bit[6]: 1(channel7 block)
 *   bit[7]: 1(channel8 block)
 * */
int max_db_cal(char *oddeven_pdm_file, int channel_count);

/* function: copy n count byte from src to dest
 *
 * */
char* ch_copy_func(char *dest, const char *src, int n);

