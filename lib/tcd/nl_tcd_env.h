/******************************************************************************/
/** @file		nl_tcd_env.h
    @date		2013-04-22
    @version	0.02
    @author		Stephan Schmitt[2012-06-14]
    @brief		<tbd>
	@note		fka mmgen 
*******************************************************************************/

#ifndef NL_TCD_ENV_H_
#define NL_TCD_ENV_H_

#include "stdint.h"


//======== public functions

void ENV_A_SetAttackTime(int32_t paramVal);
void ENV_A_SetAttackCurvature(int32_t paramVal);
void ENV_A_SetDecay1Time(int32_t paramVal);
void ENV_A_SetBreakpointLevel(int32_t paramVal);
void ENV_A_SetDecay2Time(int32_t paramVal);				// real-time parameter change in envelope
void ENV_A_SetSustainLevel(int32_t paramVal);			// real-time parameter change in envelope
void ENV_A_SetReleaseTime(int32_t paramVal);			// real-time parameter change in envelope
void ENV_A_SetLevelVelocity(int32_t paramVal);
void ENV_A_SetAttackVelocity(int32_t paramVal);
void ENV_A_SetReleaseVelocity(int32_t paramVal);
void ENV_A_SetLevelKeyTrk(int32_t paramVal);
void ENV_A_SetTimeKeyTrk(int32_t paramVal);

void ENV_B_SetAttackTime(int32_t paramVal);
void ENV_B_SetAttackCurvature(int32_t paramVal);
void ENV_B_SetDecay1Time(int32_t paramVal);
void ENV_B_SetBreakpointLevel(int32_t paramVal);
void ENV_B_SetDecay2Time(int32_t paramVal);				// real-time parameter change in envelope
void ENV_B_SetSustainLevel(int32_t paramVal);			// real-time parameter change in envelope
void ENV_B_SetReleaseTime(int32_t paramVal);			// real-time parameter change in envelope
void ENV_B_SetLevelVelocity(int32_t paramVal);
void ENV_B_SetAttackVelocity(int32_t paramVal);
void ENV_B_SetReleaseVelocity(int32_t paramVal);
void ENV_B_SetLevelKeyTrk(int32_t paramVal);
void ENV_B_SetTimeKeyTrk(int32_t paramVal);

void ENV_C_SetAttackTime(int32_t paramVal);
void ENV_C_SetAttackCurvature(int32_t paramVal);
void ENV_C_SetDecay1Time(int32_t paramVal);
void ENV_C_SetBreakpointLevel(int32_t paramVal);
void ENV_C_SetDecay2Time(int32_t paramVal);
void ENV_C_SetSustainLevel(int32_t paramVal);
void ENV_C_SetReleaseTime(int32_t paramVal);
void ENV_C_SetLevelVelocity(int32_t paramVal);
void ENV_C_SetAttackVelocity(int32_t paramVal);
void ENV_C_SetReleaseVelocity(int32_t paramVal);
void ENV_C_SetLevelKeyTrk(int32_t paramVal);
void ENV_C_SetTimeKeyTrk(int32_t paramVal);

void ENV_Init(void);

void ENV_Start(uint32_t v, uint32_t timeInUs); 			// will be called when a key is pressed
void ENV_Release(uint32_t v, uint32_t timeInUs);		// will be called when a key is released
void ENV_Process(void);                            		// called with every timer interval (125 us)

void ENV_SetVoicePitch(uint32_t v, int32_t pitch);		// called from the pitch calculation in poly.c

void ENV_SetUnisonVoices(uint32_t uv);					// called from poly.c

void ENV_StartWaitForFadeOut(void);						// called from param_work.c

#endif /* NL_TCD_ENV_H_ */
