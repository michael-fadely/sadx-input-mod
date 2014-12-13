#pragma once

namespace xinput
{
	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleA(int a1, signed int a2);
	void __cdecl RumbleB(int a1, signed int a2, signed int a3, int a4);
	void __cdecl Rumble(int a1);
	namespace deadzone
	{
		extern short stickL[4];
		extern short stickR[4];
		extern short triggers[4];
	}
}