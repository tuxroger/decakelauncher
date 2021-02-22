#include "DecakeLauncher.h"
#include "OPCUAMethods.h"
#include <stdlib.h>

// static definitions
bool DecakeLauncher::connected_ = false;
std::shared_ptr<spdlog::logger> DecakeLauncher::launcherLogger_ = nullptr;
std::mutex DecakeLauncher::connectedMutex_;

DecakeLauncher::DecakeLauncher(const std::string& host, int port) : client_(nullptr), host_(host), port_(port) 
{ 
	launcherLogger_ = spdlog::get(LOGGER_NAME);
	if (launcherLogger_ == nullptr)
	{
		throw std::runtime_error(std::string("DecakeLauncher::DecakeLauncher: Cannot get logger"));
	}
}

void DecakeLauncher::RunIteration()
{
	if (!connectClient())
	{
		launcherLogger_->error("DecakeLauncher::Run: Error while calling connectClient.");
		Stop(false);
	}

	if (!LaunchAutoDecake(true, true, 1800))
	{
		launcherLogger_->error("DecakeLauncher::Run: Error while calling LaunchWFA.");
		Stop(false);
	}
	if (!LaunchBuildUnitExit(1800))
	{
		launcherLogger_->error("DecakeLauncher::Run: Error while calling LaunchWFA.");
		Stop(false);
	}

	Stop(true);
}

void DecakeLauncher::Stop(bool success)
{
	stopLauncher(success);
}

void DecakeLauncher::SleepWithKeepAlive(int seconds)
{
	time_t before = time(NULL);
	time_t elapsed = (time(NULL) - before);
	do 
	{
		launcherLogger_->info("DecakeLauncher::SleepWithKeepAlive: Elapsed '{}' seconds", elapsed);
		Sleep(1000);

		// keepAlive
		UA_Variant dummy;
		UA_Client_readValueAttribute(client_, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &dummy);
		
		elapsed = (time(NULL) - before);

	} while (elapsed < seconds);
}

bool DecakeLauncher::connectClient()
{
	launcherLogger_->info("DecakeLauncher::ConnectClient");

	launcherLogger_->info("DecakeLauncher::ConnectClient: Disconnecting client");
	if (client_ != nullptr) UA_Client_disconnect(client_);

	launcherLogger_->info("DecakeLauncher::ConnectClient: Deleting client");
	if (client_ != nullptr) UA_Client_delete(client_);

	launcherLogger_->info("DecakeLauncher::ConnectClient: Client = nullptr");
	client_ = nullptr;

	// CREATE CLIENT
	client_ = UA_Client_new();
	UA_ClientConfig* config = UA_Client_getConfig(client_);
	config->clientDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC((char*)"en-US", (char*)"tuxroger_HAMM_Certification");
	config->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
	UA_ClientConfig_setDefault(config);
	config->stateCallback = clientStateCallback;

	std::string connectionString = "opc.tcp://" + host_ + ":" + std::to_string(port_);
	// CONNECT CLIENT
	UA_StatusCode retval = UA_Client_connect(client_, connectionString.c_str());
	if (retval != UA_STATUSCODE_GOOD)
	{
		launcherLogger_->info("DecakeLauncher::ConnectClient: NOT CONNECTED!");
		UA_Client_delete(client_);
		return false;
	}
	connected_ = true;
	launcherLogger_->info("DecakeLauncher::ConnectClient: CONNECTED!");
	return true;
}

bool DecakeLauncher::LaunchAutoDecake(bool autoFine, bool autoCoarse, int timeoutSec)
{
	if (!isConnected())
	{
		// try reconnect 5 times
		if (!connectRetry(10)) 
			//stopLauncher(false);
			return false;
	}
	// Powder feeding
	if (!launchAutoDecake(autoFine, autoCoarse))
	{
		launcherLogger_->error("DecakeLauncher::LaunchAutoDecake: Error while Launching AutoDecake");
		//stopLauncher(false);
		return false;
	}
	if (!isConnected())
	{
		if (!connectRetry(10))
			//stopLauncher(false);
			return false;
	}
	if (!waitIntCondition(COND_END_AUTOCLEAN, Isa88ProcessFinalValues, timeoutSec, 10))
	{
		launcherLogger_->error("DecakeLauncher::LaunchAutoDecake: Error while waiting for AutoClean process to end");
		//stopLauncher(false);
		return false;
	}
	return true;
}

bool DecakeLauncher::LaunchBuildUnitExit(int timeoutSec)
{
	if (!isConnected())
	{
		// try reconnect 5 times
		if (!connectRetry(10))
			//stopLauncher(false);
			return false;
	}
	// Powder feeding
	if (!launchBuildUnitExit())
	{
		launcherLogger_->error("DecakeLauncher::LaunchBuildUnitExit: Error while Launching Build Unit Exit");
		//stopLauncher(false);
		return false;
	}
	if (!isConnected())
	{
		if (!connectRetry(10))
			//stopLauncher(false);
			return false;
	}
	if (!waitIntCondition(COND_END_BUEXIT, Isa88ProcessFinalValues, timeoutSec, 10))
	{
		launcherLogger_->error("DecakeLauncher::LaunchBuildUnitExit: Error while waiting for Build Unit Exit process to end");
		//stopLauncher(false);
		return false;
	}
	return true;
}

bool DecakeLauncher::connectRetry(int maxRetry)
{
	launcherLogger_->info("DecakeLauncher::ConnectRetry: '{}' retries", maxRetry);

	int retry = 0;
	bool connectSuccess = false;
	do
	{
		connectSuccess = connectClient();
		if (!connectSuccess) retry++;
	} while ((retry < maxRetry) && (!connectSuccess));
	return connectSuccess;
}

bool DecakeLauncher::isConnected()
{
	std::lock_guard<std::mutex> guard(connectedMutex_);
	return connected_;
}

void DecakeLauncher::stopLauncher(bool success)
{
	launcherLogger_->info("DecakeLauncher::stopLauncher");
	if (client_ != nullptr)
	{
		UA_Client_disconnect(client_);
		UA_Client_delete(client_);
	}
	if (success) 
		exit(EXIT_SUCCESS);
	else 
		exit(EXIT_FAILURE);
}

bool DecakeLauncher::launchAutoDecake(bool autoFine, bool autoCoarse)
{
	launcherLogger_->info("DecakeLauncher::launchWFA");

	bool ret = true;

	// INPUT
	const int inputParamsSize = 2;
	UA_Variant inputParams[inputParamsSize];
	// arg1 autoFine
	UA_Variant_init(&inputParams[0]);
	UA_Variant_setScalarCopy(&inputParams[0], &autoFine, &UA_TYPES[UA_TYPES_BOOLEAN]);
	// arg1 autoCoarse
	UA_Variant_init(&inputParams[1]);
	UA_Variant_setScalarCopy(&inputParams[1], &autoCoarse, &UA_TYPES[UA_TYPES_BOOLEAN]);

	// OUTPUT
	size_t outputSize;
	UA_Variant* output;

	// CALL METHOD
	UA_StatusCode retval = UA_Client_call(	client_, 
											OID_LAUNCH_AUTOCLEAN, 
											MID_LAUNCH_AUTOCLEAN,
											inputParamsSize, 
											inputParams,	
											&outputSize, 
											&output);

	if (retval == UA_STATUSCODE_GOOD)
	{
		launcherLogger_->info("DecakeLauncher::launchAutoDecake: Successfully called method");
	}
	else
	{
		launcherLogger_->error("DecakeLauncher::launchAutoDecake: Error while calling method. Error '{}'", UA_StatusCode_name(retval));
		ret = false;
	}

	// FREE MEMORY
	for (int i = 0; i < inputParamsSize; i++) UA_Variant_deleteMembers(&inputParams[i]);

	return ret;
}

bool DecakeLauncher::launchBuildUnitExit()
{
	launcherLogger_->info("DecakeLauncher::launchBuildUnitExit");

	//bool ret = true;

	//// INPUT
	//const int inputParamsSize = 2;
	//UA_Variant inputParams[inputParamsSize];
	//// arg1 autoFine
	//UA_Variant_init(&inputParams[0]);
	//UA_Variant_setScalarCopy(&inputParams[0], &autoFine, &UA_TYPES[UA_TYPES_BOOLEAN]);
	//// arg1 autoCoarse
	//UA_Variant_init(&inputParams[1]);
	//UA_Variant_setScalarCopy(&inputParams[1], &autoCoarse, &UA_TYPES[UA_TYPES_BOOLEAN]);

	//// OUTPUT
	//size_t outputSize;
	//UA_Variant* output;

	//// CALL METHOD
	//UA_StatusCode retval = UA_Client_call(client_,
	//	OID_LAUNCH_BUEXIT,
	//	MID_LAUNCH_BUEXIT,
	//	inputParamsSize,
	//	inputParams,
	//	&outputSize,
	//	&output);

	//if (retval == UA_STATUSCODE_GOOD)
	//{
	//	launcherLogger_->info("DecakeLauncher::launchBuildUnitExit: Successfully called method");
	//}
	//else
	//{
	//	launcherLogger_->error("DecakeLauncher::launchBuildUnitExit: Error while calling method. Error '{}'", UA_StatusCode_name(retval));
	//	ret = false;
	//}

	//// FREE MEMORY
	//for (int i = 0; i < inputParamsSize; i++) UA_Variant_deleteMembers(&inputParams[i]);

	return false;
}

void DecakeLauncher::clientStateCallback(UA_Client* client, UA_ClientState clientState)
{
	if (clientState == UA_CLIENTSTATE_DISCONNECTED)        /* The client is disconnected */
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UA_CLIENTSTATE_DISCONNECTED");
		{
			std::lock_guard<std::mutex> guard(connectedMutex_);
			connected_ = false;
		}
	}
	else if (clientState == UA_CLIENTSTATE_CONNECTED)           /* A TCP connection to the server is open */
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UA_CLIENTSTATE_CONNECTED");
	}
	else if (clientState == UA_CLIENTSTATE_SECURECHANNEL)       /* A SecureChannel to the server is open */
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UA_CLIENTSTATE_SECURECHANNEL");
	}
	else if (clientState == UA_CLIENTSTATE_SESSION)             /* A session with the server is open */
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UA_CLIENTSTATE_SESSION");
	}
	else if (clientState == UA_CLIENTSTATE_SESSION_RENEWED)      /* A session with the server is open (renewed) */
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UA_CLIENTSTATE_SESSION_RENEWED");
	}
	else
	{
		launcherLogger_->info("DecakeLauncher::clientStateCallback: UNKNOWN!");
	}
}
