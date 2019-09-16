#include "stdafx.h"
#include <Windows.h>
#include <direct.h>	// for _getcwd

#include <string>
#include <sstream>	// because

#include "SDL.h"

#include <SADXModLoader.h>
#include <IniFile.hpp>

#include "typedefs.h"
#include "FileExists.h"
#include "input.h"
#include "rumble.h"
#include "KeyboardVariables.h"

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
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 };
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
		// Disable call to CreateMouseDevice
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

		const std::string config_path = build_mod_path(path, "config.ini");

	#ifdef _DEBUG
		const bool debug_default = true;
	#else
		const bool debug_default = false;
	#endif

		IniFile config(config_path);

		input::debug = config.getBool("Config", "Debug", debug_default);

		// This defaults RadialR to enabled if smooth-cam is detected.
		const bool smooth_cam = GetModuleHandleA("smooth-cam.dll") != nullptr;

		for (ushort i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad::Settings& settings = DreamPad::controllers[i].settings;

			const std::string section = "Controller " + std::to_string(i + 1);

			const int deadzone_l = config.getInt(section, "DeadzoneL", GAMEPAD_LEFT_THUMB_DEADZONE);
			const int deadzone_r = config.getInt(section, "DeadzoneR", GAMEPAD_RIGHT_THUMB_DEADZONE);

			settings.set_deadzone_l(deadzone_l);
			settings.set_deadzone_r(deadzone_r);

			settings.radial_l = config.getBool(section, "RadialL", true);
			settings.radial_r = config.getBool(section, "RadialR", smooth_cam);

			settings.trigger_threshold = config.getInt(section, "TriggerThreshold", GAMEPAD_TRIGGER_THRESHOLD);

			settings.rumble_factor = clamp(config.getFloat(section, "RumbleFactor", 1.0f), 0.0f, 1.0f);

			settings.mega_rumble = config.getBool(section, "MegaRumble", false);
			settings.rumble_min_time = static_cast<ushort>(config.getInt(section, "RumbleMinTime", 0));

			settings.allow_keyboard = config.getBool(section, "AllowKeyboard", !i);

			if (input::debug)
			{
				PrintDebug("[Input] Deadzones for P%d (L/R/T): %05d / %05d / %05d\n", (i + 1),
				           settings.deadzone_l, settings.deadzone_r, settings.trigger_threshold);
			}
		}

		// Load keyboard settings.
		WriteCall((void*)0x437547, GetEKey); // Return true when the correct key is held.
		// Layout 1
		KButton_A = FindKey(config.getString("Layout 1", "Button A", "X"));
		KButton_B = FindKey(config.getString("Layout 1", "Button B", "Z"));
		KButton_X = FindKey(config.getString("Layout 1", "Button X", "A"));
		KButton_Y = FindKey(config.getString("Layout 1", "Button Y", "S"));
		KButton_Z = FindKey(config.getString("Layout 1", "Button Z", "None"));
		KButton_C = FindKey(config.getString("Layout 1", "Button C", "None"));
		KButton_D = FindKey(config.getString("Layout 1", "Button D", "None"));
		KButton_Start = FindKey(config.getString("Layout 1", "Button Start", "Enter"));
		KButton_L = FindKey(config.getString("Layout 1", "Trigger L", "Q"));
		KButton_R = FindKey(config.getString("Layout 1", "Trigger R", "W"));
		KButton_Up = FindKey(config.getString("Layout 1", "Analog Up", "Up"));
		KButton_Down = FindKey(config.getString("Layout 1", "Analog Down", "Down"));
		KButton_Left = FindKey(config.getString("Layout 1", "Analog Left", "Left"));
		KButton_Right = FindKey(config.getString("Layout 1", "Analog Right", "Right"));
		KButton_DPadUp = FindKey(config.getString("Layout 1", "D-Pad Up", "I"));
		KButton_DPadDown = FindKey(config.getString("Layout 1", "D-Pad Down", "M"));
		KButton_DPadLeft = FindKey(config.getString("Layout 1", "D-Pad Left", "J"));
		KButton_DPadRight = FindKey(config.getString("Layout 1", "D-Pad Right", "K"));
		KButton_Center = FindKey(config.getString("Layout 1", "Center Camera", "E"));
		// Layout 2
		KButton2_A = FindKey(config.getString("Layout 2", "Button A", "Space"));
		KButton2_B = FindKey(config.getString("Layout 2", "Button B", "Escape"));
		KButton2_X = FindKey(config.getString("Layout 2", "Button X", "None"));
		KButton2_Y = FindKey(config.getString("Layout 2", "Button Y", "None"));
		KButton2_Z = FindKey(config.getString("Layout 2", "Button Z", "None"));
		KButton2_C = FindKey(config.getString("Layout 2", "Button C", "None"));
		KButton2_D = FindKey(config.getString("Layout 2", "Button D", "None"));
		KButton2_Start = FindKey(config.getString("Layout 2", "Button Start", "Home"));
		KButton2_L = FindKey(config.getString("Layout 2", "Trigger L", "None"));
		KButton2_R = FindKey(config.getString("Layout 2", "Trigger R", "None"));
		KButton2_Up = FindKey(config.getString("Layout 2", "Analog Up", "R"));
		KButton2_Down = FindKey(config.getString("Layout 2", "Analog Down", "C"));
		KButton2_Left = FindKey(config.getString("Layout 2", "Analog Left", "D"));
		KButton2_Right = FindKey(config.getString("Layout 2", "Analog Right", "F"));
		KButton2_DPadUp = FindKey(config.getString("Layout 2", "D-Pad Up", "None"));
		KButton2_DPadDown = FindKey(config.getString("Layout 2", "D-Pad Down", "None"));
		KButton2_DPadLeft = FindKey(config.getString("Layout 2", "D-Pad Left", "None"));
		KButton2_DPadRight = FindKey(config.getString("Layout 2", "D-Pad Right", "None"));
		KButton2_Center = FindKey(config.getString("Layout 2", "Center Camera", "None"));
		// Layout 3
		KButton3_A = FindKey(config.getString("Layout 3", "Button A", "None"));
		KButton3_B = FindKey(config.getString("Layout 3", "Button B", "V"));
		KButton3_X = FindKey(config.getString("Layout 3", "Button X", "None"));
		KButton3_Y = FindKey(config.getString("Layout 3", "Button Y", "None"));
		KButton3_Z = FindKey(config.getString("Layout 3", "Button Z", "None"));
		KButton3_C = FindKey(config.getString("Layout 3", "Button C", "None"));
		KButton3_D = FindKey(config.getString("Layout 3", "Button D", "None"));
		KButton3_Start = FindKey(config.getString("Layout 3", "Button Start", "None"));
		KButton3_L = FindKey(config.getString("Layout 3", "Trigger L", "None"));
		KButton3_R = FindKey(config.getString("Layout 3", "Trigger R", "None"));
		KButton3_Up = FindKey(config.getString("Layout 3", "Analog Up", "Numpad 8"));
		KButton3_Down = FindKey(config.getString("Layout 3", "Analog Down", "Numpad 2"));
		KButton3_Left = FindKey(config.getString("Layout 3", "Analog Left", "Numpad 4"));
		KButton3_Right = FindKey(config.getString("Layout 3", "Analog Right", "Numpad 6"));
		KButton3_DPadUp = FindKey(config.getString("Layout 3", "D-Pad Up", "None"));
		KButton3_DPadDown = FindKey(config.getString("Layout 3", "D-Pad Down", "None"));
		KButton3_DPadLeft = FindKey(config.getString("Layout 3", "D-Pad Left", "None"));
		KButton3_DPadRight = FindKey(config.getString("Layout 3", "D-Pad Right", "None"));
		KButton3_Center = FindKey(config.getString("Layout 3", "Center Camera", "None"));

		PrintDebug("[Input] Initialization complete.\n");
	}

	__declspec(dllexport) void OnFrame()
	{
		ClearVanillaKeys();
	}

	__declspec(dllexport) void OnExit()
	{
		for (auto& i : DreamPad::controllers)
		{
			i.close();
		}
	}
}
