#pragma once

#define DllExport	__declspec(dllexport)
#define DllImport	__declspec(dllimport)

#ifdef EXPORT_DLL
#define DLL_API	DllExport
#else
#define DLL_API	DllImport?
#endif