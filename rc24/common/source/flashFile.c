/*
 Copyright 2008 - 2013 © Alan Hopper

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
/*
 flash storage code

 This stores just one file using the top two sectors of flash

 This tries to keep flash wipes to a minimum by storing a version number against files
 and finding the latest.  This has changed from previous versions due to
 limitations in overwriting flash in the JN5168
 It ensures that even during a write a complete copy of settings is on flash.
 This is done by alternating between two erasable blocks of flash.
*/
#include <jendefs.h>
#include <flashFile.h>
#include <AppHardwareApi.h>
#include "hwutils.h"

#define STORE_NORECORD 255
#define STORE_LIVERECORD 63

#ifdef JN5168
void flushFlashBuffer(flashFile* s)
{
	if(s->flashWriteBuffPos>0)
	{
		bAHI_FullFlashProgram(s->flashWriteAddr, s->flashWriteBuffPos, (uint8*) s->flashWriteBuff);
		s->flashWriteAddr+=PAGEWORDLEN;
		s->flashWriteBuffPos=0;
	}
}
#endif
bool flashOpenRead(flashFile* s,uint32 filename)
{
	uint8 b;
	int sector;
	uint32 pos=0;
	uint32 len;
	uint32 version=0;
	uint32 tversion;
	uint32 mostRecent=0;
	uint32 mostRecentLen=0;
	//find record with newest version

	for (sector = flashNumSectors()-2; sector <= flashNumSectors()-1; sector++)
	{
		pos = sector * flashSectorSize();
		bAHI_FullFlashRead(pos, 1, &b);
		while (b == STORE_LIVERECORD)
		{
		//	dbgPrintf(".");

			bAHI_FullFlashRead(pos + 1, 4, (uint8*) &tversion);
			bAHI_FullFlashRead(pos + PAGEWORDLEN, 4, (uint8*) &len);

			if(len==0xffff || len==0)
			{
				//incomplete record
				dbgPrintf(" incomplete %d ",len);

				break;
			}
			else
			{
				if(tversion>version)
				{
					version=tversion;
					mostRecent=pos;
					mostRecentLen=len;
				}
			}
			pos += len;
			if (pos >= (sector + 1) * flashSectorSize())
				break;
			bAHI_FullFlashRead(pos, 1, &b);
		}
	//	dbgPrintf("-");

	}
	if(mostRecent>0)
	{
		s->readPos = mostRecent + 2*PAGEWORDLEN;
		s->base = mostRecent;
		s->size = mostRecentLen;
		s->version = version;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
void flashOpenWrite(flashFile* s,uint32 filename)
{
	flashFile os;
	int pos;
	uint32 version=1;
	if(flashOpenRead(&os,filename) == FALSE)
	{
		//nothing stored - wipe to be sure
		bAHI_FlashEraseSector(flashNumSectors()-2);
		bAHI_FlashEraseSector(flashNumSectors()-1);
		pos = flashSectorSize()*(flashNumSectors()-1);
	//	dbgPrintf(" wipe ");

	}
	else
	{
		version=os.version+1;

		//check for room or faulty record at end of current one
		//allow new record to be twice as big as old one
		pos = os.base + os.size;
		int sector = pos / flashSectorSize();
		uint8 b;
		bAHI_FullFlashRead(pos, 1, &b);

		//swap sectors if no room or faulty record
		if (pos + os.size * 2 >= (sector + 1) * flashSectorSize() || b != STORE_NORECORD)
		{
			if (sector == 2)
			{
				pos = flashSectorSize()*(flashNumSectors()-1);
				bAHI_FlashEraseSector(flashNumSectors()-1);
			}
			else
			{
				pos = flashSectorSize()*(flashNumSectors()-2);
				bAHI_FlashEraseSector(flashNumSectors()-2);

			}
		}
	}
	s->base = pos;
	s->flashWriteAddr = pos;
	flashWriteUint8(s, STORE_LIVERECORD);
	s->version=version;
	flashWriteUint32(s,version);
	//make room for length to be stored later in its own pageword
	//flashWriteSeek(s,PAGEWORDLEN*2-(s->flashWriteAddr-s->base));
	flashAbsWriteSeek(s,PAGEWORDLEN*2);

}
void flashAbsWriteSeek(flashFile* s,int i)
{
#ifdef JN5168
	flushFlashBuffer(s);
#endif
	s->flashWriteAddr=s->base+i;
}
void flashWriteSeek(flashFile* s,int i)
{
#ifdef JN5168
	if(s->flashWriteBuffPos>0)
	{
		i=i-(PAGEWORDLEN-s->flashWriteBuffPos);
		flushFlashBuffer(s);
	}
#endif
	s->flashWriteAddr+=i;
}
void flashReadSeek(flashFile* s,int i)
{
	s->readPos+=i;
}
void flashWrite(flashFile* s,uint8 *pu8Data,uint16 u16Len)
{
#ifdef JN5168
	while(u16Len>0)
	{
		s->flashWriteBuff[s->flashWriteBuffPos++]=*(pu8Data++);
		u16Len--;
		if(s->flashWriteBuffPos>=PAGEWORDLEN)flushFlashBuffer(s);
	}
#else
	bAHI_FullFlashProgram(s->flashWriteAddr, u16Len, pu8Data);
	s->flashWriteAddr+=u16Len;
//	dbgPrintf(" w %d %d ",*pu8Data,u16Len);

#endif
}
void flashRead(flashFile* s,uint8 *pu8Data,uint16 u16Len)
{
	bAHI_FullFlashRead(s->readPos, u16Len, pu8Data);
	s->readPos+=u16Len;
}

void flashClose(flashFile* s)
{
	//write length to file header
#ifdef JN5168
	flushFlashBuffer(s);
#endif
	uint32 len = s->flashWriteAddr-s->base;
	flashAbsWriteSeek(s,PAGEWORDLEN);
	flashWriteUint32(s,len);
#ifdef JN5168
	flushFlashBuffer(s);
#endif
//	dbgPrintf("close ");

}
//utility methods
void flashWriteUint8(flashFile* s,uint8 val)
{
	flashWrite(s,&val,1);
}
void flashWriteUint32(flashFile* s,uint32 val)
{
	flashWrite(s,(uint8*)&val,4);
}
void flashWriteInt32(flashFile* s,int val)
{
	flashWrite(s,(uint8*)&val,4);
}
uint8 flashReadUint8(flashFile* s)
{
	uint8 ret;
	flashRead(s,&ret,1);
	return ret;
}
uint32 flashReadUint32(flashFile* s)
{
	uint32 ret;
	flashRead(s,(uint8*)&ret,4);
	return ret;
}
int flashReadInt32(flashFile* s)
{
	int ret;
	flashRead(s,(uint8*)&ret,4);
	return ret;
}
//strings prefixed with 32 bit length
void flashWriteString(flashFile* s,char* str)
{
	size_t len=strlen(str);
	flashWriteUint32(s,len);
	flashWrite(s,(uint8*)str,len);
}
uint32 flashReadString(flashFile* s,char* str,uint32 buflen)
{
	uint32 len=flashReadUint32(s);
	len=MIN(len,buflen-1);
	flashRead(s,(uint8*)str,len);
	str[len]='\0';
	return len;
}
