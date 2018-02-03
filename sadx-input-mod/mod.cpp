#include "stdafx.h"
#include <Windows.h>
#include <direct.h>	// for _getcwd

#include <string>
#include <sstream>	// because

#include "SDL.h"

#include <SADXModLoader.h>

#include "typedefs.h"
#include "FileExists.h"
#include "input.h"
#include "rumble.h"

static void* RumbleA_ptr            = reinterpret_cast<void*>(0x004BCBC0);
static void* RumbleB_ptr            = reinterpret_cast<void*>(0x004BCC10);
static void* UpdateControllers_ptr  = reinterpret_cast<void*>(0x0040F460);
static void* AnalogHook_ptr         = reinterpret_cast<void*>(0x0040F343);
static void* InitRawControllers_ptr = reinterpret_cast<void*>(0x0040F451); // End of function (hook)

PointerInfo jumps[] = {
	{ rumble::pdVibMxStop, rumble::pdVibMxStop_r },
	{ RumbleA_ptr, rumble::RumbleA_r },
	{ RumbleB_ptr, rumble::RumbleB_r },
	{ AnalogHook_ptr, input::WriteAnalogs_hook },
	{ InitRawControllers_ptr, input::InitRawControllers_hook },
	{ EnableController, input::EnableController_r },
	{ DisableController, input::DisableController_r },
	{ reinterpret_cast<void*>(0x0042D52D), rumble::default_rumble },
	// Used to skip over the standard controller update function.
	// This has no effect on the OnInput hook.
	{ UpdateControllers_ptr, reinterpret_cast<void*>(0x0040FDB3) }
};

static std::string build_mod_path(const char* modpath, const char* path)
{
	std::stringstream result;
	char workingdir[FILENAME_MAX] {};

	result << _getcwd(workingdir, FILENAME_MAX) << "\\" << modpath << "\\" << path;

	return result.str();
}

extern "C"
{
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
	__declspec(dllexport) PointerList Jumps[] = { { arrayptrandlengthT(jumps, int) } };

	__declspec(dllexport) void OnInput()
	{
		input::poll_controllers();
	}

	__declspec(dllexport) void Init(const char* path, const HelperFunctions&)
	{
		std::string dll = build_mod_path(path, "SDL2.dll");

		const auto handle = LoadLibraryA(dll.c_str());

		if (handle == nullptr)
		{
			PrintDebug("[Input] Unable to load SDL2.dll.\n");

			MessageBoxA(nullptr, "Error loading SDL. See debug message for details.",
						"SDL Load Error", MB_OK | MB_ICONERROR);

			return;
		}

		int init;
		if ((init = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_EVENTS)) != 0)
		{
			PrintDebug("[Input] Unable to initialize SDL. Error code: %i\n", init);
			MessageBoxA(nullptr, "Error initializing SDL. See debug message for details.",
						"SDL Init Error", MB_OK | MB_ICONERROR);
			return;
		}

		// Disable call to CreateKeyboardDevice
		WriteData<5>(reinterpret_cast<void*>(0x0077F0D7), 0x90i8);
		// Disable call to CreateMousedevice
		WriteData<5>(reinterpret_cast<void*>(0x0077F03E), 0x90i8);
		// Disable call to DirectInput_Init
		WriteData<5>(reinterpret_cast<void*>(0x0077F205), 0x90i8);

		// EnableControl
		WriteData(reinterpret_cast<bool**>(0x40EF80), &input::controller_enabled[0]);
		WriteData(reinterpret_cast<bool**>(0x40EF86), &input::controller_enabled[1]);
		WriteData(reinterpret_cast<bool**>(0x40EF90), input::controller_enabled);

		// DisableControl
		WriteData(reinterpret_cast<bool**>(0x40EFB0), &input::controller_enabled[0]);
		WriteData(reinterpret_cast<bool**>(0x40EFB6), &input::controller_enabled[1]);
		WriteData(reinterpret_cast<bool**>(0x40EFC0), input::controller_enabled);

		// IsControllerEnabled
		WriteData(reinterpret_cast<bool**>(0x40EFD8), input::controller_enabled);

		// Control
		WriteData(reinterpret_cast<bool**>(0x40FE0D), input::controller_enabled);
		WriteData(reinterpret_cast<bool**>(0x40FE2F), &input::controller_enabled[1]);

		// WriteAnalogs
		WriteData(reinterpret_cast<bool**>(0x40F30C), input::controller_enabled);

		input::controller_enabled[0] = true;
		input::controller_enabled[1] = true;

		std::string dbpath = build_mod_path(path, "gamecontrollerdb.txt");

		if (FileExists(dbpath))
		{
			int result = SDL_GameControllerAddMappingsFromFile(dbpath.c_str());

			if (result == -1)
			{
				PrintDebug("[Input] Error loading gamecontrollerdb: %s\n", SDL_GetError());
			}
			else
			{
				PrintDebug("[Input] Controller mappings loaded: %i\n", result);
			}
		}

		std::string config = build_mod_path(path, "config.ini");

		if (!FileExists(config))
		{
			for (int i = 0; i < GAMEPAD_COUNT; i++)
			{
				DreamPad::controllers[i].settings.allow_keyboard = !i;
			}
		}
		else
		{
		#ifdef _DEBUG
			bool debug_default = true;
		#else
			bool debug_default = false;
		#endif

			const char* config_cstr = config.c_str();
			input::debug = GetPrivateProfileIntA("Config", "Debug", static_cast<int>(debug_default), config_cstr) != 0;

			// This defaults RadialR to enabled if smooth-cam is detected.
			int smooth_cam = GetModuleHandleA("smooth-cam.dll") != nullptr ? 1 : 0;

			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
			{
				DreamPad::Settings& settings = DreamPad::controllers[i].settings;

				std::string section = "Controller " + std::to_string(i + 1);
				const char* section_cstr = section.c_str();

				const int deadzone_l = GetPrivateProfileIntA(section_cstr, "DeadzoneL", GAMEPAD_LEFT_THUMB_DEADZONE, config_cstr);
				const int deadzone_r = GetPrivateProfileIntA(section_cstr, "DeadzoneR", GAMEPAD_RIGHT_THUMB_DEADZONE, config_cstr);

				settings.set_deadzone_l(deadzone_l);
				settings.set_deadzone_r(deadzone_r);

				settings.radial_l = GetPrivateProfileIntA(section_cstr, "RadialL", 1, config_cstr) != 0;
				settings.radial_r = GetPrivateProfileIntA(section_cstr, "RadialR", smooth_cam, config_cstr) != 0;

				settings.trigger_threshold = GetPrivateProfileIntA(section_cstr, "TriggerThreshold", GAMEPAD_TRIGGER_THRESHOLD, config_cstr);

				// honestly how else would I do it
				char float_buffer[256] {};

				GetPrivateProfileStringA(section_cstr, "RumbleFactor", "1.0", float_buffer, LengthOfArray(float_buffer), config_cstr);
				settings.rumble_factor = clamp(static_cast<float>(atof(float_buffer)), 0.0f, 1.0f);

				settings.mega_rumble = GetPrivateProfileIntA(section_cstr, "MegaRumble", 0, config_cstr) != 0;
				settings.rumble_min_time = static_cast<ushort>(GetPrivateProfileIntA(section_cstr, "RumbleMinTime", 0, config_cstr));

				settings.allow_keyboard = GetPrivateProfileIntA(section_cstr, "AllowKeyboard", !i, config_cstr) != 0;

				if (input::debug)
				{
					PrintDebug("[Input] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
							   settings.deadzone_l, settings.deadzone_r, settings.trigger_threshold);
				}
			}
		}

		PrintDebug("[Input] Initialization complete.\n");
	}

	__declspec(dllexport) void OnExit()
	{
		for (auto& i : DreamPad::controllers)
		{
			i.close();
		}
	}
}
