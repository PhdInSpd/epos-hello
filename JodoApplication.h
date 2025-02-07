#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "TeachData.h"

#ifdef  WIN32
	#define sleep Sleep
#endif //  WIN32


/***************************************************************/
/* messaging*/
void  LogError(std::string functionName, int p_lResult, unsigned int p_ulErrorCode);
void  LogInfo(std::string message);
/*************************************************************/

/*************************************************************/
/* drives motion*/
bool disableDrives(HANDLE keyHandle, DWORD* pErrorCode);
bool enableDrives(HANDLE keyHandle, DWORD* pErrorCode);
void getActualPositionDrives(HANDLE keyHandle, long pos[] );
template <typename T>
void printPosition(std::string show, T pos[]) {
	std::stringstream msg;
	msg << show;
	for (size_t i = 0; i < NUM_AXES; i++) {
		msg << pos[i] / scld[i] << ",";
	}
	LogInfo(msg.str());
}

bool haltPositionMovementDrives(HANDLE keyHandle, DWORD* pErrorCode);
bool activateProfilePositionModeDrives(HANDLE keyHandle, DWORD* pErrorCode);
/************************************************************/

/************************************************************/
/* continuous path from given profile*/
bool jodoContunuousCatheterPath(HANDLE pDevice,
								const  TeachData& data,
								DWORD& rErrorCode);
/*****************************************************************/