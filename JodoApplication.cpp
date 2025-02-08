#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include "PLC.h"
#include "TeachData.h"


void LogError(std::string functionName, int p_lResult, unsigned int p_ulErrorCode)
{
	std::cerr <<  functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << std::endl;
}

void LogInfo(std::string message)
{
	std::cout << message << std::endl;
}

/// <summary>
/// on all drives
/// </summary>
/// <param name="keyHandle"></param>
/// <param name="pErrorCode"></param>
/// <returns></returns>
bool disableDrives(HANDLE keyHandle, DWORD* pErrorCode) {
	bool success = true;
	for (WORD i = 0; i < NUM_AXES; i++) {
		if (!VCS_SetDisableState(keyHandle, 1+i, pErrorCode)) {
			LogError("VCS_SetDisableState",
				false,
				*pErrorCode);
			success = false;
		}
	}
	return success;
}

/// <summary>
/// on all drives
/// </summary>
/// <param name="keyHandle"></param>
/// <param name="pErrorCode"></param>
/// <returns></returns>
bool enableDrives(HANDLE keyHandle, DWORD* pErrorCode) {
	bool success = true;
	for (WORD i = 0; i < NUM_AXES; i++)
	{
		if (!VCS_SetEnableState(keyHandle, 1 + i, pErrorCode))
		{
			LogError("VCS_SetEnableState",
				false,
				*pErrorCode);
			success = false;
		}
	}
	return success;
}

/// <summary>
/// on all drives
/// </summary>
/// <param name="keyHandle"></param>
/// <param name="pErrorCode"></param>
/// <returns></returns>
bool haltPositionMovementDrives(HANDLE keyHandle, DWORD* pErrorCode) {
	bool success = true;
	for (size_t i = 0; i < NUM_AXES; i++) {
		if (!VCS_HaltPositionMovement( keyHandle, 1 + i, pErrorCode)) {
			LogError("VCS_HaltPositionMovement",
				false,
				*pErrorCode);
			success = false;
		}
	}
	return success;
}

/// <summary>
/// on all drives
/// </summary>
/// <param name="keyHandle"></param>
/// <param name="pErrorCode"></param>
/// <returns></returns>
bool activateProfilePositionModeDrives(HANDLE keyHandle, DWORD* pErrorCode) {
	bool success = true;
	for (size_t i = 0; i < NUM_AXES; i++) {
		if (!VCS_ActivateProfilePositionMode(keyHandle, 1 + i, pErrorCode)) {
			LogError("VCS_HaltPositionMovement",
				false,
				*pErrorCode);
			success = false;
		}
	}
	return success;
}

void getActualPositionDrives(HANDLE keyHandle, long pos[]) {
	for (size_t i = 0; i < NUM_AXES; i++) {
		pos[i] = getActualPosition(keyHandle, 1 + i);
	}
}

/* continuous path from given profile*/
bool jodoContinuousCatheterPath(HANDLE pDevice,
	const  TeachData & data,
	DWORD& rErrorCode) {
	bool success = true;

	LogInfo("set profile position mode");

	if (!(success = activateProfilePositionModeDrives(pDevice, &rErrorCode))) {
		return success;
	}

	double unitDirection[NUM_AXES] = { 0 };

	int noPoints = data.points.size();
	for (size_t point = 0; point < noPoints; point++) {
		double startPos[NUM_AXES] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			startPos[i] = point == 0 ?
				getCommandPosition(pDevice, 1 + i) / scld[i] :
				//getCommandPosition(pDevice, 1 + i) / scld[i];
				data.points[point - 1][i];
		}

		double delta[NUM_AXES] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			delta[i] = data.points[point][i] - startPos[i];
		}

		// results with zero magnitude are invalid unitDirection
		int faildedDelta = 0;
		for (size_t i = 0; i < NUM_AXES; i++) {
			faildedDelta += (long)(delta[i] * scld[i]) == 0 ? 1 : 0;
		}
		if (faildedDelta >= NUM_AXES) {
			LogInfo("FAILED MAGNITUDE");
			continue;
		}

		double nextDelta[NUM_AXES] = { 0 };
		bool nextSegmentContinuous = false;
		// does next getment exist?
		if ( (point + 1) < noPoints) {
			for (size_t i = 0; i < NUM_AXES; i++) {
				nextDelta[i] = data.points[point+1][i] - data.points[point][i];
			}
			faildedDelta = 0;
			for (size_t i = 0; i < NUM_AXES; i++) {
				faildedDelta += (long)(nextDelta[i] * scld[i]) == 0 ? 1 : 0;
			}
			if (faildedDelta < NUM_AXES) {
				// sigh must have same direction
				bool signPassed = true;
				for (size_t i = 0; i < NUM_AXES; i++) {
					// sign flipped
					if (nextDelta[i] * delta[i] < 0) {
						signPassed = false;
						break;
					}
				}
				nextSegmentContinuous = signPassed;
			}
		}

		std::stringstream msg;
		msg << "abs move = " << delta[0] << "," << delta[1];
		LogInfo(msg.str());

		double magSquared = 0;
		for (size_t i = 0; i < NUM_AXES; i++) {
			magSquared += (delta[i] * delta[i]);
		}
		double magnitude = sqrt(magSquared);
		for (size_t i = 0; i < NUM_AXES; i++) {
			unitDirection[i] = (long)(delta[i] * scld[i]) != 0 ? delta[i] / magnitude : 1;
			// If unitdirection is to low the move takes too long
			unitDirection[i] = fabs(unitDirection[i]) >= 0.001 ?  unitDirection[i]:1;
		}

		DWORD	profileVelocity[NUM_AXES] = { 0 },
			profileAcceleration[NUM_AXES] = { 0 },
			profileDeceleration[NUM_AXES] = { 0 };

		for (size_t i = 0; i < NUM_AXES; i++) {
			int last = data.points[point].size() - 1;
			profileVelocity[i] = fabs((double)(data.points[point][last] * sclv[i] * unitDirection[i]));
			profileAcceleration[i] = fabs((double)(data.pathAcceleration * scla[i] * unitDirection[i]));
			profileDeceleration[i] = fabs((double)(data.pathAcceleration * scla[i] * unitDirection[i]));
		}

		for (size_t i = 0; i < NUM_AXES; i++) {
			if (!VCS_SetPositionProfile(pDevice,
				1 + i,
				profileVelocity[i],
				profileAcceleration[i],
				profileDeceleration[i],
				&rErrorCode)) {
				std::string error = "VCS_SetPositionProfile " + i;
				LogError(error, success, rErrorCode);
				success = false;
				return success;
			}
		}

		if (nextSegmentContinuous) {
			for (size_t i = 0; i < NUM_AXES; i++) {
				setTargetPosition(pDevice, 1 + i, data.points[point + 1][i] * scld[i]);
			}
		}
		else {
			for (size_t i = 0; i < NUM_AXES; i++) {
				setTargetPosition(pDevice, 1 + i, data.points[point][i] * scld[i]);
			}
		}


		// 0:	all axis start move at the same time.
		//		Only works for unit1
		// moveRel(pDeviceHandle, 0);
		double start = SEC_TIME();
		for (size_t i = 0; i < NUM_AXES; i++) {
			moveAbs(pDevice, 1 + i);
		}
		// only moves unit 1. using key or subkey 
		//moveAbs(pDevice, 0);
		double end = SEC_TIME();

		double elapsed = end - start;
		std::stringstream msgMove;
		msgMove << nextSegmentContinuous + 1 << "segments = " << elapsed;
		LogInfo(msgMove.str());

		for (size_t i = 0; i < NUM_AXES; i++) {
			moveReset(pDevice, 1 + i);
		}

		if (nextSegmentContinuous) {
			// wait trigger reached
			bool triggerReached = true;
			long cmdPos[NUM_AXES] = { 0 };
			long triggerPos = data.points[point][0] * scld[0];

			do {
				// each getCommandPosition() takes 2.5 ms
				start = SEC_TIME();
				for (size_t i = 0; i < NUM_AXES; i++) {
					cmdPos[i] = getCommandPosition(pDevice, 1 + i);
				}
				end = SEC_TIME();
				double elapsed = end - start;

				if (delta[0] < 0) {
					triggerReached = cmdPos[0] <= triggerPos;
				}
				else {
					triggerReached = cmdPos[0] >= triggerPos;
				}
				//sleep(1);
			} while (!triggerReached);
			std::stringstream msg;
			msg << " trigger done: cmdPos = ";
			for (size_t i = 0; i < NUM_AXES; i++) {
				msg << cmdPos[i] << "\t";
			}
			msg <<  elapsed;
			LogInfo(msg.str());
		}
		else {
			// wait target reached
			bool targetsReached = true;
			do {
				// each getCommandPosition() takes 2.5 ms
				start = SEC_TIME();
				long cmdPos[NUM_AXES] = { 0 };
				for (size_t i = 0; i < NUM_AXES; i++) {
					cmdPos[i] = getCommandPosition(pDevice, 1 + i);
				}
				end = SEC_TIME();
				double elapsed = end - start;

				std::stringstream msg;
				msg << "cmdPos= ";
				for (size_t i = 0; i < NUM_AXES; i++) {
					msg << cmdPos[i] << "\t";
				}
				msg <<  elapsed;
				LogInfo(msg.str());

				targetsReached = true;
				for (size_t i = 0; i < NUM_AXES; i++) {
					targetsReached = getTargetReached(pDevice, 1 + i) && targetsReached;
				}
				sleep(1);
			} while (!targetsReached);
		}

		long finalPos[NUM_AXES] = { 0 };
		getActualPositionDrives(pDevice, finalPos);
		double finalPosUserUnits[NUM_AXES] = { 0 };
		scalePosition(finalPos, finalPosUserUnits);
		printPosition("finalPosUI= ", finalPosUserUnits);
		printPosition("finalPoI= ", finalPos);
		sleep(1);
	}
	return success;
}