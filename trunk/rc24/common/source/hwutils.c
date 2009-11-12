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

#include "hwutils.h"

void cycleDelay(uint32 del)
{

#ifdef JN5148
	asm volatile("bg.addi r3,r3,-4;");
	asm volatile("bg.bgesi r3,0,-4;");
	//adjust to 1 cycle accuracy -

	asm volatile("bg.beqi r3,-4,16;");
	asm volatile("bg.beqi r3,-3,12;");
	asm volatile("bg.beqi r3,-2,8;");
	asm volatile("bg.beqi r3,-1,4;");

	asm volatile("bt.nop 0;");
#else

	//delay for del cycles + fixed overhead
	//interrupts will affect the timing

	//del passed in r3

	//loop to 4 cycle accuracy
	asm volatile("l.sfgtsi r3,3;");
	asm volatile("l.bf -4;");
	asm volatile("l.addi r3,r3,-4;");
	//adjust to 1 cycle accuracy - l.bf 8 takes an extra cycle if it jumps
	asm volatile("l.sfgesi r3,-3;");
	asm volatile("l.bf 8;");
	asm volatile("l.sfgesi r3,-2;");
	asm volatile("l.bf 8;");
	asm volatile("l.sfgesi r3,-1;");
	asm volatile("l.bf 8;");
	asm volatile("l.nop;");
	asm volatile("l.nop;");

#endif
}
/* test cycle delay
 *
 *
 * vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);
 vAHI_TickTimerWrite(0);
 vAHI_TickTimerInterval(200000);
 vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_RESTART);

 uint32 t;
 for(t=100;t<500;t+=101)
 {

 uint32 ss=u32AHI_TickTimerRead();
 cycleDelay(t);
 uint32 ee=u32AHI_TickTimerRead();
 pcComsPrintf(" %d",ee-ss);

 }
 */

uint16 u16ReadADC(uint8 channel)
{

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, channel);

	vAHI_AdcStartSample(); /* start an AtoD sample here */
	while (bAHI_AdcPoll() != 0x00)
		; /* wait for conversion complete */
	return (u16AHI_AdcRead()); /* return result */
}

uint32 flashSectorSize()
{
#ifdef JN5148
	return 0x10000;
#else
	return 0x8000;
#endif
}
uint32 flashNumSectors()
{
#ifdef JN5148
	return 8;
#else
	return 4;
#endif
}

