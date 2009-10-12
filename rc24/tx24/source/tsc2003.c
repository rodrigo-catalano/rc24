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
#include "tsc2003.h"
#include "ps2.h"




bool initTsc2003(tsc2003* tsc,uint8 addr,int rx,int ry)
{
    tsc->addr=addr; //0x48 default
    tsc->rx=rx;
    tsc->ry=ry;

    //tl 240,29
    //br 14,228
    tsc->calibL=240;
    tsc->calibR=14;
    tsc->calibT=29;
    tsc->calibB=228;

    tsc->pressure=0;

    vSMBusInit();

    //rx 723 ry 196
    return readTsc2003(tsc);
}
bool pressedTsc2003(tsc2003* tsc)
{
   //quick test for pressed - to save time / power in wakeup event
    vSMBusInit();

    bool_t bOk = TRUE;
    uint8 z1;
    bOk &=bSMBusWrite(tsc->addr, 0xe6, 0, NULL);
    cycleDelay(10*16);
    bOk &=bSMBusSequentialRead(tsc->addr, 1, &z1);

    if(z1<5) return FALSE;
    else return TRUE;

}


bool readTsc2003(tsc2003* tsc)
{
    uint8 x,y,z1,z2;
    int t;
    bool_t bOk = TRUE;
  //  bOk &=bSMBusRandomRead(tsc->addr, 0xc6, 1, &t);

    bOk &=bSMBusWrite(tsc->addr, 0xc6, 0, NULL);
    cycleDelay(10*16);

    bOk &= bSMBusSequentialRead(tsc->addr, 1, &x);
  //y

    bOk &=bSMBusWrite(tsc->addr, 0xd6, 0, NULL);
    cycleDelay(10*16);
    bOk &=bSMBusSequentialRead(tsc->addr, 1, &y);


    //z1

    bOk &=bSMBusWrite(tsc->addr, 0xe6, 0, NULL);
    cycleDelay(10*16);
    bOk &=bSMBusSequentialRead(tsc->addr, 1, &z1);

    //z2
    bOk &=bSMBusWrite(tsc->addr, 0xf6, 0, NULL);
    cycleDelay(10*16);
    bOk &=bSMBusSequentialRead(tsc->addr, 1, &z2);

    if(bOk)
    {
        //method 1
 //   int p=tsc->rx*tsc->x*(z2*256/z1-256)/65536;
 //method 2
    //    int p =tsc->rx*tsc->x/256*(256/z1-1)-tsc->ry*(1-(tsc->y/256));


  //   if(p>255)p=255;
    // tsc->pressure=z1;
        tsc->lastPressure=tsc->pressure;

         if(z1<5) tsc->pressure=0;
          else tsc->pressure=255;

        t=(x-tsc->calibL) *128/(tsc->calibR-tsc->calibL);
        if(t>=128) t=127;
        if(t<0)t=0;
        tsc->x= (uint8)t;

        t=(y-tsc->calibT) *64/(tsc->calibB-tsc->calibT);
        if(t>=64) t=63;
        if(t<0)t=0;
        tsc->y= (uint8)t;
    }


    return bOk;
}
void sleepTsc2003(tsc2003* tsc)
{
    bool_t bOk = TRUE;
    uint8 x;
    bOk &=bSMBusWrite(tsc->addr, 0xc2, 0, NULL);
    cycleDelay(10*16);

    bOk &= bSMBusSequentialRead(tsc->addr, 1, &x);


}
