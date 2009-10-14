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
    public class route
    {
        private byte[] _address;
        public static route DirectLink = new route(new byte[] { 0 },0);
        public route()
        {

        }
        public route(byte[] buff,int start)
        {
            int addrLen=(buff[start]>>4)+1;
            _address = new byte[addrLen];
            Array.Copy(buff, start, _address, 0, addrLen);
        }
        public route(route parent, byte connector)
        {
            _address = new byte[parent.Length+1];
            Array.Copy(parent._address, _address, parent.Length);
            _address[_address.Length-1]=connector;
            _address[0] += 16;
        }
        
        public route getReturnRoute()
        {
            route ret = new route();
            ret._address=new byte[Length];
            ret._address[0] = (byte)((Length-1) << 4);
            for (int i = 1; i < Length; i++)
            {
                ret._address[i] = _address[Length - i];
            }
            return ret;
        }
        public int Length
        {
            get
            {
                return _address.Length;
            }
        }
        public byte getLink(int idx)
        {
            return _address[idx + 1];
        }
        public void CopyTo(byte[] buf,int start)
        {
            _address.CopyTo(buf,start);
        }
        public bool isForMe()
        {
            return (_address[0]&0x0f)==(_address[0]>>4);
        }
    }
}
