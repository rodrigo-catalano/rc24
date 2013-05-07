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
 //       Dictionary<String, bluntFunction> methodCodes = new Dictionary<String, bluntFunction>();
        Dictionary<String, bluntFunction> ExternalFunctions = new Dictionary<String, bluntFunction>();
        functionList functions = new functionList();
       
        Dictionary<String, List<int>> StringConstants = new Dictionary<string, List<int>>();

        public compiler()
        {
            //get opcodes from vm
            opCodes = new BluntInterp4().opCodes;

        }
        public void run(int[] inputs,bool trace,List<object> objects)
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
       
            foreach(var f in opCodes.Keys) functions.Add(f,opCodes[f]);
            foreach (var f in externalFunctions.Values) functions.Add(f);
             //entry point is main
                       
            pc+=6;

            getMethods(n);
            inferTypes(n, null,null);

            compile(n, null, 0, rootlocals, new Dictionary<String, bluntLabel>());
            //prog[0] = rootlocals.Count;
            //call main function - could really be avoided by calling directly 
            //but entry point needs to be stored somewhere
            prog[0] = 0;
            prog[1] = opCodes["@call user fn"].opCode;

            //call main function, assume only one
            var main=functions.Values.First(t => t.name == "main");
            prog[2] = main.opCode;
            prog[3] = main.fparams.Length - 1;
            prog[4] = main.localStackUsage;
            prog[5] = opCodes["@ret"].opCode;

            //update addresses for forward declared functions
            foreach (var f in functions.Values)
            {
                foreach (int i in f.callees) prog[i] = f.opCode;
            }

            //link in string constants
            foreach (string s in StringConstants.Keys)
            {
                // link all references to string
                foreach (int location in StringConstants[s])
                {
                    prog[location]=pc;
                }
                //for now store as UTF-8, maybe change to more efficient packing
                byte[] sbytes = Encoding.UTF8.GetBytes(s); 
                //write string byte length
                int nbytes = sbytes.Length;
                prog[pc++] = nbytes;
                //pack into int[] - a bit silly
                int i=0;
                while (i<nbytes)
                {
                    byte[] bytes = new byte[4];
                    bytes[3] = sbytes[i++];
                    if (i < nbytes) bytes[2] = sbytes[i++];
                    if (i < nbytes) bytes[1] = sbytes[i++];
                    if (i < nbytes) bytes[0] = sbytes[i++];

                    prog[pc++] = BitConverter.ToInt32(bytes, 0);
                }
            }

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
                 //   locals.Add(n.name, new bluntVar(locals.Count, vtype.num,null));
                }
                if (fn != null && fn.fparams[idx+1].ptype == vtype.vref)
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
                prog[pc++] = opCodes["@push immediate"].opCode;
                prog[pc++] = int.Parse(n.val);	//val on stack
            }
            else if (n.type == "LABEL")
            {             
                labels[n.name].address = pc;
            }
            else if (n.type == "ARRAY")//load id on stack
            {
                // stack should have array ptr then index 
                // should know size of array items 
                compileChildren(n, fn, idx, locals, labels);
                prog[pc++] = opCodes["@get_array_item"].opCode;


            }
            else if (n.type == "STRING")
            {
                //store string constant somewhere avoiding duplicates
                if (!StringConstants.ContainsKey(n.val))
                {
                    StringConstants.Add(n.val, new List<int>());
                }
                prog[pc++] = opCodes["@push immediate"].opCode;
                // save pc for linking stage when location of string is known
                StringConstants[n.val].Add(pc++);
                
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
                
                bluntFunction fnn;
                if (functions.ContainsKey(n.name))
                {
                    fnn = functions[n.name];
                }
                else
                {
                    compilerError(n,"Unknown function " + n.name);
                    return;
                }

                
                if (fnn.name == "return")
                {
                    compileChildren(n, fnn, idx, locals, labels);
                    prog[pc++] = functions["@ret user fn"].opCode;
                    prog[pc++] = fn.fparams.Length-1;
                    return;

                }
                
                int nparams = 0;
                if (n.children != null)
                {
                    nparams = n.children.Count;
                    for (int i = 0; i < n.children.Count; i++)
                    {
                        astNode paramNode = n.children[i];
                        //if param should be a lambda
                        if (fnn.fparams[i+1].ptype == vtype.code)
                        {
                            //bodge to check for lambda ptr
                            astNode pNode = paramNode.children[0];
                            //if parameter is already lambda ptr - just copy ptr
                               
                            if (pNode != null && pNode.type == "ID" && locals[pNode.name].type == vtype.code)
                            {
                                compile(paramNode, fnn, i, locals, labels);
                            }
                            else
                            {
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
                        else if (fnn.fparams[i+1].ptype == vtype.label)
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
                if (fnn.functionType==fType.SCRIPT)
                {
                    prog[pc++] = opCodes["@call user fn"].opCode;
                    fnn.callees.Add(pc);//allow forward function declarations
                    prog[pc++] = fnn.opCode;
                    prog[pc++] = nparams;
                    prog[pc++] = fnn.localStackUsage;  
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
                bluntFunction lf = (bluntFunction)n.context;
                lf.opCode = stackLocals;
         
                //create code block
                compile(n.children[n.children.Count - 1], lf, idx, lf.locals, lf.labels);
   //             lf.localStackUsage = fnLocals.Count - nParams;
                //	prog[pc++]=0;//return
                prog[pc++] = opCodes["@ret user fn"].opCode;
                prog[pc++] = lf.fparams.Length-1;

                foreach (bluntLabel label in lf.labels.Values)
                {
                    foreach (int addr in label.callees)
                    {
                        prog[addr] = label.address - addr;
                    }
                }
            }
            else
            {
                compileChildren(n, fn, idx, locals, labels);
            }

            //update all label references - highly non optimal place to do it !!
            
        }

        private bluntParam getParam(astNode node)
        {
            bluntParam ret = new bluntParam();
            if (node.name == "code")
            {
                ret.ptype = vtype.code;
            }
            else if (ret.name == "num")
            {
                ret.ptype = vtype.num;
            }
            else
            {
                ret.ptype = vtype.obj;
            }
            ret.objectType = node.name;
            return ret;
        }
/*
        public static void fixDotNotation(astNode n)
        {
            //probably could be done in parser
            //convert dot function to function call or property getter/setter
            if (n.children == null) return;

            if (n.name == "=" && n.children[0].name == ".")
      //      if (n.name == "=" && n.children[0].type == "FN")
            {
                //setter or 
                // a.getArrayItem(10)=23
                // setArrayItem(a,10,23)
                var dot = n.children[0];
                var fn = dot.children[1];
                //replace = node with function
                var val = n.children[1];
                n.children.Clear();
                n.children.Add(dot.children[0]); //object param

                for (int c = 1; c < fn.children.Count; c++) n.children.Add(fn.children[c]);
                n.children.Add(val); //value param
                if (dot.children[1].type == "FN") n.name = fn.name.Replace("get", "set");//array
                else n.name = "set" + fn.name;//property setter
            }
            for (int i = 0; i < n.children.Count; i++)
            {
                var child = n.children[i];
                if (child.name == ".")
                {
                    //replace node with right of dot as function with left of dot as first parameter
                    if (child.children[1].type == "FN")//method call
                    {
                        var fn = child.children[1];
                        fn.children.Insert(0, child.children[0]);//add object as first parameter
                        //replace dot node with function
                        n.children[i] = fn;
                        child = fn;
                        
                    }
                    else
                    {
                        //getter
                        var fn = child.children[1];
                        fn.children.Insert(0, child.children[0]);//add object as first parameter
                        //replace dot node with function
                        fn.name = "get" + fn.name;
                        fn.type = "FN";
                        n.children[i] = fn;
                        child = fn;
                 
                    }
                }
                fixDotNotation(child);
            }
 
             
        }
 */

        public static void fixDotNotation(astNode n)
        {
            //probably could be done in parser
            //convert dot function to function call or property getter/setter
            //children first to cope with nested dots
            if (n.children != null) 
            {
                //this can change children but not the number
                for (int i = 0; i < n.children.Count;i++ ) fixDotNotation(n.children[i]);
            }
            //.(a,b())=> b(a)
            //.(a,b)=> getb(a)
            if (n.name == ".")
            {
                var fn = n.children[1];
                var obj = n.children[0];

                fn.children.Insert(0,obj);
                if (fn.type == "ID")
                {
                    //property
                    fn.type = "FN";
                    fn.name = "get" + fn.name;
                }

                int i=n.parent.children.IndexOf(n);
                n.parent.children.RemoveAt(i);
                n.parent.children.Insert(i, fn);
            }
            //getx(a)=b => setx(a,b)
            if (n.name == "=")
            {
                if (n.children[0].type == "FN")
                {
                    //=(getx(tx),Value) => setx(tx,value)
                    //build setter
                    var fn = n.children[0];
                    fn.name = fn.name.Replace("get", "set");
                    fn.children.Add(n.children[1]);

                    int i = n.parent.children.IndexOf(n);
                    n.parent.children.RemoveAt(i);
                    n.parent.children.Insert(i, fn);
                }
            }

        }


        // pass 1
        public void getMethods(astNode n)
        {
            //get declared function signatures and local stack space
            if (n.type == "FN DECL")
            {
                int nParams = (n.children.Count - 2) / 2+1;

                bluntParam[] pTypes = new bluntParam[nParams];
                //return type
                pTypes[0] = getParam(n.children[0]);
                bluntFunction lf = new bluntFunction(n.name, pTypes, -1);
                
                for (int p = 1; p < nParams; p++)
                {
                    astNode ptype = n.children[(p << 1) - 1];
                    pTypes[p] = getParam(ptype);
                   
                    astNode param = n.children[(p << 1) ];
                    pTypes[p].name = param.name;
                    lf.locals.Add(param.name, new bluntVar(p-1, pTypes[p].ptype, ptype.name) { typename = pTypes[p].objectType });
                }
                getLabels(n, lf.labels);
                lf.locals.Add("@@ret_addr_place_holder", new bluntVar(nParams-1, vtype.num, null));
                lf.locals.Add("@@stack_frame_place_holder", new bluntVar(nParams , vtype.num, null));
                lf.locals.Add("@@function_frame_place_holder", new bluntVar(nParams + 1, vtype.num, null));

                //find stack usage for local vars
                getLocals(n.children[n.children.Count - 1], lf);
                  
                lf.localStackUsage = lf.locals.Count - (nParams-1);
                lf.functionType = fType.SCRIPT;
                if(functions.ContainsKey(lf.mangledName))
                {
                    compilerError(n, "function "+ n.name + " with same parameters already declared");
                }
                else functions.Add(lf);
                n.context = lf;
            }
            else if (n.children != null)
            {
                foreach(var child in n.children)getMethods(child);
            }
        }
        public void getLocals(astNode n, bluntFunction fn)
        {
            //get local stack space
            if (n.type == "ID" && fn.locals!=null)
            {
                if (!fn.labels.ContainsKey(n.name) && !fn.locals.ContainsKey(n.name))
                {
                    fn.locals.Add(n.name, new bluntVar(fn.locals.Count, vtype.num,null));
                }
            }
            else if (n.children != null)
            {
                foreach(var child in n.children)getLocals(child, fn);
            }
        }
        //pass 2 get types
        public void inferTypes(astNode n, Dictionary<String, bluntVar> locals, Dictionary<String, bluntLabel> labels)
        {
            switch (n.type)
            {
                case "ID":
                    if (labels.ContainsKey(n.name))
                    {
                        n.typename = "label";
                    }
                    else if(locals.ContainsKey(n.name))
                    {
                        n.typename = locals[n.name].typename; 
                    }
                    if(n.typename==null) compilerError(n,"Variable "+n.name+" used without being assigned");
                    break;
                case "CONST": n.typename="num"; break;
                case "STRING": n.typename="string"; break;
                case "LABEL": n.typename="label"; break;
                case "FN":
                    if (n.name == "=")//assignments are where type is defined
                    {
                        string varname = n.children[0].name;
                        astNode valNode=n.children[1];
                        inferTypes(valNode,locals,labels);
                        //check type is not changed if var already defined
                        if (locals[varname].typename!=null)
                        {
                            if (locals[varname].typename != valNode.typename)
                                compilerError(n.children[0],"Variable type changed");
                        }
                        else
                        {
                            locals[varname].typename=n.children[1].typename;
                        }
                        n.typename=n.children[1].typename;
                        //assign needs some thought
                        n.name = "=__num_vref_num";
                    
                    }
                    else
                    {
                        //get types of all parameters
                        if (n.children != null)
                            foreach (var child in n.children) inferTypes(child,locals,labels);
                        // now find matching function and hence return type
                        // the parser can't tell the difference between a 'code' parameter and any other type
                        // so we can't simply look up mangled name
                        bluntFunction func = null;
                        bool functionNameFound = false;
                        foreach (bluntFunction fn in functions.Values)
                        {
                            if (fn.name == n.name )//&& fn.fparams.Length - 1 == n.children.Count)
                            {
                                functionNameFound = true;
                                bool match = true;
                                int constants = 0;
                                for (int i = 1; i < fn.fparams.Length; i++)
                                {
                                    
                                    var p = fn.fparams[i];
                                    if (p.ptype == vtype.constant)
                                    {
                                        constants++;
                                        continue;
                                    }
                                    if (n.children.Count < i - 1 - constants-1)
                                    {
                                        match = false;
                                        break;
                                    }
                                    if (p.typeName == "code" || p.typeName == n.children[i - 1-constants].typename)
                                    {
                                    }
                                    else match = false;
                                }
                                if (fn.fparams.Length - 1 != n.children.Count + constants) match = false;
                                if (match)
                                {
                                    func = fn;
                                    for (int i = 1; i < fn.fparams.Length; i++)
                                    {
                                        if (fn.fparams[i].ptype == vtype.constant)
                                        {
                                            astNode cons = new astNode() { val = fn.fparams[i].constantValue.ToString(), type = "CONST", typename = "const" };
                                            n.children.Insert(i - 1, cons);

                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        if (func == null)
                        {
                            if(functionNameFound)compilerError(n, "Wrong parameters for function "+ n.name);
                            else compilerError(n, "Unknown function " + n.name);
                        }
                        else
                        {
                            n.typename = func.fparams[0].typeName;
                            n.name = func.mangledName;
                        }
                        /*
                        if (functions.ContainsKey(n.name))
                        {
                            bluntFunction fn = functions[n.name];
                            if (fn.fparams.Length - 1 != n.children.Count) compilerError(n, "Wrong number of function parameters for function " + n.name);
                            for (int i = 1; i < fn.fparams.Length; i++)
                            {
                                var p = fn.fparams[i];
                                if (p.typeName == "code" || p.typeName == n.children[i-1].typename)
                                {

                                }
                                else compilerError(n, "Wrong type of parameter "+i +" for function " + n.name);
                            }
                            n.typename = fn.fparams[0].typeName;
                            n.name = fn.mangledName;

                        }
                        else compilerError(n, "Unknown function "+ n.name);
                         */
                    }
                    break;
                case "FN DECL":
                    //process body of function
                    inferTypes(n.children[n.children.Count - 1], ((bluntFunction)n.context).locals, ((bluntFunction)n.context).labels);
                
                    break;
                default : 
                    if (n.children != null)
                    {
                        foreach(var child in n.children)inferTypes(child,locals,labels);
                        n.typename = n.children[0].typename;
                    }
                    break;
            }
        }
        public void getLabels(astNode node, Dictionary<String, bluntLabel>labels)
        {
            if (node.type == "LABEL") labels.Add(node.name, new bluntLabel());
            if (node.children != null)
            {
                foreach(var child in node.children)getLabels(child,labels);
            }
        }
        public void compilerError(astNode node,string message)
        {
            //for now throw first error
            //  TODO log a number of errors before throwing
            var e = new Exception(message);
            e.Data.Add("line", node.line);
            throw e;
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
                try
                {
                    bluntFunction fn = RopCodes[bytecode[i]];
                    sb.Append("" + i + "\t" + bytecode[i] + "\t" + fn.name + " ");
                    for (int p = 1; p < fn.opLen; p++)
                    {
                        sb.Append(" " + bytecode[++i]);
                    }
                    sb.Append("\r\n");
                }
                catch (Exception)
                {
                    sb.Append("ERROR" + bytecode[i] +"\r\n");
                }
            }
            Console.Write(sb.ToString());
        }


        public string listMethods()
        {
            return listMethods(opCodes)+
                    listMethods(ExternalFunctions);
        }
        public string listMethods(Dictionary<String, bluntFunction> methods)
        {
            StringBuilder ret = new StringBuilder();
            foreach(string name in methods.Keys)
            {
                bluntFunction f = methods[name];

                if (f.fparams.Length > 0) ret.Append(f.fparams[0].typeName + " ");
                ret.Append(name + "(");
                bool first = true;
                for (int i = 1; i < f.fparams.Length; i++)
                {
                    bluntParam p = f.fparams[i];
                    if (p.ptype != vtype.constant)
                    {
                        if (!first)
                        {
                            ret.Append(", ");
                        }
                        first = false;

                         ret.Append(p.typeName + " " + p.name);
                    }
                }
                //f.fparams[0].ptype

                ret.Append(")\r\n");
            }

            return ret.ToString();
        }
    }

}
