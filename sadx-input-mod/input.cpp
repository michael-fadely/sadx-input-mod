#include "stdafx.h"

#include <SADXModLoader.h>
#include "minmax.h"

#include "input.h"
#include "rumble.h"
#include "DreamPad.h"

// TODO: controller half press modifier

struct AnalogThing
{
	Angle angle;
	float magnitude;
};

DataArray(AnalogThing, NormalizedAnalogs, 0x03B0E7A0, 8);

namespace input
{
	ControllerData RawInput[GAMEPAD_COUNT];
	bool _ControllerEnabled[GAMEPAD_COUNT];
	bool debug = false;

	void PollControllers()
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
					for (uint i = 0; i < GAMEPAD_COUNT; i++)
					{
						// Checking for both in cases like the DualShock 4 and DS4Windows where the controller might be
						// "connected" twice with the same ID. DreamPad::Open automatically closes if already open.
						if (!DreamPad::Controllers[i].Connected() || DreamPad::Controllers[i].ControllerID() == which)
						{
							DreamPad::Controllers[i].Open(which);
							break;
						}
					}
					break;
				}

				case SDL_CONTROLLERDEVICEREMOVED:
				{
					int which = event.cdevice.which;
					for (uint i = 0; i < GAMEPAD_COUNT; i++)
					{
						if (DreamPad::Controllers[i].ControllerID() == which)
						{
							DreamPad::Controllers[i].Close();
							break;
						}
					}
					break;
				}
			}
		}

		KeyboardMouse::Poll();

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad& dreampad = DreamPad::Controllers[i];

			dreampad.Poll();
			dreampad.Copy(RawInput[i]);

			// Compatibility for mods who use ControllersRaw directly.
			// This will only copy the first four controllers.
			if (i < ControllersRaw_Length)
			{
				ControllersRaw[i] = RawInput[i];
			}

#ifdef EXTENDED_BUTTONS
			if (debug && RawInput[i].HeldButtons & Buttons_C)
			{
				ControllerData* pad = &RawInput[i];
				Motor m = DreamPad::Controllers[i].GetActiveMotor();

				DisplayDebugStringFormatted(NJM_LOCATION(0, 8 + (3 * i)), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Large), (m & Motor::Small) >> 1);
				DisplayDebugStringFormatted(NJM_LOCATION(4, 9 + (3 * i)), "LS: %4d/%4d (%f) RS: %4d/%4d (%f)",
					pad->LeftStickX, pad->LeftStickY, dreampad.NormalizedL(), pad->RightStickX, pad->RightStickY, dreampad.NormalizedR());

				if (pad->HeldButtons & Buttons_Z)
				{
					int pressed = pad->PressedButtons;
					if (pressed & Buttons_Up)
					{
						dreampad.settings.rumbleFactor += 0.125f;
					}
					else if (pressed & Buttons_Down)
					{
						dreampad.settings.rumbleFactor -= 0.125f;
					}
					else if (pressed & Buttons_Left)
					{
						rumble::RumbleA(i, 0);
					}
					else if (pressed & Buttons_Right)
					{
						rumble::RumbleB(i, 7, 59, 6);
					}

					DisplayDebugStringFormatted(NJM_LOCATION(4, 10 + (3 * i)),
						"Rumble factor (U/D): %f (L/R to test)", dreampad.settings.rumbleFactor);
				}
			}
#endif
		}
	}

	static void FixAnalogs()
	{
		if (!ControlEnabled)
		{
			return;
		}

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			if (!_ControllerEnabled[i])
			{
				continue;
			}

			const DreamPad& dreamPad = DreamPad::Controllers[i];

			if (dreamPad.Connected())
			{
				const ControllerData& pad = dreamPad.DreamcastData();
				// SADX's internal deadzone is 12 of 127. It doesn't set the relative forward direction
				// unless this is exceeded in WriteAnalogs(), so the analog shouldn't be set otherwise.
				if (abs(pad.LeftStickX) > 12 || abs(pad.LeftStickY) > 12)
				{
					NormalizedAnalogs[i].magnitude = dreamPad.NormalizedL();
				}
			}
		}
	}

	void __declspec(naked) WriteAnalogs_Hook()
	{
		__asm
		{
			call FixAnalogs
			ret
		}
	}

	static void RedirectRawControllers()
	{
		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			ControllerPointers[i] = &RawInput[i];
		}
	}

	void __declspec(naked) RedirectRawControllers_Hook()
	{
		__asm
		{
			call RedirectRawControllers
			ret
		}
	}

	void EnableController_hook(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			_ControllerEnabled[0] = true;
			_ControllerEnabled[1] = true;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < min(index, (Uint8)GAMEPAD_COUNT); i++)
			{
				EnableController_hook(i);
			}
		}
		else
		{
			_ControllerEnabled[index] = true;
		}
	}

	void DisableController_hook(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			_ControllerEnabled[0] = false;
			_ControllerEnabled[1] = false;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < min(index, (Uint8)GAMEPAD_COUNT); i++)
			{
				DisableController_hook(i);
			}
		}
		else
		{
			_ControllerEnabled[index] = false;
		}
	}
}
