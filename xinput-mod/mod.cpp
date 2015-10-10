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
void* UpdateControllers_ptr = (void*)0x0040F460; // Ditto

PointerInfo jumps[] = {
	{ RumbleLarge_ptr, xinput::RumbleLarge },
	{ RumbleSmall_ptr, xinput::RumbleSmall }
};

std::string BuildConfigPath(const char* modpath)
{
	std::stringstream result;
	char workingdir[FILENAME_MAX];

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\xinput.ini";

	return result.str();
}

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

extern "C"
{
	__declspec(dllexport) void Init(const char* path, const HelperFunctions& helperFunctions)
	{
		int init;
		if ((init = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC)) != 0)
		{
			PrintDebug("Unable to initialize SDL. Error code: %i\n", init);
			MessageBoxA(nullptr, "Error initializing SDL. See debug message for details.", "SDL Init Error", 0);
		}

		// TODO: Hot plugging!
		for (int i = 0; i < SDL_NumJoysticks(); i++)
			DreamPad::Controllers[i].Open(i);

		std::string config = BuildConfigPath(path);

		if (FileExists(config))
		{
			for (ushort i = 0; i < PAD_COUNT; i++)
			{
				std::string section = "Controller " + std::to_string(i + 1);
				const char* section_cstr = section.c_str();
				const char* config_cstr = config.c_str();

				int deadzoneL = GetPrivateProfileIntA(section_cstr, "DeadzoneL", XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, config_cstr);
				int deadzoneR = GetPrivateProfileIntA(section_cstr, "DeadzoneR", XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, config_cstr);

				bool radialL = GetPrivateProfileIntA(section_cstr, "RadialL", 1, config_cstr) != 0;
				bool radialR = GetPrivateProfileIntA(section_cstr, "RadialR", 0, config_cstr) != 0;

				int triggerThreshold = GetPrivateProfileIntA(section_cstr, "TriggerThreshold", XINPUT_GAMEPAD_TRIGGER_THRESHOLD, config_cstr);

				// TODO: Not this
				char wtf[255];

				GetPrivateProfileStringA(section_cstr, "RumbleFactor", "1.0", wtf, 255, config_cstr);
				float rumbleFactor = (float)atof(wtf);

				GetPrivateProfileStringA(section_cstr, "ScaleFactor", "1.5", wtf, 255, config_cstr);
				float scaleFactor = (float)atof(wtf);

				DreamPad::Settings* settings = &DreamPad::Controllers[i].settings;
				settings->apply(deadzoneL, deadzoneR, radialL, radialR, triggerThreshold, rumbleFactor, scaleFactor);

				PrintDebug("[XInput] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
					settings->deadzoneL, settings->deadzoneR, settings->triggerThreshold);
			}
		}

		PrintDebug("[XInput] Initialization complete.\n");
	}

	__declspec(dllexport) void OnInput()
	{
		xinput::UpdateControllersXInput();
	}

	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlength(jumps) } };
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}
