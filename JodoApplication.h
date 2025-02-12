#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "xplatform.h"
#include "TeachData.h"

using namespace std;
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
// Method 1: Using std::setw and std::left/right (for fixed-width positioning)
void setTextPosition(int x, int y);
void textFixedWidth(const std::string& text, int width, char fillChar = ' ');
void displayTextFixedWidth(const std::string& text, int x, int y, int width, char fillChar = ' ');
std::string doubleToStringFixed(double value, int precision);
void displayMotion(vector<string> headers,
	vector<double> p0,
	vector<double> p1,
	int startX = 100,
	int precision = 3,
	int XLength = 30);

using Action = void (*)(HANDLE keyHandle); // Creates a type alias named MathFunction

JoyRsp runJoystickMode(HANDLE pDevice,
	std::vector<bool>& joyEnable,
	std::string msg,
	DWORD& rErrorCode,
	Action updateUI=nullptr);

int countOn(const std::vector<bool>& value);

void setAll(std::vector<bool>& value, bool on);

/* continuous path from given profile*/
bool jodoContinuousCatheterPath(HANDLE pDevice,
								const  TeachData& data,
								DWORD& rErrorCode);
bool jodoJogCatheterPath(HANDLE pDevice,
						const  TeachData& data,
						TeachData& copy,
						Action showStatus,
						DWORD& rErrorCode);
double getVectorMagnitude(std::vector<double>& delta);
int countFailedDeltas(std::vector<double> delta);
int countFlippedDeltas(const std::vector<double>& delta, const std::vector<double>& nextDelta);
/*****************************************************************/