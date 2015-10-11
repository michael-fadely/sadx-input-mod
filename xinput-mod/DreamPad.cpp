#include "stdafx.h"
#include "SDL.h"
#include "Convert.h"
#include <limits.h>
#include <Windows.h>
#include <XInput.h> // for deadzone stuff
#include "minmax.h"

#include "DreamPad.h"

DreamPad DreamPad::Controllers[PAD_COUNT];

DreamPad::DreamPad() : controller_id(-1), gamepad(nullptr), haptic(nullptr), effect({}),
	effect_id(-1), rumbleTime_L(0), rumbleTime_S(0), rumbleState(Motor::None), pad({})
{
	effect.type = SDL_HAPTIC_SINE;
	effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
	effect.leftright.length = SDL_HAPTIC_INFINITY;
}
DreamPad::~DreamPad()
{
	Close();
}

/// <summary>
/// Opens the specified controller ID.
/// </summary>
/// <param name="id">Controller ID to open.</param>
/// <returns><c>true</c> on success. Note that haptic can fail and the function will still return true.</returns>
bool DreamPad::Open(int id)
{
	if (isConnected)
		Close();

	this->gamepad = SDL_GameControllerOpen(id);

	if (gamepad == nullptr)
		return isConnected = false;

	pad.Support = (PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY
#ifdef EXTENDED_BUTTONS
		| PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD | PDD_DEV_SUPPORT_TZ
#endif
		| PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
		| PDD_DEV_SUPPORT_ST | PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
		| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2);

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(this->gamepad);

	if (joystick == nullptr)
		return isConnected = false;

	controller_id = id;
	this->haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
		return isConnected = true;

	if (SDL_HapticRumbleSupported(haptic))
	{
		effect_id = SDL_HapticNewEffect(haptic, &effect);
	}
	else
	{
		SDL_HapticClose(haptic);
		haptic = nullptr;
	}

	return isConnected = true;
}

/// <summary>
/// Closes this instance.
/// </summary>
void DreamPad::Close()
{
	if (!isConnected)
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
	isConnected = false;
}

/// <summary>
/// Calls Poll and UpdateMotor automatically.
/// </summary>
void DreamPad::Update()
{
	Poll();
	UpdateMotor();
}

/// <summary>
/// Polls input for this instance.
/// </summary>
void DreamPad::Poll()
{
	if (!isConnected)
		return;

	SDL_GameControllerUpdate();

	short axis[2];

	axis[0] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
	axis[1] = -SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);

	xinput::ConvertAxes(settings.scaleFactor, &pad.LeftStickX, axis, settings.deadzoneL, settings.radialL);

	axis[0] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
	axis[1] = -SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

	xinput::ConvertAxes(settings.scaleFactor, &pad.RightStickX, axis, settings.deadzoneR, settings.radialR);

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
}

void DreamPad::UpdateMotor()
{
	if (effect_id == -1 || haptic == nullptr || rumbleState == Motor::None)
		return;

	Motor turn_off = Motor::None;
	uint now = GetTickCount();

	if (now - rumbleTime_L >= 250)
		turn_off = (Motor)(turn_off | Motor::Large);

	if (now - rumbleTime_S >= 1000)
		turn_off = (Motor)(turn_off | Motor::Small);

	if (turn_off != Motor::None)
		SetActiveMotor(turn_off, 0);
}

void DreamPad::SetActiveMotor(Motor motor, short magnitude)
{
	if (effect_id == -1 || haptic == nullptr)
		return;

	const float f = settings.rumbleFactor;

	if (motor & Motor::Large)
	{
		effect.leftright.large_magnitude = (short)min(SHRT_MAX, (int)(magnitude * f));
		rumbleTime_L = GetTickCount();
		rumbleState = (Motor)((magnitude > 0) ? rumbleState | motor : rumbleState & ~Motor::Large);
	}

	if (motor & Motor::Small)
	{
		effect.leftright.small_magnitude = (short)min(SHRT_MAX, (int)(magnitude * (2 + f)));
		rumbleTime_S = GetTickCount();
		rumbleState = (Motor)((magnitude > 0) ? rumbleState | motor : rumbleState & ~Motor::Small);
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


DreamPad::Settings::Settings()
{
	deadzoneL = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	deadzoneR = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	triggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	radialL = true;
	radialR = false;
	rumbleFactor = 1.0f;
	scaleFactor = 1.5f;
}
void DreamPad::Settings::apply(short deadzoneL, short deadzoneR, bool radialL, bool radialR, uint8 triggerThreshold, float rumbleFactor, float scaleFactor)
{
	this->deadzoneL = clamp(deadzoneL, (short)0, (short)SHRT_MAX);
	this->deadzoneR = clamp(deadzoneR, (short)0, (short)SHRT_MAX);
	this->radialL = radialL;
	this->radialR = radialR;
	this->triggerThreshold = min((uint8)UCHAR_MAX, triggerThreshold);
	this->rumbleFactor = rumbleFactor;
	this->scaleFactor = max(1.0f, scaleFactor);
}
