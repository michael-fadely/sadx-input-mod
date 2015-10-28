#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace input
{
	// Ingame functions
	void PollControllers();
	void WriteAnalogs_Hook();
	void Rumble(ushort id, int magnitude, Motor motor);
	void __cdecl RumbleLarge(int playerNumber, int magnitude);
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4);
	void RedirectRawControllers_Hook();

	extern ControllerData RawInput[GAMEPAD_COUNT];
	extern bool _ControllerEnabled[GAMEPAD_COUNT];
}
