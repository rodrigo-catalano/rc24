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


typedef struct
{
    uint8 Model;
    uint8 Status;
    uint8 LeftButtons;
    uint8 RightButtons;
    uint8 RJoyX;
    uint8 RJoyY;
    uint8 LJoyX;
    uint8 LJoyY;
    bool LFire1;
    bool LFire2;
    bool RFire1;
    bool RFire2;
    bool LUp;
    bool LDown;
    bool LLeft;
    bool LRight;
    bool RUp;
    bool RDown;
    bool RLeft;
    bool RRight;
    bool Select;
    bool Start;
    bool RJoy;
    bool LJoy;
}ps2ControllerOp;

bool readPS2Controller(ps2ControllerOp* ps2);
bool initPS2Controller(ps2ControllerOp* ps2);
void ps2Dump(ps2ControllerOp* ps2);
void cycleDelay(uint32 del);
