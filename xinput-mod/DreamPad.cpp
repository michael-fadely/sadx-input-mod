#include "SDL.h"
#include "Convert.h"
#include <limits.h>

#include "DreamPad.h"

DreamPad::DreamPad() : gamepad(nullptr), haptic(nullptr), effect({}), effect_id(-1),
	rumbleTime_L(0), rumbleTime_S(0), rumbleState(Motor::None), pad({})
{
	effect.type = SDL_HAPTIC_SINE;
	effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
	effect.leftright.large_magnitude = 1;
	effect.leftright.small_magnitude = 1;
	effect.leftright.length = 1;
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

	pad.Support = (PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD
		| PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY | PDD_DEV_SUPPORT_TZ | PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
		| PDD_DEV_SUPPORT_ST | PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
		| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2);

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(this->gamepad);

	if (joystick == nullptr)
		return isConnected = false;

	this->haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
		return isConnected = true;

	if (SDL_HapticRumbleSupported(haptic))
	{
		effect_id = SDL_HapticNewEffect(haptic, &effect);
		/*
		if (SDL_HapticRumbleInit(haptic) != 0)
		{
			//PrintDebug("Haptic Rumble Init: %s\n", SDL_GetError());
			SDL_HapticClose(haptic);
			haptic = nullptr;
		}
		else
		{
			}
		*/
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

	xinput::ConvertAxes(1.0f, &pad.LeftStickX, axis);

	axis[0] = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
	axis[1] = -SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

	xinput::ConvertAxes(1.0f, &pad.RightStickX, axis);

	pad.LTriggerPressure = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	pad.RTriggerPressure = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

	int buttons = 0;

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

	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		buttons |= Buttons_C;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK))
		buttons |= Buttons_D;
	if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		buttons |= Buttons_Z;

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

	static const float f = 1.0f;

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

Motor DreamPad::GetActiveMotor() const
{
	return rumbleState;
}

void DreamPad::Copy(ControllerData& dest) const
{
	dest = pad;
}

DreamPad::~DreamPad()
{
	Close();
}
