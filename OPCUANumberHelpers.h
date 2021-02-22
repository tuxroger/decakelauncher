#ifndef _OPCUANUMBERHELPERS_
#define _OPCUANUMBERHELPERS_

#include "open62541.h"

//handling numbers
bool isNumber(UA_Variant variant)
{
	if (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT16]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT32]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT64]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT16]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT32]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT64]))
	{

		return true;
	}
	else
	{
		return false;
	}
}

int getNumberValue(UA_Variant variant)
{
	int number = 0;
	if (variant.type == &UA_TYPES[UA_TYPES_INT16]) number = static_cast<int>(*(UA_Int16*)variant.data);
	if (variant.type == &UA_TYPES[UA_TYPES_INT32]) number = static_cast<int>(*(UA_Int32*)variant.data);
	if (variant.type == &UA_TYPES[UA_TYPES_INT64]) number = static_cast<int>(*(UA_Int64*)variant.data);
	if (variant.type == &UA_TYPES[UA_TYPES_UINT16]) number = static_cast<int>(*(UA_UInt16*)variant.data);
	if (variant.type == &UA_TYPES[UA_TYPES_UINT32]) number = static_cast<int>(*(UA_UInt32*)variant.data);
	if (variant.type == &UA_TYPES[UA_TYPES_UINT64]) number = static_cast<int>(*(UA_UInt64*)variant.data);
	return number;
}

bool UA_ReadNumber(UA_Client* client, UA_NodeId nodeId, int* retval)
{
	/* Read the value attribute of the node. UA_Client_readValueAttribute is a
	 * wrapper for the raw read service available as UA_Client_Service_read. */
	UA_Variant value; /* Variants can hold scalar values and arrays of any type */
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status = UA_Client_readValueAttribute(client, nodeId, &value);
	/* NodeId of the variable holding the current time */
	if ((status == UA_STATUSCODE_GOOD) && isNumber(value))
	{
		*retval = getNumberValue(value);
	}
	else
	{
		mainLogger->error("UA_ReadNumber: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}

	/* Clean up */
	UA_Variant_clear(&value);
	return ret;
}

#endif