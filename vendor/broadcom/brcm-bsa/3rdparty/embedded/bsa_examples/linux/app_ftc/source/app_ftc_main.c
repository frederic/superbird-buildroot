/*****************************************************************************
 **
 **  Name:           app_ftc_main.c
 **
 **  Description:    Bluetooth FTC application
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "app_ftc.h"

#include "bsa_api.h"

#include "gki.h"
#include "uipc.h"

#include "app_utils.h"
#include "app_mgt.h"
#include "app_xml_param.h"

#include "app_disc.h"

#define APP_TEST_FILE_PATH "./test_file.txt"
#define APP_TEST_FILE_PATH1 "./test_file1.txt"
#define APP_TEST_FILE_PATH2 "./test_file2.txt"
#define APP_TEST_FILE_PATH3 "./test_file3.txt"
#define APP_TEST_DIR "test_dir"


/* ui keypress definition */
enum
{
    APP_FTC_KEY_OPEN,
    APP_FTC_KEY_PUT,
    APP_FTC_KEY_GET,
    APP_FTC_KEY_LS,
    APP_FTC_KEY_LS_XML,
    APP_FTC_KEY_CD,
    APP_FTC_KEY_MK_DIR,
    APP_FTC_KEY_CP,
    APP_FTC_KEY_MV,
    APP_FTC_KEY_RM,
    APP_FTC_KEY_CLOSE,
    APP_FTC_KEY_DISC,
    APP_FTC_KEY_QUIT
};

/*******************************************************************************
 **
 ** Function         app_ftc_display_main_menu
 **
 ** Description      This is the main menu
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_ftc_display_main_menu(void)
{
    printf("\nBluetooth FTP Client Menu\n");
    printf("    %d => open ftp link \n",      APP_FTC_KEY_OPEN);
    printf("    %d => put %s\n",              APP_FTC_KEY_PUT, APP_TEST_FILE_PATH);
    printf("    %d => get %s from src name %s\n",    APP_FTC_KEY_GET, APP_TEST_FILE_PATH3, APP_TEST_FILE_PATH);
    printf("    %d => list \n",               APP_FTC_KEY_LS);
    printf("    %d => list xml\n",            APP_FTC_KEY_LS_XML);
    printf("    %d => Change dir\n",       APP_FTC_KEY_CD);
    printf("    %d => Create dir\n",       APP_FTC_KEY_MK_DIR);
    printf("    %d => remove %s \n",          APP_FTC_KEY_RM, APP_TEST_FILE_PATH);
    printf("    %d => close\n",               APP_FTC_KEY_CLOSE);
    printf("    %d => discovery\n",           APP_FTC_KEY_DISC);
    printf("    %d => quit\n",                APP_FTC_KEY_QUIT);
}

/*******************************************************************************
 **
 ** Function         app_ftc_mgt_callback
 **
 ** Description      This callback function is called in case of server
 **                  disconnection (e.g. server crashes)
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static BOOLEAN app_ftc_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    switch(event)
    {
    case BSA_MGT_DISCONNECT_EVT:
        APP_DEBUG1("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
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
    int status;
    int choice;

    /* Open connection to BSA Server */
    app_mgt_init();
    if (app_mgt_open(NULL, app_ftc_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start FTS application */
    status = app_start_ftc();
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start FTC\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_ftc_display_main_menu();
        choice = app_get_choice("Select action");

        switch (choice)
        {
        case APP_FTC_KEY_OPEN:
            app_ftc_open();
            break;
        case APP_FTC_KEY_PUT:
            app_ftc_put_file(APP_TEST_FILE_PATH);
            break;
        case APP_FTC_KEY_GET:
            app_ftc_get_file(APP_TEST_FILE_PATH3, APP_TEST_FILE_PATH);
            break;
        case APP_FTC_KEY_CD:
            app_ftc_cd();
            break;
        case APP_FTC_KEY_MK_DIR:
            app_ftc_mkdir();
            break;
        case APP_FTC_KEY_CP:
            app_ftc_cp_file(APP_TEST_FILE_PATH, APP_TEST_FILE_PATH1);
            break;
        case APP_FTC_KEY_MV:
            app_ftc_mv_file(APP_TEST_FILE_PATH, APP_TEST_FILE_PATH2);
            break;
        case APP_FTC_KEY_RM:
            app_ftc_rm_file(APP_TEST_FILE_PATH);
            break;
        case APP_FTC_KEY_LS:
            app_ftc_list_dir("./",FALSE);
            break;
        case APP_FTC_KEY_LS_XML:
            app_ftc_list_dir("./", TRUE);
            break;
        case APP_FTC_KEY_CLOSE:
            app_ftc_close();
            break;
        case APP_FTC_KEY_DISC:
            /* Example to perform Device discovery (in blocking mode) */
            app_disc_start_regular(NULL);
            break;

        case APP_FTC_KEY_QUIT:
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_FTC_KEY_QUIT); /* While user don't exit application */

    /* app_stop_ftc(); // Dont need to do this since BSA will be closed but testing the function here anyway */

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}

