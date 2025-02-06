#include "PLC.h"

double DLL_API SEC_TIME() {
	static auto  start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<double>(end - start);
	return duration.count();
}