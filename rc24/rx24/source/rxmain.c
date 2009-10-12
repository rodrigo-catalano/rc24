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


#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Printf.h>
#include <stdlib.h>


#include "config.h"
#include "swEventQueue.h"
#include "servopwm.h"
#include "hopping.h"
#include "nmeagps.h"
#include "myrxs.h"
#include "routedmessage.h"
#include "store.h"
#include "pcComs.h"
#include "hwutils.h"
#include "codeupdate.h"
#include "radiocoms.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_ACTIVE_SCANNING,
    E_STATE_ASSOCIATING,
    E_STATE_ASSOCIATED,

}teState;

typedef struct
{
    teState eState;
    uint8   u8Channel;
    uint8   u8TxPacketSeqNb;
    uint8   u8RxPacketSeqNb;
    uint16  u16Address;
    uint16  u16PanId;
    uint8   u8ChannelSeqNo;

}tsEndDeviceData;

uint32 u32TimerTicks=0;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vProcessEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len);

PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, bool broadcast);

PUBLIC void rPutC(unsigned char c);

void frameStartEvent(void* buff);



void rxSendRoutedMessage(uint8* msg,uint8 len,uint8 toCon);
void txComsSendRoutedPacket(uint8* msg, uint8 len);
void rxHandleRoutedMessage(uint8* msg,uint8 len,uint8 fromCon);

void loadDefaultSettings(void);
void loadSettings(void);
void storeSettings(void);


#define CONTX 0
#define CONPC 1

//PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE tsEndDeviceData sEndDeviceData;


uint16 thop=0;

uint32 intstore;

uint32 rxdpackets=0;
uint32 frameperiods=0;
uint32 resyncs=0;
uint32 reportNo=0;
uint8 txLinkQuality=0;

uint32 frameCounter=0;//good for almost 3 years at 50/sec

unsigned char radiodebugbuf[64];
int radiodebugidx=1;

//todo make debug messages routed so they can be sent anywhere

#define UARTDEBUG 0
#define RADIODEBUG 1

//int debugchannel = RADIODEBUG;
int debugchannel = UARTDEBUG;


//store for received channel data
uint16 rxDemands[20];

nmeaGpsData gpsData;

uint8 returnPacketIdx;

uint32 txMACh=0;
uint32 txMACl=0;

pcCom pccoms;

extern uint32 maxActualLatency;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/



extern void intr_handler(void);

PUBLIC void dumpCode(void* addr, uint32 len)
{
  //     uint32* code=(uint32*)addr;
		uint8* code=(uint8*)addr;
		uint32 i;
        for(i=0;i<len;i++)
        {
                vPrintf("%x",*(code++));
        }
        vPrintf("done");
        while(1==1);
}

#if (JENNIC_CHIP_FAMILY == JN514x)
	#define BUS_ERROR *((volatile uint32 *)(0x4000000))
	#define UNALIGNED_ACCESS *((volatile uint32 *)(0x4000008))
	#define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x40000C0))
#else
	#define BUS_ERROR *((volatile uint32 *)(0x4000008))
	#define UNALIGNED_ACCESS *((volatile uint32 *)(0x4000018))
	#define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x400001C))
#endif


// event handler function prototypes
PUBLIC void vBusErrorhandler(void);
PUBLIC void vUnalignedAccessHandler(void);
PUBLIC void vIllegalInstructionHandler(void);

PUBLIC void vBusErrorhandler(void)
{
	volatile uint32 u32BusyWait = 1600000;
	// log the exception
	pcComsPrintf("\nBus Error Exception");
	// wait for the UART write to complete

	while(u32BusyWait--){}

	vAHI_SwReset();
}
PUBLIC void vUnalignedAccessHandler (void)
{
	volatile uint32 u32BusyWait = 1600000;
	// log the exception
	pcComsPrintf("\nUnaligned Error Exception");
	// wait for the UART write to complete
	while(u32BusyWait--){}

	vAHI_SwReset();
}
PUBLIC void vIllegalInstructionHandler(void)
{
	volatile uint32 u32BusyWait = 1600000;
	// log the exception
	pcComsPrintf("\nIllegal Instruction Error Exception");
	// wait for the UART write to complete
	while(u32BusyWait--){}

	vAHI_SwReset();
}

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Initialises system and runs
 * main loop.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/



PUBLIC void AppColdStart(void)
{

#if (JENNIC_CHIP_FAMILY == JN514x)
	//todo use watch dog and probably disable brownout reset
	vAHI_WatchdogStop();
#else
    vAppApiSetBoostMode(TRUE);
#endif

    switch (debugchannel)
    {
        case UARTDEBUG :// vInitPrintf((void*)vPutC);
						break;
        case RADIODEBUG : vInitPrintf((void*)rPutC);
                            radiodebugbuf[0]=255;
                            break;
    }
    setHopMode(hoppingRxStartup);

 //   loadSettings();

    vInitSystem();
    if(debugchannel==UARTDEBUG)
    {
        //don't use uart pins for servo op
    //    vUART_Init(FALSE);

        initPcComs(&pccoms,CONPC,0,rxHandleRoutedMessage);

        vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);

    }


    BUS_ERROR = (uint32) vBusErrorhandler;
    UNALIGNED_ACCESS = (uint32) vUnalignedAccessHandler;
    ILLEGAL_INSTRUCTION = (uint32) vIllegalInstructionHandler;


   // bAHI_SetClockRate(3);

    pcComsPrintf("rx24 2.10");

    setRadioDataCallback(rxHandleRoutedMessage,CONTX);


    int h;
	 uint8 scc=0;
	 for(h=0;h<31;h++)
		 {
	//		vPrintf(" %d",getNextInHopSequence(&scc));
		 }


    MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

    pcComsPrintf("mac %x %x",macptr->u32H , macptr->u32L);

    pcComsPrintf("seed %d %d ",macptr->u32H ^ macptr->u32L, macptr->u32L ^ macptr->u32H);

    //use dio 16 for test sync pulse
    vAHI_DioSetDirection(0,1<<16);

    initTxMac(&txMACh,&txMACl);

    //set demands to impossible values
    int i;
    for(i=0;i<20;i++)rxDemands[i]=4096;

    initInputs();

    initOutputs();


   // initalise Analogue peripherals
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
	                 E_AHI_AP_INT_DISABLE,
	                 E_AHI_AP_SAMPLE_8,
	                 E_AHI_AP_CLOCKDIV_500KHZ,
	                 E_AHI_AP_INTREF);

    while(bAHI_APRegulatorEnabled() == 0);

    startServoPwm();

    while (1)
    {
        vProcessEventQueues();
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Simply jumps to AppColdStart
 * as, in this instance, application will never warm start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitSystem
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
     MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

  //   randomizeHopSequence(((uint32)macptr->u32H) ^ ((uint32)macptr->u32L) );

     uint32 mach=((uint32)macptr->u32H);
     uint32 macl=((uint32)macptr->u32L);

//     randomizeHopSequence(mach^macl);
//todo something odd happens with seeding the hop sequence from the mac ptr on 5148

     randomizeHopSequence(2043433);



    /* Setup interface to MAC */
   (void)u32AHI_Init();
   (void)u32AppQApiInit(NULL, NULL, NULL);

    /* Initialise end device state */
    sEndDeviceData.eState = E_STATE_IDLE;
    sEndDeviceData.u8TxPacketSeqNb = 0;
    sEndDeviceData.u8RxPacketSeqNb = 0;
    sEndDeviceData.u8ChannelSeqNo  = 0;

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID in PIB (also sets match register in hardware) */
    MAC_vPibSetPanId(s_pvMac, PAN_ID);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);


    // sometimes useful during development
    // all messages are passed up from lower levels
    // MAC_vPibSetPromiscuousMode(s_pvMac, TRUE, FALSE);


}
/****************************************************************************
 *
 * NAME: vProcessEventQueues
 *
 * DESCRIPTION:
 * Check each of the three event queues and process and items found.
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessEventQueues(void)
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
	MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    /* Check for anything on the MCPS upward queue */
    do
    {
        psMcpsInd = psAppQApiReadMcpsInd();
        if (psMcpsInd != NULL)
        {
            vProcessIncomingMcps(psMcpsInd);
            vAppQApiReturnMcpsIndBuffer(psMcpsInd);
        }
    } while (psMcpsInd != NULL);

    /* Check for anything on the MLME upward queue */
    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if (psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    } while (psMlmeInd != NULL);

    /* Check for anything on the AHI upward queue */
    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if (psAHI_Ind != NULL)
        {
            vProcessIncomingHwEvent(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    } while (psAHI_Ind != NULL);

    /* check for app software events*/
    while( processSwEventQueue()==TRUE);
}

/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 * DESCRIPTION:
 * Process any hardware events.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psAHI_Ind
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{

 //   if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TICK_TIMER)


 //   if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TIMER0)
 //   {

 //   }
}


/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 * DESCRIPTION:
 * Process any incoming managment events from the stack.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMlmeInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    /* We respond to several MLME indications and confirmations, depending
       on mode */
    switch (psMlmeInd->u8Type)
    {
    /* Deferred confirmation that the scan is complete */
    case MAC_MLME_DCFM_SCAN:

        break;

    /* Deferred confirmation that the association process is complete */
    case MAC_MLME_DCFM_ASSOCIATE:

        break;

    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 * DESCRIPTION:
 * Process incoming data events from the stack.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psMcpsInd
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    /* Only handle incoming data events one device has been started as a
       coordinator */
 //   if (sEndDeviceData.eState >= E_STATE_ASSOCIATED)
  //  {
        switch(psMcpsInd->u8Type)
        {
        case MAC_MCPS_IND_DATA:  /* Incoming data frame */
            vHandleMcpsDataInd(psMcpsInd);
            break;
        case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
            vHandleMcpsDataDcfm(psMcpsInd);
            break;
        default:
            break;
        }
  //  }
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataDcfm
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if (psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS)
    {
        /* Data frame transmission successful */
        ackLowPriorityData();
    }
    else
    {
        /* Data transmission failed after 3 retries at MAC layer. */
    }
}


/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *2000
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;

    psFrame = &psMcpsInd->uParam.sIndData.sFrame;

    MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

    if (sEndDeviceData.eState != E_STATE_ASSOCIATED)
    {
        //check for bind acceptance
       if((psFrame->au8Sdu[0]&0x80)!=0)
       {
            //routed packet
            rxHandleRoutedMessage(&psFrame->au8Sdu[1],psFrame->u8SduLength-1,CONTX);
       }
    }

    //only accept from bound tx
    if(TX_ADDRESS_MODE == 3 && (psFrame->sSrcAddr.uAddr.sExt.u32H != txMACh ||
        psFrame->sSrcAddr.uAddr.sExt.u32L != txMACl)) return;

    //first frame
	if (sEndDeviceData.eState != E_STATE_ASSOCIATED)
	{
		 sEndDeviceData.u16PanId=psFrame->sSrcAddr.u16PanId;
		 MAC_vPibSetPanId(s_pvMac, sEndDeviceData.u16PanId);

		 pcComsPrintf("tx found \r\n");

		 sEndDeviceData.u16Address = 1;
		 MAC_vPibSetShortAddr(s_pvMac, sEndDeviceData.u16Address);

		 sEndDeviceData.eState = E_STATE_ASSOCIATED;

	//     MAC_vPibSetPromiscuousMode(s_pvMac, FALSE, FALSE);

//         MAC_vPibSetPanId(s_pvMac, sEndDeviceData.u16PanId);
//         MAC_vPibSetShortAddr(s_pvMac, sEndDeviceData.u16Address);

//          vPrintf("h %d %d l %d %d \r\n",psFrame->sDstAddr.uAddr.sExt.u32H,
 //            macptr->u32H,
   //          psFrame->sDstAddr.uAddr.sExt.u32L,
	 //        macptr->u32L);



		setHopMode(hoppingContinuous);

	}

//  if (psFrame->sSrcAddr.uAddr.u16Short == COORDINATOR_ADR)
//   {
   //the same packet can be received many times if the ack was not received by the sender



   if (psFrame->au8Sdu[0] != sEndDeviceData.u8RxPacketSeqNb )
   {

	   txLinkQuality = psFrame->u8LinkQuality;


	// packet  [0]seqno [1&2]time

		int txTime=(psFrame->au8Sdu[1]+(psFrame->au8Sdu[2]<<8))*160;
		int rxTime=(txTime+2000*16)%(31*20000*16);

	//	vPrintf("- %d %d",txTime/1600, getSeqClock()/1600);


		calcSyncError(rxTime);

		static int ml=0;

		if(ml!=maxActualLatency)
			{
			pcComsPrintf("- %d",maxActualLatency);
			ml=maxActualLatency;
			}

 //  vPrintf("- %d",sc/(20000*16));


		rxdpackets++;

		sEndDeviceData.u8RxPacketSeqNb=psFrame->au8Sdu[0];
		vProcessReceivedDataPacket(&psFrame->au8Sdu[3], (psFrame->u8SduLength) -3);



//send some data back

		// should check there is time to do this and limit retries

		//if any debug stuff send it
		if(radiodebugidx!=1 && debugchannel == RADIODEBUG)
		{
		//	vTransmitDataPacket(radiodebugbuf,radiodebugidx+1,FALSE,FALSE);
			radiodebugidx=1;
		}
		else
		{
			//cycle through sensor data
			uint8 au8Packet[6];
			uint8 packetLen=4;

			au8Packet[1]=returnPacketIdx;
			uint16 val=0;

			switch (returnPacketIdx)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				{
					val = u16ReadADC(returnPacketIdx);
					break;
				}
				case 6:
				{
					val=getErrorRate()+(txLinkQuality<<8);
					if( readNmeaGps(&gpsData)==FALSE)returnPacketIdx=14;
					break;
				}
				case 7:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmeaspeed,4);
					packetLen=6;
					break;
				}
				case 8:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmeaheight,4);
					packetLen=6;
					break;
				}
				case 9:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmeatrack,4);
					packetLen=6;
					break;
				}
				case 10:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmeatime,4);
					packetLen=6;
					break;
				}
				case 11:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmealat,4);
					packetLen=6;
					break;
				}
				case 12:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmealong,4);
					packetLen=6;
					break;
				}
				case 13:
				{
					readNmeaGps(&gpsData);
					memcpy(&au8Packet[2],&gpsData.nmeasats,4);
					packetLen=6;
					break;
				}
			}

			au8Packet[0]=packetLen-1;
			if(packetLen==4)
			{
				au8Packet[2]=val&0xff;
				au8Packet[3]=val>>8;
			}
			vTransmitDataPacket(au8Packet,packetLen,FALSE);

			returnPacketIdx++;

			if(returnPacketIdx>=14)returnPacketIdx=0;
		}
  }
}
/****************************************************************************
 *
 * NAME: vProcessReceivedDataPacket
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len)
{
    //4 * 12 bit values + 12bit value and channel indicator

    if (u8Len >= 8)
    {
        //decode channel data from packet

        rxDemands[0] = pu8Data[0] + ((pu8Data[1] & 0x00f0) << 4);
        rxDemands[1] = ((pu8Data[1] & 0x000f) ) + (pu8Data[2] <<4);
        rxDemands[2] = pu8Data[3] + ((pu8Data[4] & 0x00f0) << 4);
        rxDemands[3] = ((pu8Data[4] & 0x000f) ) + (pu8Data[5] <<4);

        uint8 channel = (pu8Data[6]>>4)+4;
        uint16 value = ((pu8Data[6] & 0x000f) ) + (pu8Data[7] <<4);

        rxDemands[channel] = value;

        updateOutputs( rxDemands );
   }
    if(u8Len>8)
    {
     	handleLowPriorityData(pu8Data+8,u8Len-8);
    }
}

void frameStartEvent(void* buff)
{
    //called every 20ms
    frameCounter++;
    //binding check
    //only do automatic bind request after 10secs to incase this
    //was an unintended reboot in flight
    //only do if not already communicating with a tx
    if(frameCounter > 500 && frameCounter < 5000 && getHopMode()==hoppingRxStartup)
    {
        //todo test binding
    	uint32 channel=0;
        eAppApiPlmeGet(PHY_PIB_ATTR_CURRENT_CHANNEL, &channel);
        if(channel==11)
        {
            uint8 packet[10];
            packet[0]=0;//no route
            packet[1]=16;//bind request

            MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

            memcpy(&packet[2],&macptr->u32L,4);
            memcpy(&packet[6],&macptr->u32H,4);

            vTransmitDataPacket(packet, sizeof(packet),TRUE);
        }

    }
}

PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, bool broadcast)
{
    MAC_McpsReqRsp_s  sMcpsReqRsp;
    MAC_McpsSyncCfm_s sMcpsSyncCfm;
    uint8 *pu8Payload, i = 0;

    /* Create frame transmission request */
    sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
    sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
    /* Set handle so we can match confirmation to request */
    sMcpsReqRsp.uParam.sReqData.u8Handle = 1;
    /* Use short address for source */

    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = TX_ADDRESS_MODE;
 //   sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = sCoordinatorData.u16PanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = 0xffff;

    if(TX_ADDRESS_MODE==3)//not sure if this is needed
    {
        MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();
        sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L = macptr->u32L;
        sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H = macptr->u32H;
    }

    if(broadcast!=TRUE)
    {
      /* Use long address for destination */
        sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = RX_ADDRESS_MODE;
        sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xffff;//sCoordinatorData.u16PanId;
        sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L = txMACl;
        sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H = txMACh;


        /* Frame requires ack but not security, indirect transmit or GTS */
        sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK ;
    }
    else
    {
        sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 0;
        sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 0;
    }

    pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;

    pu8Payload[0] = sEndDeviceData.u8TxPacketSeqNb++;


    for (i = 1; i < (u8Len + 1); i++)
    {
        pu8Payload[i] = *pu8Data++;
    }

    i+=appendLowPriorityData(&pu8Payload[i],8);


    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = i;


    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
}


//route vPrinf calls over the radio
PUBLIC void rPutC(unsigned char c)
{
    //ignore overflow
    if(radiodebugidx<62)
    {
        radiodebugbuf[radiodebugidx++]=c;
    }
    else radiodebugbuf[63]='#';

}



//routed message handlers these can either relay messages along routes or handle them

void txComsSendRoutedPacket(uint8* msg, uint8 len)
{
	queueLowPriorityData(msg,len);
}
void rxSendRoutedMessage(uint8* msg,uint8 len,uint8 toCon)
{

    switch(toCon)
    {
        case CONTX:
        {
            txComsSendRoutedPacket(msg, len);
            break;
        }
        case CONPC:
        {
            //wrap routed packet with a 255 cmd to allow coexistance with existing coms
            pcComsSendPacket(msg, 0, len,255);
            break;
        }
    }
}

void rxHandleRoutedMessage(uint8* msg,uint8 len,uint8 fromCon)
{
	uint8 replyBuf[256];

	//see if packet has reached its destination
    if(rmIsMessageForMe(msg)==TRUE)
    {
     	//message is for us - unwrap the payload
    	uint8* msgBody;
    	uint8 msgLen;
    	rmGetPayload(msg,len,&msgBody,&msgLen);

    	uint8 addrLen=rmBuildReturnRoute(msg, replyBuf);
    	uint8* replyBody=replyBuf+addrLen;
        uint8 replyLen=0;

       switch( msgBody[0])
       {
            case 0: //enumerate name and all children except fromCon
            {
                uint8 i;

                *replyBody++=1;//enumerate response id
                *replyBody++=2;//string length
                *replyBody++='R';
                *replyBody++='X';
                for(i=0;i<2;i++)
                {
                    if(i!=fromCon)*replyBody++=i;
                }
                replyLen=5;
                break;
            }
            case 2: //enumerate all methods
            {
                break;
            }
            case 17: //bind response
            {
                //if in bind mode
                //set txmac
                memcpy(&txMACl,msgBody,4);
                memcpy(&txMACh,msgBody+4,4);
              break;
            }

            case 0x90: //start upload
             	replyLen=startCodeUpdate(msgBody,replyBody);
            	break;
            case 0x92: //upload chunk
            	replyLen=codeUpdateChunk(msgBody,replyBody);
				break;
            case 0x94: //commit upload
             	replyLen=commitCodeUpdate(msgBody,replyBody);
				break;

            case 0xff:  //display local text message
            	pcComsPrintf("Test0xff ");
            	break;
            default :
            	pcComsPrintf("UnsupportedCmd %d ",msgBody[0]);
            	break;
       }
       if(replyLen>0)
       {
           rxSendRoutedMessage(replyBuf,replyLen+addrLen, fromCon);
       }
    }
    else
    {
    	  //relay message
    	  uint8 toCon;
    	  //swap last 'to' to 'from' in situ
    	  toCon=rmBuildRelayRoute(msg,fromCon);
    	  //pass message on to connector defined by 'to' address
    	  rxSendRoutedMessage( msg, len, toCon);
    }
}


void loadDefaultSettings()
{

}
void loadSettings()
{

    bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

    loadDefaultSettings();

    store s;
    store section;
    int tag;
    if(getOldStore(&s)==TRUE)
    {
        while((tag=storeGetSection(&s,&section))>0)
        {
            switch(tag)
            {
               case STORERXBINDING_MACL:
                   txMACl=(uint32)readInt32(&section);
                   break;
               case STORERXBINDING_MACH:
                   txMACh=(uint32)readInt32(&section);
                   break;

            }
        }
    }
}
void storeSettings()
{
    store s;
    store old;
    store* pold;
    bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);


    if(getOldStore(&old)==TRUE)
    {
        pold=&old;
    }
    else
    {
       pold=NULL;
    }

    getNewStore(&s);

    storeInt32Section(&s,STORERXBINDING_MACL,(uint32)txMACl);
    storeInt32Section(&s,STORERXBINDING_MACH,(uint32)txMACh);

    commitStore(&s);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
