/*
 * nl_tcd_poly.c
 *
 *  Created on: 27.01.2015
 *      Author: ssc
 */

#include "math.h"

#include "nl_tcd_poly.h"

#include "nl_tcd_env.h"
#include "nl_tcd_param_work.h"
#include "nl_tcd_msg.h"
#include "nl_tcd_valloc.h"


static int32_t v_UnisonIdx[NUM_VOICES] = {};
static int32_t v_UnisonPhase[NUM_VOICES] = {};

static int32_t v_KeyPos[NUM_VOICES] = {};

static int32_t e_KeyPan;

static int32_t e_UnisonVoices;
static int32_t e_UnisonPan;
static int32_t e_UnisonDetune;
static int32_t e_UnisonPhase;

static int32_t e_NoteShift;

static uint32_t e_ScaleBase;
static int16_t e_ScaleOffset[12];

static uint32_t velTable[65] = {};									// converts time difference (timeInUs) to velocities
																	// element  0: shortest timeInUs   (2500 us or lower) -> 4096 = max. velocity
																	// element 64: longest timeInUs (526788 us or higher) -> 0    = min. velocity


/*******************************************************************************
@brief  	ENV_Generate_VelTable - generates the elements of velTable[]
@param[in]	curve - selects one of the five verlocity curves
			table index: 0 ... 64 (shortest ... longest key-switch time)
			table values: 4096 ... 0
*******************************************************************************/

void POLY_Generate_VelTable(uint32_t curve)
{
	float_t vel_max = 4096.0;	// the hyperbola goes from vel_max (at 0) to 0 (at i_max)

	float_t b = 0.5;

	switch (curve)	// b defines the curve shape
	{
		case VEL_CURVE_VERY_SOFT:
			b = 0.125;
			break;
		case VEL_CURVE_SOFT:
			b = 0.25;
			break;
		case VEL_CURVE_NORMAL:
			b = 0.5;
			break;
		case VEL_CURVE_HARD:
			b = 1.0;
			break;
		case VEL_CURVE_VERY_HARD:
			b = 2.0;
			break;
		default:
			/// Error
			break;
	}

	uint32_t i_max = 64;

	uint32_t i;

	for (i = 0; i <= i_max; i++)
	{
		velTable[i] = (uint32_t)( ( vel_max + vel_max / (b * i_max) ) / (1.0 + b * i) - vel_max / (b * i_max) + 0.5 );
	}
}



//================= Initialisation:

void POLY_Init(void)
{
	uint32_t v;
	uint32_t i;

	for (v = 0; v < NUM_VOICES; v++)
	{
		v_KeyPos[v] = 0;
		v_UnisonIdx[v] = 0;
		v_UnisonPhase[v] = 0;
	}

	e_KeyPan = 0;

	e_UnisonVoices = 1;
	e_UnisonPan = 0;
	e_UnisonDetune = 0;
	e_UnisonPhase = 0;

	e_NoteShift = 0;

	e_ScaleBase = 0;

	for (i = 0; i < 12; i++)
	{
		e_ScaleOffset[i] = 0;			// e_ScaleOffset[0] bleibt immer Null (base key)
	}

	POLY_Generate_VelTable(VEL_CURVE_NORMAL);
}


//================= Custom Scale interface:


void POLY_SetScaleBase(uint32_t baseKey)
{
	if (baseKey < 12)
	{
		e_ScaleBase = 12 - baseKey;
	}
	/// else error
}


void POLY_SetScaleOffset(uint32_t paramId, uint32_t offset)
{
	uint32_t step;

	step = 1 + paramId - PARAM_ID_SCALE_OFFSET_1;

	if (step < 12)
	{
		if (offset & 0x8000)
		{
			e_ScaleOffset[step] = -(offset & 0x7FFF);
		}
		else
		{
			e_ScaleOffset[step] = offset;
		}
	}											/// es wäre besser, auch klingende Stimmen verstimmen zu können, ähnlich wie Tune/Unison
	/// else error
}



//================= Keybed Events:


void POLY_KeyDown(uint32_t allocVoice, uint32_t key, uint32_t timeInUs)
{
	int32_t out;
	int32_t pitch;
	uint32_t vel;
	uint32_t voice;
	uint32_t minVoice;
	uint32_t maxVoice;


	//--- Calc pitch

	pitch = (key - 24) * 1000 + e_NoteShift + e_ScaleOffset[(key + e_ScaleBase) % 12];


	//--- Calc On velocity

	if (timeInUs <= 2500)							// clipping the low end: zero at a times <= 2.5 ms
	{
		vel = velTable[0];
	}
	else if (timeInUs >= (524288 + 2500))			// clipping at a maximum of 524 ms (2^19 us)
	{
		vel = velTable[64];
	}
	else
	{
		timeInUs -= 2500;	 						// shifting the curve to the left adjusting the input to zero for a time of 2.5 ms

		uint32_t fract = timeInUs & 0x1FFF;												// lower 13 bits used for interpolation
		uint32_t index = timeInUs >> 13;												// upper 6 bits (0...63) used as index in the table
		vel = (velTable[index] * (8192 - fract) + velTable[index + 1] * fract) >> 13;	// (0...4096) * 8192 / 8192
	}


	minVoice = allocVoice * e_UnisonVoices;
	maxVoice = minVoice + e_UnisonVoices;

	MSG_KeyPreload();	// the whole block of messages starting a note is preloaded

	MSG_SelectVoice(minVoice);

	for (voice = minVoice; voice < maxVoice; voice++)
	{
		v_KeyPos[voice] = key;

		//-------------- Phases

		MSG_SetDestinationSigned(v_UnisonPhase[voice]);

		//------------- NotePitch

		out = pitch + v_UnisonIdx[voice] * e_UnisonDetune;

		MSG_SetDestinationSigned(out);

		//-------------- VoicePan

		out = (key - 30) * e_KeyPan + v_UnisonIdx[voice] * e_UnisonPan;

		MSG_SetDestinationSigned(out);

		//-------------- Voice Steal

		MSG_SetDestinationSigned(0);				/// detection not yet implemented !!!

		//-------------- Key Down

		MSG_KeyDown(vel);
	}

	MSG_ApplyPreloadedValues();
}



void POLY_KeyUp(uint32_t allocVoice, uint32_t timeInUs)
{
	uint32_t vel;
	uint32_t voice;
	uint32_t minVoice;
	uint32_t maxVoice;


	//--- Calc Off velocity

	if (timeInUs <= 2500)							// clipping the low end: zero at a times <= 2.5 ms
	{
		vel = velTable[0];
	}
	else if (timeInUs >= (524288 + 2500))			// clipping at a maximum of 524 ms (2^19 us)
	{
		vel = velTable[64];
	}
	else
	{
		timeInUs -= 2500;	 							// shifting the curve to the left adjusting the input to zero for a time of 2.5 ms

		uint32_t fract = timeInUs & 0x1FFF;												// lower 13 bits used for interpolation
		uint32_t index = timeInUs >> 13;												// upper 6 bits (0...63) used as index in the table
		vel = (velTable[index] * (8192 - fract) + velTable[index + 1] * fract) >> 13;	// (0...4096) * 8192 / 8192
	}


	minVoice = allocVoice * e_UnisonVoices;
	maxVoice = minVoice + e_UnisonVoices;

	MSG_KeyPreload();	// the whole block of messages ending a note is preloaded

	MSG_SelectVoice(minVoice);

	for (voice = minVoice; voice < maxVoice; voice++)
	{
		MSG_KeyUp(vel);
	}

	MSG_ApplyPreloadedValues();
}


//==========  Called by POLY_SetUnisonVoices():

void SetUnisonIdx(uint32_t voice, int32_t unisonIdx)
{
	int32_t out;

	MSG_SelectVoice(voice);									/// eigentlich ein Fall für den List-Mode (Iteration über alle Voices)

	//------------- NotePitch

	out = (v_KeyPos[voice] - 24) * 1000 + e_NoteShift + unisonIdx * e_UnisonDetune;

	MSG_SelectParameter(PARAM_ID_NOTEPITCH);
	MSG_SetDestinationSigned(out);

	//------------- Phases

	v_UnisonPhase[voice] = unisonIdx * e_UnisonPhase;

	//-------------- VoicePan

	out = (v_KeyPos[voice] - 30) * e_KeyPan + unisonIdx * e_UnisonPan;

	MSG_SelectParameter(PARAM_ID_VOICE_PAN);
	MSG_SetDestinationSigned(out);

	//-----------------

	v_UnisonIdx[voice] = unisonIdx;
}



//================= Parameter-Events:


void POLY_SetUnisonPhase(int32_t value)
{
	if (e_UnisonVoices > 1)
	{
		uint32_t v;

		for (v = 0; v < NUM_VOICES; v++)
		{
			v_UnisonPhase[v] = value * v_UnisonIdx[v];
		}
	}

	e_UnisonPhase = value;
}




void POLY_SetUnisonPan(int32_t value)
{
	if (e_UnisonVoices > 1)
	{
		uint32_t v;
		int32_t out;

		MSG_SelectParameter(PARAM_ID_VOICE_PAN);

		for (v = 0; v < NUM_VOICES; v++)										/// eigentlich ein Fall für List-Mode !!!
		{
			out = (v_KeyPos[v] - 30) * e_KeyPan + v_UnisonIdx[v] * value;

			MSG_SelectVoice(v);
			MSG_SetDestinationSigned(out);
		}
	}

	e_UnisonPan = value;
}


/*****************************************************************************
*	@brief  POLY_SetKeyPan - ...
*   @param  value: key-panning amount, range: 0 ... 16000 (0 ... 100 %)
******************************************************************************/

void POLY_SetKeyPan(int32_t value)
{
	uint32_t v;
	int32_t out;

	e_KeyPan = (value * 17) >> 10;			// 17 / 1024 = approximately 1/60

	MSG_SelectParameter(PARAM_ID_VOICE_PAN);

	for (v = 0; v < NUM_VOICES; v++)											/// eigentlich ein Fall für List-Mode !!!
	{
		out = (v_KeyPos[v] - 30) * e_KeyPan + v_UnisonIdx[v] * e_UnisonPan;

		MSG_SelectVoice(v);
		MSG_SetDestinationSigned(out);
	}
}



void POLY_SetUnisonDetune(int32_t value)
{
	e_UnisonDetune = value;

	if (e_UnisonVoices > 1)
	{
		uint32_t v;
		int32_t out;

		MSG_SelectParameter(PARAM_ID_NOTEPITCH);

		for (v = 0; v < NUM_VOICES; v++)									/// eigentlich ein Fall für List-Mode !!!
		{
			out = (v_KeyPos[v] - 24) * 1000 + e_NoteShift + v_UnisonIdx[v] * e_UnisonDetune;

			MSG_SelectVoice(v);
			MSG_SetDestinationSigned(out);
		}
	}
}



void POLY_SetUnisonVoices(int32_t value)
{
	if (value != e_UnisonVoices)
	{
		e_UnisonVoices = value;

		VALLOC_Init((uint32_t)(NUM_VOICES / e_UnisonVoices));

		uint32_t v;

		for (v = 0; v < NUM_VOICES; v++)
		{
			if (e_UnisonVoices % 2)		// odd
			{
				v_UnisonIdx[v] = (v % e_UnisonVoices) - (int32_t)(e_UnisonVoices / 2);		// e.g.: -1, 0, 1
			}
			else						// even
			{
				v_UnisonIdx[v] = (v % e_UnisonVoices) + 1 - (int32_t)(e_UnisonVoices / 2);	// e.g.: -1, 0, 1, 2   /// besser um -0.5 verschieben ?
			}

			SetUnisonIdx(v, v_UnisonIdx[v]);
		}

		MSG_SetUnisonVoices(e_UnisonVoices);
	}
}



void POLY_SetNoteShift(uint32_t shift)
{
	if (shift & 0x8000)
	{
		e_NoteShift = -(shift & 0x7FFF) * 1000;
	}
	else
	{
		e_NoteShift = shift * 1000;
	}
}
