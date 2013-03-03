using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    class compiler
    {
        int[] prog = new int[4096];
        int[] stack = new int[1024];
        int pc = 0;
        Dictionary<String, bluntVar> rootlocals = new Dictionary<String, bluntVar>();

        Dictionary<String, bluntFunction> opCodes;
        Dictionary<String, bluntFunction> methodCodes = new Dictionary<String, bluntFunction>();
        Dictionary<String, bluntFunction> ExternalFunctions = new Dictionary<String, bluntFunction>();


        public compiler()
        {
            //get opcodes from vm
            opCodes = new BluntInterp4().opCodes;

        }
        public void run(int[] inputs,bool trace,object[] objects)
        {
            var interp=new BluntInterp4();
            interp.trace = trace;
            interp.run(prog, stack, 0, opCodes,inputs,objects);
        }
        public void compileChildren(astNode n, bluntFunction fn, int idx, Dictionary<String, bluntVar> locals, Dictionary<String, bluntLabel> labels)
        {
            if (n.children != null)
            {
                for (int i = 0; i < n.children.Count; i++)
                {
                    compile(n.children[i], fn, i, locals, labels);
                }
            }
        }
        public int[] compile(astNode n,Dictionary<String, bluntFunction> externalFunctions)
        {
            ExternalFunctions = externalFunctions;
             //entry point is main
                       
            pc+=6;

            getMethods(n, null);

            compile(n, null, 0, rootlocals, new Dictionary<String, bluntLabel>());
            //prog[0] = rootlocals.Count;
            //call main function - could really be avoided by calling directly 
            //but entry point needs to be stored somewhere
            prog[0] = 0;
            prog[1] = opCodes["@call user fn"].opCode;
            prog[2] = methodCodes["main"].opCode;
            prog[3] = methodCodes["main"].fparams.Length;
            prog[4] = methodCodes["main"].localStackUsage;
            prog[5] = opCodes["@ret"].opCode;
            int[] ret = new int[pc];
            System.Array.Copy(prog, 0, ret, 0, pc);
            return ret;
        }
        public void compile(astNode n, bluntFunction fn, int idx, Dictionary<String, bluntVar> locals, Dictionary<String, bluntLabel> labels)
        {
            if (n.type == "CompilationUnit")
            {
                compileChildren(n, fn, idx, locals, labels);
            }
            else if (n.type == "ID")//load id on stack
            {
                // if external var put different op codes to get val or ptr
                // how does assign know to call setter
                // special case for assign            

                if (!locals.ContainsKey(n.name))
                {
                    locals.Add(n.name, new bluntVar(locals.Count, vtype.num,null));
                }
                if (fn != null && fn.fparams[idx].ptype == vtype.vref)
                {
                    prog[pc++] = opCodes["@push var ref"].opCode;
                    prog[pc++] = locals[n.name].offset;	//reference to var on stack
                }
                else
                {
                    prog[pc++] = opCodes["@push var val"].opCode;
                    prog[pc++] = locals[n.name].offset;	//var value on stack
                }
            }
            else if (n.type == "CONST")
            {
                prog[pc++] = opCodes["@push immediate"].opCode; ;
                prog[pc++] = int.Parse(n.val);	//val on stack
            }
            else if (n.type == "LABEL")
            {
                String lname = n.name;
                if (!labels.ContainsKey(lname)) labels.Add(lname, new bluntLabel(pc));
                else labels[lname].address = pc;
            }
            else if (n.type == "ARRAY")//load id on stack
            {
                // stack should have array ptr then index 
                // should know size of array items 
                compileChildren(n, fn, idx, locals, labels);
                prog[pc++] = opCodes["@get_array_item"].opCode;


            }
            else if (n.type == "FN")
            {
                //get param names and call in correct order 
                //and combine duplicates to arrays for varags stuff
                //if single item and not array put it in one
                //if already correct type eg code[] leave as is

                //should become list of fnpc/id/const/label
                List<bluntLabel> opParams = new List<bluntLabel>();
                //list of unamed
                List<astNode> unNamedParams = new List<astNode>();
                Dictionary<String, List<astNode>> namedParams = new Dictionary<String, List<astNode>>();

                if (n.name == ".")
                {
                    //replace node with right of dot as function with left of dot as first parameter
                    astNode dot = n;
                    n = n.children[1];
                    n.children.Insert(0, dot.children[0]);
                    //name mangle function by preceding with object type
                    n.name = locals[dot.children[0].name].objectType + n.name;

                }

                if (n.children != null)
                {
                    for (int i = 0; i < n.children.Count; i++)
                    {
                        astNode parameter = n.children[i];
                        if (parameter.name == null || parameter.name == "")
                        {
                            unNamedParams.Add(parameter);
                        }
                        else
                        {
                            if (!namedParams.ContainsKey(parameter.name))
                            {
                                namedParams.Add(parameter.name, new List<astNode>());
                            }
                            namedParams[parameter.name].Add(parameter);
                        }
                    }
                }
                

                //function call
                //for lambdas put block elsewhere on prog stack and put ptr on stack 
                //put load ref for each parameter
                //ref point to stack which holds pc for fn
                //just like global vars
                //locals.put("#"+n.val,locals.size());
                //stack[locals.get("#"+n.val)]=n.val;
                //why not direct pc in stack
                //var could be a fn

                //redirect op or put jump in after fn but before

                //put child refs
                //then jump
                //then bodies of lambdas
                //then fn call

                int code = 2;
                bluntFunction fnn;
                if (opCodes.ContainsKey(n.name))
                {
                    fnn = opCodes[n.name];
                }
                else if (methodCodes.ContainsKey(n.name))
                {
                    fnn = methodCodes[n.name];
                    code = 4;
                }
                else if (ExternalFunctions.ContainsKey(n.name))
                {
                    //inject routedObject index into parameters
                    fnn = ExternalFunctions[n.name];
                }
                else
                {
                    throw new Exception("Unknown function " + n.name);
                    
                }

                //insert any default parameters                
                for (int i = 0; i < fnn.fparams.Length; i++)
                {
                    if (fnn.fparams[i].ptype == vtype.constant)
                    {
                        astNode cons = new astNode() { val = fnn.fparams[i].constantValue.ToString(), type = "CONST" };
                        n.children.Insert(i, cons);
              
                    }
                }
 
                if (n.name == "return")
                {
                    compileChildren(n, fnn, idx, locals, labels);
                    prog[pc++] = opCodes["@ret user fn"].opCode;
                    prog[pc++] = fn.fparams.Length;
                    return;

                }
                /*
                if (n.name == "while")
                {
                    //special case for while for now
                    int teststart = pc;
                    //put in test code
                    compile(n.children[0], fnn, 0, locals, labels);
                    //test jump
                    prog[pc++] = opCodes["@jz"].opCode;//jump to exit
                    int testjump = pc;
                    pc++;
                    //put in action code
                    compile(n.children[1], fnn, 1, locals, labels);
                    //jump back
                    prog[pc++] = opCodes["@jump"].opCode;//jump to top
                    prog[pc++] = teststart - pc + 1;
                    prog[testjump] = pc - testjump;//set test jump
                    return;
                }
                */
                int nparams = 0;
                if (n.children != null)
                {
                    nparams = n.children.Count;
                    for (int i = 0; i < n.children.Count; i++)
                    {
                        astNode paramNode = n.children[i];
                        //if param should be a lambda
                        if (fnn.fparams[i].ptype == vtype.code)
                        {
                            /* foreach(code a, code b)
                             * {
                             * 	interp(a); - should just leave id on stack
                             * 	interp(12*56); - compiles to push lambda ptr -maybe not allow
                             * 
                             * 	foreach(a,b) - pass on lambda ptr
                             * 	//need to know type to select
                             * 
                             * }
                             * foreach(a=a*b*c,d=d/10)
                             * 
                             * identify in parser ?
                             * wrap code in {} or [] - messy
                             */
                            //track types on stack
                            //bodge to check for lambda ptr
                            astNode pNode = paramNode.children[0];
                            if (pNode != null && pNode.type == "ID" && locals[pNode.name].type == vtype.code)
                            {
                                compile(paramNode, fnn, i, locals, labels);
                            }
                            else
                            {
                                //if parameter is already lambda ptr - just copy ptr
                                prog[pc++] = opCodes["@push lambda ptr"].opCode;//put ptr to lambda on stack
                                prog[pc++] = pc + 2;//put stack base here as well - not known at compile time
                                prog[pc++] = opCodes["@jump"].opCode;//jump
                                int lenPc = pc++;
                                //  prog[pc++]=0;//no stack used for locals - could this be used for something else? 
                                compile(paramNode, fnn, i, locals, labels);
                                prog[pc++] = opCodes["@ret user fn"].opCode;//return
                                prog[pc++] = 0;//TODO allow for anon functions with parameters
                                prog[lenPc] = pc - lenPc;
                            }
                        }
                        else if (fnn.fparams[i].ptype == vtype.label)
                        {
                            //always part of op code
                            string labelName = paramNode.getLeftBottomLeaf().name;

                            if (!labels.ContainsKey(labelName)) labels.Add(labelName, new bluntLabel(0));
                            opParams.Add(labels[labelName]);
                        }
                        else
                        {
                            compile(paramNode, fnn, i, locals, labels);
                        }
                    }
                }
                if (code == 4)
                {
                    prog[pc++] = opCodes["@call user fn"].opCode;
                    prog[pc++] = fnn.opCode;
                    prog[pc++] = nparams;
                    prog[pc++] = fnn.localStackUsage;  //not known yet for recursive function
           
                }
                else
                {
                    prog[pc++] = fnn.opCode;
                    //add any opcode parameters
                    foreach (bluntLabel l in opParams)
                    {
                        l.callees.Add(pc++);
                    }
                }
                //ideally pick version of op with or without return as reqd
                if (!fnn.retEmptyStack)
                {
                    if (n.parent.type == "CompilationUnit" || n.parent.type == "FN DECL" || n.parent.type == "BLOCK") prog[pc++] = opCodes["@dec sp"].opCode;//clear stack
                }
                else
                {
                    if (!(n.parent.type == "CompilationUnit" || n.parent.type == "FN DECL" || n.parent.type == "BLOCK")) prog[pc++] = opCodes["@inc sp"].opCode;//leave value on stack
                }
            }
            else if (n.type == "FN DECL")
            {
                int stackLocals = pc; //start of code block
                //set up parameters
                Dictionary<String, bluntVar> fnLocals = new Dictionary<String, bluntVar>();
                int nParams = (n.children.Count - 2) / 2;
                //	vtype ret = 
                bluntParam[] pTypes = new bluntParam[nParams];
                for (int p = 0; p < nParams; p++)
                {
                    pTypes[p] = new bluntParam();
                    astNode ptype = n.children[(p << 1) + 1];
                    if (ptype.name == "code")
                    {
                        pTypes[p].ptype = vtype.code;
                    }
                    else if (ptype.name == "num")
                    {
                        pTypes[p].ptype = vtype.num;
                    }
                    else
                    {
                        pTypes[p].ptype = vtype.obj;
                        pTypes[p].objectType = ptype.name;
                    }
                    astNode param = n.children[(p << 1) + 2];
                    pTypes[p].name = param.name;
                    fnLocals.Add(param.name, new bluntVar(p, pTypes[p].ptype,ptype.name));
                }
                //bluntFunction lf = new bluntFunction(n.name, pTypes, stackLocals);
                //methodCodes.Add(n.name, lf);
                bluntFunction lf = methodCodes[n.name];
                lf.opCode = stackLocals;
                
                
                //make place in stack for pushed return variables
                fnLocals.Add("@@ret_addr_place_holder", new bluntVar(nParams, vtype.num,null));
                fnLocals.Add("@@stack_frame_place_holder", new bluntVar(nParams + 1, vtype.num,null));
                fnLocals.Add("@@function_frame_place_holder", new bluntVar(nParams + 1, vtype.num,null));

                //create code block
                compile(n.children[n.children.Count - 1], lf, idx, fnLocals, new Dictionary<String, bluntLabel>());
   //             lf.localStackUsage = fnLocals.Count - nParams;
                //	prog[pc++]=0;//return
                prog[pc++] = opCodes["@ret user fn"].opCode;
                prog[pc++] = nParams;
             
                //	prog[stackLocals]=lf.localStackUsage;
                //store stack ptr

                //lambda fns call vars in stack above
                //they have ptrs relative to calling fn and not the closure
                /*
                 * for(intfn testfn postfn fn)
                 * {
                 * 		exec fn
                 * }
                 * a=20
                 * for( , , , a=a+1)
                 * 
                 * 
                 * push n
                 * push var at n
                 * pushrel n  stack push (fnbasesp+n)
                 * so stack always has 
                 */

            }
            else
            {
                compileChildren(n, fn, idx, locals, labels);
            }

            //update all label references - highly non optimal place to do it !!
            foreach (bluntLabel label in labels.Values)
            {
                foreach (int addr in label.callees)
                {
                    prog[addr] = label.address - addr;
                }
            }
        }

        public void getMethods(astNode n, Dictionary<String, bluntVar> fnLocals)
        {
            //get declared fuction signatures and local stack space
            if (n.type == "ID" && fnLocals!=null)
            {
                if (!fnLocals.ContainsKey(n.name))
                {
                    fnLocals.Add(n.name, new bluntVar(fnLocals.Count, vtype.num,null));
                }
            }
            else if (n.type == "FN DECL")
            {
                fnLocals = new Dictionary<String, bluntVar>();
                int nParams = (n.children.Count - 2) / 2;
                //	vtype ret = 
                bluntParam[] pTypes = new bluntParam[nParams];
                for (int p = 0; p < nParams; p++)
                {
                    pTypes[p] = new bluntParam();
                    astNode ptype = n.children[(p << 1) + 1];
                    if (ptype.name == "code")
                    {
                        pTypes[p].ptype = vtype.code;
                    }
                    else if (ptype.name == "num")
                    {
                        pTypes[p].ptype = vtype.num;
                    }
                    else
                    {
                        pTypes[p].ptype = vtype.obj;
                        pTypes[p].objectType = ptype.name;
                    }
                    astNode param = n.children[(p << 1) + 2];
                    pTypes[p].name = param.name;
                    fnLocals.Add(param.name, new bluntVar(p, pTypes[p].ptype,ptype.name));
                }
               
                getMethods(n.children[n.children.Count - 1], fnLocals);
                  
                // don't know location of function yet
                bluntFunction lf = new bluntFunction(n.name, pTypes, -1);
                lf.localStackUsage = fnLocals.Count - nParams+3;
                
                methodCodes.Add(n.name, lf);
            }
            else if (n.children != null)
            {
                for (int i = 0; i < n.children.Count; i++)
                {
                    getMethods(n.children[i], fnLocals);
                }
            }
        }
        public void list(int[] bytecode)
        {
            StringBuilder sb = new StringBuilder();
            Dictionary<int, bluntFunction> RopCodes = new Dictionary<int, bluntFunction>();

            foreach (bluntFunction bf in opCodes.Values)
            {
                RopCodes.Add(bf.opCode, bf);
            }
            sb.Append("\r\npc\top\tmnemonic\r\n");

            //assumes valid bytecode

            for (int i = 1; i < bytecode.Length; i++)
            {
                bluntFunction fn = RopCodes[bytecode[i]];
                sb.Append("" + i + "\t" + bytecode[i] + "\t" + fn.name + " ");
                for (int p = 1; p < fn.opLen; p++)
                {
                    sb.Append(" " + bytecode[++i]);
                }
                sb.Append("\r\n");
            }
            Console.Write(sb.ToString());
        }
    }

}
