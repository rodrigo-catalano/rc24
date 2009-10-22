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

// Graphic user interface for the Mk1 Walnut transmitter.

// TODO weed out unused includes

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
#include "hwutils.h"
#include "gui_walnut.h"


// TODO get rid of externs one way or another. Very quick fix to get gui out of tx main
extern int txbat;
extern int rxbat;
extern int txretries;
extern int rxpackets;
extern int txacks;
extern modelEx liveModel;

extern int tsDebounceLast;
extern int tsDebounceCount;
extern int tsPressedTimer;
extern int tsClickX;
extern int tsClickY;

extern int rxLinkQuality;
extern int txLinkQuality;
extern int channelAcks[16];
extern int channelAckCounter[16];
extern int channelRetryCounter[16];

extern int rxData[256];
extern uint32 radioRange;

extern int totalLostFrames;
extern int lowestRxFrameRate;

extern int totalRxFrames;
extern int flightTimer;

extern int txInputs[20];

extern tsc2003 touchScreen;

extern int backlightTimer;

void pollTouchScreen(void);

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

void sleepClick(clickEventArgs* clickargs);

//build display controls and pages

// images imported using rc24 config pc program
uint8 imgUpButton[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 130, 194, 226, 242, 226, 194, 130, 2, 2, 2, 2,
		2, 2, 2, 252, 0, 255, 0, 0, 16, 24, 28, 30, 31, 31, 255, 255, 255, 255,
		255, 31, 31, 30, 28, 24, 16, 0, 0, 255, 0, 127, 128, 128, 128, 128,
		128, 128, 128, 128, 159, 159, 159, 159, 159, 128, 128, 128, 128, 128,
		128, 128, 128, 127 };

uint8 imgDownButton[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 2, 242, 242, 242, 242, 242, 2, 2, 2, 2, 2, 2, 2,
		2, 252, 0, 255, 0, 0, 16, 48, 112, 240, 240, 240, 255, 255, 255, 255,
		255, 240, 240, 240, 112, 48, 16, 0, 0, 255, 0, 127, 128, 128, 128, 128,
		128, 128, 129, 131, 135, 143, 159, 143, 135, 131, 129, 128, 128, 128,
		128, 128, 128, 127 };

uint8 imgLeftButton[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 130, 194, 226, 242, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 252, 0, 255, 0, 0, 16, 56, 124, 254, 255, 255, 255, 255, 255, 126,
		126, 126, 126, 126, 126, 126, 126, 0, 0, 255, 0, 127, 128, 128, 128,
		128, 128, 128, 129, 131, 135, 143, 159, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 127 };

uint8 imgRightButton[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 242, 226, 194, 130, 2, 2, 2, 2, 2, 2,
		2, 252, 0, 255, 0, 0, 124, 124, 124, 124, 124, 124, 124, 124, 255, 255,
		255, 255, 255, 254, 124, 56, 16, 0, 0, 255, 0, 127, 128, 128, 128, 128,
		128, 128, 128, 128, 128, 128, 159, 143, 135, 131, 129, 128, 128, 128,
		128, 128, 128, 127 };

uint8 hiicon[] =
{ 0, 252, 2, 242, 242, 2, 2, 242, 242, 2, 50, 242, 242, 50, 2, 252, 0, 127,
		128, 159, 159, 131, 131, 159, 159, 128, 152, 159, 159, 152, 128, 127 };

uint8 lowicon[] =
{ 0, 252, 2, 242, 242, 2, 2, 2, 2, 242, 242, 50, 242, 242, 2, 254, 0, 255, 128,
		159, 159, 152, 152, 152, 128, 159, 159, 152, 159, 159, 128, 255 };

uint8 homeicon[] =
{ 0, 252, 2, 2, 130, 130, 66, 66, 34, 34, 18, 18, 10, 18, 18, 34, 34, 122, 122,
		130, 130, 2, 2, 252, 0, 255, 0, 1, 255, 0, 0, 14, 10, 202, 78, 64, 64,
		64, 78, 202, 10, 14, 0, 0, 255, 1, 0, 255, 0, 127, 128, 128, 159, 144,
		144, 144, 144, 159, 144, 144, 144, 144, 144, 159, 144, 144, 144, 144,
		159, 128, 128, 127 };

uint8 homeicon16x16[] =
{ 0, 252, 2, 130, 194, 98, 50, 26, 14, 26, 50, 122, 194, 130, 2, 252, 0, 127,
		128, 128, 159, 144, 145, 156, 146, 156, 145, 144, 159, 128, 128, 127 };

uint8 upicon22x24[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 130, 194, 226, 242, 226, 194, 130, 2, 2, 2, 2, 2,
		2, 252, 0, 255, 0, 0, 8, 12, 14, 15, 15, 15, 255, 255, 255, 15, 15, 15,
		14, 12, 8, 0, 0, 255, 0, 127, 128, 128, 128, 128, 128, 128, 128, 128,
		143, 143, 143, 128, 128, 128, 128, 128, 128, 128, 128, 127 };

uint8 downicon22x24[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 2, 242, 242, 242, 2, 2, 2, 2, 2, 2, 2, 2, 252,
		0, 255, 0, 0, 16, 48, 112, 240, 240, 240, 255, 255, 255, 240, 240, 240,
		112, 48, 16, 0, 0, 255, 0, 127, 128, 128, 128, 128, 128, 128, 129, 131,
		135, 143, 135, 131, 129, 128, 128, 128, 128, 128, 128, 127 };

uint8 lefticon21x32[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 252, 0, 255, 0,
		0, 0, 128, 192, 224, 240, 248, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0,
		255, 0, 2, 7, 15, 31, 63, 127, 255, 255, 7, 7, 7, 7, 7, 7, 7, 0, 0,
		255, 0, 127, 128, 128, 128, 128, 128, 128, 128, 128, 129, 128, 128,
		128, 128, 128, 128, 128, 128, 128, 127 };

uint8 righticon21x32[] =
{ 0, 252, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 252, 0, 255, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 252, 248, 240, 224, 192, 128, 0, 0, 0, 255, 0,
		255, 0, 7, 7, 7, 7, 7, 7, 7, 7, 255, 255, 127, 63, 31, 15, 7, 2, 0,
		255, 0, 127, 128, 128, 128, 128, 128, 128, 128, 128, 128, 129, 128,
		128, 128, 128, 128, 128, 128, 128, 127 };

uint8 lefticon8x8[] =
{ 0, 8, 28, 62, 127, 28, 28, 28 };
uint8 righticon8x8[] =
{ 0, 28, 28, 28, 127, 62, 28, 8 };
uint8 upicon8x8[] =
{ 8, 12, 126, 127, 126, 12, 8, 0 };
uint8 downicon8x8[] =
{ 8, 24, 63, 127, 63, 24, 8, 0 };

visualControl* focusedControl = NULL;

labelControl headerLabel =
{ "tx24 2.09", 0, 0, 48, 8, 8, 0, TRUE, FALSE, NULL };
labelControl modelLabel =
{ &liveModel.name, 48, 0, 64, 8, 8, 0, TRUE, FALSE, NULL };

labelControl txbatLabel =
{ "tx bat", 0, 8, 30, 8, 8, 0, TRUE, FALSE, NULL };
numberControl txbatVal =
{ &txbat, 36, 8, 20, 8, 8, 0, TRUE, 0, 2, "i" };

labelControl rxbatLabel =
{ "rx bat", 0, 16, 30, 8, 8, 0, TRUE, FALSE, NULL };
numberControl rxbatVal =
{ &rxbat, 36, 16, 20, 8, 8, 0, TRUE, 0, 2, "i" };

numberControl acksVal =
{ &txacks, 60, 8, 30, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl retryVal =
{ &txretries, 80, 8, 25, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl rxerrorVal =
{ &rxpackets, 60, 16, 30, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl txLinkQ =
{ &txLinkQuality, 105, 8, 23, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl rxLinkQ =
{ &rxLinkQuality, 105, 16, 23, 8, 8, 0, TRUE, 0, 0, "i" };

labelControl rxspeedLabel =
{ "speed", 0, 32, 30, 8, 8, 0, TRUE, FALSE, NULL };
numberControl rxspeedVal =
{ &rxData[rxspeedidx], 36, 32, 24, 8, 8, 0, TRUE, 0, 3, "i" };
labelControl rxheightLabel =
{ "height", 64, 32, 30, 8, 8, 0, TRUE, FALSE, NULL };
numberControl rxheightVal =
{ &rxData[rxheightidx], 96, 32, 30, 8, 8, 0, TRUE, 0, 0, "i" };
labelControl rxrangeLabel =
{ "range", 0, 48, 30, 8, 8, 0, TRUE, FALSE, NULL };
numberControl rxrangeVal =
{ &radioRange, 36, 48, 64, 8, 8, 0, TRUE, 0, 0, "i" };

numberControl rxWorst =
{ &lowestRxFrameRate, 0, 56, 24, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl flightTimerVal =
{ &flightTimer, 64, 56, 64, 8, 8, 0, TRUE, 0, 0, "i" };

barControl bar1 =
{ &channelAcks[0], 0, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar2 =
{ &channelAcks[1], 5, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar3 =
{ &channelAcks[2], 10, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar4 =
{ &channelAcks[3], 15, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar5 =
{ &channelAcks[4], 20, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar6 =
{ &channelAcks[5], 25, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar7 =
{ &channelAcks[6], 30, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar8 =
{ &channelAcks[7], 35, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar9 =
{ &channelAcks[8], 40, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar10 =
{ &channelAcks[9], 45, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar11 =
{ &channelAcks[10], 50, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar12 =
{ &channelAcks[11], 55, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar13 =
{ &channelAcks[12], 60, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar14 =
{ &channelAcks[13], 65, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar15 =
{ &channelAcks[14], 70, 24, 4, 8, 8, 0, TRUE, 0, 0 };
barControl bar16 =
{ &channelAcks[15], 75, 24, 4, 8, 8, 0, TRUE, 0, 0 };

imageControl p1Down =
{ imgDownButton, 104, 40, 24, 24, TRUE, FALSE, pageDown };
imageControl p1Up =
{ imgUpButton, 104, 0, 24, 24, TRUE, FALSE, pageUp };

visualControl page1[] =
{
{ &headerLabel, dctLabel },
{ &modelLabel, dctLabel },
{ &txbatLabel, dctLabel },
{ &txbatVal, dctNumber },
{ &rxbatLabel, dctLabel },
{ &rxbatVal, dctNumber },
{ &acksVal, dctNumber },
{ &retryVal, dctNumber },
{ &rxerrorVal, dctNumber },
{ &rxLinkQ, dctNumber },
{ &txLinkQ, dctNumber },
{ &bar1, dctBar },
{ &bar2, dctBar },
{ &bar3, dctBar },
{ &bar4, dctBar },
{ &bar5, dctBar },
{ &bar6, dctBar },
{ &bar7, dctBar },
{ &bar8, dctBar },
{ &bar9, dctBar },
{ &bar10, dctBar },
{ &bar11, dctBar },
{ &bar12, dctBar },
{ &bar13, dctBar },
{ &bar14, dctBar },
{ &bar15, dctBar },
{ &bar16, dctBar },
{ &rxspeedLabel, dctLabel },
{ &rxspeedVal, dctNumber },
{ &rxheightLabel, dctLabel },
{ &rxheightVal, dctNumber },
{ &rxrangeLabel, dctLabel },
{ &rxrangeVal, dctNumber },
{ &rxWorst, dctNumber },
{ &flightTimerVal, dctNumber },
{ &p1Up, dctImage },
{ &p1Down, dctImage } };
//basic settings page

labelControl p2Label =
{ "Select Model", 0, 0, 64, 8, 8, 0, TRUE, FALSE, NULL };
labelControl p2modelUp =
{ "Up", 0, 16, 64, 8, 8, 0, TRUE, FALSE, setModelUp };
labelControl p2modelLabel =
{ liveModel.name, 0, 32, 64, 8, 8, 0, TRUE, FALSE, NULL };
labelControl p2modelDown =
{ "Down", 0, 48, 64, 8, 8, 0, TRUE, FALSE, setModelDown };
imageControl p2light =
{ homeicon, 64, 0, 24, 24, TRUE, FALSE, toggleBackLight };
imageControl p2Off =
{ homeicon, 64, 24, 24, 24, TRUE, FALSE, sleepClick };

visualControl page2[] =
{
{ &p2Label, dctLabel },
{ &p2modelUp, dctLabel },
{ &p2modelLabel, dctLabel },
{ &p2modelDown, dctLabel },
{ &p2light, dctImage },
{ &p2Off, dctImage },
{ &p1Up, dctImage },
{ &p1Down, dctImage }

};

//test and debug page

int tsX, tsY, tsP;
int dbc1 = 0;

numberControl tsx =
{ &tsX, 0, 32, 30, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl tsy =
{ &tsY, 48, 32, 30, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl tsp =
{ &tsP, 96, 32, 30, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl ncdbc1 =
{ &dbc1, 0, 8, 30, 8, 8, 0, TRUE, 0, 0, "i" };

numberControl ch1 =
{ &txInputs[0], 0, 0, 60, 8, 8, 0, TRUE, 0, 0, "i" };
numberControl ch2 =
{ &txInputs[1], 64, 0, 60, 8, 8, 0, TRUE, 0, 0, "i" };

visualControl tsTest[] =
{
{ &tsx, dctNumber },
{ &tsy, dctNumber },
{ &tsp, dctNumber },
{ &p1Up, dctImage },
{ &p1Down, dctImage },
{ &ch1, dctNumber },
{ &ch2, dctNumber },
{ &ncdbc1, dctNumber } };

//primary trim page

imageControl trim4l =
{ lefticon21x32, 0, 16, 21, 32, TRUE, FALSE, trim4lClick };
imageControl trim4r =
{ righticon21x32, 43, 16, 21, 32, TRUE, FALSE, trim4rClick };
imageControl trim1l =
{ lefticon21x32, 64, 16, 21, 32, TRUE, FALSE, trim1lClick };
imageControl trim1r =
{ righticon21x32, 107, 16, 21, 32, TRUE, FALSE, trim1rClick };

imageControl trim3u =
{ upicon22x24, 21, 0, 22, 24, TRUE, FALSE, trim3uClick };
imageControl trim3d =
{ downicon22x24, 21, 40, 22, 24, TRUE, FALSE, trim3dClick };
imageControl trim2u =
{ upicon22x24, 85, 0, 22, 24, TRUE, FALSE, trim2uClick };
imageControl trim2d =
{ downicon22x24, 85, 40, 22, 24, TRUE, FALSE, trim2dClick };

imageControl trimrate =
{ hiicon, 21, 24, 16, 16, TRUE, FALSE, rateClick };
imageControl trimhome =
{ homeicon16x16, 85, 24, 16, 16, TRUE, FALSE, homeClick };

visualControl primaryTrimPage[] =
{
{ &trim4l, dctImage },
{ &trim4r, dctImage },
{ &trim1l, dctImage },
{ &trim1r, dctImage },
{ &trim3u, dctImage },
{ &trim3d, dctImage },
{ &trim2u, dctImage },
{ &trim2d, dctImage },
{ &trimrate, dctImage },
{ &trimhome, dctImage } };

//edit model page

labelControl editModModLabel =
{ &liveModel.name, 0, 0, 100, 8, D_STD_FONT, D_NORMAL, D_VISIBLE, D_REDRAW,
		NULL };
labelControl editModCopyLabel =
{ "Copy", 0, 8, 36, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		copyModelClick };
labelControl editModBindLabel =
{ "Bind", 64, 8, 36, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		bindModelClick };

visualControl editModelPage[] =
{
{ &editModModLabel, dctLabel },
{ &editModCopyLabel, dctLabel },
{ &editModBindLabel, dctLabel },
{ &p1Up, dctImage },
{ &p1Down, dctImage } };

//imageControl editModModDown={downicon8x8,24,0,8,8,TRUE,FALSE,editModModDownClick};
//imageControl editModModUp={upicon8x8,32,0,8,8,TRUE,FALSE,editModModUpClick};
//labelControl editModModVal={&liveModel.name,40,0,24,8,8,0,TRUE,0,0,"i"};


//edit mixes page
int currentMixEdit = 0;

labelControl editMixMixLabel =
{ "Mix", 0, 0, 24, 8, 8, 0, TRUE, FALSE, NULL };
imageControl editMixMixDown =
{ downicon8x8, 24, 0, 8, 8, TRUE, FALSE, editMixMixDownClick };
numberControl editMixMixVal =
{ &currentMixEdit, 32, 0, 24, 8, 8, 0, TRUE, 0, 0, "i" };
imageControl editMixMixUp =
{ upicon8x8, 56, 0, 8, 8, TRUE, FALSE, editMixMixUpClick };

labelControl editMixInLabel =
{ "In", 0, 8, 24, 8, 8, 0, TRUE, FALSE, NULL };
imageControl editMixInDown =
{ downicon8x8, 24, 8, 8, 8, TRUE, FALSE, editMixInDownClick };
numberControl editMixInVal =
{ &liveModel.mixes[0].inChannel, 32, 8, 24, 8, 8, 0, TRUE, 0, 0, "b" };
imageControl editMixInUp =
{ upicon8x8, 56, 8, 8, 8, TRUE, FALSE, editMixInUpClick };

labelControl editMixOutLabel =
{ "Out", 0, 16, 24, 8, 8, 0, TRUE, FALSE, NULL };
numberControl editMixOutVal =
{ &liveModel.mixes[0].outChannel, 32, 16, 24, 8, 8, 0, TRUE, 0, 0, "b" };

labelControl editMixLoLabel =
{ "Lo", 0, 24, 24, 8, 8, 0, TRUE, FALSE, NULL };
numberControl editMixLoVal =
{ &liveModel.mixes[0].rateLow, 32, 24, 24, 8, 8, 0, TRUE, 0, 0, "hi" };

labelControl editMixHiLabel =
{ "Hi", 0, 32, 24, 8, 8, 0, TRUE, FALSE, NULL };
numberControl editMixHiVal =
{ &liveModel.mixes[0].rateHigh, 32, 32, 24, 8, 8, 0, TRUE, 0, 0, "hi" };

visualControl editMixPage[] =
{
{ &editMixMixLabel, dctLabel },
{ &editMixMixUp, dctImage },
{ &editMixMixVal, dctNumber },
{ &editMixMixDown, dctImage },
{ &editMixInLabel, dctLabel },
{ &editMixInUp, dctImage },
{ &editMixInVal, dctNumber },
{ &editMixInDown, dctImage },
{ &editMixOutLabel, dctLabel },
{ &editMixOutVal, dctNumber },
{ &editMixLoLabel, dctLabel },
{ &editMixLoVal, dctNumber },
{ &editMixHiLabel, dctLabel },
{ &editMixHiVal, dctNumber },
{ &p1Up, dctImage },
{ &p1Down, dctImage }

};

//string editor page - on screen keyboard
labelControl editStringVal =
{ liveModel.name, 0, 0, 100, 8, D_STD_FONT, D_NORMAL, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

labelControl
		editString1 =
		{ "1", 4, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
				virtualKeyClick };
labelControl editString2 =
{ "2", 13, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString3 =
{ "3", 22, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString4 =
{ "4", 31, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString5 =
{ "5", 40, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString6 =
{ "6", 49, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString7 =
{ "7", 58, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString8 =
{ "8", 67, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString9 =
{ "9", 76, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editString0 =
{ "0", 85, 16, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

labelControl
		editStringq =
		{ "q", 8, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
				virtualKeyClick };
labelControl editStringw =
{ "w", 17, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringe =
{ "e", 26, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringr =
{ "r", 35, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringt =
{ "t", 44, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringy =
{ "y", 53, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringu =
{ "u", 62, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringi =
{ "i", 71, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringo =
{ "o", 80, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringp =
{ "p", 89, 24, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

labelControl editStringa =
{ "a", 12, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStrings =
{ "s", 21, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringd =
{ "d", 30, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringf =
{ "f", 39, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringg =
{ "g", 48, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringh =
{ "h", 57, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringj =
{ "j", 66, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringk =
{ "k", 75, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringl =
{ "l", 84, 32, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

labelControl editStringz =
{ "z", 17, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringx =
{ "x", 26, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringc =
{ "c", 35, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringv =
{ "v", 44, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringb =
{ "b", 53, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringn =
{ "n", 62, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };
labelControl editStringm =
{ "m", 71, 40, 8, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

labelControl editStringSpace =
{ " ", 16, 48, 64, 8, D_STD_FONT, D_INVERSE, D_VISIBLE, D_REDRAW,
		virtualKeyClick };

imageControl editStringDel =
{ lefticon8x8, 88, 48, 8, 8, TRUE, FALSE, virtualKeyDelClick };
imageControl editStringCaps =
{ upicon8x8, 0, 32, 8, 8, TRUE, FALSE, virtualKeyCapsClick };

visualControl editStringPage[] =
{
{ &editStringDel, dctImage },
{ &editStringVal, dctLabel },

{ &editString1, dctLabel },
{ &editString2, dctLabel },
{ &editString3, dctLabel },
{ &editString4, dctLabel },
{ &editString5, dctLabel },
{ &editString6, dctLabel },
{ &editString7, dctLabel },
{ &editString8, dctLabel },
{ &editString9, dctLabel },
{ &editString0, dctLabel },

{ &editStringq, dctLabel },
{ &editStringw, dctLabel },
{ &editStringe, dctLabel },
{ &editStringr, dctLabel },
{ &editStringt, dctLabel },
{ &editStringy, dctLabel },
{ &editStringu, dctLabel },
{ &editStringi, dctLabel },
{ &editStringo, dctLabel },
{ &editStringp, dctLabel },

{ &editStringa, dctLabel },
{ &editStrings, dctLabel },
{ &editStringd, dctLabel },
{ &editStringf, dctLabel },
{ &editStringg, dctLabel },
{ &editStringh, dctLabel },
{ &editStringj, dctLabel },
{ &editStringk, dctLabel },
{ &editStringl, dctLabel },

{ &editStringz, dctLabel },
{ &editStringx, dctLabel },
{ &editStringc, dctLabel },
{ &editStringv, dctLabel },
{ &editStringb, dctLabel },
{ &editStringn, dctLabel },
{ &editStringm, dctLabel },

{ &editStringSpace, dctLabel },
{ &editStringCaps, dctImage },
{ &p1Up, dctImage },
{ &p1Down, dctImage } };

//array of all screen pages in system
visualPage pages[] =
{
{ page1, sizeof(page1) / sizeof(page1[0]) },
{ page2, sizeof(page2) / sizeof(page2[0]) },
{ editModelPage, sizeof(editModelPage) / sizeof(editModelPage[0]) },
{ editMixPage, sizeof(editMixPage) / sizeof(editMixPage[0]) },
{ editStringPage, sizeof(editStringPage) / sizeof(editStringPage[0]) },
{ tsTest, sizeof(tsTest) / sizeof(tsTest[0]) },
{ primaryTrimPage, sizeof(primaryTrimPage) / sizeof(primaryTrimPage[0]) },

};

lcdEADog lcd;
//screen buffer
uint8 lcdbuf[8* 128 ]__attribute__ ((aligned (4)));

int currentPage = 0;
int lastPage = -1;


/****************************************************************************
 *
 * NAME: initGUI
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *  				model
 *  				txparams
 *
 * RETURNS: void
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void initGUI()
{
	modelLabel.txt = liveModel.name;
	modelLabel.valid = FALSE;

	initLcdEADog(E_AHI_SPIM_SLAVE_ENBLE_2, E_AHI_DIO4_INT, E_AHI_DIO5_INT, 180,
				&lcd);
	currentPage = 0;


}

/****************************************************************************
 *
 * NAME: refreshGUI()
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void refreshGUI()
{
	renderPage(pages[currentPage].controls, pages[currentPage].len,
						lastPage != currentPage, lcdbuf, 128);
	LcdBitBlt(lcdbuf, 128, 0, 0, 128, 64, &lcd);
	lastPage = currentPage;
}

/****************************************************************************
 *
 * NAME: pollTouchScreen
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS: void
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
void pollTouchScreen()
{
	if (readTsc2003(&touchScreen) == TRUE)
	{
		// pcComsPrintf("tsc %d %d %d \r\n",touchScreen.x,touchScreen.y,touchScreen.pressure);


		if (touchScreen.pressure > 0)
		{
			tsPressedTimer++;

			//ignore first reading of a click
			if (tsPressedTimer == 2)
			{
				//allow 2 clicks per sec and auto repeat at that rate

				tsDebounceLast = tsDebounceCount;
				displayClick(touchScreen.x, touchScreen.y);
				//pcComsPrintf("C %d %d ",touchScreen.x,tsY=touchScreen.y);
				tsClickX = touchScreen.x;
				tsClickY = touchScreen.y;

			}
			//test for drag event
			if (tsPressedTimer > 2 && tsPressedTimer % 5 == 0)
			{
				int dx = touchScreen.x - tsClickX;
				int dy = touchScreen.y - tsClickY;
				if (abs(dx) > 1 || abs(dy) > 1)
				{
					displayDrag(dx >> 1, dy >> 1);
				}
				tsClickX = touchScreen.x;
				tsClickY = touchScreen.y;

			}
		}
		else
		{
			tsPressedTimer = 0;
			//clear focus
			focusedControl = NULL;
		}
		tsX = touchScreen.x;
		tsY = touchScreen.y;
		tsP = touchScreen.pressure;

	}

	tsDebounceCount++;
}

void setModelUp(clickEventArgs* clickargs)
{
	//brute force load each model as we move through list

	nextModel();
	p2modelLabel.valid = FALSE;
	editModModLabel.valid = FALSE;
	modelLabel.valid = FALSE;
	p2modelLabel.valid = FALSE;
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
	liveModel.trim[3] -= 32;
}
void trim4rClick(clickEventArgs* clickargs)
{
	liveModel.trim[3] += 32;
}
void trim1lClick(clickEventArgs* clickargs)
{
	liveModel.trim[0] -= 32;
}
void trim1rClick(clickEventArgs* clickargs)
{
	liveModel.trim[0] += 32;
}
void trim3uClick(clickEventArgs* clickargs)
{
	liveModel.trim[2] += 32;
}
void trim3dClick(clickEventArgs* clickargs)
{
	liveModel.trim[2] -= 32;
}
void trim2uClick(clickEventArgs* clickargs)
{
	liveModel.trim[1] += 32;
}
void trim2dClick(clickEventArgs* clickargs)
{
	liveModel.trim[1] -= 32;
}
void rateClick(clickEventArgs* clickargs)
{
	if (liveModel.rateMode == 0)
	{
		liveModel.rateMode = 1;
		//ought to automate this, maybe bind image list to enums or int
		trimrate.image = hiicon;
		trimrate.valid = FALSE;
	}
	else
	{
		liveModel.rateMode = 0;
		trimrate.image = lowicon;
		trimrate.valid = FALSE;
	}
}
void homeClick(clickEventArgs* clickargs)
{
	currentPage = 0;
}

void bindMixEditor()
{
	editMixInVal.num = &liveModel.mixes[currentMixEdit].inChannel;
	editMixOutVal.num = &liveModel.mixes[currentMixEdit].outChannel;
	editMixLoVal.num = &liveModel.mixes[currentMixEdit].rateLow;
	editMixHiVal.num = &liveModel.mixes[currentMixEdit].rateHigh;

}
void editMixMixUpClick(clickEventArgs* clickargs)
{
	currentMixEdit++;
	if (currentMixEdit > MAXMIXES)
		currentMixEdit = 0;
	bindMixEditor();
}
void editMixMixDownClick(clickEventArgs* clickargs)
{
	currentMixEdit--;
	if (currentMixEdit < 0)
		currentMixEdit = MAXMIXES - 1;
	bindMixEditor();

}
void editMixInUpClick(clickEventArgs* clickargs)
{
	uint8 val = liveModel.mixes[currentMixEdit].inChannel;
	val++;
	if (val >= MAXCHANNELS)
		val = NOMIX;
	liveModel.mixes[currentMixEdit].inChannel = val;
}
void editMixInDownClick(clickEventArgs* clickargs)
{
	uint8 val = liveModel.mixes[currentMixEdit].inChannel;
	if (val == NOMIX)
		val = MAXCHANNELS - 1;
	else
		val--;
	liveModel.mixes[currentMixEdit].inChannel = val;
}

void copyModelClick(clickEventArgs* clickargs)
{
	copyModel();


	//storeSettings();
	//must improve binding of strings
	editModModLabel.valid = FALSE;
	modelLabel.valid = FALSE;
	p2modelLabel.valid = FALSE;
}

void pageUp(clickEventArgs* clickargs)
{
	if (currentPage >= sizeof(pages) / sizeof(visualPage) - 1)
		currentPage = 0;
	else
		currentPage++;
}
void pageDown(clickEventArgs* clickargs)
{
	if (currentPage == 0)
		currentPage = sizeof(pages) / sizeof(visualPage) - 1;
	else
		currentPage--;
}
void virtualKeyCapsClick(clickEventArgs* clickargs)
{
	int i;
	for (i = 0; i < 26; i++)
	{
		labelControl* lc = (labelControl*) editStringPage[i + 12].control;
		char c = lc->txt[0];
		if (c > 'Z')
			c -= 'a' - 'A';
		else
			c += 'a' - 'A';
		lc->txt[0] = c;
		lc->valid = FALSE;
	}
}
void virtualKeyDelClick(clickEventArgs* clickargs)
{
	int len;
	len = strlen(editStringVal.txt);
	if (len > 0)
	{
		editStringVal.txt[len - 1] = '\0';

		editStringVal.valid = FALSE;
		editModModLabel.valid = FALSE;
		modelLabel.valid = FALSE;
		p2modelLabel.valid = FALSE;
	}
}
void virtualKeyClick(clickEventArgs* clickargs)
{
	visualControl* vc = clickargs->sender;
	int len;
	if (vc->type == dctLabel)
	{
		//copy text from button
		labelControl* lc = (labelControl*) vc->control;
		len = strlen(editStringVal.txt);
		if (len < 30)
		{
			editStringVal.txt[len + 1] = '\0';
			editStringVal.txt[len] = lc->txt[0];
		}
	}
	editStringVal.valid = FALSE;
	editModModLabel.valid = FALSE;
	modelLabel.valid = FALSE;
	p2modelLabel.valid = FALSE;
}

void displayClick(uint8 x, uint8 y)
{
	//turn on backlight for a while
	backlightTimer = 50* 30 ;
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

	if (focusedControl != NULL)
	{
		//bind to displayed number
		if (focusedControl->type == dctNumber)
		{

			numberControl* nc = (numberControl*) focusedControl->control;
			if (nc->format[0] == 'b')//uint8 binding
			{
				*((uint8*) (nc->num)) += (uint8) dx;
			}
			if (nc->format[0] == 'h')//uint8 binding
			{
				*((int16*) (nc->num)) += (int16) dx;
			}
		}
	}
}
void sleepClick(clickEventArgs* clickargs)
{
	sleep();
}

