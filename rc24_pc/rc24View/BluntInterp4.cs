﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace bluntsharp
{
    public class BluntInterp4
    {

        private int[] _prog;
        private int[] _stack;
        private int _sp = 0;

        private int _freeBlocks;
        private int _usedBlocks;

        public Dictionary<String, bluntFunction> opCodes = new Dictionary<String, bluntFunction>();


        const int OP__ret = 0;
        const int OP__push_var_val = 1;
        const int OP__push_var_ref = 2;
        const int OP__push_immediate = 3;
        const int OP__push_lambda_ptr = 4;
        const int OP__jump = 5;
        const int OP__return = 6;
        const int OP__call_user_fn = 7;
        const int OP__ret_user_fn = 8;

        const int OP_assign = 9;
        const int OP_add = 10;
        const int OP_sub = 11;
        const int OP_multiply = 12;
        const int OP_divide = 13;
        const int OP_remainder = 14;
        const int OP_gt = 15;
        const int OP_lt = 16;
        const int OP_gte = 17;
        const int OP_lte = 18;
        const int OP_eq = 19;
        const int OP_ne = 20;

        const int OP_print = 21;
        const int OP_interp = 22;
        const int OP_dec = 23;

        const int OP__jz = 24;
        const int OP__jnz = 25;
        const int OP__j = 26;
        const int OP__dec_sp = 27;
        const int OP__inc_sp = 28;
        const int OP_get_array_item = 29;
        const int OP_create_array = 30;
        const int OP_neg = 31;

        public const int OP_get_routed_int16 = 32;
        public const int OP_set_routed_int16 = 33;
        public const int OP_get_routed_array_item = 34;
        public const int OP_set_routed_array_item = 35;

        const int PREV = 0;
        const int NEXT = -1;
        const int SIZE = -2;
        const int HEADER = 3;

        public bool trace = false;

        public BluntInterp4()
        {
            //
            opCodes.Add("@ret", new bluntFunction("@ret", new bluntParam[] { }, OP__ret));
            opCodes.Add("@push var val", new bluntFunction("@push var val", new bluntParam[] { }, OP__push_var_val, 0, false, 2));
            opCodes.Add("@push var ref", new bluntFunction("@push var ref", new bluntParam[] { }, OP__push_var_ref, 0, false, 2));
            opCodes.Add("@push immediate", new bluntFunction("@push immediate", new bluntParam[] { }, OP__push_immediate, 0, false, 2));
            opCodes.Add("@push lambda ptr", new bluntFunction("@push lambda ptr", new bluntParam[] { }, OP__push_lambda_ptr, 0, false, 2));
            opCodes.Add("@jump", new bluntFunction("@jump", new bluntParam[] { }, OP__jump, 0, false, 2));
            opCodes.Add("return", new bluntFunction("return", new bluntParam[] { new bluntParam(vtype.num) }, OP__return));
            opCodes.Add("@call user fn", new bluntFunction("@call user fn", new bluntParam[] { }, OP__call_user_fn, 0, false, 4));

            opCodes.Add("@jz", new bluntFunction("@jz", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.label, "to", ParamLocation.stack) }, OP__jz, 0, true, 2));
            opCodes.Add("jnz", new bluntFunction("jnz", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.label, "to", ParamLocation.stack) }, OP__jnz, 0, true, 2));
            opCodes.Add("@j", new bluntFunction("@j", new bluntParam[] { new bluntParam(vtype.label, "to", ParamLocation.stack) }, OP__j, 0, true, 2));
            opCodes.Add("@dec sp", new bluntFunction("@dec sp", new bluntParam[] { }, OP__dec_sp));
            opCodes.Add("@inc sp", new bluntFunction("@inc_sp", new bluntParam[] { }, OP__inc_sp));
            opCodes.Add("@ret user fn", new bluntFunction("@ret user fn", new bluntParam[] { }, OP__ret_user_fn, 0, false, 2));


            //core lib
            opCodes.Add("=", new bluntFunction("assign", new bluntParam[] { new bluntParam(vtype.vref), new bluntParam(vtype.num) }, OP_assign, 0, true));
            opCodes.Add("+", new bluntFunction("add", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_add));
            opCodes.Add("-", new bluntFunction("sub", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_sub));
            opCodes.Add("*", new bluntFunction("multiply", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_multiply));
            opCodes.Add("/", new bluntFunction("divide", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_divide));
            opCodes.Add("%", new bluntFunction("remainder", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_remainder));
            opCodes.Add(">", new bluntFunction(">", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_gt));
            opCodes.Add("<", new bluntFunction("<", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_lt));
            opCodes.Add(">=", new bluntFunction(">=", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_gte));
            opCodes.Add("<=", new bluntFunction("<=", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_lte));
            opCodes.Add("==", new bluntFunction("==", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_eq));
            opCodes.Add("!=", new bluntFunction("!=", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_ne));
            opCodes.Add("print", new bluntFunction("print", new bluntParam[] { new bluntParam(vtype.num) }, OP_print, 0, true));
            opCodes.Add("interp", new bluntFunction("interp", new bluntParam[] { new bluntParam(vtype.code) }, OP_interp));
            opCodes.Add("dec", new bluntFunction("dec", new bluntParam[] { new bluntParam(vtype.vref) }, OP_dec));
            opCodes.Add("neg", new bluntFunction("neg", new bluntParam[] { new bluntParam(vtype.num) }, OP_neg));

            opCodes.Add("get array item", new bluntFunction("get array item", new bluntParam[] { new bluntParam(vtype.vref), new bluntParam(vtype.num) }, OP_get_array_item, 0, true));

            opCodes.Add("OP_get_routed_int16", new bluntFunction("OP_get_routed_int16", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_get_routed_int16));
            opCodes.Add("OP_set_routed_int16", new bluntFunction("OP_set_routed_int16", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_set_routed_int16));
            opCodes.Add("OP_get_routed_array_item", new bluntFunction("OP_get_routed_array_item", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_get_routed_array_item));
            opCodes.Add("OP_set_routed_array_item", new bluntFunction("OP_set_routed_array_item", new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.num), new bluntParam(vtype.num), new bluntParam(vtype.num) }, OP_set_routed_array_item));


        }

        public void run(int[] prog, int[] stack, int sp, Dictionary<String, bluntFunction> oc, int[] inputs,object[] objects)
        {
            _prog = prog;
            _stack = stack;
            _sp = sp;
            opCodes = oc;

            //setup heap lists - not used yet

            _usedBlocks = stack.Length - 1;
            _stack[_usedBlocks + NEXT] = 0;
            _stack[_usedBlocks + PREV] = 0;
            _stack[_usedBlocks + SIZE] = 0;

            _freeBlocks = stack.Length - 4;
            _stack[_freeBlocks + NEXT] = 0;
            _stack[_freeBlocks + PREV] = 0;
            _stack[_freeBlocks + SIZE] = stack.Length / 2;

            if (inputs != null)
            {
                for (int i = 0; i < inputs.Length; i++)
                {
                    _stack[_sp++] = inputs[i];
                }
            }

            interp(0, 0,objects);
        }
        //in c prog becomes list of fn ptrs
        //
        void dumpStack(int pc, int sb, int fb)
        {
            StringBuilder strb = new StringBuilder();
            strb.Append(pc + ":");

            for (int i = 0; i < _sp; i++)
            {
                strb.Append(" " + _stack[i]);
            }

            foreach (bluntFunction f in opCodes.Values)
            {
                if (f.opCode == _prog[pc]) strb.Append(" - " + f.name);
            }

            strb.Append(" - " + _prog[pc + 1] + " sb " + sb + " fb " + fb);
            Console.WriteLine(strb.ToString());
        }

        void interp(int pc, int stackBase,object[]objects)
        {
            int nparams;
            int functionBase = stackBase;
            _sp += _prog[pc++]; //make room for locals - probably redundant now
            if (trace) Console.Out.WriteLine(" interp ");

            while (_prog[pc] != OP__ret)
            {
                if (trace) dumpStack(pc, stackBase, functionBase);
                switch (_prog[pc++])
                {
                    case OP__push_var_val:
                        _stack[_sp++] = _stack[stackBase + _prog[pc++]];//put var val  on stack
                        break;
                    case OP__push_var_ref:
                        _stack[_sp++] = stackBase + _prog[pc++];//put var ref on stack or const
                        break;
                    case OP__push_immediate:
                        _stack[_sp++] = _prog[pc++];//put immediate on stack
                        break;
                    case OP__push_lambda_ptr:
                        _stack[_sp++] = _prog[pc++] + (stackBase << 16);//put lambda on stack with stackbase
                        break;
                    case OP__jump: pc += (_prog[pc]);//jump
                        break;
                    case OP__return:
                        break;

                    case OP__call_user_fn: //call user function
                        //use stack
                        // params,ret stuff,local space

                        int fpc = _prog[pc++];
                        nparams = _prog[pc++];
                        int localStackUsage = _prog[pc++];//I removed this at some point but don't know how it was meant to work without it
                        int tsp = _sp;
                        //save current state
                        _stack[_sp++] = pc;
                        _stack[_sp++] = stackBase;
                        _stack[_sp++] = functionBase;
                        _sp += localStackUsage - 3;
                        pc = fpc;
                        stackBase = tsp - nparams;
                        functionBase = stackBase;
                        break;


                    case OP__ret_user_fn:
                        //could put fn preamble on stack before params
                        //means two ops for fn call
                        nparams = _prog[pc++];
                        //leave just ret val on stack
                        //stack should have 1 item on

                        int retval = _stack[_sp - 1];
                        _sp = functionBase;
                        pc = _stack[functionBase + nparams];
                        stackBase = _stack[functionBase + nparams + 1];
                        functionBase = _stack[functionBase + nparams + 2];
                        _stack[_sp++] = retval;
                        break;

                    case OP_assign: assign(); break;
                    case OP_add: fnadd(); break;
                    case OP_sub: fnsub(); break;
                    case OP_multiply: fnmul(); break;
                    case OP_divide: fndivide(); break;
                    case OP_remainder: fnremainder(); break;
                    case OP_gt: fngt(); break;
                    case OP_lt: fnlt(); break;
                    case OP_gte: fngte(); break;
                    case OP_lte: fnlte(); break;
                    case OP_eq: fneq(); break;
                    case OP_ne: fnne(); break;
                    case OP_print: fnprint(); break;
                    case OP_interp:
                        /* call anon function in scope of original function
                         * lambda ptr on stack has pc and a stack frame encoded
                         * 
                         * pass some indicator of function frame
                         * so return can find address
                         * ret could pass offset from _sp
                         */
                        int lptr = _stack[--_sp];
                        //save current state
                        _stack[_sp++] = pc;
                        _stack[_sp++] = stackBase;
                        _stack[_sp++] = functionBase;
                        pc = lptr & 0x0000ffff;
                        stackBase = lptr >> 16;
                        functionBase = _sp - 3;
                        break;
                    case OP_dec:
                        _stack[_stack[_sp - 1]]--;
                        _sp -= 1; //TODO leave on stack and clean at end of expression if no assign

                        break;
                    case OP_neg:
                        _stack[_sp - 1] = -_stack[_sp - 1];
                        break;

                    case OP__jz:
                        if (_stack[--_sp] == 0) pc += _prog[pc];
                        else pc++;
                        break;

                    case OP__jnz:
                        if (_stack[--_sp] != 0) pc += _prog[pc];
                        else pc++;
                        break;

                    case OP__j:
                        pc += _prog[pc];
                        break;
                    case OP__dec_sp: _sp--; break;
                    case OP__inc_sp: _sp++; break;
                    case OP_get_array_item:
                        // TODO allow for diff size items 
                        _stack[_sp - 2] = _stack[_stack[_sp - 2] + _stack[_sp - 1]];
                        _sp--;
                        break;
                    case OP_create_array:
                        //array items on stack

                        int alen = _stack[--_sp];
                        int buffer = malloc(alen + 1);

                        _stack[buffer] = alen;
                        //copy items from stack to array
                        while (alen > 0)
                        {
                            _stack[buffer - alen] = _stack[--_sp];
                            alen--;
                        }
                        _stack[_sp++] = buffer;
                        break;


                    case OP_get_routed_int16:
                        int idx = _stack[_sp - 1];
                        int obj = _stack[_sp - 2];
                        _stack[_sp - 2] = 123;
                        _sp -= 1;
                        break;
                    case OP_set_routed_int16:
                        _sp -= 2;
                        break;
                    case OP_get_routed_array_item:
                        int arridx = _stack[_sp - 1];
                        int idx2 = _stack[_sp - 2];
                        int obj2 = _stack[_sp - 3];
                        _stack[_sp - 3] = ((rc24.routedObject)objects[obj2]).getArrayItemAsInt(idx2, arridx);
                        _sp -= 2;
                        break;
                    case OP_set_routed_array_item:
                        
                        arridx = _stack[_sp - 2];
                        idx2 = _stack[_sp - 3];
                        obj2 = _stack[_sp - 4];
                        ((rc24.routedObject)objects[obj2]).setArrayItemFromInt(idx2, arridx, _stack[_sp - 1]);
                        _sp -= 4;
                        break;
                        
                    default: break;
                }
            }
            if (trace) dumpStack(pc, stackBase, functionBase);
            if (trace) Console.Out.WriteLine("<-interp ");

        }

        private void assign()
        {
            _stack[_stack[_sp - 2]] = _stack[_sp - 1];
            _stack[_sp - 2] = _stack[_sp - 1];
            _sp -= 2;//default to empty stack as most common usage 
        }
        private void fnadd()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 1] + _stack[_sp - 2];

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fnsub()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 2] - _stack[_sp - 1];

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fnmul()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 1] * _stack[_sp - 2];

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fndivide()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 2] / _stack[_sp - 1];

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fnremainder()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 2] % _stack[_sp - 1];

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fngt()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 2] > _stack[_sp - 1] ? 1 : 0;

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fnlt()
        {
            //val,val,val,||,val,ref,val,ref
            int t = _stack[_sp - 2] < _stack[_sp - 1] ? 1 : 0;

            _stack[_sp - 2] = t;//val
            _sp -= 1;
        }
        private void fngte()
        {
            int t = _stack[_sp - 2] >= _stack[_sp - 1] ? 1 : 0;
            _stack[_sp - 2] = t;
            _sp -= 1;
        }
        private void fnlte()
        {
            int t = _stack[_sp - 2] <= _stack[_sp - 1] ? 1 : 0;
            _stack[_sp - 2] = t;
            _sp -= 1;
        }
        private void fneq()
        {
            int t = _stack[_sp - 2] == _stack[_sp - 1] ? 1 : 0;
            _stack[_sp - 2] = t;
            _sp -= 1;
        }
        private void fnne()
        {
            int t = _stack[_sp - 2] != _stack[_sp - 1] ? 1 : 0;
            _stack[_sp - 2] = t;
            _sp -= 1;
        }

        private void fnprint()
        {
            Console.Out.WriteLine(">" + _stack[_sp - 1]);
            _sp -= 1;
        }
        private void fnreturn()
        {
            //	_sp-=1;
        }
        
        private void fndec()
        {
            _stack[_stack[_sp - 1]]--;
            _sp -= 1; //TODO leave on stack and clean at end of expression if no assign
        }
        private void free(int block)
        {

        }

        private int malloc(int size)
        {
            // emulate raw use of memory - not the way to do it in java but closer to
            // c implementation

            int newblock = _freeBlocks;

            while (newblock != 0)
            {
                int avail = _stack[newblock + SIZE];
                if (avail >= size)
                {
                    //put block in usedBlocks list
                    int nextu = _stack[_usedBlocks + NEXT];
                    _stack[_usedBlocks + NEXT] = newblock;//set next on prev
                    _stack[nextu + PREV] = newblock;//set prev on next

                    int nextf = _stack[newblock + NEXT];

                    _stack[newblock + PREV] = _usedBlocks;//set prev
                    _stack[newblock + NEXT] = nextu;//set next

                    //tidy up free list
                    if (avail > size + 4)
                    {
                        //split current free block
                        _stack[newblock + SIZE] = size;
                        //create new free block
                        int newfree = newblock - size - HEADER;
                        _stack[newfree + SIZE] = avail - size - HEADER;
                        _stack[newfree + PREV] = _freeBlocks;
                        _stack[newfree + NEXT] = nextf;

                        _stack[_freeBlocks + NEXT] = newfree;
                        _stack[nextf + PREV] = newfree;
                    }
                    else
                    {
                        //take whole block
                        _stack[_freeBlocks + NEXT] = nextf;
                        _stack[nextf + PREV] = _freeBlocks;
                    }
                    return newblock - 3;
                }
                else newblock = _stack[newblock + NEXT];
            }
            //no memory found
            //ask for bigger heap
            return 0;
        }


        static void test()
        {


        }
    }
}
