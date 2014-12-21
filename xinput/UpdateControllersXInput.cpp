#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>

#include <limits>

#include <SADXModLoader.h>
#include <G:\Libraries\LazyTypedefs.h>

#include "UpdateControllersXInput.h"

DataArray(ControllerData, Controller_Data_0, 0x03B0E9C8, 8);
DataPointer(int, rumble_related_3B2A2E4, 0x3B2A2E4);
DataPointer(char, enableRumble, 0x00913B10);

namespace xinput
{
	short GetWithinDeadzone(short analog, short deadzone);
	int XInputToDreamcast(XINPUT_GAMEPAD* xpad, ushort id);

	const uint64 rumble_l_timer = 250;
	const uint64 rumble_r_timer = 1000;

	XINPUT_VIBRATION vibration = {};
	Motor rumble = Motor::None;
	uint64 rumble_l_elapsed = 0;
	uint64 rumble_r_elapsed = 0;

	bool multi_gate = false;
	float rumble_multi = 255.0;

	namespace deadzone
	{
		short stickL[4] = {
			XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
		};

		short stickR[4] = {
			XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
			XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
		};

		short triggers[4] = {
			XINPUT_GAMEPAD_TRIGGER_THRESHOLD,
			XINPUT_GAMEPAD_TRIGGER_THRESHOLD,
			XINPUT_GAMEPAD_TRIGGER_THRESHOLD,
			XINPUT_GAMEPAD_TRIGGER_THRESHOLD
		};
	}

	// TODO: Keyboard & Mouse
	void __cdecl UpdateControllersXInput()
	{
		for (uint8 i = 0; i < 4; i++)
		{
			ControllerData* pad = &Controller_Data_0[i];
			XINPUT_STATE state = {};
			XInputGetState(i, &state);
			XINPUT_GAMEPAD* xpad = &state.Gamepad;

#ifdef _DEBUG
			DisplayDebugStringFormatted(10 + i, "P%d L X/Y: %04d/%04d - R X/Y: %04d/%04d", (i + 1), pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);
			
			if (i == 0 && xpad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
			{
				if (xpad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
				{
					if (multi_gate == false)
						rumble_multi += 8.0;
					multi_gate = true;
				}
				else if (xpad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
				{
					if (multi_gate == false)
						rumble_multi -= 8.0;
					multi_gate = true;
				}
				else
				{
					multi_gate = false;
				}
				
				DisplayDebugStringFormatted(15, "Rumble multiplier: %f", rumble_multi);
			}
#endif
			// Gotta get that enum set up for this.
			pad->Support = 0x3F07FEu;

			// L Analog
			pad->LeftStickX = GetWithinDeadzone(xpad->sThumbLX, deadzone::stickL[i]);
			pad->LeftStickY = GetWithinDeadzone(-xpad->sThumbLY, deadzone::stickL[i]);

			// R Analog
			pad->RightStickX = GetWithinDeadzone(xpad->sThumbRX, deadzone::stickR[i]);
			pad->RightStickY = GetWithinDeadzone(-xpad->sThumbRY, deadzone::stickR[i]);

			// Trigger pressure
			pad->LTriggerPressure = xpad->bLeftTrigger;
			pad->RTriggerPressure = xpad->bRightTrigger;

			// Now, get the new buttons from the XInput xpad
			pad->HeldButtons = XInputToDreamcast(xpad, i);
			pad->NotHeldButtons = ~pad->HeldButtons;

			// Now set the released buttons to the difference between
			// the last and currently held buttons
			pad->ReleasedButtons = pad->HeldButtons ^ pad->Old;

			// Do some fancy math to "press" only the necessary buttons
			pad->PressedButtons = pad->HeldButtons;
			pad->PressedButtons &= ~pad->Old;

			// Set the "last held" to held
			pad->Old = pad->HeldButtons;
		}

		// Disable rumble if the timer says it's a good idea.
		if (rumble != Motor::None)
		{
			Motor result = Motor::None;
			uint64 now = GetTickCount64();

			if ((now - rumble_l_elapsed) >= rumble_l_timer)
			{
				result = (Motor)(result | Motor::Left);
				rumble = (Motor)(rumble ^ Motor::Left);
			}
			if ((now - rumble_r_elapsed) >= rumble_r_timer)
			{
				result = (Motor)(result | Motor::Right);
				rumble = (Motor)(rumble ^ Motor::Right);
			}

			if (result != Motor::None)
				Rumble(0, result);
		}
	}

	void Rumble(int a1, Motor motor)
	{
		short intensity = 4 * a1;
		
		if (a1 > 0)
		{
			float m = 0;
			
			// RumbleLarge only ever passes in a value in that is <= 10,
			// and scaling that to 2 bytes is super annoying, so here's
			// some arbitrary values to increase the intensity.
			if (a1 >= 1 && a1 <= 10)
				m = max(0.2375f, intensity / 25.0f);
			else
				m = intensity / rumble_multi;

			intensity = (short)(SHRT_MAX * min(1.0f, m));

#ifdef _DEBUG
			PrintDebug("Multiplier/Intensity: %f | %d\n", m, intensity);
#endif

			rumble = (Motor)(rumble | motor);
		}

		if (motor & Motor::Left)
		{
			vibration.wLeftMotorSpeed = intensity;
			rumble_l_elapsed = GetTickCount64();
		}
		if (motor & Motor::Right)
		{
			vibration.wRightMotorSpeed = intensity * 2;
			rumble_r_elapsed = GetTickCount64();
		}

		for (uint8 i = 0; i < 4; i++)
			XInputSetState(i, &vibration);
	}
	void __cdecl RumbleLarge(int playerNumber, signed int intensity)
	{
		int _intensity; // eax@4

		if (!rumble_related_3B2A2E4)
		{
			if (enableRumble)
			{
				// Only continue if the calling player(?) is Player 1 (0)
				if (!playerNumber)
				{
					_intensity = intensity;
					if (intensity <= 255)
					{
						// If the intensity is <= 0, set to the default of 1
						if (intensity < 0 || intensity < 1)
							_intensity = 1;
						Rumble(_intensity, Motor::Left);
					}
					else
					{
						Rumble(255, Motor::Left);
					}
				}
			}
		}
	}
	void __cdecl RumbleSmall(int a1, signed int a2, signed int a3, int a4)
	{
		signed int _a2; // ecx@4
		signed int _a3; // eax@12
		signed int intensity; // eax@16

		if (!rumble_related_3B2A2E4)
		{
			if (enableRumble)
			{
				if (!a1)
				{
					_a2 = a2;
					if (a2 <= 4)
					{
						if (a2 >= -4)
						{
							if (a2 == 1)
							{
								_a2 = 2;
							}
							else
							{
								if (a2 == -1)
									_a2 = -2;
							}
						}
						else
						{
							_a2 = -4;
						}
					}
					else
					{
						_a2 = 4;
					}
					_a3 = a3;
					if (a3 <= 59)
					{
						if (a3 < 7)
							_a3 = 7;
					}
					else
					{
						_a3 = 59;
					}
					intensity = a4 * _a3 / (4 * _a2);
					if (intensity <= 0)
						intensity = 1;
					Rumble(intensity, Motor::Right);
				}
			}
		}
	}

	// If analog exceeds deadzone, return SADX-friendly
	// version of the value; else 0.
	short GetWithinDeadzone(short analog, short deadzone)
	{
		// tl;dr: if analog exceeds the deadzone, convert to SADX-friendly
		// value, then make sure it's not < -127 or > 127 and return it; else 0.
		if (analog < -deadzone || analog > deadzone)
			return min(max(-127, (analog / 256)), 127);
		else
			return 0;
	}
	// Converts wButtons in XINPUT_GAMEPAD to Sonic Adventure compatible buttons and returns the value.
	int XInputToDreamcast(XINPUT_GAMEPAD* xpad, ushort id)
	{
		int result = 0;
		int buttons = xpad->wButtons;

		if (buttons & XINPUT_GAMEPAD_A)
			result |= Buttons_A;
		if (buttons & XINPUT_GAMEPAD_B)
			result |= Buttons_B;
		if (buttons & XINPUT_GAMEPAD_X)
			result |= Buttons_X;
		if (buttons & XINPUT_GAMEPAD_Y)
			result |= Buttons_Y;
		if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
			result |= Buttons_Z;

		if (xpad->bLeftTrigger > deadzone::triggers[id])
			result |= Buttons_L;
		if (xpad->bRightTrigger > deadzone::triggers[id])
			result |= Buttons_R;

		if (buttons & XINPUT_GAMEPAD_START)
			result |= Buttons_Start;

		if (buttons & XINPUT_GAMEPAD_DPAD_UP)
			result |= Buttons_Up;
		if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
			result |= Buttons_Down;
		if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
			result |= Buttons_Left;
		if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
			result |= Buttons_Right;

		return result;
	}
}