#include "sdlhack.h"
#include <cmath>
#include <limits>
#include <utility>
#include <algorithm>
#include "DreamPad.h"

using std::min;
using std::max;
using std::clamp;

static const uint32_t PAD_SUPPORT =
	PDD_DEV_SUPPORT_TA | PDD_DEV_SUPPORT_TB | PDD_DEV_SUPPORT_TX | PDD_DEV_SUPPORT_TY | PDD_DEV_SUPPORT_ST
	#ifdef EXTENDED_BUTTONS
	| PDD_DEV_SUPPORT_TC | PDD_DEV_SUPPORT_TD | PDD_DEV_SUPPORT_TZ
	#endif
	| PDD_DEV_SUPPORT_AR | PDD_DEV_SUPPORT_AL
	| PDD_DEV_SUPPORT_KU | PDD_DEV_SUPPORT_KD | PDD_DEV_SUPPORT_KL | PDD_DEV_SUPPORT_KR
	| PDD_DEV_SUPPORT_AX1 | PDD_DEV_SUPPORT_AY1 | PDD_DEV_SUPPORT_AX2 | PDD_DEV_SUPPORT_AY2;

DreamPad DreamPad::controllers[GAMEPAD_COUNT];

inline int digital_trigger(const ushort trigger, const ushort threshold, const int button)
{
	return trigger > threshold ? button : 0;
}

DreamPad::DreamPad(DreamPad&& other) noexcept
{
	move_from(std::move(other));
}

DreamPad::~DreamPad()
{
	close();
}

DreamPad& DreamPad::operator=(DreamPad&& other) noexcept
{
	move_from(std::move(other));
	return *this;
}

bool DreamPad::open(int id)
{
	if (connected_)
	{
		close();
	}

	gamepad = SDL_GameControllerOpen(id);

	if (gamepad == nullptr)
	{
		connected_ = false;
		return false;
	}

	dc_pad.Support = PAD_SUPPORT;

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);

	if (joystick == nullptr)
	{
		connected_ = false;
		return false;
	}

	controller_id_ = id;
	haptic = SDL_HapticOpenFromJoystick(joystick);

	if (haptic == nullptr)
	{
		connected_ = true;
		return true;
	}

	if (SDL_HapticRumbleSupported(haptic))
	{
		// TODO: Properly detect supported rumble types
		effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
		effect_id = SDL_HapticNewEffect(haptic, &effect);
	}
	else
	{
		SDL_HapticClose(haptic);
		haptic = nullptr;
	}

	connected_ = true;
	return true;
}

void DreamPad::close()
{
	if (!connected_)
	{
		return;
	}

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

	controller_id_ = -1;
	connected_ = false;
}

void DreamPad::poll()
{
	if (!connected_ && !settings.allow_keyboard)
	{
		return;
	}

	if (!connected_)
	{
		normalized_l_     = 0.0f;
		dc_pad.LeftStickX = 0;
		dc_pad.LeftStickY = 0;
	}
	else
	{
		Point2I axis = {
			axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX),
			axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY)
		};

		normalized_l_ = convert_axes(reinterpret_cast<Point2I*>(&dc_pad.LeftStickX), axis, settings.deadzone_l, settings.radial_l);
	}

	if (!connected_)
	{
		normalized_r_      = 0.0f;
		dc_pad.RightStickX = 0;
		dc_pad.RightStickY = 0;
	}
	else
	{
		Point2I axis = {
			axis.x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTX),
			axis.y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_RIGHTY)
		};

		normalized_r_ = convert_axes(reinterpret_cast<Point2I*>(&dc_pad.RightStickX), axis, settings.deadzone_r, settings.radial_r);
	}

	constexpr short short_max = std::numeric_limits<short>::max();

	if (!connected_)
	{
		dc_pad.LTriggerPressure = 0;
	}
	else
	{
		auto lt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		dc_pad.LTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(lt) / static_cast<float>(short_max)));
	}

	if (!connected_)
	{
		dc_pad.RTriggerPressure = 0;
	}
	else
	{
		auto rt = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		dc_pad.RTriggerPressure = static_cast<short>(255.0f * (static_cast<float>(rt) / static_cast<float>(short_max)));
	}

	Uint32 buttons = 0;

	buttons |= digital_trigger(dc_pad.LTriggerPressure, settings.trigger_threshold, PDD_DGT_TL);
	buttons |= digital_trigger(dc_pad.RTriggerPressure, settings.trigger_threshold, PDD_DGT_TR);

	if (connected_)
	{
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A))
		{
			buttons |= PDD_DGT_TA;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_B))
		{
			buttons |= PDD_DGT_TB;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_X))
		{
			buttons |= PDD_DGT_TX;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_Y))
		{
			buttons |= PDD_DGT_TY;
		}

		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_START))
		{
			buttons |= PDD_DGT_ST;
		}

#ifdef EXTENDED_BUTTONS
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		{
			buttons |= PDD_DGT_TC;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK))
		{
			buttons |= PDD_DGT_TD;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		{
			buttons |= PDD_DGT_TZ;
		}
#endif

		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
		{
			buttons |= PDD_DGT_KU;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
		{
			buttons |= PDD_DGT_KD;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
		{
			buttons |= PDD_DGT_KL;
		}
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
		{
			buttons |= PDD_DGT_KR;
		}
	}

	update_buttons(dc_pad, buttons);
}

Motor DreamPad::active_motor() const
{
	return rumble_state;
}

bool DreamPad::connected() const
{
	return connected_;
}

int DreamPad::controller_id() const
{
	return controller_id_;
}

float DreamPad::normalized_l() const
{
	return normalized_l_;
}

float DreamPad::normalized_r() const
{
	return normalized_r_;
}

const DCControllerData& DreamPad::dreamcast_data() const
{
	return dc_pad;
}

void DreamPad::set_active_motor(Motor motor, bool enable)
{
	if (effect_id == -1 || haptic == nullptr)
	{
		return;
	}

	if (settings.mega_rumble)
	{
		motor = Motor::both;
	}

	const float f = settings.rumble_factor;

	if (motor & Motor::large)
	{
		effect.leftright.large_magnitude = enable ? static_cast<ushort>(std::numeric_limits<ushort>::max() * f) : 0;
		rumble_state = Motor::_from_integral(enable ? (rumble_state | motor) : (rumble_state & ~Motor::large));
	}

	if (motor & Motor::small)
	{
		effect.leftright.small_magnitude = enable ? static_cast<ushort>(std::numeric_limits<ushort>::max() * f) : 0;
		rumble_state = Motor::_from_integral(enable ? (rumble_state | motor) : (rumble_state & ~Motor::small));
	}

	SDL_HapticUpdateEffect(haptic, effect_id, &effect);
	SDL_HapticRunEffect(haptic, effect_id, 1);
}

float DreamPad::convert_axes(Point2I* dest, const Point2I& source, short deadzone, bool radial)
{
	if (abs(source.x) < deadzone && abs(source.y) < deadzone)
	{
		*dest = {};
		return 0.0f;
	}

	constexpr short short_max = std::numeric_limits<short>::max();

	const auto x = static_cast<float>(std::clamp<short>(source.x, -short_max, short_max));
	const auto y = static_cast<float>(std::clamp<short>(source.y, -short_max, short_max));

	const float m = std::sqrt(x * x + y * y);

	const float nx = (m < static_cast<float>(deadzone)) ? 0 : (x / m); // Normalized (X)
	const float ny = (m < static_cast<float>(deadzone)) ? 0 : (y / m); // Normalized (Y)
	const float n  = (min(static_cast<float>(short_max), m) - static_cast<float>(deadzone)) / static_cast<float>(short_max - deadzone);

	if (!radial && abs(source.x) < deadzone)
	{
		dest->x = 0;
	}
	else
	{
		dest->x = clamp<short>(static_cast<short>(128 * (nx * n)), -127, 127);
	}

	if (!radial && abs(source.y) < deadzone)
	{
		dest->y = 0;
	}
	else
	{
		dest->y = clamp<short>(static_cast<short>(128 * (ny * n)), -127, 127);
	}

	return n;
}

void DreamPad::update_buttons(DCControllerData& pad, Uint32 buttons)
{
	pad.HeldButtons     = buttons;
	pad.NotHeldButtons  = ~buttons;
	pad.ReleasedButtons = pad.Old & (buttons ^ pad.Old);
	pad.PressedButtons  = buttons & (buttons ^ pad.Old);
	pad.Old             = pad.HeldButtons;
}

void DreamPad::move_from(DreamPad&& other)
{
	gamepad        = other.gamepad;
	haptic         = other.haptic;
	effect         = other.effect;
	controller_id_ = other.controller_id_;
	effect_id      = other.effect_id;
	connected_     = other.connected_;
	rumble_state   = other.rumble_state;
	normalized_l_  = other.normalized_l_;
	normalized_r_  = other.normalized_r_;
	settings       = other.settings;

	other.gamepad        = nullptr;
	other.haptic         = nullptr;
	other.controller_id_ = -1;
	other.effect_id      = -1;
	other.connected_     = false;
}

DreamPad::Settings::Settings()
{
	allow_keyboard    = false;
	deadzone_l        = GAMEPAD_LEFT_THUMB_DEADZONE;
	deadzone_r        = GAMEPAD_RIGHT_THUMB_DEADZONE;
	trigger_threshold = GAMEPAD_TRIGGER_THRESHOLD;
	radial_l          = true;
	radial_r          = false;
	rumble_factor     = 1.0f;
	mega_rumble       = false;
	rumble_min_time   = 0;
}

void DreamPad::Settings::set_deadzone_l(const short deadzone)
{
	this->deadzone_l = clamp<short>(deadzone, 0, std::numeric_limits<short>::max());
}

void DreamPad::Settings::set_deadzone_r(const short deadzone)
{
	this->deadzone_r = clamp<short>(deadzone, 0, std::numeric_limits<short>::max());
}
