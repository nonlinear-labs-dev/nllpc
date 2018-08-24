/******************************************************************************/
/**	@file 	M0_IPC_Test.c
	@date	2016-03-01 DTZ
  	@author	2015-01-31 DTZ

	@brief	see the M4_IPC_Test.c file for the documentation

  	@note	!!!!!! USE optimized most -O3 for compiling !!!!!!

*******************************************************************************/
#include <stdint.h>
#include "cmsis/LPC43xx.h"

#include "sys/delays.h"
#include "boards/emphase_v4.h"
#include "ipc/emphase_ipc.h"

#include "drv/nl_rit.h"
#include "drv/nl_dbg.h"
#include "drv/nl_gpdma.h"
#include "drv/nl_kbs.h"

#include "espi/nl_espi_core.h"

#include "espi/dev/nl_espi_dev_aftertouch.h"
#include "espi/dev/nl_espi_dev_attenuator.h"
#include "espi/dev/nl_espi_dev_pedals.h"
#include "espi/dev/nl_espi_dev_pitchbender.h"
#include "espi/dev/nl_espi_dev_ribbons.h"
#include "espi/dev/nl_espi_dev_volpoti.h"

#define M0_SYSTICK_IN_NS			62500
#define M0_SYSTICK_MULTIPLIER  		2

#define PENDING_INTERRUPT 	1
#define SCHED_FINISHED 		0

#define M0_DEBUG	0

#define ESPI_MODE_ADC      LPC_SSP0, ESPI_CPOL_0 | ESPI_CPHA_0
#define ESPI_MODE_ATT_DOUT LPC_SSP0, ESPI_CPOL_0 | ESPI_CPHA_0
#define ESPI_MODE_DIN      LPC_SSP0, ESPI_CPOL_1 | ESPI_CPHA_1

static volatile uint8_t stateFlag = 0;

void SendInterruptToM4(void);



/******************************************************************************/
/** @note	Espi device functions do NOT switch mode themselves!
 	 	 	espi bus speed: 1.6 MHz -> 0.625 µs                    Bytes    t_µs
                                                     POL/PHA | multi | t_µs_sum
--------------------------------------------------------------------------------
P1D1   ADC-2CH     ribbon_board         MCP3202   R/W  0/1   3   2   15  30
P1D2   ADC-1CH     pitch_bender_board   MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P0D1   1CH-ADC     aftertouch_board     MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P2D2   ATTENUATOR  pedal_audio_board    LM1972    W    0/0   2   2   10  20
--------------------------------------------------------------------------------
P3D1   ADC-1CH     volume_poti_board    MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P4D1   ADC-8CH     pedal_audio_board    MCP3208   R/W  0/1   3   4   15  60
P4D2   DETECT      pedal_audio_board    HC165     R    1/1   1   1    5   5
P4D3   SET_PULL_R  pedal_audio_board    HC595     W    0/0   1   1    5   5
--------------------------------------------------------------------------------
                                                                13      165
*******************************************************************************/

void Scheduler(void)
{

#if M0_DEBUG
	DBG_Pod_3_On();
#endif

	static uint8_t state = 0;

	switch (state)
	{
		case 1:		// keybed scanner: 51.6 µs best case - 53.7 µs worst case
		case 3:
		case 5:
		case 7:
		case 9:
		case 11:
		case 13:
		case 15:
		case 17:
		case 19:
		case 21:
		case 23:
		{
			KBS_Process();
			break;
		}

		case 0:		// switch mode: 13.6 µs
		{
			SPI_DMA_SwitchMode(ESPI_MODE_ADC);
			NL_GPDMA_Poll();
			break;
		}

		case 2:		// pedal 1 : 57 µs
		{
			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_1_ADC_RING);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_ADC_RING,  ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_1_ADC_RING));

			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_1_ADC_TIP);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_ADC_TIP, ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_1_ADC_TIP));
			break;
		}

		case 4:		// pedal 2 : 57 µs
		{
			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_2_ADC_RING);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_ADC_RING,  ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_2_ADC_RING));

			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_2_ADC_TIP);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_ADC_TIP, ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_2_ADC_TIP));
			break;
		}

		case 6:		// pedal 3 : 57 µs
		{
			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_3_ADC_RING);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_ADC_RING,  ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_3_ADC_RING));

			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_3_ADC_TIP);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_ADC_TIP, ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_3_ADC_TIP));
			break;
		}

		case 8:		// pedal 4 : 57 µs
		{
			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_4_ADC_RING);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_ADC_RING,  ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_4_ADC_RING));

			ESPI_DEV_Pedals_EspiPullChannel_Blocking(EMPHASE_IPC_PEDAL_4_ADC_TIP);
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_ADC_TIP, ESPI_DEV_Pedals_GetValue(EMPHASE_IPC_PEDAL_4_ADC_TIP));
			break;
		}

		case 10:	// detect pedals: 32.5 µs
		{
			SPI_DMA_SwitchMode(ESPI_MODE_DIN);
			NL_GPDMA_Poll();

			ESPI_DEV_Pedals_Detect_EspiPull();
			NL_GPDMA_Poll();

			uint8_t detect = ESPI_DEV_Pedals_Detect_GetValue();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_DETECT, ((detect & 0b00010000) >> 4));
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_DETECT, ((detect & 0b00100000) >> 5));
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_DETECT, ((detect & 0b01000000) >> 6));
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_DETECT, ((detect & 0b10000000) >> 7));
			break;
		}

		case 12:	// set pull resistors: best case: 17.3 µs - worst case: 36 µs
		{
			SPI_DMA_SwitchMode(ESPI_MODE_ATT_DOUT);
			NL_GPDMA_Poll();

#ifdef C15_VERSION_4
			uint8_t pullResistorsValue = 0;
			pullResistorsValue  =  (uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_PULLR_TO_TIP);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_PULLR_TO_RING) << 1);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_PULLR_TO_TIP)  << 2);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_PULLR_TO_RING) << 3);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_PULLR_TO_TIP)  << 4);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_PULLR_TO_RING) << 5);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_PULLR_TO_TIP)  << 6);
			pullResistorsValue |= ((uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_PULLR_TO_RING) << 7);
			ESPI_DEV_Pedals_PullResistors_SetValue(pullResistorsValue);
#elif defined C15_VERSION_5
			ESPI_DEV_Pedals_SetPedalState( 1, (uint8_t)Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_STATE) );
			ESPI_DEV_Pedals_SetPedalState( 2, (uint8_t)Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_STATE) );
			ESPI_DEV_Pedals_SetPedalState( 3, (uint8_t)Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_STATE) );
			ESPI_DEV_Pedals_SetPedalState( 4, (uint8_t)Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_STATE) );
#else
	#error "No version defined."
#endif

			ESPI_DEV_Pedals_PullResistors_EspiSendIfChanged();

			NL_GPDMA_Poll();
			break;
		}

		case 14:	// attenuator: best case: 4.9 µs, worst case: 44.8 µs
		{
			uint8_t attenuatorValue = (uint8_t) Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_ATTENUATOR_ADC);
			ESPI_Attenuator_Channel_Set(0, attenuatorValue);
			ESPI_Attenuator_Channel_Set(1, attenuatorValue);

			ESPI_Attenuator_EspiSendIfChanged(0);
			NL_GPDMA_Poll();

			ESPI_Attenuator_EspiSendIfChanged(1);
			NL_GPDMA_Poll();
			break;
		}

		case 16:	// pitchbender: 42 µs
		{
			SPI_DMA_SwitchMode(ESPI_MODE_ADC);
			NL_GPDMA_Poll();

			ESPI_DEV_Pitchbender_EspiPull();
			NL_GPDMA_Poll();

			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PITCHBENDER_ADC, ESPI_DEV_Pitchbender_GetValue());
			break;
		}

		case 18:	// aftertouch: 29 µs
		{
			ESPI_DEV_Aftertouch_EspiPull();
			NL_GPDMA_Poll();

			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_AFTERTOUCH_ADC, ESPI_DEV_Aftertouch_GetValue());
			break;
		}

		case 20:	// 2 ribbons: 57 µs
		{
			ESPI_DEV_Ribbons_EspiPull_Upper();
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_RIBBON_1_ADC, ESPI_DEV_Ribbons_GetValue(UPPER_RIBBON));

			ESPI_DEV_Ribbons_EspiPull_Lower();
			NL_GPDMA_Poll();
			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_RIBBON_2_ADC, ESPI_DEV_Ribbons_GetValue(LOWER_RIBBON));
			break;
		}

		case 22:	// volume poti: 29 µs
		{
			ESPI_DEV_VolPoti_EspiPull();
			NL_GPDMA_Poll();

			Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_VOLUME_POTI_ADC, ESPI_DEV_VolPoti_GetValue());

			static uint32_t hbLedCnt = 0;
			hbLedCnt++;

			switch(hbLedCnt)
			{
				case 1:
					DBG_Led_Cpu_On();
					break;
				case 201:
					DBG_Led_Cpu_Off();
					break;
				case 400:
					hbLedCnt = 0;
					break;
				default:
					break;
			}

			break;
		}

		default:
			break;
	}

	state++;
	if (state == 24)
		state = 0;

	stateFlag = SCHED_FINISHED;

#if M0_DEBUG
	DBG_Pod_3_Off();
#endif

}




/******************************************************************************/

int main(void)
{
#ifdef C15_VERSION_4
	EMPHASE_V4_M0_Init();
#elif defined C15_VERSION_5
	EMPHASE_V5_M0_Init();
#else
	#error "No version defined."
#endif


	Emphase_IPC_M0_Init();

	NL_GPDMA_Init(0b00000011);		/// inverse to the mask in the M4_Main

	ESPI_Init(1600000);

	ESPI_DEV_VolPoti_Init();
	ESPI_DEV_Pedals_Init();
	ESPI_DEV_Aftertouch_Init();
	ESPI_DEV_Attenuator_Init();
	ESPI_DEV_Pitchbender_Init();
	ESPI_DEV_Ribbons_Init();

	RIT_Init_IntervalInNs(M0_SYSTICK_IN_NS);

	while(1)
	{
		if (stateFlag == PENDING_INTERRUPT)
		{
			Scheduler();
		}
	}

	return 0;
}



/******************************************************************************/
void M0_RIT_OR_WWDT_IRQHandler(void)
{
	RIT_ClearInt();

	static uint8_t sysTickMultiplier = 0;
	sysTickMultiplier++;

	if (sysTickMultiplier == M0_SYSTICK_MULTIPLIER)
	{
		SendInterruptToM4();
	}

	if (sysTickMultiplier == 2)
		sysTickMultiplier = 0;

	if (stateFlag == SCHED_FINISHED)
	{
		stateFlag = PENDING_INTERRUPT;
	}
	else
	{
		DBG_Led_Warning_On();
	}
}



/******************************************************************************/
void SendInterruptToM4(void)
{
	__DSB();
	__SEV();
}
