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

/*
 flash storage code
 This tries to keep flash wipes to a minimum by marking old records as old and moving on to
 the next clear section.
 It ensures that even during a write a complete copy of settings is on flash.
 This is done by alternating between two erasable blocks of flash.

 The data is stored in a crude sort of binary xml
 elements are identified by the low 6 bits of the first byte so a given element can have 64 unique
 child elements.
 The length of each element is stored.

 The top two bits of the first byte identify the data type, this is to reduce the storage overhead
 for the most common types

 01 - uint8 - packet length is inferred
 10 - int16 - packet length is inferred
 11 - int32 - packet length is inferred
 00 - anything else - the following uint16 is the element length

 The tagged hierarchical structure was used to ease reading records from
 previous versions as the software changes


 */
/* this all got rather messed up in trying to make it work with the different flash on the 5168
 * in the receiver code it has been replaced with much neater code in objectstore
 * the tx code still needs to be changed to use objectstore
 */

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <string.h>

#include "hwutils.h"

#include "store.h"



#ifdef JN5168
void flushFlashBuffer(store* s)
{
	if(s->flashWriteBuffPos>0)
	{
		bAHI_FullFlashProgram(s->flashWriteAddr, s->flashWriteBuffPos, (uint8*) s->flashWriteBuff);
		s->flashWriteAddr+=PAGEWORDLEN;
		s->flashWriteBuffPos=0;
	}
}
#endif
void flashOpen(store* s,uint32 u32Addr)
{
	s->flashWriteAddr=u32Addr;
#ifdef JN5168
	s->flashWriteBuffPos=0;
#endif
}
void flashWriteSeek(store* s,int i)
{
#ifdef JN5168
	flushFlashBuffer(s);
#endif
	s->flashWriteAddr+=i;
}
void flashWrite(store* s,uint16 u16Len,uint8 *pu8Data)
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
#endif
}

uint32 flashClose(store* s)
{
	return s->flashWriteAddr;
}






void storeStartSection(store* parent, uint8 tag, store* section)
{
	section->base = parent->currentPos;
	section->flashWriteAddr=parent->flashWriteAddr;
	section->currentPos = parent->currentPos;
	writeUint8(section, tag);
	section->currentPos += 2;
	section->flashWriteAddr+=2;

#ifdef JN5168
	//TODO make work on 5168 or swap to object store
#endif
}
uint16 storeEndSection(store* parent, store* section)
{
	uint16 len = section->currentPos - section->base;
//	bAHI_FullFlashProgram(section->base + 1, 2, (uint8*) &len);
	parent->currentPos = section->currentPos;
	parent->flashWriteAddr=section->flashWriteAddr;
	section->flashWriteAddr=section->base + 1;
	flashWrite(section,2, (uint8*) &len);

	return len;
}
bool storeFindSection(store* parent, uint8 tag, store* result)
{
	int ftag;
	while ((ftag = storeGetSection(parent, result)) > 0)
	{
		if (ftag == tag)
		{
			return TRUE;
		}
	}
	return FALSE;
}
int storeGetSection(store* parent, store* section)
{
	if (parent->currentPos >= parent->base + parent->size)
		return -1;
	section->base = parent->currentPos;
	section->currentPos = parent->currentPos;
	int tag;
	tag = readUint8(section);
	uint8 type = (tag & 0xc0) >> 6;
	switch (type)
	{
	case 0:
		section->size = readInt16(section);
		break;
	case 1:
		section->size = 2;
		break;
	case 2:
		section->size = 3;
		break;
	case 3:
		section->size = 5;
		break;
	}
	//move parent to end of tag
	parent->currentPos += section->size;
	return tag;
}
void storeInt16Section(store* s, uint8 tag, int16 val)
{
	writeUint8(s, tag | STOREINT16);
	writeInt16(s, val);
}
void storeInt32Section(store* s, uint8 tag, int32 val)
{
	writeUint8(s, tag | STOREINT32);
	writeInt32(s, val);
}
void storeUint8Section(store* s, uint8 tag, uint8 val)
{
	writeUint8(s, tag | STOREINT8);
	writeUint8(s, val);
}

uint16 writeInt32(store* s, int32 val)
{
	//bAHI_FullFlashProgram(s->currentPos, 4, (uint8*) &val);
	flashWrite(s,4, (uint8*) &val);
	s->currentPos += 4;
	return 4;
}
int32 readInt32(store* s)
{
	int32 val;
	bAHI_FullFlashRead(s->currentPos, 4, (uint8*) &val);
	s->currentPos += 4;
	return val;
}
uint16 writeUint32(store* s, uint32 val)
{
	//bAHI_FullFlashProgram(s->currentPos, 4, (uint8*) &val);
	flashWrite(s,4, (uint8*) &val);
	s->currentPos += 4;
	return 4;
}
uint16 writeInt16(store* s, int16 val)
{
	//bAHI_FullFlashProgram(s->currentPos, 2, (uint8*) &val);
	flashWrite(s, 2, (uint8*) &val);
	s->currentPos += 2;
	return 2;
}
int16 readInt16(store* s)
{
	int16 val;
	bAHI_FullFlashRead(s->currentPos, 2, (uint8*) &val);
	s->currentPos += 2;
	return val;
}
uint16 writeUint8(store* s, uint8 val)
{
	//bAHI_FullFlashProgram(s->currentPos, 1, (uint8*) &val);
	flashWrite(s,1, (uint8*) &val);
	 s->currentPos++;
	return 1;
}
uint8 readUint8(store* s)
{
	uint8 val;
	bAHI_FullFlashRead(s->currentPos, 1, (uint8*) &val);
	s->currentPos++;
	return val;
}
uint16 writeString(store* s, char* val)
{
	int len = strlen(val);
	writeUint8(s, (uint8) len);
	//bAHI_FullFlashProgram(s->currentPos, len, (uint8*) val);
	flashWrite(s,len, (uint8*) val);
	s->currentPos += len;
	return len + 1;
}
void readString(store* s, char* val, uint16 maxlen)
{
	uint8 len = readUint8(s);
	uint16 rlen = len;
	if (rlen > maxlen)
		rlen = maxlen;
	bAHI_FullFlashRead(s->currentPos, rlen, (uint8*) val);
	*(val + len) = '\0';
	s->currentPos += len;

}
uint16 writeBuffer(store* s, uint8* val, uint16 len)
{
//	bAHI_FullFlashProgram(s->currentPos, len, (uint8*) val);
	flashWrite(s,len, (uint8*) val);
	s->currentPos += len;
	return len;
}
void readBuffer(store* s, char* val, uint16 len)
{

	bAHI_FullFlashRead(s->currentPos, len, (uint8*) val);
	s->currentPos += len;

}

//alternate storage between top two sectors so it is never only in memory
//this also means we don't need to be able to fit full store in ram
//avoid erasing too often by marking records as old
//we can set bits from 1 to 0 without erase except on JN5168
//11111111b = no record
//01111111b = half written record
//00111111b = live record
//00011111b = old record

#define STORE_NORECORD 255
#define STORE_PENDINGRECORD 127
#define STORE_LIVERECORD 63
#define STORE_OLDRECORD 31

// records have above byte header then uint16 length so we can jump
// old records fairly quickly.
// length includes the whole header

/* JN5168 has blocks of 16 bytes that should not be overwritten
 * so write STORE_LIVERECORD and version to first 16byte block
 * then length  to second block at commit
 * a record is incomplete if no length stored
 *
 *
 *
 *
 */
#ifdef JN5168
void getNewStore(store* s)
{
	store os;
	int pos;
	uint32 version=1;
	if (getOldStore(&os) == FALSE)
	{
		//nothing stored - wipe to be sure
		bAHI_FlashEraseSector(flashNumSectors()-2);
		bAHI_FlashEraseSector(flashNumSectors()-1);
		pos = flashSectorSize()*(flashNumSectors()-1);
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
	flashOpen(pos);
	s->base = pos;
	s->currentPos = pos;
	writeUint8(s, STORE_LIVERECORD);
	s->version=version;
	writeUint32(s,version);
	flashWriteSeek(PAGEWORDLEN*2-(s->currentPos-s->base));
	s->currentPos = s->base+2*PAGEWORDLEN;

}

#else

void getNewStore(store* s)
{
	store os;
	int pos;
	if (getOldStore(&os) == FALSE)
	{
		//nothing stored - wipe to be sure
		bAHI_FlashEraseSector(flashNumSectors()-2);
		bAHI_FlashEraseSector(flashNumSectors()-1);
		pos = flashSectorSize()*(flashNumSectors()-1);
		pcComsPrintf("erased %d %d\r\n", flashNumSectors()-2,flashNumSectors()-1);

	}
	else
	{
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
	flashOpen(s,pos);
	s->base = pos;
	s->currentPos = pos;
	writeUint8(s, STORE_PENDINGRECORD);
	s->currentPos += 2;
	s->flashWriteAddr +=2;
}
#endif
#ifdef JN5168
void commitStore(store* s)
{
	// writing the length value marks the file as complete
	uint32 len=flashClose();

	flashWriteSeek(s->base+PAGEWORDLEN);
	writeInt32(s,len);
	flashClose();

}
#else
void commitStore(store* s)
{
	store os;
	bool osvalid;
	//set length
//	uint32 len=flashClose();

	uint16 len = s->currentPos - s->base;


	bAHI_FullFlashProgram(s->base + 1, 2, (uint8*) &len);

	osvalid = getOldStore(&os);

	//set to new
	s->currentPos = s->base;
	s->flashWriteAddr = s->base;
	writeUint8(s, STORE_LIVERECORD);
	//mark last record as old
	//if an error / power down happens here there could be two records marked as new
	//the first one in flash would be picked
	if (osvalid)
	{
		os.currentPos = os.base;
		os.flashWriteAddr = os.base;
		writeUint8(&os, STORE_OLDRECORD);
	}
}
#endif

#ifdef JN5168

bool getOldStore(store* s)
{
	uint8 b;
	int sector;
	uint32 pos=0;
	uint16 len;
	uint32 version=0;
	uint32 tversion;
	uint32 mostRecent=0;
	//find record with newest version

	for (sector = flashNumSectors()-2; sector <= flashNumSectors()-1; sector++)
	{
		pos = sector * flashSectorSize();
		bAHI_FullFlashRead(pos, 1, &b);
		while (b != STORE_LIVERECORD)
		{

			bAHI_FullFlashRead(pos + 1, 4, (uint8*) &tversion);
			bAHI_FullFlashRead(pos + PAGEWORDLEN, 2, (uint8*) &len);

			if(len==0xffff)
			{
				//incomplete record
				break;
			}
			else
			{
				if(tversion>version)
				{
					version=tversion;
					mostRecent=pos;
				}

			}
			pos += len;
			if (pos >= (sector + 1) * flashSectorSize())
				break;
			bAHI_FullFlashRead(pos, 1, &b);
		}
	}
	if(mostRecent>0)
	{
		s->currentPos = pos + 2*PAGEWORDLEN;
		s->base = pos;
		s->size = len;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


#else
bool getOldStore(store* s)
{
	uint8 b;
	int sector;
	int pos;
	uint16 len;
	for (sector = flashNumSectors()-2; sector <= flashNumSectors()-1; sector++)
	{
		pos = sector * flashSectorSize();
		bAHI_FullFlashRead(pos, 1, &b);
		while (b != STORE_NORECORD)
		{
			bAHI_FullFlashRead(pos + 1, 2, (uint8*) &len);
			if (b == STORE_LIVERECORD)
			{
				s->currentPos = pos + 3;
				s->base = pos;
				s->size = len;
				return TRUE;
			}
			else
			{
				pos += len;
				if (pos >= (sector + 1) * flashSectorSize())
					break;
				bAHI_FullFlashRead(pos, 1, &b);
			}
		}
	}
	return FALSE;
}
#endif
