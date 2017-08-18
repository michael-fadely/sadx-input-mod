#pragma once

#include "minmax.h"
#include <ninja.h>
#include <SADXStructs.h>

struct KeyboardStick : NJS_POINT2I
{
	Uint32 directions;
	static constexpr auto amount = 8192;

	void update()
	{
		auto horizontal = directions & (Buttons_Left | Buttons_Right);

		if (horizontal == Buttons_Left)
		{
			x = max(x - amount, -SHRT_MAX);
		}
		else if (horizontal == Buttons_Right)
		{
			x = min(x + amount, SHRT_MAX);
		}
		else
		{
			if (x < 0)
			{
				x = min(x + amount, 0);
			}
			else if (x > 0)
			{
				x = max(x - amount, 0);
			}
		}

		auto vertical = directions & (Buttons_Up | Buttons_Down);

		if (vertical == Buttons_Up)
		{
			y = max(y - amount, -SHRT_MAX);
		}
		else if (vertical == Buttons_Down)
		{
			y = min(y + amount, SHRT_MAX);
		}
		else
		{
			if (y < 0)
			{
				y = min(y + amount, 0);
			}
			else if (y > 0)
			{
				y = max(y - amount, 0);
			}
		}
	}
};

class KeyboardMouse
{
public:
	static const ControllerData& DreamcastData()
	{
		return pad;
	}
	static float NormalizedL()
	{
		return normalized_L;
	}
	static float NormalizedR()
	{
		return normalized_R;
	}

	static void Poll();
	static void UpdateKeyboardButtons(Uint32 key, bool down);
	static void UpdateCursor(Sint32 xrel, Sint32 yrel);
	static void ResetCursor();
	static void UpdateMouseButtons(Uint32 button, bool down);
	static LRESULT ReadWindowMessage(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam);
	static void HookWndProc();

private:
	static ControllerData pad;
	static float normalized_L, normalized_R;
	static bool mouse_update;
	static bool half_press;
	static NJS_POINT2I cursor;
	static KeyboardStick sticks[2];
	static Sint16 mouse_x;
	static Sint16 mouse_y;
	static WNDPROC lpPrevWndFunc;
};
