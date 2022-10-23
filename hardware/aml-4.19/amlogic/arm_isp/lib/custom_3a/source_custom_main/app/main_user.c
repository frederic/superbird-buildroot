//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#include <signal.h>

#include "acamera_types.h"
#include "acamera_logger.h"
#include "acamera_firmware_config.h"
#include "acamera_firmware_api.h"
#include "acamera_control_config.h"
#include "acamera_command_api.h"
#include "acamera_connection.h"
#include "system_interrupts.h"
#include "system_control.h"

// the settings for each firmware context were pre-generated and
// saved in the header file. They are given as a reference and should be changed
// according to the customer needs.
#include "runtime_initialization_settings.h"

// This function can be used to customize initialization process.
// It is called by the firmware at the end of each context initialization.
// The context id can be used to differentiate them
void custom_initialization( uint32_t ctx_num )
{
}

// The ISP pipeline can have several outputs such as Full Resolution, DownScaler1, DownScaler2 etc
// It is possible to set up the firmware to return the metadata for each output frame from
// the specific channel. This callbacks must be set in acamera_settings structure and passed to the firmware in
// acamera_init api function
// The context id can be used to differentiate contexts

// Callback from FR output pipe
void callback_fr( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
    //printf("callback1 addr:0x%lx status:%d\n",aframe->address,(int)aframe->status);
}

// Callback from DS1 output pipe
void callback_ds1( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
    //printf("callback addr:0x%lx\n",aframe->address);
}

// Callback from DS2 output pipe
void callback_ds2( uint32_t ctx_num, tframe_t *tframe, const metadata_t *metadata )
{
}


// On systems which support pthreads it is more efficient to run
// control channel in a separate thread to let the firmware to communicate
// with ACamera Control Tool (ACT)
// ACT allows to change the firmware behaviour by calling API functions, change ISP registers and
// update calibration LUTs.
// Please read the ACamera Control Tool User Guide for details
#if ISP_HAS_STREAM_CONNECTION && CONNECTION_IN_THREAD
#include <pthread.h>
static void *connection_thread( void *foo )
{
    // acamera_connection_init is used to initialize the
    // communication channel between the firmware application
    // and the firmware. It is used only together with ACT tool
    // and may be omitted on the customer discretion
    // if ACT is not required
    acamera_connection_init();
    for ( ;; ) {
        // the function checks the incoming requests from
        // ACT tool and call the corresponding API command.
        // Please note that acamera_connection_process may be ommitted
        // if ACT tool is not used for the firmware API.
        acamera_connection_process();
    }
}
#endif


// This example will run the infinite loop to control firmware.
// This variable also can be changed outside to stop the test or just
// changed on another condition.
volatile int32_t acamera_main_loop_active = 1;


// this is a main application IRQ handler to drive the firmware
// The main purpose is to redirect ISP irq event to the firmware core
// Please see the ACamera Porting Guide for details.
static void interrupt_handler( void *ptr, uint32_t mask )
{
    // the lower bits are for ISP interrupts on ACamera FPGA reference platform
    uint32_t isp_mask = mask & 0x0000FFFF;

    // tell the firmware that isp interrupts happened
    if ( isp_mask ) {
        // the first irq pins are connected to ISP
        acamera_interrupt_handler();
    }
}

void halt_handler( int arg )
{
    LOG( LOG_DEBUG, "at halt_handler %d", arg );
    acamera_main_loop_active = 0;
}


// The basic usage example for
// ACamera firmware is given below.
int main( void )
{
    int32_t result = 0;

    signal( SIGINT, halt_handler );
    // The custom platform must be ready to run
    // any system routines from ./platform folder.
    // So bsp_init allows to initialise the system if necessary.
    // This function may be omitted if no initialisation is required
    bsp_init();

// ACamera provides a simple protocol to communicate with firmware
// outside application. Several channels are supported depend on the
// system requirements.
// To start using ACamera Control Tool the connection must be initialised
// before by calling acamera_connection_init
// The connection module parses input commands from ACT and call the required
// api command like acamera_command or acamera_calibrations.
// Please see acamera_command_api.h for details.
#if ISP_HAS_STREAM_CONNECTION
#if !CONNECTION_IN_THREAD
    acamera_connection_init();
#else
    pthread_t control_thread_ptr = 0;
    int res = pthread_create( &control_thread_ptr, NULL, connection_thread, NULL );
#endif
#endif

    // The firmware supports multicontext.
    // It means that the customer can use the same firmware for controlling
    // several instances of different sensors/isp. To initialise a context
    // the structure acamera_settings must be filled properly.
    // the total number of initialized context must not exceed FIRMWARE_CONTEXT_NUMBER
    // all contexts are numerated from 0 till ctx_number - 1
    result = acamera_init( settings, FIRMWARE_CONTEXT_NUMBER );


    if ( result == 0 ) {
        // set the interrupt handler. The system must call this interrupt_handler
        // function whenever the ISP interrupt happens.
        // This interrupt handling procedure is only advisable and is used in ACamera demo application.
        // It can be changed by a customer discretion.
        system_interrupt_set_handler( interrupt_handler, NULL );


        // acamera_process function must be called on every incoming interrupt
        // to give the firmware the possibility to apply
        // all internal algorithms and change the ISP state.
        // The external application can be run in the same loop on bare metal systems.
        while ( acamera_main_loop_active ) {
            // the function processes all events which were
            // generated by the internal firmware logic on
            // the incoming interrupts.
            acamera_process();
#if ISP_HAS_STREAM_CONNECTION && !CONNECTION_IN_THREAD
            // acamera_connection_process is used for communication between
            // firmware and ACT through different possible channels like
            // cmd_queue memory in ISP, socket, UART, chardev etc.
            // Different channels can be supported depending on the target
            // platform. The common case when cmd_queue buffer is used
            // ( see acamera_isp_config.h )
            // The channels supported by this routine can be used not only on
            // NIOS2 platform but on the customer system as well.
            acamera_connection_process();
#endif
        }
    } else {
        LOG( LOG_ERR, "Failed to start firmware processing thread. " );
    }

#if ISP_HAS_STREAM_CONNECTION && CONNECTION_IN_THREAD
    pthread_join( control_thread_ptr, NULL );
#endif
    // this api function will free
    // all allocated resources by the firmware
    acamera_terminate();

    // Additional work may be required after firmware finished
    // this function can be used to free memory regions or
    // for any other purposes.
    bsp_destroy();

    return 0;
}
