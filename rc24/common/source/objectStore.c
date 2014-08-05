#include <jendefs.h>
#include <string.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "flashFile.h"
#include "routedObject.h"
#include "objectStore.h"

#define STORE_OBJECT_END 0xff


/*
 * routed object store
 * has the following structure
 * uint32 version/object id to prevent reading old data to different structure
 * list of
 * uint8 parameter idx or STORE_OBJECT_END for end of object
 * uint8 type of parameter
 * value preceded by uint32 length for array types
 *
*/
/*
give each object a version no or enum for all versions
eg
typedef enum
{
	RO_RX_1,
	RO_1,
	RO_RX_2
}objectID

or object name then version

so add new id to list if order changes
*/


int readParamLen(flashFile* s, ccType t)
{
	switch(t)
	{
	case CC_BOOL: return 4;
	case CC_STRING : return flashReadInt32(s);
	case CC_UINT8: return 1;
	case CC_INT8: return 1;
	case CC_UINT16: return 2;
	case CC_INT16: return 2;
	case CC_UINT32: return 4;
	case CC_INT32: return 4;
	case CC_UINT64: return 8;
	case CC_INT64: return 8;
	case CC_BOOL_ARRAY: return flashReadInt32(s);
	case CC_STRING_ARRAY: return flashReadInt32(s);
	case CC_UINT8_ARRAY: return flashReadInt32(s);
	case CC_INT8_ARRAY: return flashReadInt32(s);
	case CC_UINT16_ARRAY: return flashReadInt32(s)*2;
	case CC_INT16_ARRAY: return flashReadInt32(s)*2;
	case CC_UINT32_ARRAY: return flashReadInt32(s)*4;
	case CC_INT32_ARRAY: return flashReadInt32(s)*4;
	case CC_UINT64_ARRAY: return flashReadInt32(s)*8;
	case CC_INT64_ARRAY: return flashReadInt32(s)*8;
	case CC_FLOAT: return 4;
	case CC_DOUBLE: return 8;
	case CC_FLOAT_ARRAY: return flashReadInt32(s)*4;
	case CC_DOUBLE_ARRAY: return flashReadInt32(s)*8;
	case CC_ENUMERATION: return 1;
	case CC_ENUMERATION_VALUES: return 0;
	case CC_VOID_FUNCTION: return 0;
	default: return 0;
	}
}

bool readParameter(routedObject* obj,flashFile* s)
{
	//read index
	uint8 idx=flashReadUint8(s);
	if(idx==STORE_OBJECT_END)return FALSE;

	ccType type=(ccType)flashReadUint8(s);

	//get len
	int len=readParamLen(s,type);
	//see if type matches and should be restored
//	dbgPrintf(obj->parameters.parameters[idx].name);

	if(obj->parameters.parameters[idx].type==type &&  (obj->parameters.parameters[idx].flags & CC_STORE)!=0)
	{
		//read value
		if(obj->parameters.parameters[idx].paramPtr!=NULL)
		{

	//		dbgPrintf("+ %i",len);

			flashRead(s,obj->parameters.parameters[idx].paramPtr,len);
		}
	}
	else //skip
	{
		flashReadSeek(s,len);
	}
	return TRUE;
}
void writeParam(flashFile* s,ccParameter* param)
{
	void* ptr=param->paramPtr;
	if(ptr==NULL)return;
	int len=param->arrayLen;

	switch(param->type)
	{
	case CC_BOOL: flashWrite(s,ptr,4);break;
	case CC_STRING : break;//TODO fix string saving
	case CC_UINT8: flashWrite(s,ptr,1);break;
	case CC_INT8: flashWrite(s,ptr,1);break;
	case CC_UINT16: flashWrite(s,ptr,2);break;
	case CC_INT16: flashWrite(s,ptr,2);break;
	case CC_UINT32: flashWrite(s,ptr,4);break;
	case CC_INT32: flashWrite(s,ptr,4);break;
	case CC_UINT64: flashWrite(s,ptr,8);break;
	case CC_INT64: flashWrite(s,ptr,8);break;
	case CC_BOOL_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len);break;
	case CC_STRING_ARRAY: break;//TODO fix string saving
	case CC_UINT8_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len);break;
	case CC_INT8_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len);break;
	case CC_UINT16_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*2);break;
	case CC_INT16_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*2);break;
	case CC_UINT32_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*4);break;
	case CC_INT32_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*4);break;
	case CC_UINT64_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*8);break;
	case CC_INT64_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*8);break;
	case CC_FLOAT: flashWrite(s,ptr,4);break;
	case CC_DOUBLE: flashWrite(s,ptr,8);break;
	case CC_FLOAT_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*4);break;
	case CC_DOUBLE_ARRAY: flashWriteInt32(s,len);flashWrite(s,ptr,len*4);break;
	case CC_ENUMERATION: flashWrite(s,ptr,1);break;
	case CC_ENUMERATION_VALUES: break;
	case CC_VOID_FUNCTION: break;

	}
}
void storeObject(routedObject* obj,flashFile* s)
{
	//write name and version
	//dbgPrintf(" <%s> ",obj->name);

	flashWriteString(s,obj->name);
	flashWriteUint32(s,obj->version);
	uint8 i;
	for(i=0;i<obj->parameters.len;i++)
	{
		if(obj->parameters.parameters[i].flags & CC_STORE)
		{
		//	dbgPrintf(obj->parameters.parameters[i].name);

			// write parameter
			// store index and type for sanity check
			flashWriteUint8(s,i);
			flashWriteUint8(s,(uint8)obj->parameters.parameters[i].type);

			//store value
			writeParam(s,&obj->parameters.parameters[i]);
		}
	}
	flashWriteUint8(s,STORE_OBJECT_END);
}
void readObject(routedObject* obj,flashFile* s)
{
	char name[32];
	name[31]='\0';
	uint32 len = flashReadString(s,name,32);
	uint32 version=flashReadUint32(s);
//	dbgPrintf(" ro %d %s ",len,name);
//	dbgPrintf(" ro %d",len);

	if(strcmp(obj->name,name)==0 && obj->version==version)
	{
		while(readParameter(obj,s));
	}
	else
	{
		//return to start of record
		flashReadSeek(s,-(len+4));
	}
}
