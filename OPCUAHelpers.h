#if 0

#ifndef  _OPCUAHELPERS_
#define _OPCUAHELPERS_

#include "open62541.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <list>
#include <string>
#include <memory>

class OPCUAHelpers
{
public:
	// Handling Numbers
	static bool OPCUAIsNumber(UA_Variant variant);
	static int OPCUAGetNumberValue(UA_Variant variant);
	static bool OPCUAReadNumber(UA_NodeId nodeId, int* retval);

	// Handling Booleans
	static bool OPCUAIsBoolean(UA_Variant variant);
	static bool OPCUAGetBooleanValue(UA_Variant variant);
	static bool OPCUAReadBoolean(UA_NodeId nodeId, bool* retval);

	// Handling Strings
	static bool OPCUAIsString(UA_Variant variant);
	static std::string OPCUAGetStringValue(UA_Variant variant);
	static bool OPCUAReadString(UA_NodeId nodeId, std::string& retval);

	// wait boolean condition
	static bool waitBooleanCondition(UA_NodeId nodeId, bool expected, int timeoutSec, int maxRetry = 10);

	// wait int condition
	static bool waitIntCondition(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, int maxRetry = 10);
	static bool waitIntConditionDescription(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry = 10);

private:
	static std::shared_ptr<spdlog::logger> opcuaHelpersLogger_;
	static void initLogger()
	{

	}
};

#endif // ! _OPCUAHELPERS_

#endif