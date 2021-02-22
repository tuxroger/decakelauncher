#ifndef _OPCUAMETHODS_
#define _OPCUAMETHODS_

#include "open62541.h"

static const std::string LOGGER_NAME	 = "Applus";
static const std::string LOGGER_FILENAME = "Applus.log";

static const int ISA88_PROCESS_STATE_ABORTED			= 311;
static const int ISA88_PROCESS_STATE_COMPLETE			= 301;
static const int ISA88_PROCESS_STATE_HELD				= 317;
static const int ISA88_PROCESS_STATE_HELD_FOR_ERROR		= 326;
static const int ISA88_PROCESS_STATE_PAUSED				= 315;
static const int ISA88_PROCESS_STATE_STOPPED			= 313;

static const std::list<int> Isa88ProcessFinalValues = { ISA88_PROCESS_STATE_ABORTED,
														ISA88_PROCESS_STATE_COMPLETE,
														ISA88_PROCESS_STATE_HELD,
														ISA88_PROCESS_STATE_HELD_FOR_ERROR,
														ISA88_PROCESS_STATE_PAUSED,
														ISA88_PROCESS_STATE_STOPPED };

static const UA_Int16 NAMESPACE_INDEX = 3;

// AUTO CLEAN PROCESS
static const UA_NodeId OID_LAUNCH_AUTOCLEAN = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)("\"OPCUALaunchAutoClean\""));
static const UA_NodeId MID_LAUNCH_AUTOCLEAN = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)("\"OPCUALaunchAutoClean\".Method"));
static const UA_NodeId COND_END_AUTOCLEAN = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)("\"AutoClean\".\"status\".\"sequencer\".\"state\".\"number\""));

// BUILD UNIT EXIT PROCESS
static const UA_NodeId OID_LAUNCH_BUEXIT = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)("\"OPCUALaunchAutoClean\""));
static const UA_NodeId MID_LAUNCH_BUEXIT = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)("\"OPCUALaunchAutoClean\".Method"));
static const UA_NodeId COND_END_BUEXIT = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)("\"AutoClean\".\"status\".\"sequencer\".\"state\".\"number\""));

#endif // !_OPCUAMETHODS_

