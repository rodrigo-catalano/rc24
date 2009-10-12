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


typedef void (*COMMS_CALLBACK_FN)(uint8* msg,uint8 len,uint8 fromCon);

typedef struct
{
    uint8  connector_id;
    uint8  comm_port;
    COMMS_CALLBACK_FN callback;

}pcCom;

void initPcComs(pcCom* com,uint8 id,uint8 port,COMMS_CALLBACK_FN cb);
void pcComsSendPacket(uint8 buff[], int start, uint8 length, uint8 cmd);
void pcComsSendPacket2(uint8 buff[], int start, uint8 length, uint8 cmd,uint8 cmd2);
uint8* getUploadBuffer(int* len);
void pcComsPrintf(const char *fmt, ...);
PUBLIC void vFifoPutC(unsigned char c);

