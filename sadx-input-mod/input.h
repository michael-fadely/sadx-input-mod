#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace input
{
	void poll_controllers();
	void WriteAnalogs_r();
	void InitRawControllers_hook();
	void __cdecl EnableController_hook(Uint8 index);
	void __cdecl DisableController_hook(Uint8 index);

	extern ControllerData raw_input[GAMEPAD_COUNT];
	extern bool controller_enabled[GAMEPAD_COUNT];
	extern bool debug;
}
