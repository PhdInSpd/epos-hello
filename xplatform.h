#pragma once
#ifdef __linux__
// call when linux
#include <unistd.h>
using DWORD = unsigned int;
using WORD = unsigned short;
using HANDLE = void *;
using BOOL = int;
#define FALSE 0
#define TRUE 1
#elif _WIN32 || _WIN64
#endif
