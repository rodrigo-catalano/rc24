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
eg

int txbat,rxbat;

//build data bound screens

labelControl txbatLabel={"tx bat",0,0,30,8,8,0,TRUE,FALSE};
numberControl txbatVal={&txbat,30,0,30,8,8,0,TRUE,0};

labelControl rxbatLabel={"rx bat",0,8,30,8,8,0,TRUE,FALSE};
numberControl rxbatVal={&rxbat,30,8,30,8,8,0,TRUE,0};

visualControl page1[]={{&txbatLabel,dctLabel},
                        {&txbatVal,dctNumber},
                        {&rxbatLabel,dctLabel},
                        {&rxbatVal,dctNumber},
                       };




*/
//todo improve string and number binding and formatting

#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <LcdFont.h>
#include "display.h"

#include "ps2.h"
PRIVATE void itoa(int num,char op[],uint8 u8Base)
{
    char buf[33];
    char *p = buf + 33;
    uint32 c, n;

    if(num<0)
    {
     num=-num;
     *(op++)=']';
    }
    *--p = '\0';
    do {
        n = num / u8Base;
        c = num - (n * u8Base);
        if (c < 10) {
            *--p = '0' + c;
        } else {
            *--p = 'a' + (c - 10);
        }
        num /= u8Base;
    } while (num != 0);

    while (*p){
        *(op++)=*p;
        p++;
    }
    *op++=' ';
    *op='\0';

    return;
}

void renderLabel(labelControl* label,bool forceUpdate,uint8* buf,int scanlen)
{
    if(forceUpdate || !label->valid)
    {
        vWriteText(buf,scanlen,label->txt, label->x, label->y, label->mask, label->w);
        label->valid=TRUE;
    }
}
void renderNumber(numberControl* label,bool forceUpdate,uint8* buf,int scanlen)
{
    char b[20];

    int val;
    // quick fix to allow binding byte values - should go for full printf formatting
    if(label->format[0]=='b')
    {
        uint8 bval =*((uint8*)(label->num));
        val=bval;
    }
    else if(label->format[0]=='h')
    {
        int16 hval =*((int16*)(label->num));
        val=hval;
    }
    else
    {
        val=*(label->num);
    }
    if(forceUpdate || label->lastValue!=val)
    {
        itoa(val,b,10);

        vWriteText(buf,scanlen,b, label->x, label->y, label->mask, label->w);
        label->lastValue = val;
    }
}
void renderBar(barControl* bar,bool forceUpdate,uint8* buf,int scanlen)
{
    uint8 vbar[]={0x00,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};
    int col;
    if(forceUpdate || bar->lastValue!=*(bar->num))
    {
        int val=*(bar->num);
        uint8 b;
        if(val<0)val=0;
        if(val>8)val=8;

        b=vbar[val];

        uint8 *pu8Shadow = &buf[(bar->y>>3) * scanlen + bar->x];

        for(col=0;col< bar->w;col++)
        {
            *(pu8Shadow++)=b;

        }
        //vWriteText(buf,scanlen,b, label->x, label->y, label->mask);
        bar->lastValue = val;
    }
}
void renderImage(imageControl* img,bool forceUpdate,uint8* buf,int scanlen)
{

    if(forceUpdate || !img->valid)
    {
        uint8* image=img->image;
        uint8 *pu8Shadow = &buf[(img->y>>3) * scanlen + img->x];
        int row,col;
        for(row=0;row< img->h>>3;row++)
        {
            for(col=0;col< img->w;col++)
            {
                *(pu8Shadow++)=*(image++);
            }
            pu8Shadow+=scanlen-img->w;
        }
        img->valid=TRUE;
    }
}
void renderPage(visualControl* controls, int len,bool forceUpdate,uint8* buf,int scanlen)
{
    if(forceUpdate)
    {
        //clear buffer
        memset(buf,0,scanlen*8);
    }
    int i;
    for(i=0;i<len;i++)
    {
        switch(controls[i].type)
        {
            case dctLabel : renderLabel((labelControl*)(controls[i].control),forceUpdate, buf, scanlen);break;
            case dctNumber : renderNumber((numberControl*)(controls[i].control),forceUpdate, buf, scanlen);break;
            case dctBar : renderBar((barControl*)(controls[i].control),forceUpdate, buf, scanlen);break;
            case dctImage : renderImage((imageControl*)(controls[i].control),forceUpdate, buf, scanlen);break;
            case dctGroup : break;
        }
    }
}
int findControl(uint8 x, uint8 y,visualPage* page)
{
    int i;
    visualControl* controls=page->controls;
    numberControl* nc;
    for(i=0;i<page->len;i++)
    {
        switch(controls[i].type)
        {
            case dctLabel :
            if(((labelControl*)(controls[i].control))->x<=x &&
            ((labelControl*)(controls[i].control))->x + ((labelControl*)(controls[i].control))->w>x &&
            ((labelControl*)(controls[i].control))->y<=y &&
            ((labelControl*)(controls[i].control))->y + ((labelControl*)(controls[i].control))->h>y
            )return i;
            ;break;
            case dctNumber :
                nc=(numberControl*)(controls[i].control);
                if(nc->x<=x && nc->x + nc->w >x && nc->y<=y && nc->y + nc->h>y)return i;
                break;
            case dctBar : break;
            case dctImage :
            if(((imageControl*)(controls[i].control))->x<=x &&
            ((imageControl*)(controls[i].control))->x + ((imageControl*)(controls[i].control))->w>x &&
            ((imageControl*)(controls[i].control))->y<=y &&
            ((imageControl*)(controls[i].control))->y + ((imageControl*)(controls[i].control))->h>y
            )return i;
            ;break;
            case dctGroup : break;

        }
    }
    return -1;
}
void vWriteText(uint8* buf,int scanlen,char *pcString, uint8 u8Column, uint8 u8Row, uint8 u8Mask, uint8 width)
{
    uint8 u8Columns;
    uint8 *pu8CharMap;
    uint8 *pu8Shadow = &buf[(u8Row>>3) * scanlen + u8Column];

    /* Column before first character */
    *pu8Shadow = u8Mask;
    pu8Shadow++;
    uint8 w=1;
    while (*pcString != 0)
    {
        pu8CharMap = pu8LcdFontGetChar(*pcString);
        u8Columns = *pu8CharMap;

        if(u8Columns+w>width)u8Columns=width-w;
        /* Copy character bitmap to shadow memory */
        w+=u8Columns;
        do
        {
            pu8CharMap++;
            *pu8Shadow = (*pu8CharMap) ^ u8Mask;
            pu8Shadow++;
            u8Columns--;
        } while (u8Columns > 0);
        if(w<width)
        {
            /* Add a column spacing */
            *pu8Shadow = u8Mask;
            pu8Shadow++;

            pcString++;
            w++;

        }
        if(w>=width)break;

    }
    while(w<width)
    {
        *pu8Shadow = u8Mask;
         pu8Shadow++;
         w++;
    }
}
