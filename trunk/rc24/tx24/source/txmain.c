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
#include <printf.h>
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
#include "pcComs.h"
#include "swEventQueue.h"
#include "tsc2003.h"

#include "smbus.h"

#include "routedmessage.h"
#include "codeupdate.h"
#include "radiocoms.h"


#define HIPOWERMODULE

/* Data type for storing data related to all end devices that have associated */
typedef struct
{
    uint16 u16ShortAdr;
    uint32 u32ExtAdrL;
    uint32 u32ExtAdrH;
}tsEndDeviceData;

typedef struct
{
    /* Data related to associated end devices */
    uint16          u16NbrEndDevices;
    tsEndDeviceData sEndDeviceData;

    teState eState;

    uint8   u8Channel;
    uint8   u8TxPacketSeqNb;
    uint8   u8RxPacketSeqNb;
    uint16  u16PanId;
    uint8   u8ChannelSeqNo;

}tsCoordinatorData;



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
PRIVATE uint16 u16ReadADC(uint8 channel);
PRIVATE void vCreateAndSendFrame(void);

PRIVATE void GetNextChannel(void);
PRIVATE void HoldOffAndSend(void);


//routed coms functions
void rxComsSendRoutedPacket(uint8* msg, int offset, uint8 len);



void updateDisplay(void);
void updatePcDisplay(void);

// ui events

void displayDrag(int dx, int dy);

void pageUp(clickEventArgs* clickargs);
void pageDown(clickEventArgs* clickargs);
void setModelUp(clickEventArgs* clickargs);
void setModelDown(clickEventArgs* clickargs);
void toggleBackLight(clickEventArgs* clickargs);

void trim4lClick(clickEventArgs* clickargs);
void trim4rClick(clickEventArgs* clickargs);
void trim1lClick(clickEventArgs* clickargs);
void trim1rClick(clickEventArgs* clickargs);
void trim3uClick(clickEventArgs* clickargs);
void trim3dClick(clickEventArgs* clickargs);
void trim2uClick(clickEventArgs* clickargs);
void trim2dClick(clickEventArgs* clickargs);
void rateClick(clickEventArgs* clickargs);
void homeClick(clickEventArgs* clickargs);

void bindMixEditor(void);
void editMixMixUpClick(clickEventArgs* clickargs);
void editMixMixDownClick(clickEventArgs* clickargs);
void editMixInUpClick(clickEventArgs* clickargs);
void editMixInDownClick(clickEventArgs* clickargs);
void copyModelClick(clickEventArgs* clickargs);
void bindModelClick(clickEventArgs* clickargs);

void virtualKeyDelClick(clickEventArgs* clickargs);
void virtualKeyCapsClick(clickEventArgs* clickargs);
void virtualKeyClick(clickEventArgs* clickargs);

void setBacklight(uint8 bri);
void checkBattery(void);

void storeSettings(void);
void loadSettings(void);
void loadDefaultSettings(void);

void sleepClick(clickEventArgs* clickargs);
void sleep(void);

void enableModelComs(void);
void txSendRoutedMessage(uint8* msg,uint8 len,uint8 toCon);


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
PRIVATE int ackedPackets=0;
PRIVATE uint32 retryPackets=0;
PRIVATE uint32 errorRate;
PRIVATE uint16 PacketDone=1;
int rxLinkQuality=0;
int txLinkQuality=0;
int channelAcks[16];
int channelAckCounter[16];
int channelRetryCounter[16];

int totalLostFrames=0;
int lowestRxFrameRate=100;

int totalRxFrames=0;
int flightTimer=0;

uint8 MaxRetries=5;
uint8 RetryNo=0;

ps2ControllerOp ps2;
WiiController wii;
tsc2003 touchScreen;

//times in micro seconds
uint32 hopPeriod=20000;
uint32 framePeriod=20000;
uint8 subFrameIdx=0;

int txDemands[20];
int txInputs[20];
int currentAuxChannel=4;

int activeAuxChannel=4;

int activeModel=0;

// only keep one model in ram at a time to avoid artificial limits on number
// of models
modelEx liveModel;
int16 liveModelIdx=0;
int16 numModels=0;

int rxData[256];

#define rxspeedidx 7
#define rxheightidx 8
#define rxtrackidx 9
#define rxtimeidx 10
#define rxlatidx 11
#define rxlongidx 12

//initial position
int initialHeight=-9999;
double initialLat=-9999;
double initialLong=-9999;
double longitudeScale=0.0;
int range=0;


int joystickOffset[4];
int joystickGain[4];

lcdEADog lcd;
//screen buffer
uint8 lcdbuf[8*128] __attribute__ ((aligned (4)));


#define PS2INPUT 0
#define ADINPUT 1
#define PWMINPUT 2
#define NUNCHUCKINPUT 3
#define PPMINPUT 4

int inputMethod=ADINPUT;


volatile uint32 sendTime;
uint32 radioRange;
volatile uint32 lowestRange;
volatile uint32 totalTime;
volatile uint32 nRangeMeasurements;

uint8 backlight=255;
uint16 batDac=1023;

int backlightTimer=50*60*1;


//build display controls and pages

// images imported using rc24 config pc program
uint8 imgUpButton[] = {0,252,2,2,2,2,2,2,2,130,194,226,242,226,194,130,2,2,2,2,2,2,2,252
	,0,255,0,0,16,24,28,30,31,31,255,255,255,255,255,31,31,30,28,24,16,0,0,255
	,0,127,128,128,128,128,128,128,128,128,159,159,159,159,159,128,128,128,128,128,128,128,128,127
	};

uint8 imgDownButton[] = {0,252,2,2,2,2,2,2,2,2,242,242,242,242,242,2,2,2,2,2,2,2,2,252
	,0,255,0,0,16,48,112,240,240,240,255,255,255,255,255,240,240,240,112,48,16,0,0,255
	,0,127,128,128,128,128,128,128,129,131,135,143,159,143,135,131,129,128,128,128,128,128,128,127
	};

uint8 imgLeftButton[] = {0,252,2,2,2,2,2,2,2,130,194,226,242,2,2,2,2,2,2,2,2,2,2,252
	,0,255,0,0,16,56,124,254,255,255,255,255,255,126,126,126,126,126,126,126,126,0,0,255
	,0,127,128,128,128,128,128,128,129,131,135,143,159,128,128,128,128,128,128,128,128,128,128,127
	};

uint8 imgRightButton[] = {0,252,2,2,2,2,2,2,2,2,2,2,242,226,194,130,2,2,2,2,2,2,2,252
	,0,255,0,0,124,124,124,124,124,124,124,124,255,255,255,255,255,254,124,56,16,0,0,255
	,0,127,128,128,128,128,128,128,128,128,128,128,159,143,135,131,129,128,128,128,128,128,128,127
	};

uint8 hiicon[] = {0,252,2,242,242,2,2,242,242,2,50,242,242,50,2,252
	,0,127,128,159,159,131,131,159,159,128,152,159,159,152,128,127
	};

uint8 lowicon[] = {0,252,2,242,242,2,2,2,2,242,242,50,242,242,2,254
	,0,255,128,159,159,152,152,152,128,159,159,152,159,159,128,255
	};

uint8 homeicon[] = {0,252,2,2,130,130,66,66,34,34,18,18,10,18,18,34,34,122,122,130,130,2,2,252
	,0,255,0,1,255,0,0,14,10,202,78,64,64,64,78,202,10,14,0,0,255,1,0,255
	,0,127,128,128,159,144,144,144,144,159,144,144,144,144,144,159,144,144,144,144,159,128,128,127
	};

uint8 homeicon16x16[] = {0,252,2,130,194,98,50,26,14,26,50,122,194,130,2,252
	,0,127,128,128,159,144,145,156,146,156,145,144,159,128,128,127
	};

uint8 upicon22x24[] = {0,252,2,2,2,2,2,2,130,194,226,242,226,194,130,2,2,2,2,2,2,252
	,0,255,0,0,8,12,14,15,15,15,255,255,255,15,15,15,14,12,8,0,0,255
	,0,127,128,128,128,128,128,128,128,128,143,143,143,128,128,128,128,128,128,128,128,127
	};

uint8 downicon22x24[] = {0,252,2,2,2,2,2,2,2,2,242,242,242,2,2,2,2,2,2,2,2,252
	,0,255,0,0,16,48,112,240,240,240,255,255,255,240,240,240,112,48,16,0,0,255
	,0,127,128,128,128,128,128,128,129,131,135,143,135,131,129,128,128,128,128,128,128,127
	};

uint8 lefticon21x32[] = {0,252,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,6,252
	,0,255,0,0,0,128,192,224,240,248,252,0,0,0,0,0,0,0,0,0,255
	,0,255,0,2,7,15,31,63,127,255,255,7,7,7,7,7,7,7,0,0,255
	,0,127,128,128,128,128,128,128,128,128,129,128,128,128,128,128,128,128,128,128,127
	};

uint8 righticon21x32[] = {0,252,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,252
	,0,255,0,0,0,0,0,0,0,0,0,252,248,240,224,192,128,0,0,0,255
	,0,255,0,7,7,7,7,7,7,7,7,255,255,127,63,31,15,7,2,0,255
	,0,127,128,128,128,128,128,128,128,128,128,129,128,128,128,128,128,128,128,128,127
	};

uint8 lefticon8x8[] = {0,8,28,62,127,28,28,28
	};
uint8 righticon8x8[] = {0,28,28,28,127,62,28,8
	};
uint8 upicon8x8[] = {8,12,126,127,126,12,8,0
	};
uint8 downicon8x8[] = {8,24,63,127,63,24,8,0
	};

int txbat=0;
int rxbat=0;
int txretries=0;
int rxpackets=0;
int txacks=0;

visualControl* focusedControl=NULL;

labelControl headerLabel={"tx24 2.09",0,0,48,8,8,0,TRUE,FALSE,NULL};
labelControl modelLabel={&liveModel.name,48,0,64,8,8,0,TRUE,FALSE,NULL};

labelControl txbatLabel={"tx bat",0,8,30,8,8,0,TRUE,FALSE,NULL};
numberControl txbatVal={&txbat,36,8,20,8,8,0,TRUE,0,2,"i"};

labelControl rxbatLabel={"rx bat",0,16,30,8,8,0,TRUE,FALSE,NULL};
numberControl rxbatVal={&rxbat,36,16,20,8,8,0,TRUE,0,2,"i"};

numberControl acksVal={&txacks,60,8,30,8,8,0,TRUE,0,0,"i"};
numberControl retryVal={&txretries,80,8,25,8,8,0,TRUE,0,0,"i"};
numberControl rxerrorVal={&rxpackets,60,16,30,8,8,0,TRUE,0,0,"i"};
numberControl txLinkQ={&txLinkQuality,105,8,23,8,8,0,TRUE,0,0,"i"};
numberControl rxLinkQ={&rxLinkQuality,105,16,23,8,8,0,TRUE,0,0,"i"};

labelControl rxspeedLabel={"speed",0,32,30,8,8,0,TRUE,FALSE,NULL};
numberControl rxspeedVal={&rxData[rxspeedidx],36,32,24,8,8,0,TRUE,0,3,"i"};
labelControl rxheightLabel={"height",64,32,30,8,8,0,TRUE,FALSE,NULL};
numberControl rxheightVal={&rxData[rxheightidx],96,32,30,8,8,0,TRUE,0,0,"i"};
labelControl rxrangeLabel={"range",0,48,30,8,8,0,TRUE,FALSE,NULL};
numberControl rxrangeVal={&radioRange,36,48,64,8,8,0,TRUE,0,0,"i"};

numberControl rxWorst={&lowestRxFrameRate,0,56,24,8,8,0,TRUE,0,0,"i"};
numberControl flightTimerVal={&flightTimer,64,56,64,8,8,0,TRUE,0,0,"i"};

barControl bar1={&channelAcks[0],0,24,4,8,8,0,TRUE,0,0};
barControl bar2={&channelAcks[1],5,24,4,8,8,0,TRUE,0,0};
barControl bar3={&channelAcks[2],10,24,4,8,8,0,TRUE,0,0};
barControl bar4={&channelAcks[3],15,24,4,8,8,0,TRUE,0,0};
barControl bar5={&channelAcks[4],20,24,4,8,8,0,TRUE,0,0};
barControl bar6={&channelAcks[5],25,24,4,8,8,0,TRUE,0,0};
barControl bar7={&channelAcks[6],30,24,4,8,8,0,TRUE,0,0};
barControl bar8={&channelAcks[7],35,24,4,8,8,0,TRUE,0,0};
barControl bar9={&channelAcks[8],40,24,4,8,8,0,TRUE,0,0};
barControl bar10={&channelAcks[9],45,24,4,8,8,0,TRUE,0,0};
barControl bar11={&channelAcks[10],50,24,4,8,8,0,TRUE,0,0};
barControl bar12={&channelAcks[11],55,24,4,8,8,0,TRUE,0,0};
barControl bar13={&channelAcks[12],60,24,4,8,8,0,TRUE,0,0};
barControl bar14={&channelAcks[13],65,24,4,8,8,0,TRUE,0,0};
barControl bar15={&channelAcks[14],70,24,4,8,8,0,TRUE,0,0};
barControl bar16={&channelAcks[15],75,24,4,8,8,0,TRUE,0,0};


imageControl p1Down={imgDownButton,104,40,24,24,TRUE,FALSE,pageDown};
imageControl p1Up={imgUpButton,104,0,24,24,TRUE,FALSE,pageUp};

visualControl page1[]={{&headerLabel,dctLabel},
                        {&modelLabel,dctLabel},
                        {&txbatLabel,dctLabel},
                        {&txbatVal,dctNumber},
                        {&rxbatLabel,dctLabel},
                        {&rxbatVal,dctNumber},
                        {&acksVal,dctNumber},
                        {&retryVal,dctNumber},
                        {&rxerrorVal,dctNumber},
                        {&rxLinkQ,dctNumber},
                        {&txLinkQ,dctNumber},
                        {&bar1,dctBar},
                        {&bar2,dctBar},
                        {&bar3,dctBar},
                        {&bar4,dctBar},
                        {&bar5,dctBar},
                        {&bar6,dctBar},
                        {&bar7,dctBar},
                        {&bar8,dctBar},
                        {&bar9,dctBar},
                        {&bar10,dctBar},
                        {&bar11,dctBar},
                        {&bar12,dctBar},
                        {&bar13,dctBar},
                        {&bar14,dctBar},
                        {&bar15,dctBar},
                        {&bar16,dctBar},
                        {&rxspeedLabel,dctLabel},
                        {&rxspeedVal,dctNumber},
                        {&rxheightLabel,dctLabel},
                        {&rxheightVal,dctNumber},
                        {&rxrangeLabel,dctLabel},
                        {&rxrangeVal,dctNumber},
                        {&rxWorst,dctNumber},
                        {&flightTimerVal,dctNumber},
                        {&p1Up,dctImage},
                        {&p1Down,dctImage}
                       };
//basic settings page

labelControl p2Label={"Select Model",0,0,64,8,8,0,TRUE,FALSE,NULL};
labelControl p2modelUp={"Up",0,16,64,8,8,0,TRUE,FALSE,setModelUp};
labelControl p2modelLabel={liveModel.name,0,32,64,8,8,0,TRUE,FALSE,NULL};
labelControl p2modelDown={"Down",0,48,64,8,8,0,TRUE,FALSE,setModelDown};
imageControl p2light={homeicon,64,0,24,24,TRUE,FALSE,toggleBackLight};
imageControl p2Off={homeicon,64,24,24,24,TRUE,FALSE,sleepClick};

visualControl page2[]={{&p2Label,dctLabel},
                       {&p2modelUp,dctLabel},
                       {&p2modelLabel,dctLabel},
                       {&p2modelDown,dctLabel},
                       {&p2light,dctImage},
                       {&p2Off,dctImage},
                       {&p1Up,dctImage},
                       {&p1Down,dctImage}

                    };


//test and debug page

int tsX,tsY,tsP;
int dbc1=0;

numberControl tsx={&tsX,0,32,30,8,8,0,TRUE,0,0,"i"};
numberControl tsy={&tsY,48,32,30,8,8,0,TRUE,0,0,"i"};
numberControl tsp={&tsP,96,32,30,8,8,0,TRUE,0,0,"i"};
numberControl ncdbc1={&dbc1,0,8,30,8,8,0,TRUE,0,0,"i"};


numberControl ch1={&txInputs[0],0,0,60,8,8,0,TRUE,0,0,"i"};
numberControl ch2={&txInputs[1],64,0,60,8,8,0,TRUE,0,0,"i"};

visualControl tsTest[]={{&tsx,dctNumber},
                       {&tsy,dctNumber},
                       {&tsp,dctNumber},
                       {&p1Up,dctImage},
                       {&p1Down,dctImage},
                       {&ch1,dctNumber},
                       {&ch2,dctNumber},
                       {&ncdbc1,dctNumber}
                    };

//primary trim page

imageControl trim4l={lefticon21x32,0,16,21,32,TRUE,FALSE,trim4lClick};
imageControl trim4r={righticon21x32,43,16,21,32,TRUE,FALSE,trim4rClick};
imageControl trim1l={lefticon21x32,64,16,21,32,TRUE,FALSE,trim1lClick};
imageControl trim1r={righticon21x32,107,16,21,32,TRUE,FALSE,trim1rClick};

imageControl trim3u={upicon22x24,21,0,22,24,TRUE,FALSE,trim3uClick};
imageControl trim3d={downicon22x24,21,40,22,24,TRUE,FALSE,trim3dClick};
imageControl trim2u={upicon22x24,85,0,22,24,TRUE,FALSE,trim2uClick};
imageControl trim2d={downicon22x24,85,40,22,24,TRUE,FALSE,trim2dClick};

imageControl trimrate={hiicon,21,24,16,16,TRUE,FALSE,rateClick};
imageControl trimhome={homeicon16x16,85,24,16,16,TRUE,FALSE,homeClick};

visualControl primaryTrimPage[]={{&trim4l,dctImage},
                       {&trim4r,dctImage},
                       {&trim1l,dctImage},
                       {&trim1r,dctImage},
                       {&trim3u,dctImage},
                       {&trim3d,dctImage},
                       {&trim2u,dctImage},
                       {&trim2d,dctImage},
                       {&trimrate,dctImage},
                       {&trimhome,dctImage}
                        };

//edit model page

labelControl editModModLabel={&liveModel.name,0,0,100,8,D_STD_FONT,D_NORMAL,D_VISIBLE,D_REDRAW,NULL};
labelControl editModCopyLabel={"Copy",0,8,36,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,copyModelClick};
labelControl editModBindLabel={"Bind",64,8,36,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,bindModelClick};

visualControl editModelPage[]={{&editModModLabel,dctLabel},
                       {&editModCopyLabel,dctLabel},
                       {&editModBindLabel,dctLabel},
                       {&p1Up,dctImage},
                       {&p1Down,dctImage}
                       };


//imageControl editModModDown={downicon8x8,24,0,8,8,TRUE,FALSE,editModModDownClick};
//imageControl editModModUp={upicon8x8,32,0,8,8,TRUE,FALSE,editModModUpClick};
//labelControl editModModVal={&liveModel.name,40,0,24,8,8,0,TRUE,0,0,"i"};



//edit mixes page
int currentMixEdit=0;

labelControl editMixMixLabel={"Mix",0,0,24,8,8,0,TRUE,FALSE,NULL};
imageControl editMixMixDown={downicon8x8,24,0,8,8,TRUE,FALSE,editMixMixDownClick};
numberControl editMixMixVal={&currentMixEdit,32,0,24,8,8,0,TRUE,0,0,"i"};
imageControl editMixMixUp={upicon8x8,56,0,8,8,TRUE,FALSE,editMixMixUpClick};

labelControl editMixInLabel={"In",0,8,24,8,8,0,TRUE,FALSE,NULL};
imageControl editMixInDown={downicon8x8,24,8,8,8,TRUE,FALSE,editMixInDownClick};
numberControl editMixInVal={&liveModel.mixes[0].inChannel,32,8,24,8,8,0,TRUE,0,0,"b"};
imageControl editMixInUp={upicon8x8,56,8,8,8,TRUE,FALSE,editMixInUpClick};

labelControl editMixOutLabel={"Out",0,16,24,8,8,0,TRUE,FALSE,NULL};
numberControl editMixOutVal={&liveModel.mixes[0].outChannel,32,16,24,8,8,0,TRUE,0,0,"b"};

labelControl editMixLoLabel={"Lo",0,24,24,8,8,0,TRUE,FALSE,NULL};
numberControl editMixLoVal={&liveModel.mixes[0].rateLow,32,24,24,8,8,0,TRUE,0,0,"hi"};

labelControl editMixHiLabel={"Hi",0,32,24,8,8,0,TRUE,FALSE,NULL};
numberControl editMixHiVal={&liveModel.mixes[0].rateHigh,32,32,24,8,8,0,TRUE,0,0,"hi"};


visualControl editMixPage[]={
                                {&editMixMixLabel,dctLabel},
                                {&editMixMixUp,dctImage},
                                {&editMixMixVal,dctNumber},
                                {&editMixMixDown,dctImage},
                                {&editMixInLabel,dctLabel},
                                {&editMixInUp,dctImage},
                                {&editMixInVal,dctNumber},
                                {&editMixInDown,dctImage},
                                {&editMixOutLabel,dctLabel},
                                {&editMixOutVal,dctNumber},
                                {&editMixLoLabel,dctLabel},
                                {&editMixLoVal,dctNumber},
                                {&editMixHiLabel,dctLabel},
                                {&editMixHiVal,dctNumber},
                                {&p1Up,dctImage},
                                {&p1Down,dctImage}

                            };

//string editor page - on screen keyboard
labelControl editStringVal={liveModel.name,0,0,100,8,D_STD_FONT,D_NORMAL,D_VISIBLE,D_REDRAW,virtualKeyClick};

labelControl editString1={"1",4,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString2={"2",13,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString3={"3",22,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString4={"4",31,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString5={"5",40,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString6={"6",49,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString7={"7",58,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString8={"8",67,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString9={"9",76,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editString0={"0",85,16,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};

labelControl editStringq={"q",8,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringw={"w",17,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringe={"e",26,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringr={"r",35,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringt={"t",44,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringy={"y",53,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringu={"u",62,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringi={"i",71,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringo={"o",80,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringp={"p",89,24,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};

labelControl editStringa={"a",12,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStrings={"s",21,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringd={"d",30,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringf={"f",39,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringg={"g",48,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringh={"h",57,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringj={"j",66,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringk={"k",75,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringl={"l",84,32,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};

labelControl editStringz={"z",17,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringx={"x",26,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringc={"c",35,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringv={"v",44,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringb={"b",53,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringn={"n",62,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};
labelControl editStringm={"m",71,40,8,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};

labelControl editStringSpace={" ",16,48,64,8,D_STD_FONT,D_INVERSE,D_VISIBLE,D_REDRAW,virtualKeyClick};

imageControl editStringDel={lefticon8x8,88,48,8,8,TRUE,FALSE,virtualKeyDelClick};
imageControl editStringCaps={upicon8x8,0,32,8,8,TRUE,FALSE,virtualKeyCapsClick};


visualControl editStringPage[]={{&editStringDel,dctImage},
                                {&editStringVal,dctLabel},

                                {&editString1,dctLabel},
                                {&editString2,dctLabel},
                                {&editString3,dctLabel},
                                {&editString4,dctLabel},
                                {&editString5,dctLabel},
                                {&editString6,dctLabel},
                                {&editString7,dctLabel},
                                {&editString8,dctLabel},
                                {&editString9,dctLabel},
                                {&editString0,dctLabel},

                                {&editStringq,dctLabel},
                                {&editStringw,dctLabel},
                                {&editStringe,dctLabel},
                                {&editStringr,dctLabel},
                                {&editStringt,dctLabel},
                                {&editStringy,dctLabel},
                                {&editStringu,dctLabel},
                                {&editStringi,dctLabel},
                                {&editStringo,dctLabel},
                                {&editStringp,dctLabel},

                                {&editStringa,dctLabel},
                                {&editStrings,dctLabel},
                                {&editStringd,dctLabel},
                                {&editStringf,dctLabel},
                                {&editStringg,dctLabel},
                                {&editStringh,dctLabel},
                                {&editStringj,dctLabel},
                                {&editStringk,dctLabel},
                                {&editStringl,dctLabel},

                                {&editStringz,dctLabel},
                                {&editStringx,dctLabel},
                                {&editStringc,dctLabel},
                                {&editStringv,dctLabel},
                                {&editStringb,dctLabel},
                                {&editStringn,dctLabel},
                                {&editStringm,dctLabel},

                                {&editStringSpace,dctLabel},
                                {&editStringCaps,dctImage},
                                {&p1Up,dctImage},
                                {&p1Down,dctImage}};


//array of all screen pages in system
visualPage pages[]={{page1,sizeof(page1)/sizeof(page1[0])},
                    {page2,sizeof(page2)/sizeof(page2[0])},
                    {editModelPage,sizeof(editModelPage)/sizeof(editModelPage[0])},
                    {editMixPage,sizeof(editMixPage)/sizeof(editMixPage[0])},
                    {editStringPage,sizeof(editStringPage)/sizeof(editStringPage[0])},
                    {tsTest,sizeof(tsTest)/sizeof(tsTest[0])},
                    {primaryTrimPage,sizeof(primaryTrimPage)/sizeof(primaryTrimPage[0])},

                    };

int currentPage=0;
int lastPage=-1;

int tsDebounceLast=0;
int tsDebounceCount=0;
int tsPressedTimer=0;
int tsClickX;
int tsClickY;

pcCom pccoms;

#if (JENNIC_CHIP_FAMILY == JN514x)
#define TICK_CLOCK_MHZ 16
#define PERIF_CLOCK_MHZ 16
#else
#define PERIF_CLOCK_MHZ 16
#define TICK_CLOCK_MHZ 16
#endif

uint32 dbgmsgtimestart;
uint32 dbgmsgtimeend;


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
	pcComsPrintf("Bus Error Exception");
	// wait for the UART write to complete

	while(u32BusyWait--){}

	vAHI_SwReset();
}
PUBLIC void vUnalignedAccessHandler (void)
{
	volatile uint32 u32BusyWait = 1600000;
	// log the exception
	pcComsPrintf("Unaligned Error Exception");
	// wait for the UART write to complete
	while(u32BusyWait--){}

	vAHI_SwReset();
}
PUBLIC void vIllegalInstructionHandler(void)
{
	volatile uint32 u32BusyWait = 1600000;
	// log the exception
	pcComsPrintf("Illegal Instruction Error Exception");
	// wait for the UART write to complete
	while(u32BusyWait--){}

	vAHI_SwReset();
}

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
    lostPackets=0;
    sentPackets=0;

    //get config from flash
//    loadSettings();
#if (JENNIC_CHIP_FAMILY == JN514x)
	vAHI_WatchdogStop();

#else
    vAppApiSetBoostMode(TRUE);
#endif



 //   setHopMode(hoppingContinuous);


 //   vInitPrintf((void*)vPutC);

//    vInitPrintf((void*)vFifoPutC);


    vInitSystem();

    initPcComs(&pccoms,CONPC,0,txHandleRoutedMessage);

    setRadioDataCallback(txHandleRoutedMessage,CONRX);

    vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);

    BUS_ERROR = (uint32) vBusErrorhandler;
    UNALIGNED_ACCESS = (uint32) vUnalignedAccessHandler;
    ILLEGAL_INSTRUCTION = (uint32) vIllegalInstructionHandler;


    pcComsPrintf("tx24 2.11 \r\n");

    initLcdEADog(E_AHI_SPIM_SLAVE_ENBLE_2,E_AHI_DIO4_INT,E_AHI_DIO5_INT,180,&lcd);
    currentPage=0;

    renderPage(pages[currentPage].controls,pages[currentPage].len,TRUE,lcdbuf,128);
    LcdBitBlt(lcdbuf,128,0,0,128,64,&lcd);

    pcComsPrintf("lcd init done \r\n");


    loadSettings();

    pcComsPrintf("settings loaded \r\n");


//try to auto detect input source

   if(initPS2Controller(&ps2)==TRUE)
   {
       inputMethod=PS2INPUT;
       // select model by holding down fire buttons at startup
       pcComsPrintf("ps2\r\n");

       readPS2Controller(&ps2);

       if(ps2.LFire1)activeModel=1;
       if(ps2.LFire2)activeModel=2;
       if(ps2.RFire1)activeModel=3;
       if(ps2.RFire2)activeModel=4;

        //wait for button release
        while(ps2.LFire1 || ps2.LFire2 || ps2.RFire1 || ps2.RFire2)
        {
            readPS2Controller(&ps2);
        }
   }
   else
   {
        //no ps2 so try to detect wii nunchuck on i2c bus
         if(initWiiController(&wii)==TRUE)
         {
            readWiiController(&wii);
            inputMethod=NUNCHUCKINPUT;
            pcComsPrintf("nunchuck\r\n");
         }
         else inputMethod=PPMINPUT;//ADINPUT;

         inputMethod=ADINPUT;
   }
   if(inputMethod==PPMINPUT)
   {
        initPpmInput(E_AHI_TIMER_0);

    }


    if(initTsc2003(&touchScreen,0x48,723,196)==TRUE)
    {
        pcComsPrintf("tsc %d %d %d \r\n",touchScreen.x,touchScreen.y,touchScreen.pressure);
    }
    else
    {
        pcComsPrintf("No Touchscreen %d \r\n",touchScreen.x);

    }
    /*
while(1==1)
{
    readTsc2003(&touchScreen);
    pcComsPrintf("tsc %d %d %d \r\n",touchScreen.x,touchScreen.y,touchScreen.pressure);
    cycleDelay(16*1000*500);
}

*/


    //use mac address of rx to seed random hopping sequence
    randomizeHopSequence(liveModel.rxMACh ^ liveModel.rxMACl );


    pcComsPrintf("Model %s\r\n",liveModel.name);
    pcComsPrintf("trim %d\r\n",liveModel.trim[1]);

    modelLabel.txt=liveModel.name;
    modelLabel.valid=FALSE;
    renderPage(pages[currentPage].controls,pages[currentPage].len,TRUE,lcdbuf,128);
    LcdBitBlt(lcdbuf,128,0,0,128,64,&lcd);


    //use dio 16 for test sync pulse
    vAHI_DioSetDirection(0,E_AHI_DIO16_INT);

    //turn backlight on
    vAHI_DioSetPullup(0,E_AHI_DIO17_INT);
    vAHI_DioSetDirection(0,E_AHI_DIO17_INT);
    vAHI_DioSetOutput(E_AHI_DIO17_INT,0);

    //turn agnd on

    vAHI_DioSetPullup(0,E_AHI_DIO18_INT);
    vAHI_DioSetDirection(0,E_AHI_DIO18_INT);
    vAHI_DioSetOutput(E_AHI_DIO18_INT,0);


     // initalise Analog peripherals
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
	                 E_AHI_AP_INT_DISABLE,
	                 E_AHI_AP_SAMPLE_2,
	                 E_AHI_AP_CLOCKDIV_500KHZ,
	                 E_AHI_AP_INTREF);

    while(bAHI_APRegulatorEnabled() == 0);


    //setup comp1 as slow adc
    vAHI_ComparatorEnable(E_AHI_AP_COMPARATOR_1,
                            E_AHI_COMP_HYSTERESIS_0MV,
                            E_AHI_COMP_SEL_DAC);

    vAHI_DacEnable (E_AHI_AP_DAC_1,
                    E_AHI_AP_INPUT_RANGE_2,
                    E_AHI_DAC_RETAIN_ENABLE,
                    batDac);


    joystickOffset[0]=-116;
    joystickGain[0]=-1766;
    joystickOffset[1]=-173;
    joystickGain[1]=-1802;
    joystickOffset[2]=-116;
    joystickGain[2]=1766;
    joystickOffset[3]=-173;
    joystickGain[3]=1802;

     //setup tick timer to fire every hop period

    vAHI_TickTimerWrite(0);
    vAHI_TickTimerInterval(TICK_CLOCK_MHZ*hopPeriod);
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


    pcComsPrintf("init done \r\n");
    sCoordinatorData.eState = E_STATE_COORDINATOR_STARTED;

    uint8 msg[2];
    msg[0]=0;
    msg[1]=0;//0xff;



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
    //check for wake up actions - do as fast as possible to save power
    touchScreen.addr=0x48;

    if(pressedTsc2003(&touchScreen))
    {
          //simplest way to ensure everything is in a valid state
        vAHI_SwReset();
        //   AppColdStart();
    }
    else
    {
        //go back to sleep again;
        sleepTsc2003(&touchScreen);

        vAHI_SiConfigure(
            FALSE,
            FALSE,
            16);

         vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0,TRUE);
         vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_0,32*1000*5);//5 second wakeup
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
	(void)u32AHI_Init();

	(void)u32AppQApiInit(NULL, NULL, NULL);

	//move to after u32AHI_Init() to work on jn5148
#ifdef HIPOWERMODULE
	   //max power for europe including antenna gain is 10dBm
       //??? boost is +2.5 ant is 2 and power set to 4 = 8.5 ????
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    bAHI_PhyRadioSetPower(2);
#endif


    /* Initialise coordinator state */
    sCoordinatorData.eState = E_STATE_IDLE;
    sCoordinatorData.u8TxPacketSeqNb  = 0;
    sCoordinatorData.u8RxPacketSeqNb  = 0;
    sCoordinatorData.u16NbrEndDevices = 0;

    sCoordinatorData.u8ChannelSeqNo   = 0;

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);



    /* Set Pan ID and short address in PIB (also sets match registers in hardware) */

    sCoordinatorData.u16PanId = (uint16)((rand())>>16);


    MAC_vPibSetPanId(s_pvMac,sCoordinatorData.u16PanId);
    MAC_vPibSetShortAddr(s_pvMac, COORDINATOR_ADR);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    /* Allow nodes to associate */
    s_psMacPib->bAssociationPermit = 1;



    // disable normal hold off and retry stuff
    // do it in app code so that we can include send time in packet

#if (JENNIC_CHIP_FAMILY == JN514x)
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
        if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ENERGY_DETECT)
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
	if(psMcpsInd->uParam.sDcfmData.u8Handle==sCoordinatorData.u8TxPacketSeqNb)
	{
		dbgmsgtimeend=u32AHI_TickTimerRead()-dbgmsgtimestart;

		if (psMcpsInd->uParam.sDcfmData.u8Status == MAC_ENUM_SUCCESS)
		{
			/* Data frame transmission successful */

			//try and time round trip time
	  //      uint roundtrip=u32AHI_TickTimerRead()-sendTime;
	  //      if(roundtrip<lowestRange)lowestRange=roundtrip;


			PacketDone=1;
			ackedPackets++;
			channelAckCounter[sCoordinatorData.u8Channel-11]--;

			ackLowPriorityData();
	//vPrintf("%d ",RetryNo);
		}
		else
		{


			/* Data transmission failed */
			//delay random backoff period and retry
			if(++RetryNo<MaxRetries)
			{
				HoldOffAndSend();
			}
			else
			{
				lostPackets++;
			}
		}
		if(PacketDone==1)
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
		sCoordinatorData.u8RxPacketSeqNb=psFrame->au8Sdu[0];
		vProcessReceivedDataPacket(&psFrame->au8Sdu[1], psFrame->u8SduLength-1);

		rxLinkQuality = (int)psFrame->u8LinkQuality;

		//todo prevent flight timer being reset on catch or close flyby
		//todo maybe use JN5148 range feature for this
		if(rxLinkQuality>180)flightTimer=0;
	}


}
void txSendRoutedMessage(uint8* msg,uint8 len,uint8 toCon)
{
   switch(toCon)
    {
        case CONRX:
        {
             rxComsSendRoutedPacket(msg, 0, len);
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


void txHandleRoutedMessage(uint8* msg,uint8 len,uint8 fromCon)
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
				*replyBody++='T';
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
            case 16: //bind request -
            {
                //if in bind mode
                if(getHopMode()==hoppingFixed)
                {
                    //todo untested binding
                	//set liveModel mac

                    *replyBody++=17;//bind response id
                    //send tx mac
                    MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();

                    memcpy(replyBody,&macptr->u32L,4);
                    memcpy(replyBody+4,&macptr->u32H,4);

                    replyLen+=9;

                    memcpy(&liveModel.rxMACl,msgBody,4);
                    memcpy(&liveModel.rxMACh,msgBody+4,4);

                    pcComsPrintf("Bound \r\n");
                }

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
			case 0xa0:  //virtual click on lcd display
			    displayClick(msgBody[1], msgBody[2]);
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
		   txSendRoutedMessage(replyBuf,replyLen+addrLen, fromCon);
	   }
    }
    else
    {

    	//relay message
      	uint8 toCon;
      	//swap last 'to' to 'from' in situ
      	toCon=rmBuildRelayRoute(msg,fromCon);
      	//pass message on to connector defined by 'to' address
      	txSendRoutedMessage( msg, len, toCon);
    }
}
void rxComsSendRoutedPacket(uint8* msg, int offset, uint8 len)
{
    queueLowPriorityData(msg+offset,len);
}

/****************************************************************************
 *
 * NAME: vProcessReceivedDataPacket
 *
 ****************************************************************************/
PRIVATE void vProcessReceivedDataPacket(uint8 *pu8Data, uint8 u8Len)
{
    //back channel from rx - getting very messy!

	uint8 PriorityDataLen=pu8Data[0];
    totalRxFrames++;

    if(pu8Data[1]==255)
    {
        //rx debug message
    	pu8Data[u8Len-1]='\0';
        pcComsPrintf("db-%s\r\n",&pu8Data[2]);
    }
    else
    {
        if(PriorityDataLen==3)//16bit data
        {
            if(pu8Data[1]==6)
            {
                txLinkQuality=pu8Data[3];
                rxData[6]=pu8Data[2];
            }
            else
            {
                 rxData[pu8Data[1]]=pu8Data[2]+(pu8Data[3]<<8);
            }

   //         vPrintf("rx %d %d  \r",pu8Data[0],rxData[pu8Data[0]]);
        }
        else if(PriorityDataLen==5)//32 bit data
        {

            int f;
            memcpy(&f,&pu8Data[2],4);
            rxData[pu8Data[1]]=f;

#if (JENNIC_CHIP_FAMILY == JN514x)
            //todo sort out maths lib on 5148
#else
            if(pu8Data[0]==rxheightidx && initialHeight==-9999)initialHeight=f;
            if(pu8Data[0]==rxlatidx && initialLat==-9999)
            {
                initialLat=((double)f)/100000;
            }
            if(pu8Data[0]==rxlongidx && initialLong==-9999)
            {
                initialLong=((double)f)/100000;

                longitudeScale=cos(initialLong*3.1415927/180)*60;
            }
            if(pu8Data[0]==rxlongidx && initialLong!=-9999)
            {
                double dlat=((double)rxData[rxlatidx])/100000-initialLat;
                double dlong=((double)rxData[rxlongidx])/100000-initialLong;

                double drange=sqrt(dlat*dlat*60 + dlong*dlong*longitudeScale);
                drange*=1852;//convert to meters
                range=(int)drange;
            //      flat earth stuff
        //      nn = cos(long)*60 calc once
        //      sqrt(((lat1-lat2)*60)^2 + (long1-long2)*nn)^2)

            }
#endif
        }
    }
    if(u8Len-PriorityDataLen>1)
    {
    	dbc1++;
    	pcComsPrintf("lp%d",u8Len-PriorityDataLen-1);
    	handleLowPriorityData(pu8Data+PriorityDataLen+1,u8Len-PriorityDataLen-1);
    }

}

PRIVATE void GetNextChannel()
{

    uint32 newchannel=getNextInHopSequence(&(sCoordinatorData.u8ChannelSeqNo));

    sCoordinatorData.u8Channel=newchannel;

    (void)eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, newchannel);

    //toggle dio16 so sync can be checked with scope
    if((sCoordinatorData.u8ChannelSeqNo&1)==1)   vAHI_DioSetOutput(1<<16,0);
    else vAHI_DioSetOutput(0,1<<16);
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


    if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TICK_TIMER)
    {
       if(sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
       {

            sCoordinatorData.u8TxPacketSeqNb++;
            sCoordinatorData.u8TxPacketSeqNb&=0x7f;

            GetNextChannel();
            channelAckCounter[sCoordinatorData.u8Channel-11]++;

            flightTimer+=2;//simple way to scale to 1/100 sec

            if(readTsc2003(&touchScreen)==TRUE)
            {
   // pcComsPrintf("tsc %d %d %d \r\n",touchScreen.x,touchScreen.y,touchScreen.pressure);


                if(touchScreen.pressure>0 )
                {
                    tsPressedTimer++;

                    //ignore first reading of a click
                    if(tsPressedTimer==2 )
                    {
                        //allow 2 clicks per sec and auto repeat at that rate

                        tsDebounceLast=tsDebounceCount;
                        displayClick(touchScreen.x,touchScreen.y);
                           //pcComsPrintf("C %d %d ",touchScreen.x,tsY=touchScreen.y);
                        tsClickX=touchScreen.x;
                        tsClickY=touchScreen.y;

                    }
                    //test for drag event
                    if(tsPressedTimer>2 && tsPressedTimer%5==0 )
                    {
                        int dx=touchScreen.x-tsClickX;
                        int dy=touchScreen.y-tsClickY;
                        if(abs(dx)>1 || abs(dy)>1 )
                        {
                            displayDrag(dx>>1,dy>>1);
                        }
                        tsClickX=touchScreen.x;
                        tsClickY=touchScreen.y;

                    }
                }
                else
                {
                    tsPressedTimer=0;
                    //clear focus
                    focusedControl=NULL;
                }
                tsX=touchScreen.x;
                tsY=touchScreen.y;
                tsP=touchScreen.pressure;

            }

            tsDebounceCount++;

            checkBattery();
            renderPage(pages[currentPage].controls,pages[currentPage].len,lastPage!=currentPage,lcdbuf,128);
            LcdBitBlt(lcdbuf,128,0,0,128,64,&lcd);
            lastPage=currentPage;

            if(backlightTimer>0)backlightTimer--;
            else setBacklight(0);



            updatePcDisplay();


            if(subFrameIdx==0 || PacketDone==0)
            {
                RetryNo=0;

                if(nRangeMeasurements>500)
                {
                //  radioRange=totalTime/nRangeMeasurements;
                  totalTime=0;
                  nRangeMeasurements=0;
                  if(lowestRange<27514)radioRange=0;
                  else radioRange=(lowestRange-27514)*29979/3200;
                    lowestRange=1000000;

                }



                HoldOffAndSend();

                sentPackets++;
                if(sentPackets>=100)
                {
                    errorRate=lostPackets;

                    sentPackets=0;
                    lostPackets=0;
                    int i;
                    for(i=0;i<16;i++)
                    {
                        //channelAcks[i]=channelAckCounter[i];
                        channelAcks[i]=channelRetryCounter[i];
                        channelAckCounter[i]=0;
                        channelRetryCounter[i]=0;
                    }



                    txacks=ackedPackets;


                    updateDisplay();
                    ackedPackets=0;
                    retryPackets=0;

                    pcComsPrintf("%d " , dbgmsgtimeend);

                }
            }
            subFrameIdx++;
            if(subFrameIdx>=framePeriod/hopPeriod)subFrameIdx=0;

       }

    }
    if(psAHI_Ind->u32DeviceId==E_AHI_DEVICE_TIMER1)
    {
       //called as a result of a random holdoff timer
        //try to send a packet

        if(sCoordinatorData.eState == E_STATE_COORDINATOR_STARTED)
        {
            retryPackets++;
            vCreateAndSendFrame();

//            dbgmsgtimestart=u32AHI_TickTimerRead();

            if(RetryNo!=0)channelRetryCounter[sCoordinatorData.u8Channel-11]++;

        }
    }
}

PRIVATE void HoldOffAndSend()
{

    uint32 backoff= (rand()%((1<<(RetryNo+2))-1))*320+128;

    //check we have enough time before channel change
    if((backoff+(u32AHI_TickTimerRead()/16))< hopPeriod-2500 )
    {


    	vAHI_TimerStartSingleShot(E_AHI_TIMER_1,0,backoff);
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
    MAC_McpsReqRsp_s  sMcpsReqRsp;
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
    if(TX_ADDRESS_MODE==3)
    {
        MAC_ExtAddr_s* macptr = pvAppApiGetMacAddrLocation();
        sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L = macptr->u32L;
        sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H = macptr->u32H;
    }

  /* Use long address and broadcast pan id for destination */
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = RX_ADDRESS_MODE;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = 0xffff;//sCoordinatorData.u16PanId;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L = liveModel.rxMACl;
    sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H = liveModel.rxMACh;


    /* Frame requires ack but not security, indirect transmit or GTS */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK ;

    pu8Payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;


    pu8Payload[0] = sCoordinatorData.u8TxPacketSeqNb;


    for (i = 1; i < (u8Len + 1); i++)
    {
        pu8Payload[i] = *pu8Data++;
    }

    i+=appendLowPriorityData(&pu8Payload[i],32);

    /* Set frame length */
    sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = i;



    /* Request transmit */
    vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
      sendTime=u32AHI_TickTimerRead();
      dbgmsgtimestart=u32AHI_TickTimerRead();

}


PRIVATE uint16 u16ReadADC(uint8 channel)
{

    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                        E_AHI_AP_INPUT_RANGE_2,
                        channel);
    uint16 ret=0;
    int i;
    for(i=0;i<4;i++)
    {
        vAHI_AdcStartSample();              /* start an AtoD sample here */
        while(bAHI_AdcPoll() != 0x00);      /* wait for conversion complete */
       ret+= u16AHI_AdcRead();
    }
    return (ret>>2);          /* return result */
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
    if(currentAuxChannel>19)currentAuxChannel=4;


    uint8 au8Packet[PAYLOAD_SIZE];
    modelEx* mod=&liveModel;

    switch( inputMethod)
    {
        case PS2INPUT:
        {
            readPS2Controller(&ps2);

         //set trims
            if(ps2.RRight && mod->trim[0]<2048) mod->trim[0]+=8;
            if(ps2.RLeft && mod->trim[0]>-2048) mod->trim[0]-=8;

            if(ps2.RUp && mod->trim[1]<2048) mod->trim[1]+=8;
            if(ps2.RDown && mod->trim[1]>-2048) mod->trim[1]-=8;

            if(ps2.LRight && mod->trim[2]<2048) mod->trim[2]+=8;
            if(ps2.LLeft && mod->trim[2]>-2048) mod->trim[2]-=8;

            if(ps2.LUp && mod->trim[3]<2048) mod->trim[3]+=8;
            if(ps2.LDown && mod->trim[3]>-2048) mod->trim[3]-=8;

        //set joystick values
             txInputs[0]=(ps2.RJoyX<<4)-2048;
             txInputs[1]=(-ps2.RJoyY<<4)+2048;
             txInputs[2]=(ps2.LJoyY<<4)-2048;
             txInputs[3]=(-ps2.LJoyX<<4)+2048;

        //set aux channels
            if(ps2.LFire1 )
            {
                 activeAuxChannel++;
                 if(activeAuxChannel>19)activeAuxChannel=4;
            }
            if(ps2.LFire2 )
            {
                activeAuxChannel--;
                if(activeAuxChannel<4)activeAuxChannel=19;
            }

            if(ps2.RFire1 )
            {
                 txInputs[activeAuxChannel]++;
                 //move changed channel to top of queue
                 currentAuxChannel=activeAuxChannel;

          //       vPrintf("A%d %d ",activeAuxChannel,txInputs[activeAuxChannel]);
            }
            if(ps2.RFire2 )
            {
                txInputs[activeAuxChannel]--;
                //move changed channel to top of queue
                currentAuxChannel=activeAuxChannel;

          //      vPrintf("A%d %d ",activeAuxChannel,txInputs[activeAuxChannel]);
            }
            // high and low rates on joystick buttons
            if(ps2.RJoy)
            {
                mod->rateMode=0;
            }
            if(ps2.LJoy)
            {
                mod->rateMode=1;
            }
            break;
        }
        case ADINPUT:
        {
            //3755=3.3v
            int ref=u16ReadADC(E_AHI_ADC_SRC_VOLT);

            txInputs[0]=((u16ReadADC(0)-2048)*3755/ref-joystickOffset[0])*2048/joystickGain[0];
            txInputs[1]=((u16ReadADC(1)-2048)*3755/ref-joystickOffset[1])*2048/joystickGain[1];
            txInputs[2]=((u16ReadADC(2)-2048)*3755/ref-joystickOffset[2])*2048/joystickGain[2];
            txInputs[3]=((u16ReadADC(3)-2048)*3755/ref-joystickOffset[3])*2048/joystickGain[3];
            break;
        }
        case NUNCHUCKINPUT:
        {
           readWiiController(&wii);
            if(wii.ZButton==TRUE)
            {
                //use accelerometers
                txInputs[0]=(wii.AccelX<<3)-4096;
                txInputs[1]=(wii.AccelY<<3)-4096;

            }
            else
            {
                //use joystick
                txInputs[0]=(wii.JoyX<<4)-2048;
                txInputs[1]=(wii.JoyY<<4)-2048;

            }


            break;
        }
        case PPMINPUT:
        {
            ppmRead(txInputs);

        }
    }


    //do mixing

    doMixingEx(txInputs,txDemands,&liveModel);


//    vPrintf("%d %d  \r",txDemands[0],txDemands[1]);


    //pack into packet
    au8Packet[2]=txDemands[0]&0x00ff;
    au8Packet[3]=((txDemands[0]&0x0f00)>>4)+(txDemands[1]&0x000f);
    au8Packet[4]=(txDemands[1]&0x0ff0)>>4;

    au8Packet[5]=txDemands[2]&0x00ff;
    au8Packet[6]=((txDemands[2]&0x0f00)>>4)+(txDemands[3]&0x000f);
    au8Packet[7]=(txDemands[3]&0x0ff0)>>4;

    au8Packet[8]= ((currentAuxChannel-4)<<4)+( txDemands[currentAuxChannel]&0x000f);
    au8Packet[9]=(txDemands[currentAuxChannel]&0x0ff0)>>4;

    //set packet time in 0.01ms units - just fits in 16 bits
    int t=(u32AHI_TickTimerRead()+sCoordinatorData.u8ChannelSeqNo*hopPeriod*16)/160;
    au8Packet[0]=t & 0x00FF;
    au8Packet[1]=t >> 8;

    PacketDone=0;

    vTransmitDataPacket(au8Packet,PAYLOAD_SIZE,1);


}


void toggleBackLight(clickEventArgs* clickargs)
{
    //one day this could be dimable
    if(backlight==255)
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
    backlight=bri;
    if(bri>0)
    {
        vAHI_DioSetOutput(E_AHI_DIO17_INT,0);
    }
    else
    {
        vAHI_DioSetOutput(0,E_AHI_DIO17_INT);
    }
}

void setModelUp(clickEventArgs* clickargs)
{
    //brute force load each model as we move through list

    store s;

    //storeSettings();
    if(getOldStore(&s))
    {
        if(liveModelIdx<numModels-1)liveModelIdx++;
        else liveModelIdx=0;

        p2modelLabel.valid=FALSE;

        loadModelByIdx(&s,&liveModel,liveModelIdx);

        enableModelComs();

        editModModLabel.valid=FALSE;
        modelLabel.valid=FALSE;
        p2modelLabel.valid=FALSE;
    }
  /*
    if(activeModel<15)activeModel++;
    else activeModel=0;
    p2modelLabel.txt=models[activeModel].name;
    p2modelLabel.valid=FALSE;
*/
}
void setModelDown(clickEventArgs* clickargs)
{
   /*
    if(activeModel>0)activeModel--;
    else activeModel=15;
    p2modelLabel.txt=models[activeModel].name;
    p2modelLabel.valid=FALSE;
*/

}
void bindModelClick(clickEventArgs* clickargs)
{
   //goto fixed channel for binding
   setHopMode(hoppingFixed);
   eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, getHopChannel(0));
   //wait for bind request

}



void trim4lClick(clickEventArgs* clickargs)
{
    liveModel.trim[3]-=32;
}
void trim4rClick(clickEventArgs* clickargs)
{
    liveModel.trim[3]+=32;
}
void trim1lClick(clickEventArgs* clickargs)
{
    liveModel.trim[0]-=32;
}
void trim1rClick(clickEventArgs* clickargs)
{
    liveModel.trim[0]+=32;
}
void trim3uClick(clickEventArgs* clickargs)
{
    liveModel.trim[2]+=32;
}
void trim3dClick(clickEventArgs* clickargs)
{
    liveModel.trim[2]-=32;
}
void trim2uClick(clickEventArgs* clickargs)
{
    liveModel.trim[1]+=32;
}
void trim2dClick(clickEventArgs* clickargs)
{
    liveModel.trim[1]-=32;
}
void rateClick(clickEventArgs* clickargs)
{
    if(liveModel.rateMode==0)
    {
        liveModel.rateMode=1;
        //ought to automate this, maybe bind image list to enums or int
        trimrate.image=hiicon;
        trimrate.valid=FALSE;
    }
    else
    {
        liveModel.rateMode=0;
        trimrate.image=lowicon;
        trimrate.valid=FALSE;
    }
}
void homeClick(clickEventArgs* clickargs)
{
    currentPage=0;
}

void bindMixEditor()
{
     editMixInVal.num=&liveModel.mixes[currentMixEdit].inChannel;
     editMixOutVal.num=&liveModel.mixes[currentMixEdit].outChannel;
     editMixLoVal.num=&liveModel.mixes[currentMixEdit].rateLow;
     editMixHiVal.num=&liveModel.mixes[currentMixEdit].rateHigh;

}
void editMixMixUpClick(clickEventArgs* clickargs)
{
    currentMixEdit++;
    if(currentMixEdit>MAXMIXES)currentMixEdit=0;
    bindMixEditor();
}
void editMixMixDownClick(clickEventArgs* clickargs)
{
    currentMixEdit--;
    if(currentMixEdit<0)currentMixEdit=MAXMIXES-1;
    bindMixEditor();

}
void editMixInUpClick(clickEventArgs* clickargs)
{
   uint8 val=liveModel.mixes[currentMixEdit].inChannel;
   val++;
   if(val>=MAXCHANNELS)val=NOMIX;
   liveModel.mixes[currentMixEdit].inChannel=val;
}
void editMixInDownClick(clickEventArgs* clickargs)
{
   uint8 val=liveModel.mixes[currentMixEdit].inChannel;
   if(val==NOMIX)val=MAXCHANNELS-1;
   else val--;
   liveModel.mixes[currentMixEdit].inChannel=val;
}

void copyModelClick(clickEventArgs* clickargs)
{
   //save any changes to current model
   storeSettings();

   //simply leave current stuff in liveModel structure
   liveModelIdx=numModels;
   numModels++;
   strcpy(liveModel.name,"New Model");


   //storeSettings();
   //must improve binding of strings
   editModModLabel.valid=FALSE;
   modelLabel.valid=FALSE;
   p2modelLabel.valid=FALSE;
}

void pageUp(clickEventArgs* clickargs)
{
    if(currentPage>=sizeof(pages)/sizeof(visualPage)-1)currentPage=0;
    else currentPage++;
}
void pageDown(clickEventArgs* clickargs)
{
    if(currentPage==0)currentPage=sizeof(pages)/sizeof(visualPage)-1;
    else currentPage--;
}
void virtualKeyCapsClick(clickEventArgs* clickargs)
{
     int i;
     for(i=0;i<26;i++)
     {
         labelControl* lc=(labelControl*)editStringPage[i+12].control;
         char c=lc->txt[0];
         if(c>'Z')c-='a'-'A';
         else c+='a'-'A';
         lc->txt[0]=c;
         lc->valid=FALSE;
     }
}
void virtualKeyDelClick(clickEventArgs* clickargs)
{
     int len;
     len=strlen(editStringVal.txt);
     if(len>0)
     {
          editStringVal.txt[len-1]='\0';

          editStringVal.valid=FALSE;
          editModModLabel.valid=FALSE;
          modelLabel.valid=FALSE;
          p2modelLabel.valid=FALSE;
     }
}
void virtualKeyClick(clickEventArgs* clickargs)
{
    visualControl* vc=clickargs->sender;
    int len;
    if(vc->type==dctLabel )
    {
        //copy text from button
        labelControl* lc=(labelControl*)vc->control;
        len=strlen(editStringVal.txt);
        if(len<30)
        {
            editStringVal.txt[len+1]='\0';
            editStringVal.txt[len]=lc->txt[0];
        }
    }
    editStringVal.valid=FALSE;
    editModModLabel.valid=FALSE;
    modelLabel.valid=FALSE;
    p2modelLabel.valid=FALSE;
}


void displayClick(uint8 x, uint8 y)
{
    //turn on backlight for a while
    backlightTimer=50*30;
    setBacklight(255);
    clickEventArgs clickargs;
    int c;
    c=findControl( x, y,&pages[currentPage]);
    if(c!=-1)
    {
        visualControl* vc=&pages[currentPage].controls[c];
        clickargs.x=x;
        clickargs.y=y;
        clickargs.pressure=255;//not properly done yet
        clickargs.sender=vc;

        if(vc->type==dctLabel )
        {
            labelControl* lc=(labelControl*)vc->control;
            if(lc->clickHandler!=NULL)
            {
               // (lc->clickHandler)(x,y,255,(void*)vc);
               (lc->clickHandler)(&clickargs);
            }
        }
        if(vc->type==dctImage )
        {
            imageControl* lc=(imageControl*)vc->control;
            if(lc->clickHandler!=NULL)
            {
               // (lc->clickHandler)(x,y,255,(void*)vc);
               (lc->clickHandler)(&clickargs);

            }
        }
        focusedControl=vc;
    }

}
void displayDrag(int dx, int dy)
{

    if(focusedControl!=NULL)
    {
       //bind to displayed number
        if(focusedControl->type==dctNumber)
        {

            numberControl* nc=(numberControl*)focusedControl->control;
            if(nc->format[0]=='b')//uint8 binding
            {
                  *((uint8*)(nc->num))+= (uint8)dx;
            }
            if(nc->format[0]=='h')//uint8 binding
            {
                  *((int16*)(nc->num))+= (int16)dx;
            }
        }
    }
}
void updateDisplay()
{

 //   txbat=u16ReadADC(E_AHI_ADC_SRC_VOLT);//3838 4096 = 2.4*1.5 =3.6v
 //   txbat=txbat*360/4096;

    rxbat=rxData[5];
    rxbat=rxbat*360/4096;

    txretries=retryPackets-100;
    rxpackets=rxData[6];

    if(totalRxFrames>4*50)
    {
        if(rxpackets<lowestRxFrameRate)lowestRxFrameRate=rxpackets;
    }


    rxData[6]=0;

 //   renderPage(pages[currentPage].controls,pages[currentPage].len,lastPage!=currentPage,lcdbuf,128);
 //   LcdBitBlt(lcdbuf,128,0,0,128,64,&lcd);
 //   lastPage=currentPage;


}
void updatePcDisplay()
{
    //crudely send part of the bitmap on each frame

    static uint8 blockidx=0;

    pcComsSendPacket2(lcdbuf, blockidx*16, 16, 0x92,blockidx);

    blockidx++;
    if(blockidx>=64)blockidx=0;
}

void checkBattery()
{
    // soft successive approximation 11 bit adc, just like the bad old days
    static uint16 step=512;
    static uint16 lastDacTry=1023;

    if(step>0)
    {
        if((u8AHI_ComparatorStatus() & E_AHI_AP_COMPARATOR_MASK_1)!=0)
        {
            //dac is below bat voltage
            lastDacTry+=step;
        }
        else
        {
            lastDacTry-=step;
        }
        step = step>>1;
    }
    else
    {
        txbat=240*((int)lastDacTry)*6 *495/518/2048;
        step=512;
        lastDacTry=1023;
    }
    vAHI_DacOutput (E_AHI_AP_DAC_1,lastDacTry);
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
void sleepClick(clickEventArgs* clickargs)
{
     sleep();
}

void storeSettings()
{
	 //todo untested on JN5148


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

 //   pcComsPrintf("s %d %d %d",s.base,old.base,old.size);

    storeModels(&s,&liveModel,pold,liveModelIdx);

    commitStore(&s);
}
void loadSettings()
{
	 //todo untested on JN5148
 //   loadDefaultSettings();
 //   return;
    pcComsPrintf("load Settings \r\n");


    bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

    store s;
    store section;
    int tag;
    if(getOldStore(&s)==TRUE)
    {
        pcComsPrintf("store found %d \r\n",s.size);

        while((tag=storeGetSection(&s,&section))>0)
        {
         //   pcComsPrintf("tag found %d %d\r\n",tag,section.size);

            switch(tag)
            {
               case STOREMODELSSECTION:
               {
                   pcComsPrintf("model section found %d\r\n",section.size);

                   readModels(&section,&liveModel,&liveModelIdx,&numModels);
                   pcComsPrintf("models found %d %d \r\n",numModels,liveModelIdx);

                   break;
               }
               case STORERADIOSECTION:
               {

                   break;
               }
               case STOREGENERALSECTION:
               {

                   break;
               }
            }
        }
    }
    else
    {
        loadDefaultSettings();
    }
    if(numModels==0)loadDefaultSettings();

}
void loadDefaultSettings()
{
    setupAlula(&liveModel);
    numModels=1;
    liveModelIdx=0;
}

void sleep()
{
    //todo untested on JN5148

	vAHI_TickTimerIntEnable(FALSE);
     vAHI_TimerDisable(E_AHI_TIMER_0);
     vAHI_TimerDisable(E_AHI_TIMER_1);

     storeSettings();

     //turn off power consuming bits
     //lights
     setBacklight(0);
     //agnd
     vAHI_DioSetOutput(0,E_AHI_DIO18_INT);
     //touchscreen
     sleepTsc2003(&touchScreen);
     //display
     sleepLcdEADog(&lcd);
    //
 //   vAHI_HighPowerModuleEnable(FALSE,FALSE);

    vAHI_SiConfigure(
    FALSE,
    FALSE,
    16);

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

#if (JENNIC_CHIP_FAMILY == JN514x)

#else
   vAppApiSetBoostMode(FALSE);
#endif
    //if you don't do this the high power amp does not sleep
    MAC_vPibSetRxOnWhenIdle(s_pvMac, FALSE, FALSE);

vAHI_ComparatorDisable(E_AHI_AP_COMPARATOR_1);
vAHI_ComparatorDisable(E_AHI_AP_COMPARATOR_2);


     vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0,TRUE);
     vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_0,32*1000*5);//5 second wakeup
     vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);

     //tx 1.0 hw currently uses 70ua whilst sleeping
     //about 45uA unaccounted for!
     //at 5v bat monitor uses 16uA - could use higher value resitors
     //ram retention 2.4uA
     //sleep  with timer 1.2uA
     //tsc2003 should be 3uA
     //eadog lcd should be 150uA on 0.1uA sleep
     //
}
void enableModelComs()
{

    randomizeHopSequence(liveModel.rxMACh ^ liveModel.rxMACl );
    setHopMode(hoppingContinuous);

}
void setTxMode(teState state)
{
     sCoordinatorData.eState = state;

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
//adc averaging?
 // WRITE_REG32(ADDR_AP_CTRL, READ_REG32(ADDR_AP_CTRL) | 0x00000800);

