#if 0

#include "OPCUAHelpers.h"

std::shared_ptr<spdlog::logger> OPCUAHelpers::opcuaHelpersLogger_;

bool OPCUAHelpers::OPCUAIsNumber(UA_Variant variant)
{
	return (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT16]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT32]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT64]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT16]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT32]) ||
		UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT64]));
}

int OPCUAHelpers::OPCUAGetNumberValue(UA_Variant variant)
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

bool OPCUAHelpers::OPCUAReadNumber(UA_NodeId nodeId, int* retval)
{
	UA_Variant value;
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status = UA_Client_readValueAttribute(client_, nodeId, &value);
	if ((status == UA_STATUSCODE_GOOD) && OPCUAIsNumber(value))
	{
		*retval = OPCUAGetNumberValue(value);
	}
	else
	{
		launcherLogger_->error("DecakeLauncher::OPCUAReadNumber: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}
	UA_Variant_clear(&value);
	return ret;
}

bool OPCUAHelpers::OPCUAIsBoolean(UA_Variant variant)
{
	return UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

bool OPCUAHelpers::OPCUAGetBooleanValue(UA_Variant variant)
{
	bool b = false;
	UA_Boolean ua_b = *(UA_Boolean*)variant.data;
	b = static_cast<bool>(ua_b);
	return b;
}

bool OPCUAHelpers::OPCUAReadBoolean(UA_NodeId nodeId, bool* retval)
{
	UA_Variant value;
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status = UA_Client_readValueAttribute(client_, nodeId, &value);
	if ((status == UA_STATUSCODE_GOOD) && OPCUAIsNumber(value))
	{
		*retval = OPCUAGetBooleanValue(value);
	}
	else
	{
		launcherLogger_->error("DecakeLauncher::OPCUAReadBoolean: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}
	UA_Variant_clear(&value);
	return ret;
}

bool OPCUAHelpers::OPCUAIsString(UA_Variant variant)
{
	return (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_STRING]));
}

std::string OPCUAHelpers::OPCUAGetStringValue(UA_Variant variant)
{
	UA_String s;
	if (variant.type == &UA_TYPES[UA_TYPES_STRING])
	{
		s = *(UA_String*)variant.data;
		std::string ret((const char*)s.data, s.length);
		return ret;
	}
	return "";
}

bool OPCUAHelpers::OPCUAReadString(UA_NodeId nodeId, std::string& retval)
{
	UA_Variant value;
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status = UA_Client_readValueAttribute(client_, nodeId, &value);
	if ((status == UA_STATUSCODE_GOOD) && OPCUAIsNumber(value))
	{
		retval = OPCUAGetStringValue(value);
	}
	else
	{
		launcherLogger_->error("DecakeLauncher::OPCUAReadString: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}
	UA_Variant_clear(&value);
	return ret;
}

bool OPCUAHelpers::waitBooleanCondition(UA_NodeId nodeId, bool expected, int timeoutSec, int maxRetry)
{
	launcherLogger_->info("DecakeLauncher::waitBooleanCondition");

	bool readValue = false;
	time_t before = time(NULL);
	bool timeout = false;
	int retry = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!OPCUAReadBoolean(nodeId, &readValue))
		{
			launcherLogger_->error("Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			retry++;
		}
		launcherLogger_->info("NID: {} Expected: {}. Read: {}. Elapsed: {} sec.", (char*)(nodeId.identifier.string.data), expected, readValue, (after - before));
		Sleep(1000);
	} while ((timeout == false) && (readValue != expected) && (retry < maxRetry));
	return !timeout;
}

bool OPCUAHelpers::waitIntCondition(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, int maxRetry)
{
	return false;
}

bool OPCUAHelpers::waitIntConditionDescription(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry)
{
	return false;
}

#endif