// Microsoft
#include <Windows.h>		// Required for XInput.h
#include <Xinput.h>			// obvious

// Standard
#include <limits>			// for min(), max(), SHRT_MAX

// Other crap
#include <SADXModLoader.h>
#include "typedefs.h"		// includes cstdint as used by _ControllerData

// This namespace
#include "UpdateControllersXInput.h"


// TODO: Figure out how to determine if a player is AI controlled or not, then enable per-controller rumble.

// From the SA2 Mod Loader
// Using this until it or PDS_PERIPHERAL gets implemented into the SADX Mod Loader.
// Aside from having all of its members named, it's otherwise exactly the same.
struct _ControllerData
{
	uint32_t ID;
	uint32_t Support;
	uint32_t HeldButtons;
	uint32_t NotHeldButtons;
	uint32_t PressedButtons;
	uint32_t ReleasedButtons;
	uint16_t RTriggerPressure;
	uint16_t LTriggerPressure;
	int16_t LeftStickX;
	int16_t LeftStickY;
	int16_t RightStickX;
	int16_t RightStickY;
	char* Name;
	void* Extend;
	uint32_t Old;
	void* Info;
};

DataArray(_ControllerData, Controller_Data_0, 0x03B0E9C8, 8);	// Yes, there are in fact *8* controller structures in SADX PC.
DataPointer(int, isCutscenePlaying, 0x3B2A2E4);					// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);					// Not sure why this is a char and ^ is an int.

#define clamp(value, low, high) min(max(low, value), high)

namespace xinput
{
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

	const uint rumble_l_timer = 250;
	const uint rumble_r_timer = 1000;

	XINPUT_VIBRATION vibration[4];
	Motor rumble[4];
	uint rumble_l_elapsed[4];
	uint rumble_r_elapsed[4];

	float rumble_multi = 255.0;

#pragma region Ingame Functions

	// TODO: Keyboard & Mouse
	void __cdecl UpdateControllersXInput()
	{
		for (uint8 i = 0; i < 4; i++)
		{
			_ControllerData* pad = &Controller_Data_0[i];
			XINPUT_STATE state = {};
			XInputGetState(i, &state);
			XINPUT_GAMEPAD* xpad = &state.Gamepad;

			// Gotta get that enum set up for this.
			pad->Support = 0x3F07FEu;

			// L Analog
			ConvertAxes((short*)&pad->LeftStickX, (short*)&xpad->sThumbLX, deadzone::stickL[i], true);

			// R Analog
			ConvertAxes((short*)&pad->RightStickX, (short*)&xpad->sThumbRX, deadzone::stickR[i], false);

			// Trigger pressure
			pad->LTriggerPressure = xpad->bLeftTrigger;
			pad->RTriggerPressure = xpad->bRightTrigger;

			// Now, get the new buttons from the XInput xpad
			pad->HeldButtons = ConvertButtons(xpad, i);
			pad->NotHeldButtons = ~pad->HeldButtons;

			// Now set the released buttons to the difference between
			// the last and currently held buttons
			pad->ReleasedButtons = pad->Old & (pad->HeldButtons ^ pad->Old);

			// Do some fancy math to "press" only the necessary buttons
			pad->PressedButtons = pad->HeldButtons & (pad->HeldButtons ^ pad->Old);

			// Set the "last held" to held
			pad->Old = pad->HeldButtons;

			// Disable rumble if the timer says it's a good idea.
			if (rumble[i] != Motor::None)
			{
				Motor result = Motor::None;
				uint now = GetTickCount();

				if ((now - rumble_l_elapsed[i]) >= rumble_l_timer)
				{
					result = (Motor)(result | Motor::Left);
					rumble[i] = (Motor)(rumble[i] & ~Motor::Left);
				}
				if ((now - rumble_r_elapsed[i]) >= rumble_r_timer)
				{
					result = (Motor)(result | Motor::Right);
					rumble[i] = (Motor)(rumble[i] & ~Motor::Right);
				}

				if (result != Motor::None)
					Rumble(i, 0, result);
			}

#ifdef _DEBUG
			if (xpad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
			{
				DisplayDebugStringFormatted(8 + (3 * i), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (rumble[i] & Motor::Left), ((rumble[i] & Motor::Right) >> 1));
				DisplayDebugStringFormatted(9 + (3 * i), "   LS: %04d/%04d RS: %04d/%04d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (i == 0 && xpad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
				{
					if (pad->PressedButtons & Buttons_Up)
						rumble_multi += 8.0;
					else if (pad->PressedButtons & Buttons_Down)
						rumble_multi -= 8.0;

					DisplayDebugStringFormatted(6, "Rumble multiplier (U/D): %f", rumble_multi);
				}
			}
#endif
		}
	}

	void Rumble(short id, int a1, Motor motor)
	{
		short intensity = 4 * a1;

		bool isWithinRange = (id >= 0 && id < 4);
		Motor resultMotor = (isWithinRange) ? rumble[id] : Motor::None;

		if (a1 > 0)
		{
			float m;

			// RumbleLarge only ever passes in a value in that is <= 10,
			// and scaling that to 2 bytes is super annoying, so here's
			// some arbitrary values to increase the intensity.
			if (a1 >= 1 && a1 <= 10)
				m = max(0.2375f, intensity / 25.0f);
			else
				m = intensity / rumble_multi;

			intensity = (short)(SHRT_MAX * max(0.0, min(1.0f, m)));

			resultMotor = (Motor)(resultMotor | motor);
		}

		// TODO: Check if the player ID is currently AI controlled (or if intensity is 0).
		// This will avoid vibrating the second controller when Tails does something stupid.
		if (isWithinRange)
		{
			SetMotor(id, motor, intensity);
			rumble[id] = (Motor)(rumble[id] | resultMotor);
			XInputSetState(id, &vibration[id]);
		}
		else
		{
			for (uint8 i = 0; i < 4; i++)
			{
				SetMotor(i, motor, intensity);
				rumble[i] = (Motor)(rumble[i] | resultMotor);
				XInputSetState(i, &vibration[i]);
			}
		}
	}
	void __cdecl RumbleLarge(int playerNumber, signed int intensity)
	{
		if (!isCutscenePlaying && rumbleEnabled)
		{
			// Only continue if the calling player is Player 1 (0)
			// Vanilla SADX only rumbles for P1.
			if (!playerNumber)
				Rumble(playerNumber, clamp(intensity, 1, 255), Motor::Left);
		}
	}
	void __cdecl RumbleSmall(int playerNumber, signed int a2, signed int a3, int a4)
	{
		if (!isCutscenePlaying && rumbleEnabled)
		{
			// Only continue if the calling player is Player 1 (0)
			// Vanilla SADX only rumbles for P1.
			if (!playerNumber)
			{
				int _a2 = clamp(a2, -4, 4);

				// Could be inline if'd, but it'd look borderline unreadable.
				if (_a2 == 1)
					_a2 = 2;
				else if (_a2 == -1)
					_a2 = -2;

				int _a3 = clamp(a3, 7, 59);

				Rumble(playerNumber, max(1, a4 * _a3 / (4 * _a2)), Motor::Right);
			}
		}
	}

#pragma endregion
	
#pragma region Utility Functions
	/// <summary>
	/// Converts from XInput (-32768 - 32767) to Dreamcast (-127 - 127) axes, including scaled deadzone.
	/// </summary>
	/// <param name="dest">The destination axes (Dreamcast).</param>
	/// <param name="source">The source axes (XInput).</param>
	/// <param name="deadzone">The deadzone.</param>
	/// <param name="radial">If set to <c>true</c>, the deadzone is handled as radial. (e.g if the X axis exceeds deadzone, both X and Y axes are converted)</param>
	void ConvertAxes(short dest[2], short source[2], short deadzone, bool radial)
	{
		// This is being intentionally limited to -32767 instead of -32768
		const float x = (float)clamp(source[0], -SHRT_MAX, SHRT_MAX);
		const float y = (float)clamp(source[1], -SHRT_MAX, SHRT_MAX);

		if (abs(source[0]) < deadzone && abs(source[1]) < deadzone)
		{
			dest[0] = dest[1] = 0;
		}
		else
		{
			const float m = sqrt(x*x + y*y);
			const float nx = (m < deadzone) ? 0 : x / m;
			const float ny = (m < deadzone) ? 0 : y / m;
			const float n = (((m > SHRT_MAX) ? SHRT_MAX : m) - deadzone) / (SHRT_MAX - deadzone);

			dest[0] = (radial || abs(source[0]) >= deadzone) ? (short)(128 * (nx * n)) : 0;
			dest[1] = (radial || abs(source[1]) >= deadzone) ? (short)(-128 * (ny * n)) : 0;
		}
	}
	
	/// <summary>
	/// Converts XInput buttons to Dreamcast buttons.
	/// </summary>
	/// <param name="xpad">The XInput gamepad containing the buttons to convert.</param>
	/// <param name="id">The controller ID (player number)</param>
	/// <returns>Converted buttons.</returns>
	int ConvertButtons(XINPUT_GAMEPAD* xpad, ushort id)
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

	inline void SetMotor(short id, Motor motor, short intensity)
	{
		if (motor & Motor::Left)
		{
			vibration[id].wLeftMotorSpeed = intensity;
			rumble_l_elapsed[id] = GetTickCount();
		}
		if (motor & Motor::Right)
		{
			// This is doubled because it's never strong enough.
			vibration[id].wRightMotorSpeed = intensity * 2;
			rumble_r_elapsed[id] = GetTickCount();
		}
	}

	void SetDeadzone(short* array, uint id, int value)
	{
		if (value >= 0)
			array[id] = min(SHRT_MAX, value);
	}
}