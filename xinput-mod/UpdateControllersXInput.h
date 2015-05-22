#pragma once

#include "typedefs.h"
#include <Xinput.h>

namespace xinput
{
	enum Motor : int8
	{
		None,
		Left,
		Right,
		Both
	};

	namespace deadzone
	{
		extern short stickL[4];
		extern short stickR[4];
		extern short triggers[4];
	}

	// Ingame functions
	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleLarge(int playerNumber, signed int intensity);
	void __cdecl RumbleSmall(int playerNumber, signed int a2, signed int a3, int a4);
	void Rumble(short id, int a1, Motor motor);

	// Utility functions
	// TODO: Pull these functions out of here for modularity.
	void ConvertAxes(short dest[2], short source[2], short deadzone, bool radial);
	int ConvertButtons(XINPUT_GAMEPAD* xpad, ushort id);
	static void SetMotor(short id, Motor motor, short intensity);
	void SetDeadzone(short* array, uint id, int value);
}