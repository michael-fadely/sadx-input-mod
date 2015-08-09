#pragma once

#include "typedefs.h"
#include <Xinput.h>

// Re-defining so it can be changed easily, and because XUSER_MAX_COUNT is far too long.
#define XPAD_COUNT XUSER_MAX_COUNT

namespace xinput
{
	enum Motor : int8
	{
		None,
		Left,
		Right,
		Both
	};

	struct Settings
	{
		Settings();

		short	deadzoneL;			// Left stick deadzone
		bool	normalizeL;
		short	deadzoneR;			// Right stick deadzone
		bool	normalizeR;
		uint8	triggerThreshold;	// Trigger threshold
		float	rumbleFactor;		// Rumble intensity multiplier (1.0 by default)
		float	scaleFactor;		// Axis scale factor (1.5 (192) by default)

		void apply(short deadzoneL, short deadzoneR,
			bool normalizeL, bool normalizeR, uint8 triggerThreshold,
			float rumbleFactor, float scaleFactor);
	};

	extern Settings settings[XPAD_COUNT];

	// Ingame functions
	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleLarge(int playerNumber, int intensity);
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4);
	void Rumble(ushort id, int a1, Motor motor);

	// Utility functions
	// TODO: Separate utility functions into separate file.
	void ConvertAxes(ushort id, short dest[2], short source[2], short deadzone = 0, bool normalize = true);
	int ConvertButtons(ushort id, XINPUT_GAMEPAD* xpad);
	static void SetMotor(ushort id, Motor motor, short intensity);
}
