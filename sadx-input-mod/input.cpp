#include "stdafx.h"
// Other crap
#include <SADXModLoader.h>
#include <limits.h>
#include "minmax.h"

// This namespace
#include "input.h"
#include "rumble.h"
#include "DreamPad.h"

// TODO: mouse buttons
// TODO: fix alt+f4

struct KeyboardStick : NJS_POINT2I
{
	Uint32 directions;

	void update()
	{
		auto horizontal = directions & (Buttons_Left | Buttons_Right);

		if (horizontal == Buttons_Left)
		{
			x = -SHRT_MAX;
		}
		else if (horizontal == Buttons_Right)
		{
			x = SHRT_MAX;
		}
		else
		{
			x = 0;
		}

		auto vertical = directions & (Buttons_Up | Buttons_Down);

		if (vertical == Buttons_Up)
		{
			y = -SHRT_MAX;
		}
		else if (vertical == Buttons_Down)
		{
			y = SHRT_MAX;
		}
		else
		{
			y = 0;
		}

	}
};

struct AnalogThing
{
	Angle	angle;
	float	magnitude;
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

inline void set_button(Uint32& i, Uint32 value, bool key_down)
{
	if (key_down)
	{
		i |= value;
	}
	else
	{
		i &= ~value;
	}
}

static void UpdateKeyboardButtons(Uint32 scancode, bool key_down)
{
	switch (scancode)
	{
		default:
			break;

		case SDL_SCANCODE_X:
		case SDL_SCANCODE_SPACE:
			set_button(add_buttons, Buttons_A, key_down);
			break;
		case SDL_SCANCODE_Z:
			set_button(add_buttons, Buttons_B, key_down);
			break;
		case SDL_SCANCODE_A:
			set_button(add_buttons, Buttons_X, key_down);
			break;
		case SDL_SCANCODE_S:
			set_button(add_buttons, Buttons_Y, key_down);
			break;
		case SDL_SCANCODE_Q:
			set_button(add_buttons, Buttons_L, key_down);
			break;
		case SDL_SCANCODE_W:
			set_button(add_buttons, Buttons_R, key_down);
			break;
		case SDL_SCANCODE_RETURN:
			set_button(add_buttons, Buttons_Start, key_down);
			break;
		case SDL_SCANCODE_D:
			set_button(add_buttons, Buttons_Z, key_down);
			break;
		case SDL_SCANCODE_C:
			set_button(add_buttons, Buttons_C, key_down);
			break;
		case SDL_SCANCODE_E:
			set_button(add_buttons, Buttons_D, key_down);
			break;

			// D-Pad
		case SDL_SCANCODE_KP_8:
			set_button(add_buttons, Buttons_Up, key_down);
			break;
		case SDL_SCANCODE_KP_5:
			set_button(add_buttons, Buttons_Down, key_down);
			break;
		case SDL_SCANCODE_KP_4:
			set_button(add_buttons, Buttons_Left, key_down);
			break;
		case SDL_SCANCODE_KP_6:
			set_button(add_buttons, Buttons_Right, key_down);
			break;

			// Left stick
		case SDL_SCANCODE_UP:
			set_button(sticks[0].directions, Buttons_Up, key_down);
			break;
		case SDL_SCANCODE_DOWN:
			set_button(sticks[0].directions, Buttons_Down, key_down);
			break;
		case SDL_SCANCODE_LEFT:
			set_button(sticks[0].directions, Buttons_Left, key_down);
			break;
		case SDL_SCANCODE_RIGHT:
			set_button(sticks[0].directions, Buttons_Right, key_down);
			break;

			// Right stick
		case SDL_SCANCODE_I:
			set_button(sticks[1].directions, Buttons_Up, key_down);
			break;
		case SDL_SCANCODE_K:
			set_button(sticks[1].directions, Buttons_Down, key_down);
			break;
		case SDL_SCANCODE_J:
			set_button(sticks[1].directions, Buttons_Left, key_down);
			break;
		case SDL_SCANCODE_L:
			set_button(sticks[1].directions, Buttons_Right, key_down);
			break;
	}
}

void UpdateCursor(Sint32 xrel, Sint32 yrel)
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
	CursorCos = 0;
	CursorSin = 0;
	CursorX = 0;
	CursorY = 0;
	cursor = {};
	mouse_update = false;
}

static void UpdateMouseButtons(Uint32 button, bool key_down)
{
	switch (button)
	{
		case SDL_BUTTON_LEFT:
			if (!key_down && !MouseMode)
			{
				ResetCursor();
			}
			mouse_update = key_down;
			break;

		case SDL_BUTTON_MIDDLE:
			break;
		case SDL_BUTTON_RIGHT:
			break;

		default:
			break;
	}
}

namespace input
{
	ControllerData RawInput[GAMEPAD_COUNT];
	bool _ControllerEnabled[GAMEPAD_COUNT];
	bool debug = false;

	void PollControllers()
	{
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

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					UpdateKeyboardButtons(event.key.keysym.scancode, event.type == SDL_KEYDOWN);
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					UpdateMouseButtons(event.button.button, event.type == SDL_MOUSEBUTTONDOWN);
					break;

				case SDL_MOUSEMOTION:
				{
					UpdateCursor(event.motion.xrel, event.motion.yrel);
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
