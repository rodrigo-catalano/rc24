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

#include "motorpwm.h"
#include "config.h"

//todo make frequency selectable and support timer 2 on JN5148
void initMotorPwm(uint32 pin)
{
    uint8 timer;
    if(pin==E_AHI_DIO10_INT)timer=E_AHI_TIMER_0;
    else if(pin==E_AHI_DIO13_INT)timer=E_AHI_TIMER_1;
    else return;
/* set up timer  PWM */
    int period=1000*16; //1khz
    vAHI_TimerEnable(timer,
                     0,
                     FALSE,
                     FALSE,
                     TRUE);

    vAHI_TimerClockSelect(timer,
                          FALSE,
                          TRUE);

    vAHI_TimerStartRepeat(timer,
                          period,       // low period (space)
                          period);      // period

}
void setMotorPWM(int channel, uint16 ontime)
{
    int period=1000*16; //1khz

    if(channel==E_AHI_DIO10_INT)WRITE_REG32(0x50000004,period-ontime);
    else WRITE_REG32(0x60000004,period-ontime);
}
