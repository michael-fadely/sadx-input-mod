// Microsoft stuff
#include <Windows.h>
#include <direct.h>	// for _getcwd

// Standard library
#include <string>
#include <sstream>	// because

// Mod loader
#include <SADXModLoader.h>

// Local stuff
#include "typedefs.h"
#include "FileExists.h"
#include "UpdateControllersXInput.h"

void* onInput				= (void*)0x0040FDB3; // For future endeavors
void* UpdateControllers_ptr = (void*)0x0040F460;
void* RumbleLarge_ptr		= (void*)0x004BCBC0;
void* RumbleSmall_ptr		= (void*)0x004BCC10;
void* Rumble_ptr			= (void*)0x004BCB60; // Unused, but here so I don't lose it.

PointerInfo jumps[] = {
	{ UpdateControllers_ptr,	xinput::UpdateControllersXInput },
	{ RumbleLarge_ptr,			xinput::RumbleLarge },
	{ RumbleSmall_ptr,			xinput::RumbleSmall }
};

std::string BuildConfigPath(const char* modpath)
{
	std::stringstream result;
	char* workingdir = new char[FILENAME_MAX];

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\xinput.ini";
	delete[] workingdir;

	return result.str();
}

extern "C"
{
	__declspec(dllexport) void _cdecl Init(const char* path, const HelperFunctions& helperFunctions)
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

	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlength(jumps) } };
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}