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
#include <utilities.h>
//#include <printf.h>
#include "swEventQueue.h"
#include "nmeagps.h"

//todo test more!
//todo configurable gps baud rate and setup string

PRIVATE void nmea_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap);

char nmeaBuffers[3][81];
int nmeaCurrentBuf=0;

char* nmeabuf=NULL;
int nmeabufidx=0;

int nmealat;
int nmealong;
int nmeaheight;
int nmeaspeed;
int nmeatime;
int nmeatrack;
int nmeasats;
bool nmeaNewData=FALSE;

uint8 gpsUart;

int parseFloat(char buf[],float* val)
{
    int i=0;
    int neg=1;
    uint8 c;
    float div=10;
    float ret=0;
    c=buf[i];

    while(c!=','&&c!=0)
    {
        if(c=='-')neg=-1;
        else if(c=='.')
        {
            c=buf[i++];
            break;
        }
        else
        {
            ret=ret*10+(c-'0');
        }
        c=buf[i++];
    }
    while(c!=','&&c!=0)
    {
        ret=ret+(c-'0')/div;
        div*=10;
        c=buf[i++];
    }
    if(c==',')i++;
    *val=ret*neg;
    return i;

}
int parseFPInt(char buf[],int* val,uint8 places)
{
    int i=0;
    int neg=1;
    uint8 c;
    int ret=0;
    c=buf[i];

    while(c!=','&&c!=0)
    {
        if(c=='-')neg=-1;
        else if(c=='.')
        {
            c=buf[++i];
            break;
        }
        else
        {
            ret=ret*10+(c-'0');
        }
        c=buf[++i];
    }
    while(c!=','&&c!=0)
    {
        if(places>0)
        {
            ret=ret*10+(c-'0');
            places--;
        }
        c=buf[++i];
    }
    while(places>0)
    {
        ret=ret*10;
        places--;
    }
    if(c==',')i++;
    *val=ret*neg;
    return i;

}
int dddmm_mmmTodddddd_ddddd(int a)
{
    int minutes=a%100000;
    int deg=a-minutes;
    return deg+minutes*10/6;

}
int parseString(char buf[])
{
    int i=0;
    char c;
    c=buf[i];
    while(c!=','&&c!=0)
    {
        c=buf[++i];
    }
    if(c==',')i++;
    return i;

}
//E_AHI_UART_0

void initNmeaGps(uint8 uart,int baudrate)
{
    gpsUart=uart;
    if(uart==E_AHI_UART_0)vAHI_Uart0RegisterCallback(nmea_HandleUartInterrupt);
    else vAHI_Uart1RegisterCallback(nmea_HandleUartInterrupt);

    vAHI_UartSetRTSCTS(uart, FALSE);

    vAHI_UartEnable(uart);
    vAHI_UartReset(uart, TRUE, TRUE);
    vAHI_UartReset(uart, FALSE, FALSE);
    vAHI_UartSetClockDivisor(uart, baudrate);
    vAHI_UartSetControl(uart, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);

    vAHI_UartSetInterrupt(uart,
        FALSE,
        FALSE,
        FALSE,
        TRUE,
        E_AHI_UART_FIFO_LEVEL_1);


}

bool readNmeaGps(nmeaGpsData* data)
{
    if(nmeaNewData==TRUE)
    {
        data->nmealat=nmealat;
        data->nmealong=nmealong;
        data->nmeaheight=nmeaheight;
        data->nmeaspeed=nmeaspeed;
        data->nmeatrack=nmeatrack;
        data->nmeatime=nmeatime;
        nmeaNewData=FALSE;
        data->nmeaPacketsRxd++;
        return TRUE;
    }
    else return FALSE;
}

void nmeaGpsParse(void* context,void* buff)
{
  // return;

   //app level callback triggered from interrupt
    char* cbuff=(char*)buff;
    int temp;
    bool valid=FALSE;
    if(memcmp(cbuff,"GPRMC",5)==0)
    {
        cbuff+=parseString(cbuff);
        cbuff+=parseFPInt(cbuff,&nmeatime,2);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='V')valid=FALSE;
        else valid=TRUE;
        cbuff+=parseFPInt(cbuff,&temp,3);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='S')temp*=-1;
        nmealat=dddmm_mmmTodddddd_ddddd(temp);
        cbuff+=parseFPInt(cbuff,&temp,3);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='W')temp*=-1;
        nmealong=dddmm_mmmTodddddd_ddddd(temp);
        cbuff+=parseFPInt(cbuff,&nmeaspeed,3);
        cbuff+=parseFPInt(cbuff,&nmeatrack,3);

        if(valid)nmeaNewData=TRUE;
    }

    else if(memcmp(cbuff,"GPGGA",5)==0)
    {
        cbuff+=parseString(cbuff);
        cbuff+=parseFPInt(cbuff,&nmeatime,2);
        cbuff+=parseFPInt(cbuff,&temp,3);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='S')temp*=-1;
        nmealat=dddmm_mmmTodddddd_ddddd(temp);
        cbuff+=parseFPInt(cbuff,&temp,3);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='W')temp*=-1;
        nmealong=dddmm_mmmTodddddd_ddddd(temp);
        cbuff+=parseString(cbuff);
        if(*(cbuff-2)=='0')valid=FALSE;
        else valid=TRUE;
        cbuff+=parseFPInt(cbuff,&nmeasats,0);
        cbuff+=parseString(cbuff);
        cbuff+=parseFPInt(cbuff,&nmeaheight,1);
        if(valid)nmeaNewData=TRUE;

    }
}

PRIVATE void nmea_HandleUartInterrupt(uint32 u32Device, uint32 u32ItemBitmap)
{
    // uart interrupt handler

    if ((u32ItemBitmap & 0x000000FF) == E_AHI_UART_INT_RXDATA)
    {
     //   while((u8AHI_UartReadLineStatus(gpsUart) & E_AHI_UART_LS_DR) !=0)
     //   {
            uint8 b=u8AHI_UartReadData(gpsUart);
            if(b=='$')
            {
                nmeaCurrentBuf++;
                if(nmeaCurrentBuf==3)nmeaCurrentBuf=0;
                nmeabuf=nmeaBuffers[nmeaCurrentBuf];
                //start new sentance
                nmeabufidx=0;
            }
            else if(b=='\r')
            {
                //post sentence to app
                nmeabuf[nmeabufidx++]=0;
                if(nmeabuf!=NULL)swEventQueuePush(nmeaGpsParse,NULL,nmeabuf);
                nmeabuf=NULL;
            }
            else
            {
                  if(nmeabuf!=NULL && nmeabufidx<80)nmeabuf[nmeabufidx++]=b;
            }
 //       }
    }
}


/*

$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,154723.978,V,,,,,,,030309,,*26
$GPGGA,154724.980,,,,,0,00,,,M,0.0,M,,0000*56
$GPGSA,A,1,,,,,,,,,,,,,,,*1EswEventQueuePush(nmeaGpsParse,nmeabuf);
$GPRMC,154724.980,V,,,,,,,030309,,*26
$GPGGA,154725.974,,,,,0,00,,,M,0.0,M,,0000*5C
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,154725.974,V,,,,,,,030309,,*2C
$GPGGA,154726.978,,,,,0,00,,,M,0.0,M,,0000*53
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPGSV,3,1,12,25,71,000,,16,59,000,,13,42,000,,23,  37,000,*73
$GPGSV,3,2,12,20,36,000,,30,32,000,,06,26,000,,01,  23,000,*7C
$GPGSV,3,3,12,24,15,000,,10,12,000,,02,12,000,,07,  10,000,*7D

$GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh

RMC  = Recommended Minimum Specific GPS/TRANSIT Data

1    = UTC of position fix
2    = Data status (V=navigation receiver warning)
3    = Latitude of fix
4    = N or S
5    = Longitude of fix
6    = E or W
7    = Speed over ground in knots
8    = Track made good in degrees True
9    = UT date
10   = Magnetic variation degrees (Easterly var. subtracts from true course)
11   = E or W
12   = Checksum

$GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh

GGA  = Global Positioning System Fix Data

1    = UTC of Position
2    = Latitude
3    = N or S
4    = Longitude
5    = E or W
6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
7    = Number of satellites in use [not those in view]
8    = Horizontal dilution of position
9    = Antenna altitude above/below mean sea level (geoid)
10   = Meters  (Antenna height unit)
11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
       mean sea level.  -=geoid is below WGS-84 ellipsoid)
12   = Meters  (Units of geoidal separation)
13   = Age in seconds since last update from diff. reference station
14   = Diff. reference station ID#
15   = Checksum


*/
