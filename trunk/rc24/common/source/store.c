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

//todo allow for bigger sector size on JN5148

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <string.h>

#include "store.h"
void storeStartSection(store* parent,uint8 tag,store* section)
{
    section->base=parent->currentPos;
    section->currentPos=parent->currentPos;
    writeUint8(section,tag);
    section->currentPos+=2;
}
uint16 storeEndSection(store* parent,store* section)
{
    uint16 len=section->currentPos-section->base;
    bAHI_FullFlashProgram(section->base+1,2,(uint8*)&len);
    parent->currentPos=section->currentPos;
    return len;
}
bool storeFindSection(store* parent,uint8 tag,store* result)
{
    int ftag;
    while ((ftag=storeGetSection(parent,result))>0)
    {
        if(ftag==tag)
        {
             return TRUE;
        }
    }
    return FALSE;
}
int storeGetSection(store* parent, store* section)
{
    if(parent->currentPos >= parent->base + parent->size)return -1;
    section->base=parent->currentPos;
    section->currentPos=parent->currentPos;
    int tag;
    tag=readUint8(section);
    uint8 type=(tag&0xc0)>>6;
    switch(type)
    {
         case 0: section->size=readInt16(section);break;
         case 1: section->size=2;break;
         case 2: section->size=3;break;
         case 3: section->size=5;break;
    }
    //move parent to end of tag
    parent->currentPos+=section->size;
    return tag;
}
void storeInt16Section(store* s,uint8 tag,int16 val)
{
    writeUint8(s,tag|STOREINT16);
    writeInt16(s,val);
}
void storeInt32Section(store* s,uint8 tag,int32 val)
{
    writeUint8(s,tag|STOREINT32);
    writeInt32(s,val);
}
void storeUint8Section(store* s,uint8 tag,uint8 val)
{
    writeUint8(s,tag|STOREINT8);
    writeUint8(s,val);
}

uint16 writeInt32(store* s,int32 val)
{
    bAHI_FullFlashProgram(s->currentPos,4,(uint8*)&val);
    s->currentPos+=4;
    return 4;
}
int32 readInt32(store* s)
{
    int32 val;
    bAHI_FullFlashRead(s->currentPos,4,(uint8*)&val);
    s->currentPos+=4;
    return val;
}
uint16 writeInt16(store* s,int16 val)
{
    bAHI_FullFlashProgram(s->currentPos,2,(uint8*)&val);
    s->currentPos+=2;
    return 2;
}
int16 readInt16(store* s)
{
    int16 val;
    bAHI_FullFlashRead(s->currentPos,2,(uint8*)&val);
    s->currentPos+=2;
    return val;
}
uint16 writeUint8(store* s,uint8 val)
{
    bAHI_FullFlashProgram(s->currentPos,1,(uint8*)&val);
    s->currentPos++;
    return 1;
}
uint8 readUint8(store* s)
{
    uint8 val;
    bAHI_FullFlashRead(s->currentPos,1,(uint8*)&val);
    s->currentPos++;
    return val;
}
uint16 writeString(store* s,char* val)
{
    int len=strlen(val);
    writeUint8(s,(uint8)len);
    bAHI_FullFlashProgram(s->currentPos,len,(uint8*)val);
    s->currentPos+=len;
    return len+1;
}
void readString(store* s,char* val, uint16 maxlen)
{
    uint8 len=readUint8(s);
    uint16 rlen=len;
    if(rlen>maxlen)rlen=maxlen;
    bAHI_FullFlashRead(s->currentPos,rlen,(uint8*)val);
    *(val+len)='\0';
    s->currentPos+=len;

}
uint16 writeBuffer(store* s,uint8* val,uint16 len)
{
    bAHI_FullFlashProgram(s->currentPos,len,(uint8*)val);
    s->currentPos+=len;
    return len;
}
void readBuffer(store* s,char* val, uint16 len)
{

    bAHI_FullFlashRead(s->currentPos,len,(uint8*)val);
    s->currentPos+=len;

}


//alternate storage between top two sectors so it is never only in memory
//this also means we don't need to be able to fit full store in ram
//avoid erasing too often by marking records as old
//we can set bits from 1 to 0 without erase
//11111111b = no record
//01111111b = half written record
//00111111b = live record
//00011111b = old record
#define FLASHSECTOR2 (0x8000*2)
#define FLASHSECTOR3 (0x8000*3)

#define STORE_NORECORD 255
#define STORE_PENDINGRECORD 127
#define STORE_LIVERECORD 63
#define STORE_OLDRECORD 31

// records have above byte header then uint16 length so we can jump
// old records fairly quickly.
// length includes the whole header


void getNewStore(store* s)
{
    store os;
    int pos;
    if(getOldStore(&os)==FALSE)
    {
        //nothing stored - wipe to be sure
        bAHI_FlashEraseSector(2);
        bAHI_FlashEraseSector(3);
        pos=FLASHSECTOR3;

    }
    else
    {
        //check for room or faulty record at end of current one
        //allow new record to be twice as big as old one
        pos=os.base+os.size;
        int sector=pos/0x8000;
        uint8 b;
        bAHI_FullFlashRead(pos,1,&b);
        //swap sectors if no room or faulty record
        if( pos+os.size*2 >= (sector+1)*0x8000  || b!=STORE_NORECORD)
        {
             if(sector==2)
             {
                  pos=FLASHSECTOR3;
                  bAHI_FlashEraseSector(3);
             }
             else
             {
                pos=FLASHSECTOR2;
                bAHI_FlashEraseSector(2);

             }
        }
    }
    s->base=pos;
    s->currentPos=pos;
    writeUint8(s,STORE_PENDINGRECORD);
    s->currentPos+=2;
}
void commitStore(store* s)
{
    store os;
    bool osvalid;
    //set length
    uint16 len=s->currentPos - s->base;
    bAHI_FullFlashProgram(s->base+1,2,(uint8*)&len);

    osvalid=getOldStore(&os);

    //set to new
    s->currentPos=s->base;
    writeUint8(s,STORE_LIVERECORD);
    //mark last record as old
    //if an error / power down happens here there could be two records marked as new
    //the first one in flash would be picked
    if(osvalid)
    {
        os.currentPos=os.base;
        writeUint8(&os,STORE_OLDRECORD);
    }
}
bool getOldStore(store* s)
{
    uint8 b;
    int sector;
    int pos;
    uint16 len;
    for(sector=2;sector<=3;sector++)
    {
        pos=sector*0x8000;
        bAHI_FullFlashRead(pos,1,&b);
        while(b!=STORE_NORECORD)
        {
            bAHI_FullFlashRead(pos+1,2,(uint8*)&len);
            if(b==STORE_LIVERECORD)
            {
                s->currentPos=pos+3;
                s->base=pos;
                s->size=len;
                return TRUE;
            }
            else
            {
                pos+=len;
                if(pos>=(sector+1)*0x8000)break;
                bAHI_FullFlashRead(pos,1,&b);
            }
        }
    }
    return FALSE;
}


