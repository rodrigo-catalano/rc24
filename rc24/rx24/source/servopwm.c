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

/*
 The code in this file handles the precision timing of
 servo outputs, frequency hopping and synchronising
 to the transmitter.  The tick timer is used for this
 as the interrupts that it generates cannot be interrupted.

 This relies on the interrupt handler from the Jennic wireless audio
 reference design AudioLib_JN5139R1.a

 The JN5148 has a more sophisticated interrupt controller but as
 yet there is no public documentation so
 extern_intr_handler() and tick_handler() are a temporary fix that
 try and emulate the 139 audio intr handler

 There is a variable latency in the interrupt being called
 as other interrupts, if in progress when the tick timer
 one occurs, don't relinquish control to the tick interrupt
 until a little way into their interrupt handler.

 This is corrected by reading the tick timer in the interrupt handler,
 this gives a precise measure of latency which can then be corrected for.

 */

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <stdlib.h>
#include <Printf.h>
#include <sys/param.h>

#include "swEventQueue.h"
#include "servopwm.h"
#include "config.h"
#include "intr.h"
#include "hopping.h"
#include "hwutils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

// Maximum number of output items
// TODO - arbitrary number??, not all items are servos, rename??
#define MAX_SERVOS 14

// TODO - Explain values
#define maxSeqClock (31 * 20000 * 16)

// Maximum length of output queue
// TODO - Explain extra slots, not all items are servos, rename??
#define maxServoQueueLen (4 + MAX_SERVOS * 2)

// Action operation bitmasks
#define NO_ACTION 0
#define IO_ACTION 1
#define HOP_ACTION 2
#define SYNC_ACTION 4
#define SERVO_CALC_ACTION 8
#define APP_EVENT_ACTION 16

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct // Output queue item
{
	uint32 action_type; // Type of action required for this item
	uint32 action_time; // Time period for this action
	uint32 on; // DIO bit to set on when executing this output
	uint32 off; // DIO bit to set off when executing this output ( previous servo)
} servoOpQueue;

typedef struct // Servo output item
{
	uint32 opbitmask; // DIO bit for this output
	uint32 demand; // Demanded value for this output
	bool active; // TRUE if this output is to be set
} servoOutputs;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

// Handler for tick timer, sets next servo output etc.
PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap);

// Builds ordered list of output items based on pulse width
PRIVATE void buildServoQueue(void);

#if (JENNIC_CHIP_FAMILY != JN514x)
// TODO - Where is this declared - should be in a header
extern void intr_handler (void);
#endif

// Tick interrupt handlers
// TODO - Check PRIVATE doesn't break anything
PRIVATE void tick_handler(void);
PRIVATE void extern_intr_handler(void);

/****************************************************************************/
/***        Exported Variables											  ***/
/****************************************************************************/

PUBLIC uint32 maxActualLatency = 0; // Maximum recorded latency

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

// TODO - not all items are servos, rename??
PRIVATE servoOpQueue servoQueue[maxServoQueueLen]; // The array of queued action items
PRIVATE servoOutputs servos[MAX_SERVOS]; // The array of servo output items
PRIVATE uint8 numServos; // Number of actual servos in use
// TODO - not all items are servos, rename??
PRIVATE uint32 maxServoQueueIdx = 0; // The biggest output item index expected
PRIVATE volatile uint8 servoQueueIdx = 255; // Current output item index, volatile as updated in interrupt
// TODO - rename, size according to MAX_SERVOS??
PRIVATE uint32 lat[10]; // Latency of servo output
// TODO - Explain
PRIVATE int seqClock = 0;
// TODO - rename??
PRIVATE uint32 currentError = 0; // Latency error for current cycle

// Statistics
PRIVATE uint32 packetsReceived = 0; // Total packets received
// TODO - rename?
PRIVATE uint32 packetsCounter = 0; // Packets received this frame
// TODO - Explain
PRIVATE uint32 errorCounter = 0; // Counter
const uint32 maxLatency = 300; // Max recorded for jn5148 114 ticks with 16Mhz clock
// So this could be reduced

PRIVATE SW_EVENT_FN frameStartCallback = NULL;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: initServoPwm
 *
 * DESCRIPTION: Initialises the servo PWM system
 *
 * PARAMETERS:      Name            RW  Usage
 * 					nServos			R	the number of servos to be handled
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PUBLIC void initServoPwm(const uint8 nServos)
{
	// Record the numer of configured servos
	numServos = nServos;

	// Configure the tick timer
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);
	vAHI_TickTimerWrite(0);
	// TODO - fix magic numbers
	// TODO - replicated but different in startServoPwm
	vAHI_TickTimerInterval(128 * 16);

	// Register the tick timer handler
#if (JENNIC_CHIP_FAMILY == JN514x)
	// TODO - fix commented code
	//vAHI_TickTimerRegisterCallback (vTick_TimerISR);
#else
	vAHI_TickTimerInit (vTick_TimerISR);
#endif

	// Calculate the maximum length of the output item queue
	// TODO - fix magic numbers
	maxServoQueueIdx = (nServos * 2) + 4;

	// Calculate the sequence clock
	// TODO - Explain, fix magic numbers
	seqClock = maxSeqClock - (5000 * 16);

	//use the audio reference low level interrupt handler
	//this is vital for jitter free servo output as
	//the standard interrupt handler will call lower
	//priority interrupts first, even if the tick timer
	//interrupted first.
#if (JENNIC_CHIP_FAMILY == JN514x)

#define TICK_INTR *((volatile uint32 *)(0x4000004))
#define EXT_INTR *((volatile uint32 *)(0x4000010))

	TICK_INTR = (uint32) tick_handler;
	EXT_INTR = (uint32) extern_intr_handler;

#else
	MICRO_SET_VSR_HANDLER(MICRO_VSR_NUM_TICK,intr_handler);
	MICRO_SET_VSR_HANDLER(MICRO_VSR_NUM_EXT, intr_handler);
#endif

	// Clear any pending tick timer interrupts and enable them
	vAHI_TickTimerIntPendClr();
	vAHI_TickTimerIntEnable(TRUE);
}
/****************************************************************************
 *
 * NAME: setFrameCallback
 *
 * DESCRIPTION: Set app context function called at start of each frame
 *
 * PARAMETERS:      Name            RW  Usage
 * 					callback		W	callback function
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/

PUBLIC void setFrameCallback(SW_EVENT_FN callback)
{
	frameStartCallback = callback;
}

/****************************************************************************
 *
 * NAME: setServoBit
 *
 * DESCRIPTION: Set the DIO bit for a servo channel
 *
 * PARAMETERS:      Name            RW  Usage
 * 					channel			R	the servo channel
 *					bit				R	DIO bit for servo channel
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PUBLIC void setServoBit(const uint16 channel, const int bit)
{
	// Record the DIO bit and set it to an output
	servos[channel].opbitmask = bit;
	vAHI_DioSetDirection(0, bit);

	// Turn the channel off
	servos[channel].active = FALSE;

	// Value never sent to servo but needed so timing loop operates
	// TODO - fix magic numbers
	servos[channel].demand = 1500 * 16;
}

/****************************************************************************
 *
 * NAME: setServoDemand
 *
 * DESCRIPTION: Return some error rate TODO - Explain
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	ErrorRate
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PUBLIC void setServoDemand(const uint16 channel, const uint32 demand)
{
	// Record the demanded value, set the channel active
	servos[channel].demand = demand;
	servos[channel].active = TRUE;
}

/****************************************************************************
 *
 * NAME: calcSyncError
 *
 * DESCRIPTION: Calculate some error value TODO - Explain
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 *  None.
 *
 ****************************************************************************/
PUBLIC void calcSyncError(const int txTime)
{
	// Do nothing if last error has not been corrected
	if (currentError == 0)
	{
		int error = txTime - getSeqClock();

		// TODO - fix commented code
		// vPrintf(" %d",error);

		// Don't correct very small errors to ensure a solid servo frame rate
		// TODO - Explain values, fix magic numbers
		if (abs(error) > (100 * 16))
		{
			// Error should be corrected, so set it
			currentError = error;
		}
	}

	// Convenient place to count received packets
	packetsCounter++;
}

/****************************************************************************
 *
 * NAME: startServoPwm
 *
 * DESCRIPTION: Starts the servo PWM state machine
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PUBLIC void startServoPwm(void)
{
	// Build the output item queue
	buildServoQueue();

	// Start with the first output
	servoQueueIdx = 0;

	// TODO - Seems to replicate (slightly different) code in initServoPwm
	vAHI_TickTimerWrite(0);
	// TODO - fix magic numbers
	// TODO - why is this different from [1] above
	vAHI_TickTimerInterval(64 * 16);

	// Let the timer go
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_RESTART);
}

/****************************************************************************
 *
 * NAME: getSeqClock
 *
 * DESCRIPTION: Return the sequence clock TODO - Explain
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	Sequence clock
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PUBLIC int getSeqClock(void)
{
	// TODO - Better explanation
	// seq time is the sum of seqClock and the tick timer
	// allow for an interrrupt between reading both parts

	int msbTry1 = seqClock;
	int ret = u32AHI_TickTimerRead();
	int msbTry2 = seqClock;
	if (msbTry1 != msbTry2)
	{
		// an interrupt has occured so the new seqClock is
		// as good a value as any as the tick timer will have reset
		return msbTry2 % maxSeqClock;
	}
	else
	{
		return (ret + msbTry1) % maxSeqClock;
	}
}

/****************************************************************************
 *
 * NAME: getErrorRate
 *
 * DESCRIPTION: Accessor for some error rate TODO - Explain
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	ErrorRate
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
// TODO - rename function or var??
PUBLIC uint32 getErrorRate(void)
{

	return packetsReceived;
}

/****************************************************************************/
/***        Local Functions                                            	 ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: compServo
 *
 * DESCRIPTION: Sort comparator for servo positions
 *
 * PARAMETERS:      Name            RW  Usage
 * 					p1				R	first index to sort
 *					p2				R	second index to sort
 *
 * RETURNS:
 * 	ErrorRate
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
PRIVATE int compServo(const void * p1, const void * p2)
{
	const uint8 s1 = *((uint8*) p1);
	const uint8 s2 = *((uint8*) p2);

	if (servos[s1].demand > servos[s2].demand)
		return 1;
	if (servos[s1].demand < servos[s2].demand)
		return -1;
	// Demands are equal, order by index to get a stable sort
	return ((s1 > s2) - (s1 < s2));
}

/****************************************************************************
 *
 * NAME: reSyncToTx
 *
 * DESCRIPTION: Called as last output action of frame to set next tick at start of next frame
 *
 * PARAMETERS:      Name            RW  Usage
 * 	None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
// TODO - rename??
PRIVATE void reSyncToTx(void)
{
	// called in interrupt

	// called in intr so tick timer is very small
	// set current seq clock

	// what will seq clock be read as?
	// at end of time period 1 -> 21 ms

	// what if correction calc is interrupted
	// getseqclock could change after being read and new correction will be wrong
	// set correction method
	// if pending correction don't calculate new one

	// we have send time  - getSeqClock will return correctly
	// intr could happen after this and leave wrong correction for next time
	// check correction status at beginning and end

	seqClock += currentError;
	if (seqClock >= maxSeqClock)
		seqClock -= maxSeqClock;
	else if (seqClock < 0)
		seqClock += maxSeqClock;

	// TODO - Fix magic numbers
	int frameError = (20000 * 16) - (seqClock % (20000 * 16));
	if (frameError < (1000 * 16))
		frameError += (20000 * 16);

	servoQueue[servoQueueIdx].action_time = frameError;
	vAHI_TickTimerInterval(servoQueue[servoQueueIdx].action_time);

	// turn this into -4 to +16ms

	// correct every 20ms
	// hop action uses clock to choose freq
	// say corection is 115 ms could mean we should be in the middle of servo op

	// delay so that we will be there within a frame
	// if we need to be at 7ms and we are at 15
	// we need to be at 7ms in 20ms time
	// if we do nothing we will be at 15 ms in 20 ms
	// so delay needs increasing by 15-7 = 8ms
	//

	// vary time to change -4ms to +16ms

	// big changes should only happen once per tx or rx reset
	// miss servo op for 1 frame
	// frequency could be wrong for 16ms but we have just had valid data

	// Permit correction in next frame
	currentError = 0;
}

/****************************************************************************
 *
 * NAME: buildServoQueue
 *
 * DESCRIPTION: Builds the entries for the output state machine
 *
 * PARAMETERS:      Name            RW  Usage
 * 	None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 * 	None.
 *
 ****************************************************************************/
// TODO - rename, not just servos
PRIVATE void buildServoQueue()
{

	//  repeated the length of the hop sequence

	//  0ms channel change
	//  5ms channel change

	//  calc servo times or copy across from non interrupt code?
	//

	//  6ms serov op
	//  10ms channel change
	//  15ms channel change & resync

	//time && io stuff for servos and channel change

	int i = 0;
	int ii;
	int n;
	uint32 stagger = 64* 16 ;
	uint8 sorted[8] =
	{ 0, 1, 2, 3, 4, 5, 6, 7 };
	uint32 totalDelay = 0;

	//sort servos based on demand

	qsort(sorted, numServos, sizeof(uint8), compServo);

	//   vPrintf("#%d %d#",sorted[0],sorted[1]);
	//0 ms
	servoQueue[i].action_time = 5000* 16 ;servoQueue[i].on=0;
	servoQueue[i].off=0;
	servoQueue[i].action_type=HOP_ACTION | APP_EVENT_ACTION;

	i++;
	// 5ms
	servoQueue[i].action_time=1000*16;
	servoQueue[i].on=0;
	servoQueue[i].off=0;
	servoQueue[i].action_type=SERVO_CALC_ACTION;// | HOP_ACTION;
	i++;
	n=0;

	//6ms
	for(ii=0;ii<numServos-1;ii++)
	{
		servoQueue[i].action_time=stagger;
		totalDelay+=servoQueue[i].action_time;
		if(servos[sorted[ii]].active)servoQueue[i].on=servos[sorted[ii]].opbitmask;
		else servoQueue[i].on=0;
		servoQueue[i].off=0;
		servoQueue[i].action_type=IO_ACTION;
		i++;
	}
	n=0;
	//turn last servo on and set time for first one off
	servoQueue[i].action_time=servos[sorted[n]].demand-(numServos-1)*stagger;

	if(servos[sorted[ii]].active)
	{
		servoQueue[i].on=servos[sorted[ii]].opbitmask;
	}
	else
	{
		servoQueue[i].on=0;
	}
	totalDelay+=servoQueue[i].action_time;

	servoQueue[i].off=0;
	servoQueue[i].action_type=IO_ACTION;
	i++;
	n++;

	//turn off servos
	for(ii=0;ii<numServos-1;ii++)
	{
		servoQueue[i].action_time=servos[sorted[n]].demand-servos[sorted[n-1]].demand+stagger;
		servoQueue[i].on=0;
		if(servos[sorted[n-1]].active)
		{
			servoQueue[i].off=servos[sorted[n-1]].opbitmask;
		}
		else
		{
			servoQueue[i].off=0;
		}
		totalDelay+=servoQueue[i].action_time;
		servoQueue[i].action_type=IO_ACTION;
		i++;
		n++;
	}

	//servoQueue[i].action_time=18000*16;
	//turn off last servo and set time for 10ms intr
	servoQueue[i].action_time=4000*16-totalDelay;

	servoQueue[i].on=0;
	servoQueue[i].off=servos[sorted[n-1]].opbitmask;
	servoQueue[i].action_type=IO_ACTION;

	i++;
	//10ms
	servoQueue[i].action_time=5000*16;
	servoQueue[i].on=0;
	servoQueue[i].off=0;
	servoQueue[i].action_type=NO_ACTION;//  HOP_ACTION;

	i++;
	//15ms
	servoQueue[i].action_time=5000*16;
	servoQueue[i].on=0;
	servoQueue[i].off=0;
	servoQueue[i].action_type=SYNC_ACTION;//  HOP_ACTION | SYNC_ACTION;

	/*
	 uint32 tt=0;
	 for(i=0;i<maxServoQueueIdx;i++)
	 {
	 tt+=servoQueue[i].action_time;
	 //       vPrintf(" %d %d %d ",servoQueue[i].action_time,servoQueue[i].on,servoQueue[i].off);

	 }
	 if(tt!=320000)
	 {
	 vPrintf("k %d",tt);
	 }
	 */
}

/****************************************************************************
 *
 * NAME: vTick_TimerISR
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 *  None.
 *
 ****************************************************************************/
PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap)
{

	//either turn on or off servos or change channel

	//correct for variation in latency
	uint32 latency = u32AHI_TickTimerRead();

	if (latency < maxLatency)
	{
		uint32 del = maxLatency - latency;
		//delay for del us to correct latency
		cycleDelay(del);
	}
	else
	{
		//should never happen - flag up error in debug build
		//      vPrintf("~%d\r\n",latency);
		//maxActualLatency=latency;
	}

	//  latency=u32AHI_TickTimerRead();

	vAHI_DioSetOutput(servoQueue[servoQueueIdx].on,
			servoQueue[servoQueueIdx].off);
	vAHI_TickTimerInterval(servoQueue[servoQueueIdx].action_time);

	if (latency > maxActualLatency)
		maxActualLatency = latency;

	//set sequence clock

	int lastIdx = servoQueueIdx - 1;
	if (lastIdx < 0)
		lastIdx = maxServoQueueIdx - 1;

	seqClock += servoQueue[lastIdx].action_time;
	//    if(seqClock>=maxSeqClock)seqClock=0;

	//if(seqClock>maxSeqClock)vPrintf(" ddd%d",seqClock);

	if (seqClock >= maxSeqClock)
		seqClock -= maxSeqClock;

	if ((servoQueue[servoQueueIdx].action_type & SYNC_ACTION) != 0)
	{
		reSyncToTx();
	}
	if ((servoQueue[servoQueueIdx].action_type & HOP_ACTION) != 0)
	{
		//change frequency
		// seqClock%(5000*16);
		//use seqClock to get channel

		uint32 newchannel = getHopChannel(seqClock % maxSeqClock);

		eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, newchannel);

		//toggle dio 16 so tx/rx sync can be checked on scope

		if ((((seqClock % maxSeqClock) / (20000* 16 ))&1) == 1)
				vAHI_DioSetOutput (
1			<<16,0);
			else
			vAHI_DioSetOutput(0,1<<16);

		}
		if((servoQueue[servoQueueIdx].action_type & SERVO_CALC_ACTION) !=0)
		{
			//calculate servo timings
			buildServoQueue();
		}
		if((servoQueue[servoQueueIdx].action_type & APP_EVENT_ACTION) !=0)
		{
			//todo test
			//send event to app context
			if(frameStartCallback!=NULL)swEventQueuePush(frameStartCallback,NULL);

		}
		servoQueueIdx++;

		if(servoQueueIdx>=maxServoQueueIdx)
		{
			servoQueueIdx=0;

			errorCounter++;
			if(errorCounter>=100)
			{
				errorCounter=0;
				packetsReceived=packetsCounter;
				packetsCounter=0;

			}
			//       vPrintf("z");
		}
		//   lat[servoQueueIdx]=latency;
	}

	/****************************************************************************
	 *
	 * NAME: extern_intr_handler
	 *
	 * DESCRIPTION:
	 *
	 * PARAMETERS:      Name            RW  Usage
	 *  None.
	 *
	 * RETURNS:
	 * 	None.
	 *
	 * NOTES:
	 *  None.
	 *
	 ****************************************************************************/
PRIVATE void extern_intr_handler()
{
#if(JENNIC_CHIP_FAMILY == JN514x)

	//enable tick intr

	asm volatile("b.mfspr r11,r0,17;");
	asm volatile("b.ori   r11,r11,2;");
	asm volatile("b.mtspr r0,r11,17;");
	//asm volatile("b.ei");

	//call std handler

	asm volatile("b.ja 0xa1cc;");
#endif
}

/****************************************************************************
 *
 * NAME: tick_handler
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *  None.
 *
 * RETURNS:
 * 	None.
 *
 * NOTES:
 *  None.
 *
 ****************************************************************************/
PRIVATE void tick_handler()
{
#if(JENNIC_CHIP_FAMILY == JN514x)
	//temp fix until a more maintainable jennic solution appears

	/*
	 * called before our intr routine by rom intr routine
	 292 bg.sw r1 r1 16365 //-19
	 296 bn.addi r1 r1 176 //-80
	 299 bn.sw r3 r1 3
	 302 bn.sw r4 r1 4
	 305 bn.sw r5 r1 5
	 308 bn.sw r6 r1 6
	 311 bn.sw r7 r1 7
	 314 bn.sw r8 r1 8
	 317 bn.sw r9 r1 9
	 320 bn.sw r11 r1 11
	 323 bn.sw r13 r1 13
	 326 bn.sw r15 r1 15
	 329 bn.sw r0 r1 0
	 332 bn.sw r2 r1 2
	 335 bn.sw r10 r1 10
	 338 bn.sw r12 r1 12
	 341 bn.sw r14 r1 14
	 344 bw.mfspr r5 r0 32
	 350 bn.sw r5 r1 18
	 353 bw.mfspr r5 r0 64
	 359 bn.sw r5 r1 17
	 362 bw.mfspr r5 r0 48
	 368 bn.sw r5 r1 19
	 371 bn.ori r4 r0 5
	 374 bn.sw r4 r1 16
	 377 bw.ori r5 r0 67108868
	 383 bn.lwz r5 r5 0
	 386 bn.or r3 r0 r1
	 389 bn.jr r5
	 //jump to here
	 */

	//do critical stuff

	asm volatile("bn.or r11,r3,r3;");

	//clear tick intr flag
	asm volatile("b.mfspr r4,r0,20480;");
	asm volatile("b.andi r4,r4,0xefffffff;");
	asm volatile("b.mtspr r0,r4,20480;");
	//	vAHI_TickTimerIntPendClr();

	vTick_TimerISR(0, 0);

	asm volatile("bn.or r1,r11,r11;");

	//enable interrupts
	//	asm volatile("b.ei");

	//restore everything that was saved

	asm volatile("b.lwz r5,76(r1)");
	asm volatile("bw.mtspr r0, r5, 48;");

	asm volatile("b.lwz r5, 68(r1);");
	asm volatile("bw.mtspr r0, r5, 64;");

	asm volatile("b.lwz r5, 72(r1);");
	asm volatile("bw.mtspr r0, r5, 32;");

	asm volatile("b.lwz r14, 56(r1);");
	asm volatile("b.lwz r12, 48(r1);");
	asm volatile("b.lwz r10, 40(r1);");
	asm volatile("b.lwz r2, 8(r1);");
	//	asm volatile("b.lwz r0, 0(r1);");
	asm volatile("b.lwz r15, 60(r1);");
	asm volatile("b.lwz r13, 52(r1);");
	asm volatile("b.lwz r11, 44(r1);");
	asm volatile("b.lwz r9, 36(r1);");
	asm volatile("b.lwz r8, 32(r1);");
	asm volatile("b.lwz r7, 28(r1);");
	asm volatile("b.lwz r6, 24(r1);");
	asm volatile("b.lwz r5, 20(r1);");
	asm volatile("b.lwz r4, 16(r1);");
	asm volatile("b.lwz r3, 12(r1);");
	asm volatile("b.lwz r1, 4(r1);");

	//return to app context
	asm volatile("bt.rfe;");
#endif
}
