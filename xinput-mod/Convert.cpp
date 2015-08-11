#include <SADXModLoader/SADXEnums.h>
#include "Common.h"
#include "Convert.h"

namespace xinput
{
	/// <summary>
	/// Converts from XInput (-32768 - 32767) to Dreamcast (-127 - 127) axes, including scaled deadzone.
	/// </summary>
	/// <param name="scaleFactor">The analog scale factor.</param>
	/// <param name="dest">The destination axes (Dreamcast).</param>
	/// <param name="source">The source axes (XInput).</param>
	/// <param name="deadzone">The deadzone.</param>
	/// <param name="radial">If set to <c>true</c>, the deadzone is treated as fully radial. (i.e one axis exceeding deadzone implies the other)</param>
	void ConvertAxes(float scaleFactor, short dest[2], short source[2], short deadzone, bool radial)
	{
		if (abs(source[0]) < deadzone && abs(source[1]) < deadzone)
		{
			dest[0] = dest[1] = 0;
			return;
		}

		// This is being intentionally limited to -32767 instead of -32768
		const float x = (float)clamp(source[0], (short)-SHRT_MAX, (short)SHRT_MAX);
		const float y = (float)clamp(source[1], (short)-SHRT_MAX, (short)SHRT_MAX);

		// Magnitude (length)	// Doing this with the default value (1.5) will deliberately put us outside the proper range,
		// but this is unfortunately required for proper diagonal movement. That's a pretty conservative default, too.
		// TODO: Investigate fixing this without deliberately going out of range (which kills my beautiful perfectly radial magic).
		const short factor = (short)(128 * scaleFactor);

		const float m = sqrt(x * x + y * y);

		const float nx = (m < deadzone) ? 0 : x / m;	// Normalized (X)
		const float ny = (m < deadzone) ? 0 : y / m;	// Normalized (Y)
		const float n = (min((float)SHRT_MAX, m) - deadzone) / ((float)SHRT_MAX - deadzone);

		// In my testing, multiplying -128 - 128 results in 127 instead, which is the desired value.
		dest[0] = (radial || abs(source[0]) >= deadzone) ? (short)clamp((short)(factor * (nx * n)), (short)-127, (short)127) : 0;
		dest[1] = (radial || abs(source[1]) >= deadzone) ? (short)clamp((short)(-factor * (ny * n)), (short)-127, (short)127) : 0;
	}

	/// <summary>
	/// Converts XInput buttons to Dreamcast buttons.
	/// </summary>
	/// <param name="buttons">Xbox 360 buttons (wButtons) to be converted.</param>
	/// <returns>Dreamcast buttons.</returns>
	int ConvertButtons(short buttons)
	{
		int result = 0;

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
