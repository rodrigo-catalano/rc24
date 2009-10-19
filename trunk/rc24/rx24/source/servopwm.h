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

#ifndef _SERVO_PWM_H
#define _SERVO_PWM_H

/****************************************************************************/
/***        Exported variables											  ***/
/****************************************************************************/

// Maximum recorded latency
PUBLIC extern uint32 maxActualLatency;

/****************************************************************************/
/***        Exported Function Prototypes                                  ***/
/****************************************************************************/

// Initialise the servo pwm system
PUBLIC void initServoPwm (const uint8 nServos);

// Set the DIO to use for a servo channel
PUBLIC void setServoBit (const uint16 channel, const int bit);

// Set the demanded position for a servo
PUBLIC void setServoDemand (const uint16 channel, const uint32 demand);

// Calculate an error value TODO - Explain
PUBLIC void calcSyncError (const int txTime);

// Start the servo pwm system
PUBLIC void startServoPwm (void);

// Return some time info TODO - Explain
PUBLIC int getSeqClock (void);

// Return an error rate TODO - Explain
PUBLIC uint32 getErrorRate (void);

// Set app context function called at start of each frame
PUBLIC void setFrameCallback(SW_EVENT_FN callback);

#endif
