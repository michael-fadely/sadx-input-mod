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

	void Rumble_Load(Uint32 port, Uint32 time, Motor motor)
	{
		// TODO: Take advantage of the built in rumble manager object
		if (port >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
				Rumble_Load(i, time, motor);

			return;
		}

		Uint32 multiplied = time * 4;
		if (debug)
			PrintDebug("[%u] Rumble Time: %i * 4 (%i frames(?), %ums)\n", port, time, multiplied, (Uint32)((1000.0 / 60.0) * multiplied));
		DreamPad::Controllers[port].SetActiveMotor(motor, multiplied);
	}
	void __cdecl RumbleA(Uint32 port, Uint32 time)
	{
		Uint32 _time; // eax@4

		if (!CutscenePlaying && RumbleEnabled && _ControllerEnabled[port])
		{
			_time = time;
			if (time <= 255)
			{
				if (time < 0 || time < 1)
				{
					_time = 1;
				}
				Rumble_Load(port, _time, Motor::Large);
			}
			else
			{
				Rumble_Load(port, 0xFFu, Motor::Large);
			}
		}
	}
	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4)
	{
		Uint32 idk; // ecx@4
		int _a3; // eax@12
		int _time; // eax@16

		if (!CutscenePlaying && RumbleEnabled && _ControllerEnabled[port])
		{
			idk = time;
			if ((signed int)time <= 4)
			{
				if ((signed int)time >= -4)
				{
					if (time == 1)
					{
						idk = 2;
					}
					else if (time == -1)
					{
						idk = -2;
					}
				}
				else
				{
					idk = -4;
				}
			}
			else
			{
				idk = 4;
			}
			_a3 = a3;
			if (a3 <= 59)
			{
				if (a3 < 7)
				{
					_a3 = 7;
				}
			}
			else
			{
				_a3 = 59;
			}
			_time = a4 * _a3 / (signed int)(4 * idk);
			if (_time <= 0)
			{
				_time = 1;
			}
			Rumble_Load(port, _time, Motor::Small);
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
}
