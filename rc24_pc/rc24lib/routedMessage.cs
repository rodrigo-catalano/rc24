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

namespace rc24
{
    public class routedMessage
    {
        private route _address;
        private List<byte> _command;
        private int _readIdx = 0;
        public routedMessage()
        {

        }
        public routedMessage(byte[] buff, int start, int len)
        {
            _address = new route(buff, start);
            _command = new List<byte>(len - _address.Length);
            int cmdStart=start+_address.Length;
            for (int i = 0; i < len - _address.Length; i++)
            {
                _command.Add(buff[cmdStart+i]);
            }
        }
        public routedMessage(route Route,byte[] cmd)
        {
            _address = Route;
            _command = new List<byte>(cmd);          
        }
        public void resetReader()
        {
            _readIdx = 0;
        }
        public route Route
        {
            get
            {
                return _address;
            }
        }

        public string readString()
        {
            byte len = readByte();  
            StringBuilder ret=new StringBuilder(len);
            for (int i = 0; i < len; i++) ret.Append((char)readByte());
            return ret.ToString();
        }
        public byte readByte()
        {
            return _command[_readIdx++];
        }

        public int bytesRemaining()
        {
            return _command.Count - _readIdx;
        }
        public byte[] toByteArray()
        {
            byte[] ret = new byte[_address.Length + _command.Count];
            _address.CopyTo(ret, 0);
            _command.CopyTo(ret, _address.Length);
            return ret;
        }
        public byte commandByte
        {
            get
            {
                return _command[0];
            }
        }
    }
}
