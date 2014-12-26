// Microsoft stuff
#include <Windows.h>
#include <direct.h>	// for _getcwd

// Standard library
#include <sstream>	// because
#include <limits>	// for min()

// Mod loader
#include <SADXModLoader.h>

// Local stuff
#include "typedefs.h"
#include "FileExists.h"
#include "UpdateControllersXInput.h"

PointerInfo jumps[] = {
	{ (void*)(0x0040F460), xinput::UpdateControllersXInput },
	/*{ (void*)(0x004BCB60), xinput::Rumble },*/	// Disabled because it didn't make sense.
	{ (void*)(0x004BCBC0), xinput::RumbleLarge },
	{ (void*)(0x004BCC10), xinput::RumbleSmall }
};

char _path[FILENAME_MAX];

void _cdecl xinput_main(const char *path, const HelperFunctions &helperFunctions)
{
	using namespace std;

	_getcwd(_path, FILENAME_MAX);

	// because
	// TODO: not
	stringstream ss;
	ss << _path << "\\" << path << "\\xinput.ini";
	string configPath = ss.str();

	if (FileExists(configPath))
	{
		for (uint i = 0; i < 4; i++)
		{
			std::string section = "Controller " + to_string(i + 1);
			int l, r, t;

			l = GetPrivateProfileIntA(section.c_str(), "DeadZoneL", -1, configPath.c_str());
			r = GetPrivateProfileIntA(section.c_str(), "DeadZoneR", -1, configPath.c_str());
			t = GetPrivateProfileIntA(section.c_str(), "TriggerThreshold", -1, configPath.c_str());

			xinput::SetDeadzone(xinput::deadzone::stickL, i, l);
			xinput::SetDeadzone(xinput::deadzone::stickR, i, r);
			xinput::SetDeadzone(xinput::deadzone::triggers, i, t);

			PrintDebug("[XInput] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
				xinput::deadzone::stickL[i], xinput::deadzone::stickR[i], xinput::deadzone::triggers[i]);
		}
	}

	PrintDebug("[XInput] Initialization complete.");
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
