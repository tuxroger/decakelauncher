#include "DecakeLauncher.h"

bool DecakeLauncher::OPCUAIsNumber(UA_Variant variant)
{
	return (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT16]) ||
			UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT32]) ||
			UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_INT64]) ||
			UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT16]) ||
			UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT32]) ||
			UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_UINT64]));
}

int DecakeLauncher::OPCUAGetNumberValue(UA_Variant variant)
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

bool DecakeLauncher::OPCUAReadNumber(UA_NodeId nodeId, int* retval)
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

bool DecakeLauncher::OPCUAIsBoolean(UA_Variant variant)
{
	return UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

bool DecakeLauncher::OPCUAGetBooleanValue(UA_Variant variant)
{
	bool b = false;
	UA_Boolean ua_b = *(UA_Boolean*)variant.data;
	b = static_cast<bool>(ua_b);
	return b;
}

bool DecakeLauncher::OPCUAReadBoolean(UA_NodeId nodeId, bool* retval)
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

bool DecakeLauncher::OPCUAIsString(UA_Variant variant)
{
	return (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_STRING]));
}

std::string DecakeLauncher::OPCUAGetStringValue(UA_Variant variant)
{
	UA_String s;
	if(variant.type == &UA_TYPES[UA_TYPES_STRING])
	{
		s = *(UA_String*)variant.data;
		std::string ret((const char *)s.data, s.length);
		return ret;
	}
	return "";
}

bool DecakeLauncher::OPCUAReadString(UA_NodeId nodeId, std::string& retval)
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

bool DecakeLauncher::waitBooleanCondition(UA_NodeId nodeId, bool expected, int timeoutSec, int maxRetry)
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
			launcherLogger_->error("DecakeLauncher::waitBooleanCondition: Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			retry++;
		}
		launcherLogger_->info("DecakeLauncher::waitBooleanCondition: NID: '{}' Expected: '{}'. Read: '{}'. Elapsed: '{}' sec.", (char*)(nodeId.identifier.string.data), expected, readValue, (after - before));
		Sleep(1000);
	} while ((timeout == false) && (readValue != expected) && (retry < maxRetry));
	return !timeout;
}

bool DecakeLauncher::waitIntCondition(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, int maxRetry)
{
	launcherLogger_->info("DecakeLauncher::waitIntCondition");

	int readValue = -1;
	time_t before = time(NULL);
	bool timeout = false;
	bool conditionMet = false;
	int retry = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!OPCUAReadNumber(nodeId, &readValue))
		{
			launcherLogger_->error("DecakeLauncher::waitIntCondition: Error while reading node '{}'. Error '{}'", (char*)nodeId.identifier.string.data);
			retry++;
		}
		launcherLogger_->info("DecakeLauncher::waitIntCondition: NID: '{}'. Read: '{}'. Elapsed: '{}' sec.", (char*)(nodeId.identifier.string.data), readValue, (after - before));
		for (std::list<int>::const_iterator it = finalValues.begin(); it != finalValues.end(); it++)
		{
			if (readValue == *it) { conditionMet = true; break; }
		}
		Sleep(1000);
	} while ((timeout == false) && (!conditionMet) && (retry < maxRetry));
	bool retryReached = (retry >= maxRetry);

	return (!timeout && !retryReached);
}

bool DecakeLauncher::waitIntConditionDescription(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry)
{
	launcherLogger_->info("DecakeLauncher::waitIntConditionDescription");

	int readValue = -1;
	std::string readDesc;
	time_t before = time(NULL);
	bool timeout = false;
	bool conditionMet = false;
	int retry = 0;
	int retry2 = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!OPCUAReadNumber(nodeId, &readValue))
		{
			launcherLogger_->error("DecakeLauncher::waitIntConditionDescription: Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			readValue = -1;
			retry++;
		}
		if (!OPCUAReadString(nodeIdDesc, readDesc))
		{
			launcherLogger_->error("DecakeLauncher::waitIntConditionDescription: Error while reading node '{}'.", (char*)nodeIdDesc.identifier.string.data);
			readDesc = "Unknown";
			retry2++;
		}
		launcherLogger_->info("DecakeLauncher::waitIntConditionDescription: NID: '{}'. Read: '{}'. Elapsed: '{}' sec. Description: '{}'", (char*)(nodeId.identifier.string.data), readValue, (after - before), readDesc);
		for (std::list<int>::const_iterator it = finalValues.begin(); it != finalValues.end(); it++)
		{
			if (readValue == *it) { conditionMet = true; break; }
		}
		Sleep(1000);
	} while ((timeout == false) && (!conditionMet) && (retry < maxRetry) && (retry2 < maxRetry));

	bool retryReached = (retry >= maxRetry);
	bool retry2Reached = (retry2 >= maxRetry);

	return (!timeout && !retryReached && !retry2Reached);
}
