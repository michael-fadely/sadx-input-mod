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
#include "input.h"
#include "rumble.h"

void* RumbleA_ptr				= (void*)0x004BCBC0;
void* RumbleB_ptr				= (void*)0x004BCC10;
void* Rumble_ptr				= (void*)0x004BCB60; // Unused, but here so I don't lose it.
void* UpdateControllers_ptr		= (void*)0x0040F460;
void* AnalogHook_ptr			= (void*)0x0040F343;
void* InitRawControllers_ptr	= (void*)0x0040F451; // End of function (hook)

PointerInfo jumps[] = {
	{ rumble::pdVibMxStop, rumble::pdVibMxStop_hook },
	{ RumbleA_ptr,				rumble::RumbleA },
	{ RumbleB_ptr,				rumble::RumbleB },
	{ AnalogHook_ptr,			input::WriteAnalogs_Hook },
	{ InitRawControllers_ptr,	input::RedirectRawControllers_Hook },
	{ EnableController,			input::EnableController_hook },
	{ DisableController,		input::DisableController_hook }
	// Used to skip over the standard controller update function.
	// This has no effect on the OnInput hook.
	//{ UpdateControllers_ptr, (void*)0x0040FDB3 }
};

std::string BuildModPath(const char* modpath, const char* path)
{
	std::stringstream result;
	char workingdir[FILENAME_MAX];

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\" << path;

	return result.str();
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlength(jumps) } };

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
			MessageBoxA(nullptr, "Error initializing SDL. See debug message for details.",
				"SDL Init Error", MB_OK | MB_ICONERROR);
			return;
		}

		// EnableControl
		WriteData((bool**)0x40EF80, &input::_ControllerEnabled[0]);
		WriteData((bool**)0x40EF86, &input::_ControllerEnabled[1]);
		WriteData((bool**)0x40EF90, input::_ControllerEnabled);

		// DisableControl
		WriteData((bool**)0x40EFB0, &input::_ControllerEnabled[0]);
		WriteData((bool**)0x40EFB6, &input::_ControllerEnabled[1]);
		WriteData((bool**)0x40EFC0, input::_ControllerEnabled);

		// IsControllerEnabled
		WriteData((bool**)0x40EFD8, input::_ControllerEnabled);

		// Control
		WriteData((bool**)0x40FE0D, input::_ControllerEnabled);
		WriteData((bool**)0x40FE2F, &input::_ControllerEnabled[1]);

		// WriteAnalogs
		WriteData((bool**)0x40F30C, input::_ControllerEnabled);

		input::_ControllerEnabled[0] = true;
		input::_ControllerEnabled[1] = true;
		
		std::string dbpath = BuildModPath(path, "gamecontrollerdb.txt");

		if (FileExists(dbpath))
		{
			int result = SDL_GameControllerAddMappingsFromFile(dbpath.c_str());

			if (result == -1)
				PrintDebug("[Input] Error loading gamecontrollerdb: %s\n", SDL_GetError());
			else
				PrintDebug("[Input] Controller mappings loaded: %i\n", result);
		}

		std::string config = BuildModPath(path, "config.ini");

		if (FileExists(config))
		{
#ifdef _DEBUG
			bool debug_default = true;
#else
			bool debug_default = false;
#endif

			const char* config_cstr = config.c_str();
			input::debug = GetPrivateProfileIntA("Config", "Debug", (int)debug_default, config_cstr) != 0;

			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
			{
				std::string section = "Controller " + std::to_string(i + 1);
				const char* section_cstr = section.c_str();

				int deadzoneL = GetPrivateProfileIntA(section_cstr, "DeadzoneL", GAMEPAD_LEFT_THUMB_DEADZONE, config_cstr);
				int deadzoneR = GetPrivateProfileIntA(section_cstr, "DeadzoneR", GAMEPAD_RIGHT_THUMB_DEADZONE, config_cstr);

				bool radialL = GetPrivateProfileIntA(section_cstr, "RadialL", 1, config_cstr) != 0;
				bool radialR = GetPrivateProfileIntA(section_cstr, "RadialR", 0, config_cstr) != 0;

				int triggerThreshold = GetPrivateProfileIntA(section_cstr, "TriggerThreshold", GAMEPAD_TRIGGER_THRESHOLD, config_cstr);

				// TODO: Not this
				char wtf[255];

				GetPrivateProfileStringA(section_cstr, "RumbleFactor", "1.0", wtf, 255, config_cstr);
				float rumbleFactor = min(1.0f, (float)atof(wtf));

				bool megaRumble = GetPrivateProfileIntA(section_cstr, "MegaRumble", 0, config_cstr) != 0;

				DreamPad::Settings* settings = &DreamPad::Controllers[i].settings;
				settings->apply(deadzoneL, deadzoneR, radialL, radialR, triggerThreshold, rumbleFactor, megaRumble);

				if (input::debug)
				{
					PrintDebug("[Input] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
						settings->deadzoneL, settings->deadzoneR, settings->triggerThreshold);
				}
			}
		}

		PrintDebug("[Input] Initialization complete.\n");
	}
}
