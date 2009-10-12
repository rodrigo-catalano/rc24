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
servo outputs, frequency hopping and syncronising
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

#include "servopwm.h"
#include <Printf.h>
#include "config.h"
#include "intr.h"
#include "hopping.h"
#include "swEventQueue.h"
#include "hwutils.h"

PRIVATE void buildServoQueue(void);
PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap);
PUBLIC void tick_handlerC(void);
PRIVATE int compServo(const void*, const void*);
PRIVATE void reSyncToTx(void);

extern void frameStartEvent(void* buff);

#define NO_ACTION 0
#define IO_ACTION 1
#define HOP_ACTION 2
#define SYNC_ACTION 4
#define SERVO_CALC_ACTION 8
#define APP_EVENT_ACTION 16


typedef struct
{
    uint32  action_type;
    uint32  action_time;
    uint32  on;
    uint32  off;


}servoOpQueue;

typedef struct
{
    uint32 opbitmask;
    uint32 demand;
    bool active;
}servoOutputs;

#define MAX_SERVOS 14

#define maxSeqClock (31*20000*16)

#define maxServoQueueLen (4+MAX_SERVOS*2)

extern void intr_handler(void);

servoOutputs servos[MAX_SERVOS];
servoOpQueue servoQueue[maxServoQueueLen];
uint8 numServos;
volatile uint8 servoQueueIdx=255;

uint32 maxServoQueueIdx=0;

 uint32 lat[10];
 uint32 istore;


 //max recorded for jn5148 114 ticks with 16Mhz clock
 //so this could be reduced
 uint32 maxLatency=300;

 int seqClock=0;

 uint32 currentError=0;

 uint32 packetsReceived=0;
 uint32 packetsCounter=0;
 uint32 errorCounter=0;

 uint32 maxActualLatency=0;

 PUBLIC void tick_handler(void);
 PUBLIC void extern_intr_handler(void);

PUBLIC uint32 getErrorRate()
{
     return packetsReceived;
}

PUBLIC uint8 servoIdx()
{
    return servoQueueIdx;
}
PUBLIC void initServoPwm(uint8 nServos)
{
    numServos=nServos;

    servoOutputs servos[MAX_SERVOS];

    vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);
    vAHI_TickTimerWrite(0);
    vAHI_TickTimerInterval(128*16);

#if (JENNIC_CHIP_FAMILY == JN514x)
//   vAHI_TickTimerRegisterCallback(vTick_TimerISR);

#else
    vAHI_TickTimerInit(vTick_TimerISR);
#endif


    maxServoQueueIdx=nServos*2+4;

    seqClock=maxSeqClock-(5000*16);

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

    vAHI_TickTimerIntPendClr();
    vAHI_TickTimerIntEnable(TRUE);


}
PUBLIC void setServoBit(uint16 channel,int bit)
{
    servos[channel].opbitmask=bit;
    vAHI_DioSetDirection(0,bit);
    servos[channel].active=FALSE;
    //value never sent to servo but needed so timing loop operates
    servos[channel].demand=1500*16;

}
PUBLIC void setServoDemand(uint16 channel,uint32 dem)
{
    servos[channel].demand=dem;
    servos[channel].active=TRUE;

}
PRIVATE int compServo(const void * p1, const void * p2)
{
    uint8 s1=*((uint8*)p1);
    uint8 s2=*((uint8*)p2);

    if(servos[s1].demand>servos[s2].demand)return 1;
    if(servos[s1].demand<servos[s2].demand)return -1;
    return 0;
}
PUBLIC void calcSyncError(int txTime)
{
    //do nothing if last error has not been corrected
    if(currentError==0)
    {
        uint32 n=getSeqClock();
        int error=txTime-n;
        //don't correct very small errors to ensure a solid servo frame rate

       // vPrintf(" %d",error);

        if(abs(error)>100*16)
        {
            currentError=error;
        }
    }
    //convienient place to count received packets
    packetsCounter++;
}

PRIVATE void reSyncToTx()
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
    // check correction status at begining and end

    //

    seqClock+=currentError;
    if(seqClock>=maxSeqClock)seqClock-=maxSeqClock;
    else if(seqClock<0)seqClock+=maxSeqClock;

    int frameError=20000*16-seqClock%(20000*16);
    if(frameError<1000*16) frameError+=20000*16;

    servoQueue[servoQueueIdx].action_time=frameError;
    vAHI_TickTimerInterval(servoQueue[servoQueueIdx].action_time);


    //turn this into -4 to +16ms

    // correct every 20ms
    // hop action uses clock to choose freq
    // say corection is 115 ms could mean we should be in the middle of servo op

    // delay so that we will be there within a frame
    // if we need to be at 7ms and we are at 15
    // we need to be at 7ms in 20ms time
    // if we do nothing we will be at 15 ms in 20 ms
    // so delay needs increasing by 15-7 = 8ms
    //

    // vary time to change   -4ms to +16ms

    // big changes should only happen once per tx or rx reset
    // miss servo op for 1 frame
    // frequency could be wrong for 16ms but we have just had valid data

    // only allow 1 correction per 20ms
    currentError=0;

}


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

    int i=0;
    int ii;
    int n;
    uint32 stagger = 64*16;
    uint8 sorted[8]={0,1,2,3,4,5,6,7};
    uint32 totalDelay=0;

//sort servos based on demand

    qsort(sorted,numServos,sizeof(uint8),compServo);

 //   vPrintf("#%d %d#",sorted[0],sorted[1]);
//0 ms
    servoQueue[i].action_time=5000*16;
    servoQueue[i].on=0;
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


PUBLIC void startServoPwm()
{

    buildServoQueue();


    servoQueueIdx=0;

    vAHI_TickTimerWrite(0);
    vAHI_TickTimerInterval(64*16);

    vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_RESTART);

}


PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap)
{

	//either turn on or off servos or change channel

    //correct for variation in latency
    uint32 latency=u32AHI_TickTimerRead();

    if(latency<maxLatency)
    {
        uint32 del=maxLatency-latency;
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

    vAHI_DioSetOutput(servoQueue[servoQueueIdx].on,servoQueue[servoQueueIdx].off);
    vAHI_TickTimerInterval(servoQueue[servoQueueIdx].action_time);

  	if(latency>maxActualLatency)maxActualLatency=latency;

//set sequence clock

    int lastIdx=servoQueueIdx-1;
    if(lastIdx<0)lastIdx=maxServoQueueIdx-1;

    seqClock+=servoQueue[lastIdx].action_time;
//    if(seqClock>=maxSeqClock)seqClock=0;

//if(seqClock>maxSeqClock)vPrintf(" ddd%d",seqClock);

    if(seqClock>=maxSeqClock)seqClock-=maxSeqClock;



    if((servoQueue[servoQueueIdx].action_type & SYNC_ACTION) !=0)
    {
        reSyncToTx();
    }
    if((servoQueue[servoQueueIdx].action_type & HOP_ACTION) !=0)
    {
        //change frequency
        // seqClock%(5000*16);
        //use seqClock to get channel

        uint32 newchannel=getHopChannel(seqClock%maxSeqClock);

        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, newchannel);
        //toggle dio 16 so tx/rx sync can be checked on scope
        if((((seqClock%maxSeqClock)/(20000*16))&1)==1)   vAHI_DioSetOutput(1<<16,0);
        else vAHI_DioSetOutput(0,1<<16);


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
 //       swEventQueuePush(frameStartEvent,NULL);
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


PUBLIC void extern_intr_handler()
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


PUBLIC void tick_handler()
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

	 vTick_TimerISR(0,0);


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



PUBLIC int getSeqClock()
{
    //seq time is the sum of seqClock and the tick timer
    //allow for an interrrupt between reading both parts

    int ret;
    int msbTry1=seqClock;
    int msbTry2;
    ret=u32AHI_TickTimerRead();
    msbTry2=seqClock;
    if(msbTry1!=msbTry2)
    {
        // an interrupt has occured so the new seqClock is
        // as good a value as any as the tick timer will have reset
        return msbTry2%maxSeqClock;
    }
    else
    {
        return (ret+msbTry1)%maxSeqClock;
    }
}


