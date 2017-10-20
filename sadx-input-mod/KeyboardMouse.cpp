#include "stdafx.h"
#include "KeyboardMouse.h"

DataPointer(int, MouseMode, 0x03B0EAE0);
DataPointer(int, CursorY, 0x03B0E990);
DataPointer(int, CursorX, 0x03B0E994);
DataPointer(int, CursorMagnitude, 0x03B0E998);
DataPointer(int, CursorCos, 0x03B0E99C);
DataPointer(int, CursorSin, 0x03B0E9A0);
DataPointer(HWND, hWnd, 0x3D0FD30);

ControllerData KeyboardMouse::pad           = {};
float          KeyboardMouse::normalized_l  = 0.0f;
float          KeyboardMouse::normalized_r  = 0.0f;
bool           KeyboardMouse::mouse_active  = false;
bool           KeyboardMouse::left_button   = false;
bool           KeyboardMouse::right_button  = false;
bool           KeyboardMouse::half_press    = false;
NJS_POINT2I    KeyboardMouse::cursor        = {};
KeyboardStick  KeyboardMouse::sticks[2]     = {};
Sint16         KeyboardMouse::mouse_x       = 0;
Sint16         KeyboardMouse::mouse_y       = 0;
WNDPROC        KeyboardMouse::lpPrevWndFunc = nullptr;

inline void set_button(Uint32& i, Uint32 value, bool down)
{
	down ? i |= value : i &= ~value;
}

LRESULT __stdcall PollKeyboardMouse(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return KeyboardMouse::read_window_message(handle, Msg, wParam, lParam);
}

inline void normalize(const NJS_POINT2I& src, float* magnitude, short* out_x, short* out_y)
{
	constexpr auto short_max = std::numeric_limits<short>::max();
	auto x = static_cast<float>(clamp<short>(src.x, -short_max, short_max));
	auto y = static_cast<float>(clamp<short>(src.y, -short_max, short_max));
	float m = sqrt(x * x + y * y);

	if (m < FLT_EPSILON)
	{
		x = 0.0f;
		y = 0.0f;
	}
	else
	{
		x = 1.0f / m * x;
		y = 1.0f / m * y;
	}

	*magnitude = min(1.0f, m / static_cast<float>(short_max));

	*out_x = static_cast<short>(127 * x);
	*out_y = static_cast<short>(127 * y);
}

void KeyboardMouse::poll()
{
	hook_wnd_proc();

	sticks[0].update();
	sticks[1].update();
	NJS_POINT2I stick;

	if (sticks[0].x || sticks[0].y)
	{
		reset_cursor();
		stick = static_cast<NJS_POINT2I>(sticks[0]);
	}
	else
	{
		stick = cursor;
	}

	normalize(stick, &normalized_l, &pad.LeftStickX, &pad.LeftStickY);
	normalize(sticks[1], &normalized_r, &pad.RightStickX, &pad.RightStickY);

	if (half_press)
	{
		pad.LeftStickX /= 2;
		pad.LeftStickY /= 2;
		pad.RightStickX /= 2;
		pad.RightStickY /= 2;
		normalized_l /= 2.0f;
		normalized_r /= 2.0f;
	}

	DreamPad::update_buttons(pad, pad.HeldButtons);

	constexpr auto uchar_max = std::numeric_limits<uchar>::max();

	pad.LTriggerPressure = !!(pad.HeldButtons & Buttons_L) ? uchar_max : 0;
	pad.RTriggerPressure = !!(pad.HeldButtons & Buttons_R) ? uchar_max : 0;
}

void KeyboardMouse::update_keyboard_buttons(Uint32 key, bool down)
{
	switch (key)
	{
		default:
			break;

		case VK_SHIFT:
			half_press = down;
			break;

		case 'X':
		case VK_SPACE:
			set_button(pad.HeldButtons, Buttons_A, down);
			break;
		case 'Z':
			set_button(pad.HeldButtons, Buttons_B, down);
			break;
		case 'A':
			set_button(pad.HeldButtons, Buttons_X, down);
			break;
		case 'S':
			set_button(pad.HeldButtons, Buttons_Y, down);
			break;
		case 'Q':
			set_button(pad.HeldButtons, Buttons_L, down);
			break;
		case 'W':
			set_button(pad.HeldButtons, Buttons_R, down);
			break;
		case VK_RETURN:
			set_button(pad.HeldButtons, Buttons_Start, down);
			break;
		case 'D':
			set_button(pad.HeldButtons, Buttons_Z, down);
			break;
		case 'C':
			set_button(pad.HeldButtons, Buttons_C, down);
			break;
		case 'E':
			set_button(pad.HeldButtons, Buttons_D, down);
			break;

			// D-Pad
		case VK_NUMPAD8:
			set_button(pad.HeldButtons, Buttons_Up, down);
			break;
		case VK_NUMPAD5:
			set_button(pad.HeldButtons, Buttons_Down, down);
			break;
		case VK_NUMPAD4:
			set_button(pad.HeldButtons, Buttons_Left, down);
			break;
		case VK_NUMPAD6:
			set_button(pad.HeldButtons, Buttons_Right, down);
			break;

			// Left stick
		case VK_UP:
			set_button(sticks[0].directions, Buttons_Up, down);
			break;
		case VK_DOWN:
			set_button(sticks[0].directions, Buttons_Down, down);
			break;
		case VK_LEFT:
			set_button(sticks[0].directions, Buttons_Left, down);
			break;
		case VK_RIGHT:
			set_button(sticks[0].directions, Buttons_Right, down);
			break;

			// Right stick
		case 'I':
			set_button(sticks[1].directions, Buttons_Up, down);
			break;
		case 'K':
			set_button(sticks[1].directions, Buttons_Down, down);
			break;
		case 'J':
			set_button(sticks[1].directions, Buttons_Left, down);
			break;
		case 'L':
			set_button(sticks[1].directions, Buttons_Right, down);
			break;
	}
}

void KeyboardMouse::update_cursor(Sint32 xrel, Sint32 yrel)
{
	if (!mouse_active)
	{
		return;
	}

	CursorX = clamp(CursorX + xrel, -200, 200);
	CursorY = clamp(CursorY + yrel, -200, 200);

	auto& x = CursorX;
	auto& y = CursorY;

	auto m = x * x + y * y;

	if (m <= 625)
	{
		CursorMagnitude = 0;
		return;
	}

	CursorMagnitude = m / 361;

	if (CursorMagnitude >= 1)
	{
		if (CursorMagnitude > 120)
		{
			CursorMagnitude = 127;
		}
	}
	else
	{
		CursorMagnitude = 1;
	}

	njPushMatrix(reinterpret_cast<NJS_MATRIX_PTR>(0x0389D650));
	njRotateZ(nullptr, NJM_RAD_ANG(atan2(x, y)));

	NJS_VECTOR v = { 0.0f, static_cast<float>(CursorMagnitude) * 1.2f, 0.0f };
	njCalcVector(nullptr, &v, &v);

	CursorCos = static_cast<int>(v.x);
	CursorSin = static_cast<int>(v.y);

	constexpr auto short_max = static_cast<int>(std::numeric_limits<short>::max());

	auto& p = cursor;
	p.x = static_cast<Sint16>(clamp(static_cast<int>(-v.x / 128.0f * short_max), -short_max, short_max));
	p.y = static_cast<Sint16>(clamp(static_cast<int>(v.y / 128.0f * short_max), -short_max, short_max));

	njPopMatrix(1);
}

void KeyboardMouse::reset_cursor()
{
	CursorMagnitude = 0;
	CursorCos       = 0;
	CursorSin       = 0;
	CursorX         = 0;
	CursorY         = 0;
	cursor          = {};
	mouse_active    = false;
}

void KeyboardMouse::update_mouse_buttons(Uint32 button, bool down)
{
	bool last_rmb = right_button;

	switch (button)
	{
		case VK_LBUTTON:
			left_button = down;

			if (!down && !MouseMode)
			{
				reset_cursor();
			}

			mouse_active = down;
			break;

		case VK_RBUTTON:
			right_button = down;
			break;

		case VK_MBUTTON:
			set_button(pad.HeldButtons, Buttons_Start, down);
			break;

		default:
			break;
	}

	// When "A" (LMB + RMB) is active, B is not activated until next RMB press.
	bool a = (right_button && right_button != last_rmb) && left_button;
	set_button(pad.HeldButtons, Buttons_A, a);
	set_button(pad.HeldButtons, Buttons_B, right_button && (!a && right_button != last_rmb));
}

LRESULT KeyboardMouse::read_window_message(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_KILLFOCUS:
			sticks[0].directions = 0;
			sticks[1].directions = 0;
			pad.HeldButtons = 0;
			pad.LeftStickX  = 0;
			pad.LeftStickY  = 0;
			pad.RightStickX = 0;
			pad.RightStickY = 0;
			reset_cursor();
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			update_mouse_buttons(VK_LBUTTON, Msg == WM_LBUTTONDOWN);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			update_mouse_buttons(VK_RBUTTON, Msg == WM_RBUTTONDOWN);
			break;

		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			update_mouse_buttons(VK_MBUTTON, Msg == WM_MBUTTONDOWN);
			break;

		case WM_MOUSEMOVE:
		{
			auto x = static_cast<short>(lParam & 0xFFFF);
			auto y = static_cast<short>(lParam >> 16);

			update_cursor(x - mouse_x, y - mouse_y);

			mouse_x = x;
			mouse_y = y;
			break;
		}

		case WM_MOUSEWHEEL:
			break; // TODO

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_KEYUP:
			update_keyboard_buttons(wParam, Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
			break;

		default:
			break;
	}

	return CallWindowProc(lpPrevWndFunc, handle, Msg, wParam, lParam);
}

void KeyboardMouse::hook_wnd_proc()
{
	if (lpPrevWndFunc == nullptr)
	{
		lpPrevWndFunc = reinterpret_cast<WNDPROC>(SetWindowLong(hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(PollKeyboardMouse)));
	}
}
