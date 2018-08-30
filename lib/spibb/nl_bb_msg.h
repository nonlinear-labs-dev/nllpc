/*
 * nl_bb_msg.h
 *
 *  Created on: 21.01.2015
 *      Author: ssc
 */

#ifndef NL_DRV_NL_BB_MSG_H_
#define NL_DRV_NL_BB_MSG_H_

#include "stdint.h"

#define BB_MSG_TYPE_PRESET_DIRECT	0x0100
#define BB_MSG_TYPE_MORPH_SET_A		0x0200
#define BB_MSG_TYPE_MORPH_SET_B		0x0300
#define BB_MSG_TYPE_PARAMETER		0x0400
#define BB_MSG_TYPE_EDIT_CONTROL	0x0500
#define BB_MSG_TYPE_MORPH_POS		0x0600
#define BB_MSG_TYPE_SETTING			0x0700
#define BB_MSG_TYPE_NOTIFICATION	0x0800			// 2 Argumente: Notification-Typ und z.B. SW-Version
#define BB_MSG_TYPE_ASSERTION		0x0900
#define BB_MSG_TYPE_REQUEST			0x0A00


//----- Setting Ids:

#define SETTING_ID_PLAY_MODE_UPPER_RIBBON_BEHAVIOUR 0	// ==> BIT 0 set if (returnMode == RETURN)
#define SETTING_ID_PLAY_MODE_LOWER_RIBBON_BEHAVIOUR 1	// ... BIT 1 set if (touchBehaviour == RELATIVE)

#define SETTING_ID_NOTE_SHIFT 2							// ==> tTcdRange (-48, 48)

#define SETTING_ID_BASE_UNIT_UI_MODE 3					// ==> PLAY = 0, PARAMETER_EDIT = 1

#define SETTING_ID_EDIT_MODE_RIBBON_BEHAVIOUR 4 		// ==> RELATIVE = 0, ABSOLUTE = 1

#define SETTING_ID_PEDAL_1_MODE 5                         // ==> STAY = 0
#define SETTING_ID_PEDAL_2_MODE 6                         // ... RETURN_TO_ZERO = 1
#define SETTING_ID_PEDAL_3_MODE 7                         // ... RETURN_TO_CENTER = 2,
#define SETTING_ID_PEDAL_4_MODE 8

#define SETTING_ID_UPPER_RIBBON_REL_FACTOR 9              // ==> tTcdRange(256, 2560)
#define SETTING_ID_LOWER_RIBBON_REL_FACTOR 10             // ==> tTcdRange(256, 2560)

#define SETTING_ID_VELOCITY_CURVE 11                      // ==> VERY_SOFT = 0, SOFT = 1, NORMAL = 2, HARD = 3, VERY_HARD = 4

#define SETTING_ID_TRANSITION_TIME 12                     // ==> tTcdRange(0, 16000)

#define SETTING_ID_PEDAL_1_TYPE 26                        // ==> PotTipActive = 0
#define SETTING_ID_PEDAL_2_TYPE 27                        // ... PotRingActive = 1
#define SETTING_ID_PEDAL_3_TYPE 28                        // ... SwitchClosing = 2
#define SETTING_ID_PEDAL_4_TYPE 29                        // ... SwitchOpening = 3

#define SETTING_ID_AFTERTOUCH_CURVE 30                    // SOFT = 0, NORMAL = 1, HARD = 2
#define SETTING_ID_BENDER_CURVE 31  	                     // SOFT = 0, NORMAL = 1, HARD = 2
#define SETTING_ID_PITCHBEND_ON_PRESSED_KEYS 32           // OFF = 0, ON = 1
#define SETTING_ID_EDIT_SMOOTHING_TIME 33                 // ==> tTcdRange(0, 16000)
#define SETTING_ID_PRESET_GLITCH_SUPPRESSION 34           // OFF = 0, ON = 1


//----- Request Ids:

#define REQUEST_ID_SW_VERSION		0x0000


//----- Notification Ids:

#define NOTIFICATION_ID_SW_VERSION	0x0000


//===========================

#define SW_VERSION 100

//===========================


int32_t BB_MSG_WriteMessage(uint16_t type, uint16_t length, uint16_t* data);
int32_t BB_MSG_WriteMessage2Arg(uint16_t type, uint16_t arg0, uint16_t arg1);
int32_t BB_MSG_WriteMessage1Arg(uint16_t type, uint16_t arg);
int32_t BB_MSG_WriteMessageNoArg(uint16_t type);

int32_t BB_MSG_SendTheBuffer(void);

void BB_MSG_ReceiveCallback(uint16_t type, uint16_t length, uint16_t* data);


#endif /* NL_DRV_NL_BB_MSG_H_ */
