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

#ifndef _ROUTED_OBJECT_H
#define _ROUTED_OBJECT_H

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>


typedef struct _routedObject routedObject;

// definiton of function that can receive a routed message for a routedObject
typedef void (*COMMS_OBJECT_CALLBACK_FN)(routedObject* obj,uint8* msg,uint8 len,uint8 fromCon);


typedef struct
{
	routedObject* obj;  //the remote object
	uint8 addr; //the address in the remote object that we are connected to
}routedConnection;



struct _routedObject
{
	routedConnection* connections;
	int nConnections;
	ccParameterList parameters;
	COMMS_OBJECT_CALLBACK_FN messageHandler;
	char* name;
	uint32 version;
};

void routedObjectHandleMessage(routedObject* obj,uint8* msg, uint8 len, uint8 fromCon);
void routedObjectConnect(routedObject* obj);

uint8 ccSetObjectParameter(routedObject* obj, uint8* inMsg, uint8* outMsg);
uint8 ccGetObjectParameter(routedObject* obj, uint8* inMsg, uint8* outMsg);
uint8 ccEnumGroupObjectCommand(routedObject* obj, uint8* inMsg, uint8* outMsg);

void ccSetObjectParameterAsNum (routedObject* obj, int index,int value,int arrayIndex);
int ccGetObjectParameterAsNum(routedObject* obj, int index,int arrayIndex);

#if defined __cplusplus
}
#endif
#endif
