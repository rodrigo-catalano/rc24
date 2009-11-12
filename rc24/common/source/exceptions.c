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
#include "exceptions.h"

#ifdef JN5148
#define BUS_ERROR *((volatile uint32 *)(0x4000000))
#define UNALIGNED_ACCESS *((volatile uint32 *)(0x4000008))
#define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x40000C0))
#else
#define BUS_ERROR *((volatile uint32 *)(0x4000008))
#define UNALIGNED_ACCESS *((volatile uint32 *)(0x4000018))
#define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x400001C))
#endif


void setExceptionMarker(resetType excep);

// event handler function prototypes
void vBusErrorhandler(void);
void vUnalignedAccessHandler(void);
void vIllegalInstructionHandler(void);

/****************************************************************************
 *
 * NAME: setExceptionMarker
 *
 * DESCRIPTION: Store a marker in the wake timer that can be read after reset
 *
 * PARAMETERS:      Name            RW  Usage
 *  				id
 *
 * RETURNS: void
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void setExceptionMarker(resetType excep)
{
	vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_1,FALSE);

#ifdef JN5148
	vAHI_WakeTimerStartLarge(E_AHI_WAKE_TIMER_1,(excep*1000)+400);
#else
	vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_1,(excep*1000)+400);
#endif
	vAHI_WakeTimerStop(E_AHI_WAKE_TIMER_1);
}

/****************************************************************************
 *
 * NAME: vBusErrorhandler
 *
 * DESCRIPTION: Handler for a bus error exception
 *
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 *
 *
 * NOTES:
 * None.
 ****************************************************************************/
void vBusErrorhandler(void)
{
	setExceptionMarker(BUSERROR);
	// Force a reset
	vAHI_SwReset();
}

/****************************************************************************
 *
 * NAME: vUnalignedAccessHandler
 *
 * DESCRIPTION: Handler for the unaligned access exception
 *
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 *
 *
 * NOTES:
 * None.
 ****************************************************************************/
void vUnalignedAccessHandler(void)
{
	setExceptionMarker(UNALIGNEDACCESS);
	// Force a reset
	vAHI_SwReset();
}

/****************************************************************************
 *
 * NAME: vIllegalInstructionHandler
 *
 * DESCRIPTION: Handler for the illegal instruction exception
 *
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 *
 *
 * NOTES:
 * None.
 ****************************************************************************/
void vIllegalInstructionHandler(void)
{
	setExceptionMarker(ILLEGALINSTRUCTION);
	// Force a reset
	vAHI_SwReset();
}
/****************************************************************************
 *
 * NAME: setExceptionHandlers
 *
 * DESCRIPTION: connect handlers for exceptions
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void setExceptionHandlers()
{
	BUS_ERROR = (uint32) vBusErrorhandler;
	UNALIGNED_ACCESS = (uint32) vUnalignedAccessHandler;
	ILLEGAL_INSTRUCTION = (uint32) vIllegalInstructionHandler;
}
/****************************************************************************
 *
 * NAME: getResetReason
 *
 * DESCRIPTION: get the reason for a reset
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
resetType getResetReason()
{
	resetType ret;
#ifdef JN5148
	ret=u64AHI_WakeTimerReadLarge(E_AHI_WAKE_TIMER_1)/1000;
#else
	ret=u32AHI_WakeTimerRead(E_AHI_WAKE_TIMER_1)/1000;
#endif
	if(ret>ILLEGALINSTRUCTION)ret=NOEXCEPTION;
	setExceptionMarker(NOEXCEPTION);
	return ret;

}
