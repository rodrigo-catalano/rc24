/*
 Copyright 2008 - 2009 © Alan Hopper

 This file is part of rc24.

 rc24 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 rc24 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with rc24.  If not, see <http://www.gnu.org/licenses/>.


 */

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "myrxs.h"
#include "swEventQueue.h"
#include "servopwm.h"
#include "motorpwm.h"
#include "nmeagps.h"

#include <printf.h>

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

// map inputs from the tx to outputs from the rx
// outputs can currently be;
//	servopwm - for servo outputs, 0 - 4096 from tx -> 1.0 - 2.0 ms pulse width
//  pwm  - suitable for simple motor speed control (2 only on timer outputs)
//  digital - digital output, > 3000 from tx -> on
//  i2c - i2c i/o
//  nc - not connected

// Set up for 6ch standard servo output
opChannel standard6channel[20] = {
	{ servopwm, E_AHI_DIO10_INT},
	{ servopwm, E_AHI_DIO13_INT},
	{ servopwm, E_AHI_DIO6_INT},
	{ servopwm, E_AHI_DIO7_INT},
	{ servopwm, E_AHI_DIO14_INT},
	{ servopwm, E_AHI_DIO15_INT},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0 }
};

// set up for 6ch standard servo output with serial debug
opChannel standard6channelDebug[20] = {
	{ servopwm, E_AHI_DIO9_INT},
	{ servopwm, E_AHI_DIO14_INT},
	{ servopwm, E_AHI_DIO15_INT},
	{ servopwm, E_AHI_DIO13_INT},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0 }
};

// setup for 2 servos and two brushed motors
opChannel propsteer[20] = {
	{ servopwm, E_AHI_DIO6_INT},
	{ servopwm, E_AHI_DIO7_INT},
	{ pwm, E_AHI_DIO10_INT},
	{ pwm, E_AHI_DIO13_INT},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0}
};

// setup for 4 servos and gps
opChannel gps[20] = {
	{ servopwm, E_AHI_DIO10_INT},
	{ servopwm, E_AHI_DIO13_INT},
	{ servopwm, E_AHI_DIO14_INT},
	{ servopwm, E_AHI_DIO15_INT},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0},
	{ nc, 0}
};

// The output mapping in use
PRIVATE opChannel* outputMapping;


/****************************************************************************
 *
 * NAME: initInputs
 *
 * DESCRIPTION: Initialise receiver inputs
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 *  None.
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void initInputs(void)
{
	// TODO - allow conditional compilation
	// or selection from flash settings
	// or make this file a template for people to put their private implementations in

	// comment out if no gps connected
	//initNmeaGps(E_AHI_UART_0,E_AHI_UART_RATE_38400);
}

/****************************************************************************
 *
 * NAME: initOutputs
 *
 * DESCRIPTION: Initialise outputs
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 *  None.
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void initOutputs()
{
	int nservos = 0;	// Count of number of configured servos

	// Set output mapping used
	//outputMapping=standard6channel;
	outputMapping = standard6channelDebug;
	//outputMapping=gps;
	//outputMapping=propsteer;

	int i;
	for (i = 0; i < 20; i++)
	{
		// For each of the configured output types
		switch (outputMapping[i].type)
		{
		case servopwm:
		{
			setServoBit(nservos++, outputMapping[i].pin);
			break;
		}
		case pwm:
		{
			initMotorPwm(outputMapping[i].pin);
			break;
		}
		case digital:
		{
			vAHI_DioSetDirection(0, outputMapping[i].pin);
			vAHI_DioSetOutput(0, outputMapping[i].pin);
			break;
		}
		case i2c:
		{
			//not implemented
			break;
		}
		case nc:
		{

			break;
		}
		}
	}

	// Initialise the servo pwm driver
	// TODO - only call if nServos > 0??
	initServoPwm(nservos);
}

/****************************************************************************
 *
 * NAME: updateOutputs
 *
 * DESCRIPTION: Update outputs with new values from Tx
 *
 * PARAMETERS:      Name            RW  Usage
 *					channelData		R	array of new chanel data
 *
 * RETURNS:
 *  None.
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void updateOutputs(uint16* channelData)
{
	// Do any mixing, control loops etc that are done in Rx

	// Set outputs

	int servo = 0;	// Index into servo array
	int i;
	for (i = 0; i < 20; i++)
	{
		// Ignore channels not received yet
		if (channelData[i] >= 4096)
			continue;

		// Iterate over each output
		switch (outputMapping[i].type)
		{
		case servopwm:
		{
			// Set a servo pwm output

			// Convert the demand to a pulse width in 1/16 of a uS
			uint32 u32Demand = channelData[i] * 16000 / 4096 + 16000;

			// Limit the pulse width to 1-2mS
			// TODO - allow travel over limits, e.g. 0.9 - 2.1
			// yep but requires careful checking of timing in servopwm.c
			if (u32Demand > 2000 * 16)
				u32Demand = 2000 * 16;
			if(u32Demand < 1000 * 16)
				u32Demand = 1000 * 16;

			// Set the demanded pulse width
			setServoDemand(servo++, u32Demand);
			break;
		}
		case pwm:
		{
			// Set the motor PWM demanded value
			setMotorPWM(outputMapping[i].pin, channelData[i] * 16000 / 4096);
			break;
		}
		case digital:
		{
			if(channelData[i] > 3000)
				vAHI_DioSetOutput(outputMapping[i].pin, 0);
			else
				vAHI_DioSetOutput(0, outputMapping[i].pin);
			break;
		}
		case i2c:
		{
			//not implemented
			break;
		}
		case nc:
		{
			break;
		}
		}
	}
}
