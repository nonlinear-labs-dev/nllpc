/******************************************************************************/
/** @file		lpc4357_evb.c
    @date		2015-10-12 NNI
    @author		2015-10-12 NNI
*******************************************************************************/
#include <stdint.h>
#include "drv/nl_pin.h"
#include "cmsis/lpc43xx_cgu.h"

static void ConfigureClocks(void);
static void Delay100(void);
static void InitI2SPins(void);
static void InitUSBPins(void);
static void InitGPIOs(void);

/******************************************************************************/
/** @brief    	Init function for the M4 core
*******************************************************************************/
void LPC4357_EVB_M4_Init(void)
{
	ConfigureClocks();
	InitI2SPins();
	InitUSBPins();
	InitGPIOs();
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

	/* connect APB1 clock (I2S and I2C) to PLL1 */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_APB1);
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_APB3);
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

static PIN_CFG_T led_3v3 = {
	.pinId  		= {0xE,7},
	.ioType 		= PIN_TYPE_GPIO,
	.gpioId 		= {7,7},
	.direction		= PIN_GPIO_DIR_OUT,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_SLOW,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 4
};

/*******************************************************************************
	I2S pin init
*******************************************************************************/
static PIN_CFG_T lpc_i2s_rx_sda = {
	.pinId  		= {3,2},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_ON,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 1
};

static PIN_CFG_T lpc_i2s_rx_sck = {
	.pinId  		= {3,0},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,			// OFF?
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 1
};
static PIN_CFG_T lpc_i2s_rx_ws = {
	.pinId  		= {3,1},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,			// OFF?
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 1
};

static PIN_CFG_T lpc_i2s_tx_sck = {
	.pinId  		= {4,7},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static PIN_CFG_T lpc_i2s_tx_ws = {
	.pinId  		= {0xC,13},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 6
};

static PIN_CFG_T lpc_i2s_tx_sda = {
	.pinId  		= {0xC,12},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 6
};

static PIN_CFG_T lpc_usb0_pwr_en = {
	.pinId  		= {2,3},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_OFF,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static PIN_CFG_T lpc_usb0_pwr_fault = {
	.pinId  		= {2,4},
	.ioType 		= PIN_TYPE_PIN,
	.inputBuffer	= PIN_INBUF_ON,
	.glitchFilter 	= PIN_FILTER_ON,
	.slewRate 		= PIN_SRATE_FAST,
	.pullDown 		= PIN_PDN_OFF,
	.pullUp 		= PIN_PUP_OFF,
	.function		= 7
};

static void InitI2SPins(void)
{
	PIN_Config(&lpc_i2s_rx_sda);
	PIN_Config(&lpc_i2s_rx_sck);
	PIN_Config(&lpc_i2s_rx_ws);
	PIN_Config(&lpc_i2s_tx_sck);
	PIN_Config(&lpc_i2s_tx_ws);
	PIN_Config(&lpc_i2s_tx_sda);
}

static void InitUSBPins(void)
{
	PIN_Config(&lpc_usb0_pwr_en);
	PIN_Config(&lpc_usb0_pwr_fault);
}

void InitGPIOs(void)
{
	PIN_Config(&led_3v3);
}
