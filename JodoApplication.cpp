#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"
#include <iostream>
#include <sstream>
#include <chrono>


void LogError(std::string functionName, int p_lResult, unsigned int p_ulErrorCode)
{
	std::cerr <<  functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << std::endl;
}

void LogInfo(std::string message)
{
	std::cout << message << std::endl;
}
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

bool haltPositionMovementDrives(HANDLE keyHandle, DWORD* pErrorCode) {
	bool success = true;
	for (size_t i = 0; i < NUM_AXES; i++)
	{
		if (!VCS_HaltPositionMovement( keyHandle, 1 + i, pErrorCode))
		{
			LogError("VCS_HaltPositionMovement",
				false,
				*pErrorCode);
			success = false;
		}
	}
	return success;
}

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

/* continuous path from given profile*/
bool jodoContunuousCatheterPath(HANDLE pDevice,
	const  std::vector<double>& pathVelocity,	// (REVSm / sec)
	const double& pathAcceleration,
	const std::vector<double> pos[NUM_AXES],
	DWORD& rErrorCode) {
	bool success = true;

	LogInfo("set profile position mode");

	if (!(success = activateProfilePositionModeDrives(pDevice, &rErrorCode)))
	{
		return success;
	}

	double unitDirection[NUM_AXES] = { 0 };

	int noPoints = pos[0].size();
	for (size_t point = 0; point < noPoints; point++) {
		double startPos[NUM_AXES] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			startPos[i] = point == 0 ?
				getCommandPosition(pDevice, 1 + i) / scld[i] :
				//getCommandPosition(pDevice, 1 + i) / scld[i];
				pos[i][point - 1];
		}

		double delta[NUM_AXES] = { 0 };
		for (size_t i = 0; i < NUM_AXES; i++) {
			delta[i] = pos[i][point] - startPos[i];
		}

		// results has zero magnitude and invalid unitDirection
		int faildedDelta = 0;
		for (size_t i = 0; i < NUM_AXES; i++) {
			faildedDelta += (long)(delta[i] * scld[i]) == 0 ? 1 : 0;
		}
		if (faildedDelta >= NUM_AXES) {
			LogInfo("FAILED MAGNITUDE");
			continue;
		}

		double nextDelta[NUM_AXES] = { 0 };
		bool nextSegmentValid = false;
		// does next getment exist?
		if ((point + 1) < noPoints) {
			for (size_t i = 0; i < NUM_AXES; i++) {
				nextDelta[i] = pos[i][point + 1] - pos[i][point];
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
				nextSegmentValid = signPassed;
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
		}

		DWORD	profileVelocity[NUM_AXES] = { 0 },
			profileAcceleration[NUM_AXES] = { 0 },
			profileDeceleration[NUM_AXES] = { 0 };

		for (size_t i = 0; i < NUM_AXES; i++) {
			profileVelocity[i] = fabs((double)(pathVelocity[point] * sclv[i] * unitDirection[i]));
			profileAcceleration[i] = fabs((double)(pathAcceleration * scla[i] * unitDirection[i]));
			profileDeceleration[i] = fabs((double)(pathAcceleration * scla[i] * unitDirection[i]));
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

		if (nextSegmentValid) {
			for (size_t i = 0; i < NUM_AXES; i++) {
				setTargetPosition(pDevice, 1 + i, pos[i][point + 1] * scld[i]);
			}
		}
		else {
			for (size_t i = 0; i < NUM_AXES; i++) {
				setTargetPosition(pDevice, 1 + i, pos[i][point] * scld[i]);
			}
		}


		// 0:	all axis start move at the same time.
		//		Only works for unit1
		// moveRel(pDeviceHandle, 0);
		auto start = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < NUM_AXES; i++) {
			moveAbs(pDevice, 1 + i);
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = end - start;
		std::stringstream msgMove;
		msgMove << nextSegmentValid + 1 << "segments = " << elapsed.count();
		LogInfo(msgMove.str());

		for (size_t i = 0; i < NUM_AXES; i++) {
			moveReset(pDevice, 1 + i);
		}

		if (nextSegmentValid)
		{
			// wait trigger reached
			bool triggerReached = true;
			long cmdPos[NUM_AXES] = { 0 };
			long triggerPos = pos[0][point] * scld[0];

			do
			{
				// each getCommandPosition() takes 2.5 ms
				start = std::chrono::high_resolution_clock::now();
				for (size_t i = 0; i < NUM_AXES; i++) {
					cmdPos[i] = getCommandPosition(pDevice, 1 + i);
				}
				end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;

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
				msg << cmdPos[i] << ",";
			}
			msg << " :" << elapsed.count();
			LogInfo(msg.str());
		}
		else {
			// wait target reached
			bool targetsReached = true;
			do
			{
				// each getCommandPosition() takes 2.5 ms
				start = std::chrono::high_resolution_clock::now();
				long cmdPos[NUM_AXES] = { 0 };
				for (size_t i = 0; i < NUM_AXES; i++) {
					cmdPos[i] = getCommandPosition(pDevice, 1 + i);
				}
				end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;

				std::stringstream msg;
				msg << "cmd position = ";
				for (size_t i = 0; i < NUM_AXES; i++) {
					msg << cmdPos[i] << ",";
				}
				msg << " :" << elapsed.count();
				LogInfo(msg.str());

				targetsReached = true;
				for (size_t i = 0; i < NUM_AXES; i++) {
					targetsReached = getTargetReached(pDevice, 1 + i) && targetsReached;
				}
				sleep(1);
			} while (!targetsReached);
		}

		long finalPos[NUM_AXES] = { 0 };
		bool gotPos = true;
		for (size_t i = 0; i < NUM_AXES; i++) {
			gotPos = VCS_GetPositionIs(pDevice, 1 + i, &finalPos[i], &rErrorCode) && gotPos;
		}
		if (gotPos) {
			std::stringstream msg;
			msg << "final position = " << finalPos[0] / scld[0] << "," << finalPos[0] << "," << finalPos[1] / scld[1] << "," << finalPos[1];
			LogInfo(msg.str());
		}
		sleep(1);
	}
	return success;
}