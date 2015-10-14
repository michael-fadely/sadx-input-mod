#include "stdafx.h"
// Microsoft stuff
#include <Windows.h>
#include <direct.h>	// for _getcwd

// Standard library
#include <string>
#include <sstream>	// because

#include "SDL.h"

// Mod loader
#include <SADXModLoader.h>

// Local stuff
#include "typedefs.h"
#include "FileExists.h"
#include "Ingame.h"

void* RumbleLarge_ptr		= (void*)0x004BCBC0;
void* RumbleSmall_ptr		= (void*)0x004BCC10;
void* Rumble_ptr			= (void*)0x004BCB60; // Unused, but here so I don't lose it.
void* UpdateControllers_ptr = (void*)0x0040F460;

PointerInfo jumps[] = {
	{ RumbleLarge_ptr, input::RumbleLarge },
	{ RumbleSmall_ptr, input::RumbleSmall },
	// Used to skip over the standard controller update function.
	// This has no effect on the OnInput hook.
	//{ UpdateControllers_ptr, (void*)0x0040FDB3 }
};

PointerInfo calls[] = {
	{ (void*)0x0040FEE6, input::WriteAnalogsWrapper }
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
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };

	__declspec(dllexport) PointerList	Jumps[] = { { arrayptrandlength(jumps) } };
	__declspec(dllexport) PointerList	Calls[] = { { arrayptrandlength(calls) } };

	__declspec(dllexport) void OnInput()
	{
		input::PollControllers();
	}

	__declspec(dllexport) void Init(const char* path, const HelperFunctions& helperFunctions)
	{
		int init;
		if ((init = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_EVENTS)) != 0)
		{
			PrintDebug("Unable to initialize SDL. Error code: %i\n", init);
			MessageBoxA(nullptr, "Error initializing SDL. See debug message for details.", "SDL Init Error", 0);
		}

		std::string config = BuildConfigPath(path);

		if (FileExists(config))
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
			{
				std::string section = "Controller " + std::to_string(i + 1);
				const char* section_cstr = section.c_str();
				const char* config_cstr = config.c_str();

				int deadzoneL = GetPrivateProfileIntA(section_cstr, "DeadzoneL", GAMEPAD_LEFT_THUMB_DEADZONE, config_cstr);
				int deadzoneR = GetPrivateProfileIntA(section_cstr, "DeadzoneR", GAMEPAD_RIGHT_THUMB_DEADZONE, config_cstr);

				bool radialL = GetPrivateProfileIntA(section_cstr, "RadialL", 1, config_cstr) != 0;
				bool radialR = GetPrivateProfileIntA(section_cstr, "RadialR", 0, config_cstr) != 0;

				int triggerThreshold = GetPrivateProfileIntA(section_cstr, "TriggerThreshold", GAMEPAD_TRIGGER_THRESHOLD, config_cstr);

				// TODO: Not this
				char wtf[255];

				GetPrivateProfileStringA(section_cstr, "RumbleFactor", "1.0", wtf, 255, config_cstr);
				float rumbleFactor = (float)atof(wtf);

				DreamPad::Settings* settings = &DreamPad::Controllers[i].settings;
				settings->apply(deadzoneL, deadzoneR, radialL, radialR, triggerThreshold, rumbleFactor);

				PrintDebug("[XInput] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
					settings->deadzoneL, settings->deadzoneR, settings->triggerThreshold);
			}
		}

		PrintDebug("[XInput] Initialization complete.\n");
	}
}
