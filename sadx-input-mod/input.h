#pragma once

#include "DreamPad.h"

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
	extern bool sadx_remapper;
}
