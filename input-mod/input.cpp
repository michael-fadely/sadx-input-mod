#include "stdafx.h"
// Other crap
#include <SADXModLoader.h>
#include <limits.h>
#include "minmax.h"

// This namespace
#include "input.h"
#include "DreamPad.h"

VoidFunc(WriteAnalogs, 0x0040F170);

struct AnalogThing
{
	int		angle;
	float	magnitude;
};

DataArray(AnalogThing, NormalizedAnalogs, 0x03B0E7A0, 8);

namespace input
{
	ControllerData RawInput[GAMEPAD_COUNT];
	bool _ControllerEnabled[GAMEPAD_COUNT];
	bool debug = false;

	// TODO: Keyboard & Mouse. Now I have no excuse.
	void PollControllers()
	{
		DreamPad::ProcessEvents();

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			// Since RawInput replaces ControllersRaw in ControllerPointers,
			// we copy the data from ControllersRaw first in the event that the controller
			// isn't connected and the keyboard is to be used.
			if (i == 0)
				RawInput[i] = ControllersRaw[i];

			DreamPad* dpad = &DreamPad::Controllers[i];

			// HACK: This enables use of the keyboard and mouse if no controllers are connected.
			if (!dpad->Connected())
				continue;

			dpad->Poll();
			dpad->Copy(RawInput[i]);

			// Compatibility for mods who use ControllersRaw directly.
			// This will only copy the first four controllers.
			if (i < ControllersRaw_Length)
				ControllersRaw[i] = RawInput[i];

#ifdef EXTENDED_BUTTONS
			if (debug && RawInput[i].HeldButtons & Buttons_C)
			{
				ControllerData* pad = &RawInput[i];
				Motor m = DreamPad::Controllers[i].GetActiveMotor();

				DisplayDebugStringFormatted(NJM_LOCATION(0, 8 + (3 * i)), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Large), (m & Motor::Small) >> 1);
				DisplayDebugStringFormatted(NJM_LOCATION(4, 9 + (3 * i)), "LS: % 4d/% 4d RS: % 4d/% 4d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (pad->HeldButtons & Buttons_Z)
				{
					if (pad->PressedButtons & Buttons_Up)
						dpad->settings.rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						dpad->settings.rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(NJM_LOCATION(4, 10 + (3 * i)), "Rumble factor (U/D): %f", dpad->settings.rumbleFactor);
				}
			}
#endif
		}
	}

	void FixAnalogs()
	{
		if (!ControlEnabled)
			return;

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			if (!_ControllerEnabled[i])
				continue;

			const DreamPad& dreamPad = DreamPad::Controllers[i];

			if (dreamPad.Connected())
			{
				const ControllerData& pad = dreamPad.DreamcastData();
				// SADX's internal deadzone is 12 of 127. It doesn't set the relative forward direction
				// unless this is exceeded in WriteAnalogs(), so the analog shouldn't be set otherwise.
				if (abs(pad.LeftStickX) > 12 || abs(pad.LeftStickY) > 12)
					NormalizedAnalogs[i].magnitude = dreamPad.NormalizedL();
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

	void RedirectRawControllers()
	{
		for (uint i = 0; i < GAMEPAD_COUNT; i++)
			ControllerPointers[i] = &RawInput[i];
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
				EnableController_hook(i);
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
				DisableController_hook(i);
		}
		else
		{
			_ControllerEnabled[index] = false;
		}
	}
}
