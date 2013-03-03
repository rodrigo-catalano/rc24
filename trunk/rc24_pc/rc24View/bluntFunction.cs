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
}
}
