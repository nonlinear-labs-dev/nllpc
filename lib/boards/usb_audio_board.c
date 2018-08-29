/******************************************************************************/
/** @file		usb_audio_board.c
    @date		2015-06-23 NNI
    @author		2015-06-23 NNI
*******************************************************************************/

#include <stdint.h>
#include "drv/nl_pin.h"
#include "cmsis/lpc43xx_cgu.h"

static void ConfigureClocks(void);
static void Delay100(void);
static void InitI2SPins(void);
static void InitMidiPins(void);

/******************************************************************************/
/** @brief    	Init function for the M4 core
*******************************************************************************/
void USB_AUDIO_BOARD_M4_Init(void)
{
	ConfigureClocks();
	InitI2SPins();
	InitMidiPins();
}

/**********************************************************************
 * @brief		Sets the PLL0 Audio to produce stable 12.288 MHz
 **********************************************************************/
static void USB_AUDIO_BOARD_Set_PLL0_Audio(void)
{
	LPC_CGU->PLL0AUDIO_CTRL = (6 << 24)   /* source = XTAL OSC 12 MHz */
	                      | _BIT(0); /* power down */
	/* PLL should be set to 256fs rate 256 * 48000 =  12288000 Hz */
	LPC_CGU->PLL0AUDIO_MDIV = 0x0A69;
	LPC_CGU->PLL0AUDIO_NP_DIV = 0x3F018;
	LPC_CGU->PLL0AUDIO_CTRL = (6 << 24)   /* source = XTAL OSC 12 MHz */
					  | (6<< 12)		  // fractional divider off and bypassed
					  | _BIT(4);   /* CLKEN */

	/* wait for lock */
	while (!(LPC_CGU->PLL0AUDIO_STAT & 1));
}

/******************************************************************************/
/** @brief    	configures the clocks of the whole system
	@details	The main cpu clock should set to 204 MHz. According to the
				datasheet, therefore the clock has to be increased in two steps.
				1. low frequency range to mid frequency range
				2. mid frequency range to high frequency range
				clock ranges:
				    - low		f_cpu: < 90 MHz
				    - mid		f_cpu: 90 MHz to 110 MHz
				  	- high		f_cpu: up to 204 MHz
				settings (f: frequency, v: value):
					- f_osc  =  12 MHz
					- f_pll1 = 204 MHz		v_pll1 = 17
					- f_cpu  = 204 MHz
*******************************************************************************/
static void ConfigureClocks(void)
{
	/* XTAL OSC */
	CGU_SetXTALOSC(12000000);													// set f_osc = 12 MHz (external XTAL OSC is a 12 MHz device)
	CGU_EnableEntity(CGU_CLKSRC_XTAL_OSC, ENABLE);								// Enable xtal osc clock entity as clcok source
	Delay100();																	// delay about 100 µs
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_CLKSRC_PLL1);					// connect XTAL to PLL1

	/* STEP 1: set cpu to mid frequency (according to datasheet) */
	CGU_SetPLL1(8);																// f_osc x 8 = 96 MHz
	CGU_EnableEntity(CGU_CLKSRC_PLL1, ENABLE);									// Enable PLL1 after setting is done
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_M4);
	CGU_UpdateClock();

	/* STEP 2: set cpu to high frequency */
	CGU_SetPLL1(17);															// set PLL1 to: f_osc x 17 = 204 MHz
	CGU_UpdateClock();															// Update Clock Frequency

	/* connect USB0 to PLL0 which is set to 480 MHz */
	CGU_EnableEntity(CGU_CLKSRC_PLL0, DISABLE);
	CGU_SetPLL0();
	CGU_EnableEntity(CGU_CLKSRC_PLL0, ENABLE);
	CGU_UpdateClock();
	CGU_EntityConnect(CGU_CLKSRC_PLL0, CGU_BASE_USB0);

	/* connect PLL0 Audio clock to I2S */
	CGU_EnableEntity(CGU_CLKSRC_PLL0_AUDIO, DISABLE);
	USB_AUDIO_BOARD_Set_PLL0_Audio();
	CGU_EnableEntity(CGU_CLKSRC_PLL0_AUDIO, ENABLE);
	CGU_UpdateClock();
	CGU_EntityConnect(CGU_CLKSRC_PLL0_AUDIO, CGU_BASE_APLL);
	CGU_EntityConnect(CGU_CLKSRC_PLL0_AUDIO, CGU_BASE_CLKOUT);

	/* connect APB1 clock (I2S and I2C) to PLL1 */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_APB1);
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_APB3);
	CGU_UpdateClock();

	/* connect UART0 to PLL1 */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_UART0);
	CGU_UpdateClock();

}

/******************************************************************************/
/** @brief  	modul internal delay function - waits at least 100 µs @ 180 MHz
				with lower frequencies it waits longer
*******************************************************************************/
static void Delay100(void)
{
	uint32_t cnt = 20000;
	for (; cnt > 0; cnt--);
}

/*******************************************************************************
	I2S pin init
*******************************************************************************/
static PIN_CFG_T lpc_i2s_rx_sda = {
	.pinId  		= {3,4},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_ON,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 6
};

static PIN_CFG_T lpc_i2s_tx_sck = {
	.pinId  		= {3,3},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static PIN_CFG_T lpc_i2s_tx_ws = {
	.pinId  		= {0,0},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static PIN_CFG_T lpc_i2s_tx_sda = {
	.pinId  		= {0,1},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static PIN_CFG_T lpc_uart0_rx = {
	.pinId			= {6,5},
	.ioType			= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_ON,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 2
};

static void InitI2SPins(void)
{
	PIN_Config(&lpc_i2s_rx_sda);
	PIN_Config(&lpc_i2s_tx_sck);
	PIN_Config(&lpc_i2s_tx_ws);
	PIN_Config(&lpc_i2s_tx_sda);

	LPC_SCU->SFSCLK_0 = PIN_PUP_OFF | PIN_SRATE_FAST | 0x1;
}

static void InitMidiPins(void)
{
	PIN_Config(&lpc_uart0_rx);
}
