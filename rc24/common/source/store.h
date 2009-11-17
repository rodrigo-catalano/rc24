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


#define STOREINT8 0x40
#define STOREINT16 0x80
#define STOREINT32 0xc0

//tx store section defines
#define STOREGENERALSECTION 1
#define STOREDEFAULTINPUT (1 | STOREINT8)

#define STORERADIOSECTION 2
#define STORERADIOHIGHPOWER (1 | STOREINT8)

#define STOREMODELSSECTION 3
#define STORE_MODEL 1
#define STORE_LASTMODELIDX (2 | STOREINT16)


//rx store defines



typedef struct
{
    int currentPos;
    int base;
    int size;
}store;


void getNewStore(store* s);
void commitStore(store* s);
bool getOldStore(store* s);
void storeStartSection(store* s,uint8 tag,store* section);
uint16 storeEndSection(store* s,store* section);
int storeGetSection(store* parent, store* section);
bool storeFindSection(store* parent,uint8 tag,store* result);

uint16 writeInt32(store* s,int32 val);
int32 readInt32(store* s);
uint16 writeInt16(store* s,int16 val);
int16 readInt16(store* s);
uint16 writeUint8(store* s,uint8 val);
uint8 readUint8(store* s);
uint16 writeString(store* s,char* val);
void readString(store* s,char* val, uint16 maxlen);
uint16 writeBuffer(store* s,uint8* val,uint16 len);
void readBuffer(store* s,char* val, uint16 len);
void storeInt16Section(store* s,uint8 tag,int16 val);
void storeInt32Section(store* s,uint8 tag,int32 val);
void storeUint8Section(store* s,uint8 tag,uint8 val);




