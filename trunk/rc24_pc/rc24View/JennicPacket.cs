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
using System;
using System.Collections.Generic;
using System.Text;
using System.IO.Ports;

namespace Serial
{
    public class JennicPacket
    {
        public static void Write(SerialPort sp, byte[] buff, int start, byte length, byte cmd)
        {
            byte[] jbuff = new byte[256];

            byte checksum = 0;
            checksum ^= (byte)(length + 2);
            checksum ^= cmd;

            jbuff[0] = (byte)(length + 2);
            jbuff[1] = cmd;
            for (int i = 0; i < length; i++)
            {
                byte b = buff[start + i];
                checksum ^= b;
                jbuff[2 + i] = b;
            }
            jbuff[length + 2] = checksum;

            sp.RtsEnable = false;
            sp.Write(jbuff, 0, length + 3);

        }
    }
}
