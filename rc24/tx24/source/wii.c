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
#include <SMBus.h>
#include "ps2.h"
#include "wii.h"

bool initWiiController(WiiController* wii)
{
    bool_t bOk = TRUE;
    vSMBusInit();

    uint8 buff[1];
    buff[0]=0;
    bOk &=bSMBusWrite(0x52, 0x40, 1, &buff[0]);
    cycleDelay(200*16);
    return bOk;
}
bool readWiiController(WiiController* wii)
{
    uint8 buf[6];
    int i;
    bool_t bOk = TRUE;

    bSMBusWrite(0x52, 0x00, 0, NULL);

    cycleDelay(200*16);

    bOk &=bSMBusSequentialRead(0x52, 6, &buf[0]);

    for(i=0;i<6;i++)
    {
        buf[i] = (buf[i] ^ 0x17) + 0x17;
    }

    if(bOk)
    {
        wii->JoyX=buf[0];
        wii->JoyY=buf[1];
        wii->AccelX=(buf[2]<<2)+((buf[5]>>2)&0x03);
        wii->AccelY=(buf[3]<<2)+((buf[5]>>4)&0x03);
        wii->AccelZ=(buf[4]<<2)+((buf[5]>>6)&0x03);
        wii->ZButton=(buf[5]&0x01)==0;
        wii->CButton=(buf[5]&0x02)==0;

        return TRUE;
    }
    else return FALSE;
 }
void wiiDump(WiiController* wii)
{
	/*
    vPrintf(" %d %d %d %d %d     \r\n",
        wii->JoyX,
        wii->JoyY,
        wii->AccelX,
        wii->AccelY,
        wii->AccelZ);

    if(wii->ZButton)vPrintf("Z ");
    if(wii->CButton)vPrintf("C ");
*/
}
