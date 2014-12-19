#pragma once

#include <G:\Libraries\LazyTypedefs.h>

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

	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleLarge(int playerNumber, signed int intensity);
	void __cdecl RumbleSmall(int a1, signed int a2, signed int a3, int a4);
	void Rumble(int a1, Motor motor);
}