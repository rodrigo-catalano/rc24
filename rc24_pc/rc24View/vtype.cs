using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public enum vtype 
    {
	    num, code,vref,numArray,codeArray,refArray,label,constant,obj
	}
    //where parameter should be written
    public enum ParamLocation
    {
        opcode,stack
    }
    //where parameter needs to come from for this opcode varient
    public enum ParamSource
    {
        ID,CONST,STACK
    }
}
