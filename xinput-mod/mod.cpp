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
	char workingdir[FILENAME_MAX];

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\xinput.ini";

	return result.str();
}

extern "C"
{
	__declspec(dllexport) void Init(const char* path, const HelperFunctions& helperFunctions)
	{
		std::string config = BuildConfigPath(path);

		if (FileExists(config))
		{
			for (ushort i = 0; i < XPAD_COUNT; i++)
			{
				std::string section = "Controller " + std::to_string(i + 1);

				int deadzoneL = GetPrivateProfileIntA(section.c_str(), "DeadzoneL", XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, config.c_str());
				int deadzoneR = GetPrivateProfileIntA(section.c_str(), "DeadzoneR", XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, config.c_str());

				bool radialL = GetPrivateProfileIntA(section.c_str(), "RadialL", 1, config.c_str()) != 0;
				bool radialR = GetPrivateProfileIntA(section.c_str(), "RadialR", 0, config.c_str()) != 0;

				int triggerThreshold = GetPrivateProfileIntA(section.c_str(), "TriggerThreshold", XINPUT_GAMEPAD_TRIGGER_THRESHOLD, config.c_str());

				xinput::Settings* settings = &xinput::settings[i];
				settings->apply(deadzoneL, deadzoneR, radialL, radialR, triggerThreshold);

				PrintDebug("[XInput] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
					settings->deadzoneL, settings->deadzoneR, settings->triggerThreshold);
			}
		}

		PrintDebug("[XInput] Initialization complete.\n");
	}

	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlength(jumps) } };
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}
