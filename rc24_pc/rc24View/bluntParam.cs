using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public class bluntParam
    {
        public vtype ptype;
        public String name;
        public String objectType;
        public ParamLocation location = ParamLocation.stack;
        public int constantValue;
        public bluntParam(vtype t, String n,ParamLocation l)
        {
            ptype = t;
            name = n;
            location = l;
        }
        public bluntParam(vtype t, String n)
        {
            ptype = t;
            name = n;
        }
        public bluntParam(vtype t)
        {
            ptype = t;
            name = "";
        }
        public bluntParam(vtype t,int constValue)
        {
            ptype = t;
            constantValue = constValue;
            name = "";
        }
        public bluntParam()
        {
            name = "";
        }
    }
}
