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
		const auto horizontal = directions & (Buttons_Left | Buttons_Right);

		if (horizontal == Buttons_Left)
		{
			x = max(x - amount, -static_cast<int>(std::numeric_limits<short>::max()));
		}
		else if (horizontal == Buttons_Right)
		{
			x = min(x + amount, static_cast<int>(std::numeric_limits<short>::max()));
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

		const auto vertical = directions & (Buttons_Up | Buttons_Down);

		if (vertical == Buttons_Up)
		{
			y = max(y - amount, -static_cast<int>(std::numeric_limits<short>::max()));
		}
		else if (vertical == Buttons_Down)
		{
			y = min(y + amount, static_cast<int>(std::numeric_limits<short>::max()));
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
		return normalized_l;
	}
	static float NormalizedR()
	{
		return normalized_r;
	}

	static void poll();
	static void update_keyboard_buttons(Uint32 key, bool down);
	static void update_cursor(Sint32 xrel, Sint32 yrel);
	static void reset_cursor();
	static void update_mouse_buttons(Uint32 button, bool down);
	static LRESULT read_window_message(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam);
	static void hook_wnd_proc();

private:
	static ControllerData pad;
	static float normalized_l, normalized_r;
	static bool mouse_update;
	static bool half_press;
	static NJS_POINT2I cursor;
	static KeyboardStick sticks[2];
	static Sint16 mouse_x;
	static Sint16 mouse_y;
	static WNDPROC lpPrevWndFunc;
};
