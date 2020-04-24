#include "stdafx.h"

#include "minmax.h"
#include "KeyboardMouse.h"
#include "SADXKeyboard.h"

DataPointer(HWND, hWnd, 0x3D0FD30);

ControllerData KeyboardMouse::pad           = {};
float          KeyboardMouse::normalized_l_ = 0.0f;
float          KeyboardMouse::normalized_r_ = 0.0f;
bool           KeyboardMouse::mouse_active  = false;
bool           KeyboardMouse::left_button   = false;
bool           KeyboardMouse::right_button  = false;
bool           KeyboardMouse::half_press    = false;
bool           KeyboardMouse::e_held	    = false;
NJS_POINT2I    KeyboardMouse::cursor        = {};
KeyboardStick  KeyboardMouse::sticks[2]     = {};
Sint16         KeyboardMouse::mouse_x       = 0;
Sint16         KeyboardMouse::mouse_y       = 0;
WNDPROC        KeyboardMouse::lpPrevWndFunc = nullptr;

void KeyboardMouse::clear_sadx_keys(bool force)
{
	for (int i = 0; i < LengthOfArray(SADX2004Keys); i++)
	{
		KeyboardKeys[SADX2004Keys[i].SADX2004Code].old = KeyboardKeys[SADX2004Keys[i].SADX2004Code].held;
		KeyboardKeys[SADX2004Keys[i].SADX2004Code].pressed = 0;
		if (force) KeyboardKeys[SADX2004Keys[i].SADX2004Code].held = 0;
	}
}

void KeyboardMouse::update_sadx_key(Uint32 key, bool down)
{
	for (int i = 0; i < LengthOfArray(SADX2004Keys); i++)
	{
		if (key == SADX2004Keys[i].WindowsCode)
		{
			if (KeyboardKeys[SADX2004Keys[i].SADX2004Code].old != down) KeyboardKeys[SADX2004Keys[i].SADX2004Code].pressed = down;
			KeyboardKeys[SADX2004Keys[i].SADX2004Code].old = KeyboardKeys[SADX2004Keys[i].SADX2004Code].held;
			KeyboardKeys[SADX2004Keys[i].SADX2004Code].held = down;
			return;
		}
	}
}

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
	if (e_held) input::e_held = true;
}

void KeyboardMouse::update_keyboard_buttons(Uint32 key, bool down)
{
	// Update vanilla SADX array
	update_sadx_key(key, down);
	// Half press
	if (key == input::keys.Button_RightStick) half_press = down;
	// Center camera
	else if (key == input::keys.Button_LeftStick) e_held = down;
	// Buttons
	else if (key == input::keys.Button_A) set_button(pad.HeldButtons, Buttons_A, down);
	else if (key == input::keys.Button_B) set_button(pad.HeldButtons, Buttons_B, down);
	else if (key == input::keys.Button_X) set_button(pad.HeldButtons, Buttons_X, down);
	else if (key == input::keys.Button_Y) set_button(pad.HeldButtons, Buttons_Y, down);
	else if (key == input::keys.Button_RightShoulder) set_button(pad.HeldButtons, Buttons_Z, down);
	else if (key == input::keys.Button_LeftShoulder) set_button(pad.HeldButtons, Buttons_C, down);
	else if (key == input::keys.Button_Back) set_button(pad.HeldButtons, Buttons_D, down);
	else if (key == input::keys.Button_Start) set_button(pad.HeldButtons, Buttons_Start, down);
	// Triggers
	else if (key == input::keys.LT) set_button(pad.HeldButtons, Buttons_L, down);
	else if (key == input::keys.RT) set_button(pad.HeldButtons, Buttons_R, down);
	// D-Pad
	else if (key == input::keys.DPad_Up) set_button(pad.HeldButtons, Buttons_Up, down);
	else if (key == input::keys.DPad_Down) set_button(pad.HeldButtons, Buttons_Down, down);
	else if (key == input::keys.DPad_Left) set_button(pad.HeldButtons, Buttons_Left, down);
	else if (key == input::keys.DPad_Right) set_button(pad.HeldButtons, Buttons_Right, down);
	// Left stick
	else if (key == input::keys.Analog1_Up) set_button(sticks[0].directions, Buttons_Up, down);
	else if (key == input::keys.Analog1_Down) set_button(sticks[0].directions, Buttons_Down, down);
	else if (key == input::keys.Analog1_Left) set_button(sticks[0].directions, Buttons_Left, down);
	else if (key == input::keys.Analog1_Right) set_button(sticks[0].directions, Buttons_Right, down);
	// Right stick
	else if (key == input::keys.Analog2_Up) set_button(sticks[1].directions, Buttons_Up, down);
	else if (key == input::keys.Analog2_Down) set_button(sticks[1].directions, Buttons_Down, down);
	else if (key == input::keys.Analog2_Left) set_button(sticks[1].directions, Buttons_Left, down);
	else if (key == input::keys.Analog2_Right) set_button(sticks[1].directions, Buttons_Right, down);
}

void KeyboardMouse::update_cursor(Sint32 xrel, Sint32 yrel)
{
	if (!mouse_active || input::disablemouse)
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
	if (input::disablemouse) return;
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

WPARAM MapLeftRightKeys(WPARAM vk, LPARAM lParam)
{
	//Solution from https://stackoverflow.com/questions/5681284/how-do-i-distinguish-between-left-and-right-keys-ctrl-and-alt
	WPARAM new_vk = vk;
	UINT scancode = (lParam & 0x00ff0000) >> 16;
	int extended = (lParam & 0x01000000) != 0;

	switch (vk) {
	case VK_RETURN:
		if (lParam & 0x01000000) new_vk = 256;
		else new_vk = vk;
		break;
	case VK_SHIFT:
		new_vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
		break;
	case VK_CONTROL:
		new_vk = extended ? VK_RCONTROL : VK_LCONTROL;
		break;
	case VK_MENU:
		new_vk = extended ? VK_RMENU : VK_LMENU;
		break;
	default:
		new_vk = vk;
		break;
	}
	return new_vk;
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
		clear_sadx_keys(true);
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
		update_keyboard_buttons(MapLeftRightKeys(wParam, lParam), Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_F2 && !(lParam & 0x40000000) && GameMode != 1 && GameMode != 8)
		{
			if (input::debug) PrintDebug("Soft reset\n");
			WriteData<1>((char*)0x3B0EAA0, 0x01u);
		}
		update_keyboard_buttons(MapLeftRightKeys(wParam, lParam), Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		update_keyboard_buttons(MapLeftRightKeys(wParam, lParam), Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
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
