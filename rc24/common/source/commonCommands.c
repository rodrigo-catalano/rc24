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

// generic coms protocol commands that all devices should implement (or as much as is applicable?)
// these allow an external device to enumerate and access all
// public interfaces, parameters, commands etc

//at some point these commands should be frozen

// TODO make sure there are no problems implementing on very simple devices

// TODO come to a decision on the max length of these messages and check for it
// AIH - my preference is to set a limit of something like 192 bytes to allow
// encapsulating protocols (routing, serial packing etc) to keep total packet
// size below 256 bytes

// TODO go to 16 bits or 8 bits with escape to 16 for parameter indexs?

#include <jendefs.h>
#include <string.h>
#include "commonCommands.h"

//helper functions to parse or build command buffers

/****************************************************************************
 *
 * NAME: ccWriteString
 *
 * DESCRIPTION: write a string to buffer prefixed with its length in a byte
 * 				the terminating null is not written
 *
 * PARAMETERS:      Name            RW  Usage
 *  				out				W	buffer to write string to
 *  				val				R   null terminated string to write
 *
 * RETURNS: uint8 number of bytes written
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccWriteString(uint8* out,const char* val)
{
	int len = strlen(val);
	*out= (uint8) len;
	memcpy(out+1,val,len);
	return (uint8)(len + 1);
}

/****************************************************************************
 *
 * NAME: ccWriteInt32
 *
 * DESCRIPTION: write int32 value to buffer in native byte order
 *
 * PARAMETERS:      Name            RW  Usage
 *  				out				W	buffer to write to
 *  				val				R 	value
 *
 * RETURNS: uint8 number of bytes written
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccWriteInt32(uint8* out,int32 val)
{
	memcpy(out,&val,4);
	return 4;
}

/****************************************************************************
 *
 * NAME: ccWriteInt16
 *
 * DESCRIPTION: write int16 value to buffer in native byte order
 *
 * PARAMETERS:      Name            RW  Usage
 *  				out				W	buffer to write to
 *  				val				R 	value
 *
 * RETURNS: uint8 number of bytes written
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccWriteInt16(uint8* out,int16 val)
{
	memcpy(out,&val,2);
	return 2;
}

/****************************************************************************
 *
 * NAME: ccEnumGroupCommand
 *
 * DESCRIPTION: Handles commands from the basic property/feature discovery
 * 				interface
 *
 * PARAMETERS:      Name            RW  Usage
 *  				paramList		RW	list of public properties
 *  				inMsg			R	incoming message
 *  				outMsg			w	return message
 *
 * RETURNS: uint8 length of return message 0 for no response
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccEnumGroupCommand(ccParameterList* paramList, uint8* inMsg, uint8* outMsg)
{
	uint8 retlen=0;
	uint8 paramIdx;
	switch (inMsg[CMD_FUNCTION_IDX])
	{
	case CMD_ENUM_GET_NODE_NAME:
		break;
	case CMD_ENUM_GET_NODE_NAME_RESP:
		break;
	case CMD_ENUM_GET_CHILD_NODES:
		break;
	case CMD_ENUM_GET_CHILD_NODES_RESP:
		break;
	case CMD_ENUM_GET_PARAMETERS:
		// Return the number of parameters - assume there are no gaps and they start from 0
		outMsg[retlen++] = CMD_ENUM_GROUP;
		outMsg[retlen++] = CMD_ENUM_GET_PARAMETERS_RESP;
		outMsg[retlen++] = paramList->len;
		break;
	case CMD_ENUM_GET_PARAMETERS_RESP:
		break;
	case CMD_ENUM_GET_PARAMETER_META:
		// Send metadata for property
		paramIdx=inMsg[2];
		outMsg[retlen++] = CMD_ENUM_GROUP;
		outMsg[retlen++] = CMD_ENUM_GET_PARAMETER_META_RESP;
		outMsg[retlen++] =  paramIdx;
		//index,name,len, type & guid, readable writeable, expert level,reset required
		ccParameter* param = &paramList->parameters[paramIdx];
		retlen+=ccWriteString(&outMsg[retlen],param->name);
		outMsg[retlen++] = (uint8)param->type;
		outMsg[retlen++] = (uint8)param->arrayLen;
		// TODO include rest of metadata once fully defined
		break;
	case CMD_ENUM_GET_PARAMETER_META_RESP:
		break;
	}

	return retlen;
}

/****************************************************************************
 *
 * NAME: ccSetParameter
 *
 * DESCRIPTION: Handles commands to set a public property
 *
 * PARAMETERS:      Name            RW  Usage
 *  				paramList		RW	list of public properties
 *  				inMsg			R	incoming message
 *  				outMsg			w	return message
 *
 * RETURNS: uint8 length of return message 0 for no response
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccSetParameter(ccParameterList* paramList, uint8* inMsg, uint8* outMsg)
{
	// Send an ack/nack response
	uint8 retlen=0;
	outMsg[retlen++] = CMD_SET_PARAM_RESP;

	bool responseAck = FALSE;	// Assume a nack response

	uint8 start=0;
	uint8 len=0;
	uint8 itemSize;	// Size of item to transfer

	ccParameter* param = &paramList->parameters[inMsg[1]];
	//if direct access to variable
	void* paramPtr = param->paramPtr;
	if (paramPtr != NULL)
	{
		switch (param->type)
		{
		case CC_BOOL:
			// Value inversion?  0 in => TRUE??
			if (inMsg[2] == 0)
				*((bool*) paramPtr) = TRUE;
			else
				*((bool*) paramPtr) = FALSE;
			responseAck = TRUE;
			break;
		case CC_STRING:
			break;
		case CC_UINT8:
			*((uint8*) paramPtr) = inMsg[2];
			responseAck = TRUE;
			break;
		case CC_INT8:
			*((int8*) paramPtr) = inMsg[2];
			responseAck = TRUE;
			break;
		case CC_UINT16:
			break;
		case CC_INT16:
			break;
		case CC_UINT32:
			break;
		case CC_INT32:
			*((int32*) paramPtr) = (inMsg[2] << 24) + (inMsg[3] << 16) + (inMsg[4] << 8) + inMsg[5];
			responseAck = TRUE;
			break;
		case CC_UINT64:
			break;
		case CC_INT64:
			break;
		case CC_BOOL_ARRAY:
			break;
		case CC_STRING_ARRAY:
			break;
		case CC_UINT8_ARRAY:
			break;
		case CC_INT8_ARRAY:
			break;
		case CC_UINT16_ARRAY:
			// Limit start and length to ensure no writes off the end
			start = MIN(inMsg[2], param->arrayLen - 1);
			len = MIN(inMsg[3], param->arrayLen - start);
			itemSize = sizeof(uint16);
			memcpy((uint16*)(paramPtr + start * itemSize), &inMsg[4], len * itemSize);
			responseAck = TRUE;
			break;
		case CC_INT16_ARRAY:
			break;
		case CC_UINT32_ARRAY:
			break;
		case CC_INT32_ARRAY:
			// Limit start and length to ensure no writes off the end
			start = MIN(inMsg[2], param->arrayLen - 1);
			len = MIN(inMsg[3], param->arrayLen - start);
			itemSize = sizeof(int32);
			memcpy((int32*)(paramPtr + start * itemSize), &inMsg[4], len * itemSize);
			responseAck = TRUE;
			break;
		case CC_UINT64_ARRAY:
			break;
		case CC_INT64_ARRAY:
			break;
		case CC_FLOAT:
			break;
		case CC_DOUBLE:
			break;
		case CC_FLOAT_ARRAY:
			break;
		case CC_DOUBLE_ARRAY:
			break;
		case CC_ENUMERATION:
			*((uint8*) paramPtr) = inMsg[2];
			responseAck = TRUE;
			break;
		case CC_ENUMERATION_VALUES:
			break;
		case CC_VOID_FUNCTION:
			break;
		}
	}
	else if(param->setFunction!=NULL)
	{
		//TODO implement get/set function interface
		//use set function ptr
		switch (param->type)
		{
			case CC_VOID_FUNCTION:
				(*param->setFunction)();
				responseAck = TRUE;
			break;
			default:
				break;
		}
	}

	// Set the response value
	outMsg[retlen++] = responseAck;
	return retlen;
}
/****************************************************************************
 *
 * NAME: ccGetParameter
 *
 * DESCRIPTION: Handles commands to get a public property
 *
 * PARAMETERS:      Name            RW  Usage
 *  				paramList		RW	list of public properties
 *  				inMsg			R	incoming message
 *  				outMsg			w	return message
 *
 * RETURNS: uint8 length of return message 0 for no response
 *
 *
 * NOTES:
 *  None.
 ****************************************************************************/

uint8 ccGetParameter(ccParameterList* paramList, uint8* inMsg, uint8* outMsg)
{
	uint8 retlen = 0;
	outMsg[retlen++] = CMD_GET_PARAM_RESP;
	outMsg[retlen++] = inMsg[1];

	uint8 start=0;
	uint8 len=0;
	uint8 itemSize;	// Size of item to transfer
	uint8 idx;

	ccParameter* param = &paramList->parameters[inMsg[1]];
	//if direct access to variable
	void* paramPtr = param->paramPtr;
	if (paramPtr != NULL)
	{
		switch (param->type)
		{
		case CC_BOOL:
			if(*((bool*) paramPtr)==TRUE)outMsg[retlen++] =0x00;
			else outMsg[retlen++] =0x01;
			break;
		case CC_STRING:
			retlen = 0;
			break;
		case CC_UINT8:
			outMsg[retlen++] = *((uint8*) paramPtr);
			break;
		case CC_INT8:
			outMsg[retlen++] = *((uint8*) paramPtr);
			break;
		case CC_UINT16:
			retlen+=ccWriteInt16(&outMsg[retlen],*((int16*) paramPtr));
			break;
		case CC_INT16:
			retlen+=ccWriteInt16(&outMsg[retlen],*((int16*) paramPtr));
			break;
		case CC_UINT32:
			retlen+=ccWriteInt32(&outMsg[retlen],*((int32*) paramPtr));
			break;
		case CC_INT32:
			retlen+=ccWriteInt32(&outMsg[retlen],*((int32*) paramPtr));
			break;
		case CC_UINT64:
			retlen = 0;
			break;
		case CC_INT64:
			retlen = 0;
			break;
		case CC_BOOL_ARRAY:
			retlen = 0;
			break;
		case CC_STRING_ARRAY:
			retlen = 0;
			break;
		case CC_UINT8_ARRAY:
			retlen = 0;
			break;
		case CC_INT8_ARRAY:
			retlen = 0;
			break;
		case CC_UINT16_ARRAY:
			outMsg[retlen++] = start = inMsg[2];
			outMsg[retlen++] = len = inMsg[3];
			// Write data in native order
			itemSize = sizeof(uint16);
			memcpy(&outMsg[retlen],(uint16*)(paramPtr + start * itemSize), len * itemSize);
			retlen += len * itemSize;
			break;
		case CC_INT16_ARRAY:
			retlen = 0;
			break;
		case CC_UINT32_ARRAY:
			retlen = 0;
			break;
		case CC_INT32_ARRAY:
			outMsg[retlen++] = start = inMsg[2];
			outMsg[retlen++] = len = inMsg[3];
			// Write data in native order
			itemSize = sizeof(int32);
			memcpy(&outMsg[retlen],(uint32*)(paramPtr + start * itemSize), len * itemSize);
			retlen += len * itemSize;
			break;
		case CC_UINT64_ARRAY:
			retlen = 0;
			break;
		case CC_INT64_ARRAY:
			retlen = 0;
			break;
		case CC_FLOAT:
			retlen = 0;
			break;
		case CC_DOUBLE:
			retlen = 0;
			break;
		case CC_FLOAT_ARRAY:
			retlen = 0;
			break;
		case CC_DOUBLE_ARRAY:
			retlen = 0;
			break;
		case CC_ENUMERATION:
			outMsg[retlen++] = *((uint8*) paramPtr);
			break;
		case CC_ENUMERATION_VALUES:
			start=inMsg[2];
			len=inMsg[3];
			outMsg[retlen++] =start;
			outMsg[retlen++] =len;
			idx=start;
			while(idx<len+start)
			{
				retlen+=ccWriteString(&outMsg[retlen],((char**)paramPtr)[idx++]);
			}
			break;
		case CC_VOID_FUNCTION:
			retlen = 0;
			break;
		}
	}
	else
	{
		//TODO implement get/set function interface
		//use get function ptr
	}
	return retlen;
}
