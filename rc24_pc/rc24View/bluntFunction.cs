using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public class bluntFunction {

	public String name;
	public bluntParam[] fparams;
	
	public int opCode;
	public int localStackUsage;
	public bool retEmptyStack=false;
	public int opLen=1;//length of opcode
    public fType functionType = fType.NATIVE;
    public List<int> callees = new List<int>();
    public Dictionary<String, bluntLabel> labels = new Dictionary<string, bluntLabel>();
    public Dictionary<String, bluntVar> locals = new Dictionary<string, bluntVar>();
    //public List<int> constantParams;

	public bluntFunction(String Name,bluntParam[] Params,int OpCode)
	{
		name=Name;
		fparams=Params;
		opCode=OpCode;
		localStackUsage=0;
	}
	public bluntFunction(String Name,bluntParam[] Params,int OpCode, int LocalStackUsage)
	{
		name=Name;
		fparams=Params;
		opCode=OpCode;
		localStackUsage=LocalStackUsage;
	}
	public bluntFunction(String Name,bluntParam[] Params,int OpCode, int LocalStackUsage, bool EmptyStack)
	{
		name=Name;
		fparams=Params;
		opCode=OpCode;
		localStackUsage=LocalStackUsage;
		retEmptyStack=EmptyStack;
	}
	public bluntFunction(String Name,bluntParam[] Params,int OpCode, int LocalStackUsage, bool EmptyStack,int len)
	{
		name=Name;
		fparams=Params;
		opCode=OpCode;
		localStackUsage=LocalStackUsage;
		retEmptyStack=EmptyStack;
		opLen=len;
	}
    public string mangledName
    {
        get
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(name);
            if (fparams != null && fparams.Length > 0)
            {
                sb.Append("_");
                foreach (var p in fparams) sb.AppendFormat("_{0}", p.typeName);
            }
            return sb.ToString();
        }
    }
}
    public class functionList : Dictionary<String, bluntFunction>
    {
        public void Add(bluntFunction function)
        {
            Add(function.mangledName, function);
        }

    }
}
