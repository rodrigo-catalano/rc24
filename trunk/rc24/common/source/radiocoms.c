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
#include <string.h>
#include <jendefs.h>
#include "routedmessage.h"

#include "pccoms.h"

#include "radiocoms.h"

/* This handles low priority messages that are tagged on to the end
 * of the high priority servo and sensor messages
 * the low priority message can be longer than will fit into one radio
 * packet, they are split and spread over a number of packets. Each part has to
 * be acked before the next part is sent.
 *
 * because of the radio packet overhead it is more efficient to do this
 * than send extra packets for other data, it also neatly limits the bandwidth used.
 *
 */


RADIO_DATA_CALLBACK datacallback;

uint8 radioCommsConnectorID;

//only queue 1 message for now
uint8 lowPriorityQueue[256];
uint8 lpqLen=0;
uint8 lpqRead=0;
uint8 lpqSeqId=0;
uint8 lpqChunkId=0;
int sendlen=0;

//Receive buffer to put message back together again
uint8 lowPriorityBuffer[256];
uint8 lpbLen=0;
uint8 lpbWrite=0;
uint8 lpbSeqId=0;
uint8 lpbChunkId=0;
uint8 lastlpbSeqId=255;


//this queue should only be modified in app context
//it is not 'interrupt safe'
//queue up data to be tagged on to the end of realtime data packets
bool queueLowPriorityData(uint8* data, uint8 len)
{
    //is there room to queue
    if(lpqLen==0)
    {
        memcpy(lowPriorityQueue,data,len);
        lpqLen=len;
        lpqRead=0;
        return TRUE;
    }
    else
    {
    	//dbgPrintf(" lpbFull ");

    	return FALSE;
    }
}

uint8 appendLowPriorityData(uint8* buffer, uint8 maxlen)
{
    uint8 len=2;
    if(lpqLen==0) return 0;

    *buffer++ = lpqSeqId;
    *buffer++ = lpqChunkId;

    if(lpqChunkId==0) //start of packet
    {
        *buffer++ = lpqLen;
        len++;
    }
    sendlen=lpqLen-lpqRead;
    if(sendlen>maxlen-len)sendlen=maxlen-len;
    memcpy(buffer,&lowPriorityQueue[lpqRead],sendlen);
    return sendlen+len;

}
void ackLowPriorityData()
{
    if(lpqLen!=0 && sendlen!=0)
    {
        lpqChunkId++;
        lpqRead+=sendlen;
        //full message sent
        if(lpqLen==lpqRead)
        {
            lpqLen=0;
            lpqRead=0;
            lpqSeqId++;
            lpqChunkId=0;
            sendlen=0;
        }
    }
}
void handleLowPriorityData(uint8* buffer, uint8 len)
{
    uint8 rxSeqId=buffer[0];
    uint8 rxChunkId=buffer[1];
    uint8 headerLen=2;

    if(rxChunkId==0)//new message
    {
        lpbLen=buffer[2];
        lpbSeqId=rxSeqId;
        lpbChunkId=0;
        headerLen++;
        lpbWrite=0;
    }
    // check seq id is the same and chunk id is +1

    if(lpbSeqId==rxSeqId && rxChunkId<=lpbChunkId )
    {
    	//ignore repeated chunk
        if(rxChunkId==lpbChunkId)
        {

        	if(lpbWrite+len-headerLen<256)
            {

        		memcpy(&lowPriorityBuffer[lpbWrite],buffer+headerLen,len-headerLen);
                lpbWrite+=len-headerLen;
                lpbChunkId++;

                if(lpbWrite==lpbLen)
                {
                    // pass message on
                	// if same seq as last time ignore as message is a repeat.
                	// Probably caused by the ack not being received or processed.
                	// There is a 1 in 256 chance of the first message being
                	// ignored if either end resets. We could send a sacrificial
                	// message at start up.
                	if(lpbSeqId != lastlpbSeqId)
                	{
                		(*datacallback)(lowPriorityBuffer,lpbLen,radioCommsConnectorID);
                		lastlpbSeqId=lpbSeqId;
                	}
                    lpbWrite=0;
                    lpbLen=0;
                }
            }
            else
            {
                    //something has gone wrong and the buffer is full
                    lpbWrite=0;
                    lpbLen=0;
              }
        }
    }
    else //missing chunk discard buffer
    {
         lpbLen=0;
    }
}
void setRadioDataCallback(RADIO_DATA_CALLBACK cb, uint8 connector_id)
{
    datacallback=cb;
    radioCommsConnectorID=connector_id;
}
