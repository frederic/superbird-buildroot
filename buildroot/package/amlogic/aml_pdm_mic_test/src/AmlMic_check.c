#include "AmlMic_check.h"

char* ch_copy_func(char *dest, const char *src, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		dest[i] = src[i];
	}
	for ( ; i < n; i++)
	{
		dest[i] = '\0';
	}
	return dest;
}

/* Function: ch8_4_2ch
 *
 *
 *
 *
 */
int ch8_4_2ch(char *pdm_file, int channal_count)
{
	uint8_t  temp;
	int i, j, ch8_size, ch2_size, fwrite_byte = 0, chan_count = 8;

	chan_count = channal_count;
	/*chan count check: only support:2 4 6 8*/
	if ((chan_count < 2) || (chan_count > 8) || (chan_count%2 != 0))
	{
		printf("chan count abnormal! chan_count: %d\n", chan_count);
		return CHAN_ABNORMAL;
	}

	/**/
	FILE *f = NULL, *write_f[chan_count/2];
	uint8_t *buf[chan_count/2], *buf_all, *p[chan_count/2],out_put_file[MAX_FILE_NAME]={0};

	f = fopen(pdm_file, "r");
	fseek(f, 0L, SEEK_END);

	/*2ch size*/
	ch8_size = ftell(f);
	ch2_size = ch8_size/(chan_count/2);
	//printf("8ch size: %d, 2ch size: %d\n", ch8_size, ch2_size);

	buf_all = (uint8_t *)malloc(ch8_size+1);
	if (buf_all == NULL)
	{
		printf("buf_all malloc failure!\n");
		return 0;
	}

	rewind(f);

	/*malloc all 2ch buf*/
	for (i = 0; i < (chan_count/2); i++)
	{
		buf[i] = (uint8_t *)malloc(ch2_size+1);
		if (buf[i] == NULL)
		{
			printf("buf[%d] malloc failure!\n", i);

			/*release buf*/
			do {
				free(buf[i]);
				i--;
			}while(i);
			/*release buf_all*/
			free(buf_all);
			return 0;
		}

		memset(buf[i], 0, ch2_size+1);
		/*p[i] point to buf[i]*/
		p[i] = buf[i];
	}

	/*read file to buf*/
	if (NULL == f)
	{
		printf("open pdm_half.bin failure!\n");
	    return 0;
	}

	/*read pdm_half.bin to buf_all*/
	while (!feof(f))
	{
		fread(buf_all, ch8_size, 1, f);
	}

	int count = 1;

	/*buf_all --> buf[i]*/
	for (i = 0; i < ch8_size;)
	{
		for (j = 0; j < (chan_count/2); j++)
		{
			ch_copy_func((char*)p[j], (const char*)(buf_all+i), 8);
			i += 8;
			p[j] += 8;
		}
	}

	/*output store file name*/
	for (i = 0; i < (chan_count/2); i++)
	{
		*(p[i] + 1) = '\0';
		switch (i) {
			case 0:
				strncpy((char *)out_put_file, CH1_CH2, strlen((const char *)CH1_CH2)); break;
			case 1:
				strncpy((char *)out_put_file, CH3_CH4, strlen((const char *)CH3_CH4)); break;
			case 2:
				strncpy((char *)out_put_file, CH5_CH6, strlen((const char *)CH5_CH6)); break;
			case 3:
				strncpy((char *)out_put_file, CH7_CH8, strlen((const char *)CH7_CH8)); break;
			case 4:
			case 5:
			case 6:
			case 7:
				printf("not support 10 12 14 16 channal data!\n"); break;
			default:
				printf("Unrecognized channel!");
		}
		write_f[i] = fopen((const char *)out_put_file, "wb");
		if (write_f[i] == NULL)
		{
			printf("creat file %s failure\n", out_put_file);
		}
	}

	/*write buf data to */
	for (i = 0; i < (chan_count/2); i++)
	{
		fwrite_byte = fwrite(buf[i], sizeof(char), ch2_size, write_f[i]);
		//printf("fwrite byte is: %d\n", fwrite_byte);
	}

	for (i = 0; i < (chan_count/2); i++)
	{
		fclose(write_f[i]);
	}
	//	fsync();
	fclose(f);

	for (i = 0; i < (chan_count/2); i++)
	{
		free(buf[i]);
	}

	free(buf_all);
}

/*function: compare ch1 ch2 value
 *
 */
int cal_ch(char *oddeven_ch_file, int channal_val)
{
	FILE *f = NULL;
	int file_size, read_size, i, j;
	float jump_size = 0, jump = 0, odd = 0,odd_avg = 0, even = 0, even_avg = 0;

	f = fopen(oddeven_ch_file, "r");
	fseek(f, 0L, SEEK_END);
	file_size = ftell(f);
	//printf("file size: %d\n", file_size);

	/*read file size 1/3 every ch1&ch2*/
	read_size = ((file_size/2)/3)/3;
	//printf("read size: %d\n", read_size);
	jump_size = file_size/3;

	rewind(f);

	for (j = 0; j < 1; j++) {
		for (i = 0; i < read_size; i++) {
			fread( &odd, sizeof(int), 1, f);
			odd_avg += odd;
			fread( &even, sizeof(int), 1, f);
			even_avg += even;
		}
		jump += jump_size;
		rewind(f);
		fseek(f, jump, SEEK_SET);
	}
#if 1
	printf("odd_avg: %f\n", odd_avg);
	printf("even_avg: %f\n", even_avg);
#endif
#if 1
	//printf("--------isnan:%d\n",isnan(odd_avg));
	if (isnan(odd_avg) && isnan(even_avg)) {
			fclose(f);
		return channal_val;
	}
#endif
	printf("channal_val: %d\n", channal_val);
	if ((int)abs(odd_avg - even_avg) == 0) {
		printf("ch[%d]  abnormal!\n", channal_val);
		fclose(f);
		return channal_val;
	}
	fclose(f);
	printf("channal_val2: %d\n", channal_val);

	return 0;
}

int bit_main(char *oddeven_ch_file, char *bit_process_out_file)
{
	uint8_t *buf;
	int i, size, fwrite_byte = 0;
	uint8_t  temp;
	FILE *f = NULL, *write_f = NULL;

	f = fopen(oddeven_ch_file, "r");
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	printf("size: %d\n", size);
	//fseek(f, 1L, SEEK_SET);
	rewind(f);
	buf = (uint8_t *)malloc(size+1);
	if (buf == NULL)
	{
		printf("malloc fail!\n");
		return 0;
	}

	/*read file to buf*/
	if (NULL == f)
	{
		printf("fail open %s!\n", oddeven_ch_file);
	}
	memset(buf, 0, size+1);
	while (!feof(f))
	{
		fread(buf, size, 1, f);
	}

	for (i = ((size - 1) - 4); i >= 3;)
	{

		/*move right 1 bit*/
		buf[i-3] >>= 1;
		temp = (1) & buf[i-2];
		buf[i-3] |= temp << 7;

		buf[i-2] >>= 1;
		temp = (1) & buf[i-1];
		buf[i-2] |= temp << 7;

		buf[i-1] >>= 1;
		temp = (1) & buf[i];
		buf[i-1] |= temp << 7;

		/*buf[i] top position 0*/
		buf[i] >>= 1;
		buf[i] &= 0x7f;

		/*even channal handle*/
		buf[i+4] &= 0x7f;
		/*-----jump Skip even channel*/
		i = i - 8;
	}

	write_f = fopen(bit_process_out_file, "wb");
	if (write_f == NULL)
	{
		printf("open handle_pdm.bin failure\n");
	}

	fwrite_byte = fwrite(buf, sizeof(char), size, write_f);
	//printf("fwrite byte is: %d\n", fwrite_byte);
	//	fsync();
	fclose(write_f);
	fclose(f);
	free(buf);
}
/* max_db_cal : sensitivity
 *
 */
int max_db_cal(char *oddeven_pdm_file, int channal_count)
{
	FILE	*f = NULL;
	float 	jump_size = 0;         /*Accuracy of the jump size*/
	float	jump = 0;
	uint8_t bad_mic_bit = 0;   	/*record bad mic by bit(bit0:ch1,bit1:ch2,bit2:ch3...bit8:ch8), max support 8ch*/
	uint8_t	bad_mic_all[9];       /*backup record bad mic buffer,eg:"12345678"*/
	uint8_t	bad_mic[2];
	int 	file_size;			/*oddeven_pdm_file size*/
	int		tmp_size = 0;			/*reserved for debug*/
	int  	read_size;			/*every cut read size*/
	int		i, j, k = 0, bit_wide = 32; /*bit_wide only support 32bit*/
	int		chan_count = 8;		/*default channal count*/
	int		cut_part = 3;			/*cut part*/
	int		precision = 3;		/*precision :1,2,3,4...n(n <= read_size), (file_size/cut_part/n)%8 == 0, default read 30% to cal db value*/
	int		tmp = 0;				/*cal peak point*/
//	int 	channal_count = 8;
	float	flag[8] = {0};			/*for cal effective*/
#if 0
    float 	db_ch[8] = {0}; 		/*store db value*/
	float 	ch_max[8] = {0};		/*store db max*/
	float 	ch_min[8] = {0};		/*store db min*/
	float 	ch_max_pre[8] = {0};	/*cal peak point*/
	float	ch_max_tmp[8] = {0};		/*cal peak point*/
#endif
    int 	db_ch[8] = {0}; 		/*store db value*/
	float   ch_max[8] = {0};
	float   ch_min[8] = {0};
	int     ch_max_pre[8] = {0};
	int     ch_max_tmp[8] = {0};
	int     max_base = pow(2, bit_wide-1);
#if 0
	printf("max_base: %d\n", max_base);
#endif
	/*init channal count*/
	chan_count = channal_count;

	/*Open analyze raw audio file*/
	f = fopen(oddeven_pdm_file, "r");
	/*Get raw audio file size*/
	fseek(f, 0L, SEEK_END);
	file_size = ftell(f);
	/*Read file size(count) every channal(ch1ch2...chan_count)*/
	read_size = (file_size/chan_count)/cut_part/precision;
	/*Jump size for precision*/
	jump_size = file_size/cut_part;

	/*Reposition the file header*/
	rewind(f);

	/*cut part*/
	for (j = 0; j < cut_part; j++)
	{
		/*read size depend on cut_part & precision*/
		for (i = 0; i < read_size; i++)
		{
			/*Get the values for each channel in order*/
			for (k = 0; k < chan_count;k++)
			{
				//tmp_size++;
				fread(&tmp, sizeof(int), 1, f);
#if 0
				printf("tmp: %d\n", tmp);
#endif
				/*rectangular plane coordinate system: get all the values in the first and second quadrant*/
				if (tmp > 0)
				{/*for get all peak point*/
					/**/
					if ((ch_max_tmp[k] == 0) && (ch_max_pre[k] == 0))
					{//handle first
						ch_max_pre[k] = tmp;
						ch_max_tmp[k] = tmp;
						flag[k] = 1;
					} else {//handle second
						if (flag[k] == 1)
						{
							ch_max_tmp[k] = tmp;
							flag[k]++;
							continue;
						}
						//handle three...
						if ((0 > tmp - ch_max_tmp[k]) && (0 < ch_max_tmp[k] - ch_max_pre[k]))
						{
							if (ch_max[k] == 0)
							{//first peak
								ch_max[k] = ch_max_tmp[k];
							} else { //second three ... preak
								flag[k]++;
								ch_max[k] += ch_max_tmp[k];
							}
#if 0
						printf("ch_max_tmp[%d]: %d, tmp: %d\n", k, ch_max_tmp[k], tmp);
#endif
						}
						ch_max_pre[k] = ch_max_tmp[k];
						ch_max_tmp[k] = tmp;
					}
				}

				/*record min value*/
				ch_min[k] = (tmp < ch_min[k])?tmp:ch_min[k];
			}
		}
		jump += jump_size;
		rewind(f);
		fseek(f, jump, SEEK_SET);
	}

	/*for bad mic*/
	memset(bad_mic_all, 0, sizeof(bad_mic_all));

	/*dB cal*/
	for (i = 0; i < chan_count; i++)
	{
		ch_max[i] = (ch_max[i]/(flag[i]-2))/sqrtf(2.0);
#if 0
		float test = (float)ch_max[i]/(float)max_base;
#endif
		db_ch[i] = 20*log10((float)ch_max[i]/(float)max_base);
		//printf("ch [%d]: db_ch: %f\n", i, fabs(db_ch[i]));
		if ((fabs(fabs(-37)-fabs(db_ch[i])) > 5 || (isnan(db_ch[i]))))
		{
			printf("ch[%d]: blocking!\n", i);
			memset(bad_mic, 0, sizeof(bad_mic));
			bad_mic_bit |= (1<<i);
#if 0
			sprintf(bad_mic,"%d",i+1);
			strcat(bad_mic_all, bad_mic);
#endif
		}
		printf("ch [%d]: db_ch: %ddB \n", i, db_ch[i]);
	}
	printf("block_mic_all:%d\n", bad_mic_bit);
#if 0
	printf("block_mic_all:%s\n", bad_mic_all);
#endif
	fclose(f);
#if 0
	return atoi((const char*)bad_mic_all);
#endif
	return bad_mic_bit;
}
