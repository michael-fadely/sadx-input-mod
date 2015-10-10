#pragma once

#include "typedefs.h"
#include "DreamPad.h"

namespace xinput
{

	struct Settings
	{
		Settings();

		short	deadzoneL;			// Left stick deadzone
		bool	radialL;			// Indicates if the stick is fully radial or semi-radial
		short	deadzoneR;			// Right stick deadzone
		bool	radialR;			// Indicates if the stick is fully radial or semi-radial
		uint8	triggerThreshold;	// Trigger threshold
		float	rumbleFactor;		// Rumble intensity multiplier (1.0 by default)
		float	scaleFactor;		// Axis scale factor (1.5 (192) by default)

		void apply(short deadzoneL, short deadzoneR,
			bool radialL, bool radialR, uint8 triggerThreshold,
			float rumbleFactor, float scaleFactor);
	};

	extern Settings settings[PAD_COUNT];

	// Ingame functions
	void __cdecl UpdateControllersXInput();
	void __cdecl RumbleLarge(int playerNumber, int magnitude);
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4);
	void Rumble(ushort id, int magnitude, Motor motor);
}
