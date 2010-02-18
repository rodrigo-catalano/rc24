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
    public class routedTreeWalker
    {
        List<route> UnwalkedRoutes=new List<route>();
        byte[] enumerateMsg=new byte[]{0x00};
        routedNode _parent;

        public routedTreeWalker(routedNode parent)
        {
            _parent = parent;
            UnwalkedRoutes.Add(route.DirectLink);
        }
        public void enumerateResponse(routedMessage resp)
        {
            route from=resp.Route.getReturnRoute();
            commandReader reader = resp.getReader();

            reader.ReadByte();
            
            string name=reader.ReadString();

            //add to tree under parent
            //_parent.findNode(from);
            routedNode p=_parent;
            
            //problem with not having the first part of the address
            // TODO do it properly!
                       
            int i;
            if (from.Length > 1)
            {
                p = p.children[0];
                for (i = 1; i < from.Length - 1; i++)
                {
                    byte childIdx = from.getLink(i-1);
                    p = p.children[childIdx];
                }
                p.children.Add(from.getLink(i - 1), new routedNode(name, from));
            }
            else p.children.Add(0, new routedNode(name, from));
         //   else if (from.Length == 2) p.children[0].children.Add((byte)p.children[0].children.Count, new routedNode(name, from));
        //    else p.children.Add(from.getLink(i-1), new routedNode(name,from));
            
            //add children to unwalked list

            try
            {
                while (1 == 1)
                {
                    byte con = reader.ReadByte();
                    UnwalkedRoutes.Add(new route(from, con));
                }
            }
            catch (Exception) { }
        }
        public routedMessage getNextRequest()
        {
            //take child off top of list
            if(UnwalkedRoutes.Count>0)
            {
                route next = UnwalkedRoutes[0];
                UnwalkedRoutes.RemoveAt(0);

                routedMessage request=new routedMessage(next,enumerateMsg);
                return request;
            }
            return null;
        }
    }
}
