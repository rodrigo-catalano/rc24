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
//#include <Printf.h>
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
#include "commonCommands.h"
#include "exceptions.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define CONTX 0
#define CONPC 1

#define FRAME_RATE 20	// Frame rate in Hz

#define RX_LED_BIT		(1 << 17)			// DIO for LED
#define LED_FLASH_SLOW 	(FRAME_RATE * 2)	// Flash every 2 secs
#define LED_FLASH_MED  	FRAME_RATE			// Flash every second
#define LED_FLASH_FAST 	(FRAME_RATE / 2)	// Flash twice per sec


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
	E_STATE_IDLE,
	E_STATE_ACTIVE_SCANNING,
	E_STATE_ASSOCIATING,
	E_STATE_ASSOCIATED,

} teState;

typedef struct
{
	teState eState;
	uint8 u8Channel;
	uint8 u8TxPacketSeqNb;
	uint8 u8RxPacketSeqNb;
	uint16 u16Address;
	uint16 u16PanId;
	uint8 u8ChannelSeqNo;

} tsEndDeviceData;

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

void frameStartEvent(void* buff);

void rxSendRoutedMessage(uint8* msg, uint8 len, uint8 toCon);
void txComsSendRoutedPacket(uint8* msg, uint8 len);
void rxHandleRoutedMessage(uint8* msg, uint8 len, uint8 fromCon);

void loadDefaultSettings(void);
void loadSettings(void);
void storeSettings(void);

//PRIVATE void vTick_TimerISR(uint32 u32Device, uint32 u32ItemBitmap);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

// Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;

PRIVATE tsEndDeviceData sEndDeviceData;	// End device state store
PRIVATE uint32 rxdpackets = 0;	// Global packet counter
PRIVATE uint8 txLinkQuality = 0;	// Link quality from last message
PRIVATE uint32 frameCounter = 0;	// Good for almost 3 years at 50/sec
PRIVATE uint8 debugCon = CONPC;	// Where to send debug messages
PRIVATE uint16 rxDemands[20];	// Demanded positions from tx
PRIVATE uint8 rxLEDFlashCount = 0;	// Counter for LED flasher
PRIVATE uint8 rxLEDFlashLimit = LED_FLASH_MED;	// Count limit to toggle LED
PRIVATE bool rxLEDState = FALSE;	// State of LED

// TODO - make local to using function
PRIVATE nmeaGpsData gpsData;	// Data from GPS device

PRIVATE uint8 returnPacketIdx=0;

PRIVATE uint32 txMACh = 0;	// Tx MAC address high word
PRIVATE uint32 txMACl = 0;	// Tx MAC address low word

// TODO - only used in a function, move there?
pcCom pccoms;

// TODO - Unused??
uint16 thop = 0;
uint32 intstore;
uint32 frameperiods = 0;
uint32 resyncs = 0;
uint32 reportNo = 0;

uint8 testparam=27;
int testparam2=2452;
//list of parameters that can be read or set by connected devices
//either by direct access to variable or through getters and setters
//their command id is defined by position in the list
ccParameter exposedParameters[]=
{
		{"testuint8",CC_UINT8,&testparam,0},
		{"testint",CC_INT32,&testparam2,0},
		{"RX Demands",CC_UINT16_ARRAY,rxDemands,sizeof(rxDemands)/sizeof(rxDemands[0])}
};
ccParameterList parameterList=
{
		exposedParameters,
		sizeof(exposedParameters)/sizeof(exposedParameters[0])
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void dbgPrintf(const char *fmt, ...)
{
	// Send as routed message
	// TODO make route a setable parameter so connected devices
	// can ask for debug messages
	// TODO - Fix magic numbers
	char buf[196];
	buf[0] = 0;
	buf[1] = 0xff;
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vsnprintf(buf + 3, 190, fmt, ap);
	va_end(ap);
	buf[2] = ret;
	rxSendRoutedMessage((uint8*) buf, ret + 3, debugCon);
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
#ifdef JN5148
	// TODO - use watch dog and probably disable brownout reset
	vAHI_WatchdogStop();
#else
	vAppApiSetBoostMode(TRUE);
#endif

	// Initialise the hopping mode status
	// TODO - move to settings?
	setHopMode(hoppingRxStartup);


	// Initialise the system
	vInitSystem();

	if (debugCon == CONPC)
	{
		// Don't use uart pins for servo op
		initPcComs(&pccoms, CONPC, 0, rxHandleRoutedMessage);
		vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);
	}

	// set up the exception handlers
	setExceptionHandlers();



	// Initialise the clock
	// TODO - fix this
	/* the jn5148 defaults to 16MHz for the processor,
	   it can be switched to 32Mhz however the servo driving
	   code would need tweaking
	   */
	//bAHI_SetClockRate(3);

	// Send init string to PC
	dbgPrintf("rx24 2.10 ");

	resetType rt=getResetReason();
	if(rt!=NOEXCEPTION)
	{
		dbgPrintf("EXCEPTION %d \r\n",rt);
	}

	// Load the receiver settings
	loadSettings();

	// Set handler for incoming data
	setRadioDataCallback(rxHandleRoutedMessage, CONTX);

	// Retrieve the MAC address and log it to the PC
	module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();
	dbgPrintf("\r\nmac %x %x ", macptr->u32H, macptr->u32L);
	// log bound tx mac to pc
	dbgPrintf("\r\nbound tx mac %x %x ",txMACh ,txMACl );

	// Use dio 16 for test sync pulse
	vAHI_DioSetDirection(0, 1 << 16);

	// Use DIO 17 for the LED
	vAHI_DioSetDirection(0, RX_LED_BIT);

	// Set demands to impossible values
	// TODO - fix magic numbers grrr
	int i;
	for (i = 0; i < 20; i++)
		rxDemands[i] = 4096;

	// Set up digital inputs and outputs
	initInputs();
	initOutputs();

	// Initialise Analogue peripherals
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE, E_AHI_AP_INT_DISABLE,
			E_AHI_AP_SAMPLE_8, E_AHI_AP_CLOCKDIV_500KHZ, E_AHI_AP_INTREF);

	while (bAHI_APRegulatorEnabled() == 0)
		;

	// Start the servo pwm generator
	setFrameCallback(frameStartEvent);
	startServoPwm();

	// Enter the never ending main handler

	while (1)
	{
		// Process any events
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

/****************************************************************************
 *
 * NAME: frameStartEvent
 *
 * DESCRIPTION: Handler for the frame start notification
 *
 * PARAMETERS:      Name            RW  Usage
 * 					buff			?	unused??
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void frameStartEvent(void* buff)
{
	// Called every frame, currently 20ms
	frameCounter++;

	// Binding check
	// Only do automatic bind request after 10secs incase this
	// was an unintended reboot in flight.
	// Only do if not already communicating with a tx
	if ((frameCounter > 500 && frameCounter < 5000) &&
		getHopMode() == hoppingRxStartup)
	{
		// TODO - add binding button check and an option to disable auto binding
		uint32 channel = 0;
		eAppApiPlmeGet(PHY_PIB_ATTR_CURRENT_CHANNEL, &channel);
		if (channel == 11)
		{
			// TODO - Get rid of magic numbers
			uint8 packet[14];
			packet[0] = 0; //no high priority data
			packet[1] = 0; //seq no
			packet[2] = 0; //chunk no
			packet[3] = 10; //length of message
			packet[4] = 0; //no route
			packet[5] = 16; //bind request

			module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

			// TODO - can't we just memcpy the whole thing in one call?
			memcpy(&packet[6], &macptr->u32L, sizeof(macptr->u32L));
			memcpy(&packet[10], &macptr->u32H, sizeof(macptr->u32H));

			vTransmitDataPacket(packet, sizeof(packet), TRUE);
		}

		// Set the flash rate to fast to indicate binding attempts
		rxLEDFlashLimit = LED_FLASH_FAST;
	}

	// LED Flasher to indicate binding state.
	if ((getHopMode() == hoppingContinuous) && !rxLEDState)
	{
		// We have bound, set the LED on
		rxLEDState = TRUE;
		vAHI_DioSetOutput(RX_LED_BIT, 0);
	}
	else
	{
		if (frameCounter > 5000)
			// We have timed out
			rxLEDFlashLimit = LED_FLASH_SLOW;

		// If the count exceeds the current limit, toggle the LED state
		if (++rxLEDFlashCount > rxLEDFlashLimit)
		{
			// Toggle the state flag
			rxLEDState = !rxLEDState;

			// Set the output accordingly
			if (rxLEDState)
				vAHI_DioSetOutput(RX_LED_BIT, 0);
			else
				vAHI_DioSetOutput(0, RX_LED_BIT);

			// Reset the counter for the next cycle
			rxLEDFlashCount = 0;
		}
	}
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitSystem
 *
 * DESCRIPTION: Initialise the radio system
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
	// Setup interface to MAC
	(void) u32AHI_Init();
	(void) u32AppQApiInit(NULL, NULL, NULL);

	// Initialise end device state
	sEndDeviceData.eState = E_STATE_IDLE;
	sEndDeviceData.u8TxPacketSeqNb = 0;
	sEndDeviceData.u8RxPacketSeqNb = 0;
	sEndDeviceData.u8ChannelSeqNo = 0;

	// Set up the MAC handles. Must be called AFTER u32AppQApiInit()
	s_pvMac = pvAppApiGetMacHandle();
	s_psMacPib = MAC_psPibGetHandle(s_pvMac);

	// Set Pan ID in PIB (also sets match register in hardware)
	MAC_vPibSetPanId(s_pvMac, PAN_ID);

	// Enable receiver to be on when idle
	MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

	// sometimes useful during development
	// all messages are passed up from lower levels
	// MAC_vPibSetPromiscuousMode(s_pvMac, TRUE, FALSE);
	module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

	//moved to after u32AHI_Init() for jn5148
	randomizeHopSequence(((uint32) macptr->u32H) ^ ((uint32) macptr->u32L));

}

/****************************************************************************
 *
 * NAME: vProcessEventQueues
 *
 * DESCRIPTION: Check each of the three event queues and process and items found.
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
	// Check for anything on the MCPS upward queue
	MAC_McpsDcfmInd_s *psMcpsInd;
	do
	{
		psMcpsInd = psAppQApiReadMcpsInd();
		if (psMcpsInd != NULL)
		{
			vProcessIncomingMcps(psMcpsInd);
			vAppQApiReturnMcpsIndBuffer(psMcpsInd);
		}
	} while (psMcpsInd != NULL);

	// Check for anything on the MLME upward queue
	MAC_MlmeDcfmInd_s *psMlmeInd;
	do
	{
		psMlmeInd = psAppQApiReadMlmeInd();
		if (psMlmeInd != NULL)
		{
			vProcessIncomingMlme(psMlmeInd);
			vAppQApiReturnMlmeIndBuffer(psMlmeInd);
		}
	} while (psMlmeInd != NULL);

	// Check for anything on the AHI upward queue
	AppQApiHwInd_s *psAHI_Ind;
	do
	{
		psAHI_Ind = psAppQApiReadHwInd();
		if (psAHI_Ind != NULL)
		{
			vProcessIncomingHwEvent(psAHI_Ind);
			vAppQApiReturnHwIndBuffer(psAHI_Ind);
		}
	} while (psAHI_Ind != NULL);

	// Check for app software events
	while (processSwEventQueue() == TRUE)
		;
}

/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 * DESCRIPTION: Process any hardware events.
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

	// TODO - fix this
	//   if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TICK_TIMER)


	//   if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TIMER0)
	//   {

	//   }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 * DESCRIPTION: Process any incoming managment events from the stack.
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
 * NAME: vProcessIncomingMcps
 *
 * DESCRIPTION: Process incoming data events from the stack.
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
	// TODO - fix this
	//   if (sEndDeviceData.eState >= E_STATE_ASSOCIATED)
	//  {
	switch (psMcpsInd->u8Type)
	{
	case MAC_MCPS_IND_DATA: /* Incoming data frame */
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
 * DESCRIPTION: Handler for deferred transmit data confirmation
 *
 * PARAMETERS:      Name            RW  Usage
 * 					psMcpsInd		R	pointer to deferred confirmation or indication
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{

	// Data frame transmission successful
	if (psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS)
	{
		ackLowPriorityData();
	}
	else
	{
		// TODO - fix this
		// Data transmission failed after 3 retries at MAC layer.
	}
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 * DESCRIPTION:  Handler for received data
 *
 * PARAMETERS:      Name            RW  Usage
 *					psMcpsInd		R	pointer to received data struct
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	// Get the received data structure
	MAC_RxFrameData_s *psFrame = &psMcpsInd->uParam.sIndData.sFrame;

	// Get the mac address
	// TODO - pvAppApiGetMacAddrLocation not in docs
	// TODO - macptr not used as code is commented out
	module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

	// If not associated ??
	// TODO - should this be merged with similar code below [1]
	if (sEndDeviceData.eState != E_STATE_ASSOCIATED)
	{
		// Check for bind acceptance
		// if routed packet, handle it
		if(psFrame->u8SduLength>11)
		{
			handleLowPriorityData(&psFrame->au8Sdu[11], psFrame->u8SduLength - 11);
			return;
		}
	}

	// Only accept from bound tx
	// TODO - the TX_ADDRESS_MODE stops the following code from being reached


	if (TX_ADDRESS_MODE == 3 && (psFrame->sSrcAddr.uAddr.sExt.u32H != txMACh
			|| psFrame->sSrcAddr.uAddr.sExt.u32L != txMACl))
		return;

	// First frame, we have an association
	// TODO - Should this be merged with code above [1]
	if (sEndDeviceData.eState != E_STATE_ASSOCIATED)
	{
		dbgPrintf("tx found \r\n");

		// Update the association state
		sEndDeviceData.eState = E_STATE_ASSOCIATED;

		// Set the hopping mode
		setHopMode(hoppingContinuous);
	}

	//the same packet can be received many times if the ack was not received by the sender
	// If the received packet seq. no. is different from the last one we have a new packet
	if (psFrame->au8Sdu[0] != sEndDeviceData.u8RxPacketSeqNb)
	{
		// Record the link quality
		txLinkQuality = psFrame->u8LinkQuality;

		/*
		 * TODO - define a structure for this
		 * packet layout:
		 * [0] - 8 bit seqno
		 * [1] - 16 bit tx time
		 */

		// retrieve the tx time, calculate the rx time ??
		// TODO - explain rx time calc
		int txTime = (psFrame->au8Sdu[1] + (psFrame->au8Sdu[2] << 8)) * 160;
		int rxTime = (txTime + 2000* 16 ) % (31* 20000* 16 ) ;

		// TODO - fix commented out code
		//vPrintf("- %d %d",txTime/1600, getSeqClock()/1600);

		// TODO - Explain this
		calcSyncError(rxTime);

		// TODO - Explain this, seems superfluous as 'ml' isn't used
		static int ml = 0;
		if (ml != maxActualLatency)
		{
			dbgPrintf("- %d", maxActualLatency);
			ml = maxActualLatency;
		}

		// TODO - fix commented out code
		//vPrintf("- %d",sc/(20000*16));

		// Update global rx data packet counter
		rxdpackets++;

		// Update latest received se. no.
		sEndDeviceData.u8RxPacketSeqNb = psFrame->au8Sdu[0];

		// Process the data packet
		vProcessReceivedDataPacket(&psFrame->au8Sdu[3], (psFrame->u8SduLength)
				- 3);

		// Send some data back
		// should check there is time to do this and limit retries

		// Declare the return packet data and default size
		uint8 au8Packet[6];
		uint8 packetLen = 4;

		// Set the return packet data indicator
		au8Packet[1] = returnPacketIdx;

		uint16 val = 0;
		switch (returnPacketIdx)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		{
			// 0 - 5 ADC channels
			val = u16ReadADC(returnPacketIdx);
			au8Packet[2] = val & 0xff;
			au8Packet[3] = val >> 8;
			break;
		}
		case 6:
		{
			// 6 - TX Quality
			au8Packet[2] = getErrorRate();
			au8Packet[3] = txLinkQuality;

			// If unable to read GPS data skip the GPS data items
			if (readNmeaGps(&gpsData) == FALSE)
				returnPacketIdx = 14;
			break;
		}
		case 7:
		{
			// 7 - GPS speed
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmeaspeed, sizeof(gpsData.nmeaspeed));
			packetLen = 6;
			break;
		}
		case 8:
		{
			// 8 - GPS height
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmeaheight,
					sizeof(gpsData.nmeaheight));
			packetLen = 6;
			break;
		}
		case 9:
		{
			// 9 - GPS track
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmeatrack, sizeof(gpsData.nmeatrack));
			packetLen = 6;
			break;
		}
		case 10:
		{
			// 10 - GPS time
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmeatime, sizeof(gpsData.nmeatime));
			packetLen = 6;
			break;
		}
		case 11:
		{
			// 11 - GPS latitude
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmealat, sizeof(gpsData.nmealat));
			packetLen = 6;
			break;
		}
		case 12:
		{
			// 12 - GPS longitude
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmealong, sizeof(gpsData.nmealong));
			packetLen = 6;
			break;
		}
		case 13:
		{
			// 13 - satellites
			readNmeaGps(&gpsData);
			memcpy(&au8Packet[2], &gpsData.nmeasats, sizeof(gpsData.nmeasats));
			packetLen = 6;
			break;
		}
		}

		// Set the data length
		au8Packet[0] = packetLen - 1;

		// Send the packet
		vTransmitDataPacket(au8Packet, packetLen, FALSE);

		// Update the data type for the next packet
		returnPacketIdx++;

		// If the last data type has been sent, reset to 0
		if (returnPacketIdx >= 14)
			returnPacketIdx = 0;
	}
}

/****************************************************************************
 *
 * NAME: vProcessReceivedDataPacket
 *
 * DESCRIPTION: Handler for the received data
 *
 * PARAMETERS:      Name            RW  Usage
 * 					pu8Data			R	pointer to data buffer
 * 					u8Len			R	length of data in buffer
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len)
{
	// 4 * 12 bit values + 12bit value and channel indicator

	// If there is sufficient data, process it
	if (u8Len >= 8)
	{
		// Decode servo data from packet
		// TODO - define a structure for this using bitfields
		rxDemands[0] = pu8Data[0] + ((pu8Data[1] & 0x00f0) << 4);
		rxDemands[1] = ((pu8Data[1] & 0x000f)) + (pu8Data[2] << 4);
		rxDemands[2] = pu8Data[3] + ((pu8Data[4] & 0x00f0) << 4);
		rxDemands[3] = ((pu8Data[4] & 0x000f)) + (pu8Data[5] << 4);

		// Decode channel and value from packet
		uint8 channel = (pu8Data[6] >> 4) + 4;
		uint16 value = ((pu8Data[6] & 0x000f)) + (pu8Data[7] << 4);

		// Set the extra channel
		rxDemands[channel] = value;

		// Set the output values
		updateOutputs(rxDemands);

		// If there is more data, process it
		if (u8Len > 8)
		{
			handleLowPriorityData(pu8Data + 8, u8Len - 8);
		}
	}
}

/****************************************************************************
 *
 * NAME: vTransmitDataPacket
 *
 * DESCRIPTION: Transmits a data packet
 *
 * PARAMETERS:      Name            RW  Usage
 * 					pu8Data			R	pointer to data to be transmitted
 * 					u8Len			R	length of data block
 * 					broadcast		R	flag, TRUE if broadcast to be used
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, bool broadcast)
{
	MAC_McpsReqRsp_s sMcpsReqRsp;

	// Create frame transmission request
	sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
	sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);

	// Set handle so we can match confirmation to request
	sMcpsReqRsp.uParam.sReqData.u8Handle = 1;

	module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

	if (broadcast != TRUE)
	{
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = TX_ADDRESS_MODE;
#if(TX_ADDRESS_MODE == 3)
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L= macptr->u32L;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H= macptr->u32H;
#endif

		// Use broadcast pan id
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = 0xffff;

		// Use long address (mac address) for destination
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode
				= RX_ADDRESS_MODE;
		// Use broadcast pan id for destination
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xffff;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L = txMACl;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H = txMACh;

		// Frame requires ack but not security, indirect transmit or GTS
		sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK;
	}
	else
	{
		// Use long source address
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 3;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L= macptr->u32L;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H= macptr->u32H;

		// Use broadcast short address for destination
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = 0xffff;
		sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 0;

		// Use a fixed pan id known to all rc24 systems for binding
		// Could use the broadcast panid but this reduces the odds on getting
		// responses from non rc24 systems
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId=PAN_ID;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId=PAN_ID;
	}

	// Get a pointer to the output data buffer
	uint8 *pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;

	// Index into output data buffer
	uint8 index = 0;

	// Set the sequence number
	pu8Payload[0] = sEndDeviceData.u8TxPacketSeqNb++;
	index += 1;

	// Copy the data packet to the data buffer
	memcpy(&(pu8Payload[index]), pu8Data, u8Len);
	index += u8Len;

	// Tack on the low priority data
	// TODO - fix the magic number for the maxlen arg
	index += appendLowPriorityData(&pu8Payload[index], 32);

	// Set frame length
	sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = index;

	// Request transmit
	// TODO - sMcpsSyncCfm isn't used
	MAC_McpsSyncCfm_s sMcpsSyncCfm;
	vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
}

// Routed message handlers these can either relay messages along routes or handle them

/****************************************************************************
 *
 * NAME: txComsSendRoutedPacket
 *
 * DESCRIPTION: Send a routed packet to the tx
 *
 * PARAMETERS:      Name            RW  Usage
 * 					msg				R	pointer to data to be transmitted
 * 					len				R	length of data block
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void txComsSendRoutedPacket(uint8* msg, uint8 len)
{
	// Add the packet to the low priority queue
	queueLowPriorityData(msg, len);
}

/****************************************************************************
 *
 * NAME: rxSendRoutedMessage
 *
 * DESCRIPTION: Send a routed packet
 *
 * PARAMETERS:      Name            RW  Usage
 * 					msg				R	pointer to data to be transmitted
 * 					len				R	length of data block
 * 					toCon			R	routing indicator
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
// TODO - make toCon an enum
void rxSendRoutedMessage(uint8* msg, uint8 len, uint8 toCon)
{
	// Send via appropriate channel
	switch (toCon)
	{
	case CONTX:
	{
		// Send to the tx
		txComsSendRoutedPacket(msg, len);
		break;
	}
	case CONPC:
	{
		// Wrap routed packet with a 255 cmd to allow coexistance with existing comms
		// TODO - Fix the command magic number
		pcComsSendPacket(msg, 0, len, 255);
		break;
	}
	}
}

/****************************************************************************
 *
 * NAME: rxHandleRoutedMessage
 *
 * DESCRIPTION: Handle a routed packet
 *
 * PARAMETERS:      Name            RW  Usage
 * 					msg				R	pointer to data to be transmitted
 * 					len				R	length of data block
 * 					fromCon			R	routing indicator
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void rxHandleRoutedMessage(uint8* msg, uint8 len, uint8 fromCon)
{
	// TODO - make a definition for the buffer length
	uint8 replyBuf[256];

	// See if packet has reached its destination
	if (rmIsMessageForMe(msg) == TRUE)
	{

		// Message is for us - unwrap the payload
		uint8* msgBody;
		uint8 msgLen;
		rmGetPayload(msg, len, &msgBody, &msgLen);

		// Start building the reply, starting with the address
		uint8 addrLen = rmBuildReturnRoute(msg, replyBuf);
		uint8* replyBody = replyBuf + addrLen;
		uint8 replyLen = 0;

		// TODO - make the function code an enum
		switch (msgBody[0])
		{
		case 0: // enumerate name and all children except fromCon
		{
			// TODO - replace magic number
			*replyBody++ = 1;//enumerate response id
			*replyBody++ = 2;//string length
			*replyBody++ = 'R';
			*replyBody++ = 'X';
			// TODO - ???
			uint8 i;
			for (i = 0; i < 2; i++)
			{
				if (i != fromCon)
					*replyBody++ = i;
			}
			replyLen = 5;
			break;
		}
		case CMD_ENUM_GROUP :
			replyLen=ccEnumGroupCommand(&parameterList, msgBody, replyBody);
			break;
		case CMD_SET_PARAM:
			replyLen=ccSetParameter(&parameterList, msgBody, replyBody);
			break;
		case CMD_GET_PARAM:
			replyLen=ccGetParameter(&parameterList, msgBody, replyBody);
			break;
		case 17: // Bind response
		{
			// If in bind mode set tx mac

			if(sEndDeviceData.eState != E_STATE_ASSOCIATED)
			{
				memcpy(&txMACl, msgBody+1, sizeof(txMACl));
				memcpy(&txMACh, msgBody + 5, sizeof(txMACh));
				//store binding
				storeSettings();
				dbgPrintf("Rx Bound");
			}
			break;
		}

		case 0x90: // Start upload
		{
			replyLen = startCodeUpdate(msgBody, replyBody);
			break;
		}

		case 0x92: // Upload chunk
		{
			replyLen = codeUpdateChunk(msgBody, replyBody);
			break;
		}

		case 0x94: // Commit upload
		{
			replyLen = commitCodeUpdate(msgBody, replyBody);
			break;
		}

		case 0x96: // Reset
		{
			vAHI_SwReset();
			break;
		}

		case 0xff: // Display local text message
		{
			// TODO - fix commented out code
			//	dbgPrintf("Test0xff ");
			break;
		}

		default:
		{
			dbgPrintf("UnsupportedCmd %d ", msgBody[0]);
			break;
		}
		}

		// If there is a response, send it out
		if (replyLen > 0)
		{
			rxSendRoutedMessage(replyBuf, replyLen + addrLen, fromCon);
		}
	}
	else
	{
		// Not for us, relay message
		// Swap last 'to' to 'from' in situ
		uint8 toCon = rmBuildRelayRoute(msg, fromCon);
		// Pass message on to connector defined by 'to' address
		rxSendRoutedMessage(msg, len, toCon);
	}
}

/****************************************************************************
 *
 * NAME: loadDefaultSettings
 *
 * DESCRIPTION: Load the default settings for the Rx
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void loadDefaultSettings()
{
	// TODO - Add some code
}

/****************************************************************************
 *
 * NAME: loadSettings
 *
 * DESCRIPTION: Load the current settings for the Rx
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void loadSettings()
{
	// Select the flash type
	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	// Retrieve the default settings
	loadDefaultSettings();

	// TODO - Explain this
	store s;
	store section;
	int tag;
	if (getOldStore(&s) == TRUE)
	{
		dbgPrintf("store found");
		while ((tag = storeGetSection(&s, &section)) > 0)
		{
			dbgPrintf(" t%d",tag);
			switch (tag)
			{
			case STORERXBINDING_MACL:
				txMACl = (uint32) readInt32(&section);
				break;
			case STORERXBINDING_MACH:
				txMACh = (uint32) readInt32(&section);
				break;

			}
		}
	}
}

/****************************************************************************
 *
 * NAME: storeSettings
 *
 * DESCRIPTION: Store the current settings for the Rx
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void storeSettings()
{
	store s;
	store old;

	// TODO - what is this used for?
	store* pold;

	// Select the flash chip type
	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	if (getOldStore(&old) == TRUE)
	{
		pold = &old;
	}
	else
	{
		pold = NULL;
	}

	// TODO - Explain
	getNewStore(&s);

	storeInt32Section(&s, STORERXBINDING_MACL, (uint32) txMACl);
	storeInt32Section(&s, STORERXBINDING_MACH, (uint32) txMACh);

	commitStore(&s);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
