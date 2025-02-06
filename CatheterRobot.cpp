//============================================================================
// Name        : HelloEposCmd.cpp
// Author      : Dawid Sienkiewicz
// Version     :
// Copyright   : maxon motor ag 2014-2021
// Description : Hello Epos in C++
//============================================================================
#include <chrono>
#include <iostream>
#include "Definitions.h"
#include <string.h>
#include <sstream>
// #include <unistd.h>
 #include "getopt.h"
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <math.h>
#include <sys/types.h>
//#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
//#include <sys/times.h>
//#include <sys/time.h>

#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"
#include "testgamecontroller.h"
#include "PLC.h"

typedef void* HANDLE;
typedef int BOOL;

enum EAppMode
{
	AM_UNKNOWN,
	AM_DEMO,
	AM_INTERFACE_LIST,
	AM_PROTOCOL_LIST,
	AM_VERSION_INFO
};

enum JSResponse {
	RUNNING,
	FAULT,
	ACCEPT,
	REJECT,
};


void* g_pKeyHandle = 0;
unsigned short g_usNodeId;
std::string g_deviceName;
std::string g_protocolStackName;
std::string g_interfaceName;
std::string g_portName;
int g_baudrate = 0;

// subdevice
HANDLE subkeyHandle = 0;
std::string subdeviceName = "EPOS4";
std::string subprotocolStackName = "CANopen";


EAppMode g_eAppMode = AM_DEMO;

const std::string g_programName = "HelloEposCmd";

#ifndef MMC_SUCCESS
	#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
	#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
	#define MMC_MAX_LOG_MSG_SIZE 512
#endif


void  PrintUsage();
void  PrintHeader();
void  PrintSettings();
int   OpenDevice(DWORD* p_pErrorCode);
bool  CloseDevice(DWORD* p_pErrorCode);
void  SetDefaultParameters();
bool  ParseArguments(int argc, char** argv);
int   DemoProfilePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD& p_rlErrorCode);

bool  Demo(DWORD* p_pErrorCode);
bool  PrepareDemo(DWORD* p_pErrorCode, WORD nodeId);
int   PrintAvailableInterfaces();
int	  PrintAvailablePorts(char* p_pInterfaceNameSel);
int	  PrintAvailableProtocols();
int main(int argc, char** argv);
bool  PrintDeviceVersion();

/****************************************************************************/
/*	jodo application 														*/
int countOn(const std::vector<bool>& value) {
	int count = std::count_if(
		value.begin(), value.end(),
		[](bool in) { return in; });
	return count;
}

void setAll(std::vector<bool>& value, bool on) {
	std::fill(value.begin(), value.end(), on);
}

JSResponse runJoystickMode(		HANDLE pDevice,
								std::vector<bool>& joyEnable,
								std::string msg,
								DWORD& rErrorCode);

JSResponse runJoystickMode(	HANDLE pDevice,
							std::vector<bool>& joyEnable,
							std::string msg,
							DWORD& rErrorCode) {
	const int NUM_JOY_CHANNELS = 6;
	const float DEAD_BAND = 0.1;

	JSResponse rsp = JSResponse::REJECT;

	LogInfo(msg.c_str());

	for (size_t i = 0; i < NUM_AXES; i++) {
		if (VCS_ActivateProfileVelocityMode(pDevice, 1 + i, &rErrorCode) == 0) {
			LogError("VCS_ActivateProfileVelocityMode", rsp, rErrorCode);
			setAll(joyEnable, false);
			rsp = JSResponse::FAULT;
			return rsp;
		}
	}

	double joySlowVel[NUM_AXES] = { 0.125,	0.125	};	// RPS
	double joyFastVel[NUM_AXES] = { 0.75,	0.500	};	// RPS
	double joyAccel[NUM_AXES]	= { 20,		10		};	// RPS^2
	double * joySelections[2]	= { joySlowVel,		joyFastVel };
	int joySelection = 0;
	
	bool diVelSelect = false;
	FTrigger ftVel, ftAccept, ftReject;
	ftVel.CLK(diVelSelect);

	while (	countOn(joyEnable)/*number of axes enabled*/ > 0) {
		// get joystick left analog 
		handlelGamecontrollerEvents();
		float channels[NUM_JOY_CHANNELS] = { 0 };
		bool connected = getAnalogInputs(channels);

		// accept?
		if (ftAccept.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_A))) {
			LogInfo("ACCEPT");
			setAll(joyEnable, false);
			rsp = JSResponse::ACCEPT;
			continue;
		}

		// reject?
		if (ftReject.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_B))) {
			LogInfo("REJECt");
			setAll(joyEnable, false);
			rsp = JSResponse::REJECT;
			continue;
		}

		// change speed?
		diVelSelect = joystickGetButton(SDL_CONTROLLER_BUTTON_GUIDE);
		if (ftVel.CLK(diVelSelect)) {
			joySelection = !joySelection;
			if (joySelection) {
				LogInfo("fast joy");
			}
			else {
				LogInfo("slow joy"); 
			}
		}

		// apply analog deadband
		for (size_t i = 0; i < NUM_JOY_CHANNELS; i++) {
			if (fabs(channels[i]) < DEAD_BAND) {
				channels[i] = 0;
			}
		}
		
		// map joy to controller axes
		float joyMap[NUM_JOY_CHANNELS] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			joyMap[i] = channels[i];
		}

		// calculate epos velocity
		int targetVels[NUM_AXES] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			targetVels[i] = joyEnable[i]?
							joyMap[i] *
							joySelections[joySelection][i] * 
							sclv[i]:0;
		}

		// set axis joystick
		for (int i = 0; i < NUM_AXES; i++) {
			if (VCS_MoveWithVelocity(pDevice, 1 + i, targetVels[i], &rErrorCode) == 0) {
				rsp = JSResponse::FAULT;
				LogError("VCS_MoveWithVelocity", rsp, rErrorCode);
				setAll(joyEnable, false);
			}
		}
		sleep(16); // 60 updates/sec
	}

	if (rsp != FAULT){
		LogInfo("halt velocity movement");

		for (size_t i = 0; i < NUM_AXES; i++) {
			if (VCS_HaltVelocityMovement(pDevice, 1 + i, &rErrorCode) == 0) {
				rsp = JSResponse::FAULT;
				LogError("VCS_HaltVelocityMovement", rsp, rErrorCode);
			}
		}

	}

	return rsp;
}

JSResponse runEnableMode(HANDLE pDevice,
						std::string msg,
						DWORD& rErrorCode);

JSResponse runEnableMode(HANDLE pDevice,
						std::string msg,
						DWORD& rErrorCode) {
	JSResponse rsp = JSResponse::RUNNING;

	LogInfo(msg.c_str());

	FTrigger ftEnable[NUM_AXES], ftAccept, ftReject;
	bool buttEnable[MAX_NUM_AXES] = { 0 };

	while (rsp == RUNNING) {
		// get joystick left analog 
		handlelGamecontrollerEvents();
		

		// accept?
		if (ftAccept.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_A))) {
			LogInfo("ACCEPT");
			rsp = JSResponse::ACCEPT;
			continue;
		}

		// reject?
		if (ftReject.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_B))) {
			LogInfo("REJECt");
			rsp = JSResponse::REJECT;
			continue;
		}

		// buttons to AXES
		buttEnable[0] = joystickGetButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		buttEnable[1] = joystickGetButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		buttEnable[2] = joystickGetButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		buttEnable[3] = joystickGetButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
		buttEnable[4] = joystickGetButton(SDL_CONTROLLER_BUTTON_BACK);
		buttEnable[5] = joystickGetButton(SDL_CONTROLLER_BUTTON_START);


		// process enable
		for (int i = 0; i < NUM_AXES; i++) {
			if ( ftEnable[i].CLK( buttEnable[i] ) ){
				BOOL isEnabled = false;
				if (!VCS_GetEnableState(pDevice, 1 + i, &isEnabled,&rErrorCode)) {
					LogError("VCS_GetEnableState",
						false,
						rErrorCode);
					rsp = JSResponse::FAULT;
				}
				if (isEnabled){
					if (!VCS_SetDisableState(pDevice, 1 + i, &rErrorCode)) {
						LogError("VCS_SetDisableState",
							false,
							rErrorCode);
						rsp = JSResponse:: FAULT;
					}
				}
				else {
					if (!VCS_SetEnableState(pDevice, 1 + i, &rErrorCode)) {
						LogError("VCS_SetEnableState",
							false,
							rErrorCode);
						rsp = JSResponse::FAULT;
					}

				}
			}
			
		}
		
		sleep(16); // 60 updates/sec
	}
	return rsp;
}

JSResponse runHomeMode(HANDLE pDevice,
						std::string msg,
						int axis,
						DWORD& rErrorCode);

JSResponse runHomeMode(	HANDLE pDevice,
						std::string msg,
						int axis,
						DWORD& rErrorCode) {
	std::vector<bool> joyEnable;
	for (size_t i = 0; i < NUM_AXES; i++) {
		joyEnable.push_back(false);
	}
	joyEnable[axis] = true;
	JSResponse rsp = runJoystickMode(	pDevice,
										joyEnable,
										msg + std::string("axis: ") + std::to_string(axis),
										rErrorCode);

	if (rsp != ACCEPT) {
		return rsp;
	} 
	if (!VCS_ActivateHomingMode(pDevice, 1 + axis, &rErrorCode)) {
		LogError("VCS_ActivateHomingMode",
			false,
			rErrorCode);
		rsp = JSResponse::FAULT;
		return rsp;
	}

	int8_t method = HM_ACTUAL_POSITION;
	if (!VCS_FindHome(pDevice, 1 + axis, method, &rErrorCode)) {
		LogError("VCS_FindHome",
			false,
			rErrorCode);
		rsp = JSResponse::FAULT;
		return rsp;
	}

	return rsp;
}

bool jodoPathDemo(HANDLE keyHandle, DWORD* pErrorCode) {
	std::vector<double> pathVelocity;
	pathVelocity.push_back(1.0);
	pathVelocity.push_back(0.05);
	pathVelocity.push_back(0.1);
	pathVelocity.push_back(0.15);
	pathVelocity.push_back(0.2);
	pathVelocity.push_back(0.25);
	pathVelocity.push_back(0.30);
	pathVelocity.push_back(0.35);
	pathVelocity.push_back(0.40);

	//(REVSm / sec)/sec
	double pathAcceleration = 20;
	std::vector<double> pos[NUM_AXES];

	// units in motor revs
	pos[0].push_back(0.0f);
	pos[0].push_back(0.2f);
	pos[0].push_back(0.5f);
	pos[0].push_back(0.7f);
	pos[0].push_back(1.0f);
	pos[0].push_back(1.3f);
	pos[0].push_back(1.7f);
	pos[0].push_back(1.7f);
	pos[0].push_back(1.7f);

	pos[1].push_back(0.0f);
	pos[1].push_back(0.1f * 2);
	pos[1].push_back(0.2f * 2);
	pos[1].push_back(0.3f * 2);
	pos[1].push_back(0.4f * 2);
	pos[1].push_back(-0.5f * 2);
	pos[1].push_back(0.6f * 2);
	pos[1].push_back(-0.6f * 2);
	pos[1].push_back(-2.8f);
	bool success = jodoContunuousCatheterPath(	keyHandle,
												pathVelocity,
												pathAcceleration,
												pos,
												*pErrorCode);
	if ( !success ) {
		LogError("jodoDemoProfilePositionMode", success, *pErrorCode);
		return success;
	}
	LogInfo("halt position movement");
	haltPositionMovementDrives(subkeyHandle, pErrorCode);
	disableDrives(subkeyHandle, pErrorCode);
	return success;
}
/****************************************************************************/

void PrintUsage()
{
	std::cout << "Usage: HelloEposCmd" << std::endl;
	std::cout << "\t-h : this help" << std::endl;
	std::cout << "\t-n : node id (default 1)" << std::endl;
	std::cout << "\t-d   : device name (EPOS2, EPOS4, default - EPOS4)"  << std::endl;
	std::cout << "\t-s   : protocol stack name (MAXON_RS232, CANopen, MAXON SERIAL V2, default - MAXON SERIAL V2)"  << std::endl;
	std::cout << "\t-i   : interface name (RS232, USB, CAN_ixx_usb 0, CAN_kvaser_usb 0,... default - USB)"  << std::endl;
	std::cout << "\t-p   : port name (COM1, USB0, CAN0,... default - USB0)" << std::endl;
	std::cout << "\t-b   : baudrate (115200, 1000000,... default - 1000000)" << std::endl;
	std::cout << "\t-l   : list available interfaces (valid device name and protocol stack required)" << std::endl;
	std::cout << "\t-r   : list supported protocols (valid device name required)" << std::endl;
	std::cout << "\t-v   : display device version" << std::endl;
}

void SeparatorLine() {
	const int lineLength = 65;
	for(int i=0; i<lineLength; i++) {
		std::cout << "-";
	}
	std::cout << std::endl;
}

void PrintSettings() {
	std::stringstream msg;

	msg << "default settings:" << std::endl;
	msg << "node id             = " << g_usNodeId << std::endl;
	msg << "device name         = '" << g_deviceName << "'" << std::endl;
	msg << "protocal stack name = '" << g_protocolStackName << "'" << std::endl;
	msg << "interface name      = '" << g_interfaceName << "'" << std::endl;
	msg << "port name           = '" << g_portName << "'"<< std::endl;
	msg << "baudrate            = " << g_baudrate;

	LogInfo(msg.str());

	SeparatorLine();
}

void SetDefaultParameters() {
	//USB
	g_usNodeId = 1;
	g_deviceName = "EPOS4"; 
	g_protocolStackName = "MAXON SERIAL V2"; 
	g_interfaceName = "USB"; 
	g_portName = "USB0"; 
	g_baudrate = 1000000; 
}

int OpenDevice(DWORD* p_pErrorCode) {
	int success = false;

	char* pDeviceName = new char[255];
	char* pProtocolStackName = new char[255];
	char* pInterfaceName = new char[255];
	char* pPortName = new char[255];

	strcpy(pDeviceName,  g_deviceName.c_str());
	strcpy(pProtocolStackName, 	g_protocolStackName.c_str());
	strcpy(pInterfaceName,	 	g_interfaceName.c_str());
	strcpy(pPortName,			g_portName.c_str());

	LogInfo("Open device...");

	g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

	if(g_pKeyHandle!=0 && *p_pErrorCode == 0)
	{
		DWORD lBaudrate = 0;
		DWORD lTimeout = 0;

		if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0) {
			if(VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, p_pErrorCode)!=0)
			{
				if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
				{
					if(g_baudrate==(int)lBaudrate)
					{
						subkeyHandle = VCS_OpenSubDevice(g_pKeyHandle, (char *)subdeviceName.c_str(), (char*)subprotocolStackName.c_str(), p_pErrorCode);
						if (subkeyHandle>0) {
							if (VCS_GetGatewaySettings(g_pKeyHandle, &lBaudrate,  p_pErrorCode) != 0) {
								printf("Gateway baudrate = %u\r\n", g_baudrate);
								success = true;
							}

						}

					}
				}
			}
		}
	}
	else
	{
		g_pKeyHandle = 0;
		subkeyHandle = 0;
	}

	delete []pDeviceName;
	delete []pProtocolStackName;
	delete []pInterfaceName;
	delete []pPortName;

	return success;
}

bool CloseDevice(DWORD* p_pErrorCode)
{
	bool success = false;

	*p_pErrorCode = 0;

	LogInfo("Close device");

	if (VCS_CloseSubDevice(subkeyHandle, p_pErrorCode))
	{
		if (VCS_CloseDevice(g_pKeyHandle, p_pErrorCode) != 0 && *p_pErrorCode == 0)
		{
			success = true;
		}
	}
	return success;
}

bool ParseArguments(int argc, char** argv)
{
	int lOption;
	bool success = true;

	opterr = 0;

	while((lOption = getopt(argc, argv, "hlrvd:s:i:p:b:n:")) != -1)
	{
		switch (lOption)
		{
			case 'h':
				PrintUsage();
				success = false;
				break;
			case 'd':
				g_deviceName = optarg;
				break;
			case 's':
				g_protocolStackName = optarg;
				break;
			case 'i':
				g_interfaceName = optarg;
				break;
			case 'p':
				g_portName = optarg;
				break;
			case 'b':
				g_baudrate = atoi(optarg);
				break;
			case 'n':
				g_usNodeId = (unsigned short)atoi(optarg);
				break;
			case 'l':
				g_eAppMode = AM_INTERFACE_LIST;
				break;
			case 'r':
				g_eAppMode = AM_PROTOCOL_LIST;
				break;
			case 'v':
				g_eAppMode = AM_VERSION_INFO;
				break;
			case '?':  // unknown option...
				std::stringstream msg;
				msg << "Unknown option: '" << char(optopt) << "'!";
				LogInfo(msg.str());
				PrintUsage();
				success = false;
				break;
		}
	}

	return success;
}

int DemoProfilePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD & p_rlErrorCode)
{
	int lResult = MMC_SUCCESS;
	std::stringstream msg;

	msg << "set profile position mode, node = " << p_usNodeId;
	LogInfo(msg.str());

	if(VCS_ActivateProfilePositionMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
	{
		LogError("VCS_ActivateProfilePositionMode", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		std::list<long> positionList;

		positionList.push_back(  50000);
		positionList.push_back(-100000);
		positionList.push_back(  50000);
		positionList.push_back(-100006);

		for(std::list<long>::iterator it = positionList.begin(); it !=positionList.end(); it++)
		{
			long targetPosition = (*it);
			std::stringstream msg;
			msg << "move to position = " << targetPosition << ", node = " << p_usNodeId;
			LogInfo(msg.str());

			bool immediate = true;
			bool absolute = true;
			if(VCS_MoveToPosition(p_DeviceHandle, p_usNodeId, targetPosition, absolute, immediate, &p_rlErrorCode) == 0)
			{
				LogError("VCS_MoveToPosition", lResult, p_rlErrorCode);
				lResult = MMC_FAILED;
				break;
			}

			while (!getTargetReached(p_DeviceHandle, p_usNodeId))
			{
				
				//if (VCS_GetPositionMust(p_DeviceHandle, p_usNodeId, &cmdPos, &p_rlErrorCode)) {
				//if (VCS_GetTargetPosition(p_DeviceHandle, p_usNodeId, &cmdPos, &p_rlErrorCode)) {
				long cmdPos = getCommandPosition(p_DeviceHandle, p_usNodeId);
				std::stringstream msg;
				msg << "cmd position = " << cmdPos << ", node = " << p_usNodeId;
				LogInfo(msg.str());
				sleep(1);
			}

			long finalPos = 0;
			if (VCS_GetPositionIs(p_DeviceHandle, p_usNodeId, &finalPos, &p_rlErrorCode) ) {
				std::stringstream msg;
				msg << "final position = " << finalPos << ", node = " << p_usNodeId;
				LogInfo(msg.str());
			}


			sleep(1);
		}
		
		if(lResult == MMC_SUCCESS)
		{
			LogInfo("halt position movement");

			if(VCS_HaltPositionMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
			{
				LogError("VCS_HaltPositionMovement", lResult, p_rlErrorCode);
				lResult = MMC_FAILED;
			}
		}
	}

	return lResult;
}

bool PrepareDemo(DWORD* pErrorCode, WORD nodeId)
{
	bool success = true;
	BOOL oIsFault = 0;

	if(!VCS_GetFaultState(subkeyHandle, nodeId, &oIsFault, pErrorCode ) )
	{
		LogError("VCS_GetFaultState", success, *pErrorCode);
		success = false;
		return success;
	}

	if(oIsFault)
	{
		std::stringstream msg;
		msg << "clear fault, node = '" << nodeId << "'";
		LogInfo(msg.str());

		if(!VCS_ClearFault(subkeyHandle, nodeId, pErrorCode) )
		{
			LogError("VCS_ClearFault", success, *pErrorCode);
			success = false;
			return success;
		}
	}

	BOOL oIsEnabled = 0;
	if(!VCS_GetEnableState(subkeyHandle, nodeId, &oIsEnabled, pErrorCode) )
	{
		LogError("VCS_GetEnableState", success, *pErrorCode);
		success = false;
		return success;
	}

	if(!oIsEnabled)
	{
		if(VCS_SetEnableState(subkeyHandle, nodeId, pErrorCode) == 0)
		{
			LogError("VCS_SetEnableState", success, *pErrorCode);
			success = false;
		}
	}
	return success;
}

int MaxFollowingErrorDemo(DWORD& p_rlErrorCode) {
	int lResult = MMC_SUCCESS;
	const unsigned int EXPECTED_ERROR_CODE = 0x8611;
	DWORD lDeviceErrorCode = 0;

	lResult = VCS_ActivateProfilePositionMode(subkeyHandle, g_usNodeId, &p_rlErrorCode);

	if(lResult)
	{
		lResult = VCS_SetMaxFollowingError(subkeyHandle, g_usNodeId, 1, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = VCS_MoveToPosition(subkeyHandle, g_usNodeId, 1000, 1, 1, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = VCS_GetDeviceErrorCode(subkeyHandle, g_usNodeId, 1, &lDeviceErrorCode, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = lDeviceErrorCode == EXPECTED_ERROR_CODE ? MMC_SUCCESS : MMC_FAILED;
	}

	return lResult;
}

bool Demo(DWORD* pErrorCode) {
	bool success = true;
	std::vector<bool> joyEnable;
	for (size_t i = 0; i < NUM_AXES; i++) {
		joyEnable.push_back(true);
	}

	success = runJoystickMode(subkeyHandle, joyEnable,"Accept or reject", * pErrorCode);
	if(!success) {
		LogError( "DemoProfileVelocityMode", success, *pErrorCode);
		return success;
	}

	success = DemoProfilePositionMode(subkeyHandle, g_usNodeId, *pErrorCode);
	if( !success ) {
		LogError("DemoProfilePositionMode", success, *pErrorCode);
		return success;
	}

	if( !VCS_SetDisableState(subkeyHandle, g_usNodeId, pErrorCode) ) {
		LogError("VCS_SetDisableState", success, *pErrorCode);
		success = false;
	}

	return success;
}

void PrintHeader()
{
	SeparatorLine();

	LogInfo("Epos Command Library Example Program, (c) maxonmotor ag 2014-2019");

	SeparatorLine();
}

int PrintAvailablePorts(char* p_pInterfaceNameSel)
{
	int lResult = MMC_FAILED;
	int lStartOfSelection = 1;
	int lMaxStrSize = 255;
	char* pPortNameSel = new char[lMaxStrSize];
	int lEndOfSelection = 0;
	DWORD ulErrorCode = 0;

	do
	{
		if(!VCS_GetPortNameSelection((char*)g_deviceName.c_str(), (char*)g_protocolStackName.c_str(), p_pInterfaceNameSel, lStartOfSelection, pPortNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
		{
			lResult = MMC_FAILED;
			LogError("GetPortNameSelection", lResult, ulErrorCode);
			break;
		}
		else
		{
			lResult = MMC_SUCCESS;
			printf("            port = %s\n", pPortNameSel);
		}

		lStartOfSelection = 0;
	}
	while(lEndOfSelection == 0);

	return lResult;
}

int PrintAvailableInterfaces()
{
	int lResult = MMC_FAILED;
	int lStartOfSelection = 1;
	int lMaxStrSize = 255;
	char* pInterfaceNameSel = new char[lMaxStrSize];
	int lEndOfSelection = 0;
	DWORD ulErrorCode = 0;

	do
	{
		if(!VCS_GetInterfaceNameSelection((char*)g_deviceName.c_str(), (char*)g_protocolStackName.c_str(), lStartOfSelection, pInterfaceNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
		{
			lResult = MMC_FAILED;
			LogError("GetInterfaceNameSelection", lResult, ulErrorCode);
			break;
		}
		else
		{
			lResult = MMC_SUCCESS;

			printf("interface = %s\n", pInterfaceNameSel);

			PrintAvailablePorts(pInterfaceNameSel);
		}

		lStartOfSelection = 0;
	}
	while(lEndOfSelection == 0);

	SeparatorLine();

	delete[] pInterfaceNameSel;

	return lResult;
}

bool PrintDeviceVersion()
{
	int success = false;
	unsigned short usHardwareVersion = 0;
	unsigned short usSoftwareVersion = 0;
	unsigned short usApplicationNumber = 0;
	unsigned short usApplicationVersion = 0;
	DWORD ulErrorCode = 0;

	if(VCS_GetVersion(g_pKeyHandle, g_usNodeId, &usHardwareVersion, &usSoftwareVersion, &usApplicationNumber, &usApplicationVersion, &ulErrorCode))
	{
		printf("%s Hardware Version    = 0x%04x\n      Software Version    = 0x%04x\n      Application Number  = 0x%04x\n      Application Version = 0x%04x\n",
				g_deviceName.c_str(), usHardwareVersion, usSoftwareVersion, usApplicationNumber, usApplicationVersion);
		success = true;
	}

	return success;
}

int PrintAvailableProtocols()
{
	int lResult = MMC_FAILED;
	int lStartOfSelection = 1;
	int lMaxStrSize = 255;
	char* pProtocolNameSel = new char[lMaxStrSize];
	int lEndOfSelection = 0;
	DWORD ulErrorCode = 0;

	do
	{
		if(!VCS_GetProtocolStackNameSelection((char*)g_deviceName.c_str(), lStartOfSelection, pProtocolNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
		{
			lResult = MMC_FAILED;
			LogError("GetProtocolStackNameSelection", lResult, ulErrorCode);
			break;
		}
		else
		{
			lResult = MMC_SUCCESS;

			printf("protocol stack name = %s\n", pProtocolNameSel);
		}

		lStartOfSelection = 0;
	}
	while(lEndOfSelection == 0);

	SeparatorLine();

	delete[] pProtocolNameSel;

	return lResult;
}


int main(int argc, char** argv)
{
	bool success = false;
	DWORD ulErrorCode = 0;

	if (!initializeGamecontroller()) {
		return 1;
	}

	PrintHeader();

	SetDefaultParameters();

	if( !(success = ParseArguments(argc, argv)) ) {
		return success;
	}

	PrintSettings();

	switch(g_eAppMode) {
		case AM_DEMO: {
			if( !(success = OpenDevice(&ulErrorCode)) ) {
				LogError("OpenDevice", success, ulErrorCode);
				return success;
			}

			for (size_t i = 0; i < NUM_AXES; i++) {
				if ( !(success = PrepareDemo(&ulErrorCode, 1+i) ) ) {
					LogError("PrepareDemo", success, ulErrorCode);
					return success;
				}
			}

			std::vector<bool> joyEnable;
			for (size_t i = 0; i < NUM_AXES; i++) {
				joyEnable.push_back(true);
			}
			JSResponse rsp = runJoystickMode(subkeyHandle,
											joyEnable,
											"accet(A) or reject(B)",
											ulErrorCode);

			if ( !( success = (rsp==ACCEPT) ) ) {
				LogError("runJoystickMode", success, ulErrorCode);
				return success;
			}

			rsp = runEnableMode(subkeyHandle,
								"Enable/DisabLe left(1) right(2) down(3) up(4) share(5) option(6)",
								ulErrorCode);

			if (!(success = (rsp == ACCEPT))) {
				LogError("runEnableMode", success, ulErrorCode);
				return success;
			}

			for (size_t i = 0; i < NUM_AXES; i++) {
				rsp = runHomeMode(subkeyHandle,
					"Jog to position and Home(A) or skip(B)",
					i,
					ulErrorCode);
				if (!(success = (rsp != FAULT))) {
					LogError("runHomeMode", success, ulErrorCode);
					return success;
				}
			}

			long finalPos[NUM_AXES] = { 0 };
			bool gotPos = true;
			for (size_t i = 0; i < NUM_AXES; i++) {
				gotPos = VCS_GetPositionIs(subkeyHandle, 1 + i, &finalPos[i], &ulErrorCode) && gotPos;
			}
			if (gotPos) {
				std::stringstream msg;
				msg << "final position = " << finalPos[0] / scld[0] << "," << finalPos[0] << "," << finalPos[1] / scld[1] << "," << finalPos[1];
				LogInfo(msg.str());
			}
			

			if( !jodoPathDemo(subkeyHandle , &ulErrorCode) ) {
				LogError("jodoPathDemo", false, ulErrorCode);
				return false;
			}

			if(!(success = CloseDevice(&ulErrorCode))) {
				LogError("CloseDevice", success, ulErrorCode);
				return success;
			}
		} break;
		case AM_INTERFACE_LIST:
			PrintAvailableInterfaces();
			break;
		case AM_PROTOCOL_LIST:
			PrintAvailableProtocols();
			break;
		case AM_VERSION_INFO:
		{
			if( !(success = OpenDevice(&ulErrorCode)) )
			{
				LogError("OpenDevice", success, ulErrorCode);
				return success;
			}

			if( !(success = PrintDeviceVersion()) )
		    {
				LogError("PrintDeviceVersion", success, ulErrorCode);
				return success;
		    }

			if( !(success = CloseDevice(&ulErrorCode)) )
			{
				LogError("CloseDevice", success, ulErrorCode);
				return success;
			}
		} break;
		case AM_UNKNOWN:
			printf("unknown option\n");
			break;
	}
	closeGamecontroller();
	return success;
}
