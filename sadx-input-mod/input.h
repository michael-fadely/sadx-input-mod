#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace input
{
	void PollControllers();
	void WriteAnalogs_Hook();
	void RedirectRawControllers_Hook();
	void __cdecl EnableController_hook(Uint8 index);
	void __cdecl DisableController_hook(Uint8 index);

	extern ControllerData RawInput[GAMEPAD_COUNT];
	extern bool _ControllerEnabled[GAMEPAD_COUNT];
	extern bool debug;
}
