using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
/*
num foo( num a, num b)
{
    
  return (a+b)
}
*/

    class parser
    {
        Dictionary<string, Op> ops = new Dictionary<string, Op>();
        public int line = 0;
        string src;
        public parser(string source)
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
          
            
            //  ops.Add("[", new Op() { precidence = 10, leftAssociative = false, nParams = 1 });
          //  ops.Add("[", new Op() { precidence = 10, leftAssociative = false, nParams = 1 });
          //  ops.Add("]=", new Op() { precidence = 10, leftAssociative = false, nParams = 2 });
            
            src = source;
        }
        public astNode parse()
        {
            astNode cu = new astNode() { type = "CompilationUnit" };
            int idx = 0;
            while (idx < src.Length)
            {
                cu.children.Add(parseFunctionDecl(ref idx));
                idx=eatSpace(idx);
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
        private astNode parseFunctionDecl(ref int idx)
        {
            astNode fndef = new astNode() { type = "FN DECL" };
            Token t = getToken(ref idx);
            if (t.type != Token.ttype.ID) throw new Exception("return type expected for function declaration");
            fndef.children.Add(new astNode() { type = "ID", name = t.name });
            t = getToken(ref idx);
            fndef.name = t.name;
            t = getToken(ref idx);
            if (t.name != "(") throw new Exception("( expected for function declaration");

            while ((t = getToken(ref idx)).name != ")")
            {
                if(t.name!=",") fndef.children.Add(new astNode() { type = "ID", name = t.name });
            }
            t = getToken(ref idx);
            if (t.name != "{") throw new Exception("{ expected for function body declaration");

            // now create node for fn body
            astNode fnBody = toRPN(ref idx);
            fndef.children.Add(fnBody);
            
            return fndef;
        }
        private Token peekToken(int idx)
        {
            int sidx = idx;
            return getToken(ref sidx);
        }

        private Token getToken(ref int idx)
        {
            Token ret=new Token();
            StringBuilder  token = new StringBuilder();
            idx = eatSpace(idx);
            if (idx == src.Length) return new Token() { type = Token.ttype.EOF };
            
            char c = src[idx];

            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')|| c=='@')
            {
                while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='@')
                {
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                if (peekToken(idx).name == ":")
                {
                    ret.type = Token.ttype.LABEL;
                    idx++;
                }
                else if (peekToken(idx).name == "(") ret.type = Token.ttype.FUNCTION;
                else ret.type = Token.ttype.ID;
            }
            else if (c >= '0' && c <= '9')
            {
                while ((c == '.') || (c >= '0' && c <= '9'))
                {
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                ret.type = Token.ttype.NUMBER;
                
            }
            else if (c == '"')
            {
                idx++;
                c = src[idx];
                while (c != '"')
                {
                    token.Append(c);
                    idx++;
                    c = src[idx];
                }
                ret.type = Token.ttype.STRING;
                idx++;
            }
            else
            {
                ret.type = Token.ttype.OP;
                //for now do simple ops
                token.Append(c);
                string dt=null;
                if (src.Length > idx+1)
                {
                    dt = src.Substring(idx,2);
                }
                if(dt!=null && ops.ContainsKey(dt))
                {
                    ret.op = ops[dt];
                    token.Append(src[++idx]);
                }
                else if (ops.ContainsKey(token.ToString()))
                {
                    ret.op = ops[token.ToString()];
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

        private void popFunction(Stack<astNode> outp, Stack<Token> opStack)
        {
            Token t = opStack.Pop();
            astNode n = new astNode() { name = t.name, type = "FN" };
            if (t.type == Token.ttype.OP)
            {
                for (int i = 0; i < t.op.nParams; i++)
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
                    param.children.Insert(0, outp.Pop());
                    
                }
                if (param != null) n.children.Insert(0, param);
                outp.Pop();
            }
            outp.Push(n);
           
        }
        
        private astNode toRPN(ref int idx)
        {
            // shunting yard
            Stack<astNode> outp = new Stack<astNode>();
            Stack<Token> opStack = new Stack<Token>();

            Token sentinel = new Token() { type = Token.ttype.OP, name=""};
            sentinel.op = new Op() { precidence = 0 };
            opStack.Push(sentinel);

            Token top;
            Token t = null;
            Token last;

            while (idx < src.Length)
            {
                last = t;
                t = getToken(ref idx);
           
                if (t.type==Token.ttype.EOF || t.name == "}")
                {
                    while (opStack.Count > 1)
                    {
                        popFunction(outp, opStack);
                    }
                    break;
                }
                //handle unary - op. subtraction op can't follow op or ( or ,
                if(last!=null && t.name=="-" && (last.name=="(" || last.name=="," || last.op!=null))
                {
                    t.op = ops["neg"];
                    t.name = "neg";
                    //if next op is a numeric constant, negate it and replace token to save negation operation
                    Token next = peekToken(idx);
                    if (next != null && next.type == Token.ttype.NUMBER)
                    {
                        t = getToken(ref idx);
                        t.name = "-" + t.name;
                    }
                }

                //detect end of statement avoiding the need for ; 

           
                //if(t.name==";")
                //new expression starts with ID or function or label and end with ) or const or id or label
                if (last != null && (t.type == Token.ttype.ID || t.type == Token.ttype.FUNCTION || t.type == Token.ttype.LABEL) && (last.type == Token.ttype.ID || last.type == Token.ttype.NUMBER || last.type == Token.ttype.LABEL || last.name == ")"))
                {
                    while (opStack.Count > 1 && opStack.Peek().name != "(" )
                    {
                        popFunction(outp, opStack);
                    }
                }
                if (t.type == Token.ttype.ID)outp.Push(new astNode() { type = "ID", name = t.name });
                else if (t.type == Token.ttype.STRING) outp.Push(new astNode() { type = "STRING", val = t.name });
                else if (t.type == Token.ttype.NUMBER) outp.Push(new astNode() { type = "CONST", val = t.name });
                else if (t.type == Token.ttype.LABEL) outp.Push(new astNode() { type = "LABEL", name = t.name });
                else if (t.type == Token.ttype.FUNCTION)
                {
                    opStack.Push(t);
                    outp.Push(new astNode(){name="FN_START"});
                }
                else if (t.type == Token.ttype.OP)
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
                        if (opStack.Peek().type == Token.ttype.FUNCTION)
                        {
                            popFunction(outp, opStack);
                        }
                    }
                    else
                    {
                        top = opStack.Peek();
                        while (top.type == Token.ttype.OP && top.op != null && ((t.op.leftAssociative && t.op.precidence <= top.op.precidence) || (!t.op.leftAssociative && t.op.precidence < top.op.precidence)))
                        {
                            popFunction(outp, opStack);
                            top = opStack.Peek();
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
    class Op
    {
        public int precidence;
        public bool leftAssociative;
        public int nParams;
    }
    class Token
    {
        public string name;
        public enum ttype{ID,FUNCTION,LABEL,NUMBER,STRING,OP,EOF};
        public ttype type;
        public Op op;

    }
    public class astNode
    {
        public List<astNode> children=new List<astNode>();
        public string name;
        public string type;
        public string val;
        public astNode parent;

        public astNode getLeftBottomLeaf()
        {
            if (children == null || children.Count == 0) return this;
            return children[0].getLeftBottomLeaf();
        }
    }
}
