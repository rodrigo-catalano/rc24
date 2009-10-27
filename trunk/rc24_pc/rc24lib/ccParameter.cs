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
using System.Linq;
using System.Text;

namespace rc24
{
    public class ccParameter
    {
        /*
        CC_BOOL,
		CC_STRING,
	    CC_UINT8,
	    CC_INT8,
	    CC_UINT16,
	    CC_INT16,
	    CC_UINT32,
	    CC_INT32,
	    CC_UINT64,
	    CC_INT64,
	    CC_BOOL_ARRAY,
		CC_STRING_ARRAY,
		CC_UINT8_ARRAY,
		CC_INT8_ARRAY,
		CC_UINT16_ARRAY,
		CC_INT16_ARRAY,
		CC_UINT32_ARRAY,
		CC_INT32_ARRAY,
		CC_UINT64_ARRAY,
		CC_INT64_ARRAY
*/

        static List<Type> types; 


        public int Index;
        public string Name;
        public Type type;
        public object Value;
        public byte TypeIdx;

        public ccParameter(int index, string name, byte typeIdx)
        {
            Index = index;
            Name=name;
            type = getTypeFromCode(typeIdx);
            TypeIdx = typeIdx;
        }
        public void parseValue(routedMessage msg)
        {
            switch (TypeIdx)
            {
                case 2:
                    byte valb = msg.readByte();
                    Value = valb;
                    break;
                case 7:
                    int vali32 = msg.readInt32();
                    Value = vali32;
                    break;
                case 14:
                    //TODO currently wrongly assume whole array is always sent
                    byte startIdx = msg.readByte();
                    byte len = msg.readByte();
                    UInt16[] valu16Array = new UInt16[len];
                    for (int i = 0; i < len; i++) valu16Array[i] = (UInt16)msg.readInt16();

                    Value = valu16Array;

                    break;

            }
        }
        public byte[] buildSetCmd()
        {
            byte[] cmd=null;
            switch (TypeIdx)
            {
                case 2:
                    cmd = new byte[3];
                    cmd[0] = 1;
                    cmd[1] = (byte)Index;
                    cmd[2] = (byte)Value;
                    break;
                case 7:
                    cmd = new byte[6];
                    cmd[0] = 1;
                    cmd[1] = (byte)Index;
                    cmd[2] = (byte)(((int)Value)>>24);
                    cmd[3] = (byte)(((int)Value) >> 16);
                    cmd[4] = (byte)(((int)Value) >> 8);
                    cmd[5] = (byte)((int)Value);
                    break;
                case 14:
                    cmd = new byte[44];
                    cmd[0] = 1;
                    cmd[1] = (byte)Index;
                    cmd[2] = 0;
                    cmd[3] = 20;
                    UInt16[] val=(UInt16[])Value;
                    int idx = 4;
                    for(int i=0;i<20;i++)
                    {
                        cmd[idx++]=(byte)(val[i]>>8);
                        cmd[idx++]=(byte)(val[i]);
                    }
                    break;

            }
            return cmd;
        }

        static public Type getTypeFromCode(byte code)
        {
            if (types == null)
            {
                types = new List<Type>();
                types.Add(typeof(bool));
                types.Add(typeof(string));
                types.Add(typeof(byte));
                types.Add(typeof(sbyte));
                types.Add(typeof(UInt16)); 
                types.Add(typeof(Int16)); 
                types.Add(typeof(UInt32));
                types.Add(typeof(Int32));
                types.Add(typeof(UInt64)); 
                types.Add(typeof(Int64));
                types.Add(typeof(bool[]));
                types.Add(typeof(string[]));
                types.Add(typeof(byte[]));
                types.Add(typeof(sbyte[]));
                types.Add(typeof(UInt16[]));
                types.Add(typeof(Int16[]));
                types.Add(typeof(UInt32[]));
                types.Add(typeof(Int32[]));
                types.Add(typeof(UInt64[]));
                types.Add(typeof(Int64[]));

            }
            return types[code];
        }
    }
}
