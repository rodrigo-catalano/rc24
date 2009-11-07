/*
 Copyright 2008 - 2009 © Alan Hopper

 This file is part of rc24.

 rc24 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 rc24 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with rc24.  If not, see <http://www.gnu.org/licenses/>.


 */

#ifndef _COMMON_COMMANDS_H
#define _COMMON_COMMANDS_H

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>

/*
 * 0x00 protocol enumeration commands
        0x00 get node name
        0x01 node name response
        0x02 get child nodes
        0x03 child node response
        0x04 enumerate parameters
        0x05 parameters response
        0x06 get parameter name, type & guid, readable writeable, expert level.
        0x07 parameter response

0x01 set parameter - could be bit, byte, string,array etc
0x02 set parameter response
0x03 get parameter
0x04 get parameter response

0x05 system commands
    0x00 reset
    0x02 store settings
0x06 code update commands

codes >= 0x80 app
 *
 */
#define CMD_GROUP_IDX 0
#define CMD_FUNCTION_IDX 1


//constants to identify commands and command groups

#define CMD_ENUM_GROUP 10 // 0 - temp value to allow coexistence with current code

#define CMD_ENUM_GET_NODE_NAME 0
#define CMD_ENUM_GET_NODE_NAME_RESP 1
#define CMD_ENUM_GET_CHILD_NODES 2
#define CMD_ENUM_GET_CHILD_NODES_RESP 3
#define CMD_ENUM_GET_PARAMETERS 4
#define CMD_ENUM_GET_PARAMETERS_RESP 5
#define CMD_ENUM_GET_PARAMETER_META 6
#define CMD_ENUM_GET_PARAMETER_META_RESP 7


#define CMD_SET_PARAM 1
#define CMD_SET_PARAM_RESP 2
#define CMD_GET_PARAM 3
#define CMD_GET_PARAM_RESP 4

	// types that can be sent in parameter commands
	typedef enum
	{
		CC_BOOL,
		CC_STRING,
	    CC_UINT8,
	    CC_INT8,
	    CC_UINT16,
	    CC_INT16,
	    CC_UINT32,
	    CC_INT32,
	    CC_UINT64,
	    CC_INT64,
	    CC_BOOL_ARRAY,
		CC_STRING_ARRAY,
		CC_UINT8_ARRAY,
		CC_INT8_ARRAY,
		CC_UINT16_ARRAY,
		CC_INT16_ARRAY,
		CC_UINT32_ARRAY,
		CC_INT32_ARRAY,
		CC_UINT64_ARRAY,
		CC_INT64_ARRAY,
		CC_FLOAT,
		CC_DOUBLE,
		CC_FLOAT_ARRAY,
		CC_DOUBLE_ARRAY,
		CC_ENUMERATION,
		CC_ENUMERATION_VALUES,
		CC_VOID_FUNCTION

	}ccType;

	typedef union
	{
		bool* cpBoolPtr;
		char* cpCharPtr;
		uint8* cpUInt8ptr;
		int8* cpInt8Ptr;
		uint16* cpUInt16Ptr;
		int16* cpInt16Ptr;
		uint32* cpUInt32Ptr;
		int32* cpInt32Ptr;
		uint64* cpUInt64Ptr;
		int64* cpInt64Ptr;

	}ccParamPtr;


	typedef struct
	{
		const char* name;
		ccType type;
		void* paramPtr;  //set to NULL for accessor function access
		int arrayLen; //length of array parameter
	//	setFunction
	//	getFunction

	} ccParameter;

	typedef struct
	{
		ccParameter* parameters;
		uint16 len;
	}ccParameterList;

	uint8 ccWriteString(uint8* out,const char* val);
	uint8 ccEnumGroupCommand(ccParameterList* paramList, uint8* inMsg, uint8* outMsg);
	uint8 ccSetParameter(ccParameterList* paramList, uint8* inMsg, uint8* outMsg);
	uint8 ccGetParameter(ccParameterList* paramList, uint8* inMsg, uint8* outMsg);


#if defined __cplusplus
}
#endif
#endif


