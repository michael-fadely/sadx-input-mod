#include "stdafx.h"
// Other crap
#include <SADXModLoader.h>
#include <limits.h>
#include "minmax.h"

// This namespace
#include "input.h"
#include "rumble.h"
#include "DreamPad.h"

// TODO: fix l/r
// TODO: fix alt+f4
// TODO: keyboard & controller half press modifier

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

struct AnalogThing
{
	Angle angle;
	float magnitude;
};

DataArray(AnalogThing, NormalizedAnalogs, 0x03B0E7A0, 8);
DataPointer(int, MouseMode, 0x03B0EAE0);
DataPointer(int, CursorY, 0x03B0E990);
DataPointer(int, CursorX, 0x03B0E994);
DataPointer(int, CursorMagnitude, 0x03B0E998);
DataPointer(int, CursorCos, 0x03B0E99C);
DataPointer(int, CursorSin, 0x03B0E9A0);

static bool mouse_update = false;
static NJS_POINT2I cursor = {};
static KeyboardStick sticks[2] = {};
static uint32 add_buttons = 0;

inline void set_button(Uint32& i, Uint32 value, bool down)
{
	if (down)
	{
		i |= value;
	}
	else
	{
		i &= ~value;
	}
}

static void UpdateKeyboardButtons(Uint32 key, bool down)
{
	switch (key)
	{
		default:
			break;

		case 'X':
		case VK_SPACE:
			set_button(add_buttons, Buttons_A, down);
			break;
		case 'Z':
			set_button(add_buttons, Buttons_B, down);
			break;
		case 'A':
			set_button(add_buttons, Buttons_X, down);
			break;
		case 'S':
			set_button(add_buttons, Buttons_Y, down);
			break;
		case 'Q':
			set_button(add_buttons, Buttons_L, down);
			break;
		case 'W':
			set_button(add_buttons, Buttons_R, down);
			break;
		case VK_RETURN:
			set_button(add_buttons, Buttons_Start, down);
			break;
		case 'D':
			set_button(add_buttons, Buttons_Z, down);
			break;
		case 'C':
			set_button(add_buttons, Buttons_C, down);
			break;
		case 'E':
			set_button(add_buttons, Buttons_D, down);
			break;

			// D-Pad
		case VK_NUMPAD8:
			set_button(add_buttons, Buttons_Up, down);
			break;
		case VK_NUMPAD5:
			set_button(add_buttons, Buttons_Down, down);
			break;
		case VK_NUMPAD4:
			set_button(add_buttons, Buttons_Left, down);
			break;
		case VK_NUMPAD6:
			set_button(add_buttons, Buttons_Right, down);
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

static void UpdateCursor(Sint32 xrel, Sint32 yrel)
{
	if (!mouse_update)
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

	njPushMatrix((NJS_MATRIX*)0x0389D650);

	auto r = (Angle)(atan2((double)x, (double)y) * 65536.0 * 0.1591549762031479);

	if (r)
	{
		njRotateZ(nullptr, r);
	}

	NJS_VECTOR v = { 0.0f, (float)CursorMagnitude * 1.2f, 0.0f };
	njCalcVector(nullptr, &v, &v);

	CursorCos = (int)v.x;
	CursorSin = (int)v.y;

	auto& p = cursor;
	p.x = (Sint16)clamp((int)(-v.x / 128.0f * SHRT_MAX), -SHRT_MAX, SHRT_MAX);
	p.y = (Sint16)clamp((int)(v.y / 128.0f * SHRT_MAX), -SHRT_MAX, SHRT_MAX);

	njPopMatrix(1);
}

static void ResetCursor()
{
	CursorMagnitude = 0;
	CursorCos       = 0;
	CursorSin       = 0;
	CursorX         = 0;
	CursorY         = 0;
	cursor          = {};
	mouse_update    = false;
}

static void UpdateMouseButtons(Uint32 button, bool down)
{
	switch (button)
	{
		case VK_LBUTTON:
			if (!down && !MouseMode)
			{
				ResetCursor();
			}
			mouse_update = down;
			break;

		case VK_RBUTTON:
			if (mouse_update)
			{
				set_button(add_buttons, Buttons_A, down);
			}
			else
			{
				set_button(add_buttons, Buttons_B, down);
			}
			break;

		case VK_MBUTTON:
			set_button(add_buttons, Buttons_Start, down);
			break;

		default:
			break;
	}
}

DataPointer(HWND, hWnd, 0x3D0FD30);
static WNDPROC lpPrevWndFunc = nullptr;

static Sint16 mouse_x = 0;
static Sint16 mouse_y = 0;

namespace input
{
	ControllerData RawInput[GAMEPAD_COUNT];
	bool _ControllerEnabled[GAMEPAD_COUNT];
	bool debug = false;

	LRESULT __stdcall PollKeyboardMouse(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg)
		{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				UpdateMouseButtons(VK_LBUTTON, Msg == WM_LBUTTONDOWN);
				break;

			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				UpdateMouseButtons(VK_RBUTTON, Msg == WM_RBUTTONDOWN);
				break;

			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				UpdateMouseButtons(VK_MBUTTON, Msg == WM_MBUTTONDOWN);
				break;

			case WM_MOUSEMOVE:
			{
				auto x = (short)(lParam & 0xFFFF);
				auto y = (short)(lParam >> 16);

				UpdateCursor(x - mouse_x, y - mouse_y);

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
				UpdateKeyboardButtons(wParam, Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN);
				break;

			default:
				break;
		}

		return CallWindowProc(lpPrevWndFunc, handle, Msg, wParam, lParam);
	}

	void HookWndProc()
	{
		if (lpPrevWndFunc == nullptr)
		{
			lpPrevWndFunc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG)PollKeyboardMouse);
		}
	}

	void PollControllers()
	{
		HookWndProc();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				default:
					break;

				case SDL_CONTROLLERDEVICEADDED:
				{
					int which = event.cdevice.which;
					for (uint i = 0; i < GAMEPAD_COUNT; i++)
					{
						// Checking for both in cases like the DualShock 4 and DS4Windows where the controller might be
						// "connected" twice with the same ID. DreamPad::Open automatically closes if already open.
						if (!DreamPad::Controllers[i].Connected() || DreamPad::Controllers[i].ControllerID() == which)
						{
							DreamPad::Controllers[i].Open(which);
							break;
						}
					}
					break;
				}

				case SDL_CONTROLLERDEVICEREMOVED:
				{
					int which = event.cdevice.which;
					for (uint i = 0; i < GAMEPAD_COUNT; i++)
					{
						if (DreamPad::Controllers[i].ControllerID() == which)
						{
							DreamPad::Controllers[i].Close();
							break;
						}
					}
					break;
				}
			}
		}

		sticks[0].update();
		sticks[1].update();

		if (sticks[0].x || sticks[0].y)
		{
			ResetCursor();
		}
		else
		{
			*(NJS_POINT2I*)&sticks[0] = cursor;
		}

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad& dpad = DreamPad::Controllers[i];

			auto buttons = !i ? add_buttons : 0;
			NJS_POINT2I* ls = !i ? &sticks[0] : nullptr;
			NJS_POINT2I* rs = !i ? &sticks[1] : nullptr;

			dpad.Poll(buttons, ls, rs);
			dpad.Copy(RawInput[i]);

			// Compatibility for mods who use ControllersRaw directly.
			// This will only copy the first four controllers.
			if (i < ControllersRaw_Length)
			{
				ControllersRaw[i] = RawInput[i];
			}

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
					int pressed = pad->PressedButtons;
					if (pressed & Buttons_Up)
					{
						dpad.settings.rumbleFactor += 0.125f;
					}
					else if (pressed & Buttons_Down)
					{
						dpad.settings.rumbleFactor -= 0.125f;
					}
					else if (pressed & Buttons_Left)
					{
						rumble::RumbleA(i, 0);
					}
					else if (pressed & Buttons_Right)
					{
						rumble::RumbleB(i, 7, 59, 6);
					}

					DisplayDebugStringFormatted(NJM_LOCATION(4, 10 + (3 * i)), "Rumble factor (U/D): %f (L/R to test)", dpad.settings.rumbleFactor);
				}
			}
#endif
		}
	}

	static void FixAnalogs()
	{
		if (!ControlEnabled)
		{
			return;
		}

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			if (!_ControllerEnabled[i])
			{
				continue;
			}

			const DreamPad& dreamPad = DreamPad::Controllers[i];

			if (dreamPad.Connected())
			{
				const ControllerData& pad = dreamPad.DreamcastData();
				// SADX's internal deadzone is 12 of 127. It doesn't set the relative forward direction
				// unless this is exceeded in WriteAnalogs(), so the analog shouldn't be set otherwise.
				if (abs(pad.LeftStickX) > 12 || abs(pad.LeftStickY) > 12)
				{
					NormalizedAnalogs[i].magnitude = dreamPad.NormalizedL();
				}
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

	static void RedirectRawControllers()
	{
		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			ControllerPointers[i] = &RawInput[i];
		}
	}

	void __declspec(naked) RedirectRawControllers_Hook()
	{
		__asm
		{
			call RedirectRawControllers
			ret
		}
	}

	void EnableController_hook(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			_ControllerEnabled[0] = true;
			_ControllerEnabled[1] = true;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < min(index, (Uint8)GAMEPAD_COUNT); i++)
			{
				EnableController_hook(i);
			}
		}
		else
		{
			_ControllerEnabled[index] = true;
		}
	}

	void DisableController_hook(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			_ControllerEnabled[0] = false;
			_ControllerEnabled[1] = false;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < min(index, (Uint8)GAMEPAD_COUNT); i++)
			{
				DisableController_hook(i);
			}
		}
		else
		{
			_ControllerEnabled[index] = false;
		}
	}
}
