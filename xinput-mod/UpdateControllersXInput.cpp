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

DataPointer(int, isCutscenePlaying, 0x3B2A2E4);		// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);		// Not sure why this is a char and ^ is an int.
DataArray(bool, Controller_Enabled, 0x00909FB4, 4);	// TODO: Figure out what toggles this for P2.

#define clamp(value, low, high) min(max(low, value), high)

namespace xinput
{
	Settings::Settings()
	{
		deadzoneL			= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		deadzoneR			= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		triggerThreshold	= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		normalizeL			= true;
		normalizeR			= false;
		rumbleFactor		= 1.0f;
		scaleFactor			= 1.5f;
	}
	void Settings::apply(short deadzoneL, short deadzoneR, bool normalizeL, bool normalizeR, uint8 triggerThreshold, float rumbleFactor, float scaleFactor)
	{
		this->deadzoneL			= clamp(deadzoneL, 0, SHRT_MAX);
		this->deadzoneR			= clamp(deadzoneR, 0, SHRT_MAX);
		this->normalizeL		= normalizeL;
		this->normalizeR		= normalizeR;
		this->triggerThreshold	= min(UCHAR_MAX, triggerThreshold);
		this->rumbleFactor		= rumbleFactor;
		this->scaleFactor		= max(1.0f, scaleFactor);
	}

	Settings settings[XPAD_COUNT];

	const uint rumble_l_timer = 250;
	const uint rumble_r_timer = 1000;

	XINPUT_VIBRATION vibration[XPAD_COUNT];
	Motor rumble[XPAD_COUNT];
	uint rumble_l_elapsed[XPAD_COUNT];
	uint rumble_r_elapsed[XPAD_COUNT];

#pragma region Ingame Functions

	// TODO: Keyboard & Mouse
	void __cdecl UpdateControllersXInput()
	{
		for (ushort i = 0; i < XPAD_COUNT; i++)
		{
			ControllerData* pad = &ControllersRaw[i];
			XINPUT_STATE state = {};
			XInputGetState(i, &state);
			XINPUT_GAMEPAD* xpad = &state.Gamepad;

			// Gotta get that enum set up for this.
			pad->Support = 0x3F07FEu;

			// L Analog
			ConvertAxes(i, (short*)&pad->LeftStickX, (short*)&xpad->sThumbLX,
				settings[i].deadzoneL, settings[i].normalizeL);

			// R Analog
			ConvertAxes(i, (short*)&pad->RightStickX, (short*)&xpad->sThumbRX,
				settings[i].deadzoneR, settings[i].normalizeR);

			// Trigger pressure
			pad->LTriggerPressure = xpad->bLeftTrigger;
			pad->RTriggerPressure = xpad->bRightTrigger;

			// Now, get the new buttons from the XInput xpad
			pad->HeldButtons = ConvertButtons(i, xpad);
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
						settings[i].rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						settings[i].rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(6, "Rumble factor (U/D): %f", settings[i].rumbleFactor);
				}
			}
#endif
		}
	}

	void Rumble(ushort id, int a1, Motor motor)
	{
		if (id >= XPAD_COUNT)
		{
			for (ushort i = 0; i < XPAD_COUNT; i++)
				Rumble(id, a1, motor);

			return;
		}

		short intensity = 4 * a1;
		Motor resultMotor = rumble[id];

		if (a1 > 0)
		{
			float m;

			// RumbleLarge only ever passes in a value in that is <= 10,
			// and scaling that to 2 bytes is super annoying, so here's
			// some arbitrary values to increase the intensity.
			if (a1 >= 1 && a1 <= 10)
				m = max(0.2375f, intensity / 25.0f);
			else
				m = (float)intensity / 256.0f;

			intensity = (short)(SHRT_MAX * clamp(m, 0.0f, 1.0f));

			resultMotor = (Motor)(resultMotor | motor);
		}

		if (intensity == 0 || Controller_Enabled[id])
		{
			SetMotor(id, motor, intensity);
			rumble[id] = (Motor)(rumble[id] | resultMotor);
			XInputSetState(id, &vibration[id]);
		}
	}
	void __cdecl RumbleLarge(int playerNumber, int intensity)
	{
		if (!isCutscenePlaying && rumbleEnabled)
			Rumble(playerNumber, clamp(intensity, 1, 255), Motor::Left);
	}
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4)
	{
		if (!isCutscenePlaying && rumbleEnabled)
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

#pragma endregion

#pragma region Utility Functions
	/// <summary>
	/// Converts from XInput (-32768 - 32767) to Dreamcast (-127 - 127) axes, including scaled deadzone.
	/// </summary>
	/// <param name="id">The controller ID (player number)</param>
	/// <param name="dest">The destination axes (Dreamcast).</param>
	/// <param name="source">The source axes (XInput).</param>
	/// <param name="deadzone">The deadzone.</param>
	/// <param name="normalize">If set to <c>true</c>, the deadzone is treated as radial. (e.g if the X axis exceeds deadzone, both X and Y axes are converted)</param>
	void ConvertAxes(ushort id, short dest[2], short source[2], short deadzone, bool normalize)
	{
		// This is being intentionally limited to -32767 instead of -32768
		const float x = (float)clamp(source[0], -SHRT_MAX, SHRT_MAX);
		const float y = (float)clamp(source[1], -SHRT_MAX, SHRT_MAX);

		// Using this will deliberately put us outside the proper range,
		// but this is unfortunately required for proper diagonal movement.
		// TODO: Investigate fixing this without deliberately going out of range (which kills my beautiful perfectly radial magic).
		const short factor = (short)(128 * settings[id].scaleFactor);

		if (abs(source[0]) < deadzone && abs(source[1]) < deadzone)
		{
			dest[0] = dest[1] = 0;
		}
		else
		{
			const float m	= sqrt(x * x + y * y);			// Magnitude (length)
			const float nx	= (m < deadzone) ? 0 : x / m;	// Normalized (X)
			const float ny	= (m < deadzone) ? 0 : y / m;	// Normalized (Y)
			const float n	= (min((float)SHRT_MAX, m) - deadzone) / ((float)SHRT_MAX - deadzone);

			// In my testing, multiplying -128 - 128 results in 127 instead, which is the desired value.
			dest[0] = (normalize || abs(source[0]) >= deadzone) ? (short)clamp((factor * (nx * n)), -127, 127) : 0;
			dest[1] = (normalize || abs(source[1]) >= deadzone) ? (short)clamp((-factor * (ny * n)), -127, 127) : 0;
		}
	}

	/// <summary>
	/// Converts XInput buttons to Dreamcast buttons.
	/// </summary>
	/// <param name="xpad">The XInput gamepad containing the buttons to convert.</param>
	/// <param name="id">The controller ID (player number)</param>
	/// <returns>Converted buttons.</returns>
	int ConvertButtons(ushort id, XINPUT_GAMEPAD* xpad)
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

#ifdef EXTENDED_BUTTONS
		if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)
			result |= Buttons_C;
		if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
			result |= Buttons_Z;
		if (buttons & XINPUT_GAMEPAD_BACK)
			result |= Buttons_D;
#endif

		if (xpad->bLeftTrigger > settings[id].triggerThreshold)
			result |= Buttons_L;
		if (xpad->bRightTrigger > settings[id].triggerThreshold)
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

	/// <summary>
	/// Rumbles the specified motor using the specified intensity.
	/// </summary>
	/// <param name="id">The identifier.</param>
	/// <param name="motor">The motor.</param>
	/// <param name="intensity">The intensity.</param>
	inline void SetMotor(ushort id, Motor motor, short intensity)
	{
		float m = settings[id].rumbleFactor;

		if (motor & Motor::Left)
		{
			vibration[id].wLeftMotorSpeed = (short)min(SHRT_MAX, (int)(intensity * m));
			rumble_l_elapsed[id] = GetTickCount();
		}

		if (motor & Motor::Right)
		{
			vibration[id].wRightMotorSpeed = (short)min(SHRT_MAX, (int)(intensity * (2 + m)));
			rumble_r_elapsed[id] = GetTickCount();
		}
	}
}
