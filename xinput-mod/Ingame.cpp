// Other crap
#include <SADXModLoader.h>
#include "Common.h"

// This namespace
#include "Motor.h"
#include "Ingame.h"
#include "Utility.h"

DataPointer(int, isCutscenePlaying, 0x3B2A2E4);		// Fun fact: Freeze at 0 to avoid cutscenes. 4 bytes from here is the cutscene to play.
DataPointer(char, rumbleEnabled, 0x00913B10);		// Not sure why this is a char and ^ is an int.
DataArray(bool, Controller_Enabled, 0x00909FB4, 4);	// TODO: Figure out what toggles this for P2.

namespace xinput
{
	Settings::Settings()
	{
		deadzoneL			= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		deadzoneR			= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		triggerThreshold	= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		radialL				= true;
		radialR				= false;
		rumbleFactor		= 1.0f;
		scaleFactor			= 1.5f;
	}
	void Settings::apply(short deadzoneL, short deadzoneR, bool radialL, bool radialR, uint8 triggerThreshold, float rumbleFactor, float scaleFactor)
	{
		this->deadzoneL			= clamp(deadzoneL, (short)0, (short)SHRT_MAX);
		this->deadzoneR			= clamp(deadzoneR, (short)0, (short)SHRT_MAX);
		this->radialL			= radialL;
		this->radialR			= radialR;
		this->triggerThreshold	= min((uint8)UCHAR_MAX, triggerThreshold);
		this->rumbleFactor		= rumbleFactor;
		this->scaleFactor		= max(1.0f, scaleFactor);
	}

	Settings settings[XPAD_COUNT];

	static inline int DigitalTrigger(uint8 trigger, uint8 threshold, int button)
	{
		return (trigger > threshold) ? button : 0;
	}

#pragma region Ingame Functions

	// TODO: Keyboard & Mouse
	void __cdecl UpdateControllersXInput()
	{
		for (ushort i = 0; i < XPAD_COUNT; i++)
		{
			ControllerData* pad = &ControllersRaw[i];
			XINPUT_STATE state = {};
			XInputGetState(i, &state);
			XINPUT_GAMEPAD* xpad = &state.Gamepad;

			// Gotta get that enum set up for this.
			pad->Support = 0x3F07FEu;

			// Analog scale factor
			const float scale = settings[i].scaleFactor;

			// L Analog
			ConvertAxes(scale, (short*)&pad->LeftStickX, (short*)&xpad->sThumbLX,
				settings[i].deadzoneL, settings[i].radialL);

			// R Analog
			ConvertAxes(scale, (short*)&pad->RightStickX, (short*)&xpad->sThumbRX,
				settings[i].deadzoneR, settings[i].radialR);

			// Trigger pressure
			pad->LTriggerPressure = xpad->bLeftTrigger;
			pad->RTriggerPressure = xpad->bRightTrigger;

			// Now, get the new buttons from the XInput xpad
			pad->HeldButtons = ConvertButtons(xpad->wButtons);
			pad->HeldButtons |= DigitalTrigger(xpad->bLeftTrigger, settings[i].triggerThreshold, Buttons_L)
				| DigitalTrigger(xpad->bRightTrigger, settings[i].triggerThreshold, Buttons_R);

			pad->NotHeldButtons = ~pad->HeldButtons;

			// Now set the released buttons to the difference between
			// the last and currently held buttons
			pad->ReleasedButtons = pad->Old & (pad->HeldButtons ^ pad->Old);

			// Do some fancy math to "press" only the necessary buttons
			pad->PressedButtons = pad->HeldButtons & (pad->HeldButtons ^ pad->Old);

			// Set the "last held" to held
			pad->Old = pad->HeldButtons;

			UpdateMotor(i);

#ifdef _DEBUG
			if (xpad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
			{
				Motor m = GetActiveMotor(i);

				DisplayDebugStringFormatted(8 + (3 * i), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
					pad->HeldButtons, pad->LTriggerPressure, pad->RTriggerPressure, (m & Motor::Left), (m & Motor::Right) >> 1);
				DisplayDebugStringFormatted(9 + (3 * i), "   LS: %04d/%04d RS: %04d/%04d",
					pad->LeftStickX, pad->LeftStickY, pad->RightStickX, pad->RightStickY);

				if (i == 0 && xpad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
				{
					if (pad->PressedButtons & Buttons_Up)
						settings[i].rumbleFactor += 0.125f;
					else if (pad->PressedButtons & Buttons_Down)
						settings[i].rumbleFactor -= 0.125f;

					DisplayDebugStringFormatted(6, "Rumble factor (U/D): %f", settings[i].rumbleFactor);
				}
			}
#endif
		}
	}

	void Rumble(ushort id, int magnitude, Motor motor)
	{
		if (id >= XPAD_COUNT)
		{
			for (ushort i = 0; i < XPAD_COUNT; i++)
				Rumble(id, magnitude, motor);

			return;
		}

		short scaled = 4 * magnitude;

		if (magnitude > 0)
		{
			float m;

			// RumbleLarge only ever passes in a value in that is <= 10,
			// and scaling that to 2 bytes is super annoying, so here's
			// some arbitrary values to increase the intensity.
			if (magnitude >= 1 && magnitude <= 10)
				m = max(0.2375f, scaled / 25.0f);
			else
				m = (float)scaled / 256.0f;

			scaled = (short)(SHRT_MAX * clamp(m, 0.0f, 1.0f));
		}

		if (Controller_Enabled[id] || scaled == 0)
			SetActiveMotor(id, motor, scaled);
	}
	void __cdecl RumbleLarge(int playerNumber, int magnitude)
	{
		if (!isCutscenePlaying && rumbleEnabled)
			Rumble(playerNumber, clamp(magnitude, 1, 255), Motor::Left);
	}
	void __cdecl RumbleSmall(int playerNumber, int a2, int a3, int a4)
	{
		if (!isCutscenePlaying && rumbleEnabled)
		{
			int _a2 = clamp(a2, -4, 4);

			if (_a2 == 1)
				_a2 = 2;
			else if (_a2 == -1)
				_a2 = -2;

			int _a3 = clamp(a3, 7, 59);

			Rumble(playerNumber, max(1, a4 * _a3 / (4 * _a2)), Motor::Right);
		}
	}

#pragma endregion

}
