#include <jendefs.h>
#include <stdio.h>
#include "blunt.h"
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"

	static int* _prog;
	static int* _stack;
	static int* _sp;

	#define OP__ret 0
	#define OP__push_var_val 1
	#define OP__push_var_ref 2
	#define OP__push_immediate 3
	#define OP__push_lambda_ptr 4
	#define OP__jump 5
	#define OP_return 6
	#define OP__call_user_fn 7
	#define OP__ret_user_fn 8

	#define OP_assign 9
	#define OP_add 10
	#define OP_sub 11
	#define OP_multiply 12
	#define OP_divide 13
	#define OP_remainder 14
	#define OP_gt 15
	#define OP_lt 16
	#define OP_gte 17
	#define OP_lte 18
	#define OP_eq 19
	#define OP_ne 20

	#define OP_print 21
	#define OP_interp 22
	#define OP_dec 23
	#define OP__jz 24
	#define OP__jnz 25
	#define OP__j 26
	#define OP__dec_sp 27
    #define OP__inc_sp 28
    #define OP_get_array_item 29
    #define OP_create_array 30
    #define OP_neg 31



	//opcodes to access routed object fields and convert to num type
	#define OP_ro_getint16 32
	#define OP_ro_setint16 33
	#define OP_ro_get_array_item 34
	#define OP_ro_set_array_item 35




	inline void assign()
	{
		*((int*)*(_sp-2))=*(_sp-1);
		*(_sp-2)=*(_sp-1);
		_sp-=2;
	}
	inline void fnadd()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-1) + *(_sp-2);

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fnsub()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-2) - *(_sp-1);

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fnmul()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-1) * *(_sp-2);

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fndivide()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-2) / *(_sp-1);

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fnremainder()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-2) % *(_sp-1);

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fngt()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-2) > *(_sp-1)?1:0;

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fnlt()
	{
		//val,val,val,||,val,ref,val,ref
		int t=*(_sp-2) < *(_sp-1)?1:0;

		*(_sp-2)=t;//val
		_sp-=1;
	}
	inline void fngte()
		{
			//val,val,val,||,val,ref,val,ref
			int t=*(_sp-2) >= *(_sp-1)?1:0;

			*(_sp-2)=t;//val
			_sp-=1;
		}
	inline void fnlte()
		{
			//val,val,val,||,val,ref,val,ref
			int t=*(_sp-2) <= *(_sp-1)?1:0;

			*(_sp-2)=t;//val
			_sp-=1;
		}
	inline void fneq()
		{
			//val,val,val,||,val,ref,val,ref
			int t=*(_sp-2) == *(_sp-1)?1:0;

			*(_sp-2)=t;//val
			_sp-=1;
		}
	inline void fnne()
		{
			//val,val,val,||,val,ref,val,ref
			int t=*(_sp-2) != *(_sp-1)?1:0;

			*(_sp-2)=t;//val
			_sp-=1;
		}
	inline void fnif(int* stackBase)
	{
		//stack is test,lt,lf
		//if val at top of stack
		if(*(_sp-3)!=0)
		{
			interp((int*)(*(_sp-2) & 0x0000ffff),(int*)(*(_sp-2)>>16));
		}
		else
		{
			interp((int*)(*(_sp-1) & 0x0000ffff),(int*)(*(_sp-1)>>16));
		}
		_sp-=3;
	}
	inline	void fnwhile(int* stackBase)
	{
		// test lamda,loop lamda
		int* test= _prog+*(_sp-2);
		int* loop= _prog+*(_sp-1);
		while(1)
		{
			interp(test,stackBase);//do test
			if(*(_sp-1)==0)
			{
				break;
			}
			else
			{
				_sp-=1;
				interp(loop,stackBase);
			}
		}
		_sp-=3;
	}
	inline void fnprint()
	{
		//printf("> %d\n", *(_sp-1));
		dbgPrintf(">%i", *(_sp-1));
		_sp-=1;
	}
	inline void fnreturn()
	{
	//	_sp-=1;
	}
	inline void fninterp(int* stackBase)
	{
		//could pack sb into 32 bit ptr - sb,pc
		int ptr = *(_sp-1);
		_sp-=1;
		interp((int*)(ptr & 0x0000ffff),(int*)(ptr>>16));
	}
	inline void fndec()
	{
		*((int*)*(_sp-1))=*((int*)*(_sp-1))-1;
		_sp-=1; //TODO leave on stack and clean at end of expression if no assign
	}


	void run(int prog[],int stack[],int sp)
	{
		_prog=prog;
		_stack=stack;
		_sp=stack+sp;

		interp(_prog,_sp);
	}

	void interp(int* pc,int* stackBase)
	{
		//code block stackres,opcodes...,0

		int fpc;
		int params;
		int* tsp;
		int* functionBase=stackBase;
		int retval;
		int lptr;

		_sp+=*(pc++); //make room for locals
		while(1)
		{
		//	dbgPrintf("%i : %i %i %i\n",pc-_prog,*pc,*(pc+1),*_stack);

			//getc(stdin);
			switch(*(pc++))
			{
			case OP__ret:return;
			case OP__push_var_val:
				*(_sp++)=*(stackBase+ *(pc++));//put var val  on stack
				break;
			case OP__push_var_ref:
				*(_sp++)=(int)(stackBase+ *(pc++));//put var ref on stack  - abs ptr
				break;
			case OP__push_immediate:
				*(_sp++)=*(pc++);//put immediate on stack
				break;
			case OP__push_lambda_ptr:
				*(_sp++)=*(pc++)+((stackBase-_stack)<<16);//put lambda on stack with stackbase
				break;
			case OP__jump: pc+=*(pc);//relative jump
				break;
			case OP_return: fnreturn();break;

			case OP__call_user_fn: //call user function
				//use stack
				// params,ret stuff,local space

				fpc=*(pc++);
				params=*(pc++);
				int localStackUsage = *(pc++);//I removed this at some point but don't know how it was meant to work

				tsp=_sp;
				//save current state
				*(_sp++)=(int)pc;
				*(_sp++)=(int)stackBase;
				*(_sp++)=(int)functionBase;
				_sp += localStackUsage-3;

				pc=_prog+fpc;
				stackBase=tsp-params;
				functionBase=stackBase;
				break;


			case OP__ret_user_fn:
				//could put fn preamble on stack before params
				//means two ops for fn call
				params=*(pc++);
								//leave just ret val on stack
				//stack should have 1 item on

				retval=*(_sp-1);
				_sp=functionBase;
				pc=(int*)*(functionBase+params);
				stackBase=(int*)*(functionBase+params+1);
				functionBase=(int*)*(functionBase+params+2);
				*(_sp++)=retval;
				break;

			case OP_assign: assign();break;
			case OP_add: fnadd();break;
			case OP_sub: fnsub();break;
			case OP_multiply: fnmul();break;
			case OP_divide: fndivide();break;
			case OP_remainder: fnremainder();break;
			case OP_gt: fngt();break;
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
				lptr = *(--_sp);
				//save current state
				*(_sp++) = (int)pc;
				*(_sp++) = (int)stackBase;
				*(_sp++) = (int)functionBase;
				pc = _prog+(lptr & 0x0000ffff);
				stackBase = _stack+(lptr >> 16);
				functionBase = _sp - 3;
				break;

			case OP_dec: fndec();break;
			case OP__jz:
				if(*(--_sp)==0)pc+=*(pc);
				else pc++;
				break;

			case OP__jnz:
				if(*(--_sp)!=0)pc+=*(pc);
				else pc++;
				break;

			case OP__j:
			//	pc=_prog+*(pc);
				pc+=*(pc);
				break;
			case OP__dec_sp:_sp--;break;
			case OP__inc_sp:_sp++;break;
			case OP_get_array_item:
				// TODO allow for diff size items
				*(_sp-2)=*(int*)(*(_sp-2)+ *(_sp-1));
				_sp--;
				break;
			case OP_create_array:
					//array items on stack
			/*
				int alen=*(--_sp);
				int buffer=malloc(alen+1);

				_stack[buffer]=alen;
				//copy items from stack to array
				while(alen > 0)
				{
					_stack[buffer-alen]=*(--_sp);
					alen--;
				}
				*(_sp++)=buffer;
				*/
				break;
			case OP_ro_getint16:
				//stack obj*,index
				_sp--;
				if(*(_sp-1)!=0)
					*(_sp-1)=ccGetObjectParameterAsNum((routedObject*) *(_sp-1), *(_sp),0 );

				break;
			case OP_ro_setint16:
				//stack obj*,index,value
				if(*(_sp-3)!=0)
					ccSetObjectParameterAsNum((routedObject*) *(_sp-3), *(_sp-2),*(_sp-1),0 );
				_sp-=3;
				break;
			case OP_ro_get_array_item:
				//stack obj*,index,array index
				if(*(_sp-3)!=0)
					*(_sp-3)=ccGetObjectParameterAsNum((routedObject*) *(_sp-3), *(_sp-2), *(_sp-1) );
				_sp-=2;
				break;
			case OP_ro_set_array_item:
				//stack obj*,index,array index,value
				if(*(_sp-4)!=0)
					ccSetObjectParameterAsNum((routedObject*) *(_sp-4), *(_sp-3),*(_sp-2),*(_sp-1) );
				_sp-=4;
				break;
			default:
				//printf("x");
			break;
			}
		}

	}

