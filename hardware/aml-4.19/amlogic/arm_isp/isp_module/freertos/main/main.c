/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


/*
 * This project contains an application demonstrating the use of the
 * FreeRTOS.org mini real time scheduler on the Luminary Micro LM3S811 Eval
 * board.  See http://www.FreeRTOS.org for more information.
 *
 * main() simply sets up the hardware, creates all the demo application tasks,
 * then starts the scheduler.  http://www.freertos.org/a00102.html provides
 * more information on the standard demo tasks.
 *
 * In addition to a subset of the standard demo application tasks, main.c also
 * defines the following tasks:
 *
 */

/* Scheduler includes. */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Demo app includes. */
#include "serial.h"
#include "watchdog.h"
#include "printf.h"
#include "hwcrypto.h"
#include "gpio.h"
#include "saradc.h"
#include "tsensor.h"
#include "FreeRTOS_CLI.h"
#include "wifibt.h"
#include "lwip/tcpip.h"
#include "pm_shutdown.h"
#if ENABLE_MODULE_SPI
#include "spicc.h"
#include "spi.h"
#include "spi_plat.h"
#endif
#ifdef ARCH64
#include "gicv2.h"
#endif

#ifdef GUEST
#include <tick_timer.h>
#endif

#include <rtosinfo.h>
#include <xlat_tables.h>

/* Delay between cycles of the 'check' task. */
#define mainCHECK_DELAY						( ( TickType_t ) 5000 / portTICK_PERIOD_MS )

/* Demo task priorities. */
#define mainQUEUE_POLL_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define mainSEM_TEST_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY		( tskIDLE_PRIORITY + 2 )

#define MAX_INPUT_LENGTH 50
#define MAX_OUTPUT_LENGTH 100

/*
 * Configure the processor and peripherals for this demo.
 */
static void prvSetupHardware( void );

/*
 * The task that controls access to the LCD.
 */
#if !defined(GUEST)
static void vPrintTask( void *pvParameter );
static void vPrintTask2( void *pvParameters );
#endif
#if configEnable_Console
static void vConsoleTask( void *pvParameters );
#endif
extern void vApplicationIdleHook( void );
extern void vMbInit( void );
#ifdef configCLKTREE_TEST
extern void vClockInit( void );
#endif
/* extern void vRamdiskInit( void ); */
extern void vSdmmcInit( void );
extern void vLedInit( void );
extern void vIspInit(void);
#ifdef ARCH64
extern void vSpiFlashInit(void);
#endif
extern void vHwClockSourceInit( void );
#ifdef GUEST
extern void vMainPrivilegeInit(void);
#endif
extern void vSdioTest(void);
extern void sdmmc_card_init(void);
#if ENABLE_MODULE_AUDIO
extern void audio_pre_init(void);
#endif
/* The semaphore used to wake the button handler task from within the GPIO
interrupt handler. */
SemaphoreHandle_t xButtonSemaphore;

/* The queue used to send strings to the print task for display on the LCD. */
QueueHandle_t xPrintQueue;
TaskHandle_t console_thread;

const mmap_region_t board_mmap[] = {
	{ 0x1000, 0x1000, 0x3ffff000,
		MT_MEMORY | MT_RW | MT_SECURE },
	{ 0 }
};

#if ENABLE_MODULE_SPI
static const int Spicc0CsGpios[] = {GPIOA_8, GPIOA_9};
static void vSpicc0Init(void)
{
	struct SpiccHwPlatformData pdata = {
		.compatible = MESON_G12A_SPICC,
		.reg = 0xfe003800,
		.bus_num = 0,
		.cs_gpios = Spicc0CsGpios,
		.num_chipselect = ARRAY_SIZE(Spicc0CsGpios),
		.clk_rate = 192000000,
	};

	/* set spicc0 pinmux */
	xPinmuxSet(GPIOA_5, PIN_FUNC3);
	xPinmuxSet(GPIOA_6, PIN_FUNC3);
	xPinmuxSet(GPIOA_7, PIN_FUNC3);

	/* TODO: set spicc0 clktree */
	u32 val = REG32(CLKTREE_SPICC_CLK_CTRL);
	val &= ~0xfff;
	/* src [11~9]:  0 = div2(768M)
	 * gate   [8]:     1
	 * rate[7~ 0]:  4 = 768M/4 = 192000000
	 */
	val |= (0 << 9) | (1 << 8) | (4 << 0);
	REG32(CLKTREE_SPICC_CLK_CTRL) = val;

	xSpiccProbe(&pdata);
}
#endif /* end of ENABLE_MODULE_SPI */
/*
static void vTestTask(void *pvParameters)
{
	uint32_t i = 0;
	pvParameters = pvParameters;

	iprintf("start test task\n");

	for ( ;; )
	{
		iprintf("test task %d\n", i++);
		vTaskDelay(1000);
		if (i >= 60) {
			iprintf("freertos finish\n");
			vPortHaltSystem();
		}
	}
}
*/
/*-----------------------------------------------------------*/
int main( void )
{
	vPortRtosInfoUpdateStatus(eRtosStat_Initializing);
	/* Configure the clocks, UART and GPIO. */
	prvSetupHardware();
#if configEnable_Console
	xTaskCreate( vConsoleTask, "console", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 1, &console_thread );
#endif
	//xTaskCreate( vTestTask, "test", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 1, NULL);
	vPortRtosInfoUpdateStatus(eRtosStat_Working);
	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was insufficient heap to start the
	scheduler. */

	return 0;
}
/*-----------------------------------------------------------*/
static void prvSetupHardware( void )
{
	/* Enable the Serial*/
	vSerialInit(NULL);

	vSerialPutString(ConsoleSerial, "A53 FreeRTOS run....\n");
	portDISABLE_INTERRUPTS();
	plat_gic_init();

#ifdef ENABLE_MODULE_GPIO
	/* initialize gpio IRQ */
	vGpioIRQInit();
#endif

#if configEnable_Console
	vSerialRegister();
	vConsoleInit();
#endif

#if ENABLE_MODULE_SPI
	vSpicc0Init();
	vSpiSlaveTest();
	vSpiDeviceTest();
#endif

#if ENABLE_MODULE_AUDIO
	audio_pre_init();
#endif
	vIspInit();
}
/*-----------------------------------------------------------*/

#if configEnable_Console
static void vConsoleTask(void *pvParameters)
{
	unsigned char rxchar;
	int8_t cInputIndex = 0;
	BaseType_t xMoreDataToFollow;
	static char pcOutputString[ MAX_OUTPUT_LENGTH ];
	static char pcInputString[ MAX_INPUT_LENGTH ];

	pvParameters = pvParameters;

	iprintf("start console #");

	for ( ;; )
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		while ( xSerialGetChar(ConsoleSerial, &rxchar) == pdPASS)
		{
			if (rxchar == 13)
			{
				pcInputString[ cInputIndex ] = '\0';
				if (strlen(pcInputString) != 0) {
					iprintf("\n%s\n", pcInputString);

					do {
						xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
								pcInputString,
								pcOutputString,
								MAX_OUTPUT_LENGTH
								);
						iprintf("%s\n", pcOutputString);
					} while (xMoreDataToFollow != pdFALSE);
					cInputIndex = 0;
					memset(pcInputString, 0x00, MAX_INPUT_LENGTH);
				} else {
					iprintf("\n");
				}

				iprintf("start console #");
			} else if (rxchar == 8) {
				iprintf("\b");
				iprintf("%c", 32);
				iprintf("\b");
				if (cInputIndex > 0) {
					cInputIndex --;
					pcInputString[ cInputIndex ] = '\0';
				}
			} else {
				iprintf("%c", rxchar);
				if (cInputIndex < MAX_INPUT_LENGTH) {
					pcInputString[ cInputIndex ] = rxchar;
					cInputIndex ++;
				}
			}
		}
	}
}
#endif

void vApplicationIdleHook( void )
{
	iprintf("IdleHook\n");
}

void vMainAssertCalled( const char *pcFileName, uint32_t ulLineNumber )
{
	iprintf( "ASSERT!  Line %d of file %s\r\n", ulLineNumber, pcFileName );
	taskENTER_CRITICAL();
	for ( ;; );
}
#if defined(GUEST)
void vMainPrivilegeInit(void)
{
	vTickTimerInit();
	gicv2_reset();
}
#endif
