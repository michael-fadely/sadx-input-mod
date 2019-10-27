#pragma once

#include "DreamPad.h"
#include <SA2Structs.h>

namespace input
{
	void poll_controllers();
	void WriteAnalogs_c();
	void InitRawControllers_hook();
	void __cdecl EnableController_r(Uint8 index);
	void __cdecl DisableController_r(Uint8 index);

	extern PDS_PERIPHERAL raw_input[GAMEPAD_COUNT];
	extern bool controller_enabled[GAMEPAD_COUNT];
	extern bool debug;
}
