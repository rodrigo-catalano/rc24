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
    public class ccParameter
    {
        // type ids as sent down the wire
        public const byte CC_BOOL=0;
        public const byte CC_STRING = 1;
        public const byte CC_UINT8 = 2;
        public const byte CC_INT8 = 3;
        public const byte CC_UINT16 = 4;
        public const byte CC_INT16 = 5;
        public const byte CC_UINT32 = 6;
        public const byte CC_INT32 = 7;
        public const byte CC_UINT64 = 8;
        public const byte CC_INT64 = 9;
        public const byte CC_BOOL_ARRAY = 10;
        public const byte CC_STRING_ARRAY = 11;
        public const byte CC_UINT8_ARRAY = 12;
        public const byte CC_INT8_ARRAY = 13;
        public const byte CC_UINT16_ARRAY = 14;
        public const byte CC_INT16_ARRAY = 15;
        public const byte CC_UINT32_ARRAY = 16;
        public const byte CC_INT32_ARRAY = 17;
        public const byte CC_UINT64_ARRAY = 18;
        public const byte CC_INT64_ARRAY = 19;
        public const byte CC_FLOAT = 20;
		public const byte CC_DOUBLE=21;
		public const byte CC_FLOAT_ARRAY=22;
		public const byte CC_DOUBLE_ARRAY=23;
        public const byte CC_ENUMERATION = 24;
		public const byte CC_ENUMERATION_VALUES=25;
        public const byte CC_VOID_FUNCTION = 26;

        // map rc24 types to .net types
        static List<Type> types=null; 

        public int Index;
        public string Name;
        public Type type;
        public object Value;
        public byte TypeIdx;
        public UInt32 ArrayLen;
        public byte EnumListIdx;
        public routedNode Owner;

        static ccParameter()
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
            types.Add(typeof(float));
            types.Add(typeof(double));
            types.Add(typeof(float[]));
            types.Add(typeof(double[]));
            types.Add(typeof(byte));
            types.Add(typeof(string[]));
            types.Add(typeof(byte));


        }
        public ccParameter(int index, string name, byte typeIdx,UInt32 arrayLen,routedNode owner)
        {
            Index = index;
            Name=name;
            type = getTypeFromCode(typeIdx);
            TypeIdx = typeIdx;
            ArrayLen = arrayLen;
            Owner = owner;
        }
        public void parseValue(routedMessage msg)
        {
            UInt32 startIdx;
            byte len;
            commandReader reader = msg.getReader();
            reader.ReadByte();
            reader.ReadByte();


            switch (TypeIdx)
            {
                case CC_BOOL:
                    Value = reader.ReadBoolean();
                    break;
                case CC_STRING:
                    Value = reader.ReadString();
                    break;
                case CC_UINT8:
                    byte valb = reader.ReadByte();
                    Value = valb;
                    break;
                case CC_INT8:
                    sbyte valsb = reader.ReadSByte();
                    Value = valsb;
                    break;
                case CC_UINT16:
                    UInt16 valui16 = reader.ReadUInt16();
                    Value = valui16;
                    break;
                case CC_INT16:
                    Int16 vali16 = reader.ReadInt16();
                    Value = vali16;
                    break;
                case CC_UINT32:
                    UInt32 valui32 = reader.ReadUInt32();
                    Value = valui32;
                    break;
                case CC_INT32:
                    Int32 vali32 = reader.ReadInt32();
                    Value = vali32;
                    break;
                case CC_UINT64:
                    throw (new NotImplementedException());
                    break;
                case CC_INT64:
                    throw (new NotImplementedException());
                    break;
                case CC_BOOL_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_STRING_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_UINT8_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new byte[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((byte[])Value)[i] = reader.ReadByte();
                    break;
                case CC_INT8_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new sbyte[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((sbyte[])Value)[i] = reader.ReadSByte();
                    break;
                case CC_UINT16_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new UInt16[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((UInt16[])Value)[i] = reader.ReadUInt16();
                    break;
                case CC_INT16_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new Int16[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((Int16[])Value)[i] = reader.ReadInt16();
                    break;
                case CC_UINT32_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new UInt32[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((UInt32[])Value)[i] = reader.ReadUInt32();
                       break;
                case CC_INT32_ARRAY:
                    startIdx = reader.Read7BitEncodedUInt32();
                    len = reader.ReadByte();
                    if (Value == null) Value = new Int32[ArrayLen];
                    for (UInt32 i = startIdx; i < startIdx+len; i++) ((Int32[])Value)[i] = reader.ReadInt32();
                    break;
                case CC_UINT64_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_INT64_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_FLOAT:
                    throw (new NotImplementedException());
                    break;
                case CC_DOUBLE:
                    throw (new NotImplementedException());
                    break;
                case CC_FLOAT_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_DOUBLE_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_ENUMERATION:
                    byte valenum = reader.ReadByte();
                    Value = valenum;
                    break;
                case CC_ENUMERATION_VALUES:
                    startIdx = reader.ReadByte();
                    len = reader.ReadByte();
                    string[] stringArray = new string[len];
                    for (int i = 0; i < len; i++) stringArray[i] = reader.ReadString();
                    Value = stringArray;
                    break;
                case CC_VOID_FUNCTION:
                    throw (new NotImplementedException());
                    break;
            }
        }
        public byte[] buildSetCmd()
        {
            return buildSetCmd(0, (byte) Math.Min(ArrayLen, 32));
        }
        public byte[] buildSetCmd(UInt32 start,byte len)
        {
            commandWriter cmd = new commandWriter();
            cmd.Write((byte)1);
            cmd.Write((byte)Index);
            switch (TypeIdx)
            {
                case CC_BOOL:
                    cmd.Write((bool)Value);
                    break;
                case CC_STRING:
                    cmd.Write((string)Value);
                    break;
                case CC_UINT8:
                    cmd.Write((byte)Value);
                    break;
                case CC_INT8:
                    cmd.Write((sbyte)Value);
                    break;
                case CC_UINT16:
                    cmd.Write((UInt16)Value);
                    break;
                case CC_INT16:
                    cmd.Write((Int16)Value);
                    break;
                case CC_UINT32:
                    cmd.Write((UInt32)Value);
                    break;
                case CC_INT32:
                    cmd.Write((Int32)Value);
                    break;
                case CC_UINT64:
                    cmd.Write((UInt64)Value);
                    break;
                case CC_INT64:
                    cmd.Write((Int64)Value);
                    break;
                case CC_BOOL_ARRAY:
                     throw (new NotImplementedException());
                    break;
                case CC_STRING_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_UINT8_ARRAY:
                    cmd.Write((byte[])Value, start, len);
                    break;
                case CC_INT8_ARRAY:
                    cmd.Write((sbyte[])Value, start, len);
                    break;
                case CC_UINT16_ARRAY:
                    cmd.Write((UInt16[])Value, start, len);
                    break;
                case CC_INT16_ARRAY:
                    cmd.Write((Int16[])Value, start, len);
                    break;
                case
                    CC_UINT32_ARRAY:
                    cmd.Write((UInt32[])Value, start, len);
                    break;
                case
                    CC_INT32_ARRAY:
                    cmd.Write((Int32[])Value, start, len);
                    break;
                case
                    CC_UINT64_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case
                    CC_INT64_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_FLOAT:
                    throw (new NotImplementedException());
                    break;
                case CC_DOUBLE:
                    throw (new NotImplementedException());
                    break;
                case CC_FLOAT_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_DOUBLE_ARRAY:
                    throw (new NotImplementedException());
                    break;
                case CC_ENUMERATION:
                    cmd.Write((byte)Value);
                    break;
                case CC_ENUMERATION_VALUES:
                    break;
                case CC_VOID_FUNCTION:
                    //no parameters
                    break;
            }
            return cmd.getCommand();
        }
       
        static public Type getTypeFromCode(byte code)
        {
            return types[code];
        }
        public List<string> GetEnumList()
        {
            // TODO tidy. quick fix enumeration values are held in another ccParameter
            // with an index held in the ArrayLen field
            ccParameter enumList=Owner.getParamByIdx((byte)ArrayLen);
            if(enumList!=null && enumList.Value!=null)
            {
                return ((string[])(enumList.Value)).ToList();
            }
            else 
            {
                return new List<string>();
            }
        }
    }
}
