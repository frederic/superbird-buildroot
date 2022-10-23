/*****************************************************************************
 **
 **  Name:           app_fm_main.c
 **
 **  Description:    FM Manager main application
 **
 **  Copyright (c) 2011-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>

#include "app_fm.h"
#include "bta_api.h"
#include "bsa_api.h"
#include "app_utils.h"
#include "app_mgt.h"
#include "bt_target.h"
#include "bsa_fm_api.h"
#include "bsa_rds_api.h"
#include "bsa_fm_hwhdr.h"
#include <stdarg.h>

#define MENU_ITEM_0 0
#define MENU_ITEM_1 1
#define MENU_ITEM_2 2
#define MENU_ITEM_3 3
#define MENU_ITEM_4 4
#define MENU_ITEM_5 5
#define MENU_ITEM_6 6
#define MENU_ITEM_7 7
#define MENU_ITEM_8 8
#define MENU_ITEM_9 9
#define MENU_ITEM_10 10

#define app_fm_print    printf

/* Menu items */
enum
{
    APP_FM_MENU_OFF,
    APP_FM_MENU_ON,
    APP_FM_MENU_TUNE,
    APP_FM_MENU_SCAN,
    APP_FM_MENU_UNMUTE,
    APP_FM_MENU_MUTE,
    APP_FM_MENU_RDS,
    APP_FM_MENU_VOLUME,
    APP_FM_MENU_SET_REGION,
    APP_FM_MENU_QUIT = 99
};

tBSA_FM_REGION_CODE g_region = BSA_FM_REGION_NA;

void app_fm_display_main_menu(void);
static UINT16 gLastScanFreq = 0;

/*******************************************************************************
**
** Function         app_fm_menu_tune_frequency
**
** Description    FM tune frequency menu
**
** Returns          void
*******************************************************************************/
void app_fm_menu_tune_frequency (void)
{
    UINT32 Frequency = 10670;

    Frequency = app_get_choice("Enter exact frequency (for example 106700 for 106.7): ");
    Frequency = Frequency/10;

    app_fm_tune_freq((UINT16)Frequency);
}

/*******************************************************************************
**
** Function         app_fm_menu_search
**
** Description    FM frequency search  menu
**
** Returns          void
*******************************************************************************/
void app_fm_menu_search(tBSA_FM_SCAN_MODE scan_mode)
{
    app_fm_search_freq(scan_mode);
}

void app_fm_menu_volume()
{
    UINT16 vol = app_get_choice(" Set Volume Menu (0 - 256)");
    app_fm_set_volume(vol);
}

void app_set_region()
{
    tBSA_FM_REGION_CODE region = 0;

    app_fm_print(" Set Region Menu \n");
    app_fm_print(" 1. North America\n");
    app_fm_print(" 2. Europe\n");
    app_fm_print(" 3. Japan\n");
#if BSA_FM_10KHZ_INCLUDED
    app_fm_print(" 4. Russia\n");
    app_fm_print(" 5. China\n");
#endif
    app_fm_print(" 0. Exit \n");

    switch(app_get_choice(" Select>"))
    {
        case MENU_ITEM_1:
            region = BSA_FM_REGION_NA;
            break;

        case MENU_ITEM_2:
            region = BSA_FM_REGION_EUR;
            break;

        case MENU_ITEM_3:
            region = BSA_FM_REGION_JP;
            break;

#if BSA_FM_10KHZ_INCLUDED
        case MENU_ITEM_4:
            region = BSA_FM_REGION_RUS;
            break;

        case MENU_ITEM_5:
            region = BSA_FM_REGION_CHN;
            break;
#endif
        case MENU_ITEM_0:
        default:
            return;
    }

    app_fm_set_region(region);
}

/*******************************************************************************
**
** Function         app_fm_menu_rds
**
** Description    Display rds menu
**
** Returns          void
*******************************************************************************/
void app_fm_menu_rds(void)
{
    app_fm_print(" RDS Menu \n");
    app_fm_print(" 1. Switch RDS On \n");
    app_fm_print(" 2. Switch RDS Off \n");
    app_fm_print(" 3. Switch AF On \n");
    app_fm_print(" 4. Switch AF Off \n");
    app_fm_print(" 5. Read RDS \n");

    app_fm_print(" 0. Exit \n");
    switch(app_get_choice(" Select>"))
    {
        case MENU_ITEM_1:
            app_fm_set_rds_mode(TRUE, FALSE);
            break;

        case MENU_ITEM_2:
            app_fm_set_rds_mode(FALSE, FALSE);
            break;

        case MENU_ITEM_3:
            app_fm_set_rds_mode(TRUE, TRUE);
            break;

        case MENU_ITEM_4:
            app_fm_set_rds_mode(TRUE, FALSE);
            break;

        case MENU_ITEM_5:
            app_fm_read_rds();
            break;

        default:
            break;

    }

}

/*******************************************************************************
**
** Function         app_fm_menu_scan
**
** Description    Display scan menu
**
** Returns          void
*******************************************************************************/
void app_fm_menu_scan(void)
{
    int choice;
    app_fm_print(" Scan Menu \n");
    app_fm_print(" 1. Scan down \n");
    app_fm_print(" 2. Scan up \n");
    app_fm_print(" 0. Exit \n");
    choice = app_get_choice(" Select>");
    gLastScanFreq = 0;

    switch (choice)
    {
        case 1:
            app_fm_menu_search(BSA_FM_SCAN_DOWN);
            break;

        case 2:
            app_fm_menu_search(BSA_FM_SCAN_UP);
            break;

        default:
            break;
    }
}

/*******************************************************************************
 **
 ** Function         app_fm_cback
 **
 ** Description
 **
 ** Returns          void
 *******************************************************************************/
void app_fm_cback(tBSA_FM_EVT event, tBSA_FM *p_data)
{
    app_fm_print("app_fm_cback\n");
    switch(event)
    {
        case BSA_FM_ENABLE_EVT:
            if (app_fm_is_enabled())
                app_fm_print(" FM enabled\n");
            else
                app_fm_print(" FM not enabled\n");
            break;

        case BSA_FM_DISABLE_EVT:
            app_fm_print(" FM disabled\n");
            break;

        case BSA_FM_TUNE_EVT:
            printf("[FM] BSA_FM_TUNE_EVT status= %d, frequency %d, rssi: %d\n",
                          p_data->status, p_data->chnl_info.freq*10, (char)(p_data->chnl_info.rssi));
            if (BSA_FM_FREQ_ERR == p_data->status )
                app_fm_print(" Requested frequency is outside the allowable range");
            if (BSA_FM_OK == p_data->status)
            {
                 app_fm_print(" Tune completed");
            }
            break;

        /* this event is sent when a good channel is found, or band edge is reached following a call to BSA_FmSearchFreq */
        case BSA_FM_SEARCH_EVT:
            app_fm_print(" Search completed");
            app_fm_print("freq=%d, rssi: %d, status=%d", p_data->chnl_info.freq*10, (char)p_data->chnl_info.rssi, p_data->chnl_info.status);
            break;

        /* this event is sent when the FM radio control parameter is set to mono/stereo/blend mode. */
        case BSA_FM_AUD_MODE_EVT:
            if (BSA_FM_OK == p_data->status)
                app_fm_print(" Set Audio mode completed");
            else
                app_fm_print(" Set Audio mode failure");
            break;

        /* this event is sent when a search operation is aborted or completed */
        case BSA_FM_SEARCH_CMPL_EVT:
            printf("BSA_FM_SEARCH_CMPL_EVT: freq=%d, rssi=%d, status=%d\n",
                   p_data->scan_data.freq*10, (char)p_data->chnl_info.rssi, p_data->chnl_info.status);
            if (p_data->chnl_info.status == BSA_FM_SCAN_FAIL || p_data->chnl_info.status == BSA_FM_SCAN_ABORT)
            {
                printf("Scan failed or aborted\n");
                break;
            }
            if (g_region != BSA_FM_REGION_NA)
                break;

            if (p_data->scan_data.freq == 10800)
            {
                if (gLastScanFreq == 8750)
                    printf("FM: no channels found\n");
                else
                {
                    app_fm_search_freq(BSA_FM_SCAN_DOWN); /* Wrap and tune on the next valid station before limite */
                }
            }
            else if (p_data->scan_data.freq == 8750)
            {
                if (gLastScanFreq == 10800)
                    printf("FM: no channels found\n");
                else
                {
                    app_fm_search_freq(BSA_FM_SCAN_UP); /* Wrap and tune on the next valid station before limited */
                }
            }
            else
            {
                app_fm_print(" Search operation cancelled or complete");
            }
            gLastScanFreq = p_data->scan_data.freq;
            break;

        case BSA_FM_AUD_PATH_EVT:
            app_fm_print("Set audio path event %s, status = 0x%02x",
                         (p_data->status == BSA_FM_OK) ? "succeeded" : "failed", p_data->status);
            break;

        case BSA_FM_VOLUME_EVT:
            app_fm_print("Volume event %s, status = 0x%02x, volume=%d",
                     (p_data->status == BSA_FM_OK) ? "succeeded" : "failed", p_data->status,
                      p_data->volume.volume);
            break;

        case BSA_FM_SET_REGION_EVT:
            app_fm_print("Region set event, status = 0x%02x, region = %d", p_data->status, p_data->region_info.region);
            if (p_data->status == 0)
                g_region = p_data->region_info.region;
            break;

        default:
            return;
    }
    app_fm_display_main_menu();
}

void rds_ouput(char * fmt, ...)
{
    char rdstext[2000];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(rdstext, fmt, ap);
    vprintf(fmt, ap);
    va_end(ap);
}

void app_main_fm_rds_cback(tBSA_FM_RDS_EVT event, UINT8 *p_data)
{
    if (NULL == p_data)
        return;

    switch(event)
    {
    case BSA_RDS_PS_EVT:
        rds_ouput("RDS Program service name: %s\n", (char*)p_data);
        break;

    case BSA_RDS_PTY_EVT:
        rds_ouput("RDS Program Type: %02x\n", p_data);
        break;

    case BSA_RDS_PTYN_EVT:
        rds_ouput("RDS Program Name: %s\n", (char*)p_data);
        break;

    case BSA_RDS_RT_EVT:
        rds_ouput("RDS Radio Text: %s\n", (char*)p_data);
        break;

    case BSA_RDS_CT_EVT:
    {
        tBSA_RDS_CT *pct = (tBSA_RDS_CT*)p_data;
        rds_ouput("BSA clocktime: day [%d] hour[0x%02x] minute[0x%02x] offset = %02x\n",
                          pct->day, pct->hour, pct->minute, pct->offset);
    }
        break;

    case BSA_RDS_MS_EVT:
        rds_ouput("BSA content: %s\n",(*((tBSA_RDS_M_S*)p_data) == BSA_RDS_SPEECH )? "Speech" : "Music");
        break;
    }
    app_fm_display_main_menu();
}


/*******************************************************************************
 **
 ** Function         app_fm_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_fm_display_main_menu(void)
{
    printf("\nFM rx Main menu:\n");
    printf("    %d => Turn FM off\n", APP_FM_MENU_OFF);
    printf("    %d => Turn FM on\n", APP_FM_MENU_ON);
    printf("    %d => Tune frequency \n", APP_FM_MENU_TUNE);
    printf("    %d => Scan \n", APP_FM_MENU_SCAN);
    printf("    %d => Unmute Audio \n", APP_FM_MENU_UNMUTE);
    printf("    %d => Mute Audio \n", APP_FM_MENU_MUTE);
    printf("    %d => RDS \n", APP_FM_MENU_RDS);
    printf("    %d => Set Volume\n", APP_FM_MENU_VOLUME);
    printf("    %d => Set Region\n", APP_FM_MENU_SET_REGION);
    printf("    %d => Quit\n", APP_FM_MENU_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_fm_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_fm_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_DEBUG0("BSA_MGT_STATUS_EVT");
        break;

    case BSA_MGT_DISCONNECT_EVT:
        APP_INFO1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
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
 ** Description      This is the main function
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_fm_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Initialize FM application */
    if (app_fm_init(app_fm_cback,app_main_fm_rds_cback) < 0)
    {
        APP_ERROR0("Unable to init FM");
        return -1;
    }

    do
    {
        app_fm_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
            case APP_FM_MENU_OFF:
                app_fm_disable();
                app_fm_init(app_fm_cback,app_main_fm_rds_cback);
                break;

            case APP_FM_MENU_ON:
                app_fm_enable( BSA_FM_REGION_NA | BSA_FM_RDS_BIT |BSA_FM_AF_BIT );
                break;

            case APP_FM_MENU_TUNE:
                app_fm_menu_tune_frequency();
                break;

            case APP_FM_MENU_SCAN:
                app_fm_menu_scan();
                break;

            case APP_FM_MENU_UNMUTE:
                app_fm_mute_audio(FALSE);
				break;

            case APP_FM_MENU_MUTE:
                app_fm_mute_audio(TRUE);
				break;

            case APP_FM_MENU_RDS:
                app_fm_menu_rds();
				break;

            case APP_FM_MENU_VOLUME:
                app_fm_menu_volume();
                break;

            case APP_FM_MENU_SET_REGION:
                app_set_region();
                break;

            case APP_FM_MENU_QUIT:
                break;

            default:
                APP_ERROR1("main: Unknown choice:%d", choice);
                break;
        }
    } while (choice != APP_FM_MENU_QUIT); /* While user don't exit application */

    app_fm_disable();
    /* Close BSA before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
