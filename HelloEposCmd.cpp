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
//#include <sys/times.h>
//#include <sys/time.h>
#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"

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

void* g_pKeyHandle = 0;
//void* g_pCanKey    = 0;
unsigned short g_usNodeId;
std::string g_deviceName;
std::string g_protocolStackName;
std::string g_interfaceName;
std::string g_portName;
int g_baudrate = 0;
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
int   CloseDevice(DWORD* p_pErrorCode);
void  SetDefaultParameters();
int   ParseArguments(int argc, char** argv);
int   DemoProfilePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD& p_rlErrorCode);

int   Demo(DWORD* p_pErrorCode);
int   PrepareDemo(DWORD* p_pErrorCode, WORD nodeId);
int   PrintAvailableInterfaces();
int	  PrintAvailablePorts(char* p_pInterfaceNameSel);
int	  PrintAvailableProtocols();
int   PrintDeviceVersion();

/****************************************************************************/
/*	jodo application 														*/



bool jodoPathDemo(DWORD* pErrorCode) {
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
	bool success = jodoContunuousCatheterPath(	g_pKeyHandle,
												pathVelocity,
												pathAcceleration,
												pos,
												*pErrorCode);
	if (!success) {
		LogError("jodoDemoProfilePositionMode", success, *pErrorCode);
		return success;
	}
	LogInfo("halt position movement");
	haltPositionMovementDrives(g_pKeyHandle, pErrorCode);
	disableDrives(g_pKeyHandle, pErrorCode);
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


void SeparatorLine()
{
	const int lineLength = 65;
	for(int i=0; i<lineLength; i++)
	{
		std::cout << "-";
	}
	std::cout << std::endl;
}

void PrintSettings()
{
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

void SetDefaultParameters()
{
	//USB
	g_usNodeId = 1;
	g_deviceName = "EPOS4"; 
	g_protocolStackName = "MAXON SERIAL V2"; 
	g_interfaceName = "USB"; 
	g_portName = "USB0"; 
	g_baudrate = 1000000; 
}

int OpenDevice(DWORD* p_pErrorCode)
{
	int lResult = MMC_FAILED;

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

		if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
		{
			if(VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, p_pErrorCode)!=0)
			{
				if(VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode)!=0)
				{
					if(g_baudrate==(int)lBaudrate)
					{
						lResult = MMC_SUCCESS;
						//g_pCanKey = VCS_OpenSubDevice(g_pKeyHandle, (char*)"EPOS4", (char*)"canopen", p_pErrorCode);

					}
				}
			}
		}
	}
	else
	{
		//g_pCanKey = 0;
		g_pKeyHandle = 0;
	}

	delete []pDeviceName;
	delete []pProtocolStackName;
	delete []pInterfaceName;
	delete []pPortName;

	return lResult;
}

int CloseDevice(DWORD* p_pErrorCode)
{
	int lResult = MMC_FAILED;

	*p_pErrorCode = 0;

	LogInfo("Close device");

	if(VCS_CloseDevice(g_pKeyHandle, p_pErrorCode)!=0 && *p_pErrorCode == 0)
	{
		lResult = MMC_SUCCESS;
	}

	return lResult;
}

int ParseArguments(int argc, char** argv)
{
	int lOption;
	int lResult = MMC_SUCCESS;

	opterr = 0;

	while((lOption = getopt(argc, argv, "hlrvd:s:i:p:b:n:")) != -1)
	{
		switch (lOption)
		{
			case 'h':
				PrintUsage();
				lResult = 1;
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
				lResult = MMC_FAILED;
				break;
		}
	}

	return lResult;
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

bool DemoProfileVelocityMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD & p_rlErrorCode)
{
	int lResult = MMC_SUCCESS;
	std::stringstream msg;

	msg << "set profile velocity mode, node = " << p_usNodeId;

	LogInfo(msg.str());

	if(VCS_ActivateProfileVelocityMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
	{
		LogError("VCS_ActivateProfileVelocityMode", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		std::list<long> velocityList;

		velocityList.push_back(100);
		velocityList.push_back(500);
		velocityList.push_back(1000);

		for(std::list<long>::iterator it = velocityList.begin(); it !=velocityList.end(); it++)
		{
			long targetvelocity = (*it);

			std::stringstream msg;
			msg << "move with target velocity = " << targetvelocity << " rpm, node = " << p_usNodeId;
			LogInfo(msg.str());

			if(VCS_MoveWithVelocity(p_DeviceHandle, p_usNodeId, targetvelocity, &p_rlErrorCode) == 0)
			{
				lResult = MMC_FAILED;
				LogError("VCS_MoveWithVelocity", lResult, p_rlErrorCode);
				break;
			}

			sleep(1);
		}

		if(lResult == MMC_SUCCESS)
		{
			LogInfo("halt velocity movement");

			if(VCS_HaltVelocityMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
			{
				lResult = MMC_FAILED;
				LogError("VCS_HaltVelocityMovement", lResult, p_rlErrorCode);
			}
		}
	}

	return lResult;
}

int PrepareDemo(DWORD* pErrorCode, WORD nodeId)
{
	int lResult = MMC_SUCCESS;
	BOOL oIsFault = 0;

	if(!VCS_GetFaultState(g_pKeyHandle, nodeId, &oIsFault, pErrorCode ) )
	{
		LogError("VCS_GetFaultState", lResult, *pErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	if(oIsFault)
	{
		std::stringstream msg;
		msg << "clear fault, node = '" << nodeId << "'";
		LogInfo(msg.str());

		if(!VCS_ClearFault(g_pKeyHandle, nodeId, pErrorCode) )
		{
			LogError("VCS_ClearFault", lResult, *pErrorCode);
			lResult = MMC_FAILED;
			return lResult;
		}
	}

	BOOL oIsEnabled = 0;
	if(!VCS_GetEnableState(g_pKeyHandle, nodeId, &oIsEnabled, pErrorCode) )
	{
		LogError("VCS_GetEnableState", lResult, *pErrorCode);
		lResult = MMC_FAILED;
		return lResult;
	}

	if(!oIsEnabled)
	{
		if(VCS_SetEnableState(g_pKeyHandle, nodeId, pErrorCode) == 0)
		{
			LogError("VCS_SetEnableState", lResult, *pErrorCode);
			lResult = MMC_FAILED;
		}
	}
	return lResult;
}

int MaxFollowingErrorDemo(DWORD& p_rlErrorCode) {
	int lResult = MMC_SUCCESS;
	const unsigned int EXPECTED_ERROR_CODE = 0x8611;
	DWORD lDeviceErrorCode = 0;

	lResult = VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &p_rlErrorCode);

	if(lResult)
	{
		lResult = VCS_SetMaxFollowingError(g_pKeyHandle, g_usNodeId, 1, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, 1000, 1, 1, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = VCS_GetDeviceErrorCode(g_pKeyHandle, g_usNodeId, 1, &lDeviceErrorCode, &p_rlErrorCode);
	}

	if(lResult)
	{
		lResult = lDeviceErrorCode == EXPECTED_ERROR_CODE ? MMC_SUCCESS : MMC_FAILED;
	}

	return lResult;
}

int Demo(DWORD* pErrorCode)
{
	int lResult = MMC_SUCCESS;

	lResult = DemoProfileVelocityMode(g_pKeyHandle, g_usNodeId, *pErrorCode);
	if(lResult != MMC_SUCCESS)
	{
		LogError("DemoProfileVelocityMode", lResult, *pErrorCode);
		return lResult;
	}

	lResult = DemoProfilePositionMode(g_pKeyHandle, g_usNodeId, *pErrorCode);
	if(lResult != MMC_SUCCESS)
	{
		LogError("DemoProfilePositionMode", lResult, *pErrorCode);
		return lResult;
	}

	if(VCS_SetDisableState(g_pKeyHandle, g_usNodeId, pErrorCode) == 0)
	{
		LogError("VCS_SetDisableState", lResult, *pErrorCode);
		lResult = MMC_FAILED;
	}

	return lResult;
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

int PrintDeviceVersion()
{
	int lResult = MMC_FAILED;
	unsigned short usHardwareVersion = 0;
	unsigned short usSoftwareVersion = 0;
	unsigned short usApplicationNumber = 0;
	unsigned short usApplicationVersion = 0;
	DWORD ulErrorCode = 0;

	if(VCS_GetVersion(g_pKeyHandle, g_usNodeId, &usHardwareVersion, &usSoftwareVersion, &usApplicationNumber, &usApplicationVersion, &ulErrorCode))
	{
		printf("%s Hardware Version    = 0x%04x\n      Software Version    = 0x%04x\n      Application Number  = 0x%04x\n      Application Version = 0x%04x\n",
				g_deviceName.c_str(), usHardwareVersion, usSoftwareVersion, usApplicationNumber, usApplicationVersion);
		lResult = MMC_SUCCESS;
	}

	return lResult;
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
	int lResult = MMC_FAILED;
	DWORD ulErrorCode = 0;

	PrintHeader();

	SetDefaultParameters();

	if((lResult = ParseArguments(argc, argv))!=MMC_SUCCESS)
	{
		return lResult;
	}

	PrintSettings();

	switch(g_eAppMode)
	{
		case AM_DEMO:
		{
			if((lResult = OpenDevice(&ulErrorCode))!=MMC_SUCCESS) {
				LogError("OpenDevice", lResult, ulErrorCode);
				return lResult;
			}

			for (size_t i = 0; i < NUM_AXES; i++)
			{
				if ((lResult = PrepareDemo(&ulErrorCode, g_usNodeId+i)) != MMC_SUCCESS) {
					LogError("PrepareDemo", lResult, ulErrorCode);
					return lResult;
				}
			}

			if( !jodoPathDemo(&ulErrorCode) )
			{
				LogError("jodoPathDemo", false, ulErrorCode);
				return false;
			}

			if((lResult = CloseDevice(&ulErrorCode))!=MMC_SUCCESS)
			{
				LogError("CloseDevice", lResult, ulErrorCode);
				return lResult;
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
			if((lResult = OpenDevice(&ulErrorCode))!=MMC_SUCCESS)
			{
				LogError("OpenDevice", lResult, ulErrorCode);
				return lResult;
			}

			if((lResult = PrintDeviceVersion()) != MMC_SUCCESS)
		    {
				LogError("PrintDeviceVersion", lResult, ulErrorCode);
				return lResult;
		    }

			if((lResult = CloseDevice(&ulErrorCode))!=MMC_SUCCESS)
			{
				LogError("CloseDevice", lResult, ulErrorCode);
				return lResult;
			}
		} break;
		case AM_UNKNOWN:
			printf("unknown option\n");
			break;
	}

	return lResult;
}
