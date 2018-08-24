/******************************************************************************/
/** @file	C15_m4.c
    @date	2016-03-09 SSC
*******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sys/nl_coos.h"
#include "sys/delays.h"
#include "boards/emphase_v4.h"

#include "drv/nl_gpio.h"
#include "drv/nl_dbg.h"
#include "drv/nl_gpdma.h"
#include "drv/nl_kbs.h"

#include "usb/nl_usb_midi.h"
#include "cpu/nl_cpu.h"
#include "ipc/emphase_ipc.h"

#include "spibb/nl_spi_bb.h"
#include "spibb/nl_bb_msg.h"

#include "tcd/nl_tcd_valloc.h"
#include "tcd/nl_tcd_env.h"
#include "tcd/nl_tcd_adc_work.h"
#include "tcd/nl_tcd_poly.h"
#include "tcd/nl_tcd_expon.h"
#include "tcd/nl_tcd_param_work.h"
#include "tcd/nl_tcd_msg.h"


volatile uint8_t  waitForFirstSysTick = 1;


#if 0
void BbbCallback(uint16_t type, uint16_t length, uint16_t* data)
{
	uint32_t param = *data;
	uint32_t value1 = *(data + 1);

	uint32_t value2;

	if (length == 3)
	{
		value2 = *(data + 2);
	}

	if (type == BB_MSG_TYPE_PARAMETER)
	{
		if (length == 3)
		{
			PARAM_Set2(param, value1, value2);
		}
		else
		{
			PARAM_Set(param, value1);
		}
	}
	else if (type == BB_MSG_TYPE_SETTING)
	{
		if (param == 0)							//  Ribbon 1 Mode
		{
			ADC_WORK_SetRibbon1AsEditControl(value1);			// 0: Play Control, 1: Edit Control
		}
	}
}
# endif




void ToggleGpios(void)
{
	static uint8_t tgl = 0;

	if(tgl == 0)
	{
		SPI_BB_TestGpios(0);
		tgl = 1;
	}
	else
	{
		SPI_BB_TestGpios(1);
		tgl = 0;
	}
}




void Task_TestBbbLpc(void)
{
	BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_PARAMETER, 284	, 10);

	if (BB_MSG_SendTheBuffer() < 0)
	{
		DBG_Led_Error_On();
		COOS_Task_Add(DBG_Led_Error_Off,  1600,    0);
	}
}





void Init(void)
{
	/* board */

#ifdef C15_VERSION_4
    EMPHASE_V4_M4_Init();
#elif defined C15_VERSION_5
	EMPHASE_V5_M4_Init();
#else
	#error "No version defined."
#endif


    /* system */
    Emphase_IPC_M4_Init();
    NL_GPDMA_Init(0b11111100);

    /* debug */
    DBG_Init();
	DBG_Led_Error_Off();
    DBG_Led_Cpu_Off();
    DBG_Led_Warning_Off();
    DBG_Led_Usb_On();
    DBG_Pod_0_Off();
    DBG_Pod_1_Off();
    DBG_Pod_2_Off();
    DBG_Pod_3_Off();
    DBG_Pod_4_Off();
    DBG_Pod_5_Off();
    DBG_Pod_6_Off();
    DBG_Pod_7_Off();

    /* USB */
    USB_MIDI_Init();
    USB_MIDI_Config(NULL);

	volatile uint32_t timeOut = 0x0FFFFF;

	do  {
		USB_MIDI_Poll();
		timeOut--;
	}	while ((!USB_MIDI_IsConfigured()) && (timeOut > 0));

	if (USB_MIDI_IsConfigured() == TRUE)
		DBG_Led_Usb_Off();

    /* lpc bbb communication */
	SPI_BB_Init(BB_MSG_ReceiveCallback);

	/* TCD */
	EXPON_Init();
	VALLOC_Init();
	ENV_Init();
	POLY_Init();
	PARAM_WORK_Init();

	/* scheduler */
    COOS_Init();

    COOS_Task_Add(NL_GPDMA_Poll,    10,    1);	// every 125 us, for all the DMA transfers (SPI devices)
    COOS_Task_Add(USB_MIDI_Poll,    15,    1);	// every 125 us, same time grid as in USB 2.0

    COOS_Task_Add(VALLOC_Process,   20,    1);	// every 125 us, reading and applying keybed events
    COOS_Task_Add(ENV_Process,      25,    1);	// every 125 us, envelope processing

    COOS_Task_Add(SPI_BB_Polling,   30,    1);	// every 125 us, checking the buffer with messages from the BBB and driving the LPC-BB "heartbeat"

    COOS_Task_Add(ADC_WORK_Init, 	60,    0);	// preparing the ADC processing (will be executed after the M0 has been initialized)
    COOS_Task_Add(ADC_WORK_Process, 70,   80);	// every 10 ms, reading ADC values and applying changes, interleaved with SPI_BB_Polling

    COOS_Task_Add(DBG_Process,      80, 4800);
    COOS_Task_Add(MSG_CheckUSB,		90, 1600);	// ever 200 ms, checking if the USB connection to the ePC or the ePC is still working

    /* M0 */
    CPU_M0_Init();
	NVIC_SetPriority(M0CORE_IRQn, M0APP_IRQ_PRIORITY);
	NVIC_EnableIRQ(M0CORE_IRQn);
}



/******************************************************************************/

int main(void)
{
	Init();

    while(waitForFirstSysTick);

    while(1)
    {
    	COOS_Dispatch();
    }

    return 0 ;
}



void M0CORE_IRQHandler(void)
{
	LPC_CREG->M0TXEVENT = 0;

	waitForFirstSysTick = 0;
	COOS_Update();
}
