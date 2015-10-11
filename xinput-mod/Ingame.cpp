#include "stdafx.h"
// Other crap
#include "SDL.h"
#include <SADXModLoader.h>
#include <limits>
#include "minmax.h"

// This namespace
#include "Ingame.h"
#include "DreamPad.h"

DataPointer(int, isCutscenePlaying, 0x3B2A2E4);		// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);		// Not sure why this is a char and ^ is an int.
DataArray(bool, Controller_Enabled, 0x00909FB4, 4);	// TODO: Figure out what toggles this for P2.

namespace xinput
{
#pragma region Ingame Functions

	// TODO: Keyboard & Mouse. Now I have no excuse.
	void __cdecl UpdateControllersXInput()
	{
		for (ushort i = 0; i < GAMEPAD_COUNT; i++)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					default:
						break;

					case SDL_CONTROLLERDEVICEADDED:
					{
						int which = event.cdevice.which;
						for (uint j = 0; j < GAMEPAD_COUNT; j++)
						{
							// Checking for both in cases like the DualShock 4 and DS4Windows where the controller might be "connected"
							// twice with the same ID. DreamPad::Open automatically closes if already open.
							if (!DreamPad::Controllers[j].Connected() || DreamPad::Controllers[j].ControllerID() == which)
							{
								DreamPad::Controllers[j].Open(which);
								break;
							}
						}
						break;
					}

					case SDL_CONTROLLERDEVICEREMOVED:
					{
						int which = event.cdevice.which;
						for (uint j = 0; j < GAMEPAD_COUNT; j++)
						{
							if (DreamPad::Controllers[j].ControllerID() == which)
							{
								DreamPad::Controllers[j].Close();
								break;
							}
						}
						break;
					}
				}
			}

			ControllerData* pad = &ControllersRaw[i];
			DreamPad* dpad = &DreamPad::Controllers[i];
			dpad->Update();
			dpad->Copy(ControllersRaw[i]);

#ifdef _DEBUG
			if (pad->HeldButtons & Buttons_C)
			{
				Motor m = DreamPad::Controllers[i].GetActiveMotor();

				DisplayDebugStringFormatted(8 + (3 * i), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Large), (m & Motor::Small) >> 1);
				DisplayDebugStringFormatted(9 + (3 * i), "   LS: % 4d/% 4d RS: % 4d/% 4d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (pad->HeldButtons & Buttons_Z)
				{
					if (pad->PressedButtons & Buttons_Up)
						dpad->settings.rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						dpad->settings.rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(10 + (3 * i), "    Rumble factor (U/D): %f", dpad->settings.rumbleFactor);
				}
			}
#endif
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
