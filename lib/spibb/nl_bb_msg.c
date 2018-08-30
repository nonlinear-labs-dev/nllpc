/*
 * nl_bb_msg.c
 *  last mod: 2016-04-27 DTZ
 *  Created on: 21.01.2015
 *      Author: ssc
 */

#include "nl_bb_msg.h"
#include "nl_spi_bb.h"

#include "tcd/nl_tcd_env.h"
#include "tcd/nl_tcd_param_work.h"
#include "tcd/nl_tcd_adc_work.h"
#include "tcd/nl_tcd_poly.h"
#include "dbg/nl_assert.h"

#define SENDBUFFER_SIZE  510						// 16-bit words, stays below the maximum of 1020 bytes

static uint16_t sendBuffer[SENDBUFFER_SIZE] = {};
static uint32_t sendBufferLen = 0;					// behind the 4-byte header



/**********************************************************************
 * @brief		Writing a generic message into the SPI send buffer
 * @param[in]	type	message type (see defines)
 * @param[in]	length	number of 16-bit data fields
 * @param[in]	data	pointer to an array of 16-bit data fields
 * @return		>= 0: "success" and remaining buffer space
 * 				-1: "buffer is full, try again later"
 *
 * 				NOTE: NO ASSERTION IN THIS FUNCTION => RECURSION
 **********************************************************************/

int32_t BB_MSG_WriteMessage(uint16_t type, uint16_t length, uint16_t* data)
{
	int32_t remainingBuffer = SENDBUFFER_SIZE - (sendBufferLen + length + 2);

	if (remainingBuffer < 0)
	{
		return -1;		// buffer is full
	}

	sendBuffer[sendBufferLen] = type;
	sendBufferLen++;
	sendBuffer[sendBufferLen] = length;
	sendBufferLen++;

	if (data != NULL)
	{
		uint32_t i;

		for (i = 0; i < length; i++)
		{
			sendBuffer[sendBufferLen] = data[i];
			sendBufferLen++;
		}
	}

	return remainingBuffer;
}


/**********************************************************************
 * @brief		Writes a message with 2 arguments into the send buffer
 * @param[in]	type	message type (see defines)
 * @param[in]	arg0	e.g. the identifier of a parameter
 * @param[in]	arg1	e.g. the value of the parameter
 * @return		>= 0: "success" and remaining buffer space
 * 				-1: "buffer is full, try again later"
 **********************************************************************/

int32_t BB_MSG_WriteMessage2Arg(uint16_t type, uint16_t arg0, uint16_t arg1)
{
	int32_t remainingBuffer = SENDBUFFER_SIZE - (sendBufferLen + 4);

	if (remainingBuffer < 0)
	{
		return -1;		// buffer is full
	}

	sendBuffer[sendBufferLen] = type;
	sendBufferLen++;
	sendBuffer[sendBufferLen] = 2;
	sendBufferLen++;

	sendBuffer[sendBufferLen] = arg0;
	sendBufferLen++;

	sendBuffer[sendBufferLen] = arg1;
	sendBufferLen++;

	NL_ASSERT(sendBufferLen < SENDBUFFER_SIZE);

	return remainingBuffer;
}


/**********************************************************************
 * @brief		Writes a message with 1 argument into the send buffer
 * @param[in]	type	message type (see defines)
 * @param[in]	arg		e.g. the morph position
 * @return		>= 0: "success" and remaining buffer space
 * 				-1: "buffer is full, try again later"
 **********************************************************************/

int32_t BB_MSG_WriteMessage1Arg(uint16_t type, uint16_t arg)
{
	int32_t remainingBuffer = SENDBUFFER_SIZE - (sendBufferLen + 3);

	if (remainingBuffer < 0)
	{
		return -1;		// buffer is full
	}

	sendBuffer[sendBufferLen] = type;
	sendBufferLen++;
	sendBuffer[sendBufferLen] = 1;
	sendBufferLen++;

	sendBuffer[sendBufferLen] = arg;
	sendBufferLen++;

	return remainingBuffer;
}


/**********************************************************************
 * @brief		Writes a message with 1 argument into the send buffer
 * @param[in]	type	message type (see defines)
 * @return		>= 0: "success" and remaining buffer space
 * 				-1: "buffer is full, try again later"
 **********************************************************************/

int32_t BB_MSG_WriteMessageNoArg(uint16_t type)
{
	int32_t remainingBuffer = SENDBUFFER_SIZE - (sendBufferLen + 1);

	if (remainingBuffer < 0)
	{
		return -1;		// buffer is full
	}

	sendBuffer[sendBufferLen] = (type << 16);	// no data fields
	sendBufferLen++;

	return remainingBuffer;
}



/**********************************************************************
 * @brief		Sending a package with one or more messages via SPI
 * @return		buffer length in bytes - success
 *              0 = "nothing to send"
 *              -1 = "failure, try again later"
 *              sendBufferLen*2 = "success"
 **********************************************************************/

int32_t BB_MSG_SendTheBuffer(void)
{
	if (sendBufferLen == 0)
	{
		return 0;	// nothing to send
	}

	uint8_t* buff = (uint8_t*)sendBuffer;

	uint32_t success = SPI_BB_Send(buff, sendBufferLen * 2);

	if (success)
	{
		sendBufferLen = 0;	// position for first messages of next buffer (behind the header)

		return success;
	}
	else	// sending failed, try again later
	{
		return -1;
	}
}



/*****************************************************************************
 * @brief		BB_MSG_ReceiveCallback - Callback for receiving messages from
 * the Beaglebone (called from the PackageParser of the spi_bb driver).
 *****************************************************************************/

void BB_MSG_ReceiveCallback(uint16_t type, uint16_t length, uint16_t* data)
{
	// data[0]	- parameter id
	// data[1]  - first value
	// data[2]  - second value

	if (type == BB_MSG_TYPE_PARAMETER)
	{
		if (length == 3)
		{
			PARAM_Set2(data[0], data[1], data[2]);
		}
		else
		{
			PARAM_Set(data[0], data[1]);
		}
	}
	else if (type == BB_MSG_TYPE_PRESET_DIRECT)
	{
		PARAM_ApplyPreset(length, data);
	}
	else if (type == BB_MSG_TYPE_SETTING)
	{
		switch (data[0])
		{
			case 0:											// Play mode ribbon 1 behaviour
				ADC_WORK_SetRibbon1Behaviour(data[1]);			// 0: Abs + Non-Return, 1: Abs + Return, 2: Rel + Non-Return, 3: Rel + Return
				break;
			case 1:											// Play mode ribbon 2 behaviour
				ADC_WORK_SetRibbon2Behaviour(data[1]);			// 0: Abs + Non-Return, 1: Abs + Return, 2: Rel + Non-Return, 3: Rel + Return
				break;
			case 2:											// Note Shift
				POLY_SetNoteShift(data[1]);						// (+/-)0...48 (uint16 with sign bit)
				break;
			case 3:											// "Unit Mode" - Ribbon 1 can be switched between Edit and Play mode.
				ADC_WORK_SetRibbon1EditMode(data[1]);			// 0: Play, 1: Parameter Edit
				break;
			case 4:											// Parameter edit mode ribbon behaviour
				ADC_WORK_SetRibbon1EditBehaviour(data[1]);		// 0: Rel, 1: Abs
				break;
			case 5:											// Behaviour of Pedal 1
				ADC_WORK_SetPedal1Behaviour(data[1]);			// 0: Non-Return, 1: Return to Zero, 2: Return to Center
				break;
			case 6:											// Behaviour of Pedal 2
				ADC_WORK_SetPedal2Behaviour(data[1]);			// 0: Non-Return, 1: Return to Zero, 2: Return to Center
				break;
			case 7:											// Behaviour of Pedal 3
				ADC_WORK_SetPedal3Behaviour(data[1]);			// 0: Non-Return, 1: Return to Zero, 2: Return to Center
				break;
			case 8:											// Behaviour of Pedal 4
				ADC_WORK_SetPedal4Behaviour(data[1]);			// 0: Non-Return, 1: Return to Zero, 2: Return to Center
				break;
			case 9:											// Factor for the increments when a ribbon is in Relative mode
				ADC_WORK_SetRibbonRelFactor(data[1]);			// factor = data[1] / 256
				break;
			case 11:										// Velocity Curve
				POLY_Generate_VelTable(data[1]);					// Parameter: 0 = very soft ... 4 = very hard
				break;
			case 12:										// Transition Time
				PARAM_SetTransitionTime(data[1]);
				break;
			case 30:										// Aftertouch Curve
				ADC_WORK_Generate_AftertouchTable(data[1]); 	// 0: soft, 1: normal, 2: hard
				break;
			case 31:										// Bender Curve
				ADC_WORK_Generate_BenderTable(data[1]);			// 0: soft, 1: normal, 2: hard
				break;
			case 32:										// Pitchbend on Pressed Keys
				break;
			case 33:										// Edit Smoothing Time
				PARAM_SetEditSmoothingTime(data[1]);
				break;
			case 34:										// Glitch Suppression
				PARAM_SetGlitchSuppression(data[1]);			// 0: off, 1: on
				break;
			default:
				/// Error
				break;
		}
	}
	else if (type == BB_MSG_TYPE_REQUEST)
	{
		if (data[0] == REQUEST_ID_SW_VERSION)		// requesting the version of the LPC ("RT") software
		{
			BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_NOTIFICATION, NOTIFICATION_ID_SW_VERSION, SW_VERSION);  // sending the software version to the BB
			BB_MSG_SendTheBuffer();
		}
	}
}
