#include "JodoCanopenMotion.h"


void  moveAbs(HANDLE deviceHandle, unsigned short nodeId) {
	PPMMask move = (PPMMask)((WORD)SwitchedOn |
		EnableVoltage |
		QuickStop |
		EnableOperation |
		NewSetpoint |	// this bit must go 0->1 for motion
		Immediately);
	setControlword(deviceHandle, nodeId, move);
}

void moveReset(HANDLE deviceHandle, unsigned short nodeId) {
	PPMMask reset = (PPMMask)((WORD)SwitchedOn |
		EnableVoltage |
		QuickStop |
		EnableOperation);
	setControlword(deviceHandle, nodeId, reset);
}
void  moveRel(HANDLE deviceHandle, unsigned short nodeId) {
	PPMMask move = (PPMMask)((WORD)SwitchedOn |
		EnableVoltage |
		QuickStop |
		EnableOperation |
		NewSetpoint |	// this bit must go 0->1 for motion
		Rel_Abs |
		Immediately);
	setControlword(deviceHandle, nodeId, move);
}

void  setControlword(HANDLE deviceHandle, unsigned short nodeId, WORD control) {
	DWORD errorCode = 0;
	DWORD NbOfBytesWritten = 0;
	bool success = VCS_SetObject(deviceHandle,
		nodeId,
		0x6040,
		0x00,
		&control,
		2,
		&NbOfBytesWritten,
		&errorCode);
	if (!success)
	{
		throw errorCode;
	}
}

WORD getControlword(HANDLE deviceHandle, unsigned short nodeId) {
	DWORD errorCode = 0;
	WORD control = 0;
	DWORD NbOfBytesRead = 0;
	bool success = VCS_GetObject(deviceHandle,
		nodeId,
		0x6040,
		0x00,
		&control,
		2,
		&NbOfBytesRead,
		&errorCode);
	if (!success)
	{
		throw errorCode;
	}
	return control;
}

void  setTargetPosition(HANDLE deviceHandle, unsigned short nodeId, int target) {
	DWORD errorCode = 0;
	DWORD NbOfBytesWritten = 0;
	bool success = VCS_SetObject(deviceHandle,
		nodeId,
		0x607A,
		0x00,
		&target,
		4,
		&NbOfBytesWritten,
		&errorCode);
	if (!success)
	{
		throw errorCode;
	}
}
int  getTargetPosition(HANDLE deviceHandle, unsigned short nodeId) {
	DWORD errorCode = 0;
	int target = 0;
	DWORD NbOfBytesRead = 0;
	bool success = VCS_GetObject(deviceHandle,
		nodeId,
		0x607A,
		0x00,
		&target,
		4,
		&NbOfBytesRead,
		&errorCode);
	if (!success)
	{
		throw errorCode;
	}
	return target;
}
bool  getTargetReached(HANDLE deviceHandle, unsigned short nodeId) {
	DWORD errorCode = 0;
	BOOL targetReached = FALSE;
	bool success = VCS_GetMovementState(deviceHandle, nodeId, &targetReached, &errorCode);
	if (!success)
	{
		throw errorCode;
	}
	return targetReached;
}

long  getCommandPosition(HANDLE deviceHandle, unsigned short nodeId) {
	DWORD errorCode = 0;
	int cmdPos = 0;
	DWORD NbOfBytesRead = 0;
	bool success = VCS_GetObject(deviceHandle,
		nodeId,
		0x6062,
		0x00,
		&cmdPos,
		4,
		&NbOfBytesRead,
		&errorCode);
	if (!success)
	{
		throw errorCode;
	}
	return cmdPos;
}

long  getActualPosition(HANDLE deviceHandle, unsigned short nodeId) {
	DWORD errorCode = 0;
	long actualPos = 0;
	bool success = VCS_GetPositionIs(deviceHandle,
									nodeId,
									&actualPos,
									&errorCode);
	if (!success)
	{
		throw errorCode;
	}
	return actualPos;
}
