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

#define	rx3dot2_6chan 0
#define	rx3dot2_4chan_gps 1
#define	rxsc1 2
#define rx_GB_1dot2 3
#define	rx3dot2_5chan_1wire 4


PUBLIC char* rxHardwareTypeEnumValues[rxHardwareTypeCount] =
{ "rx3.2 6 channnel", "rx3.2 4 channel + gps", "rx single cell 1.0", "rx GB 1.2","rx3.2 5 channel + 1wire" };

//PUBLIC const size_t rxHardwareTypeCount=sizeof(rxHardwareTypeEnumValues) / sizeof(rxHardwareTypeEnumValues[0];

PUBLIC uint8 rxHardwareType = 0;

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
// Set up for 5ch standard servo output + 1 wire bus on uart1
opChannel standard5channel_1wire[20] = {
	{ servopwm, E_AHI_DIO6_INT},
	{ servopwm, E_AHI_DIO7_INT},
	{ servopwm, E_AHI_DIO10_INT},
	{ servopwm, E_AHI_DIO15_INT},
	{ servopwm, E_AHI_DIO14_INT},
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


// Set up for GB rx 1.2 with 8 channels
opChannel gb1_2_8channel[20] = {
	{ servopwm, E_AHI_DIO4_INT},
	{ servopwm, E_AHI_DIO5_INT},
	{ servopwm, E_AHI_DIO6_INT},
	{ servopwm, E_AHI_DIO7_INT},
	{ servopwm, E_AHI_DIO8_INT},
	{ servopwm, E_AHI_DIO9_INT},
	{ servopwm, E_AHI_DIO10_INT},
	{ servopwm, E_AHI_DIO14_INT},
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


// setup for rx sc1 5 servos and two brushed motors
opChannel rxsc1_5servo[20] = {
	{ servopwm, E_AHI_DIO6_INT},
	{ servopwm, E_AHI_DIO7_INT},
	{ pwm, E_AHI_DIO10_INT},
	{ pwm, E_AHI_DIO13_INT},
	{ servopwm, E_AHI_DIO14_INT},
	{ servopwm, E_AHI_DIO15_INT},
	{ servopwm, E_AHI_DIO20_INT},
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
void initInputs(rxHardwareOptions* rxHardware)
{
	// TODO - allow conditional compilation
	// or selection from flash settings
	// or make this file a template for people to put their private implementations in

	if(!rxHardware->uart0InUse && rxHardwareType == rx3dot2_4chan_gps)
	{
		initNmeaGps(E_AHI_UART_0,E_AHI_UART_RATE_38400);
		rxHardware->uart0InUse=TRUE;
	}
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
void initOutputs(rxHardwareOptions* rxHardware)
{
	int nservos = 0;	// Count of number of configured servos

	rxHardware->ledBit=(1 << 17);

	rxHardware->batVoltageChannel=E_AHI_ADC_SRC_ADC_4;
	rxHardware->batVoltageMultiplier=1331; //2.4*12.2/2.2
	rxHardware->batVoltageOffset=0;

	switch(rxHardwareType)
	{
	case rx3dot2_6chan :
		outputMapping=standard6channel;
		rxHardware->ledBit=(1 << 5);
		break;
	case rx3dot2_4chan_gps :
		outputMapping=gps;
		rxHardware->ledBit=(1 << 5);
		break;
	case rx3dot2_5chan_1wire :
		outputMapping=standard5channel_1wire;
		rxHardware->ledBit=(1 << 5);
		rxHardware->oneWirePort=E_AHI_UART_1;
		rxHardware->oneWireEnabled=TRUE;
		rxHardware->uart1InUse=TRUE;
		break;
	case rxsc1 :
		outputMapping = rxsc1_5servo;
		//use internal bat monitor
		rxHardware->batVoltageChannel=E_AHI_ADC_SRC_VOLT;
		rxHardware->batVoltageMultiplier=360;
		rxHardware->batVoltageOffset=60;

		break;
	case rx_GB_1dot2 :
		outputMapping = gb1_2_8channel;
		break;
	}


	int i;
	for (i = 0; i < 20; i++)
	{
		//prevent output on uart0 pins if used
		if(rxHardware->uart0InUse)
		{
			if(outputMapping[i].pin==E_AHI_DIO6_INT || outputMapping[i].pin==E_AHI_DIO7_INT)
			{
				outputMapping[i].type=nc;
			}
		}
		//prevent output on uart1 pins if used
		if(rxHardware->uart1InUse)
		{
			if(outputMapping[i].pin==E_AHI_DIO19_INT || outputMapping[i].pin==E_AHI_DIO20_INT)
			{
				outputMapping[i].type=nc;
			}
		}
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
 *					channelData		R	array of new channel data
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
			setMotorPWM(outputMapping[i].pin, channelData[i]);
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
