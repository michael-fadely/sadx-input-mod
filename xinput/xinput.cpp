#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "SADXModLoader.h"

extern "C"				// Required for proper export
__declspec(dllexport)	// This data is being exported from this DLL
ModInfo SADXModInfo = {
	ModLoaderVer,		// Struct version
	NULL,				// Initialization function
	NULL, 0,			// List of Patches & Patch Count
	NULL, 0,			// List of Jumps & Jump Count
	NULL, 0,			// List of Calls & Call Count
	NULL, 0,			// List of Pointers & Pointer Count
};
