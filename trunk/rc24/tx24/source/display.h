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


#define D_STD_FONT 8
#define D_INVERSE 255
#define D_NORMAL 0
#define D_REDRAW FALSE
#define D_VISIBLE TRUE




typedef enum
{
    dctLabel,
    dctNumber,
    dctBar,
    dctImage,
    dctGroup,
    dctNumberSpinner,
}displayControlType;

typedef struct
{
    void* control;
    displayControlType type;
}visualControl;

typedef struct
{
    visualControl* controls;
    int len;
}visualPage;

typedef struct
{
    uint8 x;
    uint8 y;
    uint8 pressure;
    visualControl* sender;
}clickEventArgs;

typedef void (*CLICK_HANDLER_FN)(clickEventArgs* eventargs);

typedef struct
{
    int* num;
    int x;
    int y;
    int w;
    int h;
    int font;
    int mask;
    bool visible;
    int lastValue;
    uint8 dplaces;
    char* format;
}numberControl;

typedef struct
{
    int* num;
    int x;
    int y;
    int w;
    int h;
    int font;
    int mask;
    bool visible;
    int lastValue;
    uint8 orientation;
}barControl;


typedef struct
{
    char* txt;
    int x;
    int y;
    int w;
    int h;
    int font;
    int mask;
    bool visible;
    bool valid;
    CLICK_HANDLER_FN clickHandler;
}labelControl;

typedef struct
{
    uint8* image;
    int x;
    int y;
    int w;
    int h;
    bool visible;
    bool valid;
    CLICK_HANDLER_FN clickHandler;
}imageControl;


void renderPage(visualControl* controls, int len,bool forceUpdate,uint8* buf,int scanlen);
void vWriteText(uint8* buf,int scanlen,char *pcString, uint8 u8Row, uint8 u8Column, uint8 u8Mask,uint8 width);
int findControl(uint8 x, uint8 y,visualPage* page);

extern uint8 rowValid[];
