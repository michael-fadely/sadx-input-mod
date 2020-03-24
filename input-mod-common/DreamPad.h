#pragma once

// This class is based loosely on this gist: https://gist.github.com/urkle/6701236
// That code doesn't actually compile due to name discrepancies, but it was a good starting point.

// XInput default deadzones blatantly copied, pasted, and renamed.
constexpr auto GAMEPAD_LEFT_THUMB_DEADZONE  = 7849;
constexpr auto GAMEPAD_RIGHT_THUMB_DEADZONE = 8689;
constexpr auto GAMEPAD_TRIGGER_THRESHOLD    = 30;
// Limit is now 8, the maximum supported by the game (depending on where you look; sometimes it's 4). XInput is no longer a limiting factor.
constexpr auto GAMEPAD_COUNT = 8;

#include "sdlhack.h"
#include "typedefs.h"
#include "types.h"
#include "../better-enums/enum.h"

BETTER_ENUM(Motor, int8,
            none,
            large = 1 << 0,
            small = 1 << 1,
            both)

class DreamPad
{
	DCControllerData dc_pad = {};

	SDL_GameController* gamepad = nullptr;
	SDL_Haptic*         haptic  = nullptr;
	SDL_HapticEffect    effect  = {};

	int controller_id_ = -1;
	int effect_id      = -1;

	bool  connected_   = false;
	Motor rumble_state = Motor::none;

	float normalized_l_ = 0.0f;
	float normalized_r_ = 0.0f;

public:
	static const uint32_t pad_support;
	
	static DreamPad controllers[GAMEPAD_COUNT];

	DreamPad(DreamPad&& other) noexcept;
	DreamPad(const DreamPad& other) = delete;
	DreamPad() = default;
	~DreamPad();

	DreamPad& operator=(DreamPad&& other) noexcept;
	DreamPad& operator=(const DreamPad& other) = delete;

	/**
	 * \brief Opens the specified controller ID.
	 * \param id Controller ID to open.
	 * \return \a true on success. Note that haptic can fail and the function will still return true.
	 */
	bool open(int id);

	/**
	 * \brief Closes this instance.
	 */
	void close();

	/**
	 * \brief Polls input for this instance.
	 */
	void poll();

	Motor active_motor() const;
	void  set_active_motor(Motor motor, bool enable);
	bool  connected() const;
	int   controller_id() const;
	float normalized_l() const;
	float normalized_r() const;

	// SDL -> Dreamcast converted input data (DCControllerData).
	const DCControllerData& dreamcast_data() const;

	struct Settings
	{
		Settings();

		bool   allow_keyboard;
		short  deadzone_l;        // Left stick deadzone
		bool   radial_l;          // Indicates if the stick is fully radial or semi-radial
		short  deadzone_r;        // Right stick deadzone
		bool   radial_r;          // Indicates if the stick is fully radial or semi-radial
		uint8  trigger_threshold; // Trigger threshold
		float  rumble_factor;     // Rumble intensity multiplier (1.0 by default)
		bool   mega_rumble;       // Always fire both motors
		ushort rumble_min_time;   // Minimum rumble time for controllers that have issues

		void set_deadzone_l(short deadzone);
		void set_deadzone_r(short deadzone);
	} settings;

	/**
	 * \brief Converts from SDL (-32768 to 32767) to Dreamcast (-127 to 127) axes, including scaled deadzone.
	 * \param dest The destination axes (Dreamcast).
	 * \param source The source axes (SDL).
	 * \param deadzone The deadzone.
	 * \param radial If set to \a true, the deadzone is treated as fully radial. (i.e one axis exceeding deadzone implies the other)
	 * \return The axis magnitude.
	 */
	static float convert_axes(Point2I* dest, const Point2I& source, short deadzone, bool radial);

	/**
	 * \brief Updates the various button fields in the provided
	 * Dreamcast controller structure with the provided buttons.
	 * \param pad The Dreamcast controller data to update.
	 * \param buttons The buttons to add.
	 */
	static void update_buttons(DCControllerData& pad, Uint32 buttons);

private:
	void move_from(DreamPad&& other);
};
