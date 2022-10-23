/*****************************************************************************
 **
 **  Name:          server_main.c
 **
 **  Description:   Entry point "main" function for the BSA server
 **
 **  Copyright (c) 2009-2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "bt_target.h"

#include "bsa_api.h"
#include "bsa_int.h"
#include "bsa_task.h"
#include "bsa_trace.h"

#include "bta_main.h"

#if ((defined(BSA_3DTV_INCLUDED) && (BSA_3DTV_INCLUDED == TRUE)) && \
     (defined(BSA_3DS_INCLUDED) && (BSA_3DS_INCLUDED == TRUE)))
#include "bsa_dm.h"
#endif

#ifndef BSA_INITIAL_TRACE_LEVEL
#define BSA_INITIAL_TRACE_LEVEL BT_TRACE_LEVEL_DEBUG
#endif

/*******************************************************************************
*   External Definitions
*******************************************************************************/
#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
extern UINT8 hci_trace_type;
extern UINT8 l2c_trace_type;
extern UINT8 sdp_trace_type;
extern UINT8 rfc_trace_type;
extern UINT8 appl_trace_level;

extern UINT8 hci_trace_level;
extern UINT8 l2c_trace_level;
extern UINT8 sdp_trace_level;
extern UINT8 rfc_trace_level;
extern UINT8 appl_trace_level;
#endif

extern UINT8 bsa_version_string[];
extern UINT8 bsa_version_info_string[];

#if defined(HCILP_INCLUDED)&&(HCILP_INCLUDED == TRUE)
extern BOOLEAN lpm_enabled ;
#endif

/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/


/*******************************************************************************
 **
 ** Function        bte_print_usage
 **
 ** Description     Print help information on command line options
 **
 ** Returns         void
 *******************************************************************************/
void bte_print_usage()
{
     printf(
      "\nUSAGE:     bsa_server [OPTION]... \n"
      "\n"
      "    -help             get option help information\n"
#if defined(BTSNOOPDISP_INCLUDED) && (BTSNOOPDISP_INCLUDED == TRUE)
      "    -b file           specify the BT Snoop file to save serial packets\n"
#endif /* BTSNOOPDISP_INCLUDED */
      "    -d device         set the hci device interface (e.g. /dev/ttyHS0)\n"
#if defined SLIP_INCLUDED && (SLIP_INCLUDED == TRUE)
      "    -f                force no hw flow control (force h5 on uart transport / no effect on usb transport)\n"
#endif
#if defined HCILP_INCLUDED && (HCILP_INCLUDED == TRUE)
      "    -lpm              enable the low power mode"
#endif
      "    -u path           prefix all the socket names with a path\n"
#if defined(PATCHRAM_INCLUDED) && (PATCHRAM_INCLUDED == TRUE)
      "    -p patchfile      force the patchfile (RAM) to use\n"
#endif
#if defined(FLASH_UPDATE_INCLUDED) && (FLASH_UPDATE_INCLUDED == TRUE)
      "    -flash patchfile      force the patchfile (FLASH) to use\n"
#endif
#if ((defined(BSA_3DTV_INCLUDED) && (BSA_3DTV_INCLUDED == TRUE)) && \
     (defined(BSA_3DS_INCLUDED) && (BSA_3DS_INCLUDED == TRUE)))
      "    -3d type          indicates which 3D type must be used\n"
      "                          type can be one of the following values:\n"
      "                              auto = automatic (not yet implemented)\n"
      "                              chip_id = based on chip Id (not safe)\n"
      "                              legacy = force the Legacy VSC\n"
      "                              3ds = force the 3D Sync Profile (CLB & VSC)\n"
#endif
#if defined(BSA_CHANGE_STARTUP_BAUDRATE)
      "    -r baudrate       set baud rate for opening port\n"
      "                              Baud rate codes:\n"
      "                              0  = 300 baud           \n"
      "                              1  = 600                \n"
      "                              2  = 1200               \n"
      "                              3  = 2400               \n"
      "                              4  = 9600               \n"
      "                              5  = 19200              \n"
      "                              6  = 57600              \n"
      "                              7  = 115200             \n"
      "                              8  = 230400             \n"
      "                              9  = 460800             \n"
      "                              10 = 921600             \n"
      "                              11 = 1M                 \n"
      "                              12 = 1_5M               \n"
      "                              13 = 2M                 \n"
      "                              14 = 2_5M               \n"
      "                              15 = 3M                 \n"
      "                              16 = 4M                 \n"
#endif
#if defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
      "    -k file           specify the BLE local key file\n"
#endif
#if defined(BSA_CHANGE_HOGP_DEV_PATH)&&(BSA_CHANGE_HOGP_DEV_PATH == TRUE)
      "    -g path           path for hogp_devices file, path must end with a / \n"
#endif
#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
      "    -all=#            set trace level to # for all layers\n"
      "    -diag=#           set DIAG trace level to #\n"
      "    -hci=#            set HCI trace level to #\n"
      "    -l2c=#            set L2CAP trace level to #\n"
      "    -app=#            set APPL trace level to #\n"
      "    -btm=#            set BTM trace level to #\n"
      "    -sdp=#            set SDP trace level to #\n"
      "    -rfc=#            set RFCOMM trace level to #\n"
#if (defined(SPP_INCLUDED) && SPP_INCLUDED == TRUE)
      "    -spp=#            set SPP trace level to #\n"
#endif
#if (defined(GAP_INCLUDED) && GAP_INCLUDED == TRUE)
      "    -gap=#            set GAP trace level to #\n"
#endif
#if (defined(OBX_INCLUDED) && OBX_INCLUDED == TRUE)
      "    -obx=#            set OBEX trace level to #\n"
#endif
#if (defined(GOEP_INCLUDED) && GOEP_INCLUDED == TRUE)
      "    -goep=#           set GOEP trace level to #\n"
#endif
#if (defined(BPP_INCLUDED) && BPP_INCLUDED == TRUE)
      "    -bpp=#            set BPP trace level to #\n"
#endif
#if (defined(BIP_INCLUDED) && BIP_INCLUDED == TRUE)
      "    -bip=#            set BIP trace level to #\n"
#endif
#if (defined(TCS_INCLUDED) && TCS_INCLUDED == TRUE)
      "    -tcs=#            set TCS trace level to #\n"
#endif
#if (defined(ICP_INCLUDED) && ICP_INCLUDED == TRUE)
      "    -icp=#            set ICP trace level to #\n"
#endif
#if (defined(CTP_INCLUDED) && CTP_INCLUDED == TRUE)
      "    -ctp=#            set CTP trace level to #\n"
#endif
#if (defined(OPP_INCLUDED) && OPP_INCLUDED == TRUE)
      "    -opp=#            set OPP trace level to #\n"
#endif
#if (defined(FTP_INCLUDED) && FTP_INCLUDED == TRUE)
      "    -ftp=#            set FTP trace level to #\n"
#endif
#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
      "    -a2d=#            set A2DP trace level to #\n"
#endif
#if (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE)
      "    -avdt=#           set AVDT trace level to #\n"
#endif
#if (defined(AVCT_INCLUDED) && AVCT_INCLUDED == TRUE)
      "    -avct=#           set AVCT trace level to #\n"
#endif
#if (defined(AVRC_INCLUDED) && AVRC_INCLUDED == TRUE)
      "    -avrc=#           set AVRC trace level to #\n"
#endif
#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
      "    -mca=#            set MCA trace level to #\n"
#endif
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
      "    -gatt=#           set GATT trace level to #\n"
#endif
     "\n"
     "   The possible levels are the following:\n"
     "      NONE    0           No trace messages at all\n"
     "      ERROR   1           Error condition trace messages\n"
     "      WARNING 2           Warning condition trace messages\n"
     "      CUSTOM  3           CUSTOM traces\n"
     "      API     4           API traces\n"
     "      EVENT   5           Debug messages for events\n"
     "      DEBUG   6           Full debug messages\n\n"
     "   The use of the option on the command line without any number will\n"
     "   result in the maximum trace level of 5 (DEBUG) for the layer.\n"

#endif /* BT_USE_TRACES */
      "\n");

      printf("\nEXAMPLES:\n"
      "Example:    bsa_server -d /dev/com10\n"
          "This command would force the HCI to use /dev/com10\n\n"
#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
      "Example:    bsa_server -d /dev/com10 -hci -l2c -app -a2d\n"
      "This command would establish the following trace setting:\n"
      "    HCI layer = 6 (DEBUG)\n"
      "    L2CAP layer = 6 (DEBUG)\n"
      "    APPL layer = 6 (DEBUG)\n"
      "    A2DP layer = 6 (DEBUG)\n"
      "\n"
      "Example:    bsa_server -hci=0 -l2c=5 -app -a2d=4\n"
      "This command would establish the following trace setting:\n"
      "    HCI layer = 0 (NONE)\n"
      "    L2CAP layer = 5 (EVENT)\n"
      "    APPL layer = 6 (DEBUG)\n"
      "    A2DP layer = 4 (API)\n"
#endif /* BT_USE_TRACES */
      "\n");
}

/*******************************************************************************
 **
 ** Function        bte_parse_cmd_line
 **
 ** Description     Parse the command line options
 **
 ** Returns         void
 *******************************************************************************/
int bte_parse_cmd_line(int argc, char **argv, tBSA_SV_BOOT *p_param)
{
    int c;
    UINT8 trace_level = BT_TRACE_LEVEL_DEBUG;

    /* by default */
    p_param->p_uart_name = NULL;
#if defined (PATCHRAM_INCLUDED) && (PATCHRAM_INCLUDED == TRUE)
    p_param->p_patchfile = NULL;
#endif
#if defined (BTSNOOPDISP_INCLUDED) && (BTSNOOPDISP_INCLUDED == TRUE)
    p_param->p_btsnoop = NULL;
#endif
#if defined (BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
    p_param->p_ble_local_key = NULL;
#endif
    p_param->p_uipc_path = NULL;
#if defined (BSA_CHANGE_STARTUP_BAUDRATE)
    p_param->p_baud_rate = NULL;
#endif
#if defined SLIP_INCLUDED && (SLIP_INCLUDED == TRUE)
    p_param->flow_control = TRUE;
#endif
#if defined(BSA_CHANGE_HOGP_DEV_PATH)&&(BSA_CHANGE_HOGP_DEV_PATH == TRUE)
    p_param->p_hogp_dev_path = NULL;
#endif
    while (1)
    {
        int option_index = 0;
        struct option long_options[] =
        {
            /* All the supported parameters must be defined here even if compiled out because the
             * index number must be identical whatever the compilation options are (see below switch
             * block).  Validity of the option is checked in the switch.  Index 1 to 25 are
             * only to set trace levels
             */
            {"help", no_argument, 0, 0},        /* 0 Help => no parameter */
            {"all", optional_argument, 0, 0},   /* 1 Set tracing all layers => 1 optional parameter */
            {"hci", optional_argument, 0, 0},   /* 2 HCI layer tracing => 1 optional parameter */
            {"l2c", optional_argument, 0, 0},   /* 3 L2CAP layer tracing => 1 optional parameter */
            {"app", optional_argument, 0, 0},   /* 4 APPL layer tracing  => 1 optional parameter */
            {"btm", optional_argument, 0, 0},   /* 5 BTM layer tracing  => 1 optional parameter */
            {"sdp", optional_argument, 0, 0},   /* 6 SDP layer tracing  => 1 optional parameter */
            {"rfc", optional_argument, 0, 0},   /* 7 RFCOMM layer tracing  => 1 optional parameter */
            {"spp", optional_argument, 0, 0},   /* 8 SPP layer tracing => 1 optional parameter */
            {"gap", optional_argument, 0, 0},   /* 9 GAP layer tracing  => 1 optional parameter */
            {"obx", optional_argument, 0, 0},   /* 10 OBEX layer tracing  => 1 optional parameter */
            {"goep", optional_argument, 0, 0},  /* 11 GOEP layer tracing  => 1 optional parameter */
            {"bpp", optional_argument, 0, 0},   /* 12 BPP layer tracing => 1 optional parameter */
            {"bip", optional_argument, 0, 0},   /* 13 BIP layer tracing  => 1 optional parameter */
            {"tcs", optional_argument, 0, 0},   /* 14 TCS layer tracing  => 1 optional parameter */
            {"icp", optional_argument, 0, 0},   /* 15 ICP layer tracing  => 1 optional parameter */
            {"ctp", optional_argument, 0, 0},   /* 16 CTP layer tracing  => 1 optional parameter */
            {"opp", optional_argument, 0, 0},   /* 17 OPP layer tracing  => 1 optional parameter */
            {"ftp", optional_argument, 0, 0},   /* 18 FTP layer tracing  => 1 optional parameter */
            {"a2d", optional_argument, 0, 0},   /* 19 A2DP layer tracing  => 1 optional parameter */
            {"avdt", optional_argument, 0, 0},  /* 20 AVDT layer tracing  => 1 optional parameter */
            {"avct", optional_argument, 0, 0},  /* 21 AVCT layer tracing  => 1 optional parameter */
            {"avrc", optional_argument, 0, 0},  /* 22 AVRC layer tracing  => 1 optional parameter */
            {"diag", optional_argument, 0, 0},  /* 23 Diag LM layer tracing => 1 optional parameter */
            {"mca", optional_argument, 0, 0},   /* 24 MCA layer tracing => 1 optional parameter */
            {"gatt", optional_argument, 0, 0},  /* 25 gatt layer tracing => 1 optional parameter */
            {"lpm", no_argument, 0, 0},         /* 26 Low Power Mode => no parameter */
            {"3d", required_argument, 0, 0},    /* 27 3D type =>  1 parameter */
            {"flash", required_argument, 0, 0}, /* 28 Flash download =>  1 parameter */
            {"pp", required_argument, 0, 0},    /* 29 patchram dir => 1 parameter */
            {NULL, 0, NULL, 0}
        };

        c = getopt_long_only (argc, argv, "b:d:p:u:r:f:g:k:", long_options, &option_index);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 0: /* long option */
#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
            if (option_index >= 1 && option_index <= 25)
            {
                if (optarg)
                {
                    if ((atoi(optarg) > MAX_TRACE_LEVEL) || (atoi(optarg) < BT_TRACE_LEVEL_NONE))
                    {
                        APPL_TRACE_ERROR1("ERROR: invalid trace level for %s", long_options[option_index].name);
                        return -1;
                    }
                    else
                    {
                        APPL_TRACE_DEBUG2("Command [%s] with arg [%s]", long_options[option_index].name, optarg);
                        trace_level = atoi(optarg);
                    }
                }
                else
                {
                    APPL_TRACE_WARNING1("Command [%s] with no arg, assuming trace level DEBUG",
                            long_options[option_index].name);
                    trace_level = BT_TRACE_LEVEL_DEBUG;
                }
                if ((option_index > 1) && trace_level)
                {
                    /* make sure the traces are not globally disabled */
                    global_trace_disable = FALSE;
                }
                APPL_TRACE_DEBUG2("[ %s ] trace level = %d", long_options[option_index].name, trace_level);
            }
#endif
            switch (option_index)
            {
            case 0:
                bte_print_usage();
                return -1;
                break;
#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
            case 1:
                BTE_SetAllTrace(trace_level, &p_param->bsa_trace_level);
                break;
            case 2:
                hci_trace_level = trace_level;
                break;
            case 3:
                l2c_trace_level = trace_level;
                break;
            case 4:
                p_param->bsa_trace_level.appl_trace_level = trace_level;
                break;
            case 5:
                p_param->bsa_trace_level.btm_trace_level = trace_level;
                p_param->bsa_trace_level.btm_trace_level = trace_level;
                break;
            case 6:
                sdp_trace_level = trace_level;
                break;
#if (defined(RFCOMM_INCLUDED) && RFCOMM_INCLUDED == TRUE)
            case 7:
                rfc_trace_level = trace_level;
                break;
#endif
#if (defined(SPP_INCLUDED) && SPP_INCLUDED == TRUE)
            case 8:
                p_param->bsa_trace_level.spp_trace_level = trace_level;
                break;
#endif
#if (defined(GAP_INCLUDED) && GAP_INCLUDED == TRUE)
            case 9:
                p_param->bsa_trace_level.gap_trace_level = trace_level;
                break;
#endif
#if (defined(OBX_INCLUDED) && OBX_INCLUDED == TRUE)
            case 10:
                p_param->bsa_trace_level.obex_trace_level = trace_level;
                break;
#endif
#if (defined(GOEP_INCLUDED) && GOEP_INCLUDED == TRUE)
            case 11:
                p_param->bsa_trace_level.goep_trace_level = trace_level;
                break;
#endif
#if (defined(BPP_INCLUDED) && BPP_INCLUDED == TRUE)
            case 12:
                p_param->bsa_trace_level.bpp_trace_level = trace_level;
                break;
#endif
#if (defined(BIP_INCLUDED) && BIP_INCLUDED == TRUE)
            case 13:
                p_param->bsa_trace_level.bip_trace_level = trace_level;
                break;
#endif
#if (defined(TCS_INCLUDED) && TCS_INCLUDED == TRUE)
            case 14:
                p_param->bsa_trace_level.tcs_trace_level = trace_level;
                break;
#endif
#if (defined(ICP_INCLUDED) && ICP_INCLUDED == TRUE)
            case 15:
                p_param->bsa_trace_level.icp_trace_level = trace_level;
                break;
#endif
#if (defined(CTP_INCLUDED) && CTP_INCLUDED == TRUE)
            case 16:
                p_param->bsa_trace_level.ctp_trace_level = trace_level;
                break;
#endif
#if (defined(OPP_INCLUDED) && OPP_INCLUDED == TRUE)
            case 17:
                p_param->bsa_trace_level.opp_trace_level = trace_level;
                break;
#endif
#if (defined(FTP_INCLUDED) && FTP_INCLUDED == TRUE)
            case 18:
                p_param->bsa_trace_level.ftp_trace_level = trace_level;
                break;
#endif
#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
            case 19:
                p_param->bsa_trace_level.a2d_trace_level = trace_level;
                break;
#endif
#if (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE)
            case 20:
                p_param->bsa_trace_level.avdt_trace_level = trace_level;
                break;
#endif
#if (defined(AVCT_INCLUDED) && AVCT_INCLUDED == TRUE)
            case 21:
                p_param->bsa_trace_level.avct_trace_level = trace_level;
                break;
#endif
#if (defined(AVRC_INCLUDED) && AVRC_INCLUDED == TRUE)
            case 22:
                p_param->bsa_trace_level.avrc_trace_level = trace_level;
                break;
#endif
            case 23:
                p_param->bsa_trace_level.diag_trace_level = trace_level;
                break;
#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
            case 24:
                p_param->bsa_trace_level.mca_trace_level = trace_level;
                break;
#endif
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
            case 25:
                p_param->bsa_trace_level.gatt_trace_level = trace_level;
                break;
#endif
#endif /* BT_USE_TRACES */
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
            case 26:
                lpm_enabled = TRUE;
                break;
#endif
#if ((defined(BSA_3DTV_INCLUDED) && (BSA_3DTV_INCLUDED == TRUE)) && \
        (defined(BSA_3DS_INCLUDED) && (BSA_3DS_INCLUDED == TRUE)))
            case 27:
                if (bsa_sv_dm_set_3d_type((UINT8 *)optarg) != BSA_SUCCESS)
                {
                    APPL_TRACE_ERROR1("ERROR: invalid 3d type option [%s]",optarg);
                    return -1;
                }
                break;
#endif
#if defined(FLASH_UPDATE_INCLUDED)&&(FLASH_UPDATE_INCLUDED == TRUE)
            case 28:
                p_param->patch_type = BSA_SV_FLASH_NO_MINIDRV_PATCH_TYPE;
                p_param->p_patchfile = optarg;
                APPL_TRACE_DEBUG1("Firmware name %s",p_param->p_patchfile);
                break;
#endif
#if (defined(PATCHRAM_DIR_INCLUDED) && (PATCHRAM_DIR_INCLUDED == TRUE))
            case 29:
                p_param->p_patchdir = optarg;
                break;
#endif  /* PATCHRAM_DIR_INCLUDED == TRUE */
            default:
                APPL_TRACE_ERROR1("[ %s ] is not compiled in application", long_options[option_index].name);
                break;
            }
            break;

#if defined(BTSNOOPDISP_INCLUDED) && (BTSNOOPDISP_INCLUDED == TRUE)
        case 'b':
            p_param->p_btsnoop = optarg;
            break;
#endif /* BTSNOOPDISP_INCLUDED */
#if defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
        case 'k':
            p_param->p_ble_local_key = optarg;
            break;
#endif /* BLE_INCLUDED */
        case 'd':
            p_param->p_uart_name = optarg;
            break;
        case 'u':
            p_param->p_uipc_path = optarg;
            /* make sure that the path ends with a / */
            if (optarg[strlen(optarg)-1] != '/')
            {
                APPL_TRACE_ERROR0("UIPC path does not end with a / (slash) character");
                return -1;
            }
            break;
#if defined(BSA_CHANGE_STARTUP_BAUDRATE)
        case 'r':
            p_param->p_baud_rate = optarg;
            /* check for valid range of baud rate code */
            if ((atoi(optarg) > 16) || (atoi(optarg) < 0))
            {
                APPL_TRACE_ERROR0("ERROR: invalid baud rate code");
                return -1;
            }
            break;
#endif
#if defined(PATCHRAM_INCLUDED)&&(PATCHRAM_INCLUDED == TRUE)
        case 'p':
#if defined(FLASH_UPDATE_INCLUDED)&&(FLASH_UPDATE_INCLUDED == TRUE)
            p_param->patch_type = BSA_SV_RAM_PATCH_TYPE;
#endif
            p_param->p_patchfile = optarg;
            break;
#endif

#if defined SLIP_INCLUDED && (SLIP_INCLUDED == TRUE)
        case 'f':
            p_param->flow_control = FALSE;
            break;
#endif
#if defined(BSA_CHANGE_HOGP_DEV_PATH)&&(BSA_CHANGE_HOGP_DEV_PATH == TRUE)
        case 'g':
            p_param->p_hogp_dev_path = optarg;
            /* make sure that the path ends with a / */
            if (optarg[strlen(optarg)-1] != '/')
            {
                APPL_TRACE_ERROR0("HOGP device file path does not end with a / (slash) character");
                return -1;
            }
            break;
#endif

        case '?':
            /*nobreak*/
        default:
            APPL_TRACE_ERROR1("ERROR: invalid param %d", c);
            bte_print_usage();
            return -1;
            break;
        }
    }

    if (optind < argc)
    {
        APPL_TRACE_ERROR1("Unknown parameter %s", argv[optind]);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      Start BSA server.
 **
 ** Returns          error level
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    tBSA_SV_BOOT  param;
    int rv;

    memset(&param,0,sizeof(param));

#if defined(BT_USE_TRACES) && (BT_USE_TRACES == TRUE)
    /* Until this function is invoked, there should be no traces */
    BTE_SetAllTrace(BSA_INITIAL_TRACE_LEVEL, &param.bsa_trace_level);
#endif

#if ((defined(BSA_3DTV_INCLUDED) && (BSA_3DTV_INCLUDED == TRUE)) && \
     (defined(BSA_3DS_INCLUDED) && (BSA_3DS_INCLUDED == TRUE)))
    bsa_sv_dm_set_3d_type((UINT8 *)"chip_id");
#endif

    /* Parse the command line parameters */
    rv = bte_parse_cmd_line(argc, argv, &param);
    if (rv)
    {
        /* the cmd line parser failed of help menu */
        /* error traces are done in bte_parse_cmd_line */
        return rv;
    }

    APPL_TRACE_DEBUG0("Starting Bluetooth Daemon");

    APPL_TRACE_DEBUG1("BSA version:%s",bsa_version_string);
    if(strlen((char *)bsa_version_info_string))
    {
        APPL_TRACE_DEBUG1("BSA server version info:%s\n",bsa_version_info_string);
    }

#if defined(BSA_SIMPLE_PAIRING_DEBUG) && (BSA_SIMPLE_PAIRING_DEBUG == TRUE)
    APPL_TRACE_ERROR0("WARNING Simple Pairing Debug Mode Enabled !!!!!");
    APPL_TRACE_ERROR0("This BSA Server cannot be used neither for Production nor for Qualification");
#endif

    /* Start the stack */
    BSA_Boot(&param);

    return 0;
}

