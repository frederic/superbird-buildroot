/*****************************************************************************
**
**  Name:           simple_app.c
**
**  Description:    Bluetooth BSA test functions
**
**  Copyright (c) 2009-2016, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "simple_app.h"
#include "app_utils.h"
#include "app_mgt.h"

#include "dm.h"
#include "security.h"
#include "discovery.h"
#include "hh.h"
#include "stress.h"
#include "av.h"
#include "app_xml_utils.h"
#include "vsc.h"
#include "hci_cmd.h"

tSIMPLE_APP_CB simple_app;

/* #define SIMPLE_APP_DEBUG TRUE */

/*
 * Local Functions
 */
static int simple_app_read_hex_nibble(char c);
static int simple_app_read_hex_byte(char *buf);

/*******************************************************************************
 **
 ** Function         parse_help
 **
 ** Description      This function parse help command
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_help(char *optarg)
{
    simple_app.bsa_command = HELP;
    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_vsc
 **
 ** Description      This function parses vsc command
 **
 ** Parameters       Pointer to the start of option argument
 ** Returns          int
 **
 *******************************************************************************/
int parse_vsc(char *optarg)
{
    int index;
    int rv;
    char *p;

    simple_app.bsa_command = VSC;

    p = optarg;

    simple_app.bsa_vsc.opcode = 0;

    /* Read Opcode LSB */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_vsc.opcode = (UINT16)rv;

    /* Read Opcode MSB */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_vsc.opcode |= (UINT16)(rv << 8);

    /* Read Parameter's length */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_vsc.length = (UINT16)rv;

    /* Read Parameters */
    for(index = 0 ; index < simple_app.bsa_vsc.length ; index++)
    {
        /* Read Parameter's, byte by byte */
        rv = simple_app_read_hex_byte(p);
        if (rv < 0)
        {
            return -1;
        }
        p += 2;
        simple_app.bsa_vsc.data[index] = (UINT8)rv;
    }

#if defined(SIMPLE_APP_DEBUG) && (SIMPLE_APP_DEBUG == TRUE)
    printf("VSC opcode:0x%04X\n", simple_app.bsa_vsc.opcode);
    printf("VSC length:0x%04X\n", simple_app.bsa_vsc.length);
    printf("VSC Data:");
    for (index = 0 ; index < (simple_app.bsa_vsc.length) ; index++)
    {
        printf ("%02X ", simple_app.bsa_vsc.data[index]);
    }
    printf("\n");
#endif
    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_hci
 **
 ** Description      This function parses HCI command
 **
 ** Parameters       Pointer to the start of option argument
 ** Returns          int
 **
 *******************************************************************************/
int parse_hci(char *optarg)
{

    int index;
    int rv;
    char *p;

    simple_app.bsa_command = HCI_CMD;

    p = optarg;

    simple_app.bsa_hci.opcode = 0;

    /* Read Opcode LSB */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_hci.opcode = (UINT16)(rv << 8);

    /* Read Opcode MSB */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_hci.opcode |= (UINT16)rv;

    /* Read Parameter's length */
    rv = simple_app_read_hex_byte(p);
    if (rv < 0)
    {
        return -1;
    }
    p += 2;
    simple_app.bsa_hci.length = (UINT16)rv;

    /* Read Parameters */
    for(index = 0 ; index < simple_app.bsa_hci.length ; index++)
    {
        /* Read Parameter's, byte by byte */
        rv = simple_app_read_hex_byte(p);
        if (rv < 0)
        {
            return -1;
        }
        p += 2;
        simple_app.bsa_hci.data[index] = (UINT8)rv;
    }

#if defined(SIMPLE_APP_DEBUG) && (SIMPLE_APP_DEBUG == TRUE)
    printf("HCI opcode:0x%04X\n", simple_app.bsa_vsc.opcode);
    printf("HCI length:0x%04X\n", simple_app.bsa_vsc.length);
    if (simple_app.bsa_vsc.length > 0)
    {
        printf("HCI Data:");
        for (index = 0 ; index < simple_app.bsa_vsc.length ; index++)
        {
            printf ("%02X ", simple_app.bsa_vsc.data[index]);
        }
        printf("\n");
    }
#endif
    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_config
 **
 ** Description      This function parse config command
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_config(char *optarg)
{
    simple_app.bsa_command = CONFIG;
    return 0;
}


/*******************************************************************************
 **
 ** Function         parse_disc
 **
 **
 ** Description      This function parse discovery command
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_disc(char *optarg)
{
    simple_app.bsa_command = DISCOVERY;
    return 0;
}


/*******************************************************************************
 **
 ** Function         parse_sec
 **
 ** Description      This function parse security commands
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_sec(char *optarg)
{
    int status = 0;

    printf("Security:");
    if (strcmp(optarg, "bond") == 0)
    {
        simple_app.bsa_command = SEC_BOND;
        printf("Bond\n");
    }
    else if (strcmp(optarg, "add") == 0)
    {
        simple_app.bsa_command = SEC_ADD;
        printf("Add\n");
    }
    else
    {
        printf("=> Unknown Security action:%s\n", optarg);
        simple_app.bsa_command = NO_COMMAND;
        status = -1;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         parse_hh
 **
 ** Description      This function parse hid connection commands
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_hh(char *optarg)
{
    int status = 0;

    printf("Hid Host:");
    if (strcmp(optarg, "open") == 0)
    {
        simple_app.bsa_command = HH_OPEN;
        printf("open\n");
    }
    else if (strcmp(optarg, "wopen") == 0)
    {
        simple_app.bsa_command = HH_WOPEN;
        printf("wopen\n");
    }
    else
    {
        printf("=> Unknown Hid Host action:%s\n", optarg);
        simple_app.bsa_command = NO_COMMAND;
        status = -1;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         parse_av
 **
 ** Description      This function parse av command
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_av(char *optarg)
{
    int status = 0;

    printf("AV:");
    if (strcmp(optarg, "tone") == 0)
    {
        simple_app.bsa_command = AV_PLAY_TONE;
        printf("play tone\n");
    }
    else
    {
        printf("=> Unknown AV action:%s\n", optarg);
        simple_app.bsa_command = NO_COMMAND;
        status = -1;
    }

    return status;
}


/*******************************************************************************
 **
 ** Function         parse_stress
 **
 ** Description      This function parse stress test command
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_stress(char *optarg)
{
    int status = 0;

    printf("Stress:");
    if (strcmp(optarg, "ping") == 0)
    {
        simple_app.bsa_command = STRESS_PING;
        printf("ping\n");
    }
    else if (strcmp(optarg, "hci") == 0)
    {
        simple_app.bsa_command = STRESS_HCI;
        printf("ping\n");
    }
    else
    {
        printf("=> Unknown Stress action:%s\n", optarg);
        simple_app.bsa_command = NO_COMMAND;
        status = -1;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         parse_bdaddr
 **
 ** Description      This function parse bd address option
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_bdaddr(char *optarg)
{
    unsigned int local_bd_addr[BD_ADDR_LEN];
    int index;

    simple_app.bd_addr_present = TRUE;

    sscanf(optarg, "%02x%02x%02x%02x%02x%02x",
        &local_bd_addr[0], &local_bd_addr[1], &local_bd_addr[2],
        &local_bd_addr[3], &local_bd_addr[4], &local_bd_addr[5]);

    for(index=0;index<BD_ADDR_LEN;index++)
    {
        simple_app.device_bd_addr[index] = local_bd_addr[index];
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_link_key
 **
 ** Description      This function parse link key option
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_link_key(char *optarg)
{
    int local_link_key[LINK_KEY_LEN] = {0};
    int index;

    simple_app.link_key_present = TRUE;

    sscanf(optarg, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        &local_link_key[0], &local_link_key[1], &local_link_key[2], &local_link_key[3],
        &local_link_key[4], &local_link_key[5], &local_link_key[6], &local_link_key[7],
        &local_link_key[8], &local_link_key[9], &local_link_key[10], &local_link_key[11],
        &local_link_key[12], &local_link_key[13], &local_link_key[14], &local_link_key[15]);

    for(index=0; index<LINK_KEY_LEN;index++)
    {
        simple_app.link_key[index] = local_link_key[index];
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_loop
 **
 ** Description      This function parse loop option
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_loop(char *optarg)
{
    sscanf(optarg, "%d", &simple_app.loop);
    return 0;
}

/*******************************************************************************
 **
 ** Function         parse_pin
 **
 ** Description      This function parse pin code option
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_pin(char *optarg)
{
    strncpy(simple_app.pin_code, optarg, sizeof(simple_app.pin_code));
    simple_app.pin_code[sizeof(simple_app.pin_code)-1] = 0;

    simple_app.pin_len = strlen(simple_app.pin_code);

    printf("pin code: %s\n",simple_app.pin_code);

    return 0;
}

/*******************************************************************************
 **
 ** Function         print_usage
 **
 ** Description      This function displays command line usage
 **
 ** Parameters
 ** Returns          void
 **
 *******************************************************************************/
void print_usage(char **argv)
{
    char    *p_cmd;

    p_cmd = argv[0] + strlen(argv[0]) - 2;
    while ((p_cmd > argv[0]) &&
           (*p_cmd != '/'))
        p_cmd--;
    if (*p_cmd == '/')
        p_cmd++;

    printf("Usage %s: command [param]\n", p_cmd);
    printf("Commands\n");
    printf("  --help                     print this help\n");
    printf("  --config                   Configure local devices\n");
    printf("  --disc                     Discover devices\n");
    printf("  --sec bond                 Bond a device\n");
    printf("  --sec add                  Add a device in security database\n");
    printf("  --hh open                  Open HH connection to a device\n");
    printf("  --hh wopen                 Wait Open HH connection from a device\n");
    printf("  --av tone                  Open AV connection to a device and play tone\n");
    printf("  --stress ping              Stress: Ping BT server daemon\n");
    printf("  --stress hci               Stress: Send ReadVerbose VSC\n");
    printf("[param]\n");
    printf("  --bdaddr <bd_address>      Enter device addr (12 hexa bytes without 0x)\n");
    printf("  --key    <link-key>        Enter Link Key (32 hexa bytes without 0x)\n");
    printf("  --loop   <nb_ping>         Enter Number of ping\n");
    printf("  --pin    <pin code>        Enter pin code\n");
    printf("  --hci    <HCI_command>     Enter <HCI command> in the format below \n");
    printf("                                   opcode (2 bytes)\n");
    printf("                                   length (1 byte)\n");
    printf("                                   data (number of bytes defined by length)\n");
    printf("  --vsc    <VSC_command>     Enter <VSC command> in the following format:\n");
    printf("                                   opcode (1 byte)\n");
    printf("                                   vsc code (1 byte)\n");
    printf("                                   length (1 byte)\n");
    printf("                                   data (number of bytes defined by length)\n");
    printf("Examples\n");
    printf("%s --config [--bdaddr 001122334455]   Configure local device\n", p_cmd);
    printf("%s --disc                             Perform a device discovery\n", p_cmd);
    printf("%s --sec bond --bdaddr 001122334455   Bond with <bdaddr>\n", p_cmd);
    printf("%s --sec add --bdaddr 001122334455 --key xx..zz  Add a remote device in data_base\n", p_cmd);
    printf("%s --hh open --bdaddr 001122334455    Open a HH connection to <bdaddr>\n", p_cmd);
    printf("%s --hh wopen --bdaddr 001122334455   Wait for <bdaddr> to open a HH connection\n", p_cmd);
    printf("%s --av tone --bdaddr 001122334455    Open AV connection to a device and play tone\n", p_cmd);
    printf("%s --vsc  01FC06001122334455          VSC Write_BD_ADDR\n", p_cmd);
    printf("%s --hci  030C00                      HCI Reset cmd \n", p_cmd);
}

/*******************************************************************************
 **
 ** Function         parse_cmd_line
 **
 ** Description      This function parse the complete command line
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int parse_cmd_line(int argc, char **argv)
{
    int c;

    typedef int (*PFI)();

    PFI parse_param[] =
    {
        parse_help,
        parse_config,
        parse_disc,
        parse_sec,
        parse_hh,
        parse_bdaddr,
        parse_link_key,
        parse_stress,
        parse_loop,
        parse_av,
        parse_pin,
        parse_vsc,
        parse_hci
    };

    while (1)
    {
        int option_index = 0;

        static struct option long_options[] = {
         {"help", 0, 0, 0},     /* Help => no parameter */
         {"config", 0, 0, 0},   /* config command => no parameter */
         {"disc", 0, 0, 0},     /* Discovery command => no parameter */
         {"sec", 1, 0, 0},      /* Security  command => 1 parameter */
         {"hh", 1, 0, 0},       /* HH command => 1 parameter */
         {"bdaddr", 1, 0, 0},   /* BdAddr parameter => 1 parameter */
         {"key", 1, 0, 0},      /* LinkKey parameter => 1 parameter */
         {"stress", 1, 0, 0},   /* Stress command => 1 parameter */
         {"loop", 1, 0, 0},     /* Loop parameter => 1 parameter */
         {"av", 1, 0, 0},       /* AV parameter => 1 parameter */
         {"pin", 1, 0, 0},       /* Pin parameter => 1 parameter */
         {"vsc", 1, 0, 0},       /* VSC parameter => 1 parameter */
         {"hci", 1, 0, 0},       /* HCI parameter => 1 parameter */
         {0, 0, 0, 0}
        };

        c = getopt_long_only (argc, argv, "d", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
        case 0:
            printf ("Command [%s]", long_options[option_index].name);

            if (optarg) {
                printf (" with arg [%s]", optarg);
            }

            printf ("\n");

            if ((*parse_param[option_index])(optarg) < 0)
            {
                return -1;
            }
        break;

        case '?':
        default:
          print_usage(argv);
          return -1;
            break;
        }
    }

    if (optind < argc)
    {
        if (optind < argc)
        {
            printf ("Unknown parameter %s\n", argv[optind]);
            print_usage(argv);
        }
        printf ("\n");
    }
    return 0;
}

/*
 * simple_app_read_hex_nibble
 */
static int simple_app_read_hex_nibble(char c)
{
    if ((c >='0') && (c <= '9'))
    {
        return c - '0';
    }
    else if ((c >='A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }
    else if ((c >='a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }

    APP_ERROR0("read_hex_nibble: Error char not in supported range");
    return -1;
}

/*
 * simple_app_read_hex_byte
 */
static int simple_app_read_hex_byte(char *p_buf)
{
    int rv;
    int value;

    /* Read First nibble (4 MSB) */
    rv = simple_app_read_hex_nibble(p_buf[0]);
    if (rv < 0)
    {
        return -1;
    }
    value = rv << 4;

    /* Read Second nibble (4 LSB) */
    rv = simple_app_read_hex_nibble(p_buf[1]);
    if (rv < 0)
    {
        return -1;
    }
    value += rv;

    return value;
}
/*******************************************************************************
 **
 ** Function         simple_app_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN simple_app_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_DISCONNECT_EVT:
        APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d\n", p_data->disconnect.reason);
        /* Connection with the Server lost => Just exit the application */
        exit(-1);
        break;

    default:
        break;
    }
    return FALSE;
}

/*******************************************************************************
 **
 ** Function         main
 **
 ** Description      main function
 **
 ** Parameters
 ** Returns          int
 **
 *******************************************************************************/
int main (int argc, char **argv)
{
    int                 index;
    tSEC_BOND_PARAM     sec_bond_param;
    tSEC_ADD_PARAM      sec_add_param;
    tDISC_PARAM         disc_param;
    tBSA_DM_SET_CONFIG  dm_set_config_param;
    tAPP_STRESS_PING    stress_ping_param;
    tAPP_STRESS_HCI     stress_hic_param;
    tAV_PLAY_TONE_PARAM av_play_tone_param;
    tBSA_TM_VSC         vsc_param;
    tBSA_TM_HCI_CMD     hci_param;
    tBSA_SEC_SET_SECURITY sec_local_config;

    memset(&simple_app,0,sizeof(simple_app));

    simple_app.pin_len = 4;
    simple_app.loop = 1;
    /* Set 0x30 to force the pin code bytes to ascii character '0' */
    memset(simple_app.pin_code,0x30,PIN_CODE_LEN);

    if (parse_cmd_line(argc, argv) < 0)
    {
        APP_ERROR0("Bad Parameter");
        return -1;
    }

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, simple_app_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    switch(simple_app.bsa_command)
    {
    case NO_COMMAND:
        print_usage(argv);
        return 1;
        break;

    case HELP:
        print_usage(argv);
        return 0;
        break;

    case CONFIG:
        dm_set_config_init_param(&dm_set_config_param);
        if (simple_app.bd_addr_present == TRUE)
        {
            bdcpy(dm_set_config_param.bd_addr,simple_app.device_bd_addr);
        }
        dm_set_config(&dm_set_config_param);
        break;

    case DISCOVERY:
        discovery_init_param(&disc_param);
        disc_param.duration = 5;
        discovery(&disc_param);
        break;

    case SEC_ADD:
        sec_add_init_param(&sec_add_param);
        if (simple_app.bd_addr_present == FALSE)
        {
            printf ("bdaddr parameter required for Sec Add\n");
            return 1;
        }

        bdcpy(sec_add_param.bd_addr,simple_app.device_bd_addr);

        memcpy(sec_add_param.link_key,simple_app.link_key,sizeof(sec_add_param.link_key));

        sec_add_param.link_key_present = simple_app.link_key_present ;

        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();

        /* Here key type is forced to 0 */
        app_xml_update_key_db(app_xml_remote_devices_db,
                              APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                              sec_add_param.bd_addr,
                              sec_add_param.link_key,
                              sec_add_param.key_type);

        app_xml_update_cod_db(app_xml_remote_devices_db,
                              APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                              sec_add_param.bd_addr,
                              sec_add_param.class_of_device);

        app_write_xml_remote_devices();

        sec_add(&sec_add_param);
        break;

    case SEC_BOND:
        sec_bond_init_param(&sec_bond_param);
        if (simple_app.bd_addr_present == FALSE)
        {
            printf ("bdaddr parameter required for Sec Bond\n");
            return 1;
        }

        bdcpy(sec_bond_param.bd_addr,simple_app.device_bd_addr);

        sec_bond(&sec_bond_param);
        break;

    case HH_OPEN:
        if (simple_app.bd_addr_present == FALSE)
        {
            printf ("bdaddr parameter required for HH Open\n");
            return 1;
        }

        dm_set_config_init_param(&dm_set_config_param);
        dm_set_config(&dm_set_config_param);

        sec_set_local_config(&sec_local_config);

        hh_open();
        break;

    case HH_WOPEN:
        if (simple_app.bd_addr_present == FALSE)
        {
            printf ("bdaddr parameter required for HH WOpen\n");
            return 1;
        }

        dm_set_config_init_param(&dm_set_config_param);
        /* Set the local config (discoverable/connectable) */
        dm_set_config(&dm_set_config_param);

        sec_set_local_config(&sec_local_config);

        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();

        for(index = 0; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db); index++)
        {
            if(app_xml_remote_devices_db[index].in_use == TRUE)
            {
                sec_add_init_param(&sec_add_param);
                bdcpy(sec_add_param.bd_addr,app_xml_remote_devices_db[index].bd_addr);
                memcpy(sec_add_param.class_of_device,
                        app_xml_remote_devices_db[index].class_of_device,
                        sizeof(sec_add_param.class_of_device));
                memcpy(sec_add_param.link_key,
                        app_xml_remote_devices_db[index].link_key,
                        sizeof(sec_add_param.link_key));
                sec_add_param.key_type = app_xml_remote_devices_db[index].key_type;
                sec_add_param.link_key_present = app_xml_remote_devices_db[index].link_key_present;
                memcpy(&sec_add_param.trusted_services,
                        &app_xml_remote_devices_db[index].trusted_services,
                        sizeof(sec_add_param.trusted_services));
                sec_add(&sec_add_param);
            }
            else
            {
                break;
            }
        }

        hh_wopen();
        break;

    case AV_PLAY_TONE:
        av_play_tone_init_param(&av_play_tone_param);
        if (simple_app.bd_addr_present == FALSE)
        {
            printf ("bdaddr parameter required for AV play tone\n");
            return 1;
        }
        bdcpy(av_play_tone_param.bd_addr,simple_app.device_bd_addr);
        av_play_tone(&av_play_tone_param);
        break;


    case STRESS_PING:
        app_stress_ping_init(&stress_ping_param);
        stress_ping_param.nb_ping = simple_app.loop;
        app_stress_ping(&stress_ping_param);
        break;

    case STRESS_HCI:
        app_stress_hci_init(&stress_hic_param);
        stress_hic_param.nb_loop = simple_app.loop;
        app_stress_hci(&stress_hic_param);
        break;

    case VSC:
        vsc_send_init_param(&vsc_param);
        vsc_param.opcode = simple_app.bsa_vsc.opcode;          /* VSC */
        vsc_param.length = simple_app.bsa_vsc.length;          /* Data Len */
        memcpy(vsc_param.data, simple_app.bsa_vsc.data, simple_app.bsa_vsc.length);
        vsc_send(&vsc_param);
        break;

    case HCI_CMD:
        hci_send_init_param(&hci_param);
        hci_param.opcode = simple_app.bsa_hci.opcode;          /* HCI  */
        hci_param.length = simple_app.bsa_hci.length;          /* Data Len */
        memcpy(hci_param.data, simple_app.bsa_hci.data, simple_app.bsa_hci.length);
        hci_send(&hci_param);
        break;

    }

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
