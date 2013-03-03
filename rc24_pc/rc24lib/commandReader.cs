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
using System.IO;

namespace rc24
{
    public class commandReader : BinaryReader
    {
        public commandReader(Stream stream):base(stream)
        {
            
        }
        public override string ReadString()
        {
            byte len = ReadByte();
            StringBuilder ret = new StringBuilder(len);
            for (int i = 0; i < len; i++) ret.Append((char)ReadByte());
            return ret.ToString();
        }
        public override Int16 ReadInt16()
        {
            return (Int16)((ReadByte() << 8) + ReadByte());
        }
        public override UInt16 ReadUInt16()
        {
            return (UInt16)((ReadByte() << 8) + ReadByte());
        }
        public override Int32 ReadInt32()
        {
            return (ReadByte() << 24) + (ReadByte() << 16)
                + (ReadByte() << 8) + ReadByte();
        }
        public override UInt32 ReadUInt32()
        {
            return (UInt32)((ReadByte() << 24) + (ReadByte() << 16)
                + (ReadByte() << 8) + ReadByte());
        }
        public override Int64 ReadInt64()
        {
            return (Int64)((ReadByte() << 56) + (ReadByte() << 48) 
                + (ReadByte() << 40) + (ReadByte() << 32)
                + (ReadByte() << 24) + (ReadByte() << 16)
                + (ReadByte() << 8) + ReadByte());
        }
        public override UInt64 ReadUInt64()
        {
            return (UInt64)((ReadByte() << 56) + (ReadByte() << 48)
                + (ReadByte() << 40) + (ReadByte() << 32)
                + (ReadByte() << 24) + (ReadByte() << 16)
                + (ReadByte() << 8) + ReadByte());
        }

        public override bool ReadBoolean()
        {
            return (ReadByte() == 0);
        }
        public UInt32 Read7BitEncodedUInt32()
        {
            UInt32 ret=0;
            byte b=0;
            while(( b=ReadByte())>=128)
            {
                ret+=(UInt32)(b & 0x7f);
                ret <<= 7;
            }
            ret += b;
            return ret;
        }
    }
}
