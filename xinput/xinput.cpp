#include <SADXModLoader.h>
#include "UpdateControllersXInput.h"

PointerInfo jumps[] = {
	{ (void*)(0x0040F460), UpdateControllersXInput },
	{ (void*)(0x004BCB60), Rumble },
	{ (void*)(0x004BCBC0), RumbleA },
	{ (void*)(0x004BCC10), RumbleB }
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
