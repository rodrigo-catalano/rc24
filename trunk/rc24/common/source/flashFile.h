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

#ifndef _FLASH_FILE_H
#define _FLASH_FILE_H

#if defined __cplusplus
extern "C" {
#endif


#include <jendefs.h>

#ifdef JN5168
#define PAGEWORDLEN 16
#else
#define PAGEWORDLEN 16
#endif


typedef struct
{
    uint32 flashWriteAddr;
    uint32 readPos;
    uint32 version;
    uint32 base;
    uint32 size;
#ifdef JN5168
    uint8 flashWriteBuff[PAGEWORDLEN];
    uint8 flashWriteBuffPos;
#endif
}flashFile;

bool flashOpenRead(flashFile* s,uint32 filename);
void flashOpenWrite(flashFile* s,uint32 filename);
void flashClose(flashFile* s);
void flashWrite(flashFile* s,uint8 *pu8Data,uint16 u16Len);
void flashRead(flashFile* s,uint8 *pu8Data,uint16 u16Len);
void flashWriteSeek(flashFile* s,int i);
void flashAbsWriteSeek(flashFile* s,int i);
void flashReadSeek(flashFile* s,int i);

void flashWriteUint8(flashFile* s,uint8 val);
void flashWriteUint32(flashFile* s,uint32 val);
void flashWriteInt32(flashFile* s,int val);
uint8 flashReadUint8(flashFile* s);
uint32 flashReadUint32(flashFile* s);
int flashReadInt32(flashFile* s);
void flashWriteString(flashFile* s,char* str);
uint32 flashReadString(flashFile* s,char* str,uint32 buflen);


#if defined __cplusplus
}
#endif
#endif
