#pragma once

#include "DreamPad.h"

struct KeyboardMapping
{
	Sint16 Analog1_Up;
	Sint16 Analog1_Down;
	Sint16 Analog1_Left;
	Sint16 Analog1_Right;
	Sint16 Analog2_Up;
	Sint16 Analog2_Down;
	Sint16 Analog2_Left;
	Sint16 Analog2_Right;
	Sint16 LT;
	Sint16 RT;
	Sint16 DPad_Up;
	Sint16 DPad_Down;
	Sint16 DPad_Left;
	Sint16 DPad_Right;
	Sint16 Button_A;
	Sint16 Button_B;
	Sint16 Button_X;
	Sint16 Button_Y;
	Sint16 Button_Start;
	Sint16 Button_LeftShoulder;
	Sint16 Button_RightShoulder;
	Sint16 Button_Back;
	Sint16 Button_LeftStick;
	Sint16 Button_RightStick;
};

struct DemoControllerData
{
	int HeldButtons;
	__int16 LTrigger;
	__int16 RTrigger;
	__int16 StickX;
	__int16 StickY;
	int NotHeldButtons;
	int PressedButtons;
	int ReleasedButtons;
};

namespace input
{
	void poll_controllers();
	void WriteAnalogs_hook();
	void InitRawControllers_hook();
	void __cdecl EnableController_r(Uint8 index);
	void __cdecl DisableController_r(Uint8 index);

	extern ControllerData raw_input[GAMEPAD_COUNT];
	extern bool controller_enabled[GAMEPAD_COUNT];
	extern bool debug;
	extern bool disable_mouse;
	extern bool e_held;
	extern bool demo;
	extern KeyboardMapping keys;
}
