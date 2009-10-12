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

//todo cope with images bigger than 1 sector
//on jn139 just write direct if no room for buffer



uint8 startCodeUpdate(uint8* inMsg, uint8* outMsg)
{
	bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);

	//clear buffer in flash 32k for 5139 or 64k for 5148

	codeUpdateLength=0;
	codeUpdateCrc=0;

	bAHI_FlashEraseSector(1);

	//send ack
	outMsg[0]=0x91;
	return 1;
}
uint8 codeUpdateChunk(uint8* inMsg, uint8* outMsg)
{
	//todo check no chunks are missed and probably use bigger crc
	uint32 sectorSize;
	int i;

	#if (JENNIC_CHIP_FAMILY == JN514x)
		sectorSize=0x10000;
	#else
		sectorSize=0x8000;
	#endif
	uint8 len=inMsg[1];

	//copy in mac address
	if(codeUpdateLength==0)
	{
		bAHI_FullFlashRead(48,32,&inMsg[48+2]);
	}
	//write to flash buffer
	bAHI_FullFlashProgram(sectorSize+codeUpdateLength,len,&inMsg[2]);
	for(i=2;i<len+2;i++)
	{
		codeUpdateCrc^=inMsg[i];
	}
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

	#if (JENNIC_CHIP_FAMILY == JN514x)
			sectorSize=0x10000;
	#else
			sectorSize=0x8000;
	#endif


	if(rxbat<300 )
	{
		outMsg[0]=0x95;//fail
		outMsg[1]=3;
		return 2;
	}
	if(codeUpdateLength>sectorSize)
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

	//copy flash sector 1 to sector 0
	bAHI_FlashEraseSector(0);
	uint32 addr=0;

	while(addr<codeUpdateLength)
	{
		bAHI_FullFlashRead(addr+sectorSize,256,copyBuf);
		bAHI_FullFlashProgram(addr,256,copyBuf);
		addr+=256;
	}
	vAHI_SwReset();
	return 0;
}
