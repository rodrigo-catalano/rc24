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
    public class routedNode
    {
        private route _route;
        public string name;

        public Dictionary<byte,routedNode> children=new Dictionary<byte,routedNode>();
        //interfaces
        //properties
        public routedNode(string Name)
        {
            name = Name;
            _route = route.DirectLink;
        }
        public routedNode(string Name, route Route)
        {
            name = Name;
            _route = Route;
        }
        public route address
        {
            get
            {
                return _route;
            }
            set
            {
                _route = value;
            }
        }
        /*
        public routedNode findNode(route search)
        {
            if (pos == route.Length - 1) return this;
            if(children.ContainsKey(route[pos])) return children[route[pos]].findNode(route,pos++);
            else return null;
        }
         */ 
    }
}
