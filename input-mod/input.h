#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace input
{
	// Ingame functions
	void PollControllers();
	void WriteAnalogs_Hook();
	void Rumble_Load(Uint32 port, Uint32 time, Motor motor);
	void __cdecl RumbleA(Uint32 port, Uint32 time);
	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4);
	void RedirectRawControllers_Hook();

	extern ControllerData RawInput[GAMEPAD_COUNT];
	extern bool _ControllerEnabled[GAMEPAD_COUNT];
	extern bool debug;
}
