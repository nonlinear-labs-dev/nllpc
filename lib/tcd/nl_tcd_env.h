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


void ENV_Init(void);

void ENV_Process(void);                            		// called with every timer interval (125 us)

void ENV_StartWaitForFadeOut(void);						// called from param_work.c

#endif /* NL_TCD_ENV_H_ */
