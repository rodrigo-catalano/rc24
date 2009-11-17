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

The is a scheme to allow messages to be passed through a series of nodes.
msg structure

byte 0  bits 4-7 length of address not including this byte
		bits 0-3 index of 'to' part of address
byte 1-n 1 byte per hop

message content

*/
#include <jendefs.h>

#include <string.h>
#include "routedmessage.h"

uint8 rmBuildReturnRoute(uint8* inbuf, uint8* outbuf)
{
    uint8 addrLen=inbuf[0]>>4;
    outbuf[0]=inbuf[0]&0xf0;
    int i;
    //reverse address
    for(i=0;i<addrLen;i++)outbuf[i+1]=inbuf[addrLen-i];
    return addrLen+1;

}
uint8 rmBuildRelayRoute(uint8* msg,uint8 fromCon)
{
	  uint8 addrToIdx=msg[0]&0x0f;
      uint8 to=msg[addrToIdx+1];
      //replace to with from
      msg[addrToIdx+1]=fromCon;
      //move index of next to address
      addrToIdx++;
      msg[0]=(msg[0]&0xf0)+addrToIdx;
      return to;
}
bool rmIsMessageForMe(uint8* msg)
{
    uint8 addrLen=msg[0]>>4;
    uint8 addrToIdx=msg[0]&0x0f;

     //see if packet has reached its destination
    if(addrToIdx==addrLen)//no to addresses
    {
    	return TRUE;
    }
    else return FALSE;

}
void rmGetPayload(uint8* msg,uint8 len,uint8** msgBody, uint8* msgLen)
{
	uint8 totalAddrLen=(msg[0]>>4)+1;
	*msgBody=msg+totalAddrLen;
	*msgLen=len-totalAddrLen;
}
/****************************************************************************
 *
 * NAME: rmWriteEncodedRoute
 *
 * DESCRIPTION: Write a route to a message buffer with first step removed
 *
 * PARAMETERS:      Name            RW  Usage
 *  				buf				W 	output buffer
 *  				r				R   route to write
 *
 * RETURNS: uint8 number of bytes written
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/
uint8 rmWriteEncodedRoute(uint8* buf,route* r)
{
	uint8 len=r->length;
	memcpy(buf+1,r->routeNodes+1,len-1);
	buf[0]=(len-1)<<4;
	return len;
}



