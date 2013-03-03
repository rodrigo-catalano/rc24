using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public class bluntVar
    {
        public int offset;
        public vtype type;
        public String objectType;
        
        public bluntVar(int Offset, vtype Type,String ObjectType)
        {
            offset = Offset;
            type = Type;
            objectType = ObjectType;
        }
    }
}
