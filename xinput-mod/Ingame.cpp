// Other crap
#include "SDL.h"
#include <SADXModLoader.h>
#include "Common.h"

// This namespace
#include "Ingame.h"
#include "Convert.h"
#include "DreamPad.h"

DataPointer(int, isCutscenePlaying, 0x3B2A2E4);		// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);		// Not sure why this is a char and ^ is an int.
DataArray(bool, Controller_Enabled, 0x00909FB4, 4);	// TODO: Figure out what toggles this for P2.

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

DreamPad controllers[PAD_COUNT];

namespace xinput
{
	Settings::Settings()
	{
		deadzoneL			= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		deadzoneR			= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		triggerThreshold	= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		radialL				= true;
		radialR				= false;
		rumbleFactor		= 1.0f;
		scaleFactor			= 1.5f;
	}
	void Settings::apply(short deadzoneL, short deadzoneR, bool radialL, bool radialR, uint8 triggerThreshold, float rumbleFactor, float scaleFactor)
	{
		this->deadzoneL			= clamp(deadzoneL, (short)0, (short)SHRT_MAX);
		this->deadzoneR			= clamp(deadzoneR, (short)0, (short)SHRT_MAX);
		this->radialL			= radialL;
		this->radialR			= radialR;
		this->triggerThreshold	= min((uint8)UCHAR_MAX, triggerThreshold);
		this->rumbleFactor		= rumbleFactor;
		this->scaleFactor		= max(1.0f, scaleFactor);
	}

	Settings settings[PAD_COUNT];

	static inline int DigitalTrigger(uint8 trigger, uint8 threshold, int button)
	{
		return (trigger > threshold) ? button : 0;
	}

#pragma region Ingame Functions

	// TODO: Keyboard & Mouse
	void __cdecl UpdateControllersXInput()
	{
		for (ushort i = 0; i < PAD_COUNT; i++)
		{
			ControllerData* pad = &ControllersRaw[i];
			DreamPad* dpad = &controllers[i];
			dpad->Update();
			dpad->Copy(ControllersRaw[i]);

#ifdef _DEBUG
			if (pad->HeldButtons & Buttons_C)
			{
				Motor m = controllers[i].GetActiveMotor();

				DisplayDebugStringFormatted(8 + (3 * i), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Large), (m & Motor::Small) >> 1);
				DisplayDebugStringFormatted(9 + (3 * i), "   LS: % 4d/% 4d RS: % 4d/% 4d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (i == 0 && pad->HeldButtons & Buttons_Z)
				{
					if (pad->PressedButtons & Buttons_Up)
						settings[i].rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						settings[i].rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(6, "Rumble factor (U/D): %f", settings[i].rumbleFactor);
				}
			}
#endif
		}
	}

	void Rumble(ushort id, int magnitude, Motor motor)
	{
		if (id >= PAD_COUNT)
		{
			for (ushort i = 0; i < PAD_COUNT; i++)
				Rumble(id, magnitude, motor);

			return;
		}

		short scaled = 4 * magnitude;

		if (magnitude > 0)
		{
			float m;

			// RumbleLarge only ever passes in a value in that is <= 10,
			// and scaling that to 2 bytes is super annoying, so here's
			// some arbitrary values to increase the intensity.
			if (magnitude >= 1 && magnitude <= 10)
				m = max(0.2375f, scaled / 25.0f);
			else
				m = (float)scaled / 256.0f;

			scaled = (short)(SHRT_MAX * clamp(m, 0.0f, 1.0f));
		}

		if (Controller_Enabled[id] || scaled == 0)
			controllers[id].SetActiveMotor(motor, scaled);
	}
	void __cdecl RumbleLarge(int playerNumber, int magnitude)
	{
		if (!isCutscenePlaying && rumbleEnabled)
			Rumble(playerNumber, clamp(magnitude, 1, 255), Motor::Large);
	}
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4)
	{
		if (!isCutscenePlaying && rumbleEnabled)
		{
			int _a2 = clamp(a2, -4, 4);

			if (_a2 == 1)
				_a2 = 2;
			else if (_a2 == -1)
				_a2 = -2;

			int _a3 = clamp(a3, 7, 59);

			Rumble(playerNumber, max(1, a4 * _a3 / (4 * _a2)), Motor::Small);
		}
	}

#pragma endregion

}
