#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm> // For std::transform
#include "PLC.h"
#include "TeachData.h"
#include "testgamecontroller.h"


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
/// <summary>
/// Count the number Of Elements that are zero
/// </summary>
/// <param name="delta"></param>
/// <returns></returns>
int countFailedDeltas(std::vector<double> delta) {
	int index = 0;
	int failures = std::count_if(delta.begin(), delta.end(), [&index](double dlt) {
		long driveDist = dlt * scld[index];
		index++;
		return driveDist == 0 ? 1 : 0;
		});
	return failures;
}

int countFlippedDeltas(const std::vector<double>& delta,const std::vector<double>& nextDelta) {
	// sigh must have same direction
	int fails = 0;
	for (size_t i = 0; i < NUM_AXES; i++) {
		// sign flipped
		if (nextDelta[i] * delta[i] < 0) {
			fails++;
		}
	}
	return fails;
}

int countOn(const std::vector<bool>& value) {
	int count = std::count_if(
		value.begin(), value.end(),
		[](bool in) { return in; });
	return count;
}

void setAll(std::vector<bool>& value, bool on) {
	std::fill(value.begin(), value.end(), on);
}

JoyRsp runJoystickMode(HANDLE pDevice,
	std::vector<bool>& joyEnable,
	std::string msg,
	DWORD& rErrorCode) {
	const int NUM_JOY_CHANNELS = 6;
	const float DEAD_BAND = 0.1;

	JoyRsp rsp = JoyRsp::REJECT;

	LogInfo(msg.c_str());

	for (size_t i = 0; i < NUM_AXES; i++) {
		if (VCS_ActivateProfileVelocityMode(pDevice, 1 + i, &rErrorCode) == 0) {
			LogError("VCS_ActivateProfileVelocityMode", rsp, rErrorCode);
			setAll(joyEnable, false);
			rsp = JoyRsp::FAULT;
			return rsp;
		}
	}

	double joySlowVel[MAX_NUM_AXES] = { .050,	.050,	.050,	.050, 	.050,	.050, };	// RPS
	double joyFastVel[MAX_NUM_AXES] = { .150,	.150, 	.150,	.150, .150,	.150, };	// RPS
	double joyAccel[MAX_NUM_AXES] = { 20,		20,		20,		20,		20,		20, };	// RPS^2
	double* joySelections[2] = { joySlowVel,		joyFastVel };
	int joySelection = 0;

	bool diVelSelect = false;
	FTrigger ftVel, ftAccept, ftReject;
	ftVel.CLK(diVelSelect);

	while (countOn(joyEnable)/*number of axes enabled*/ > 0) {
		// get joystick left analog 
		handlelGamecontrollerEvents();
		float channels[NUM_JOY_CHANNELS] = { 0 };
		bool connected = getAnalogInputs(channels);

		bool axis4AI = joystickGetButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		bool axis5AI = joystickGetButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		channels[4] = axis4AI ? -1 : channels[4];
		channels[5] = axis5AI ? -1 : channels[5];
		// accept?
		if (ftAccept.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_A))) {
			LogInfo("ACCEPT");
			setAll(joyEnable, false);
			rsp = JoyRsp::ACCEPT;
			continue;
		}

		// reject?
		if (ftReject.CLK(joystickGetButton(SDL_CONTROLLER_BUTTON_B))) {
			LogInfo("REJECt");
			setAll(joyEnable, false);
			rsp = JoyRsp::REJECT;
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
			targetVels[i] = joyEnable[i] ?
				joyMap[i] *
				joySelections[joySelection][i] *
				sclv[i] : 0;
		}

		// set axis joystick
		for (int i = 0; i < NUM_AXES; i++) {
			if (VCS_MoveWithVelocity(pDevice, 1 + i, targetVels[i], &rErrorCode) == 0) {
				rsp = JoyRsp::FAULT;
				LogError("VCS_MoveWithVelocity", rsp, rErrorCode);
				setAll(joyEnable, false);
			}
		}
		sleep(16); // 60 updates/sec
	}

	if (rsp != FAULT) {
		LogInfo("halt velocity movement");

		for (size_t i = 0; i < NUM_AXES; i++) {
			if (VCS_HaltVelocityMovement(pDevice, 1 + i, &rErrorCode) == 0) {
				rsp = JoyRsp::FAULT;
				LogError("VCS_HaltVelocityMovement", rsp, rErrorCode);
			}
		}

	}

	return rsp;
}
/// <summary>
/// continuous path from given profilecontinuous path from given profile
/// </summary>
/// <param name="pDevice"></param>
/// <param name="data"></param>
/// <param name="rErrorCode"></param>
/// <returns></returns>
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
		#pragma region calculate segment i profile to make line


		std::vector<double> startPos(NUM_AXES);
		for (size_t i = 0; i < NUM_AXES; i++) {
			startPos[i] = point == 0 ?
				getCommandPosition(pDevice, 1 + i) / scld[i] :
				//getCommandPosition(pDevice, 1 + i) / scld[i];
				data.points[point - 1][i];
		}

		std::vector<double> delta(NUM_AXES);
		for (size_t i = 0; i < NUM_AXES; i++) {
			delta[i] = data.points[point][i] - startPos[i];
		}
		

	/*	int faildedDelta = 0;
		for (size_t i = 0; i < NUM_AXES; i++) {
			long driveDist = delta[i] * scld[i];
			faildedDelta += driveDist == 0 ? 1 : 0;
		}
		if (faildedDelta >= NUM_AXES) {
			LogInfo("FAILED MAGNITUDE");
			continue;
		}*/

		// results with zero magnitude are invalid unitDirection
		int faildedDelta = countFailedDeltas(delta);
		if (faildedDelta >= NUM_AXES) {
			LogInfo("FAILED MAGNITUDE");
			continue;
		}

		std::vector<double> nextDelta(NUM_AXES);
		bool nextSegmentContinuous = false;
		// does next gegment exist?
		if ( (point + 1) < noPoints) {
			for (size_t i = 0; i < NUM_AXES; i++) {
				nextDelta[i] = data.points[point+1][i] - data.points[point][i];
			}
			faildedDelta = countFailedDeltas(nextDelta);
			if (faildedDelta < NUM_AXES) {
				int countFlipped = countFlippedDeltas(delta, nextDelta);
				nextSegmentContinuous = countFlipped <= 0;
			}
		}
		std::cout <<formatArray("rel move = ", delta)<<std::endl ;

		double magnitude = getVectorMagnitude(delta);

		// if delta  is not 0 move will not happen so set unitDirection[i] to 1
		for (size_t i = 0; i < NUM_AXES; i++) {
			unitDirection[i] = (long)(delta[i] * scld[i]) != 0 ? delta[i] / magnitude : 1;
		}

		int triggerAxis = -1;
		double unitDirMag = 0;
		// largest fabs(unitDirection[i]) is trigger axis
		for (size_t i = 0; i < NUM_AXES; i++) {
			if ((long)(delta[i] * scld[i]) == 0) {
				continue;
			}
			if (fabs(unitDirection[i])>unitDirMag){
				triggerAxis = i;
				unitDirMag = fabs(unitDirection[i]);
			}
		}

		// If vector component is to low the move takes too long so set unitDirrection[i] to 1
		for (size_t i = 0; i < NUM_AXES; i++) {
			unitDirection[i] = fabs(unitDirection[i]) >= 0.001 ? unitDirection[i] : 1;
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
		#pragma endregion

		// set line profile
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

		// set endPoint
		int endPoint = nextSegmentContinuous ? point + 1 : point;
		for (size_t i = 0; i < NUM_AXES; i++) {
			setTargetPosition(pDevice, 1 + i, data.points[endPoint][i] * scld[i]);
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
		msgMove << nextSegmentContinuous + 1 << ":segments = " << elapsed;
		LogInfo(msgMove.str());

		for (size_t i = 0; i < NUM_AXES; i++) {
			moveReset(pDevice, 1 + i);
		}

		if (nextSegmentContinuous) {
			// wait trigger reached
			bool triggerReached = true;
			long cmdPos[NUM_AXES] = { 0 };
			long triggerPos = data.points[point][triggerAxis] * scld[triggerAxis];

			do {
				// each getCommandPosition() takes 2.5 ms
				start = SEC_TIME();
				for (size_t i = 0; i < NUM_AXES; i++) {
					cmdPos[i] = getCommandPosition(pDevice, 1 + i);
				}
				end = SEC_TIME();
				elapsed = end - start;

				if (delta[triggerAxis] < 0) {
					triggerReached = cmdPos[triggerAxis] <= triggerPos;
				}
				else {
					triggerReached = cmdPos[triggerAxis] >= triggerPos;
				}
				//sleep(1);
			} while (!triggerReached);
			printPosition("trgPos done : cmdPos = ", cmdPos, std::to_string(elapsed));
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
				printPosition("cmdPos= ", cmdPos, std::to_string(elapsed));

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
		printPosition("finalPos= ", finalPos);
		sleep(1);
	}
	return success;
}

bool jodoJogCatheterPath(HANDLE pDevice,
	const  TeachData& data,
	DWORD& rErrorCode) {
	bool success = true;

	LogInfo("set profile position mode");

	

	double unitDirection[NUM_AXES] = { 0 };

	int noPoints = data.points.size();
	for (size_t point = 0; point < noPoints; point++) {
		if (!(success = activateProfilePositionModeDrives(pDevice, &rErrorCode))) {
			return success;
		}

	#pragma region calculate segment i profile to make line


		std::vector<double> startPos(NUM_AXES);
		for (size_t i = 0; i < NUM_AXES; i++) {
			startPos[i] = point == 0 ?
				getCommandPosition(pDevice, 1 + i) / scld[i] :
				//getCommandPosition(pDevice, 1 + i) / scld[i];
				data.points[point - 1][i];
		}

		std::vector<double> delta(NUM_AXES);
		for (size_t i = 0; i < NUM_AXES; i++) {
			delta[i] = data.points[point][i] - startPos[i];
		}

		// results with zero magnitude are invalid unitDirection
		int faildedDelta = countFailedDeltas(delta);
		if (faildedDelta >= NUM_AXES) {
			LogInfo("FAILED MAGNITUDE");
			continue;
		}

		std::cout << formatArray("rel move = ", delta) << std::endl;

		double magnitude = getVectorMagnitude(delta);

		// if delta  is not 0 move will not happen so set unitDirection[i] to 1
		for (size_t i = 0; i < NUM_AXES; i++) {
			unitDirection[i] = (long)(delta[i] * scld[i]) != 0 ? delta[i] / magnitude : 1;
		}

		// If vector component is to low the move takes too long so set unitDirrection[i] to 1
		for (size_t i = 0; i < NUM_AXES; i++) {
			unitDirection[i] = fabs(unitDirection[i]) >= 0.001 ? unitDirection[i] : 1;
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
		#pragma endregion

		// set line profile
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

		// set endPoint
		int endPoint = point;
		for (size_t i = 0; i < NUM_AXES; i++) {
			setTargetPosition(pDevice, 1 + i, data.points[endPoint][i] * scld[i]);
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

		for (size_t i = 0; i < NUM_AXES; i++) {
			moveReset(pDevice, 1 + i);
		}

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
			printPosition("cmdPos= ", cmdPos, std::to_string(elapsed));

			targetsReached = true;
			for (size_t i = 0; i < NUM_AXES; i++) {
				targetsReached = getTargetReached(pDevice, 1 + i) && targetsReached;
			}
			sleep(1);
		} while (!targetsReached);

		long finalPos[NUM_AXES] = { 0 };
		getActualPositionDrives(pDevice, finalPos);
		double finalPosUserUnits[NUM_AXES] = { 0 };
		scalePosition(finalPos, finalPosUserUnits);
		printPosition("finalPosUI= ", finalPosUserUnits);
		printPosition("finalPos= ", finalPos);

		std::vector<bool> joyEnable(NUM_AXES, true);
		JoyRsp rsp = runJoystickMode(pDevice,
			joyEnable,
			"verify point: accept(A) or reject(B)",
			rErrorCode);

		if (!((rsp == ACCEPT))) {
			LogError("runJoystickMode", false, rErrorCode);
		}

		sleep(1);
	}
	return success;
}

double getVectorMagnitude(std::vector<double>& delta)
{
	double magSquared = 0;
	for (size_t i = 0; i < NUM_AXES; i++) {
		magSquared += (delta[i] * delta[i]);
	}
	return sqrt(magSquared);
}
