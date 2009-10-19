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

#include "myrxs.h"
#include "swEventQueue.h"
#include "servopwm.h"
#include "motorpwm.h"
#include "nmeagps.h"

#include <printf.h>


// map inputs from the tx to outputs from the rx
// outputs can currently be servo pwm,
// constant pwm suitable for simple motor speed control (2 only on timer outputs )
// or on/off digital outputs

// set up for 6ch standard servo output
opChannel standard6channel[20]={ {servopwm,E_AHI_DIO10_INT},
                                 {servopwm,E_AHI_DIO13_INT},
                                 {servopwm,E_AHI_DIO6_INT},
                                 {servopwm,E_AHI_DIO7_INT},
                                 {servopwm,E_AHI_DIO14_INT},
                                 {servopwm,E_AHI_DIO15_INT},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0}};

// set up for 6ch standard servo output with serial debug
opChannel standard6channelDebug[20]={ {servopwm,E_AHI_DIO9_INT},
                                 {servopwm,E_AHI_DIO14_INT},
                                 {servopwm,E_AHI_DIO15_INT},
                                 {servopwm,E_AHI_DIO13_INT},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0}};

// setup for 2 servos and two brushed motors
opChannel propsteer[20]={        {servopwm,E_AHI_DIO6_INT},
                                 {servopwm,E_AHI_DIO7_INT},
                                 {pwm,E_AHI_DIO10_INT},
                                 {pwm,E_AHI_DIO13_INT},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0}};
// setup for 4 servos and gps
opChannel gps[20]=      {        {servopwm,E_AHI_DIO10_INT},
                                 {servopwm,E_AHI_DIO13_INT},
                                 {servopwm,E_AHI_DIO14_INT},
                                 {servopwm,E_AHI_DIO15_INT},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0},
                                 {nc,0}};

opChannel* outputMapping;

void initTxMac(uint32* machigh,uint32* maclow)
{
    //enter the mac address of your transmitter here
    *maclow=0x000a5f1f;
    *machigh=0x00158d00;
}

void initInputs()
{
     //comment out if no gps connected
 //    initNmeaGps(E_AHI_UART_0,E_AHI_UART_RATE_38400);
}

void initOutputs()
{
    int nservos=0;

    int i;
    //set outputs used
//    outputMapping=standard6channel;
    outputMapping=standard6channelDebug;
   // outputMapping=gps;
  //  outputMapping=propsteer;

    for(i=0;i<20;i++)
    {
        switch(outputMapping[i].type)
        {
            case servopwm:
            {
                setServoBit(nservos++,outputMapping[i].pin);
                break;
            }
            case pwm:
            {
                initMotorPwm(outputMapping[i].pin);
                 break;
            }
            case digital:
            {
                vAHI_DioSetDirection(0,outputMapping[i].pin);
                vAHI_DioSetOutput(0,outputMapping[i].pin);
                break;
            }
            case i2c:
            {
                //not implemented
                break;
            }
            case nc:
            {

                break;
            }
        }
    }
    initServoPwm(nservos);
}

void updateOutputs( uint16* channelData )
{
    int i;
    //do any mixing, control loops etc


    //set outputs

    int servo=0;
    for(i=0;i<20;i++)
    {
        if(channelData[i]>=4096) continue;//ignore channels not received yet
        switch(outputMapping[i].type)
        {
            case servopwm:
            {
                uint32 u32Demand;
                u32Demand = channelData[i]*16000/4096+16000;
                if(u32Demand > 2000*16)u32Demand=2000*16;
                if(u32Demand < 1000*16)u32Demand=1000*16;
                setServoDemand(servo++,u32Demand);
                break;
            }
            case pwm:
            {
                setMotorPWM(outputMapping[i].pin,channelData[i]*16000/4096 );
                break;
            }
            case digital:
            {
                if(channelData[i]>3000)vAHI_DioSetOutput(outputMapping[i].pin,0);
                else vAHI_DioSetOutput(0,outputMapping[i].pin);
                break;
            }
            case i2c:
            {
                //not implemented
                break;
            }
            case nc:
            {

                break;
            }
        }
    }
}
