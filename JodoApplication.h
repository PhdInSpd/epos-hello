#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "TeachData.h"

#ifdef  WIN32
	#define sleep Sleep
#endif //  WIN32


enum JoyRsp {
	RUNNING,
	FAULT,
	ACCEPT,
	REJECT,
};
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
std::string formatArray(std::string show, T pos[]) {
	std::stringstream msg;
	msg << show;
	for (size_t i = 0; i < NUM_AXES; i++) {
		msg << pos[i] / scld[i] << "\t";
	}
	return(msg.str());
}

template <typename T>
std::string formatArray(std::string show, std::vector<T> pos) {
	std::stringstream msg;
	msg << show;
	for (size_t i = 0; i < pos.size(); i++) {
		msg << pos[i] / scld[i] << "\t";
	}
	return(msg.str());
}

template <typename T>
void printPosition(std::string show, T pos[]) {
	LogInfo(formatArray(show,pos));
}

template <typename T>
void printPosition(std::string show, T pos[], std::string suffix) {
	LogInfo(formatArray(show, pos)+suffix);
}

bool haltPositionMovementDrives(HANDLE keyHandle, DWORD* pErrorCode);
bool activateProfilePositionModeDrives(HANDLE keyHandle, DWORD* pErrorCode);
/************************************************************/

/************************************************************/
JoyRsp runJoystickMode(HANDLE pDevice,
	std::vector<bool>& joyEnable,
	std::string msg,
	DWORD& rErrorCode);

int countOn(const std::vector<bool>& value);

void setAll(std::vector<bool>& value, bool on);

/* continuous path from given profile*/
bool jodoContinuousCatheterPath(HANDLE pDevice,
								const  TeachData& data,
								DWORD& rErrorCode);
double getVectorMagnitude(std::vector<double>& delta);
int countFailedDeltas(std::vector<double> delta);
int countFlippedDeltas(const std::vector<double>& delta, const std::vector<double>& nextDelta);
/*****************************************************************/