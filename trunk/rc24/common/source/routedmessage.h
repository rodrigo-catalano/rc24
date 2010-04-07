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

// definiton of function that can receive a routed message
typedef void (*COMMS_CALLBACK_FN)(uint8* msg,uint8 len,uint8 fromCon);


//this defines a full route including the first step that is not included
//in a routed message packet
typedef struct
{
	uint8 length;
	uint8 routeNodes[];
}route;

uint8 rmBuildReturnRoute(uint8* inbuf, uint8* outbuf);
uint8 rmBuildRelayRoute(uint8* msg,uint8 fromCon);
bool rmIsMessageForMe(uint8* msg);
void rmGetPayload(uint8* msg,uint8 len,uint8** msgBody, uint8* msgLen);
uint8 rmWriteEncodedRoute(uint8* buf,route* r);
