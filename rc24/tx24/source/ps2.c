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
#include <printf.h>

#include "ps2.h"
#include "hwutils.h"

PRIVATE void delay(int t);
uint8 ps2cmd(uint8 cmd);

uint8 ps2cmd(uint8 cmd)
{
 //   cycleDelay(20*16);

    vAHI_SpiStartTransfer8(cmd);
    vAHI_SpiWaitBusy();
    return u8AHI_SpiReadTransfer8();
}

bool initPS2Controller(ps2ControllerOp* ps2)
{
    //force analog mode
    //0x01, 0x43, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 //enter config mode
    //0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00 // set to analog mode and lock out the mode button
    //0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

    uint8 cmd[4][9]={{0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x01, 0x43, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
                    {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00},
                    {0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

    int i;
    int j;
    int del=160;
    uint8 u8Temp;


     vAHI_SpiConfigure( 1, /* number of slave select lines in use */
            E_AHI_SPIM_LSB_FIRST,
             1   ,
             1   ,
            63,

            E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
            E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */

    /* select spi device */
   delay(del*200);


    for(j=0;j<3;j++)
    {
        vAHI_SpiSelect(E_AHI_SPIM_SLAVE_ENBLE_1);

        delay(del*2);
        for(i=0;i<9;i++)
        {

            u8Temp=ps2cmd(cmd[j][i]);
  //          delay(del);

        }
        vAHI_SpiStop();
        delay(del*6);
    }
    return readPS2Controller(ps2);

}
bool readPS2Controller(ps2ControllerOp* ps2)
{
        uint8 u8Temp;
        //mode 3 spi 250khz

        vAHI_SpiConfigure( 1, /* number of slave select lines in use */
            E_AHI_SPIM_LSB_FIRST,
             1  ,
             1  ,
            32,

            E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
            E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */



      /* select spi device */
        vAHI_SpiSelect(E_AHI_SPIM_SLAVE_ENBLE_1);

 cycleDelay(48*16);


        u8Temp=ps2cmd(0x01);
        ps2->Model=ps2cmd(0x42);
        ps2->Status=ps2cmd(0);
        if(ps2->Status!=0x5a || !(ps2->Model==0xf3 || ps2->Model==0xd3))
        {
            return FALSE;
        }
        ps2->LeftButtons=ps2cmd(0);
        ps2->RightButtons=ps2cmd(0);
        ps2->RJoyX=ps2cmd(0);
        ps2->RJoyY=ps2cmd(0);
        ps2->LJoyX=ps2cmd(0);
        ps2->LJoyY=ps2cmd(0);

cycleDelay(48*16);


        vAHI_SpiStop();

        if(ps2->Model==0x73||ps2->Model==0xf3)
        {
            ps2->LFire1 = (ps2->RightButtons&4)==0;
            ps2->LFire2 = (ps2->RightButtons&1)==0;
            ps2->RFire1 = (ps2->RightButtons&8)==0;
            ps2->RFire2 = (ps2->RightButtons&2)==0;
            ps2->LUp    = (ps2->LeftButtons&16)==0;
            ps2->LDown  = (ps2->LeftButtons&64)==0;
            ps2->LLeft  = (ps2->LeftButtons&128)==0;
            ps2->LRight = (ps2->LeftButtons&32)==0;
            ps2->RUp    = (ps2->RightButtons&16)==0;
            ps2->RDown  = (ps2->RightButtons&64)==0;
            ps2->RLeft  = (ps2->RightButtons&128)==0;
            ps2->RRight = (ps2->RightButtons&32)==0;
            ps2->Select = (ps2->LeftButtons&1)==0;
            ps2->Start  = (ps2->LeftButtons&8)==0;
            ps2->RJoy   = (ps2->LeftButtons&4)==0;
            ps2->LJoy   = (ps2->LeftButtons&2)==0;

        }
        if(ps2->Model==0x53||ps2->Model==0xd3)
        {
            ps2->LFire1 = (ps2->RightButtons&2)==0;
            ps2->LFire2 = (ps2->RightButtons&1)==0;
            ps2->RFire1 = (ps2->RightButtons&8)==0;
            ps2->RFire2 = (ps2->RightButtons&128)==0;
            ps2->LUp    = (ps2->LeftButtons&16)==0;
            ps2->LDown  = (ps2->LeftButtons&64)==0;
            ps2->LLeft  = (ps2->LeftButtons&128)==0;
            ps2->LRight = (ps2->LeftButtons&32)==0;
            ps2->RUp    = (ps2->RightButtons&8)==0;
            ps2->RDown  = (ps2->RightButtons&64)==0;
            ps2->RLeft  = (ps2->RightButtons&4)==0;
            ps2->RRight = (ps2->RightButtons&32)==0;
            ps2->Select = FALSE;
            ps2->Start  = FALSE;
            ps2->RJoy   = FALSE;
            ps2->LJoy   = FALSE;

        }

    if(ps2->Status==0x5a)return TRUE;
    else return FALSE;

}
void ps2Dump(ps2ControllerOp* ps2)
{
    vPrintf(" %d %d %d %d %d %d %d %d      \r\n",
        ps2->Model,
        ps2->Status,
        ps2->LeftButtons,
        ps2->RightButtons,
        ps2->RJoyX,
        ps2->RJoyY,
        ps2->LJoyX,
        ps2->LJoyY);

    if(ps2->LFire1)vPrintf("LFire1 ");
    if(ps2->LFire2)vPrintf("LFire2 ");
    if(ps2->RFire1)vPrintf("RFire1 ");
    if(ps2->RFire2)vPrintf("RFire2 ");
    if(ps2->LUp)vPrintf("LUp ");
    if(ps2->LDown)vPrintf("LDown ");
    if(ps2->LLeft)vPrintf("LLeft ");
    if(ps2->LRight)vPrintf("LRight ");
    if(ps2->RUp)vPrintf("RUp ");
    if(ps2->RDown)vPrintf("RDown ");
    if(ps2->RLeft)vPrintf("RLeft ");
    if(ps2->RRight)vPrintf("RRight ");
    if(ps2->Select)vPrintf("Select ");
    if(ps2->Start)vPrintf("Start ");
    if(ps2->LJoy)vPrintf("LJoy ");
    if(ps2->RJoy)vPrintf("RJoy ");


}


PRIVATE void delay(int t)
{
    static int h=0;
    int i;
    for(i=0;i<t*8;i++)
    {
        h++;
    }

}

