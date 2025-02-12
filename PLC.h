#pragma once
#include <chrono>
#include "DllTools.h"

double DLL_API SEC_TIME();

class DLL_API Trigger
{
protected:
	bool _oldInput;
public:
	bool Input;
	bool Q;
public:
	Trigger() {
		_oldInput = false;
		Input = false;
		Q = false;
	}
	
	bool CLK(bool clk) {
		Input = clk;
		return Run();
	}
	virtual bool Run() = 0;
};

//Rising Trigger
class DLL_API RTrigger :public Trigger {
public:
	virtual bool Run() {
		Q =  !_oldInput && Input;
		_oldInput = Input;
		return Q;
	}
};

//Falling Trigger
class DLL_API FTrigger :public Trigger {
public:
	virtual bool Run() {
		Q = _oldInput && !Input;
		_oldInput = Input;
		return Q;
	}
};

//Edge Trigger
class DLL_API ETrigger :public Trigger {
public:
	virtual bool Run() {
		bool bFall = _oldInput  && !Input;
		bool bRise = !_oldInput && Input;
		Q = bFall || bRise;
		_oldInput = Input;
		return Q;
	}
};

// Number Changed
template<typename T> class NumTrigger {
protected:
	T _oldInput;
public:
	T Input;

	bool Q;
	NumTrigger() {
		_oldInput = 0;
		Input = 0;
		Q = false;
	}
	bool CLK(long clk) {
		Input = clk;
		return Run();
	}
	bool Run() {
		Q = (_oldInput != Input);
		_oldInput = Input;
		return	Q;
	}
};

class DLL_API Ton : public Trigger {
protected:
	RTrigger _rtInput;
	double	_timeout;
public:
	double DelaySec;	//In seconds
	Ton() {
		_timeout = 0;
		DelaySec = 1;
	}


	virtual bool Run() {
		if (_rtInput.CLK(Input)) {
			_timeout = SEC_TIME() + DelaySec;
			
		}

		Q = Input ? SEC_TIME() >= _timeout : false;
		return Q;
	}
};