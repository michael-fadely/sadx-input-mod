#include <SADXModLoader.h>
#include "WriteControllerXInput.h"

PointerInfo jumps[] = {
	{ (void*)(0x0040F460), WriteControllerXInput }
};

extern "C"				// Required for proper export
__declspec(dllexport)	// This data is being exported from this DLL
ModInfo SADXModInfo = {
	ModLoaderVer,		// Struct version
	NULL,				// Initialization function
	NULL, 0,			// List of Patches & Patch Count
	arrayptrandlength(jumps),			// List of Jumps & Jump Count
	NULL, 0,			// List of Calls & Call Count
	NULL, 0,			// List of Pointers & Pointer Count
};
