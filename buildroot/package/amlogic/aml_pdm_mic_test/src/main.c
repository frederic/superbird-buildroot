#include "AmlMic_check.h"
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

char PDM_FILE[30] = {0};

static long parse_long(const char *str, int *err)
{
	long val;
	char *endptr;

	errno = 0;
	val = strtol(str, &endptr, 0);

	if (errno != 0 || *endptr != '\0')
		*err = -1;
	else
		*err = 0;

	return val;
}
static void usage(char *command)
{
	printf(
"usage: %s  option\n"
"\n"
"-h, --help              help\n"
"-f, input source file, eg: pdm.wav\n"
"-c, input source file channal number\n"
"-r, mic ic type value, eg: -37\n"
"-j, test function(1: rosin joint, 2:block)\n"

	, command);
}

int main(int argc, char *argv[])
{
	int option_index, err;
	int join_block_flag = 0, odd_flag = 0;
	char INPUT_PDM_CH_ODD_EVEN[30], OUTPUT_PDM_CH_BIT_ODD_EVEN[30];

	static const char short_options[] = "h:f:c:r:j:b"
#ifdef AMLPRICISION
	"p"
#endif
	;
	int c, void_join = 0;
	long channal = 8, reference_type = -37;
	char* command;

	command = argv[0];
	static const struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"pdm file", 0, 0, 'f'},
		{"function choice", 0, 0, 'j'},
		{"chnanl count", 0, 0, 'c'},
		{"Standard sensitivity", 0, 0, 'r'},
		{0, 0, 0, 0}
	}
	;

	while ((c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (c) {
			case 'h':
				usage(command);
				return 0;
			case 'f':
				printf("f optarg : %s\n", optarg);
				if (sizeof(optarg) > 29)
				{
					printf("%s overflow! (default is <29)\n", optarg);
					return -1;
				}
				strncpy(PDM_FILE, optarg, strlen(optarg));
				break;
			case 'c':
				printf("c optarg : %s\n", optarg);
				 channal = parse_long(optarg, &err);
				 if (err < 0) {
					 printf("invalid channels argument %s", optarg);
					 return 1;
				 }
				 break;
			case 's':
				printf("s optarg : %s\n", optarg);
				 reference_type = parse_long(optarg, &err);
				 if (err < 0) {
					 printf("invalid Standard sensitivity  %s", optarg);
					 return 1;
				 }
				 break;
			case 'j':
				printf("j optarg : %s\n", optarg);
				 void_join = parse_long(optarg, &err);
				 if (err < 0) {
					 printf("invalid function module %s", optarg);
					 return 1;
				 }
				 break;
#ifdef AMLPRICISION
			case 'p':
				 printf("reserved!\n");
				 break;
#endif
			default:
				 channal = 8;
				 reference_type = -37;
				 void_join = 1;
		}
	}
	printf("void join :%d\n", void_join);

	/*void join test need kernel disable filter*/
	/*void join test*/
	if (void_join == 1)
	{
		ch8_4_2ch(PDM_FILE, channal);
		/*first check even channal*/
		int i = 2;
		for (i = 2; i <= channal;)
		{
			memset(INPUT_PDM_CH_ODD_EVEN, 0, sizeof(INPUT_PDM_CH_ODD_EVEN));
			memset(OUTPUT_PDM_CH_BIT_ODD_EVEN, 0, sizeof(OUTPUT_PDM_CH_BIT_ODD_EVEN));

			switch (i) {
				case 2:
					strncpy(INPUT_PDM_CH_ODD_EVEN, CH1_CH2, sizeof(CH1_CH2));
					strncpy(OUTPUT_PDM_CH_BIT_ODD_EVEN, CH1_CH2_BIT, sizeof(CH1_CH2_BIT));
					break;
				case 4:
					strncpy(INPUT_PDM_CH_ODD_EVEN, CH3_CH4, sizeof(CH3_CH4));
					strncpy(OUTPUT_PDM_CH_BIT_ODD_EVEN, CH3_CH4_BIT, sizeof(CH3_CH4_BIT));
					break;
				case 6:
					strncpy(INPUT_PDM_CH_ODD_EVEN, CH5_CH6, sizeof(CH5_CH6));
					strncpy(OUTPUT_PDM_CH_BIT_ODD_EVEN, CH5_CH6_BIT, sizeof(CH5_CH6_BIT));
					break;
				case 8:
					strncpy(INPUT_PDM_CH_ODD_EVEN, CH7_CH8, sizeof(CH7_CH8));
					strncpy(OUTPUT_PDM_CH_BIT_ODD_EVEN, CH7_CH8_BIT, sizeof(CH7_CH8_BIT));
					break;
				defalut:
					printf("not support your channal value|n");
			}

			switch (cal_ch(INPUT_PDM_CH_ODD_EVEN, i)) {
				case 2:
					join_block_flag |= (1<<1);
					break;
				case 4:
					join_block_flag |= (1<<3);
					break;
				case 6:
					join_block_flag |= (1<<5);
					break;
				case 8:
					join_block_flag |= (1<<7);
					break;
#if 0
				case 0:
					bit_main(INPUT_PDM_CH_ODD_EVEN, OUTPUT_PDM_CH_BIT_ODD_EVEN);
					odd_flag = cal_ch(OUTPUT_PDM_CH_BIT_ODD_EVEN, i-1);
					if (odd_flag != 0) {
						join_block_flag |= (1<<(odd_flag-1));
					}
					break;
#endif
				default:
					printf("nor suport odd even value");
			}
#if  1  /*handle odd & even void join at the same time*/
			bit_main(INPUT_PDM_CH_ODD_EVEN, OUTPUT_PDM_CH_BIT_ODD_EVEN);
			odd_flag = cal_ch(OUTPUT_PDM_CH_BIT_ODD_EVEN, i-1);
			printf("-----odd_flag: %d\n", odd_flag);
			if (odd_flag != 0) {
				join_block_flag |= (1<<(odd_flag-1));
			}
#endif
			i +=2;
		}
	}

	/*block test need kernel enable filter*/
	/*block flag test*/
	if (void_join == 2)
	{
		join_block_flag = max_db_cal(PDM_FILE, channal);
	}
		printf("------------join_block_flag: %d\n", join_block_flag);
	return join_block_flag;
}
