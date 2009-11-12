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
#include "ppm.h"


#define MAX_PPM_CHANNELS 8

uint8 ppmTimer;
bool ppmSynced=FALSE;
int ppmSeqIdx=0;

int ppmInputs[MAX_PPM_CHANNELS];


void ppm_interrupt_handler(uint32 u32DeviceId,uint32 u32ItemBitmap)
{
     uint16 high;
     uint16 low;
     int len;

     vAHI_TimerReadCaptureFreeRunning(
        ppmTimer,
        &high,
        &low);

    len=low;

    if(len>3000*2 && len< 30000*2)
    {
        ppmSynced=TRUE;
        ppmSeqIdx=-1;
    }
    else
    {
        ppmSeqIdx++;

        if(ppmSynced==TRUE)
        {
            //check pulse is of believable length
            if(len<2300*2 && len>700*2)
            {
                if(ppmSeqIdx>=0 && ppmSeqIdx<MAX_PPM_CHANNELS)
                {
                    ppmInputs[ppmSeqIdx]=len;
                }
            }
            else
            {
                ppmSynced=FALSE;
            }
        }
    }
}

void initPpmInput(uint8 timer)
{
    int i;
    for(i=0;i<8;i++)
    {
        ppmInputs[i]=-1;
    }

    ppmTimer=timer;
    vAHI_TimerEnable(
        timer,
        3,//seems to work giving max period of 32ms and resolution of 0.5us
        FALSE,
        TRUE,
        FALSE);

    if(timer==E_AHI_TIMER_0)vAHI_Timer0RegisterCallback(ppm_interrupt_handler);
    else vAHI_Timer1RegisterCallback(ppm_interrupt_handler);

    vAHI_TimerStartCapture(timer);
}
void ppmRead(int* values)
{
    int i;
    for(i=0;i<MAX_PPM_CHANNELS;i++)
    {
        values[i]=(ppmInputs[i]-3000)*2048/1000;
    }
}
