/*
 * nl_tcd_adc_work.h
 *
 *  Created on: 13.03.2015
 *      Author: ssc
 */

#ifndef TCD_NL_TCD_ADC_WORK_H_
#define TCD_NL_TCD_ADC_WORK_H_


#include "stdint.h"


#define NON_RETURN			0
#define RETURN_TO_ZERO		1
#define RETURN_TO_CENTER	2


//------- public functions

void ADC_WORK_Init(void);

void ADC_WORK_Process(void);
void ADC_WORK_Suspend(void);
void ADC_WORK_Resume(void);

void ADC_WORK_SendBBMessages(void);

void ADC_WORK_SetPedal1Behaviour(uint32_t behaviour);
void ADC_WORK_SetPedal2Behaviour(uint32_t behaviour);
void ADC_WORK_SetPedal3Behaviour(uint32_t behaviour);
void ADC_WORK_SetPedal4Behaviour(uint32_t behaviour);

void ADC_WORK_SetRibbon1EditMode(uint32_t mode);
void ADC_WORK_SetRibbon1EditBehaviour(uint32_t behaviour);
void ADC_WORK_SetRibbon1Behaviour(uint32_t behaviour);
void ADC_WORK_SetRibbon2Behaviour(uint32_t behaviour);
void ADC_WORK_SetRibbonRelFactor(uint32_t factor);

uint32_t ADC_WORK_GetPedal1Behaviour(void);
uint32_t ADC_WORK_GetPedal2Behaviour(void);
uint32_t ADC_WORK_GetPedal3Behaviour(void);
uint32_t ADC_WORK_GetPedal4Behaviour(void);
uint32_t ADC_WORK_GetRibbon1Behaviour(void);
uint32_t ADC_WORK_GetRibbon2Behaviour(void);

void ADC_WORK_Check_Pedal_Start(uint32_t pedalId);
void ADC_WORK_Check_Pedal_Cancel(uint32_t pedalId);

void ADC_WORK_Generate_BenderTable(uint32_t curve);
void ADC_WORK_Generate_AftertouchTable(uint32_t curve);

#endif /* TCD_NL_TCD_ADC_WORK_H_ */
