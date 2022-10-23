/*****************************************************************************
**
**  Name:           app_sc.c
**
**  Description:    Bluetooth Phonebook client sample application
**
**  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
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
#include <errno.h>
#include "gki.h"
#include "uipc.h"
#include "bsa_api.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_disc.h"
#include "app_mgt.h"
#include "app_utils.h"

#define APP_TEST_VCAR_PATH "(1.vcf)"
#define APP_TEST_DIR "(telecom, pb etc.)"
#define MAX_PATH_LENGTH 512
#define DEFAULT_SEC_MASK (BSA_SEC_AUTHENTICATION | BSA_SEC_ENCRYPTION)


/*****************************************************************************
**  Constants
*****************************************************************************/
#define BSA_SC_READER_ID       (3)                             /* Card Reader identifier */
#define BSA_SC_READER_FLAGS    (0)                             /* Card Reader flags 0=Not removable */
#define BSA_SC_MSGMIN          32                              /* Min msg size for accessing SIM card */
#define BSA_SC_MSGMAX          300                             /* Max msg size for accessing SIM card */


/*
 * Types
 */
typedef struct
{
    BD_ADDR bd_addr;
} tAPP_SC_CB;

/*
 * global variable
 */

tAPP_SC_CB app_sc_cb;

/* ui keypress definition */
enum
{
    APP_SC_MENU_NULL,
    APP_SC_MENU_SET_CARD_STATUS,
    APP_SC_MENU_SET_READER_STATUS,
    APP_SC_MENU_CLOSE,
    APP_SC_MENU_QUIT
};


/*******************************************************************************
**
** Function         app_sc_display_main_menu
**
** Description      This is the main menu
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sc_display_main_menu(void)
{
    printf("\nBluetooth SAP Server Test Application\n");
    printf("    %d => Set Card Status \n", APP_SC_MENU_SET_CARD_STATUS);
    printf("    %d => Set Reader Status \n", APP_SC_MENU_SET_READER_STATUS);
    printf("    %d => Close Connection\n", APP_SC_MENU_CLOSE);
    printf("    %d => Quit\n", APP_SC_MENU_QUIT);
}

/*******************************************************************************
**
** Function         app_sc_mgt_callback
**
** Description      This callback function is called in case of server
**                  disconnection (e.g. server crashes)
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
static BOOLEAN app_sc_mgt_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
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
** Function         app_sc_set_evt
**
** Description      Response events received in response to GET command
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sc_set_evt(tBSA_SC_MSG *p_data)
{
    APPL_TRACE_DEBUG1("app_sc_set_evt SET EVT response received... p_data = %x", p_data);

    if(!p_data)
        return;

    switch(p_data->set.type)
    {
    case BSA_SC_SET_CARD_STATUS:       /* Set Card status complete */
        APPL_TRACE_DEBUG0("app_sc_set_evt SET EVT  Set Card status complete received");
        break;

    case BSA_SC_SET_READER_STATUS:       /* Set Card status complete */
        APPL_TRACE_DEBUG0("app_sc_set_evt SET EVT  Set Card status complete received");
        break;
    default:
        break;
    }
}

/*******************************************************************************
**
** Function         app_sc_open_evt
**
** Description      Response events received in response to OPEN command
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sc_open_evt(tBSA_SC_MSG *p_data)
{
    APPL_TRACE_DEBUG0("app_sc_open_evt received...");

    if(p_data)
            printf("Status : %d \n", p_data->open.status);

    if(p_data && p_data->open.status == BSA_SUCCESS)
    {
        printf("Remote bdaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
            p_data->open.bd_addr[0], p_data->open.bd_addr[1], p_data->open.bd_addr[2], p_data->open.bd_addr[3], p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
    }
}

/*******************************************************************************
**
** Function         app_sc_close_evt
**
** Description      Response events received in response to CLOSE command
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sc_close_evt(tBSA_SC_MSG *p_data)
{
    APPL_TRACE_DEBUG0("app_sc_close_evt received...");

}

/*******************************************************************************
**
** Function         app_sc_cback
**
** Description      Example of SC callback function to handle events related to SC
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void app_sc_cback(tBSA_SC_EVT event, tBSA_SC_MSG *p_data)
{
    switch (event)
    {
    case BSA_SC_ENABLE_EVT: /* Enable event */
        printf("BSA_SC_ENABLE_EVT received\n");
        break;

    case BSA_SC_OPEN_EVT: /* Connection Open (for info) */
        printf("BSA_SC_OPEN_EVT Status %d\n", p_data->open.status);
        app_sc_open_evt(p_data);
        break;

    case BSA_SC_CLOSE_EVT: /* Connection Closed (for info)*/
        printf("BSA_SC_CLOSE_EVT\n");
        app_sc_close_evt(p_data);
        break;

    case BSA_SC_DISABLE_EVT: /* SAP Server module disabled*/
        printf("BSA_SC_DISABLE_EVT\n");
        break;

    case BSA_SC_SET_EVT:
        printf("BSA_SC_SET_EVT status %d\n", p_data->set.status);
        app_sc_set_evt(p_data);
        break;

    default:
        fprintf(stderr, "app_sc_cback unknown event:%d\n", event);
        break;
    }
}


/*******************************************************************************
**
** Function         app_start_sc
**
** Description      Example of function to enable SC functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_start_sc(void)
{
    tBSA_STATUS status = BSA_SUCCESS;

    tBSA_SC_ENABLE enable_param;

    printf("app_start_sc\n");

        /* Initialize the control structure */
    memset(&app_sc_cb, 0, sizeof(app_sc_cb));


    status = BSA_ScEnableInit(&enable_param);

    enable_param.p_cback = app_sc_cback;

     enable_param.sec_mask = DEFAULT_SEC_MASK;
     enable_param.reader_id = BSA_SC_READER_ID;
     enable_param.reader_flags = BSA_SC_READER_FLAGS;
     enable_param.msg_size_min = BSA_SC_MSGMIN;
     enable_param.msg_size_max = BSA_SC_MSGMAX;
     strcpy(enable_param.service_name, "TEST SAP Server");


    status = BSA_ScEnable(&enable_param);
    printf("app_start_sc Status: %d \n", status);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_start_sc: Unable to enable SC status:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_stop_sc
**
** Description      Example of function to stop SC functionality
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_stop_sc(void)
{
    tBSA_STATUS status;

    tBSA_SC_DISABLE disable_param;

    printf("app_stop_sc\n");

    status = BSA_ScDisableInit(&disable_param);

    status = BSA_ScDisable(&disable_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_stop_sc: Error:%d\n", status);
    }
    return status;
}

/*******************************************************************************
**
** Function         app_sc_close
**
** Description      Example of function to disconnect current connection
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sc_close(void)
{
    int choice = 0;
    tBSA_STATUS status;

    tBSA_SC_CLOSE close_param;

    printf("app_sc_close\n");

    status = BSA_ScCloseInit(&close_param);

    APP_INFO0("Choose Close Type:");
    APP_INFO0("    0 BSA_SC_DISC_GRACEFUL");
    APP_INFO0("    1 BSA_SC_DISC_IMMEDIATE");
    choice = app_get_choice("Close Type");

    close_param.type = choice ? BSA_SC_DISC_IMMEDIATE : BSA_SC_DISC_GRACEFUL;

    status = BSA_ScClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sc_close: Error:%d\n", status);
    }
    return status;
}


/*******************************************************************************
**
** Function         app_sc_set_card_status
**
** Description      Example of function to perform a set current folder operation
**6
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sc_set_card_status()
{  int choice = 0;
   tBSA_SC_SET set_param;
   tBSA_STATUS status;

    status = BSA_ScSetInit(&set_param);
    set_param.type = BSA_SC_SET_CARD_STATUS;

    APP_INFO0("Choose Card Status:");

    APP_INFO0("    0 BSA_SC_CARD_RESET");
    APP_INFO0("    1 BSA_SC_CARD_REMOVED");
    APP_INFO0("    2 BSA_SC_CARD_INSERTED");
    choice = app_get_choice("Set Card Status to: ");

    if(choice == 0)
        set_param.card_status = BSA_SC_CARD_RESET;
    else if(choice == 1)
        set_param.card_status = BSA_SC_CARD_REMOVED;
    else if(choice == 2)
        set_param.card_status = BSA_SC_CARD_INSERTED;

    status = BSA_ScSet(&set_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sc_set_card_status: Error:%d\n", status);
    }
    return status;
}



/*******************************************************************************
**
** Function         app_sc_set_reader_status
**
** Description      Example of function to perform a set current folder operation
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
tBSA_STATUS app_sc_set_reader_status()
{  int choice = 0;
   tBSA_SC_SET set_param;
   tBSA_STATUS status;

    status = BSA_ScSetInit(&set_param);
    set_param.type = BSA_SC_SET_READER_STATUS;

    APP_INFO0("Choose Reader Status:");
    APP_INFO0("    0 BSA_SC_READER_STATUS_ATTACHED");
    APP_INFO0("    1 BSA_SC_READER_STATUS_REMOVED");
    choice = app_get_choice("Set Reader Status ?");

    set_param.reader_status = choice ? BSA_SC_READER_STATUS_REMOVED : BSA_SC_READER_STATUS_ATTACHED;

    status = BSA_ScSet(&set_param);

    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "app_sc_set_reader_status: Error:%d\n", status);
    }
    return status;
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
    if (app_mgt_open(NULL, app_sc_mgt_callback) < 0)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    /* Example of function to start SC application */
    status = app_start_sc();
    if (status != BSA_SUCCESS)
    {
        fprintf(stderr, "main: Unable to start SC\n");
        app_mgt_close();
        return status;
    }

    do
    {
        app_sc_display_main_menu();
        choice = app_get_choice("Select action");

        APPL_TRACE_DEBUG1("SC Main Choise: %d", choice);

        switch (choice)
        {
        case APP_SC_MENU_NULL:
            break;

        case APP_SC_MENU_SET_CARD_STATUS:
            app_sc_set_card_status();
            break;

        case APP_SC_MENU_SET_READER_STATUS:
            app_sc_set_reader_status();
            break;

        case APP_SC_MENU_CLOSE:
            app_sc_close();
            break;

        case APP_SC_MENU_QUIT:
            break;

        default:
            printf("main: Unknown choice:%d\n", choice);
            break;
        }
    } while (choice != APP_SC_MENU_QUIT); /* While user don't exit application */

    /* example to stop SC service */
    app_stop_sc();

    /* Just to make sure we are getting the disable event before close the
    connection to the manager */
    sleep(2);

    /* Close BSA Connection before exiting (to release resources) */
    app_mgt_close();

    return 0;
}
