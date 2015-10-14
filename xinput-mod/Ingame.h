#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace xinput
{
	// Ingame functions
	void UpdateControllersXInput();
	void WriteAnalogsWrapper();
	void __cdecl RumbleLarge(int playerNumber, int magnitude);
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4);
	void Rumble(ushort id, int magnitude, Motor motor);
}
