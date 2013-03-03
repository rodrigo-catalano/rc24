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

#include "hwutils.h"
#include "codeUpdate.h"

uint32 codeUpdateLength;
uint8 codeUpdateCrc;

/*
On the jn5148 this copies the new code into the second flash sector
and only erases the first sector once all code has been downloaded
the code is then copied from sector 1 to sector 0.

On the jn5139 there is no enough flash to buffer the code so it
writes it as it goes along, if something goes wron during download
the device will be left partly programmed and it will need reprogramming
in bootloader mode


*/

uint8 startCodeUpdate(uint8* inMsg, uint8* outMsg)
{
	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	//clear buffer in flash 32k for 5139 or 64k for 5148

	codeUpdateLength=0;
	codeUpdateCrc=0;

	//send ack

	outMsg[0]=0x91;
	//todo store module type somewhere
	bAHI_FlashEraseSector(1);
#ifdef JN5148
		outMsg[1]=6;
		outMsg[2]='J';
		outMsg[3]='N';
		outMsg[4]='5';
		outMsg[5]='1';
		outMsg[6]='4';
		outMsg[7]='8';
	#else

		outMsg[1]=6;
		outMsg[2]='J';
		outMsg[3]='N';
		outMsg[4]='5';
		outMsg[5]='1';
		outMsg[6]='3';
		outMsg[7]='9';
	#endif

	return 8;
}
uint8 codeUpdateChunk(uint8* inMsg, uint8* outMsg)
{
	//todo check no chunks are missed and probably use bigger crc
	uint32 sectorSize;
	uint32 writeBase;
	int i;

#ifdef JN5148
		sectorSize=0x10000;
		writeBase=sectorSize;
	#else
		sectorSize=0x8000;
		writeBase=0;
	#endif
	uint8 len=inMsg[1];
	for(i=2;i<len+2;i++)
	{
		codeUpdateCrc^=inMsg[i];
	}
	//copy in mac address and clear sectors on jn5139
	if(codeUpdateLength==0)
	{
		bAHI_FullFlashRead(48,32,&inMsg[48+2]);
	#if (JENNIC_CHIP_FAMILY == JN514x)

	#else
		bAHI_FlashEraseSector(0);
	#endif
	}
	//write to flash buffer
	bAHI_FullFlashProgram(writeBase+codeUpdateLength,len,&inMsg[2]);

	codeUpdateLength+=len;
	//send ack
	outMsg[0]=0x93;
	return 1;
}
uint8 commitCodeUpdate(uint8* inMsg, uint8* outMsg)
{
	uint32 sectorSize;
    int rxbat=u16ReadADC(E_AHI_ADC_SRC_VOLT);//3838 4096 = 2.4*1.5 =3.6v
    rxbat=rxbat*360/4096;
    uint8 copyBuf[256];
    uint32 maxUpload;

#ifdef JN5148
			sectorSize=0x10000;
			maxUpload=sectorSize;
	#else
			sectorSize=0x8000;
			maxUpload=2*sectorSize;
	#endif


	if(rxbat<300 )
	{
		outMsg[0]=0x95;//fail
		outMsg[1]=3;
		return 2;
	}
	if(codeUpdateLength>maxUpload)
	{
		outMsg[0]=0x95;//fail
		outMsg[1]=2;
		return 2;
	}
	if(codeUpdateCrc!=inMsg[1])
	{
		outMsg[0]=0x95;//fail
		outMsg[1]=1;
		return 2;
	}

#ifdef JN5148
	//copy flash sector 1 to sector 0
	bAHI_FlashEraseSector(0);
	uint32 addr=0;

	while(addr<codeUpdateLength)
	{
		bAHI_FullFlashRead(addr+sectorSize,256,copyBuf);
		bAHI_FullFlashProgram(addr,256,copyBuf);
		addr+=256;
	}
#endif

	vAHI_SwReset();
	return 0;
}
