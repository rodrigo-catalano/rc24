using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public class bluntLabel
    {
        public int address;
        public List<int> callees = new List<int>();
        public bluntLabel()
        {
        }
        public bluntLabel(int pc)
        {
            address = pc;
        }
    }
}
