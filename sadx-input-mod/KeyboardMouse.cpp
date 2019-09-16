#include "stdafx.h"

#include "minmax.h"
#include "KeyboardMouse.h"
#include "KeyboardVariables.h"

DataPointer(HWND, hWnd, 0x3D0FD30);

ControllerData KeyboardMouse::pad           = {};
float          KeyboardMouse::normalized_l_ = 0.0f;
float          KeyboardMouse::normalized_r_ = 0.0f;
bool           KeyboardMouse::mouse_active  = false;
bool           KeyboardMouse::left_button   = false;
bool           KeyboardMouse::right_button  = false;
bool           KeyboardMouse::half_press    = false;
NJS_POINT2I    KeyboardMouse::cursor        = {};
KeyboardStick  KeyboardMouse::sticks[2]     = {};
Sint16         KeyboardMouse::mouse_x       = 0;
Sint16         KeyboardMouse::mouse_y       = 0;
WNDPROC        KeyboardMouse::lpPrevWndFunc = nullptr;

bool DisableMouse = false;

inline void set_button(Uint32& i, Uint32 value, bool down)
{
	down ? i |= value : i &= ~value;
}

LRESULT __stdcall poll_keyboard_mouse(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
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
		x = x / m;
		y = y / m;
	}

	*magnitude = min(1.0f, m / static_cast<float>(short_max));

	*out_x = static_cast<short>(127 * x);
	*out_y = static_cast<short>(127 * y);
}

// TODO: framerate-independent interpolation
void KeyboardStick::update()
{
#define INTERPOLATE

	const auto horizontal = directions & (Buttons_Left | Buttons_Right);

	if (horizontal == Buttons_Left)
	{
	#ifdef INTERPOLATE
		x = max(x - amount, -static_cast<int>(std::numeric_limits<short>::max()));
	#else
		x = -static_cast<int>(std::numeric_limits<short>::max());
	#endif
	}
	else if (horizontal == Buttons_Right)
	{
	#ifdef INTERPOLATE
		x = min(x + amount, static_cast<int>(std::numeric_limits<short>::max()));
	#else
		x = static_cast<int>(std::numeric_limits<short>::max());
	#endif
	}
	else
	{
	#ifdef INTERPOLATE
		if (x < 0)
		{
			x = min(x + amount, 0);
		}
		else if (x > 0)
		{
			x = max(x - amount, 0);
		}
	#else
		x = 0;
	#endif
	}

	const auto vertical = directions & (Buttons_Up | Buttons_Down);

	if (vertical == Buttons_Up)
	{
	#ifdef INTERPOLATE
		y = max(y - amount, -static_cast<int>(std::numeric_limits<short>::max()));
	#else
		y = -static_cast<int>(std::numeric_limits<short>::max());
	#endif
	}
	else if (vertical == Buttons_Down)
	{
	#ifdef INTERPOLATE
		y = min(y + amount, static_cast<int>(std::numeric_limits<short>::max()));
	#else
		y = static_cast<int>(std::numeric_limits<short>::max());
	#endif
	}
	else
	{
	#ifdef INTERPOLATE
		if (y < 0)
		{
			y = min(y + amount, 0);
		}
		else if (y > 0)
		{
			y = max(y - amount, 0);
		}
	#else
		y = 0;
	#endif
	}
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

	normalize(stick, &normalized_l_, &pad.LeftStickX, &pad.LeftStickY);
	normalize(sticks[1], &normalized_r_, &pad.RightStickX, &pad.RightStickY);

	if (half_press)
	{
		pad.LeftStickX /= 2;
		pad.LeftStickY /= 2;
		pad.RightStickX /= 2;
		pad.RightStickY /= 2;
		normalized_l_ /= 2.0f;
		normalized_r_ /= 2.0f;
	}

	DreamPad::update_buttons(pad, pad.HeldButtons);

	constexpr auto uchar_max = std::numeric_limits<uchar>::max();

	pad.LTriggerPressure = !!(pad.HeldButtons & Buttons_L) ? uchar_max : 0;
	pad.RTriggerPressure = !!(pad.HeldButtons & Buttons_R) ? uchar_max : 0;
}

void KeyboardMouse::update_keyboard_buttons(Uint32 key, bool down)
{
	SetVanillaSADXKey(key, down);
	if (key == VK_SHIFT) half_press = down;
	if (key == KButton_A || key == KButton2_A || key == KButton3_A) set_button(pad.HeldButtons, Buttons_A, down);
	if (key == KButton_B || key == KButton2_B || key == KButton3_B) set_button(pad.HeldButtons, Buttons_B, down);
	if (key == KButton_X || key == KButton2_X || key == KButton3_X) set_button(pad.HeldButtons, Buttons_X, down);
	if (key == KButton_Y || key == KButton2_Y || key == KButton3_Y) set_button(pad.HeldButtons, Buttons_Y, down);
	if (key == KButton_L || key == KButton2_L || key == KButton3_L) set_button(pad.HeldButtons, Buttons_L, down);
	if (key == KButton_R || key == KButton2_R || key == KButton3_R) set_button(pad.HeldButtons, Buttons_R, down);
	if (key == KButton_Z || key == KButton2_Z || key == KButton3_Z) set_button(pad.HeldButtons, Buttons_Z, down);
	if (key == KButton_C || key == KButton2_C || key == KButton3_C) set_button(pad.HeldButtons, Buttons_C, down);
	if (key == KButton_D || key == KButton2_D || key == KButton3_D) set_button(pad.HeldButtons, Buttons_D, down);
	if (key == KButton_Center || key == KButton2_Center || key == KButton3_Center) CenterKey = down;
	if (key == KButton_Start || key == KButton2_Start || key == KButton3_Start) set_button(pad.HeldButtons, Buttons_Start, down);
	if (key == KButton_DPadUp || key == KButton2_DPadUp || key == KButton3_DPadUp) set_button(sticks[1].directions, Buttons_Up, down);
	if (key == KButton_DPadDown || key == KButton2_DPadDown || key == KButton3_DPadDown) set_button(sticks[1].directions, Buttons_Down, down);
	if (key == KButton_DPadLeft || key == KButton2_DPadLeft || key == KButton3_DPadLeft) set_button(sticks[1].directions, Buttons_Left, down);
	if (key == KButton_DPadRight || key == KButton2_DPadRight || key == KButton3_DPadRight) set_button(sticks[1].directions, Buttons_Right, down);
	if (key == KButton_Up || key == KButton2_Up || key == KButton3_Up) set_button(sticks[0].directions, Buttons_Up, down);
	if (key == KButton_Down || key == KButton2_Down || key == KButton3_Down) set_button(sticks[0].directions, Buttons_Down, down);
	if (key == KButton_Left || key == KButton2_Left || key == KButton3_Left) set_button(sticks[0].directions, Buttons_Left, down);
	if (key == KButton_Right || key == KButton2_Right || key == KButton3_Right) set_button(sticks[0].directions, Buttons_Right, down);
}

void KeyboardMouse::update_cursor(Sint32 xrel, Sint32 yrel)
{
	if (!mouse_active || DisableMouse)
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
	if (DisableMouse) return;
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

	if (left_button)
	{
		set_button(pad.HeldButtons, Buttons_B, right_button && right_button == last_rmb);
		set_button(pad.HeldButtons, Buttons_A, right_button && right_button != last_rmb);
	}
	else
	{
		set_button(pad.HeldButtons, Buttons_A, false);
		set_button(pad.HeldButtons, Buttons_B, right_button);
	}
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
		lpPrevWndFunc = reinterpret_cast<WNDPROC>(SetWindowLong(hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(poll_keyboard_mouse)));
	}
}
