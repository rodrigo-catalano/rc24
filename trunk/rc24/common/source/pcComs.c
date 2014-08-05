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
#include "routedmessage.h"
#include "pcComs.h"
#include "swEventQueue.h"
#include "hopping.h"


//todo switch to better framing with sync bytes
//higher baud rate
//make all packets routed and remove jennic bootloader stuff
//use fifo properly

PRIVATE void pcComs_HandleUart0Interrupt(uint32 u32Device, uint32 u32ItemBitmap);



//buffers to receive packets from uart that will be passed to app context
uint8 pcComsBuffers[3][256];
int pcComsCurrentBuf=0;
int pcComsUploadLen=0;
uint8 pcComsChecksum=0;

//jennic bootloader responses
uint8 pcComsAck[]={0};
uint8 pcComsCRCError[]={0xfc};

//buffer for radio programming of rx
uint8 uploadRxBuffer[256] __attribute__ ((aligned (4)));
uint8 uploadRxLen=0;
uint16 uploadPacketIndex=0;

//circular buffer for uart tx
uint8 pcComsTxBuffer[256];
uint8 pcComsWriteIdx=0;
uint8 pcComsReadIdx=0;
volatile uint8 pcComsTxLock=0;

uint32 pcComsCrcErrors=0;

uint8* getUploadBuffer(int* len)
{
    *(len)=uploadRxLen;
    return uploadRxBuffer;
}

pcCom* pcComsPortToIdx[2];

PUBLIC void vFifoPutCf(unsigned char c)
{
    //wait for tx fifo empty (bit 5 set in LSR when empty)
    while ((u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_THRE ) == 0);
    // ..and send the character
    vAHI_UartWriteData(E_AHI_UART_0,c);

}


PUBLIC void vFifoPutC(unsigned char c)
{
#ifdef JN5168
	vAHI_UartWriteData(E_AHI_UART_0,c);
#else
 // while ((u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_THRE ) == 0);

  pcComsTxLock=1;
  pcComsTxBuffer[pcComsWriteIdx++]=c;

  if((u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_THRE ) != 0)
  {
     vAHI_UartWriteData(E_AHI_UART_0,pcComsTxBuffer[pcComsReadIdx++]);
  }

  pcComsTxLock=0;

 // while(pcComsWriteIdx-pcComsReadIdx>250);
#endif
}

void initPcComs(pcCom* com,uint8 id,uint8 port,COMMS_CALLBACK_FN cb)
{
	com->connector_id=id;
	com->comm_port=port;
	com->callback=cb;

	pcComsPortToIdx[port]=com;

	//todo allow use of either or both uarts
#ifdef JN5168
	vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);
	bAHI_UartEnable(E_AHI_UART_0,pcComsTxBuffer,256,uploadRxBuffer,256);
	vAHI_UartSetBaudRate(E_AHI_UART_0,E_AHI_UART_RATE_38400);
	vAHI_UartSetControl(E_AHI_UART_0, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);
	vAHI_UartReset(E_AHI_UART_0, TRUE, TRUE);
//    vAHI_UartReset(E_AHI_UART_0, FALSE, FALSE);
	vAHI_Uart0RegisterCallback(pcComs_HandleUart0Interrupt);

	       vAHI_UartSetInterrupt(E_AHI_UART_0,
	        FALSE,
	        FALSE,
	        FALSE,//tx fifo empty
	        TRUE,//rx data
	        E_AHI_UART_FIFO_LEVEL_1);

#else
    /* Enable UART 0: 38400-8-N-1 */
    vAHI_UartEnable(E_AHI_UART_0);

    vAHI_UartReset(E_AHI_UART_0, TRUE, TRUE);
    vAHI_UartReset(E_AHI_UART_0, FALSE, FALSE);

    vAHI_UartSetClockDivisor(E_AHI_UART_0, E_AHI_UART_RATE_38400);
    vAHI_UartSetControl(E_AHI_UART_0, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);

    vAHI_Uart0RegisterCallback(pcComs_HandleUart0Interrupt);

       vAHI_UartSetInterrupt(E_AHI_UART_0,
        FALSE,
        FALSE,
        TRUE,//tx fifo empty
        TRUE,//rx data
        E_AHI_UART_FIFO_LEVEL_1);
		vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);

#endif

}

void pcComsPrintf(const char *fmt, ...)
{
    char buf[256];
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret=vsnprintf(buf,240,fmt, ap);
    va_end(ap);

    pcComsSendPacket((uint8*)buf,0,ret-1,0x90);

}
/*
void pcComsSendPacket0(uint8 buff[], int start, uint8 length)
{
   int i;
   uint8 checksum=0;
   vFifoPutC(length+1);
   checksum^=length+1;
   for(i=0;i<length;i++)
   {
       uint8 b=buff[start+i];
       vFifoPutC(b);
       checksum^=b;
   }
   vFifoPutC(checksum);
}
*/
void pcComsSendPacket(uint8 buff[], int start, uint8 length, uint8 cmd)
{
   int i;
   uint8 checksum=0;
   vFifoPutC(length+2);
   vFifoPutC(cmd);
   checksum^=length+2;
   checksum^=cmd;
   for(i=0;i<length;i++)
   {
       uint8 b=buff[start+i];
       vFifoPutC(b);
       checksum^=b;
   }
   vFifoPutC(checksum);
}
void pcComsSendPacket2(uint8 buff[], int start, uint8 length, uint8 cmd,uint8 cmd2)
{
   int i;
   uint8 checksum=0;
   vFifoPutC(length+3);
   vFifoPutC(cmd);
   vFifoPutC(cmd2);
   checksum^=length+3;
   checksum^=cmd;
   checksum^=cmd2;

   for(i=0;i<length;i++)
   {
       uint8 b=buff[start+i];
       vFifoPutC(b);
       checksum^=b;
   }
   vFifoPutC(checksum);
}


void pcComsParsePacket(void* context,void* buff)
{
    //runs in app not interrupt context
    uint8 retbuf[256];
    uint8* packet=(uint8*)buff;
    uint8 len=packet[0];
    if(len>0 && packet[len]==0)
    {
        uint8 cmd=packet[1];
     //   int cc=(int)cmd;
    //    vPrintf("%d %d",len,cc);
        switch(cmd)
        {
            // emulate jennic bootloader commands
            case 0x09: //flash write
            {
                bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);
                int addr=packet[2]+(packet[3]<<8)+(packet[4]<<16)+(packet[5]<<24);
                bAHI_FullFlashProgram(addr,len-6,&packet[6]);
                pcComsSendPacket(pcComsAck,0,1,0x0a);
                break;
            }
            case 0x0b: //flash read
            {
                bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);
                int addr=packet[2]+(packet[3]<<8)+(packet[4]<<16)+(packet[5]<<24);
                int len=packet[6];
                retbuf[0]=0;
                bAHI_FullFlashRead(addr,len,&retbuf[1]);
                pcComsSendPacket(retbuf,0,len+1,0x0c);
                break;
            }
            case 0x0d: //flash erase sector
            {
                bAHI_FlashInit(E_FL_CHIP_AUTO, NULL);
                bAHI_FlashEraseSector(packet[2]);
                pcComsSendPacket(pcComsAck,0,1,0x0e);
                break;
            }
            case 0x0f: //write sr  - ignore
            {
                pcComsSendPacket(pcComsAck,0,1,0x10);
                break;
            }
            case 0x25: //read flash type
            {
                retbuf[0]=0;
#ifdef JN5148
                retbuf[1]=0x12;
                retbuf[2]=0x12;
#else
                retbuf[1]=0x10;
                retbuf[2]=0x10;
#endif
                pcComsSendPacket(retbuf,0,3,0x26);
                break;
            }
            case 0x2c: //write flash type - ignore
            {
                pcComsSendPacket(pcComsAck,0,1,0x2d);
                break;
            }
            // custom commands
            case 0x80: //reset
            {
               //don't ack as it wil be cut short
               // pcComsSendPacket(pcComsAck,0,1,0x81);
                vAHI_SwReset();
                break;
            }
            case 0x82:  //tunnel through radio
            {
                //strip of
                //defer ack until acked over radio
                break;
            }
            case 0x84:  //init over air rx programming
            {
           //     initOverAirUpload(packet);

                //defer ack until acked over radio
                break;
            }
            case 0x86:  //tunnel through radio
            {
          //      overAirUploadPacket(packet);
                //defer ack until acked over radio
                break;
            }
            case 0x94:  //virtual click on lcd display
            {
            //    displayClick(packet[2], packet[3]);
                break;
            }
            case 0xff:  //new routed protocol
            {
                //get callback and con id, we don't know uart?

            	(*(pcComsPortToIdx[0]->callback))(&packet[2],len-2,pcComsPortToIdx[0]->connector_id);


            	//len 0xff
              //  txHandleRoutedMessage(&packet[2],len-2,CONPC);


                break;
            }
        }
    }
    else
    {
    	if(len>0&& packet[len]!=0)pcComsCrcErrors++;
    }
}

PRIVATE void pcComs_HandleUart0Interrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    // handle communication with pc

    if (u32Device == E_AHI_DEVICE_UART0)
    {
        if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_RXDATA
            || (u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_TIMEOUT)
        {
            uint8 b=u8AHI_UartReadData(E_AHI_UART_0);
            pcComsBuffers[pcComsCurrentBuf][pcComsUploadLen++]=b;
            if(pcComsUploadLen>=255)
            {
                pcComsUploadLen=0;
                pcComsChecksum=0;
            }

            if(pcComsUploadLen>0)
            {
                //len cmd cs 2 37 cs
                if( pcComsUploadLen<=pcComsBuffers[pcComsCurrentBuf][0])
                {
                    pcComsChecksum^=b;
                }
                else
                {
                    //set checksum to 0 if ok
                    pcComsBuffers[pcComsCurrentBuf][pcComsUploadLen-1]-=pcComsChecksum;
                    // post to app context
                    swEventQueuePush(pcComsParsePacket,NULL,&pcComsBuffers[pcComsCurrentBuf][0]);

                    //reset for next packet
                    pcComsUploadLen=0;
                    pcComsChecksum=0;
                    pcComsCurrentBuf++;
                    if(pcComsCurrentBuf>=3)pcComsCurrentBuf=0;

                }
            }
        }
        if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_TX)
        {
            //write up to 16 bytes from circular buffer to uart

          //  uint8 queued=pcComsWriteIdx-pcComsReadIdx;
          //  if(queued>12)queued=12;
          //  while(queued>0)
          if(pcComsTxLock==0)
          {
            if(pcComsWriteIdx!=pcComsReadIdx)
            {
                vAHI_UartWriteData(E_AHI_UART_0,pcComsTxBuffer[pcComsReadIdx++]);
            //    queued--;
            }
          }
        }
    }
}

PRIVATE int vNum2String(uint32 u32Number, uint8 u8Base,char* opbuf,char* maxbuf)
{
    char buf[33];
    char *p = buf + 33;
    uint32 c, n;
    int ret=0;

    *--p = '\0';
    do {
        n = u32Number / u8Base;
        c = u32Number - (n * u8Base);
        if (c < 10) {
            *--p = '0' + c;
        } else {
            *--p = 'a' + (c - 10);
        }
        u32Number /= u8Base;
    } while (u32Number != 0);

    while (*p && opbuf<maxbuf){
        *(opbuf++)=*p;
        p++;
        ret++;
    }

    return ret;
}

PUBLIC int vsnprintf(char* buf,size_t size,const char *fmt, va_list ap)
{
    char *bp = (char *)fmt;
    char*buff=buf;
    char c;
    char *p;
    int32 i;

    char*maxbuf=buf+size-2;


    while ((c = *bp++)) {
        if (c != '%') {

            if(buff<maxbuf) *(buff++)=c;
            continue;
        }

        switch ((c = *bp++)) {

        /* %d - show a decimal value */
        case 'd':
            buff+=vNum2String(va_arg(ap, uint32), 10,buff,maxbuf);
            break;

        /* %x - show a value in hex */
        case 'x':
            if(buff<maxbuf) *(buff++)='0';
            if(buff<maxbuf) *(buff++)='x';
            buff+=vNum2String(va_arg(ap, uint32), 16,buff,maxbuf);
            break;

        /* %b - show a value in binary */
        case 'b':
            if(buff<maxbuf) *(buff++)='0';
            if(buff<maxbuf) *(buff++)='b';
            buff+=vNum2String(va_arg(ap, uint32), 2,buff,maxbuf);
            break;

        /* %c - show a character */
        case 'c':
            if(buff<maxbuf) *(buff++)=va_arg(ap, int);
            break;

        case 'i':
            i = va_arg(ap, int32);
            if(i < 0){
                if(buff<maxbuf) *(buff++)='-';
                buff+=vNum2String((~i)+1, 10,buff,maxbuf);
            } else {
                buff+=vNum2String(i, 10,buff,maxbuf);
            }
            break;

        /* %s - show a string */
        case 's':
            p = va_arg(ap, char *);
            do {
                if(buff<maxbuf) *(buff++)=*(p++);
            } while (*p);
            break;

        /* %% - show a % character */
        case '%':
            if(buff<maxbuf) *(buff++)='%';
            break;

        /* %something else not handled ! */
        default:
            if(buff<maxbuf) *(buff++)='?';
            break;

        }
    }
    *(buff++)='\0';
    return buff-buf;
}
PUBLIC int snprintf(char* buf,size_t size,const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret=vsnprintf(buf,size,fmt, ap);
    va_end(ap);
    return ret;
}

