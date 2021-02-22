#ifndef _DECAKELAUNCHER_
#define _DECAKELAUNCHER_

#include "open62541.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <string>
#include <mutex>

class DecakeLauncher
{
public:
	DecakeLauncher(const std::string& host, int port);

	// RUN SEQUENCE
	void RunIteration();

	// STOP SEQUENCE
	void Stop(bool success = true);

	// SLEEP WITH KEEPALIVE
	void SleepWithKeepAlive(int seconds);
	
	// WORKFLOWS
	bool LaunchAutoDecake(bool autoFine, bool autoCoarse, int timeoutSec = 1800);
	bool LaunchBuildUnitExit(int timeoutSec = 1800);

private:

	bool connectClient();
	bool connectRetry(int maxRetry);

	// thread safe isConnected
	bool isConnected();

	// stop Decake Launcher
	void stopLauncher(bool success);

	// internal methods
	bool launchAutoDecake(bool autoFine, bool autoCoarse);
	bool launchBuildUnitExit();

	static void clientStateCallback(UA_Client* client, UA_ClientState clientState);

	UA_Client* client_;
	std::string host_;
	int port_;

	static bool connected_;
	static std::shared_ptr<spdlog::logger> launcherLogger_;
	static std::mutex connectedMutex_;

	// Handling Numbers
	bool OPCUAIsNumber(UA_Variant variant);
	int OPCUAGetNumberValue(UA_Variant variant);
	bool OPCUAReadNumber(UA_NodeId nodeId, int* retval);

	// Handling Booleans
	bool OPCUAIsBoolean(UA_Variant variant);
	bool OPCUAGetBooleanValue(UA_Variant variant);
	bool OPCUAReadBoolean(UA_NodeId nodeId, bool* retval);

	// Handling Strings
	bool OPCUAIsString(UA_Variant variant);
	std::string OPCUAGetStringValue(UA_Variant variant);
	bool OPCUAReadString(UA_NodeId nodeId, std::string &retval);

	// wait boolean condition
	bool waitBooleanCondition(UA_NodeId nodeId, bool expected, int timeoutSec, int maxRetry = 10);
	
	// wait int condition
	bool waitIntCondition(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, int maxRetry = 10);
	bool waitIntConditionDescription(UA_NodeId nodeId, const std::list<int>& finalValues, int timeoutSec, UA_NodeId nodeIdDesc, int maxRetry = 10);

	//UA_ReadRequest request;
	//UA_ReadRequest_init(&request);
	//UA_ReadValueId ids[2];
	//UA_ReadValueId_init(&ids[0]);
	//ids[0].attributeId = UA_ATTRIBUTEID_VALUE;
	//ids[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY);

	//UA_ReadValueId_init(&ids[1]);
	//ids[1].attributeId = UA_ATTRIBUTEID_VALUE;
	//ids[1].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_STATUS);

	//// set here the nodes you want to read
	//request.nodesToRead = ids;
	//request.nodesToReadSize = 2;

	//UA_ReadResponse response = UA_Client_Service_read(client, request);

	//// do something with the response
};

#endif