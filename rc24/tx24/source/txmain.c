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
#include <stdlib.h>
#include <math.h>

#include "txmain.h"
#include "config.h"
#include "ps2.h"
#include "hopping.h"
#include "store.h"
#include "model.h"
#include "mymodels.h"
#include "lcdEADog.h"
#include "display.h"
#include "wii.h"
#include "ppm.h"
#include "routedmessage.h"
#include "pcComs.h"
#include "swEventQueue.h"
#include "tsc2003.h"

#include "smbus.h"

#include "codeupdate.h"
#include "radiocoms.h"
#include "hwutils.h"

#include "gui_walnut.h"
#include "commonCommands.h"

#include "exceptions.h"

//#include "scriptIL.h"

/* Data type for storing data related to all end devices that have associated */
typedef struct
{
	uint16 u16ShortAdr;
	uint32 u32ExtAdrL;
	uint32 u32ExtAdrH;
} tsEndDeviceData;

typedef struct
{
	/* Data related to associated end devices */
	uint16 u16NbrEndDevices;
	tsEndDeviceData sEndDeviceData;

	teState eState;

	uint8 u8Channel;
	uint8 u8TxPacketSeqNb;
	uint8 u8RxPacketSeqNb;
	uint16 u16PanId;
	uint8 u8ChannelSeqNo;

} tsCoordinatorData;

#define ALARM_PIN E_AHI_DIO13_INT

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vProcessEventQueues(void);

//radio events
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, uint16 u16DestAdr);

PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len);
PRIVATE uint16 u16ReadADCAverage(uint8 channel);
PRIVATE void vCreateAndSendFrame(void);

PRIVATE void GetNextChannel(void);
PRIVATE void HoldOffAndSend(void);

//routed coms functions
void rxComsSendRoutedPacket(uint8* msg, int offset, uint8 len);

void updateDisplay(void);
void updatePcDisplay(void);

void checkBattery(void);

void storeSettings(void);
void loadSettings(void);
void loadRadioSettings(store* s);
void saveRadioSettings(store* s);
void loadGeneralSettings(store* s);
void saveGeneralSettings(store* s);

void loadDefaultSettings(void);

void sleep(void);

void enableModelComs(void);
void txSendRoutedMessage(uint8* msg, uint8 len, uint8 toCon);
void loadModel(int idx);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE tsCoordinatorData sCoordinatorData;

//monitoring stuff
PRIVATE uint32 lostPackets;
PRIVATE uint32 sentPackets;
PRIVATE int ackedPackets = 0;
PRIVATE uint32 retryPackets = 0;
PRIVATE uint32 errorRate;
PRIVATE uint16 PacketDone = 1;
int rxLinkQuality = 0;
int txLinkQuality = 0;
int channelAcks[16];
int channelAckCounter[16];
int channelRetryCounter[16];

int totalLostFrames = 0;
int lowestRxFrameRate = 100;

int totalRxFrames = 0;
int flightTimer = 0;

uint8 MaxRetries = 5;
uint8 RetryNo = 0;

bool useHighPowerModule = FALSE;

ps2ControllerOp ps2;
WiiController wii;
tsc2003 touchScreen;

// Times in micro seconds
uint32 hopPeriod = 20000;
uint32 framePeriod = 20000;
uint8 subFrameIdx = 0;

int txDemands[20];
int txInputs[20];
int currentAuxChannel = 4;

int activeAuxChannel = 4;


// only keep one model in ram at a time to avoid artificial limits on number
// of models
modelEx liveModel;
int16 liveModelIdx = 0;
int16 numModels = 0;

int rxData[256];

//initial position
int initialHeight = -9999;
double initialLat = -9999;
double initialLong = -9999;
double longitudeScale = 0.0;
int range = 0;

int joystickOffset[4];
int joystickGain[4];

#define PS2INPUT 0
#define ADINPUT 1
#define NUNCHUCKINPUT 2
#define PPMINPUT 3
#define SERIALINPUT 4
#define RAWINPUT 5

char* defaultInputEnumValues[] =
{ "PS2", "Custom", "WII Nunchuck", "PPM", "Serial", "Raw" };

uint8 inputMethod = ADINPUT;

volatile uint32 sendTime;
uint32 radioRange;
volatile uint32 lowestRange;
volatile uint32 totalTime;
volatile uint32 nRangeMeasurements;

uint8 backlight = 255;
uint16 batDac = 1023;

int backlightTimer = 50* 60* 1 ;

int txbat = 0;
int rxbat = 0;
int txretries = 0;
int rxpackets = 0;
int txacks = 0;

int tsDebounceLast = 0;
int tsDebounceCount = 0;
int tsPressedTimer = 0;
int tsClickX;
int tsClickY;

pcCom pccoms;

#ifdef JN5148
#define TICK_CLOCK_MHZ 16
#define PERIF_CLOCK_MHZ 16
#else
#define PERIF_CLOCK_MHZ 16
#define TICK_CLOCK_MHZ 16
#endif

uint32 dbgmsgtimestart;
uint32 dbgmsgtimeend;

//list of parameters that can be read or set by connected devices
//either by direct access to variable or through getters and setters
//their command id is defined by position in the list
ccParameter exposedParameters[] =
{
	{ "High Power Module", CC_BOOL, &useHighPowerModule, 0 ,CC_NO_GETTER,CC_NO_SETTER},
	{ "Default Input", CC_ENUMERATION, &inputMethod, 2 ,CC_NO_GETTER,CC_NO_SETTER},
	{ "Input Enum", CC_ENUMERATION_VALUES, defaultInputEnumValues,
		sizeof(defaultInputEnumValues) / sizeof(defaultInputEnumValues[0]) ,CC_NO_GETTER,CC_NO_SETTER},
	{ "TX Inputs", CC_INT32_ARRAY, txInputs, sizeof(txInputs) / sizeof(txInputs[0]) ,CC_NO_GETTER,CC_NO_SETTER},
	{ "TX Demands", CC_INT32_ARRAY, txDemands, sizeof(txDemands) / sizeof(txDemands[0]) ,CC_NO_GETTER,CC_NO_SETTER},
	{ "Save Settings",CC_VOID_FUNCTION,CC_NO_VAR_ACCESS,0,CC_NO_GETTER,storeSettings }
};

ccParameterList parameterList =
{ exposedParameters, sizeof(exposedParameters) / sizeof(exposedParameters[0]) };


/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
	lostPackets = 0;
	sentPackets = 0;

	//get config from flash
	//    loadSettings();
#ifdef JN5148
	vAHI_WatchdogStop();

#else
	vAppApiSetBoostMode(TRUE);
#endif

	//   setHopMode(hoppingContinuous);


	//   vInitPrintf((void*)vPutC);

	//    vInitPrintf((void*)vFifoPutC);


	vInitSystem();

	setExceptionHandlers();

	setRadioDataCallback(txHandleRoutedMessage, CONRX);

	vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);


	pcComsPrintf("tx24 2.11 \r\n");

	//check for reset caused by exception and report
	resetType rt=getResetReason();
	if(rt!=NOEXCEPTION)
	{
		pcComsPrintf("TX EXCEPTION %d \r\n",rt);
	}


	pcComsPrintf("lcd init done \r\n");

	initGUI();
	refreshGUI();

	// setup for selected input device
	switch (inputMethod)
	{
	case PS2INPUT:
	if (initPS2Controller(&ps2) == TRUE)
	{
		// select model by holding down fire buttons at startup
		pcComsPrintf("ps2 input\r\n");

		readPS2Controller(&ps2);

		if (ps2.LFire1)
			loadModel(1);
		if (ps2.LFire2)
			loadModel(2);
		if (ps2.RFire1)
			loadModel(3);
		if (ps2.RFire2)
			loadModel(4);

		//wait for button release
		while (ps2.LFire1 || ps2.LFire2 || ps2.RFire1 || ps2.RFire2)
		{
			readPS2Controller(&ps2);
		}
	}
		break;

	case NUNCHUCKINPUT:
		if (initWiiController(&wii) == TRUE)
		{
			readWiiController(&wii);
			pcComsPrintf("nunchuck input\r\n");
		}
		break;
	case ADINPUT:
		pcComsPrintf("AD input\r\n");
		break;
	case PPMINPUT:
		initPpmInput(E_AHI_TIMER_0);
		pcComsPrintf("ppm input\r\n");
		break;
	case SERIALINPUT:
		pcComsPrintf("serial input\r\n");
		break;
	case RAWINPUT:
		// Do nothing
		pcComsPrintf("raw input\r\n");
		break;
	}

	if (initTsc2003(&touchScreen, 0x48, 723, 196) == TRUE)
	{
		pcComsPrintf("tsc %d %d %d \r\n", touchScreen.x, touchScreen.y,
				touchScreen.pressure);
	}
	else
	{
		pcComsPrintf("No Touchscreen %d \r\n", touchScreen.x);
	}

	//use mac address of rx to seed random hopping sequence
	randomizeHopSequence(liveModel.rxMACh ^ liveModel.rxMACl);
	setHopMode(hoppingContinuous);

	pcComsPrintf("Model %s\r\n", liveModel.name);
	pcComsPrintf("trim %d\r\n", liveModel.trim[1]);

	refreshGUI();

	//use dio 16 for test sync pulse
	vAHI_DioSetDirection(0, E_AHI_DIO16_INT);



	//turn backlight on
	vAHI_DioSetPullup(0, E_AHI_DIO17_INT);
	vAHI_DioSetDirection(0, E_AHI_DIO17_INT);
	vAHI_DioSetOutput(E_AHI_DIO17_INT, 0);

	//turn agnd on

	vAHI_DioSetPullup(0, E_AHI_DIO18_INT);
	vAHI_DioSetDirection(0, E_AHI_DIO18_INT);
	vAHI_DioSetOutput(E_AHI_DIO18_INT, 0);

	// initalise Analog peripherals
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE, E_AHI_AP_INT_DISABLE,
			E_AHI_AP_SAMPLE_2, E_AHI_AP_CLOCKDIV_500KHZ, E_AHI_AP_INTREF);

	while (bAHI_APRegulatorEnabled() == 0)
		;

	//setup comp1 as slow adc
	vAHI_ComparatorEnable(E_AHI_AP_COMPARATOR_1, E_AHI_COMP_HYSTERESIS_0MV,
			E_AHI_COMP_SEL_DAC);

	vAHI_DacEnable(E_AHI_AP_DAC_1, E_AHI_AP_INPUT_RANGE_2,
			E_AHI_DAC_RETAIN_ENABLE, batDac);

	joystickOffset[0] = -116;
	joystickGain[0] = -1766;
	joystickOffset[1] = -173;
	joystickGain[1] = -1802;
	joystickOffset[2] = -116;
	joystickGain[2] = 1766;
	joystickOffset[3] = -173;
	joystickGain[3] = 1802;

	//setup tick timer to fire every hop period

	vAHI_TickTimerWrite(0);
	vAHI_TickTimerInterval(TICK_CLOCK_MHZ * hopPeriod);
	vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_RESTART);
	vAHI_TickTimerIntEnable(TRUE);

	/*
	 modelEx tmod;

	 setupDefaultModel(&tmod);
	 int i;
	 for(i=0;i<20;i++)txInputs[i]=0;

	 uint32 s=u32AHI_TickTimerRead();

	 doMixingEx(txInputs ,txDemands, &tmod);

	 uint32 se=u32AHI_TickTimerRead();

	 pcComsPrintf("mix %d \r\n",se-s);

	 for(i=0;i<20;i++)pcComsPrintf(" %d ",txDemands[i]);
	 */

	//setup 1us timer for retries

	vAHI_TimerEnable(E_AHI_TIMER_1, 4, FALSE, TRUE, FALSE);
	vAHI_TimerClockSelect(E_AHI_TIMER_1, FALSE, FALSE);
	vAHI_TimerDIOControl(E_AHI_TIMER_1, FALSE);

	//setup alarm pin
	vAHI_DioSetPullup(0, ALARM_PIN);
	vAHI_DioSetDirection(0, ALARM_PIN );
	vAHI_DioSetOutput(0,ALARM_PIN );



	pcComsPrintf("init done \r\n");
	sCoordinatorData.eState = E_STATE_COORDINATOR_STARTED;

	uint8 msg[2];
	msg[0] = 0;
	msg[1] = 0;//0xff;

	//   txSendRoutedMessage(msg,2, CONRX);




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
 * Entry point for application from boot loader.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
	// Check for wake up actions - do as fast as possible to save power
	touchScreen.addr = 0x48;

	if (pressedTsc2003(&touchScreen))
	{
		//simplest way to ensure everything is in a valid state
		vAHI_SwReset();
		//   AppColdStart();
	}
	else
	{
		//go back to sleep again;
		sleepTsc2003(&touchScreen);

		vAHI_SiConfigure(FALSE, FALSE, 16);

		vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
		vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_0, 32* 1000* 5 ) ;//5 second wakeup
		vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);
	}
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

	/* Setup interface to MAC */
	(void) u32AHI_Init();

	(void) u32AppQApiInit(NULL, NULL, NULL);

	initPcComs(&pccoms, CONPC, 0, txHandleRoutedMessage);

	loadSettings();

	pcComsPrintf("settings loaded \r\n");

	//move to after u32AHI_Init() to work on jn5148
	if (useHighPowerModule == TRUE)
	{
		//max power for europe including antenna gain is 10dBm
		//??? boost is +2.5 ant is 2 and power set to 4 = 8.5 ????
		vAHI_HighPowerModuleEnable(TRUE, TRUE);
		bAHI_PhyRadioSetPower(2);
	}

	/* Initialise coordinator state */
	sCoordinatorData.eState = E_STATE_IDLE;
	sCoordinatorData.u8TxPacketSeqNb = 0;
	sCoordinatorData.u8RxPacketSeqNb = 0;
	sCoordinatorData.u16NbrEndDevices = 0;

	sCoordinatorData.u8ChannelSeqNo = 0;

	/* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
	s_pvMac = pvAppApiGetMacHandle();
	s_psMacPib = MAC_psPibGetHandle(s_pvMac);

	/* Set Pan ID and short address in PIB (also sets match registers in hardware) */

	sCoordinatorData.u16PanId = PAN_ID;

	MAC_vPibSetPanId(s_pvMac, sCoordinatorData.u16PanId);
	MAC_vPibSetShortAddr(s_pvMac, COORDINATOR_ADR);

	/* Enable receiver to be on when idle */
	MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

	/* Allow nodes to associate */
	s_psMacPib->bAssociationPermit = 1;

	// disable normal hold off and retry stuff
	// do it in app code so that we can include send time in packet

#ifdef JN5148
	//thanks to Jennic support for this
	s_psMacPib->u8MaxFrameRetries = 0;

#else
	//*(volatile uint8 *)0x04000de8 = (1);
	//Set number of retries
	MAC_MlmeReqRsp_s sMlmeReqRsp;
	MAC_MlmeSyncCfm_s sMlmeSyncCfm;

	sMlmeReqRsp.u8Type = MAC_MLME_REQ_SET;
	sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqSet_s);
	sMlmeReqRsp.uParam.sReqSet.u8PibAttribute = MAC_PIB_ATTR_MAX_FRAME_RETRIES;
	sMlmeReqRsp.uParam.sReqSet.uPibAttributeValue.u8MaxFrameRetries = 0;
	vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);

#endif

	MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 0);
	MAC_vPibSetMinBe(s_pvMac, 0);

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
	AppQApiHwInd_s *psAHI_Ind;

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
	while (processSwEventQueue() == TRUE)
		;
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
	switch (psMlmeInd->u8Type)
	{
	case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
		if (sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
		{
			//       vHandleNodeAssociation(psMlmeInd);
		}
		break;

	case MAC_MLME_DCFM_SCAN: /* Incoming scan results */
		if (psMlmeInd->uParam.sDcfmScan.u8ScanType
				== MAC_MLME_SCAN_TYPE_ENERGY_DETECT)
		{
			if (sCoordinatorData.eState == E_STATE_ENERGY_SCANNING)
			{
				/* Process energy scan results and start device as coordinator */
				//        vHandleEnergyScanResponse(psMlmeInd);
			}
		}
		if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ACTIVE)
		{
			if (sCoordinatorData.eState == E_STATE_ACTIVE_SCANNING)
			{
				//         vHandleActiveScanResponse(psMlmeInd);
			}
		}
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
	if (sCoordinatorData.eState >= E_STATE_COORDINATOR_STARTED)
	{
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
	}
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
	//   vAHI_DioSetOutput(0,1<<16);


	//ignore any responses left from previous frame
	if (psMcpsInd->uParam.sDcfmData.u8Handle
			== sCoordinatorData.u8TxPacketSeqNb)
	{
		dbgmsgtimeend = u32AHI_TickTimerRead() - dbgmsgtimestart;

		if (psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS)
		{
			/* Data frame transmission successful */

			//try and time round trip time
			//      uint roundtrip=u32AHI_TickTimerRead()-sendTime;
			//      if(roundtrip<lowestRange)lowestRange=roundtrip;


			PacketDone = 1;
			ackedPackets++;
			channelAckCounter[sCoordinatorData.u8Channel - 11]--;

			ackLowPriorityData();
			//vPrintf("%d ",RetryNo);
		}
		else
		{

			/* Data transmission failed */
			//delay random backoff period and retry
			if (++RetryNo < MaxRetries)
			{
				HoldOffAndSend();
			}
			else
			{
				lostPackets++;
			}
		}
		if (PacketDone == 1)
		{
		}
	}

}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	MAC_RxFrameData_s *psFrame;

	psFrame = &psMcpsInd->uParam.sIndData.sFrame;

	/* Check application layer sequence number of frame and reject if it is
	 the same as the last frame, i.e. same frame has been received more
	 than once. */

	if (psFrame->au8Sdu[0] != sCoordinatorData.u8RxPacketSeqNb)
	{
		sCoordinatorData.u8RxPacketSeqNb = psFrame->au8Sdu[0];
		vProcessReceivedDataPacket(&psFrame->au8Sdu[1], psFrame->u8SduLength
				- 1);

		rxLinkQuality = (int) psFrame->u8LinkQuality;

		//todo prevent flight timer being reset on catch or close flyby
		//todo maybe use JN5148 range feature for this
		if (rxLinkQuality > 180)
			flightTimer = 0;
	}
}

void txSendRoutedMessage(uint8* msg, uint8 len, uint8 toCon)
{
	switch (toCon)
	{
	case CONRX:
	{
		rxComsSendRoutedPacket(msg, 0, len);
		break;
	}
	case CONPC:
	{
		//wrap routed packet with a 255 cmd to allow coexistance with existing coms
		pcComsSendPacket(msg, 0, len, 255);
		break;
	}
	}
}

void txHandleRoutedMessage(uint8* msg, uint8 len, uint8 fromCon)
{
	uint8 replyBuf[256];

	//see if packet has reached its destination
	if (rmIsMessageForMe(msg) == TRUE)
	{

		//message is for us - unwrap the payload
		uint8* msgBody;
		uint8 msgLen;
		rmGetPayload(msg, len, &msgBody, &msgLen);

		uint8 addrLen = rmBuildReturnRoute(msg, replyBuf);
		uint8* replyBody = replyBuf + addrLen;
		uint8 replyLen = 0;

		switch (msgBody[0])
		{
		case 0: //enumerate name and all children except fromCon
		{
			uint8 i;

			*replyBody++ = 1;//enumerate response id
			*replyBody++ = 2;//string length
			*replyBody++ = 'T';
			*replyBody++ = 'X';
			for (i = 0; i < 2; i++)
			{
				if (i != fromCon)
					*replyBody++ = i;
			}
			replyLen = 5;
			break;
		}

		case CMD_ENUM_GROUP:
			replyLen = ccEnumGroupCommand(&parameterList, msgBody, replyBody);
			break;
		case CMD_SET_PARAM:
			replyLen = ccSetParameter(&parameterList, msgBody, replyBody);
			break;
		case CMD_GET_PARAM:
			replyLen = ccGetParameter(&parameterList, msgBody, replyBody);
			break;
		case 16: //bind request -
		{
			//if in bind mode
			if (getHopMode() == hoppingFixed)
			{
				//set liveModel mac

				*replyBody++ = 17;//bind response id
				//send tx mac
				module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

				memcpy(replyBody, &macptr->u32L, 4);
				memcpy(replyBody + 4, &macptr->u32H, 4);

				replyLen += 9;

				memcpy(&liveModel.rxMACl, msgBody + 1, 4);
				memcpy(&liveModel.rxMACh, msgBody + 5, 4);

				pcComsPrintf("Bound %x %x \r\n", liveModel.rxMACh,
						liveModel.rxMACl);

				enableModelComs();
			}

			break;
		}
		case 0x90: //start upload
			replyLen = startCodeUpdate(msgBody, replyBody);
			break;
		case 0x92: //upload chunk
			replyLen = codeUpdateChunk(msgBody, replyBody);
			break;
		case 0x94: //commit upload
			replyLen = commitCodeUpdate(msgBody, replyBody);
			break;
		case 0x96: // Reset
			vAHI_SwReset();
			break;
		case 0xa0: //virtual click on lcd display
			displayClick(msgBody[1], msgBody[2]);
			break;
		case 0xa2: //virtual drag on lcd display
			displayDrag(((int8)msgBody[1]), ((int8)msgBody[2]));
			break;
		case 0xff: //display local text message
			pcComsPrintf("Test0xff ");
			break;
		default:
			pcComsPrintf("UnsupportedCmd %d ", msgBody[0]);
			break;

		}
		if (replyLen > 0)
		{
			txSendRoutedMessage(replyBuf, replyLen + addrLen, fromCon);
		}
	}
	else
	{

		//relay message
		uint8 toCon;
		//swap last 'to' to 'from' in situ
		toCon = rmBuildRelayRoute(msg, fromCon);
		//pass message on to connector defined by 'to' address
		txSendRoutedMessage(msg, len, toCon);
	}
}
void rxComsSendRoutedPacket(uint8* msg, int offset, uint8 len)
{
	queueLowPriorityData(msg + offset, len);
}

/****************************************************************************
 *
 * NAME: vProcessReceivedDataPacket
 *
 ****************************************************************************/
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len)
{
	// Back channel from rx - getting very messy!

	uint8 PriorityDataLen = pu8Data[0];
	totalRxFrames++;

	if (pu8Data[1] == 255)
	{
		//rx debug message
		pu8Data[u8Len - 1] = '\0';
		pcComsPrintf("db-%s\r\n", &pu8Data[2]);
	}
	else
	{
		if (PriorityDataLen == 3) // 16 bit data
		{
			if (pu8Data[1] == 6)
			{
				txLinkQuality = pu8Data[3];
				rxData[6] = pu8Data[2];
			}
			else
			{
				rxData[pu8Data[1]] = pu8Data[2] + (pu8Data[3] << 8);
			}

			//         vPrintf("rx %d %d  \r",pu8Data[0],rxData[pu8Data[0]]);
		}
		else if (PriorityDataLen == 5)//32 bit data
		{

			int f;
			memcpy(&f, &pu8Data[2], 4);
			rxData[pu8Data[1]] = f;

			if(pu8Data[1]==rxheightidx && initialHeight==-9999)initialHeight=f;
			if(pu8Data[1]==rxlatidx && initialLat==-9999)
			{
				initialLat=((double)f)/100000;
				longitudeScale=cos(initialLat*3.1415927/180)*60;
			}
			if(pu8Data[1]==rxlongidx && initialLong==-9999)
			{
				initialLong=((double)f)/100000;
			}
			if(pu8Data[1]==rxlongidx && initialLong!=-9999)
			{
				double dlat=((double)rxData[rxlatidx])/100000-initialLat;
				double dlong=((double)rxData[rxlongidx])/100000-initialLong;
				dlat*=60;
				dlong*=longitudeScale;
				double drange=sqrt(dlat*dlat + dlong*dlong);
				drange*=1852;//convert to meters
				range=(int)drange;
				//      flat earth stuff
				//      nn = cos(long)*60 calc once
				//      sqrt(((lat1-lat2)*60)^2 + ((long1-long2)*nn)^2)

			}
		}
	}
	if (u8Len - PriorityDataLen > 1)
	{
		handleLowPriorityData(pu8Data + PriorityDataLen + 1, u8Len
				- PriorityDataLen - 1);
	}
}

PRIVATE void GetNextChannel()
{

	uint32 newchannel =
			getNextInHopSequence(&(sCoordinatorData.u8ChannelSeqNo));

	sCoordinatorData.u8Channel = newchannel;

	(void) eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, newchannel);

	//toggle dio16 so sync can be checked with scope
	if ((sCoordinatorData.u8ChannelSeqNo & 1) == 1)
		vAHI_DioSetOutput(1 << 16, 0);
	else
		vAHI_DioSetOutput(0, 1 << 16);
}

/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{
	//todo major tidy - use tick timer for all events (like rx )to free up timer 1 for
	// future features such as ir or radio ppm output

	//capture ticktimer stuff - fires on each channel change


	if (psAHI_Ind->u32DeviceId == E_AHI_DEVICE_TICK_TIMER)
	{
		if (sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
		{

			sCoordinatorData.u8TxPacketSeqNb++;
			sCoordinatorData.u8TxPacketSeqNb &= 0x7f;

			GetNextChannel();
			channelAckCounter[sCoordinatorData.u8Channel - 11]++;

			flightTimer += 2;//simple way to scale to 1/100 sec

			pollTouchScreen();

			checkBattery();
			refreshGUI();

			if (backlightTimer > 0)
				backlightTimer--;
			else
				setBacklight(0);

			updatePcDisplay();

			if (subFrameIdx == 0 || PacketDone == 0)
			{
				RetryNo = 0;

				if (nRangeMeasurements > 500)
				{
					//  radioRange=totalTime/nRangeMeasurements;
					totalTime = 0;
					nRangeMeasurements = 0;
					if (lowestRange < 27514)
						radioRange = 0;
					else
						radioRange = (lowestRange - 27514) * 29979 / 3200;
					lowestRange = 1000000;

				}

				HoldOffAndSend();

				sentPackets++;
				if (sentPackets >= 100)
				{
					errorRate = lostPackets;

					sentPackets = 0;
					lostPackets = 0;
					int i;
					for (i = 0; i < 16; i++)
					{
						//channelAcks[i]=channelAckCounter[i];
						channelAcks[i] = channelRetryCounter[i];
						channelAckCounter[i] = 0;
						channelRetryCounter[i] = 0;
					}

					txacks = ackedPackets;

					updateDisplay();
					ackedPackets = 0;
					retryPackets = 0;

				}
			}
			subFrameIdx++;
			if (subFrameIdx >= framePeriod / hopPeriod)
				subFrameIdx = 0;

		}

	}
	if (psAHI_Ind->u32DeviceId == E_AHI_DEVICE_TIMER1)
	{
		//called as a result of a random holdoff timer
		//try to send a packet

		if (sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
		{
			retryPackets++;
			vCreateAndSendFrame();

			//            dbgmsgtimestart=u32AHI_TickTimerRead();

			if (RetryNo != 0)
				channelRetryCounter[sCoordinatorData.u8Channel - 11]++;

		}
	}
}

PRIVATE void HoldOffAndSend()
{

	uint32 backoff = (rand() % ((1 << (RetryNo + 2)) - 1)) * 320 + 128;

	//check we have enough time before channel change
	if ((backoff + (u32AHI_TickTimerRead() / 16)) < hopPeriod - 2500)
	{

		vAHI_TimerStartSingleShot(E_AHI_TIMER_1, 0, backoff);
	}
	else
	{

		lostPackets++;
	}

}

/****************************************************************************
 *
 * NAME: vTransmitDataPacket
 *
 ****************************************************************************/
PRIVATE void vTransmitDataPacket(uint8 *pu8Data, uint8 u8Len, uint16 u16DestAdr)
{
	MAC_McpsReqRsp_s sMcpsReqRsp;
	MAC_McpsSyncCfm_s sMcpsSyncCfm;
	uint8 *pu8Payload, i = 0;

	/* Create frame transmission request */
	sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
	sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
	/* Set handle so we can match confirmation to request */
	sMcpsReqRsp.uParam.sReqData.u8Handle = sCoordinatorData.u8TxPacketSeqNb;

	sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = TX_ADDRESS_MODE;
	sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = 0xffff;
	//   sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = COORDINATOR_ADR;
	if (TX_ADDRESS_MODE == 3)
	{
		module_MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L
				= macptr->u32L;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H
				= macptr->u32H;
	}

	/* Use long address and broadcast pan id for destination */
	sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = RX_ADDRESS_MODE;
	sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xffff;//sCoordinatorData.u16PanId;
	sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L
			= liveModel.rxMACl;
	sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H
			= liveModel.rxMACh;

	/* Frame requires ack but not security, indirect transmit or GTS */
	sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK;

	pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;

//	pu8Payload[0] = sCoordinatorData.u8TxPacketSeqNb;
	pu8Payload[0] = u8Len;

	for (i = 1; i < (u8Len + 1); i++)
	{
		pu8Payload[i] = *pu8Data++;
	}

	i += appendLowPriorityData(&pu8Payload[i], 32);

	/* Set frame length */
	sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = i;

	/* Request transmit */
	vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
	sendTime = u32AHI_TickTimerRead();
	dbgmsgtimestart = u32AHI_TickTimerRead();

}

PRIVATE uint16 u16ReadADCAverage(uint8 channel)
{

	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT, E_AHI_AP_INPUT_RANGE_2, channel);
	uint16 ret = 0;
	int i;
	for (i = 0; i < 4; i++)
	{
		vAHI_AdcStartSample(); /* start an AtoD sample here */
		while (bAHI_AdcPoll() != 0x00)
			; /* wait for conversion complete */
		ret += u16AHI_AdcRead();
	}
	return (ret >> 2); /* return result */
}

/****************************************************************************
 *
 * NAME: vCreateAndSendFrame
 *
 * DESCRIPTION:
 * Reads the inputs and tries to send a frame instantly
 *
 ****************************************************************************/
PRIVATE void vCreateAndSendFrame(void)
{

	currentAuxChannel++;
	if (currentAuxChannel > 19)
		currentAuxChannel = 4;

	uint8 au8Packet[PAYLOAD_SIZE];
	modelEx* mod = &liveModel;

	switch (inputMethod)
	{
	case PS2INPUT:
	{
		readPS2Controller(&ps2);

		//set trims
		if (ps2.RRight && mod->trim[0] < 2048)
			mod->trim[0] += 8;
		if (ps2.RLeft && mod->trim[0] > -2048)
			mod->trim[0] -= 8;

		if (ps2.RUp && mod->trim[1] < 2048)
			mod->trim[1] += 8;
		if (ps2.RDown && mod->trim[1] > -2048)
			mod->trim[1] -= 8;

		if (ps2.LRight && mod->trim[2] < 2048)
			mod->trim[2] += 8;
		if (ps2.LLeft && mod->trim[2] > -2048)
			mod->trim[2] -= 8;

		if (ps2.LUp && mod->trim[3] < 2048)
			mod->trim[3] += 8;
		if (ps2.LDown && mod->trim[3] > -2048)
			mod->trim[3] -= 8;

		//set joystick values
		txInputs[0] = (ps2.RJoyX << 4) - 2048;
		txInputs[1] = (-ps2.RJoyY << 4) + 2048;
		txInputs[2] = (ps2.LJoyY << 4) - 2048;
		txInputs[3] = (-ps2.LJoyX << 4) + 2048;

		//set aux channels
		if (ps2.LFire1)
		{
			activeAuxChannel++;
			if (activeAuxChannel > 19)
				activeAuxChannel = 4;
		}
		if (ps2.LFire2)
		{
			activeAuxChannel--;
			if (activeAuxChannel < 4)
				activeAuxChannel = 19;
		}

		if (ps2.RFire1)
		{
			txInputs[activeAuxChannel]++;
			//move changed channel to top of queue
			currentAuxChannel = activeAuxChannel;

			//       vPrintf("A%d %d ",activeAuxChannel,txInputs[activeAuxChannel]);
		}
		if (ps2.RFire2)
		{
			txInputs[activeAuxChannel]--;
			//move changed channel to top of queue
			currentAuxChannel = activeAuxChannel;

			//      vPrintf("A%d %d ",activeAuxChannel,txInputs[activeAuxChannel]);
		}
		// high and low rates on joystick buttons
		if (ps2.RJoy)
		{
			mod->rateMode = 0;
		}
		if (ps2.LJoy)
		{
			mod->rateMode = 1;
		}
		break;
	}
	case ADINPUT:
	{
		//3755=3.3v
		int ref = u16ReadADCAverage(E_AHI_ADC_SRC_VOLT);

		txInputs[0] = ((u16ReadADCAverage(0) - 2048) * 3755 / ref
				- joystickOffset[0]) * 2048 / joystickGain[0];
		txInputs[1] = ((u16ReadADCAverage(1) - 2048) * 3755 / ref
				- joystickOffset[1]) * 2048 / joystickGain[1];
		txInputs[2] = ((u16ReadADCAverage(2) - 2048) * 3755 / ref
				- joystickOffset[2]) * 2048 / joystickGain[2];
		txInputs[3] = ((u16ReadADCAverage(3) - 2048) * 3755 / ref
				- joystickOffset[3]) * 2048 / joystickGain[3];
		break;
	}
	case NUNCHUCKINPUT:
	{
		readWiiController(&wii);
		if (wii.ZButton == TRUE)
		{
			//use accelerometers
			txInputs[0] = (wii.AccelX << 3) - 4096;
			txInputs[1] = (wii.AccelY << 3) - 4096;

		}
		else
		{
			//use joystick
			txInputs[0] = (wii.JoyX << 4) - 2048;
			txInputs[1] = (wii.JoyY << 4) - 2048;

		}

		break;
	}
	case PPMINPUT:
	{
		ppmRead(txInputs);
		break;
	}
	case SERIALINPUT:
		// Do nothing
		break;
	case RAWINPUT:
		// Do nothing
		break;
	}

	// Do mixing
	if (inputMethod == RAWINPUT)
	{
		// No input device or mixing, just raw
		memcpy(txDemands, txInputs, sizeof(txDemands) / sizeof(txDemands[0]));
	}
	else
	{
		doMixingEx(txInputs, txDemands, &liveModel);
	}

	//    vPrintf("%d %d  \r",txDemands[0],txDemands[1]);

	// Pack into packet

	uint32 packetIdx=2;

	au8Packet[packetIdx++] = txDemands[0] & 0x00ff;
	au8Packet[packetIdx++] = ((txDemands[0] & 0x0f00) >> 4) + (txDemands[1] & 0x000f);
	au8Packet[packetIdx++] = (txDemands[1] & 0x0ff0) >> 4;

	au8Packet[packetIdx++] = txDemands[2] & 0x00ff;
	au8Packet[packetIdx++] = ((txDemands[2] & 0x0f00) >> 4) + (txDemands[3] & 0x000f);
	au8Packet[packetIdx++] = (txDemands[3] & 0x0ff0) >> 4;

	if(liveModel.nFullSpeedChannels>4)
	{
		au8Packet[packetIdx++] = txDemands[4] & 0x00ff;
		au8Packet[packetIdx++] = ((txDemands[4] & 0x0f00) >> 4) + (txDemands[5] & 0x000f);
		au8Packet[packetIdx++] = (txDemands[5] & 0x0ff0) >> 4;
	}
	if(liveModel.nFullSpeedChannels>6)
	{
		au8Packet[packetIdx++] = txDemands[6] & 0x00ff;
		au8Packet[packetIdx++] = ((txDemands[6] & 0x0f00) >> 4) + (txDemands[7] & 0x000f);
		au8Packet[packetIdx++] = (txDemands[7] & 0x0ff0) >> 4;
	}

	au8Packet[packetIdx++] = ((currentAuxChannel - 4) << 4)
			+ (txDemands[currentAuxChannel] & 0x000f);
	au8Packet[packetIdx++] = (txDemands[currentAuxChannel] & 0x0ff0) >> 4;

	// Set packet time in 0.01ms units - just fits in 16 bits
	int t = (u32AHI_TickTimerRead() + sCoordinatorData.u8ChannelSeqNo
			* hopPeriod * 16) / 160;
	au8Packet[0] = t & 0x00FF;
	au8Packet[1] = t >> 8;

	PacketDone = 0;

	vTransmitDataPacket(au8Packet, packetIdx, 1);
}

void toggleBackLight(clickEventArgs* clickargs)
{
	//one day this could be dimable
	if (backlight == 255)
	{
		setBacklight(0);
	}
	else
	{
		setBacklight(255);
	}
}

void setBacklight(uint8 bri)
{
	backlight = bri;
	if (bri > 0)
	{
		vAHI_DioSetOutput(E_AHI_DIO17_INT, 0);
	}
	else
	{
		vAHI_DioSetOutput(0, E_AHI_DIO17_INT);
	}
}

void updateDisplay()
{
	//   txbat=u16ReadADC(E_AHI_ADC_SRC_VOLT);//3838 4096 = 2.4*1.5 =3.6v
	//   txbat=txbat*360/4096;


	rxbat = rxData[14];

	txretries = retryPackets - 100;
	rxpackets = rxData[6];

	if (totalRxFrames > 4* 50 )
	{
		if (rxpackets < lowestRxFrameRate)
			lowestRxFrameRate = rxpackets;
	}

	rxData[6] = 0;

	//   renderPage(pages[currentPage].controls,pages[currentPage].len,lastPage!=currentPage,lcdbuf,128);
	//   LcdBitBlt(lcdbuf,128,0,0,128,64,&lcd);
	//   lastPage=currentPage;

	//crude rx battery alarm
	bool alarm=FALSE;
	//no alarm if no reading
	if(rxbat>0)
	{
		//really need to do this on a per model basis
		if(rxbat<310)alarm=TRUE; //single cell
		if(rxbat>540 && rxbat<620)alarm=TRUE; // 2cell lipo but allow for case of 5v bec
		if(rxbat>860 && rxbat<930)alarm=TRUE; // 3 cell allowing for fresh 2 cell
	}
	if(alarm)
	{
		vAHI_DioSetOutput(ALARM_PIN , 0);
	}
	else
	{
		vAHI_DioSetOutput(0,ALARM_PIN );
	}

}

void updatePcDisplay()
{
	//crudely send part of the bitmap on each frame

	static uint8 blockidx = 0;
	uint8 bits[]={1,2,4,8,16,32,64,128};
	int row=blockidx/8;
	int bit=blockidx%8;
	int tries=0;

	while(tries<64)
	{
		if(bit==0)
		{
			if(rowValid[row]==0)
			{
				tries+=8;
				row++;
				if(row>7)row=0;
				continue;
			}
		}
		if((rowValid[row] & bits[bit])!=0)
		{
			blockidx=row*8+bit;
			pcComsSendPacket2(lcdbuf, blockidx * 16, 16, 0x92, blockidx);
			rowValid[row]^=bits[bit];

			break;
		}
		bit++;
		if(bit>7)
		{
			bit=0;
			row++;
			if(row>7)row=0;
		}

		tries++;
	}
	blockidx++;
	if (blockidx >= 64)blockidx = 0;
}
void updatePcDisplayOld()
{
	//crudely send part of the bitmap on each frame

	static uint8 blockidx = 0;
	int i;
	int j;
	int bit;
	bool regionFound=FALSE;
	for(i=0;i<8;i++)
	{
		uint8 blockValid=rowValid[i];
		if(blockValid!=0)
		{
			bit=1;
			for(j=0;j<8;j++)
			{
				if((blockValid & bit)!=0)
				{
					rowValid[i]^=bit;
					blockidx=i*8+j;
					pcComsSendPacket2(lcdbuf, blockidx * 16, 16, 0x92, blockidx);
					regionFound=TRUE;
					break;
				}
				bit<<=1;
			}
		}
		if(regionFound)break;
	}


//	pcComsSendPacket2(lcdbuf, blockidx * 16, 16, 0x92, blockidx);

	blockidx++;
	if (blockidx >= 64)
		blockidx = 0;
}


void checkBattery()
{
	// soft successive approximation 11 bit adc, just like the bad old days
	static uint16 step = 512;
	static uint16 lastDacTry = 1023;

	if (step > 0)
	{
		if ((u8AHI_ComparatorStatus() & E_AHI_AP_COMPARATOR_MASK_1) != 0)
		{
			//dac is below bat voltage
			lastDacTry += step;
		}
		else
		{
			lastDacTry -= step;
		}
		step = step >> 1;
	}
	else
	{
		txbat = 240* ( (int)lastDacTry)*6 *495/518/2048;
		step=512;
		lastDacTry=1023;
	}
	//jn5148 has 12 bit dacs
#ifdef JN5148
	vAHI_DacOutput (E_AHI_AP_DAC_1,lastDacTry<<1);
#else
	vAHI_DacOutput (E_AHI_AP_DAC_1,lastDacTry);
#endif
}

void CalibrateJoysticksTopRight()
{
}

void CalibrateJoysticksBottomLeft()
{
}

void CalibrateJoysticksNeutral()
{
}

void loadModel(int idx)
{
	store s;

	//storeSettings();
	if (getOldStore(&s))
	{
		if (liveModelIdx < numModels )
		{
			liveModelIdx=idx;
			loadModelByIdx(&s, &liveModel, liveModelIdx);

			enableModelComs();
		}
	}
}
void nextModel()
{
	store s;

	//storeSettings();
	if (getOldStore(&s))
	{
		if (liveModelIdx < numModels - 1)
			liveModelIdx++;
		else
			liveModelIdx = 0;

		loadModelByIdx(&s, &liveModel, liveModelIdx);

		enableModelComs();
	}
}

void copyModel()
{
	//save any changes to current model
	storeSettings();

	//simply leave current stuff in liveModel structure
	liveModelIdx = numModels;
	numModels++;
	strcpy(liveModel.name, "New Model");
}

void storeSettings()
{
	//todo untested on JN5148
	pcComsPrintf("Store settings\r\n");

	store s;
	store old;
	store* pold;
	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	if (getOldStore(&old) == TRUE)
	{
		pold = &old;
	}
	else
	{
		pold = NULL;
	}

	getNewStore(&s);

	//   pcComsPrintf("s %d %d %d",s.base,old.base,old.size);

	saveGeneralSettings(&s);
	saveRadioSettings(&s);
	storeModels(&s, &liveModel, pold, liveModelIdx);

	commitStore(&s);
}

void loadSettings()
{
	pcComsPrintf("load Settings \r\n");

	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	store s;
	store section;
	int tag;
	if (getOldStore(&s) == TRUE)
	{
		while ((tag = storeGetSection(&s, &section)) > 0)
		{
			//   pcComsPrintf("tag found %d %d\r\n",tag,section.size);

			switch (tag)
			{
			case STOREMODELSSECTION:
				pcComsPrintf("model section found %d\r\n", section.size);

				readModels(&section, &liveModel, &liveModelIdx, &numModels);
				pcComsPrintf("models found %d %d \r\n", numModels, liveModelIdx);

				break;

			case STORERADIOSECTION:
				loadRadioSettings(&section);
				break;

			case STOREGENERALSECTION:
				loadGeneralSettings(&section);
				break;

			}
		}
	}
	else
	{
		loadDefaultSettings();
	}
	if (numModels == 0)
		loadDefaultSettings();
}

void loadGeneralSettings(store* s)
{
	store section;
	int tag;
	while ((tag = storeGetSection(s, &section)) > 0)
	{
		switch (tag)
		{
		case STOREDEFAULTINPUT:
			inputMethod=readUint8(&section);
			break;
		}
	}
}

void saveGeneralSettings(store* s)
{
	store section;
	storeStartSection(s, STOREGENERALSECTION, &section);

	storeUint8Section(&section, STOREDEFAULTINPUT, (uint8)inputMethod);

	storeEndSection(s, &section);
}

void loadRadioSettings(store* s)
{
	store section;
	int tag;
	while ((tag = storeGetSection(s, &section)) > 0)
	{
		switch (tag)
		{
		case STORERADIOHIGHPOWER:
			if (readUint8(&section) == 0)
				useHighPowerModule = TRUE;
			else
				useHighPowerModule = FALSE;
			break;
		}
	}
}

void saveRadioSettings(store* s)
{
	store section;
	storeStartSection(s,STORERADIOSECTION,&section);
	if (useHighPowerModule == TRUE)
		storeUint8Section(&section, STORERADIOHIGHPOWER, 0);
	else
		storeUint8Section(&section, STORERADIOHIGHPOWER, 1);
	storeEndSection(s,&section);
}

void loadDefaultSettings()
{
	//	setupAlula(&liveModel);
	setupDefaultModel(&liveModel);
	numModels = 1;
	liveModelIdx = 0;
}

void sleep()
{
	// TODO untested on JN5148

	vAHI_TickTimerIntEnable(FALSE);
	vAHI_TimerDisable(E_AHI_TIMER_0);
	vAHI_TimerDisable(E_AHI_TIMER_1);

	storeSettings();

	//turn off power consuming bits
	//lights
	setBacklight(0);
	//agnd
	vAHI_DioSetOutput(0, E_AHI_DIO18_INT);
	//touchscreen
	sleepTsc2003(&touchScreen);
	//display
	sleepLcdEADog(&lcd);
	//
	//   vAHI_HighPowerModuleEnable(FALSE,FALSE);

	vAHI_SiConfigure(FALSE, FALSE, 16);

	// vAHI_SpiConfigure( 0, /* number of slave select lines in use */
	//        E_AHI_SPIM_MSB_FIRST, /* send data MSB first */
	//        FALSE,
	//        FALSE,
	//        1,//8MHz
	//        E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
	//        E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */

	// vAHI_UartDisable(E_AHI_UART_0);
	//vAHI_UartDisable(E_AHI_UART_1);

	//  vAHI_DioSetDirection(lcd.resetpin,0);
	//   vAHI_DioSetPullup(lcd.resetpin,0);
	//   vAHI_DioSetDirection(lcd.a0pin,0);
	//   vAHI_DioSetPullup(lcd.a0pin,0);
	//   vAHI_DioSetDirection(lcd.spicspin,0);
	//   vAHI_DioSetPullup(lcd.spicspin,0);

	//vAHI_DioSetOutput(0,lcd.resetpin);
	//vAHI_DioSetOutput(lcd.spicspin,0);

#ifdef JN5148

#else
	vAppApiSetBoostMode(FALSE);
#endif
	//if you don't do this the high power amp does not sleep
	MAC_vPibSetRxOnWhenIdle(s_pvMac, FALSE, FALSE);

	vAHI_ComparatorDisable(E_AHI_AP_COMPARATOR_1);
	vAHI_ComparatorDisable(E_AHI_AP_COMPARATOR_2);

	vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
	vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_0, 32* 1000* 5 ) ;//5 second wakeup
	vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);

	//tx 1.0 hw currently uses 70ua whilst sleeping
	//about 45uA unaccounted for!
	//at 5v bat monitor uses 16uA - could use higher value resistors
	//ram retention 2.4uA
	//sleep  with timer 1.2uA
	//tsc2003 should be 3uA
	//eadog lcd should be 150uA on 0.1uA sleep
	//
}

void enableModelComs()
{

	randomizeHopSequence(liveModel.rxMACh ^ liveModel.rxMACl);
	setHopMode(hoppingContinuous);

}

void setTxMode(teState state)
{
	sCoordinatorData.eState = state;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
