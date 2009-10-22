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
#ifndef _GUI_WALNUT_H
#define _GUI_WALNUT_H

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Exported variables											  ***/
/****************************************************************************/

//extern visualPage* pages;
extern lcdEADog lcd;
extern uint8 lcdbuf[8* 128 ]__attribute__ ((aligned (4)));

/****************************************************************************/
/***        Exported Function Prototypes                                  ***/
/****************************************************************************/

void initGUI(void);
void pollTouchScreen(void);
void refreshGUI(void);

#if defined __cplusplus
}
#endif
#endif
