#include "stdafx.h"
#include "SDL.h"
#include <limits.h>
#include <Windows.h>
#include "minmax.h"

#include "DreamPad.h"

DreamPad DreamPad::Controllers[GAMEPAD_COUNT];

DreamPad::DreamPad() : controller_id(-1), gamepad(nullptr), haptic(nullptr), effect({}), effect_id(-1),
	rumbleStart_L(0), rumbleDuration_L(0), rumbleStart_S(0), rumbleDuration_S(0), rumble_state(Motor::None),
	pad({}), normalized_L(0.0f), normalized_R(0.0f)
{
	// TODO: Properly detect supported rumble types
	effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
}
DreamPad::~DreamPad()
{
	Close();
}

bool DreamPad::Open(int id)
{
	if (connected)
		Close();

	gamepad = SDL_GameControllerOpen(id);

	if (gamepad == nullptr)
		return connected = false;

	pad.Support = (PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY | PDD_DEV_SUPPORT_ST
#ifdef EXTENDED_BUTTONS
		| PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD | PDD_DEV_SUPPORT_TZ
#endif
		| PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
		| PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
		| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2);

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);

	if (joystick == nullptr)
		return connected = false;

	controller_id = id;
	haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
		return connected = true;

	if (SDL_HapticRumbleSupported(haptic))
	{
		effect_id = SDL_HapticNewEffect(haptic, &effect);
	}
	else
	{
		SDL_HapticClose(haptic);
		haptic = nullptr;
	}

	return connected = true;
}

void DreamPad::Close()
{
	if (!connected)
		return;

	if (haptic != nullptr)
	{
		SDL_HapticDestroyEffect(haptic, effect_id);
		SDL_HapticClose(haptic);

		effect_id = -1;
		haptic = nullptr;
	}

	if (gamepad != nullptr)
	{
		SDL_GameControllerClose(gamepad);
		gamepad = nullptr;
	}

	controller_id = -1;
	connected = false;
}

void DreamPad::Poll()
{
	if (!connected)
		return;

	SDL_GameControllerUpdate();

	short axis[2];

	axis[0] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
	axis[1] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);

	normalized_L = ConvertAxes(&pad.LeftStickX, axis, settings.deadzoneL, settings.radialL);

	axis[0] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
	axis[1] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

	normalized_R = ConvertAxes(&pad.RightStickX, axis, settings.deadzoneR, settings.radialR);

	short lt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	short rt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

	pad.LTriggerPressure = (short)(255.0f * ((float)lt / (float)SHRT_MAX));
	pad.RTriggerPressure = (short)(255.0f * ((float)rt / (float)SHRT_MAX));;

	int buttons = 0;

	buttons |= DigitalTrigger(pad.LTriggerPressure, settings.triggerThreshold, Buttons_L);
	buttons |= DigitalTrigger(pad.RTriggerPressure, settings.triggerThreshold, Buttons_R);

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A))
		buttons |= Buttons_A;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_B))
		buttons |= Buttons_B;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_X))
		buttons |= Buttons_X;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_Y))
		buttons |= Buttons_Y;

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_START))
		buttons |= Buttons_Start;

#ifdef EXTENDED_BUTTONS
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		buttons |= Buttons_C;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK))
		buttons |= Buttons_D;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		buttons |= Buttons_Z;
#endif

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
		buttons |= Buttons_Up;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
		buttons |= Buttons_Down;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
		buttons |= Buttons_Left;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
		buttons |= Buttons_Right;

	pad.HeldButtons		= buttons;
	pad.NotHeldButtons	= ~buttons;
	pad.ReleasedButtons	= pad.Old & (buttons ^ pad.Old);
	pad.PressedButtons	= buttons & (buttons ^ pad.Old);
	pad.Old				= pad.HeldButtons;

	UpdateMotor();
}

void DreamPad::UpdateMotor()
{
	if (effect_id == -1 || haptic == nullptr || rumble_state == Motor::None)
		return;

	Motor turn_off = Motor::None;
	uint now = GetTickCount();

	if (now - rumbleStart_L >= rumbleDuration_L)
		turn_off = (Motor)(turn_off | Motor::Large);

	if (now - rumbleStart_S >= rumbleDuration_S)
		turn_off = (Motor)(turn_off | Motor::Small);

	if (turn_off != Motor::None)
		SetActiveMotor(turn_off, 0);
}

void DreamPad::SetActiveMotor(Motor motor, Uint32 time)
{
	if (effect_id == -1 || haptic == nullptr)
		return;

	const float f = settings.rumbleFactor;
	const bool disable = time < 1;

	if (motor & Motor::Large)
	{
		effect.leftright.large_magnitude = !disable ? (ushort)(USHRT_MAX * f) : 0;
		rumbleStart_L = GetTickCount();
		rumbleDuration_L = (uint)((1000.0 / 60.0) * time);
		rumble_state = (Motor)(!disable ? rumble_state | motor : rumble_state & ~Motor::Large);
	}

	if (motor & Motor::Small)
	{
		effect.leftright.small_magnitude = !disable ? (ushort)(USHRT_MAX * f) : 0;
		rumbleStart_S = GetTickCount();
		rumbleDuration_S = (uint)((1000.0 / 60.0) * time);
		rumble_state = (Motor)(!disable ? rumble_state | motor : rumble_state & ~Motor::Small);
	}

	SDL_HapticUpdateEffect(haptic, effect_id, &effect);
	SDL_HapticRunEffect(haptic, effect_id, 1);
}

void DreamPad::Copy(ControllerData& dest) const
{
	dest = pad;
}

inline int DreamPad::DigitalTrigger(ushort trigger, ushort threshold, int button)
{
	return (trigger > threshold) ? button : 0;
}

float DreamPad::ConvertAxes(short* dest, short* source, short deadzone, bool radial) const
{
	if (abs(source[0]) < deadzone && abs(source[1]) < deadzone)
	{
		dest[0] = dest[1] = 0;
		return 0.0f;
	}

	// This is being intentionally limited to -32767 instead of -32768
	const float x = (float)clamp(source[0], (short)-SHRT_MAX, (short)SHRT_MAX);
	const float y = (float)-clamp(source[1], (short)-SHRT_MAX, (short)SHRT_MAX);

	const float m = sqrt(x * x + y * y);

	const float nx	= (m < deadzone) ? 0 : x / m;	// Normalized (X)
	const float ny	= (m < deadzone) ? 0 : y / m;	// Normalized (Y)
	const float n	= (min((float)SHRT_MAX, m) - deadzone) / ((float)SHRT_MAX - deadzone);

	// In my testing, multiplying -128 to 128 results in 127 instead, which is the desired value.
	dest[0] = (radial || abs(source[0]) >= deadzone) ? (short)clamp((short)(128 * (nx * n)), (short)-127, (short)127) : 0;
	dest[1] = (radial || abs(source[1]) >= deadzone) ? (short)clamp((short)(-128 * (ny * n)), (short)-127, (short)127) : 0;

	return n;
}

void DreamPad::ProcessEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			default:
				break;

			case SDL_CONTROLLERDEVICEADDED:
			{
				int which = event.cdevice.which;
				for (uint i = 0; i < GAMEPAD_COUNT; i++)
				{
					// Checking for both in cases like the DualShock 4 and DS4Windows where the controller might be "connected"
					// twice with the same ID. DreamPad::Open automatically closes if already open.
					if (!Controllers[i].Connected() || Controllers[i].ControllerID() == which)
					{
						Controllers[i].Open(which);
						break;
					}
				}
				break;
			}

			// TODO: Zero out raw input structure on disconnect.
			case SDL_CONTROLLERDEVICEREMOVED:
			{
				int which = event.cdevice.which;
				for (uint i = 0; i < GAMEPAD_COUNT; i++)
				{
					if (Controllers[i].ControllerID() == which)
					{
						Controllers[i].Close();
						break;
					}
				}
				break;
			}
		}
	}
}


DreamPad::Settings::Settings()
{
	deadzoneL			= GAMEPAD_LEFT_THUMB_DEADZONE;
	deadzoneR			= GAMEPAD_RIGHT_THUMB_DEADZONE;
	triggerThreshold	= GAMEPAD_TRIGGER_THRESHOLD;
	radialL				= true;
	radialR				= false;
	rumbleFactor		= 1.0f;
}
void DreamPad::Settings::apply(short deadzoneL, short deadzoneR, bool radialL, bool radialR, uint8 triggerThreshold, float rumbleFactor)
{
	this->deadzoneL			= clamp(deadzoneL, (short)0, (short)SHRT_MAX);
	this->deadzoneR			= clamp(deadzoneR, (short)0, (short)SHRT_MAX);
	this->radialL			= radialL;
	this->radialR			= radialR;
	this->triggerThreshold	= triggerThreshold;
	this->rumbleFactor		= rumbleFactor;
}
