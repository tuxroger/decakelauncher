#if 0
#include "open62541.h"
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <list>

#include <windows.h>

#include "getopt.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"


std::shared_ptr<spdlog::logger> mainLogger;
UA_Client* client = nullptr;
std::string ghost;
int gport = -1;
bool connected = false;
std::mutex connected_mutex;

bool connectClient(const std::string& host, int port);

void exitApp(UA_Client* client)
{
	mainLogger->info("exitApp");
	if (client != nullptr)
	{
		UA_Client_disconnect(client);
		UA_Client_delete(client);
	}
	exit(1);
}

// typedef void (*UA_ClientStateCallback)(UA_Client *client, UA_ClientState clientState);
void clientStateCallback(UA_Client* client, UA_ClientState clientState)
{
	if (clientState == UA_CLIENTSTATE_DISCONNECTED)        /* The client is disconnected */
	{
		mainLogger->info("clientStateCallback: UA_CLIENTSTATE_DISCONNECTED");
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			connected = false;
		}
	}
	else if (clientState == UA_CLIENTSTATE_CONNECTED)           /* A TCP connection to the server is open */
	{
		mainLogger->info("clientStateCallback: UA_CLIENTSTATE_CONNECTED");
	}
	else if (clientState == UA_CLIENTSTATE_SECURECHANNEL)       /* A SecureChannel to the server is open */
	{
		mainLogger->info("clientStateCallback: UA_CLIENTSTATE_SECURECHANNEL");
	}
	else if (clientState == UA_CLIENTSTATE_SESSION)             /* A session with the server is open */
	{
		mainLogger->info("clientStateCallback: UA_CLIENTSTATE_SESSION");
	}
	else if (clientState == UA_CLIENTSTATE_SESSION_RENEWED)      /* A session with the server is open (renewed) */
	{
		mainLogger->info("clientStateCallback: UA_CLIENTSTATE_SESSION_RENEWED");
	}
	else
	{
		mainLogger->info("clientStateCallback: UNKNOWN!");
	}
}

bool connectClient(const std::string& host, int port)
{
	mainLogger->info("Connect client..");

	//if (client != nullptr) UA_Client_delete(client);
	mainLogger->info("Connect client: Disconnecting client");
	if (client != nullptr) UA_Client_disconnect(client);
	mainLogger->info("Connect client: Deleting client");
	if (client != nullptr) UA_Client_delete(client);
	mainLogger->info("Connect client: Client = nullptr");
	client = nullptr;

	// CREATE CLIENT
	client = UA_Client_new();
	UA_ClientConfig* config = UA_Client_getConfig(client);
	config->clientDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC((char*)"en-US", (char*)"tuxroger_HAMM_Certification");
	config->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
	//config->requestedSessionTimeout = 60000;
	UA_ClientConfig_setDefault(config);
	config->stateCallback = clientStateCallback;

	std::string connectionString = "opc.tcp://" + host + ":" + std::to_string(port);
	// CONNECT CLIENT
	UA_StatusCode retval = UA_Client_connect(client, connectionString.c_str());
	if (retval != UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Connect client.. NOT CONNECTED!");
		UA_Client_delete(client);
		return false;
	}
	connected = true;
	mainLogger->info("Connect client.. CONNECTED!");
	return true;
}

bool connectRetry(const std::string host, int port, int maxRetry)
{
	mainLogger->info("connectRetry");

	int retry = 0;
	bool connect_success = false;
	do
	{
		connect_success = connectClient(host, port);
		if (!connect_success) retry++;
	} while ((retry < maxRetry) && (!connect_success));
	return connect_success;
}

//handling bools
bool isBoolean(UA_Variant variant)
{
	return UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

bool getBooleanValue(UA_Variant variant)
{
	bool b = false;
	UA_Boolean ua_b = *(UA_Boolean*)variant.data;
	b = static_cast<bool>(ua_b);
	return b;
}

bool UA_ReadBoolean(UA_Client* client, UA_NodeId nodeId, bool* retval)
{
	/* Read the value attribute of the node. UA_Client_readValueAttribute is a
	 * wrapper for the raw read service available as UA_Client_Service_read. */
	UA_Variant value; /* Variants can hold scalar values and arrays of any type */
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status = UA_Client_readValueAttribute(client, nodeId, &value);
	if ((status == UA_STATUSCODE_GOOD) && isBoolean(value))
	{
		*retval = getBooleanValue(value);
	}
	else
	{
		mainLogger->error("UA_ReadBoolean: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}

	/* Clean up */
	UA_Variant_clear(&value);
	return ret;
}

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
		mainLogger->warn("isNumber: UA_Variant is not a number!");
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

//handling strings
bool isString(UA_Variant variant)
{
	return (UA_Variant_hasScalarType(&variant, &UA_TYPES[UA_TYPES_STRING]));
}

std::string getStringValue(UA_Variant variant)
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

bool UA_ReadString(UA_Client* client, UA_NodeId nodeId, std::string& retval)
{
	/* Read the value attribute of the node. UA_Client_readValueAttribute is a
	 * wrapper for the raw read service available as UA_Client_Service_read. */
	UA_Variant value; /* Variants can hold scalar values and arrays of any type */
	UA_Variant_init(&value);
	bool ret = true;
	UA_StatusCode status;
	/* NodeId of the variable holding the current time */
	if ((status = UA_Client_readValueAttribute(client, nodeId, &value) == UA_STATUSCODE_GOOD) && isString(value))
	{
		retval = getStringValue(value);
	}
	else
	{
		mainLogger->error("UA_ReadString: Error while calling UA_Client_readValueAttribute. Node '{}'. Status '{}'. Error '{}'", (char*)nodeId.identifier.string.data, status, UA_StatusCode_name(status));
		ret = false;
	}

	/* Clean up */
	UA_Variant_clear(&value);
	return ret;
}

void printUsage()
{
	std::cout << "Usage: load_launch -h <host> -p <port> -t <platform_height_microns> -f <fake_feeding_duration> -s <sleepTime>" << std::endl;
}

const UA_UInt16 NAMESPACE_INDEX = 3;

const std::string BULOAD_OID = "\"OPCUALaunchProcessLoad\"";
const std::string BULOAD_MID = BULOAD_OID + ".Method";
const UA_NodeId NodeBULoadOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BULOAD_OID.c_str());
const UA_NodeId NodeBULoadMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BULOAD_MID.c_str());

const std::string BUCONNECT_OID = "\"OPCUABuildUnitConnect\"";
const std::string BUCONNECT_MID = BUCONNECT_OID + ".Method";
const UA_NodeId NodeBUConnectOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUCONNECT_OID.c_str());
const UA_NodeId NodeBUConnectMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUCONNECT_MID.c_str());

const std::string BUHOME_OID = "\"OPCUABuildUnitHome\"";
const std::string BUHOME_MID = BUHOME_OID + ".Method";
const UA_NodeId NodeBUHomeOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUHOME_OID.c_str());
const UA_NodeId NodeBUHomeMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUHOME_MID.c_str());

const std::string DOORLOCK_OID = "\"OPCUADoorLockRestart\"";
const std::string DOORLOCK_MID = DOORLOCK_OID + ".Method";
const UA_NodeId NodeDoorLockRestartOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)DOORLOCK_OID.c_str());
const UA_NodeId NodeDoorLockRestartMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)DOORLOCK_MID.c_str());

const std::string OPENREFILLER_OID = "\"OPCUAOpenRefiller\"";
const std::string OPENREFILLER_MID = OPENREFILLER_OID + ".Method";
const UA_NodeId NodeOpenRefillerOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)OPENREFILLER_OID.c_str());
const UA_NodeId NodeOpenRefillerMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char*)OPENREFILLER_MID.c_str());

const std::string PHBULOAD_OID = "\"OPCUAPH_BuildUnitLoad\"";
const std::string PHBULOAD_MID = PHBULOAD_OID + ".Method";
//const UA_NodeId NodeOpenRefillerOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_OID.c_str());
//const UA_NodeId NodeOpenRefillerMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_MID.c_str());

const std::string LAUNCHFAKE_OID = "\"OPCUAStartPowderTransportToInputTank\"";
const std::string LAUNCHFAKE_MID = LAUNCHFAKE_OID + ".Method";
//const UA_NodeId NodeOpenRefillerOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_OID.c_str());
//const UA_NodeId NodeOpenRefillerMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_MID.c_str());

const std::string STOPFAKE_OID = "\"OPCUAStopPowderTransportToInputTank\"";
const std::string STOPFAKE_MID = STOPFAKE_OID + ".Method";
//const UA_NodeId NodeOpenRefillerOID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_OID.c_str());
//const UA_NodeId NodeOpenRefillerMID = UA_NODEID_STRING(NAMESPACE_INDEX, (char *)OPENREFILLER_MID.c_str());

const std::string COND_BU_CONNECTED = "\"buControl\".\"status\".\"trolley\".\"connection\".\"connected\"";
const std::string COND_FRONT_PLATFORM_HOMED = "\"buControl\".\"status\".\"trolley\".\"front\".\"homed\"";
const std::string COND_REAR_PLATFORM_HOMED = "\"buControl\".\"status\".\"trolley\".\"rear\".\"homed\"";
const std::string COND_REFILLER_ISOPEN = "\"refiller\".\"status\".\"isOpen\"";
const std::string COND_LOAD_PROCESS_STATE = "\"processLoad\".\"status\".\"sequencer\".\"state\".\"number\"";
const std::string COND_LOAD_PROCESS_STATE_DESC = "\"processLoad\".\"status\".\"sequencer\".\"description\"";

const std::string COND_BU_FRONT_POSITION = "\"buControl\".\"status\".\"trolley\".\"front\".\"position\"";
const std::string COND_BU_REAR_POSITION = "\"buControl\".\"status\".\"trolley\".\"front\".\"position\"";

const int ISA88_CMD_START = 0;
const int ISA88_CMD_STOP = 1;
const int ISA88_CMD_HOLD = 2;
const int ISA88_CMD_ABORT = 3;
const int ISA88_CMD_RESTART = 4;
const int ISA88_CMD_PAUSE = 5;
const int ISA88_CMD_RESUME = 6;
const int ISA88_CMD_COMPLETE = 7;

const int ISA88_PROCESS_STATE_ABORTED = 311;
const int ISA88_PROCESS_STATE_COMPLETE = 301;
const int ISA88_PROCESS_STATE_HELD = 317;
const int ISA88_PROCESS_STATE_HELD_FOR_ERROR = 326;
const int ISA88_PROCESS_STATE_PAUSED = 315;
const int ISA88_PROCESS_STATE_STOPPED = 313;

bool callBuildUnitLoad(UA_Client* client, UA_Int32 platformHeight)
{
	mainLogger->info("callBuildUnitLoad: PlatformHeight '{}' microns", platformHeight);

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];

	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &platformHeight, &UA_TYPES[UA_TYPES_INT32]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BULOAD_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BULOAD_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", BULOAD_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", BULOAD_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

bool callBuildUnitConnect(UA_Client* client)
{
	mainLogger->info("callBuildUnitConnect");

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];
	bool dummy = false;
	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &dummy, &UA_TYPES[UA_TYPES_BOOLEAN]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUCONNECT_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUCONNECT_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", BUCONNECT_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", BUCONNECT_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

bool callBuildUnitHome(UA_Client* client, UA_Boolean front, UA_Boolean rear, UA_Boolean main)
{
	mainLogger->info("callBuildUnitHome: FRONT '{}' REAR '{}' MAIN '{}'", front, rear, main);

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[3];

	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &front, &UA_TYPES[UA_TYPES_BOOLEAN]);
	UA_Variant_init(&input[1]);
	UA_Variant_setScalarCopy(&input[1], &rear, &UA_TYPES[UA_TYPES_BOOLEAN]);
	UA_Variant_init(&input[2]);
	UA_Variant_setScalarCopy(&input[2], &main, &UA_TYPES[UA_TYPES_BOOLEAN]);

	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUHOME_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)BUHOME_MID.c_str()),
		3, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", BUHOME_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", BUHOME_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	UA_Variant_deleteMembers(&input[1]);
	UA_Variant_deleteMembers(&input[2]);
	return ret;
}

bool callDoorLockRestart(UA_Client* client)
{
	mainLogger->info("callDoorLockRestart");

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];
	bool dummy = false;
	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &dummy, &UA_TYPES[UA_TYPES_BOOLEAN]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)DOORLOCK_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)DOORLOCK_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", DOORLOCK_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", DOORLOCK_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

bool callOpenRefiller(UA_Client* client)
{
	mainLogger->info("callOpenRefiller");

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];
	bool dummy = false;
	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &dummy, &UA_TYPES[UA_TYPES_BOOLEAN]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)OPENREFILLER_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)OPENREFILLER_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", OPENREFILLER_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", OPENREFILLER_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

bool callPHBuildUnitLoad(UA_Client* client, UA_Int16 command)
{
	mainLogger->info("callPHBuildUnitLoad: COMMAND '{}'", command);

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];

	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &command, &UA_TYPES[UA_TYPES_INT16]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)PHBULOAD_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)PHBULOAD_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", PHBULOAD_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", PHBULOAD_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

bool callLaunchFakeMaterialFeeding(UA_Client* client, UA_Float vacuumRpm, UA_Float mixerRpm)
{
	mainLogger->info("callLaunchFakeMaterialFeeding: VACUUM_RPM '{}' MIXER_RPM '{}'", vacuumRpm, mixerRpm);

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[2];

	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &vacuumRpm, &UA_TYPES[UA_TYPES_FLOAT]);
	UA_Variant_init(&input[1]);
	UA_Variant_setScalarCopy(&input[1], &mixerRpm, &UA_TYPES[UA_TYPES_FLOAT]);

	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)LAUNCHFAKE_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)LAUNCHFAKE_MID.c_str()),
		2, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", LAUNCHFAKE_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", LAUNCHFAKE_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	UA_Variant_deleteMembers(&input[1]);
	return ret;
}

bool callStopFakeMaterialFeeding(UA_Client* client)
{
	mainLogger->info("callStopFakeMaterialFeeding");

	bool ret = true;
	/* Call a remote method */
	UA_Variant input[1];
	bool dummy = false;
	UA_Variant_init(&input[0]);
	UA_Variant_setScalarCopy(&input[0], &dummy, &UA_TYPES[UA_TYPES_BOOLEAN]);
	size_t outputSize;
	UA_Variant* output;
	UA_StatusCode retval = UA_Client_call(client,
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)STOPFAKE_OID.c_str()),
		UA_NODEID_STRING(NAMESPACE_INDEX, (char*)STOPFAKE_MID.c_str()),
		1, input,
		&outputSize, &output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		mainLogger->info("Successfully called '{}'", STOPFAKE_MID);
	}
	else
	{
		mainLogger->error("Error while calling method '{}'. Error '{}'", STOPFAKE_MID, UA_StatusCode_name(retval));
		ret = false;
	}
	UA_Variant_deleteMembers(&input[0]);
	return ret;
}

/*
False		ISA88_PROCESS_STATE_ABORTED	Int	311	The process is in the aborted steady state.
False		ISA88_PROCESS_STATE_ABORTING	Int	310	The process is in the aborting transient state.
False		ISA88_PROCESS_STATE_COMPLETE	Int	301	The processes reaches a completed steady state.
False		ISA88_PROCESS_STATE_HELD	Int	317	The process is in the held steady state.
False		ISA88_PROCESS_STATE_HELD_FOR_ERROR	Int	326	The process is in the held-for-error steady state.
False		ISA88_PROCESS_STATE_HOLDING	Int	316	The process is in the holding transient state.
False		ISA88_PROCESS_STATE_HOLDING_FOR_ERROR	Int	327	The process is in the holding-for-error transient state.
False		ISA88_PROCESS_STATE_IDLE	Int	303	The process is in the idle steady state.
False		ISA88_PROCESS_STATE_PAUSED	Int	315	The process is in the paused steady state.
False		ISA88_PROCESS_STATE_PAUSING	Int	314	The process is in the pausing transient state.
False		ISA88_PROCESS_STATE_RESTARTING	Int	318	The process is in the restarting transient state.
False		ISA88_PROCESS_STATE_RUNNING	Int	302	The process is in the running state.
False		ISA88_PROCESS_STATE_STOPPED	Int	313	The process is in the stopped steady state.
False		ISA88_PROCESS_STATE_STOPPING	Int	312	The process is in the stopping transient state.
 */
bool waitIntCondition(UA_Client* client, UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, int maxRetry = 10)
{
	mainLogger->info("waitIntCondition");

	int readValue = -1;
	time_t before = time(NULL);
	bool timeout = false;
	bool condition_met = false;
	int retry = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!UA_ReadNumber(client, nodeId, &readValue))
		{
			mainLogger->error("Error while reading node '{}'. Error '{}'", (char*)nodeId.identifier.string.data);
			retry++;
		}
		mainLogger->info("NID: {}. Read: {}. Elapsed: {} sec.", (char*)(nodeId.identifier.string.data), readValue, (after - before));
		bool int_found = false;
		for (std::list<int>::const_iterator it = finalValues.begin(); it != finalValues.end(); it++)
		{
			if (readValue == *it) { int_found = true; break; }
		}
		condition_met = (int_found);
		Sleep(1000);
	} while ((timeout == false) && (!condition_met) && (retry < maxRetry));
	return !timeout;
}

bool waitIntConditionExt(UA_Client* client, UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry = 10)
{
	mainLogger->info("waitIntConditionExt");

	int readValue = -1;
	std::string readDesc;
	time_t before = time(NULL);
	bool timeout = false;
	bool condition_met = false;
	int retry = 0;
	int retry2 = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!UA_ReadNumber(client, nodeId, &readValue))
		{
			mainLogger->error("Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			readValue = -1;
			retry++;
		}
		if (!UA_ReadString(client, nodeIdDesc, readDesc))
		{
			mainLogger->error("Error while reading node '{}'.", (char*)nodeIdDesc.identifier.string.data);
			readDesc = "Unknown";
			retry2++;
		}
		mainLogger->info("NID: {}. Read: {}. Elapsed: {} sec. Description: {}", (char*)(nodeId.identifier.string.data), readValue, (after - before), readDesc);
		bool int_found = false;
		for (std::list<int>::const_iterator it = finalValues.begin(); it != finalValues.end(); it++)
		{
			if (readValue == *it) { int_found = true; break; }
		}
		condition_met = (int_found);
		Sleep(1000);
	} while ((timeout == false) && (!condition_met) && (retry < maxRetry) && (retry2 < maxRetry));
	return !timeout;
}

bool waitBooleanCondition(UA_Client* client, UA_NodeId nodeId, bool expected, int timeoutSec, int maxRetry = 10)
{
	mainLogger->info("waitBooleanCondition");

	bool readValue = false;
	time_t before = time(NULL);
	bool timeout = false;
	int retry = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!UA_ReadBoolean(client, nodeId, &readValue))
		{
			mainLogger->error("Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			retry++;
		}
		mainLogger->info("NID: {} Expected: {}. Read: {}. Elapsed: {} sec.", (char*)(nodeId.identifier.string.data), expected, readValue, (after - before));
		Sleep(1000);
	} while ((timeout == false) && (readValue != expected) && (retry < maxRetry));
	return !timeout;
}

bool waitBooleanConditionExt(UA_Client* client, UA_NodeId nodeId, bool expected, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry = 10)
{
	mainLogger->info("waitBooleanCondition");

	bool readValue = false;
	int readDesc = -1;
	time_t before = time(NULL);
	bool timeout = false;
	int retry = 0;
	int retry2 = 0;
	do
	{
		time_t after = time(NULL);
		timeout = (after - before) > timeoutSec; // timeout
		if (!UA_ReadBoolean(client, nodeId, &readValue))
		{
			mainLogger->error("Error while reading node '{}'.", (char*)nodeId.identifier.string.data);
			retry++;
		}
		if (!UA_ReadNumber(client, nodeIdDesc, &readDesc))
		{
			mainLogger->error("Error while reading node '{}'.", (char*)nodeIdDesc.identifier.string.data);
			readDesc = -1;
			retry2++;
		}
		mainLogger->info("NID: {} Expected: {}. Read: {}. Elapsed: {} sec. Position: '{}'", (char*)(nodeId.identifier.string.data), expected, readValue, (after - before), readDesc);
		Sleep(1000);
	} while ((timeout == false) && (readValue != expected) && (retry < maxRetry));
	return !timeout;
}

int main(int argc, char* argv[])
{
	int loopCount = 0;
	int option;
	std::string host;
	bool hostSet = false;
	int port = -1;
	bool portSet = false;
	int platformHeight = -1;
	bool platformHeightSet = false;

	int fakeDuration = -1;
	bool fakeDurationSet = false;

	int sleepTime = -1;
	bool sleepTmetSet = false;

	while ((option = getopt(argc, argv, "h:p:t:f:s:")) != -1)
	{
		switch (option)
		{
		case 'h':
			if (optarg != nullptr)
			{
				host = optarg;
				hostSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			if (optarg != nullptr)
			{
				port = std::atoi(optarg);
				portSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			if (optarg != nullptr)
			{
				platformHeight = std::atoi(optarg);
				platformHeightSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 'f':
			if (optarg != nullptr)
			{
				fakeDuration = std::atoi(optarg);
				fakeDurationSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 's':
			if (optarg != nullptr)
			{
				sleepTime = std::atoi(optarg);
				sleepTmetSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printUsage();
			exit(EXIT_FAILURE);
			break;
		}
	}
	if (!hostSet || !portSet || !platformHeightSet || !fakeDurationSet || !sleepTmetSet)
	{
		printUsage();
		exit(EXIT_FAILURE);
	}

	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_st>("applus.log", 1024 * 1024 * 100, 5, false);
	auto console = std::make_shared<spdlog::sinks::stdout_sink_st>();
	mainLogger = std::make_shared<spdlog::logger>("applus");
	mainLogger->sinks().push_back(rotating);
	mainLogger->sinks().push_back(console);
	spdlog::register_logger(mainLogger);
	spdlog::flush_every(std::chrono::seconds(1));

	ghost = host;
	gport = port;

	if (!connectClient(host, port))
	{
		mainLogger->error("Error while connecting.");
		exitApp(client);
	}

	bool check_connected;
	{
		std::lock_guard<std::mutex> guard(connected_mutex);
		check_connected = connected;
	}
	if (!check_connected)
	{
		// try reconnect 5 times
		connectClient(ghost, gport);
	}
	if (!callPHBuildUnitLoad(client, ISA88_CMD_COMPLETE))
	{
		mainLogger->error("Error while Sending FIRST COMPLETE to All processes");
		exitApp(client);
	}

	while (1)
	{
		bool check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callDoorLockRestart(client))
		{
			mainLogger->error("Error while Performing Door Lock Restart");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callOpenRefiller(client))
		{
			mainLogger->error("Error while Performing Open Refiller");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// wait condition
		if (!waitBooleanCondition(client, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_REFILLER_ISOPEN.c_str()), true, 10))
		{
			mainLogger->error("Timed out waiting for Open Refiller");
			exitApp(client);
		}
		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callBuildUnitConnect(client))
		{
			mainLogger->error("Error while Performing Build Unit connect");
			exitApp(client);
		}
		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// wait condition
		if (!waitBooleanCondition(client, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_BU_CONNECTED.c_str()), true, 90))
		{
			mainLogger->error("Timed out waiting for Build Unit Connected");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callBuildUnitHome(client, true, false, false))
		{
			mainLogger->error("Error while Performing HOME in FRONT platform");
			exitApp(client);
		}
		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// wait condition
		if (!waitBooleanCondition(client, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_FRONT_PLATFORM_HOMED.c_str()), true, 300))
		{
			mainLogger->error("Timed out waiting for Front Patform Home");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// home rear
		if (!callBuildUnitHome(client, false, true, false))
		{
			mainLogger->error("Error while Performing HOME in REAR platform");
			exitApp(client);
		}
		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// wait condition
		if (!waitBooleanCondition(client, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_REAR_PLATFORM_HOMED.c_str()), true, 300))
		{
			mainLogger->error("Timed out waiting for Rear Patform Home");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callBuildUnitLoad(client, static_cast<UA_Int32>(platformHeight)))
		{
			mainLogger->error("Error while calling Process Build Unit Load");
			exitApp(client);
		}

		// wait condition
		/*
	False		ISA88_PROCESS_STATE_ABORTED	Int	311	The process is in the aborted steady state.
	False		ISA88_PROCESS_STATE_ABORTING	Int	310	The process is in the aborting transient state.
	False		ISA88_PROCESS_STATE_COMPLETE	Int	301	The processes reaches a completed steady state.
	False		ISA88_PROCESS_STATE_HELD	Int	317	The process is in the held steady state.
	False		ISA88_PROCESS_STATE_HELD_FOR_ERROR	Int	326	The process is in the held-for-error steady state.
	False		ISA88_PROCESS_STATE_HOLDING	Int	316	The process is in the holding transient state.
	False		ISA88_PROCESS_STATE_HOLDING_FOR_ERROR	Int	327	The process is in the holding-for-error transient state.
	False		ISA88_PROCESS_STATE_IDLE	Int	303	The process is in the idle steady state.
	False		ISA88_PROCESS_STATE_PAUSED	Int	315	The process is in the paused steady state.
	False		ISA88_PROCESS_STATE_PAUSING	Int	314	The process is in the pausing transient state.
	False		ISA88_PROCESS_STATE_RESTARTING	Int	318	The process is in the restarting transient state.
	False		ISA88_PROCESS_STATE_RUNNING	Int	302	The process is in the running state.
	False		ISA88_PROCESS_STATE_STOPPED	Int	313	The process is in the stopped steady state.
	False		ISA88_PROCESS_STATE_STOPPING	Int	312	The process is in the stopping transient state.
		 */


		std::list<int> final_values = { ISA88_PROCESS_STATE_ABORTED,
				ISA88_PROCESS_STATE_COMPLETE,
				ISA88_PROCESS_STATE_HELD,
				ISA88_PROCESS_STATE_HELD_FOR_ERROR,
				ISA88_PROCESS_STATE_PAUSED,
				ISA88_PROCESS_STATE_STOPPED };
		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!waitIntConditionExt(client, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_LOAD_PROCESS_STATE.c_str()), final_values, 1800, UA_NODEID_STRING(NAMESPACE_INDEX, (char*)COND_LOAD_PROCESS_STATE_DESC.c_str())))
		{
			mainLogger->error("Timed out / Error while waiting for Build Unit Load process");
			exitApp(client);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// Powder feeding
		if (!callLaunchFakeMaterialFeeding(client, 5000, 5000))
		{
			mainLogger->error("Error while Launching FAKE powder feeding");
			exitApp(client);
		}

		time_t before = time(NULL);
		while (time(NULL) - before < fakeDuration)
		{
			mainLogger->info("Fake Powder feeding in progress ... {} elapsed secs...", time(NULL) - before);
			Sleep(1000);

			// keepAlive
			UA_Variant value;
			UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &value);
		}

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		// Powder feeding
		if (!callStopFakeMaterialFeeding(client))
		{
			mainLogger->error("Error while Launching FAKE powder feeding");
			exitApp(client);
		}

		loopCount++;
		mainLogger->info("LOOP DONE!!!. Current done loops: {}", loopCount);

		check_connected = false;
		{
			std::lock_guard<std::mutex> guard(connected_mutex);
			check_connected = connected;
		}
		if (!check_connected)
		{
			// try reconnect 5 times
			if (!connectRetry(ghost, gport, 10)) exitApp(client);
		}
		if (!callPHBuildUnitLoad(client, ISA88_CMD_COMPLETE))
		{
			mainLogger->error("Error while Sending COMPLETE to All Processes");
			exitApp(client);
		}

		before = time(NULL);
		while (time(NULL) - before < sleepTime)
		{
			mainLogger->info("Sleeping between cycles .... Elapsed secs {}", time(NULL) - before);
			Sleep(1000);

			// keepAlive
			UA_Variant value;
			UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &value);
		}
	}
	exitApp(client);
	return EXIT_SUCCESS;
}
#endif 

#include "DecakeLauncher.h"
#include "getopt.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "OPCUAMethods.h"
#include <windows.h>
#include <iostream>

void printUsage()
{
	std::cout << "Usage: decake_launch -h <host> -p <port> -s <sleep_between_cycles>" << std::endl;
}

int main(int argc, char* argv[])
{
	int option;

	std::string host;
	bool hostSet = false;

	int port = -1;
	bool portSet = false;

	//int platformHeight = -1;
	//bool platformHeightSet = false;

	//int fakeDuration = -1;
	//bool fakeDurationSet = false;

	int sleepTime = -1;
	bool sleepTimetSet = false;

	int iteration = 0;

	while ((option = getopt(argc, argv, "h:p:s:")) != -1)
	{
		switch (option)
		{
		case 'h':
			if (optarg != nullptr)
			{
				host = optarg;
				hostSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			if (optarg != nullptr)
			{
				port = std::atoi(optarg);
				portSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		case 's':
			if (optarg != nullptr)
			{
				sleepTime = std::atoi(optarg);
				sleepTimetSet = true;
			}
			else
			{
				printUsage();
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printUsage();
			exit(EXIT_FAILURE);
			break;
		}
	}
	if (!hostSet || !portSet || !sleepTimetSet)
	{
		printUsage();
		exit(EXIT_FAILURE);
	}

	std::shared_ptr<spdlog::logger> mainLogger = std::make_shared<spdlog::logger>(LOGGER_NAME);
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_st>(LOGGER_FILENAME, 1024 * 1024 * 100, 5, false);
	auto console = std::make_shared<spdlog::sinks::stdout_sink_st>();
	mainLogger->sinks().push_back(rotating);
	mainLogger->sinks().push_back(console);
	spdlog::register_logger(mainLogger);
	spdlog::flush_every(std::chrono::seconds(1));

	DecakeLauncher *dl = new DecakeLauncher(host, port);

	while (1)
	{
		mainLogger->info("[App]: Starting iteration '{}'", iteration++);

		dl->RunIteration();

		mainLogger->info("[App]: Finished iteration '{}'", iteration);

		mainLogger->info("[App]: Sleeping for '{}' seconds", sleepTime);
		
		dl->SleepWithKeepAlive(sleepTime);

		mainLogger->info("[App]: Finished sleeping");
	}

	return EXIT_SUCCESS;
}
