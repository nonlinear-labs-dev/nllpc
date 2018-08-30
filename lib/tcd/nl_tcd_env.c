/******************************************************************************/
/** @file		nl_tcd_gen.c
    @date		2013-04-22
    @version	0.02
    @author		Stephan Schmitt[2012-06-14]
    @brief		<tbd>
	@note		fka mmgen 
	@ingroup	nl_tcd_modules
*******************************************************************************/

#include "math.h"

#include "nl_tcd_env.h"

#include "nl_tcd_expon.h"
#include "nl_tcd_msg.h"
#include "nl_tcd_param_work.h"
#include "nl_tcd_valloc.h"


/*******************************************************************************
	modul local defines
*******************************************************************************/

#define TIME_UNITS_PER_TICK  6   						// SysTick_time (125 us)  is 6 times  TIME_UNIT_MS (20.8333 us = 1/48000 Hz)


#define TICKS_BEFORE_FADE_OUT	200						// Ticks to wait before fade-out & flush (= 25 ms)
#define TICKS_BEFORE_FADE_IN 	300						// Ticks to wait before fade-in          (= 37.5 ms)


/*******************************************************************************
	modul local variables
*******************************************************************************/



static uint32_t waitForFadeOutCount;
static uint32_t waitForFadeInCount;


/******************************************************************************/
/**	@brief  Inits the conversion tables: velocity, exp_dB, exp_semitone
*******************************************************************************/

void ENV_Init(void)
{
	waitForFadeOutCount = 0;
	waitForFadeInCount = 0;
}



/******************************************************************************/
/**	@brief 	This function is called with every timer interval (125 us).
 * 			It generates the timed breakpoints.
			Only the Attack segment and the Decay 1 segment have a timed
			duration.
*******************************************************************************/

void ENV_Process(void)
{
	/// ...

	MSG_SendMidiBuffer();

	if (waitForFadeOutCount)
	{
		waitForFadeOutCount--;

		if (waitForFadeOutCount == 0)
		{
			/// fade-out and start flushing

			waitForFadeInCount = TICKS_BEFORE_FADE_IN;
		}
	}

	if (waitForFadeInCount)
	{
		waitForFadeInCount--;

		if (waitForFadeInCount == 0)
		{
			/// Apply + fade-in
		}
	}
}



/******************************************************************************
	@brief  Is used by param_work.c to delay the fade-in after preset recall
*******************************************************************************/

void ENV_StartWaitForFadeOut(void)
{
	waitForFadeOutCount = TICKS_BEFORE_FADE_OUT;
}
