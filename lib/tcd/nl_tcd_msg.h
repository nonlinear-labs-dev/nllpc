/******************************************************************************/
/** @file		nl_tcd_msg.h
    @date		2013-04-23
    @version	0.02
    @author		Stephan Schmitt [2012-06-15]
    @brief		<tbd>
*******************************************************************************/

#ifndef NL_TCD_MSG_H_
#define NL_TCD_MSG_H_


#include "stdint.h"

/******************************************************************************
*	defines
******************************************************************************/

#define CURVE_TYPE_LIN 				0
#define CURVE_TYPE_EXP 				1
#define CURVE_TYPE_CLIPPED_EXP 		2
#define CURVE_TYPE_POLYNOM 			3

#define SEGMENT_PRIO_APPEND_OPEN 	0
#define SEGMENT_PRIO_APPEND_CLOSED	128
#define SEGMENT_PRIO_INSERT_OPEN 	256
#define SEGMENT_PRIO_INSERT_CLOSED	384


/******************************************************************************
*	public functions
******************************************************************************/

void MSG_CheckUSB(void);
void MSG_SendMidiBuffer(void);

void MSG_SelectVoice(uint32_t v);
void MSG_SelectMultipleVoices(uint32_t v_last);
void MSG_SelectVoiceBlock(uint32_t v_first);
void MSG_SelectAllVoices(void);
void MSG_AddVoice(uint32_t v);

void MSG_SelectParameter(uint32_t p);
void MSG_SelectMultipleParameters(uint32_t p_last);
void MSG_SelectAllParameters(void);
void MSG_AddParameter(uint32_t p);

void MSG_SetTime(uint32_t t);
void MSG_BendTime(uint32_t t);

void MSG_SetCurveType(uint32_t ct);
void MSG_SetCurvature(int32_t c);

void MSG_SetDestination(uint32_t d);
void MSG_SetDestinationSigned(int32_t d);
void MSG_BendDestination(uint32_t d);
void MSG_BendDestinationSigned(int32_t d);

void MSG_DisablePreload(void);							// disables the Preload mode: a D message immediately starts a transition
void MSG_EnablePreload(void);							// enables the Preload mode: the transition will start when the Apply message arrives
void MSG_ApplyPreloadedValues(void);					// applies preloaded D values and starts the transition(s)

void MSG_Parallel_V_P(void);
void MSG_Serial_V_Parallel_P(void);
void MSG_Parallel_V_Serial_P(void);
void MSG_Serial_V_P(void);


void MSG_RefreshAddresses(void);						// the following messages will include voice and parameter addresses

void MSG_SetUnisonVoices(uint32_t uv);					// called from poly.c


#endif /* NL_TCD_MSG_H_ */
