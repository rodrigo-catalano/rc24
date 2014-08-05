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
//#include <AppQueueApi.h>
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

//processor speed
#define CLOCKSPERMICROSECOND 16
//tick timer speed
#define TICKSPERMICROSECOND 16

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

// Maximum number of output items
// TODO - arbitrary number??, not all items are servos, rename??
#define MAX_SERVOS 14

// channel hopping every 20ms over a sequence of 31 hops
#define maxSeqClock (31 * 20000 * TICKSPERMICROSECOND)

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
#define MIX_EVENT_ACTION 32
#define LAST_ACTION 64

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

typedef struct // sortable servo item, packs into a uint32 for fast swaps
{
	uint16 channel;
	uint16 value;
} servoOp;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

// Handler for tick timer, sets next servo output etc.
PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap);

// Builds ordered list of output items based on pulse width

PRIVATE void buildTickTimerQueue(void);


#ifdef JN5139
// TODO - Where is this declared - should be in a header

extern void intr_handler (void);
#endif

#if (defined JN5148 )
// Tick interrupt handlers
// TODO - Check PRIVATE doesn't break anything
#define TICK_INTR *((volatile uint32 *)(0x4000004))
#define EXT_INTR *((volatile uint32 *)(0x4000010))
PRIVATE void tick_handler(void);
PRIVATE void extern_intr_handler(void);
#endif

extern uint32 debugCount1;
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
PRIVATE volatile uint8 servoQueueIdx = 255; // Current output item index, volatile as updated in interrupt

// TODO - Explain
PRIVATE volatile int seqClock = 0;
//time since last tick timer interrupt
PRIVATE int lastActionTime = 0;
volatile uint32 sysClock = 0;

PRIVATE volatile int currentSyncError = 0; // tx sync error for current cycle

// Statistics
PRIVATE uint32 packetsReceived = 0; // Total packets received
// TODO - rename?
PRIVATE uint32 packetsCounter = 0; // Packets received this frame
// TODO - Explain
PRIVATE uint32 errorCounter = 0; // Counter

PRIVATE int servoCyleTicks=20000* TICKSPERMICROSECOND;
// the maximum interrupt latency that is corrected for
#ifdef JN5148
const uint32 maxLatency = 116; // Max recorded for jn5148 114 ticks with 16Mhz clock
#endif

#ifdef JN5168
const uint32 maxLatency = 300;
#endif

#ifdef JN5139
const uint32 maxLatency = 300;
#endif


PRIVATE SW_EVENT_FN frameStartCallback = NULL;
PRIVATE SW_EVENT_FN mixCallback = NULL;



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
	seqClock = 0;

	// Configure the tick timer
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);
	vAHI_TickTimerWrite(0);
	vAHI_TickTimerInterval(128 * 16);

	// Register the tick timer handler
#ifdef JN5148
	TICK_INTR = (uint32) tick_handler;
	EXT_INTR = (uint32) extern_intr_handler;
#endif

#ifdef JN5168
	vAHI_TickTimerRegisterCallback (vTick_TimerISR);
	vAHI_InterruptSetPriority(1<<15 ,15);
#endif

#ifdef JN5139
	vAHI_TickTimerInit (vTick_TimerISR);

	//use the audio reference low level interrupt handler
	//this is vital for jitter free servo output as
	//the standard interrupt handler will call lower
	//priority interrupts first, even if the tick timer
	//interrupted first.

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
 * NAME: setFrameCallback
 *
 * DESCRIPTION: Set app context function called shortly before servo output
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

PUBLIC void setMixCallback(SW_EVENT_FN callback)
{
	mixCallback = callback;
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
	servos[channel].demand = 1500 * TICKSPERMICROSECOND;
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
	if (currentSyncError == 0)
	{
		int error = txTime - getSeqClock();

		// Don't correct very small errors to ensure a solid servo frame rate
		// TODO - Explain values, fix magic numbers
		if (abs(error) > (100 * 16))
		{
			// Error should be corrected, so set it
			if(error<0) error+=maxSeqClock;
			currentSyncError = error;
		//	debugCount1++;
//			dbgPrintf(" %i ",error);

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
PUBLIC void startServoPwm(uint8 servoUpdateRate)
{
	switch(servoUpdateRate)
	{
	case 0: servoCyleTicks=20000* TICKSPERMICROSECOND;break;
	case 1: servoCyleTicks=10000* TICKSPERMICROSECOND;break;
	case 2: servoCyleTicks=5000* TICKSPERMICROSECOND;break;
	case 3: servoCyleTicks=2500* TICKSPERMICROSECOND;break;
	}
	// Build the output item queue
	buildTickTimerQueue();

	// Start with the first output
	servoQueueIdx = 0;

	// short pause before first interrupt
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
PUBLIC uint32 getSysClock(void)
{

	// sys time is the sum of sysClock and the tick timer
	// allow for an interrrupt between reading both parts

	uint32 msbTry1 = sysClock;
	uint32 ret = u32AHI_TickTimerRead();
	uint32 msbTry2 = sysClock;
	if (msbTry1 != msbTry2)return msbTry2;
	else return ret + msbTry1;

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



#define SWAPSERVO(a,b)\
if(servos[a].value>servos[b].value)\
{\
	temp.channel=servos[a].channel;\
	temp.value=servos[a].value;\
	servos[a].channel=servos[b].channel;\
	servos[a].value=servos[b].value;\
	servos[b].channel=temp.channel;\
	servos[b].value=temp.value;\
}\

void sort(servoOp* servos,int n)
{
	// optimised network sort
	// from http://pages.ripco.net/~jgamble/nw.html

	servoOp temp;
	switch(n)
	{
	case 0: return;
	case 1: return;
	case 2: SWAPSERVO(0, 1);
			break;
	case 3: SWAPSERVO(1, 2);
			SWAPSERVO(0, 2);
			SWAPSERVO(0, 1);
			break;
	case 4: SWAPSERVO(0, 1);
			SWAPSERVO(2, 3);
			SWAPSERVO(0, 2);
			SWAPSERVO(1, 3);
			SWAPSERVO(1, 2);
			break;
	case 5: SWAPSERVO(0, 1);
			SWAPSERVO(3, 4);
			SWAPSERVO(2, 4);
			SWAPSERVO(2, 3);
			SWAPSERVO(0, 3);
			SWAPSERVO(0, 2);
			SWAPSERVO(1, 4);
			SWAPSERVO(1, 3);
			SWAPSERVO(1, 2);
			break;
	case 6: SWAPSERVO(1, 2);
			SWAPSERVO(0, 2);
			SWAPSERVO(0, 1);
			SWAPSERVO(4, 5);
			SWAPSERVO(3, 5);
			SWAPSERVO(3, 4);
			SWAPSERVO(0, 3);
			SWAPSERVO(1, 4);
			SWAPSERVO(2, 5);
			SWAPSERVO(2, 4);
			SWAPSERVO(1, 3);
			SWAPSERVO(2, 3);
			break;
	case 7: SWAPSERVO(1, 2);
			SWAPSERVO(0, 2);
			SWAPSERVO(0, 1);
			SWAPSERVO(3, 4);
			SWAPSERVO(5, 6);
			SWAPSERVO(3, 5);
			SWAPSERVO(4, 6);
			SWAPSERVO(4, 5);
			SWAPSERVO(0, 4);
			SWAPSERVO(0, 3);
			SWAPSERVO(1, 5);
			SWAPSERVO(2, 6);
			SWAPSERVO(2, 5);
			SWAPSERVO(1, 3);
			SWAPSERVO(2, 4);
			SWAPSERVO(2, 3);
			break;
	case 8: SWAPSERVO(0, 1);
			SWAPSERVO(2, 3);
			SWAPSERVO(0, 2);
			SWAPSERVO(1, 3);
			SWAPSERVO(1, 2);
			SWAPSERVO(4, 5);
			SWAPSERVO(6, 7);
			SWAPSERVO(4, 6);
			SWAPSERVO(5, 7);
			SWAPSERVO(5, 6);
			SWAPSERVO(0, 4);
			SWAPSERVO(1, 5);
			SWAPSERVO(1, 4);
			SWAPSERVO(2, 6);
			SWAPSERVO(3, 7);
			SWAPSERVO(3, 6);
			SWAPSERVO(2, 4);
			SWAPSERVO(3, 5);
			SWAPSERVO(3, 4);
			break;
	}
}

/****************************************************************************
 *
 * NAME: buildTickTimerQueue
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
PRIVATE void buildTickTimerQueue()
{
 	//  0ms channel change and calc this
	//  start turning servos on
	//  1-2 ms turn off servos

	//time && io stuff for servos and channel change

	int i = 0; // Index into item array
	int ii; // Index into servos array
	int n;

	int stagger = 50 * TICKSPERMICROSECOND;
	int startDelay = 170 * TICKSPERMICROSECOND;

	servoOp sorted[8];
	for(ii=0;ii<numServos;ii++)
	{
		sorted[ii].channel=ii;
		sorted[ii].value=servos[ii].demand;
	}
	int totalDelay = 0; // Total servo operation time - TODO Explain

	servoQueue[i].action_time = startDelay ;
//	totalDelay+=startDelay;
	servoQueue[i].on = 0;
	servoQueue[i].off = 0;
	servoQueue[i].action_type = SERVO_CALC_ACTION | MIX_EVENT_ACTION ;

	if((seqClock)%(20000*TICKSPERMICROSECOND)==0)
	{
		servoQueue[i].action_type |= HOP_ACTION;
		servoQueue[i].action_type |= APP_EVENT_ACTION;
	}
//	else debugCount1++;
	i++;

	//sort servos based on demand
	sort(sorted,numServos);

	n = 0;

	for (ii = 0; ii < (numServos - 1); ii++)
	{
		// For each servo output item, stagger the operation from the last servo
		servoQueue[i].action_time = stagger;

		// Update the total delay - TODO could just use maths to set this
	//	totalDelay += stagger;

		// If the servo is active, set the DIO bit to use
		if (servos[sorted[ii].channel].active)
			servoQueue[i].on = servos[sorted[ii].channel].opbitmask;
		else
			servoQueue[i].on = 0;

		// Clear the off bit and set the action type
		servoQueue[i].off = 0;
		servoQueue[i].action_type = IO_ACTION;

		// Bump the output item index for the next item
		i++;
	}
	n = 0;
	// Turn last servo on and set time for first one off - TODO explain
	servoQueue[i].action_time = servos[sorted[n].channel].demand - (numServos - 1)
			* stagger;

	// TODO - Explain
	if (servos[sorted[ii].channel].active)
	{
		servoQueue[i].on = servos[sorted[ii].channel].opbitmask;
	}
	else
	{
		servoQueue[i].on = 0;
	}


	servoQueue[i].off = 0;
	servoQueue[i].action_type = IO_ACTION;
	i++;
	n++;

	// Turn off servos
	for (ii = 0; ii < numServos - 1; ii++)
	{
		servoQueue[i].action_time = servos[sorted[n].channel].demand -
		                            servos[sorted[n - 1].channel].demand +
		                            stagger;
		servoQueue[i].on = 0;
		if (servos[sorted[n - 1].channel].active)
		{
			servoQueue[i].off = servos[sorted[n - 1].channel].opbitmask;
		}
		else
		{
			servoQueue[i].off = 0;
		}
	//	totalDelay += servoQueue[i].action_time;
		servoQueue[i].action_type = IO_ACTION;
		i++;
		n++;
	}
	totalDelay=startDelay+(numServos-1)*stagger+sorted[numServos-1].value;
	// do tx/rx synchronisation correction here
	// correct cycleTime to	match received clock from tx
	//if(currentError!=0)debugCount1++;
	//currentError=0;
	int correctSeqClock = seqClock+currentSyncError+totalDelay;
	if (correctSeqClock >= maxSeqClock)
		correctSeqClock -= maxSeqClock;
	else if (correctSeqClock < 0)
		correctSeqClock += maxSeqClock;


	int frameError = (servoCyleTicks) - (correctSeqClock % (servoCyleTicks));
	if (frameError < stagger)
	{
			frameError += (servoCyleTicks);
	}


	// Turn off last servo and set time to next calc action
	servoQueue[i].action_time = frameError;
	servoQueue[i].on = 0;
	if(servos[sorted[n - 1].channel].active)
			servoQueue[i].off = servos[sorted[n - 1].channel].opbitmask;
	else servoQueue[i].off = 0;
	servoQueue[i].action_type = IO_ACTION | LAST_ACTION;
	if(currentSyncError!=0)servoQueue[i].action_type |= SYNC_ACTION;


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

	// Either turn on or off servos or change channel

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
//	vAHI_DioSetOutput(1 << 16, 0);

	vAHI_DioSetOutput(servoQueue[servoQueueIdx].on,
			servoQueue[servoQueueIdx].off);
	vAHI_TickTimerInterval(servoQueue[servoQueueIdx].action_time);

	if (latency > maxActualLatency)
		maxActualLatency = latency;

	//set sequence clock

	seqClock += lastActionTime;
	sysClock += lastActionTime;
	lastActionTime=servoQueue[servoQueueIdx].action_time;

	if (seqClock >= maxSeqClock)
		seqClock -= maxSeqClock;

	if ((servoQueue[servoQueueIdx].action_type & SYNC_ACTION) != 0)
	{

#ifdef JN5168IGNORE
		//clear transmission queue as 5168 dosen't like channel changed while transmitting

		MAC_McpsReqRsp_s sMcpsReqRsp;
		MAC_McpsSyncCfm_s sMcpsSyncCfm;
		/* Send request to remove a data frame from transaction queue */
		sMcpsReqRsp.u8Type = MAC_MCPS_REQ_PURGE;
		sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqPurge_s);
		sMcpsReqRsp.uParam.sReqPurge.u8Handle = 1;
		vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);

#endif

		seqClock += currentSyncError;
		if (seqClock >= maxSeqClock)
			seqClock -= maxSeqClock;
		else if (seqClock < 0)
			seqClock += maxSeqClock;
		currentSyncError=0;
//		reSyncToTx();
	}
	if ((servoQueue[servoQueueIdx].action_type & SERVO_CALC_ACTION) != 0)
	{
		// Calculate servo timings
		//buildServoQueue();
		buildTickTimerQueue();
	}
	if ((servoQueue[servoQueueIdx].action_type & HOP_ACTION) != 0)
	{
		//change frequency
		//use seqClock to get channel

		uint32 newchannel = getHopChannel(seqClock % maxSeqClock);

#ifdef JN5168
#else
		eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, newchannel);
#endif
		// Toggle dio 16 so tx/rx sync can be checked on scope

	//	if ((((seqClock % maxSeqClock) / (20000 * 16)) & 1) == 1)
	//		vAHI_DioSetOutput(1 << 16, 0);
	//	else
	//		vAHI_DioSetOutput(0, 1 << 16);
		errorCounter++;
		if (errorCounter >= 100)
		{
			errorCounter = 0;
			packetsReceived = packetsCounter;
			packetsCounter = 0;

		}
	}

	if ((servoQueue[servoQueueIdx].action_type & APP_EVENT_ACTION) != 0)
	{
		//todo test
		//send event to app context
		if (frameStartCallback != NULL)
			swEventQueuePush(frameStartCallback,NULL, NULL);

	}
	if ((servoQueue[servoQueueIdx].action_type & MIX_EVENT_ACTION) != 0)
	{
		//send event to app context
		if (mixCallback != NULL)
			swEventQueuePush(mixCallback,NULL, NULL);

	}
	if ((servoQueue[servoQueueIdx].action_type & LAST_ACTION) != 0)
	{
		servoQueueIdx=0;
	}
	else servoQueueIdx++;
}

#if (defined JN5148 )

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
PRIVATE void extern_intr_handler(void)
{
	//enable tick intr

	asm volatile("b.mfspr r11,r0,17;");
	asm volatile("b.ori   r11,r11,2;");
	asm volatile("b.mtspr r0,r11,17;");
	//asm volatile("b.ei");

	//call std handler

	asm volatile("b.ja 0xa1cc;");
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
PRIVATE void tick_handler(void)
{
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
}

#endif
