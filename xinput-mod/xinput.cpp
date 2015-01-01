// Microsoft stuff
#include <Windows.h>
#include <direct.h>	// for _getcwd

// Standard library
#include <string>
#include <sstream>	// because
#include <limits>	// for min()

// Mod loader
#include <SADXModLoader.h>

// Local stuff
#include "typedefs.h"
#include "FileExists.h"
#include "UpdateControllersXInput.h"

void* UpdateControllers_ptr	= (void*)(0x0040F460);
void* RumbleLarge_ptr		= (void*)(0x004BCBC0);
void* RumbleSmall_ptr		= (void*)(0x004BCC10);
void* Rumble_ptr			= (void*)(0x004BCB60); // Unused, but here so I don't lose it.

PointerInfo jumps[] = {
	{ UpdateControllers_ptr, xinput::UpdateControllersXInput },
	{ RumbleLarge_ptr, xinput::RumbleLarge },
	{ RumbleSmall_ptr, xinput::RumbleSmall }
};

std::string BuildConfigPath(const char* modpath)
{
	std::stringstream result;
	char* workingdir = new char[FILENAME_MAX];
	
	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\xinput.ini";
	delete[] workingdir;

	return result.str();
}

void _cdecl xinput_main(const char* path, const HelperFunctions& helperFunctions)
{
	std::string config = BuildConfigPath(path);

	if (FileExists(config))
	{
		for (uint i = 0; i < 4; i++)
		{
			std::string section = "Controller " + std::to_string(i + 1);
			int l, r, t;

			l = GetPrivateProfileIntA(section.c_str(), "DeadZoneL", -1, config.c_str());
			r = GetPrivateProfileIntA(section.c_str(), "DeadZoneR", -1, config.c_str());
			t = GetPrivateProfileIntA(section.c_str(), "TriggerThreshold", -1, config.c_str());

			xinput::SetDeadzone(xinput::deadzone::stickL, i, l);
			xinput::SetDeadzone(xinput::deadzone::stickR, i, r);
			xinput::SetDeadzone(xinput::deadzone::triggers, i, t);

			PrintDebug("[XInput] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
				xinput::deadzone::stickL[i], xinput::deadzone::stickR[i], xinput::deadzone::triggers[i]);
		}
	}

	PrintDebug("[XInput] Initialization complete.\n");
}

extern "C"						// Required for proper export
__declspec(dllexport)			// This data is being exported from this DLL
ModInfo SADXModInfo = {
	ModLoaderVer,				// Struct version
	xinput_main,				// Initialization function
	NULL, 0,					// List of Patches & Patch Count
	arrayptrandlength(jumps),	// List of Jumps & Jump Count
	NULL, 0,					// List of Calls & Call Count
	NULL, 0,					// List of Pointers & Pointer Count
};
