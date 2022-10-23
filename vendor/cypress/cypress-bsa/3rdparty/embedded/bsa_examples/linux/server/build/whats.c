/*
** whats.exe
**
** DESCRIPTION:
** Decode Broadcom BSA Server packager ID's
**
** USAGE:   whats bt_server_XX-YYYY-YYYY-YYYY-YYYY
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char *hi_names[] =
{
/* 3            2               1               0     */
  "TM",         "Traces",       "HCI-Usb",      "HCI-Uart-H4",  /* HCI Transport & TM */
  "AV-Video",   "AV-Audio",     "HS/HF",        "AG",        /*  AG & HS & AV */
  "BRR",        "UCD",          "AVK-Video",    "AVK-Audio", /* AVK */
  NULL,         "PBS",          "HH",           "DG",        /* DG & HH */

  "OPS",        "OPC",          "FTS",          "FTC",
  "HCI-Uart-H5","HCI-Reopen",   "Coex",         NULL,
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
};

static const char *lo_names[] =
{
/* 3            2               1               0     */
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,

  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
  NULL,         NULL,           NULL,           NULL,
};

int main(int argc, char *argv[])
{
   if(argc < 2)
   {
       printf( "USAGE:  whats bsa_server-YYYY-YYYY-YYYY-YYYY\n");
   }
   else
   {
       char *bank1, *bank2, *bank3, *bank4;
       char i;
       unsigned long hi_config, lo_config;
       char *supported[64];              // Hold "present" list
       char *missing[64];                // Hold "not-present" list
       char supported_ct = 0;
       char missing_ct = 0;
       char tmp_missing[50];

       printf( "\n*********************************************\n");
       printf( "BSA Server: %s\n", argv[1]);

       strtok( argv[1], "-" );

       if ((bank1 = strtok( NULL, "-" )) == NULL)
       {
           return 0;
       }
       bank2 = strtok( NULL, "-" );
       bank3 = strtok( NULL, "-" );
       bank4 = strtok( NULL, "-" );
       strcat( bank1, bank2 );
       strcat( bank3, bank4 );
       hi_config = strtoul( bank1, NULL, 16 );
       lo_config = strtoul( bank3, NULL, 16 );

       memset(supported, 0, sizeof(supported));
       memset(missing, 0, sizeof(missing));

       for( i = 0; i < sizeof(hi_names)/sizeof(char *); i++ )
       {
           if (hi_names[i])
           {
               if (((1UL << 31) >> i) & hi_config)
               {
                   supported[supported_ct++] = (char *)hi_names[i];
               }
               else
               {
                   missing[missing_ct++] = (char *)hi_names[i];
               }
           }
       }

       for( i = 0; i < sizeof(lo_names)/sizeof(char *); i++ )
       {
           if (lo_names[i])
           {
               if (((1UL << 31) >> i) & lo_config)
               {
                   supported[supported_ct++] = (char *)lo_names[i];
               }
               else
               {
                   missing[missing_ct++] = (char *)lo_names[i];
               }
           }
       }

       printf("Features Present:");
       for( i = 0; i < supported_ct; i++ )
       {
               printf("\n");
           if (supported[i])
               printf( "%12s", supported[i]);
           else
               printf( "           .");
       }

       printf("\n\nFeatures Not Present:");
       for (i = 0; i < missing_ct; i++)
       {
               printf("\n");
           if (missing[i])
               printf("%12s", missing[i]);
           else
               printf( "           .");
       }
        printf("\n");
   }
   printf("\n");
   return 0;
}

