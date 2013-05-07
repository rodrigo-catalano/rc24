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


#include <jendefs.h>
#include <string.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"

/****************************************************************************
 *
 * NAME: routedObjectHandleMessage
 *
 * DESCRIPTION: Handles standard messages sent to a routed object
 * 				can be overridden by setting the messageHandler
 * 				in the routedObject struct
 *
 * PARAMETERS:      Name            RW  Usage
 *  				paramList		RW	list of public properties
 *  				inMsg			R	incoming message
 *  				outMsg			w	return message
 *
 * RETURNS: uint8 length of return message 0 for no response
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

void routedObjectHandleMessage(routedObject* obj,uint8* msg, uint8 len, uint8 fromCon)
{
	//cope with standard message to routed object
	// TODO - make a definition for the buffer length
	uint8 replyBuf[256];
	// See if packet has reached its destination
	if (rmIsMessageForMe(msg) == TRUE)
	{
		// Message is for us - unwrap the payload
		uint8* msgBody;
		uint8 msgLen;
		rmGetPayload(msg, len, &msgBody, &msgLen);

		// Start building the reply, starting with the address
		uint8 addrLen = rmBuildReturnRoute(msg, replyBuf);
		uint8* replyBody = replyBuf + addrLen;
		uint8 replyLen = 0;

		// TODO - make the function code an enum
		switch (msgBody[0])
		{
		case 0: // enumerate name and ids of all children except fromCon
		{
			// TODO - replace magic number
			replyBody[replyLen++]  = 1;//enumerate response id
			replyLen+=ccWriteString(&replyBody[replyLen],obj->name);

			uint8 i;
			for (i = 0; i < obj->nConnections; i++)
			{
				if (i != fromCon)
					replyBody[replyLen++] = i;
			}
			break;
		}
		case CMD_ENUM_GROUP :
			replyLen=ccEnumGroupObjectCommand(obj, msgBody, replyBody);
			break;
		case CMD_SET_PARAM:
			replyLen=ccSetObjectParameter(obj, msgBody, replyBody);
			break;
		case CMD_GET_PARAM:
			replyLen=ccGetObjectParameter(obj, msgBody, replyBody);
			break;

		default:
		{
			//dbgPrintf("UnsupportedCmd %d ", msgBody[0]);
			break;
		}
		}

		// If there is a response, send it out
		if (replyLen > 0)
		{
			routedObject* dest=obj->connections[fromCon].obj;
			(*dest->messageHandler)(dest,replyBuf,replyLen+ addrLen,obj->connections[fromCon].addr);
		}
	}
	else
	{
		// Not for us, relay message
		// Swap last 'to' to 'from' in situ
		uint8 toCon = rmBuildRelayRoute(msg, fromCon);
		// Pass message on to connector defined by 'to' address
		routedObject* dest=obj->connections[toCon].obj;

		(*dest->messageHandler)(dest,msg,len,obj->connections[toCon].addr);

	}
}
void routedObjectBindParams(routedObject* obj)
{
	//calculate pointer for parameter that are part of object and only have an offset
	int i;
	for( i=0;i<obj->parameters.len;i++)
	{
		ccParameter* param = &obj->parameters.parameters[i];
		if(param->paramPtr==NULL && param->paramOffset>0)
		{
			param->paramPtr=((uint8*)obj)+param->paramOffset;
		}
	}
}

void routedObjectConnect(routedObject* obj)
{
	//set return address for all connections
	int i,n;
	for(i=0;i<obj->nConnections;i++)
	{
		//find which address obj is connected to on remote object
		routedObject* remoteObj=obj->connections[i].obj;
		for(n=0;n<remoteObj->nConnections;n++)
		{
			if(obj==remoteObj->connections[n].obj)
			{
				obj->connections[i].addr=n;
				break;
			}
		}
	}
	routedObjectBindParams(obj);
}



