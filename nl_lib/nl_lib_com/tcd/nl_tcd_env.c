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

#define ATTACK_SEGMENT  0
#define DECAY_1_SEGMENT 1
#define DECAY_2_SEGMENT 2
#define RELEASE_SEGMENT 3

#define TICKS_BEFORE_FADE_OUT	200						// Ticks to wait before fade-out & flush (= 25 ms)
#define TICKS_BEFORE_FADE_IN 	300						// Ticks to wait before fade-in          (= 37.5 ms)


/*******************************************************************************
	modul local variables
*******************************************************************************/

static int32_t voicePitch[NUM_VOICES] = {};							// pitch for key-tracking, is set by poly.c

static uint32_t envA_segment[NUM_VOICES] = {};						// index of the current segments of Envelope A
static uint32_t envB_segment[NUM_VOICES] = {};						// index of the current segments of Envelope B
static uint32_t envC_segment[NUM_VOICES] = {};						// index of the current segments of Envelope C
static int32_t envA_timeCount[NUM_VOICES] = {};						// can have negative values because we decrement until <= 0
static int32_t envB_timeCount[NUM_VOICES] = {};						// can have negative values because we decrement until <= 0
static int32_t envC_timeCount[NUM_VOICES] = {};						// can have negative values because we decrement until <= 0

static int32_t envA_timeKTOffset[NUM_VOICES] = {};					// offset for all logarithmic times (A, D1, D2, R) of Envelope A, resulting from the key tracking
static int32_t envB_timeKTOffset[NUM_VOICES] = {};					// offset for all logarithmic times (A, D1, D2, R) of Envelope B, resulting from the key tracking
static int32_t envC_timeKTOffset[NUM_VOICES] = {};					// offset for all logarithmic times (A, D1, D2, R) of Envelope C, resulting from the key tracking
static uint32_t envA_levelFactor[NUM_VOICES] = {};					// factor for all levels (Peak, B, S) of Envelope A, resulting from the key tracking and velocity
static uint32_t envB_levelFactor[NUM_VOICES] = {};					// factor for all levels (Peak, B, S) of Envelope B, resulting from the key tracking and velocity
static int32_t envC_levelFactor[NUM_VOICES] = {};					// factor for all levels (Peak, B, S) of Envelope C, resulting from the key tracking and velocity

static uint32_t expTable_dB[85] = {};								// level (dB) to amplitude
																	// element  0: 	  0 dB 	= 1			(16384)	(2^14)
																	// element 84:  -84 dB	= 1/16384	    (1)	(2^0)

static uint32_t expTableSemitone[121] = {}; 						// pitch (semitone) to frequency/time
																	// element   0: -5 octaves 	= 1/32	   (32) (2^5)
																	// element  60:  0			= 1		 (1024) (2^10)
																	// element 120: +5 octaves 	= 32	(32768) (2^15)
																	// 0...120 - 61-key keyboard transposed by max. -3/+2 octaves


// Envelope A parameters																on the UI

static int32_t envA_attackTime;										// 0 ... 16000		[0.1 ... 16000 ms, log scaled]
static int32_t envA_attackCurvature;								// -8000 ... 8000
static int32_t envA_decay1Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static uint32_t envA_breakpointLevel;								// 0 ... 16000		[0 ... 100 %]
static int32_t envA_decay2Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static uint32_t envA_sustainLevel;									// 0 ... 16000		[0 ... 100 %]
static int32_t envA_releaseTime;									// 0 ... 16160		[1 ... 160000 ms, log scaled + inf]

static uint32_t envA_levelVelocity;									// 0 ... 15360		[0 ... 60 dB]
static uint32_t envA_attackVelocity;								// 0 ... 12000		[0 ... 60 dB]
static uint32_t envA_releaseVelocity;								// 0 ... 12000		[0 ... 60 dB]
static int32_t envA_levelKeyTrk;									// -8000 ... 8000	[-1.0 ... 1.0 dB/st]
static int32_t envA_timeKeyTrk;										// 0 ... 16000		[0 ... 100 %, -6 dB_T/oct, 0.5 dB_T/st]


// Envelope B parameters

static int32_t envB_attackTime;										// 0 ... 16000		[0.1 ... 16000 ms, log scaled]
static int32_t envB_attackCurvature;								// -8000 ... 8000
static int32_t envB_decay1Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static uint32_t envB_breakpointLevel;								// 0 ... 16000		[0 ... 100 %]
static int32_t envB_decay2Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static uint32_t envB_sustainLevel;									// 0 ... 16000		[0 ... 100 %]
static int32_t envB_releaseTime;									// 0 ... 16160		[1 ... 160000 ms, log scaled + inf]

static uint32_t envB_levelVelocity;									// 0 ... 15360		[0 ... 60 dB]
static uint32_t envB_attackVelocity;								// 0 ... 12000		[0 ... 60 dB]
static uint32_t envB_releaseVelocity;								// 0 ... 12000		[0 ... 60 dB]
static int32_t envB_levelKeyTrk;									// -8000 ... 8000	[-1.0 ... 1.0 dB/st]
static int32_t envB_timeKeyTrk;										// 0 ... 16000		[0 ... 100 %, -6 dB_T/oct, 0.5 dB_T/st]


// Envelope C parameters

static int32_t envC_attackTime;										// 0 ... 16000		[0.1 ... 16000 ms, log scaled]
static int32_t envC_attackCurvature;								// -8000 ... 8000
static int32_t envC_decay1Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static int32_t envC_breakpointLevel;								// -8000 ... 8000	[-100 ... 100 %]
static int32_t envC_decay2Time;										// 0 ... 16000		[1 ... 160000 ms, log scaled]
static int32_t envC_sustainLevel;									// -8000 ... 8000	[-100 ... 100 %]
static int32_t envC_releaseTime;									// 0 ... 16160		[1 ... 160000 ms, log scaled + inf]

static uint32_t envC_levelVelocity;									// 0 ... 15360		[0 ... 60 dB]
static uint32_t envC_attackVelocity;								// 0 ... 12000		[0 ... 60 dB]
static uint32_t envC_releaseVelocity;								// 0 ... 12000		[0 ... 60 dB]
static int32_t envC_levelKeyTrk;									// -8000 ... 8000	[-1.0 ... 1.0 dB/st]
static int32_t envC_timeKeyTrk;										// 0 ... 16000		[0 ... 100 %, -6 dB_T/oct, 0.5 dB_T/st]

static uint32_t unisonVoices;

static uint32_t waitForFadeOutCount;
static uint32_t waitForFadeInCount;


/******************************************************************************/
/**	@brief  Init_ExpTable_dB
 			index: 0...84 [dB damping]
			output: 16384 (0 dB) ... 1 (-84 dB)
*******************************************************************************/

static void Init_ExpTable_dB(void)
{
	uint32_t i;

	for (i = 0; i < 85; i++)
	{
		float exp = -(i / 20.0);
		expTable_dB[i] = (uint32_t)(16384.0 * pow(10.0, exp) + 0.5);
	}
}



/******************************************************************************/
/**	@brief  Init_ExpTableSemitone
 *          Double/half per octave, 0.5017 dB/st, 1024 at 60
			index: 0...60...120 [semitones]
			output: (2^-5 ... 1 ... 2^5) * 1024  =  32...1024...32768
*******************************************************************************/

static void Init_ExpTableSemitone(void)
{
	uint32_t i;

	for (i = 0; i < 121; i++)
	{
		float exp = (i - 60.0) / 12.0;
		expTableSemitone[i] = (uint32_t)(1024.0 * pow(2.0, exp) + 0.5);
	}
}



/******************************************************************************/
/**	@brief  Inits the conversion tables: velocity, exp_dB, exp_semitone
*******************************************************************************/

void ENV_Init(void)
{
	uint32_t v;

	for (v = 0; v < NUM_VOICES; v++)
	{
		envA_segment[v] = ATTACK_SEGMENT;
		envB_segment[v] = ATTACK_SEGMENT;
		envC_segment[v] = ATTACK_SEGMENT;
	}

	Init_ExpTable_dB();
	Init_ExpTableSemitone();

	unisonVoices = 1;

	waitForFadeOutCount = 0;
	waitForFadeInCount = 0;
}



/*****************************************************************************/
/**	@brief  	ENV_SetVoicePitch: is called when the pitch of a voice changes
	@param[in]	v: id of the voice (0 ... NUM_VOICES - 1)
	@param[in]  pitch: pitch of the voice (in 1/1000 semitones,  C3: 0)
******************************************************************************/

void ENV_SetVoicePitch(uint32_t v, int32_t pitch)
{
	if (pitch < -60000)			// limiting the range to 10 octaves
	{
		voicePitch[v] = -60000;
	}
	else if (pitch > 60000)
	{
		voicePitch[v] = 60000;
	}
	else
	{
		voicePitch[v] = pitch;
	}
}



/*****************************************************************************/
/**	@brief  	ENV_Start: is called when a key is pressed
	@param[in]	v: id of the voice (0 ... NUM_VOICES)
	@param[in]  vel: on velocity (0 ... 4096)
******************************************************************************/

void ENV_Start(uint32_t v, uint32_t vel)
{
	uint32_t lVel = 4096 - vel;							// 4096 - (0...4096), will be used for level velocities of all envelopes

	//============= Envelope A =============

	MSG_SelectParameter(PARAM_ID_ENV_A);

	//--- Time KeyTrk - depending on envA_timeKeyTrk all envelope times are shortened with rising key positions

	int32_t var;

	var = -(voicePitch[v] * envA_timeKeyTrk) / 207454;							// ((-60000...60000) * (0...16000) / 207454
																				// = -4628...0...4628 (+/-60 dB_T)
	envA_timeKTOffset[v] = var;													// the value will be used for the other times, too

	//--- Attack Velocity - the influence of the velocity on the attack time

    var -= (vel * ((envA_attackVelocity * 12593 ) >> 14)) >> 12;				// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
    																			// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
    var = EXPON_Time(envA_attackTime + var);

    envA_timeCount[v] = var;

	MSG_SetTime(var);															// sends the Attack time

	MSG_SetCurveType(CURVE_TYPE_POLYNOM + SEGMENT_PRIO_INSERT_CLOSED);			// sends the Attack curve type (= polynominal)
	MSG_SetCurvature(envA_attackCurvature);										// sends the Attack curvature

	//--- Level Velocity - the influence of the velocity on the envelope levels

	uint32_t levelVel = (lVel * envA_levelVelocity) >> 14;						// (4096...0) * (0...15360) / 16384  =>  3840...0  (64 pro dB; 60 dB = 3840)

	uint32_t fract = levelVel & 0x3F;														// lower 6 bits: 0...63
	uint32_t index = levelVel >> 6;															// upper 6 bits: 0...60  =  0 dB ... -60 dB (expTable_dB is invers)
	levelVel = (expTable_dB[index] * (64 - fract) + expTable_dB[index + 1] * fract) >> 6;	// (16...16384) * 64 / 64

	//--- Level KeyTrk - the influence of the key position on the envelope levels

	var = 60 * 256 + ((voicePitch[v] * envA_levelKeyTrk) / 16000);				// 60 * 256 + ((-60000...60000) * (-8000...8000) / 16000
																				// maximum KT: (500/256) * 0.5017 dB/st = 0.98 dB/st		/// warum nicht 1.0 ???
	if (var <= 0)							// clipping at -30.1 dB
	{
		var = expTableSemitone[0];
	}
	else if (var >= 120 * 256)				// clipping at +30.1 dB
	{
		var = expTableSemitone[120];
	}
	else
	{
		fract = var & 0xFF;																				// lower 8 bits: 0...255
		index = var >> 8;																				// 0...119 [semitone]
		var = (expTableSemitone[index] * (256 - fract) + expTableSemitone[index + 1] * fract) >> 8;		// (...1024...) * 256 / 256
	}

	var = (var * levelVel) >> 12;												// ((32...1024...32768) * (16...16384)) / 4096
	envA_levelFactor[v] = var;													// = 0...4096...131072  -  will be used for the breakpoint and sustain levels
	var = (16000 * var) >> 12;													// (16000 * (0...4096...131072)) / 4096
																				// 16000 is the normalized peak level (no velocity and KT influence)
	MSG_SetDestination(var);													// sends the peak level

	envA_segment[v] = ATTACK_SEGMENT;


	//============= Envelope B =============

	MSG_SelectParameter(PARAM_ID_ENV_B);

	//--- Time KeyTrk - depending on envB_timeKeyTrk all envelope times are shortened with rising key positions

	var = -(voicePitch[v] * envB_timeKeyTrk) / 207454;							// ((-60000...60000) * (0...16000) / 207454
																				// = -4628...0...4628 (+/-60 dB_T)

	envB_timeKTOffset[v] = var;													// the value will be used for the other times, too

	//--- Attack Velocity - the influence of the velocity on the attack time

    var -= (vel * ((envB_attackVelocity * 12593 ) >> 14)) >> 12;				// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
    																			// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
    var = EXPON_Time(envB_attackTime + var);

    envB_timeCount[v] = var;

	MSG_SetTime(var);															// sends the Attack time

	MSG_SetCurveType(CURVE_TYPE_POLYNOM + SEGMENT_PRIO_INSERT_CLOSED);			// sends the Attack curve type (= polynominal)
	MSG_SetCurvature(envB_attackCurvature);										// sends the Attack curvature

	//--- Level Velocity - the influence of the velocity on the envelope levels

	levelVel = (lVel * envB_levelVelocity) >> 14;								// (4096...0) * (0...15360) / 16384  =>  3840...0  (64 pro dB; 60 dB = 3840)

	fract = levelVel & 0x3F;																// lower 6 bits: 0...63
	index = levelVel >> 6;																	// upper 6 bits: 0...60  =  0 dB ... -60 dB (expTable_dB is invers)
	levelVel = (expTable_dB[index] * (64 - fract) + expTable_dB[index + 1] * fract) >> 6;	// (16...16384) * 64 / 64

	//--- Level KeyTrk - the influence of the key position on the envelope levels

	var = 60 * 256 + ((voicePitch[v] * envB_levelKeyTrk) / 16000);				// 60 * 256 + ((-60000...60000) * (-8000...8000) / 16000
																				// maximum KT: (500/256) * 0.5017 dB/st = 0.98 dB/st
	if (var <= 0)							// clipping at -30.1 dB
	{
		var = expTableSemitone[0];
	}
	else if (var >= 120 * 256)				// clipping at +30.1 dB
	{
		var = expTableSemitone[120];
	}
	else
	{
		fract = var & 0xFF;																				// lower 8 bits: 0...255
		index = var >> 8;																				// 0...119 [semitone]
		var = (expTableSemitone[index] * (256 - fract) + expTableSemitone[index + 1] * fract) >> 8;		// (...1024...) * 256 / 256
	}

	var = (var * levelVel) >> 12;												// ((32...1024...32768) * (16...16384)) / 4096
	envB_levelFactor[v] = var;													// = 0...4096...131072  -  will be used for the breakpoint and sustain levels
	var = (16000 * var) >> 12;													// (16000 * (0...4096...131072)) / 4096
																				// 16000 is the normalized peak level (no velocity and KT influence)
	MSG_SetDestination(var);													// sends the peak level

	envB_segment[v] = ATTACK_SEGMENT;


	//============= Envelope C =============

	MSG_SelectParameter(PARAM_ID_ENV_C);

	//--- Time KeyTrk - depending on envB_timeKeyTrk all envelope times are shortened with rising key positions

	var = -(voicePitch[v] * envC_timeKeyTrk) / 207454;							// ((-60000...60000) * (0...16000) / 207454
																				// = -4628...0...4628 (+/-60 dB_T)

	envC_timeKTOffset[v] = var;													// the value will be used for the other times, too

	//--- Attack Velocity - the influence of the velocity on the attack time

    var -= (vel * ((envC_attackVelocity * 12593 ) >> 14)) >> 12;				// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
    																			// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
    var = EXPON_Time(envC_attackTime + var);

    envC_timeCount[v] = var;

	MSG_SetTime(var);															// sends the Attack time

	MSG_SetCurveType(CURVE_TYPE_POLYNOM + SEGMENT_PRIO_INSERT_CLOSED);			// sends the Attack curve type (= polynominal)
	MSG_SetCurvature(envC_attackCurvature);										// sends the Attack curvature

	//--- Level Velocity - the influence of the velocity on the envelope levels

	levelVel = (lVel * envC_levelVelocity) >> 14;								// (4096...0) * (0...15360) / 16384  =>  3840...0  (64 pro dB; 60 dB = 3840)

	fract = levelVel & 0x3F;																// lower 6 bits: 0...63
	index = levelVel >> 6;																	// upper 6 bits: 0...60  =  0 dB ... -60 dB (expTable_dB is invers)
	levelVel = (expTable_dB[index] * (64 - fract) + expTable_dB[index + 1] * fract) >> 6;	// (16...16384) * 64 / 64

	//--- Level KeyTrk - the influence of the key position on the envelope levels

	var = 60 * 256 + ((voicePitch[v] * envC_levelKeyTrk) / 16000);				// 60 * 256 + ((-60000...60000) * (-8000...8000) / 16000
																				// maximum KT: (500/256) * 0.5017 dB/st = 0.98 dB/st
	if (var <= 0)							// clipping at -30.1 dB
	{
		var = expTableSemitone[0];
	}
	else if (var >= 120 * 256)				// clipping at +30.1 dB
	{
		var = expTableSemitone[120];
	}
	else
	{
		fract = var & 0xFF;																				// lower 8 bits: 0...255
		index = var >> 8;																				// 0...119 [semitone]
		var = (expTableSemitone[index] * (256 - fract) + expTableSemitone[index + 1] * fract) >> 8;		// (...1024...) * 256 / 256
	}

	var = (var * levelVel) >> 12;												// ((32...1024...32768) * (16...16384)) / 4096
	envC_levelFactor[v] = var;													// = 0...4096...131072  -  will be used for the breakpoint and sustain levels
	var = (8000 * var) >> 12;													// (8000 * (0...4096...131072)) / 4096
																				// 8000 is the normalized peak level (no velocity and KT influence) - not 16000 because EnvC is bipolar
	MSG_SetDestination(var);													// sends the peak level

	envC_segment[v] = ATTACK_SEGMENT;


	//========== Gate ==========

	MSG_SelectParameter(PARAM_ID_GATE);
	MSG_SetTime(0);																// step to the full level
	MSG_SetDestination(16000);													// opens the Gate
}



/******************************************************************************/
/**	@brief  ENV_Release: is called when a key is released
	@param  v: Id of the voice (0 ... NUM_VOICES)
	@param	vel: off velocity (0 ... 4096)
*******************************************************************************/

void ENV_Release(uint32_t v, uint32_t vel)
{
	//============= Envelope A =============

	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_A);

	if (envA_releaseTime > 16000)								// Release Time has been set to "inf"
	{
		var = 0xFFFFFFF;
	}
	else
	{
		var = envA_timeKTOffset[v];										// -4629...0...4629

		var -= (vel * ((envA_releaseVelocity * 12593 ) >> 14)) >> 12;	// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
																		// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
		var = EXPON_Time(envA_releaseTime + var);
	}

	MSG_SetTime(var);

	MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_INSERT_OPEN);	// exponential curve

	MSG_SetDestination(0);

	envA_segment[v] = RELEASE_SEGMENT;								// the Release segment has started


	//============= Envelope B =============

	MSG_SelectParameter(PARAM_ID_ENV_B);

	if (envB_releaseTime > 16000)		// Release Time has been set to "inf"
	{
		var = 0xFFFFFFF;
	}
	else
	{
		var = envB_timeKTOffset[v];											// -4629...0...4629

		var -= (vel * ((envB_releaseVelocity * 12593 ) >> 14)) >> 12;		// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
																			// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
		var = EXPON_Time(envB_releaseTime + var);
	}

	MSG_SetTime(var);

	MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_INSERT_OPEN);		// exponential curve

	MSG_SetDestination(0);

	envB_segment[v] = RELEASE_SEGMENT;									// the Release segment has started


	//============= Envelope C =============

	MSG_SelectParameter(PARAM_ID_ENV_C);

	if (envC_releaseTime > 16000)		// Release Time has been set to "inf"
	{
		var = 0xFFFFFFF;
	}
	else
	{
		var = envC_timeKTOffset[v];											// -4629...0...4629

		var -= (vel * ((envC_releaseVelocity * 12593 ) >> 14)) >> 12;		// ((0...4096) * (((0...12000) * 12593) / 16384)) / 4096
																			// 9223.46 for a range of 60 dB_T, (16384 / 12000) * 9223.46 = 12593.1
		var = EXPON_Time(envC_releaseTime + var);
	}

	MSG_SetTime(var);

	MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_INSERT_OPEN);		// exponential curve

	MSG_SetDestination(0);

	envC_segment[v] = RELEASE_SEGMENT;									// the Release segment has started


	//========== Gate ==========

	MSG_SelectParameter(PARAM_ID_GATE);
	MSG_SetTime(480);													// release time of 10 ms
	MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_INSERT_OPEN);		// exponential curve
	MSG_SetDestination(0);												// closing the Gate
}



/******************************************************************************/
/**	@brief 	This function is called with every timer interval (125 us).
 * 			It generates the timed breakpoints.
			Only the Attack segment and the Decay 1 segment have a timed
			duration.
*******************************************************************************/

void ENV_Process(void)
{
	uint32_t v;
	int32_t var;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		//------ Envelope A

		if (envA_segment[v] == ATTACK_SEGMENT)
		{
			envA_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envA_timeCount[v] <= 0 )  										//---- end of Attack segment, start of Decay 1 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_A);

				var = envA_decay1Time + envA_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);
				envA_timeCount[v] = var;

				MSG_SetTime(var);												// send the Decay 1 time

				MSG_SetCurveType(CURVE_TYPE_LIN + SEGMENT_PRIO_APPEND_CLOSED);	// linear curve

				var = (envA_breakpointLevel * envA_levelFactor[v]) >> 12;		// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestination(var);										// go to the Breakpoint level

				envA_segment[v] = DECAY_1_SEGMENT;
			}
		}
		else if (envA_segment[v] == DECAY_1_SEGMENT)
		{
			envA_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envA_timeCount[v] <= 0)  										//---- end of Decay 1 segment, start of Decay 2 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_A);

				var = envA_decay2Time + envA_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_SetTime(var);												// send the Decay 2 time

				MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_APPEND_OPEN);	// send the Curve Type (exponential)

				var = (envA_sustainLevel * envA_levelFactor[v]) >> 12;			// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestination(var);										// go to the Sustain level

				envA_segment[v] = DECAY_2_SEGMENT;
			}
		}


		//------ Envelope B

		if (envB_segment[v] == ATTACK_SEGMENT)
		{
			envB_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envB_timeCount[v] <= 0 )  										//---- end of Attack segment, start of Decay 1 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_B);

				var = envB_decay1Time + envB_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);
				envB_timeCount[v] = var;

				MSG_SetTime(var);												// send the Decay 1 time

				MSG_SetCurveType(CURVE_TYPE_LIN + SEGMENT_PRIO_APPEND_CLOSED);	// linear curve

				var = (envB_breakpointLevel * envB_levelFactor[v]) >> 12;		// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestination(var);										// go to the Breakpoint level

				envB_segment[v] = DECAY_1_SEGMENT;
			}
		}
		else if (envB_segment[v] == DECAY_1_SEGMENT)
		{
			envB_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envB_timeCount[v] <= 0)  										//---- end of Decay 1 segment, start of Decay 2 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_B);

				var = envB_decay2Time + envB_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_SetTime(var);												// send the Decay 2 time

				MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_APPEND_OPEN);	// send the Curve Type (exponential)

				var = (envB_sustainLevel * envB_levelFactor[v]) >> 12;			// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestination(var);										// go to the Sustain level

				envB_segment[v] = DECAY_2_SEGMENT;
			}
		}


		//------ Envelope C

		if (envC_segment[v] == ATTACK_SEGMENT)
		{
			envC_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envC_timeCount[v] <= 0 )  										//---- end of Attack segment, start of Decay 1 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_C);

				var = envC_decay1Time + envC_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);
				envC_timeCount[v] = var;

				MSG_SetTime(var);												// send the Decay 1 time

				MSG_SetCurveType(CURVE_TYPE_LIN + SEGMENT_PRIO_APPEND_CLOSED);	// linear curve

				var = (envC_breakpointLevel * envC_levelFactor[v]) / 4096;		// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestinationSigned(var);									// go to the Breakpoint level

				envC_segment[v] = DECAY_1_SEGMENT;
			}
		}
		else if (envC_segment[v] == DECAY_1_SEGMENT)
		{
			envC_timeCount[v] -= TIME_UNITS_PER_TICK;

			if (envC_timeCount[v] <= 0)  										//---- end of Decay 1 segment, start of Decay 2 segment
			{
				MSG_SelectVoiceBlock(v);
				MSG_SelectParameter(PARAM_ID_ENV_C);

				var = envC_decay2Time + envC_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_SetTime(var);												// send the Decay 2 time

				MSG_SetCurveType(CURVE_TYPE_EXP + SEGMENT_PRIO_APPEND_OPEN);	// send the Curve Type (exponential)

				var = (envC_sustainLevel * envC_levelFactor[v]) / 4096;			// ((0...16000) * (0...4096...131072)) / 4096
				MSG_SetDestinationSigned(var);									// go to the Sustain level

				envC_segment[v] = DECAY_2_SEGMENT;
			}
		}
	}

	MSG_SendMidiBuffer();

	if (waitForFadeOutCount)
	{
		waitForFadeOutCount--;

		if (waitForFadeOutCount == 0)
		{
			MSG_SelectParameter(PARAM_ID_FADE_POINT);
			MSG_SetDestination(2);						// fade-out and start flushing

			waitForFadeInCount = TICKS_BEFORE_FADE_IN;
		}
	}

	if (waitForFadeInCount)
	{
		waitForFadeInCount--;

		if (waitForFadeInCount == 0)
		{
			MSG_ApplyPreloadedValues();

			MSG_SelectParameter(PARAM_ID_FADE_POINT);	// fade-in
			MSG_SetDestination(1);
		}
	}


}



//=============================================== ENV A - Set Functions ===============================================


void ENV_A_SetAttackTime(int32_t paramVal)
{
	envA_attackTime = paramVal;
}


void ENV_A_SetAttackCurvature(int32_t paramVal)
{
	envA_attackCurvature = paramVal;
}


void ENV_A_SetDecay1Time(int32_t paramVal)
{
	envA_decay1Time = paramVal;
}


void ENV_A_SetBreakpointLevel(int32_t paramVal)
{
	envA_breakpointLevel = paramVal;
}



/******************************************************************************/
/**	@brief 	Sets the Decay 2 time of Env A, applying the changes immediately to
 *          voices which are in the Decay 2 segment
	@paramVal: Decay 2 time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_A_SetDecay2Time(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_A);

	envA_decay2Time = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envA_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = envA_decay2Time + envA_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
			var = EXPON_Time(var);
			MSG_BendTime(var);												// send the new Decay 2 time
		}
	}
}



/******************************************************************************/
/**	@brief 	Sets the Sustain level of Env A, applying the changes immediately to
 *          the voices which are in the Decay 2 segment.
	@param  value: Sustain level, 0...16000  [0.0...1.0]
*******************************************************************************/

void ENV_A_SetSustainLevel(int32_t paramVal)
{
	uint32_t v;
	uint32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_A);

	envA_sustainLevel = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envA_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = (envA_sustainLevel * envA_levelFactor[v]) >> 12;		// ((0...16000) * (0...4096...131072)) / 4096
			MSG_SetDestination(var);									// go to the new Sustain level
		}
	}
}


/******************************************************************************/
/**	@brief 	Sets the Release time of Env A, applying the changes immediately to
 *          the voices which are in the Release segment.
	@paramVal: Release time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_A_SetReleaseTime(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_A);

	envA_releaseTime = paramVal;

	if (paramVal > 16000)											// "inf"
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envA_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				MSG_BendTime(0xFFFFFFF);									// changing the Release time to "inf"
			}
		}
	}
	else
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envA_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				var = envA_releaseTime + envA_timeKTOffset[v];				// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_BendTime(var);											// changing the Release time
			}
		}
	}
}



void ENV_A_SetLevelVelocity(int32_t paramVal)
{
	envA_levelVelocity = paramVal;
}



void ENV_A_SetAttackVelocity(int32_t paramVal)
{
	envA_attackVelocity = paramVal;
}



void ENV_A_SetReleaseVelocity(int32_t paramVal)
{
	envA_releaseVelocity = paramVal;
}



void ENV_A_SetLevelKeyTrk(int32_t paramVal)
{
	envA_levelKeyTrk = paramVal;
}


void ENV_A_SetTimeKeyTrk(int32_t paramVal)
{
	envA_timeKeyTrk = paramVal;
}


//=============================================== ENV B - Set Functions ===============================================

void ENV_B_SetAttackTime(int32_t paramVal)
{
	envB_attackTime = paramVal;
}


void ENV_B_SetAttackCurvature(int32_t paramVal)
{
	envB_attackCurvature = paramVal;
}


void ENV_B_SetDecay1Time(int32_t paramVal)
{
	envB_decay1Time = paramVal;
}


void ENV_B_SetBreakpointLevel(int32_t paramVal)
{
	envB_breakpointLevel = paramVal;
}


/******************************************************************************/
/**	@brief 	Sets the Decay 2 time of Env B, applying the changes immediately to
 *          the voices which are in the Decay 2 segment.
	@paramVal: Decay 2 time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_B_SetDecay2Time(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_B);

	envB_decay2Time = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envB_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = envB_decay2Time + envB_timeKTOffset[v];					// (0...16000) + (-4629...0...4629)
			var = EXPON_Time(var);
			MSG_BendTime(var);												// send the new Decay 2 time
		}
	}
}



/******************************************************************************/
/**	@brief 	Sets the Sustain level of Env B, applying the changes immediately to
 *          the voices which are in the Decay 2 segment.
	@param  value: Sustain level, 0...16000  [0.0...1.0]
*******************************************************************************/

void ENV_B_SetSustainLevel(int32_t paramVal)
{
	uint32_t v;
	uint32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_B);

	envB_sustainLevel = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envB_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = (envB_sustainLevel * envB_levelFactor[v]) >> 12;		// ((0...16000) * (0...4096...131072)) / 4096
			MSG_SetDestination(var);									// go to the new sustain level
		}
	}
}


/******************************************************************************/
/**	@brief 	Sets the Release time of Env B, applying the changes immediately to
 *          the voices which are in the Release segment.
	@paramVal: Release time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_B_SetReleaseTime(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_B);

	envB_releaseTime = paramVal;

	if (paramVal > 16000)											// "inf"
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envB_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				MSG_BendTime(0xFFFFFFF);									// changing the Release time to "inf"
			}
		}
	}
	else
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envB_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				var = envB_releaseTime + envB_timeKTOffset[v];				// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_BendTime(var);											// changing the Release time
			}
		}
	}
}


void ENV_B_SetLevelVelocity(int32_t paramVal)
{
	envB_levelVelocity = paramVal;
}


void ENV_B_SetAttackVelocity(int32_t paramVal)
{
	envB_attackVelocity = paramVal;
}


void ENV_B_SetReleaseVelocity(int32_t paramVal)
{
	envB_releaseVelocity = paramVal;
}


void ENV_B_SetLevelKeyTrk(int32_t paramVal)
{
	envB_levelKeyTrk = paramVal;
}


void ENV_B_SetTimeKeyTrk(int32_t paramVal)
{
	envB_timeKeyTrk = paramVal;
}


//=============================================== ENV C - Set Functions ===============================================

void ENV_C_SetAttackTime(int32_t paramVal)
{
	envC_attackTime = paramVal;
}


void ENV_C_SetAttackCurvature(int32_t paramVal)
{
	envC_attackCurvature = paramVal;
}


void ENV_C_SetDecay1Time(int32_t paramVal)
{
	envC_decay1Time = paramVal;
}


void ENV_C_SetBreakpointLevel(int32_t paramVal)
{
	envC_breakpointLevel = paramVal;
}


/******************************************************************************/
/**	@brief 	Sets the Decay 2 time of Env C, applying the changes immediately to
 *          the voices which are in the Decay 2 segment.
	@paramVal: Decay 2 time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_C_SetDecay2Time(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_C);

	envC_decay2Time = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envC_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = envC_decay2Time + envC_timeKTOffset[v];				// (0...16000) + (-4629...0...4629)
			var = EXPON_Time(var);
			MSG_BendTime(var);											// send the new Decay 2 time
		}
	}
}



/******************************************************************************/
/**	@brief 	Sets the Sustain level of Env C, applying the changes immediately to
 *          the voices which are in the Decay 2 segment.
	@param  value: Sustain level, 0...16000  [0.0...1.0]
*******************************************************************************/

void ENV_C_SetSustainLevel(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_C);

	envC_sustainLevel = paramVal;

	for (v = 0; v < NUM_VOICES; v += unisonVoices)
	{
		if (envC_segment[v] == DECAY_2_SEGMENT)
		{
			MSG_SelectVoiceBlock(v);

			var = (envC_sustainLevel * envC_levelFactor[v]) / 4096;		// ((-8000...8000) * (0...4096...131072)) / 4096
			MSG_SetDestinationSigned(var);								// go to the new Sustain level
		}
	}
}


/******************************************************************************/
/**	@brief 	Sets the Release time of Env C, applying the changes immediately to
 *          the voices which are in the Release segment.
	@paramVal: Release time, 0...16000  [1...160000 ms, log scaled]
*******************************************************************************/

void ENV_C_SetReleaseTime(int32_t paramVal)
{
	uint32_t v;
	int32_t var;

	MSG_SelectParameter(PARAM_ID_ENV_C);

	envC_releaseTime = paramVal;

	if (paramVal > 16000)											// "inf"
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envC_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				MSG_BendTime(0xFFFFFFF);								// changing the Release time to "inf"
			}
		}
	}
	else
	{
		for (v = 0; v < NUM_VOICES; v += unisonVoices)
		{
			if (envC_segment[v] == RELEASE_SEGMENT)
			{
				MSG_SelectVoiceBlock(v);

				var = envC_releaseTime + envC_timeKTOffset[v];			// (0...16000) + (-4629...0...4629)
				var = EXPON_Time(var);

				MSG_BendTime(var);										// changing the Release time
			}
		}
	}
}


void ENV_C_SetLevelVelocity(int32_t paramVal)
{
	envC_levelVelocity = paramVal;
}


void ENV_C_SetAttackVelocity(int32_t paramVal)
{
	envC_attackVelocity = paramVal;
}


void ENV_C_SetReleaseVelocity(int32_t paramVal)
{
	envC_releaseVelocity = paramVal;
}


void ENV_C_SetLevelKeyTrk(int32_t paramVal)
{
	envC_levelKeyTrk = paramVal;
}


void ENV_C_SetTimeKeyTrk(int32_t paramVal)
{
	envC_timeKeyTrk = paramVal;
}


/******************************************************************************
	@brief  Is used by poly.c to set the number of unison voices
*******************************************************************************/

void ENV_SetUnisonVoices(uint32_t uv)
{
	unisonVoices = uv;
}


/******************************************************************************
	@brief  Is used by param_work.c to delay the fade-in after preset recall
*******************************************************************************/

void ENV_StartWaitForFadeOut(void)
{
	waitForFadeOutCount = TICKS_BEFORE_FADE_OUT;
}
