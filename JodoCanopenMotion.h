#pragma once
#include "Definitions.h"
enum PPMMask
{
	SwitchedOn = 0x1,
	EnableVoltage = 0x2,
	QuickStop = 0x4,
	EnableOperation = 0x8,
	NewSetpoint = 0x10,
	Immediately = 0x20,
	Rel_Abs = 0x40,
	FaultReset = 0x80,
	Halt = 0x100,
	EndlessMovement = 0x8000,
};
bool  getTargetReached(HANDLE deviceHandle, unsigned short nodeId);
long  getCommandPosition(HANDLE deviceHandle, unsigned short nodeId);
int	  getTargetPosition(HANDLE deviceHandle, unsigned short nodeId);
void  setTargetPosition(HANDLE deviceHandle, unsigned short nodeId, int target);
WORD  getControlword(HANDLE deviceHandle, unsigned short nodeId);
void  setControlword(HANDLE deviceHandle, unsigned short nodeId, WORD control);
void  moveAbs(HANDLE deviceHandle, unsigned short nodeId);
void  moveRel(HANDLE deviceHandle, unsigned short nodeId);
void moveReset(HANDLE deviceHandle, unsigned short nodeId);
