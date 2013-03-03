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


#include <jendefs.h>
#include <AppHardwareApi.h>
#include <stdlib.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <stdio.h>

#include "swEventQueue.h"
#include "routedmessage.h"
#include "commonCommands.h"
#include "routedObject.h"

#include "oneWireBus.h"

// TODO use fifo properly

PRIVATE void oneWire_HandleUart1Interrupt(uint32 u32Device, uint32 u32ItemBitmap);



//buffers to receive packets from uart that will be passed to app context
uint8 oneWireComsBuffers[3][256];
int oneWireComsCurrentBuf=0;
int oneWireComsUploadLen=0;
uint8 oneWireComsChecksum=0;


//circular buffer for uart tx
uint8 oneWireComsTxBuffer[256];
uint8 oneWireComsWriteIdx=0;
uint8 oneWireComsReadIdx=0;
volatile uint8 oneWireComsTxLock=0;


bool ignoreEcho=FALSE;


PRIVATE ccParameter oneWireExposedParameters[]=
{
	{ "oneWire Crc Errors",CC_UINT32,NULL,offsetof(oneWireBus,oneWireCrcErrors),0,CC_NO_GETTER,CC_NO_SETTER },
	{ "oneWire Packets Sent",CC_UINT32,NULL,offsetof(oneWireBus,oneWirePacketsSent),0,CC_NO_GETTER,CC_NO_SETTER },
	{ "oneWire Packets Received",CC_UINT32,NULL,offsetof(oneWireBus,oneWirePacketsReceived),0,CC_NO_GETTER,CC_NO_SETTER },
};

PRIVATE ccParameterList oneWireParameterList=
{
		oneWireExposedParameters,
		sizeof(oneWireExposedParameters)/sizeof(oneWireExposedParameters[0])
};



oneWireBus* oneWirePortToIdx[2];

PRIVATE void vFifoPutCf(unsigned char c)
{
    //wait for tx fifo empty (bit 5 set in LSR when empty)
    while ((u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_THRE ) == 0);
    // ..and send the character
    vAHI_UartWriteData(E_AHI_UART_0,c);

}


PRIVATE void vFifoPutC(unsigned char c)
{
	oneWireComsTxLock=1;
	oneWireComsTxBuffer[oneWireComsWriteIdx++]=c;

  if((u8AHI_UartReadLineStatus(E_AHI_UART_1) & E_AHI_UART_LS_THRE ) != 0)
  {
     vAHI_UartWriteData(E_AHI_UART_1,oneWireComsTxBuffer[oneWireComsReadIdx++]);
  }

  oneWireComsTxLock=0;
}
void createOneWireBus(oneWireBus* com)
{
	com->ro.parameters=oneWireParameterList;
	com->ro.messageHandler=&routedObjectHandleMessage;
}
void initOneWireBus(oneWireBus* com,uint8 id,uint8 port,COMMS_CALLBACK_FN cb)
{
	com->connector_id=id;
	com->comm_port=port;
	com->callback=cb;

	//map uart to structure
	oneWirePortToIdx[port]=com;

    /* Enable UART 0: 125000-8-N-1 */

	vAHI_UartSetRTSCTS(port, FALSE);

	vAHI_UartEnable(port);

    vAHI_UartReset(port, TRUE, TRUE);
    vAHI_UartReset(port, FALSE, FALSE);

    vAHI_UartSetBaudDivisor(port,1000000/125000);

    vAHI_UartSetControl(port, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);

    if(port==E_AHI_UART_0) vAHI_Uart0RegisterCallback(oneWire_HandleUart1Interrupt);
    else  vAHI_Uart1RegisterCallback(oneWire_HandleUart1Interrupt);

       vAHI_UartSetInterrupt(port,
        FALSE,
        FALSE,
        TRUE,//tx fifo empty
        TRUE,//rx data
        E_AHI_UART_FIFO_LEVEL_1);

       com->enabled=TRUE;
}
//message handler for sensor proxy - passes message from onewire object to sensor
void oneWireSendToBus(routedObject* obj,uint8* msg,uint8 len,uint8 fromCon)
{
	//this isn't a one wire object, it is a sensor proxy
	//as such it should have the to address
//	oneWireSendPacket((oneWireBus*)obj,msg, 0, length, uint8 addr);
}

void oneWireSendPacket(oneWireBus* me,uint8 buff[], int start, uint8 length, uint8 addr)
{
   ignoreEcho=TRUE;
   int i;
   uint8 checksum=0;
   vFifoPutC(length+2);
   vFifoPutC(addr);
   checksum^=length+2;
   checksum^=addr;
   for(i=0;i<length;i++)
   {
       uint8 b=buff[start+i];
       vFifoPutC(b);
       checksum^=b;
   }
   vFifoPutC(checksum);
   me->oneWirePacketsSent++;
}

void oneWireParsePacket(void*context,void* buff)
{
	oneWireBus* me=(oneWireBus*)context;

	me->oneWirePacketsReceived++;

    if(ignoreEcho==TRUE)
    {
    	ignoreEcho=FALSE;
    	return;
    }

	//runs in app not interrupt context

    uint8* packet=(uint8*)buff;
    uint8 len=packet[0];
    if(len>0 && packet[len]==0)
    {
    	//pass message from sensor to i wire object
    	//to address is the sensor id
    	(*me->ro.messageHandler)(&me->ro,&packet[2],len-2,packet[1]);

//    	oneWireHandleRoutedMessage(&packet[2],len-2,packet[1]);
    }
    else
    {
    	if(len>0&& packet[len]!=0)me->oneWireCrcErrors++;
    }
}

void oneWireSendRoutedMessage(oneWireBus* me,uint8* msg, uint8 len, uint8 toCon)
{
	// Send via appropriate channel
	if(toCon==0)
	{
		//send up to rx main node
		// TODO make this a settable callback connection ie fn* and conid
//		rxHandleRoutedMessage(msg, len, 4);
	}
	else
	{
		//send to one wire bus
		oneWireSendPacket(me,msg, 0, len, toCon);
	}

}
/*
void oneWireHandleRoutedMessage(uint8* msg, uint8 len, uint8 fromCon)
{
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
		case 0: // enumerate name and all children except fromCon
		{
			// TODO - replace magic number
			replyBody[replyLen++]  = 1;//enumerate response id
			replyLen+=ccWriteString(&replyBody[replyLen],"1 Wire");

			// TODO - ???
			// enumerate from cache
			uint8 i;
			//enable the first 7 addresses until we have a method for either sending
			//all 256 or only active ones

		//	for (i = 0; i < 8; i++)
		//	{
		//		if (i != fromCon)
		//			replyBody[replyLen++] = i;
		//	}

			break;
		}
		case CMD_ENUM_GROUP :
			replyLen=ccEnumGroupCommand(&oneWireParameterList, msgBody, replyBody);
			break;
		case CMD_SET_PARAM:
			replyLen=ccSetParameter(&oneWireParameterList, msgBody, replyBody);
			break;
		case CMD_GET_PARAM:
			replyLen=ccGetParameter(&oneWireParameterList, msgBody, replyBody);
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
			oneWireSendRoutedMessage(replyBuf, replyLen + addrLen, fromCon);
		}
	}
	else
	{
		// Not for us, relay message
		// Swap last 'to' to 'from' in situ
		uint8 toCon = rmBuildRelayRoute(msg, fromCon);
		// Pass message on to connector defined by 'to' address
		oneWireSendRoutedMessage(msg, len, toCon);
	}
}
*/

PRIVATE void oneWire_HandleUart1Interrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    // handle communication with pc

    if (u32Device == E_AHI_DEVICE_UART1)
    {
        if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_RXDATA
            || (u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_TIMEOUT)
        {
            uint8 b=u8AHI_UartReadData(E_AHI_UART_1);
            oneWireComsBuffers[oneWireComsCurrentBuf][oneWireComsUploadLen++]=b;
            if(oneWireComsUploadLen>=255)
            {
            	oneWireComsUploadLen=0;
            	oneWireComsChecksum=0;
            }

            if(oneWireComsUploadLen>0)
            {
                //len cmd cs 2 37 cs
                if( oneWireComsUploadLen<=oneWireComsBuffers[oneWireComsCurrentBuf][0])
                {
                	oneWireComsChecksum^=b;
                }
                else
                {
                    //set checksum to 0 if ok
                	oneWireComsBuffers[oneWireComsCurrentBuf][oneWireComsUploadLen-1]-=oneWireComsChecksum;
                    // post to app context
                	//TODO MUST pass context
                    swEventQueuePush(oneWireParsePacket,NULL,&oneWireComsBuffers[oneWireComsCurrentBuf][0]);

                    //reset for next packet
                    oneWireComsUploadLen=0;
                    oneWireComsChecksum=0;
                    oneWireComsCurrentBuf++;
                    if(oneWireComsCurrentBuf>=3)oneWireComsCurrentBuf=0;

                }
            }
        }
        if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_TX)
        {
            //write up to 16 bytes from circular buffer to uart

          //  uint8 queued=pcComsWriteIdx-pcComsReadIdx;
          //  if(queued>12)queued=12;
          //  while(queued>0)
          if(oneWireComsTxLock==0)
          {
            if(oneWireComsWriteIdx!=oneWireComsReadIdx)
            {
                vAHI_UartWriteData(E_AHI_UART_1,oneWireComsTxBuffer[oneWireComsReadIdx++]);
            //    queued--;
            }
          }
        }
    }
}
