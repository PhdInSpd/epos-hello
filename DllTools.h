#pragma once

#ifdef __linux__
#define DllExport
#define DllImport
#elif _WIN32 || _WIN64
#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)
#endif

#ifdef EXPORT_DLL
#define DLL_API DllExport
#else
#define DLL_API DllImport
#endif