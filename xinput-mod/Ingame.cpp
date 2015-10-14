#include "stdafx.h"
// Other crap
#include <SADXModLoader.h>
#include <limits.h>
#include "minmax.h"

// This namespace
#include "Ingame.h"
#include "DreamPad.h"

DataPointer(int, isCutscenePlaying, 0x3B2A2E4);		// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);		// Not sure why this is a char and ^ is an int.
DataArray(bool, Controller_Enabled, 0x00909FB4, 4);	// TODO: Figure out what toggles this for P2.

VoidFunc(WriteAnalogs, 0x0040F170);
DataArray(float, NormalizedAnalogs, 0x03B0E7A4, 0);
DataPointer(char, ControlEnabled, 0x00909FB0);

#define OFFSET(x, y) (x << 16 | y)

namespace input
{
#pragma region Ingame Functions

	// TODO: Keyboard & Mouse. Now I have no excuse.
	void PollControllers()
	{
		DreamPad::ProcessEvents();

		for (ushort i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad* dpad = &DreamPad::Controllers[i];
			// HACK This enables use of the keyboard and mouse if no controllers are connected.
			if (!dpad->Connected())
				continue;

			dpad->Poll();
			dpad->Copy(ControllersRaw[i]);

#if defined(_DEBUG) && defined(EXTENDED_BUTTONS)
			ControllerData* pad = &ControllersRaw[i];
			if (pad->HeldButtons & Buttons_C)
			{
				Motor m = DreamPad::Controllers[i].GetActiveMotor();

				DisplayDebugStringFormatted(OFFSET(0, 8 + (3 * i)), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Large), (m & Motor::Small) >> 1);
				DisplayDebugStringFormatted(OFFSET(4, 9 + (3 * i)), "LS: % 4d/% 4d RS: % 4d/% 4d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (pad->HeldButtons & Buttons_Z)
				{
					if (pad->PressedButtons & Buttons_Up)
						dpad->settings.rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						dpad->settings.rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(OFFSET(4, 10 + (3 * i)), "Rumble factor (U/D): %f", dpad->settings.rumbleFactor);
				}
			}
#endif
		}
	}

	void WriteAnalogsWrapper()
	{
		WriteAnalogs();

		for (uint i = 0; i < 8; i++)
		{
			if (!ControlEnabled || !Controller_Enabled[i])
				continue;

			if (i < GAMEPAD_COUNT && DreamPad::Controllers[i].Connected())
			{
				// SADX's internal deadzone is 12 of 127. It doesn't set the relative forward direction
				// unless this is exceeded in WriteAnalogs(), so the analog shouldn't be set otherwise.
				if (abs(ControllersRaw[i].LeftStickX) > 12 || abs(ControllersRaw[i].LeftStickY) > 12)
					NormalizedAnalogs[2 * i] = DreamPad::Controllers[i].NormalizedL();
			}
		}
	}

	void Rumble(ushort id, int magnitude, Motor motor)
	{
		if (id >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
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
			DreamPad::Controllers[id].SetActiveMotor(motor, scaled);
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
