#pragma once
#include <string>
#include <vector>

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
bool haltPositionMovementDrives(HANDLE keyHandle, DWORD* pErrorCode);
bool activateProfilePositionModeDrives(HANDLE keyHandle, DWORD* pErrorCode);
/************************************************************/

/************************************************************/
/* continuous path from given profile*/
bool jodoContunuousCatheterPath(HANDLE pDevice,
								const  std::vector<double>& pathVelocity,	// (REVSm / sec)
								const double& pathAcceleration,
								const std::vector<double> pos[NUM_AXES],
								DWORD& rErrorCode);
/*****************************************************************/