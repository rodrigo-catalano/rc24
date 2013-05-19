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

#include <LcdFont.h>

#include "lcdEADog.h"
#include "hwutils.h"




void initLcdEADog(uint8 cs,int reset,int a0,int angle,lcdEADog* lcd)
{
    lcd->spicspin=cs;
    lcd->resetpin=reset;
    lcd->a0pin=a0;
    lcd->angle=angle;

    uint8 config0deg[14]={0x40,0xa1,0xc0,0xa6,0xa2,0x2f,0xf8,0x00,0x27,0x81,0x16,0xac,0x00,0xaf};
    uint8 config180deg[14]={0x40,0xa0,0xc8,0xa6,0xa2,0x2f,0xf8,0x00,0x27,0x81,0x16,0xac,0x00,0xaf};

    vAHI_SpiConfigure( 2, /* number of slave select lines in use */
        E_AHI_SPIM_MSB_FIRST, /* send data MSB first */
        FALSE,
        FALSE,
        1,//8MHz
        E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
        E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */


    vAHI_DioSetDirection(0,lcd->resetpin);
    vAHI_DioSetDirection(0,lcd->a0pin);


    vAHI_DioSetOutput(0,lcd->resetpin);

    cycleDelay(16*4000);
    vAHI_DioSetOutput(lcd->resetpin,0);

    vAHI_DioSetOutput(0,lcd->a0pin);


    int i;

    vAHI_SpiSelect(lcd->spicspin);

    for(i=0;i<sizeof(config0deg);i++)
    {
        if(angle==180)vAHI_SpiStartTransfer8(config180deg[i]);
        else vAHI_SpiStartTransfer8(config0deg[i]);
        vAHI_SpiWaitBusy();
    }
    vAHI_SpiStop();
}
void LcdBitBlt(uint8* buf,int scanlen,int x,int y,int w,int h,lcdEADog* lcd)
{
    //copy an area of memory to lcd
    vAHI_SpiConfigure( 2, /* number of slave select lines in use */
        E_AHI_SPIM_MSB_FIRST, /* send data MSB first */
        FALSE,
        FALSE,
        0,//16MHz
        E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
        E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */



    vAHI_SpiSelect(lcd->spicspin);
    uint32* buf32=(uint32*)buf;

    if(lcd->angle!=0)x+=4;
    int xx,yy;
    int ints=w>>2;
    uint8 colhigh=0x10+(x>>4);
    uint8 collow=0x00+(x&0x0f);
    h=h>>3;
    for(yy=0;yy<h;yy++)
    {
        vAHI_DioSetOutput(0,lcd->a0pin);

        vAHI_SpiStartTransfer8(0xb0+yy);
        vAHI_SpiWaitBusy();
        vAHI_SpiStartTransfer8(colhigh);
        vAHI_SpiWaitBusy();
        vAHI_SpiStartTransfer8(collow);
        vAHI_SpiWaitBusy();

        vAHI_DioSetOutput(lcd->a0pin,0);

        for(xx=0;xx< ints;xx++)
        {
            vAHI_SpiStartTransfer32(*buf32++);

#if (defined JN5148 || defined JN5168 )
      vAHI_SpiWaitBusy();
#else
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
            asm volatile("l.nop;");
#endif
        }

    }

    vAHI_SpiStop();

}
void sleepLcdEADog(lcdEADog* lcd)
{
     vAHI_SpiConfigure( 2, /* number of slave select lines in use */
        E_AHI_SPIM_MSB_FIRST, /* send data MSB first */
        FALSE,
        FALSE,
        0,//16MHz
        E_AHI_SPIM_INT_DISABLE, /* Disable SPI interrupt */
        E_AHI_SPIM_AUTOSLAVE_DSABL); /* Disable auto slave select        */


    vAHI_DioSetOutput(0,lcd->a0pin);

    vAHI_SpiSelect(lcd->spicspin);
    vAHI_SpiStartTransfer8(0xae);
    vAHI_SpiWaitBusy();
    vAHI_SpiStartTransfer8(0xa5);
    vAHI_SpiWaitBusy();

    vAHI_SpiStop();

    //saves 0.1mA
    vAHI_DioSetOutput(lcd->a0pin,0);

//vAHI_DioSetOutput(lcd->spicspin,0);

//   vAHI_DioSetOutput(0,lcd->resetpin);



}


/*
void vWriteText(uint8* buf,int scanlen,char *pcString,int w,int h, uint8 u8Row, uint8 u8Column, uint8 u8Mask)
{
    //masks for fonts up to 24 pixels high

    static int masks[]{0x0001,0x0003,0x0007,0x000f,0x001f,0x003f,0x007f,0x00ff,
        0x01ff,0x03ff,0x07ff,0x0fff,0x01fff,0x03fff,0x07fff,0xffff,
        0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff};

    int shift=y&0x7;
    int firstrow=y>>3;
    int mask=masks[h]<<shift;

    uint8* maskbytes=(uint8*)&mask;

    while (*pcString != 0)
    {
        pu8CharMap = pu8LcdFontGetChar(*pcString);
        u8Columns = *pu8CharMap;
        do
        {
            pu8CharMap++;
            *pu8Shadow = (*pu8CharMap) ^ u8Mask;
            pu8Shadow++;
            u8Columns--;
        } while (u8Columns > 0);

    }
}
*/
