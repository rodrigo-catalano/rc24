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
    public class commandWriter : BinaryWriter
    {
        MemoryStream _buffer = new MemoryStream();
        public commandWriter()
        {
            this.OutStream = _buffer;
        }
        public override void Write(bool value)
        {
            if(value==true)Write((byte)0);
            else Write((byte)1);
        }
        public override void Write(string s)
        {
            int len=s.Length;
            Write((byte)len);
            for (int i = 0; i < len; i++) Write((byte)s[i]);
        }
        public override void Write(UInt16 value)
        {
            Write((byte)(value >> 8));
            Write((byte)value);    
        }
        public override void Write(Int16 value)
        {
            Write((byte)(value >> 8));
            Write((byte)value);
        }
        public override void Write(UInt32 value)
        {
            Write((byte)(value >> 24));
            Write((byte)(value >> 16));
            Write((byte)(value >> 8));
            Write((byte)value);
        }
        public override void Write(Int32 value)
        {
            Write((byte)(value >> 24));
            Write((byte)(value >> 16));
            Write((byte)(value >> 8));
            Write((byte)value);
        }
        public override void Write(UInt64 value)
        {
            Write((byte)(value >> 56));
            Write((byte)(value >> 48));
            Write((byte)(value >> 40));
            Write((byte)(value >> 32));
            Write((byte)(value >> 24));
            Write((byte)(value >> 16));
            Write((byte)(value >> 8));
            Write((byte)value);
        }
        public override void Write(Int64 value)
        {
            Write((byte)(value >> 56));
            Write((byte)(value >> 48));
            Write((byte)(value >> 40));
            Write((byte)(value >> 32));
            Write((byte)(value >> 24));
            Write((byte)(value >> 16));
            Write((byte)(value >> 8));
            Write((byte)value);
        }
        public void Write7BitEncodedUInt32(UInt32 value)
        {
            //msb first
            if (value < 128)
            {
                Write((byte)value);
            }
            else if (value < 128 << 7)
            {
                Write((byte)((value>>7)|0x80));
                Write((byte)((value) & 0x7f));
            }
            else if (value < 128 << 14)
            {
                Write((byte)((value >> 14) | 0x80));
                Write((byte)((value >> 7) | 0x80));
                Write((byte)((value) & 0x7f));
            }
            else if (value < 128 << 21)
            {
                Write((byte)((value >> 21) | 0x80));
                Write((byte)((value >> 14) | 0x80));
                Write((byte)((value >> 7) | 0x80));
                Write((byte)((value) & 0x7f));
            }
            else 
            {
                Write((byte)((value >> 28) | 0x80));
                Write((byte)((value >> 21) | 0x80));
                Write((byte)((value >> 14) | 0x80));
                Write((byte)((value >> 7) | 0x80));
                Write((byte)((value) & 0x7f));
            }
           
            
        }
        public virtual void Write(bool[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(String[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(byte[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            Write(value,start,len);
        }
        public virtual void Write(sbyte[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            for (UInt32 i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(Int16[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            for (UInt32 i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(UInt16[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            for (UInt32 i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(UInt32[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            for (UInt32 i = start; i < start+len; i++) Write(value[i]);
        }
        public virtual void Write(Int32[] value, UInt32 start, byte len)
        {
            Write7BitEncodedUInt32(start);
            Write(len);
            for (UInt32 i = start; i < start+len; i++) Write(value[i]);
        }
        public virtual void Write(UInt64[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(Int64[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(Single[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }
        public virtual void Write(Double[] value, byte start, byte len)
        {
            Write(start);
            Write(len);
            for (int i = start; i < len; i++) Write(value[i]);
        }

        public byte[] getCommand()
        {
            return _buffer.ToArray();
        }
    }
}
