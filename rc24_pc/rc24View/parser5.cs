using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    class parser5
    {
        Dictionary<string, Op> ops = new Dictionary<string, Op>();
        public int line = 1;
        string src;
        List<astNode> tokens;
        public parser5(string source)
        {
            ops.Add("=", new Op() { precidence = 1, leftAssociative = false, nParams = 2 });
            
            ops.Add("==", new Op() { precidence = 2, leftAssociative = true, nParams = 2 });
            ops.Add("!=", new Op() { precidence = 2, leftAssociative = true, nParams = 2 });

            ops.Add("<", new Op() { precidence = 3, leftAssociative = true, nParams = 2 });
            ops.Add(">", new Op() { precidence = 3, leftAssociative = true, nParams = 2 });
            ops.Add("<=", new Op() { precidence = 3, leftAssociative = true, nParams = 2 });
            ops.Add(">=", new Op() { precidence = 3, leftAssociative = true, nParams = 2 });
            
            ops.Add("+", new Op() { precidence = 4, leftAssociative = true,  nParams = 2 });
            ops.Add("-", new Op() { precidence = 4, leftAssociative = true,  nParams = 2 });
            ops.Add("*", new Op() { precidence = 5, leftAssociative = true,  nParams = 2 });
            ops.Add("/", new Op() { precidence = 5, leftAssociative = true,  nParams = 2 });
            ops.Add("%", new Op() { precidence = 5, leftAssociative = true,  nParams = 2 });
            ops.Add("!", new Op() { precidence = 6, leftAssociative = false, nParams = 1 });

            ops.Add(".", new Op() { precidence = 20, leftAssociative = true, nParams = 2 });
            ops.Add("neg", new Op() { precidence = 10, leftAssociative = false, nParams = 1 });
          
            src = source;
        }
        public astNode parse()
        {
            astNode cu = new astNode() { type = "CompilationUnit" };
            tokens=getTokens();
            int idx = 0;
            while (idx < tokens.Count)
            {
                cu.children.Add(parseFunctionDecl(ref idx));
            }
            setParents(cu);
            return cu;
        }
        private void setParents(astNode parent)
        {
            foreach (astNode child in parent.children)
            {
                child.parent = parent;
                setParents(child);
            }
        }
        private List<astNode> getTokens()
        {
            List<astNode>tokens=new List<astNode>();
            int idx=0;
            astNode n;
            while((n=getToken(ref idx))!=null)
            {
                if(n.name=="[")
                {
                    tokens.Add(new astNode(){name=".",type="OP",line=n.line});
                    tokens.Add(new astNode(){name="getArrayItem",type="FN",line=n.line});
                    tokens.Add(new astNode(){name="(",type="OP",line=n.line});
                }
                else
                {
                    if(n.name=="]")n.name=")";
                    tokens.Add(n);
                }
            }
            return tokens;
        }
        private string peekChar(int idx)
        {
            idx = eatSpace(idx);
            if (idx == src.Length) return "";
            return src[idx].ToString();
           
        }
        private astNode getToken(ref int idx)
        {
            idx = eatSpace(idx);
            if (idx == src.Length) return null;
            var ret=new astNode();
            StringBuilder  token = new StringBuilder();
           
            ret.line = line;
            char c = src[idx];

            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')|| c=='@')
            {
                while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='@')
                {
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                if (c == '#')
                {
                    ret.type = "LABEL";
                    idx++;
                }
                else if (c == ':')
                {
                    ret.type = "PARAMNAME";
                    idx++;
                }
                
                else if (peekChar(idx) == "(") ret.type = "FN";
                else ret.type = "ID";
            }
            else if (c >= '0' && c <= '9')
            {
                while ((c == '.') || (c >= '0' && c <= '9'))
                {
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                ret.type = "CONST";
                ret.val = token.ToString();
            }
            else if (c == '"')
            {
                //strings can cross lines and " is escaped by "" rather like c# @"
                //maybe strip whitespace up to tab position of first line to keep source indented
                idx++;
                c = src[idx];
                while (true)
                {
                    if(c=='"')
                    {
                        if(src[++idx]!='"')break;
                    }
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                ret.type = "STRING";
                ret.val = token.ToString();
             }
            else
            {
                ret.type = "OP";
                //for now do simple ops
                token.Append(c);
                string dt=null;
                if (src.Length > idx+1)
                {
                    dt = src.Substring(idx,2);
                }
                if(dt!=null && ops.ContainsKey(dt))
                {
                    token.Append(src[++idx]);
                }
                else if (ops.ContainsKey(token.ToString()))
                {
                }
                idx++;
            }
            ret.name=token.ToString();
            return ret;
        }
        private int eatComment(int idx)
        {
            if (idx < src.Length - 1)
            {
                if (src[idx] == '/' && src[idx + 1] == '/')
                {
                    idx+=2;
                    while (idx < src.Length && src[idx] != '\r')
                    {
                        idx++;
                    }
                }
            }
            return idx;
        }
        private int eatSpace(int idx)
        {
            idx = eatComment(idx);
            while (idx < src.Length && (src[idx] == ' ' || src[idx] == ' ' || src[idx] == '\t' || src[idx] == '\r' || src[idx] == '\n'))
            {
                if (src[idx] == '\r') line++;
                idx++;
                idx = eatComment(idx);
            }
            return idx;
        }
        private astNode parseFunctionDecl(ref int idx)
        {
            astNode fndef = new astNode() { type = "FN DECL" };
            astNode t = tokens[idx++];
            if (t.type !="ID") throw new Exception("return type expected for function declaration");
            fndef.children.Add(new astNode() { type = "ID", name = t.name });
            t = tokens[idx++];
            fndef.name = t.name;
            t = tokens[idx++];
            if (t.name != "(") throw new Exception("( expected for function declaration");

            while ((t = tokens[idx++]).name != ")")
            {
                if(t.name!=",") fndef.children.Add(new astNode() { type = "ID", name = t.name });
            }
            t = tokens[idx++];
            if (t.name != "{") throw new Exception("{ expected for function body declaration");

            // now create node for fn body
            astNode fnBody = toTree(ref idx);
            fndef.children.Add(fnBody);
            
            return fndef;
        }
        private void popFunction(Stack<astNode> outp, Stack<astNode> opStack)
        {
            astNode t = opStack.Pop();
     //       if (t.type == "OP" && t.name == "") return;
            astNode n = new astNode() { name = t.name, type = "FN",line=t.line };
            if (t.type == "OP")
            {
                for (int i = 0; i < ops[t.name].nParams; i++)
                {
                    n.children.Insert(0, outp.Pop());
                }
            }
            else
            {
                //maybe zero parameters
                astNode param = null;// new astNode() { name = "PARAM" };
                while (outp.Peek().name != "FN_START")
                {
                    if(outp.Peek().name == ",")
                    {
                        n.children.Insert(0, param);
                        param = new astNode() { name = "PARAM" };
                        outp.Pop();
                    }
                    if (param == null) param = new astNode() { name = "PARAM" };
                    if (outp.Peek().type == "PARAMNAME") param.val = outp.Pop().val;
                    else param.children.Insert(0, outp.Pop());
                    
                }
                if (param != null) n.children.Insert(0, param);
                outp.Pop();
            }
            outp.Push(n);
           
        }
        
        private astNode toTree(ref int idx)
        {
            // shunting yard
            Stack<astNode> outp = new Stack<astNode>();
            Stack<astNode> opStack = new Stack<astNode>();

            astNode sentinel = new astNode() { type = "OP", name=""};
            opStack.Push(sentinel);

            astNode top;
            astNode t = null;
            astNode last;

            while (idx < tokens.Count)
            {
                last = t;
                t = tokens[idx++];
           
                if (t.type=="EOF" || t.name == "}")
                {
                    while (opStack.Count > 1)
                    {
                        popFunction(outp, opStack);
                    }
                    break;
                }
                //handle unary - op. subtraction op can't follow op or ( or ,
                if(last!=null && t.name=="-" && (last.name=="(" || last.name=="," || last.type=="OP"))
                {
                    t.name = "neg";
                    //if next op is a numeric constant, negate it and replace token to save negation operation
                    var next = tokens[idx];
                    if (next != null && next.type == "CONST")
                    {
                        t = tokens[idx++];
                        t.name = "-" + t.name;
                        t.val = "-" + t.val;
                    }
                }

                //detect end of statement avoiding the need for ; 

           
                //if(t.name==";")
                //new expression starts with ID or function or label and end with ) or const or id or label
                if (last != null && (t.type == "ID" || t.type == "FN" || t.type == "LABEL") && (last.type == "ID" || last.type == "CONST" || last.type == "LABEL" || last.name == ")"))
                {
                    while (opStack.Count > 1 && opStack.Peek().name != "(" )
                    {
                        popFunction(outp, opStack);
                    }
                }
                if (t.type == "ID")outp.Push(t);
                else if (t.type =="STRING") outp.Push(t);
                else if (t.type == "CONST") outp.Push(t);
                else if (t.type == "LABEL") outp.Push(t);
                else if (t.type == "PARAMNAME") outp.Push(t);
                else if (t.type == "FN")
                {
                    opStack.Push(t);
                    outp.Push(new astNode(){name="FN_START"});
                }
                else if (t.type == "OP")
                {
                    if (t.name == ",")
                    {
                        while (opStack.Peek().name != "(")
                        {
                            popFunction(outp, opStack);
                        }
                        outp.Push(new astNode() { name = "," });
                    }
                    else if (t.name == "(")
                    {
                        opStack.Push(t);
                    }
                    else if (t.name == ")")
                    {
                        while (opStack.Peek().name != "(")
                        {
                            popFunction(outp, opStack);
                        }
                        opStack.Pop();
                        if (opStack.Peek().type == "FN")
                        {
                            popFunction(outp, opStack);
                        }
                    }
                    else
                    {
                        top = opStack.Peek();
                        Op topop=null;
                        if(ops.ContainsKey(top.name))topop=ops[top.name];
                       
                        var cop=ops[t.name];
                        while (top.type == "OP" && topop != null && ((cop.leftAssociative && cop.precidence <= topop.precidence) || (!cop.leftAssociative && cop.precidence < topop.precidence)))
                        {
                            popFunction(outp, opStack);
                            top = opStack.Peek();
                            if (ops.ContainsKey(top.name)) topop = ops[top.name];
                            else topop = null;
                       
                        }
                        opStack.Push(t);
                    }
                }
            }
            astNode ret = new astNode();
            ret.name = "BLOCK";
            ret.type = "BLOCK";
            while (outp.Count > 0) ret.children.Insert(0, outp.Pop());
            
            return ret;
        }
    }
}