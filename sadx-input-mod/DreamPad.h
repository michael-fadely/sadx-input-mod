#pragma once

// This class is based loosely on this gist: https://gist.github.com/urkle/6701236
// That code doesn't actually compile due to name discrepancies, but it was a good starting point.

// XInput default deadzones blatantly copied, pasted, and renamed.
#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define GAMEPAD_TRIGGER_THRESHOLD    30
// Limit is now 8, the maximum supported by the game (depending on where you look; sometimes it's 4). XInput is no longer a limiting factor.
#define GAMEPAD_COUNT 8

#include "SDL.h"
#include <SADXModLoader.h>
#include "typedefs.h"

enum Motor : int8
{
	None,
	Large,
	Small,
	Both
};

class DreamPad
{
public:
	DreamPad();
	~DreamPad();

	/// <summary>
	/// Opens the specified controller ID.
	/// </summary>
	/// <param name="id">Controller ID to open.</param>
	/// <returns><c>true</c> on success. Note that haptic can fail and the function will still return true.</returns>
	bool Open(int id);
	/// <summary>
	/// Closes this instance.
	/// </summary>
	void Close();
	/// <summary>
	/// Polls input for this instance.
	/// </summary>
	void Poll();

	// Poor man's properties
	Motor GetActiveMotor() const { return rumble_state; }
	void SetActiveMotor(Motor motor, bool enable);
	bool Connected() const { return connected; }
	int ControllerID() const { return controller_id; }
	float NormalizedL() const { return normalized_L; }
	float NormalizedR() const { return normalized_R; }
	// SDL -> Dreamcast converted input data (ControllerData).
	const ControllerData& DreamcastData() const { return pad; }

	void Copy(ControllerData& dest) const;

	struct Settings
	{
		Settings();

		short	deadzoneL;			// Left stick deadzone
		bool	radialL;			// Indicates if the stick is fully radial or semi-radial
		short	deadzoneR;			// Right stick deadzone
		bool	radialR;			// Indicates if the stick is fully radial or semi-radial
		uint8	triggerThreshold;	// Trigger threshold
		float	rumbleFactor;		// Rumble intensity multiplier (1.0 by default)
		bool	megaRumble;			// Always fire both motors
		ushort	rumbleMinTime;		// Minimum rumble time for controllers that have issues

		void apply(short deadzoneL, short deadzoneR,
			bool radialL, bool radialR, uint8 triggerThreshold, float rumbleFactor, bool megaRumble, ushort rumbleMinTime);
	} settings;


	static int DigitalTrigger(ushort trigger, ushort threshold, int button);
	/// <summary>
	/// Converts from SDL (-32768 to 32767) to Dreamcast (-127 to 127) axes, including scaled deadzone.
	/// </summary>
	/// <param name="dest">The destination axes (Dreamcast).</param>
	/// <param name="source">The source axes (SDL).</param>
	/// <param name="deadzone">The deadzone.</param>
	/// <param name="radial">If set to <c>true</c>, the deadzone is treated as fully radial. (i.e one axis exceeding deadzone implies the other)</param>
	float ConvertAxes(short* dest, short* source, short deadzone, bool radial) const;

	/// <summary>
	/// Handles certain SDL events (such as controller connect and disconnect).
	/// </summary>
	static void ProcessEvents();
	static DreamPad Controllers[GAMEPAD_COUNT];

private:
	bool connected = false;
	int controller_id;
	SDL_GameController* gamepad;

	SDL_Haptic* haptic;
	SDL_HapticEffect effect;
	int effect_id;
	Motor rumble_state;

	ControllerData pad;
	float normalized_L, normalized_R;
};

enum PDD_DEV_SUPPORT : uint32_t
{
	//	Right stick Y
	PDD_DEV_SUPPORT_AY2 = (1 << 21),
	//	Right stick X
	PDD_DEV_SUPPORT_AX2 = (1 << 20),
	//	Left stick Y
	PDD_DEV_SUPPORT_AY1 = (1 << 19),
	//	Left stick X
	PDD_DEV_SUPPORT_AX1 = (1 << 18),
	//	Analog trigger L
	PDD_DEV_SUPPORT_AL = (1 << 17),
	//	Analog trigger R
	PDD_DEV_SUPPORT_AR = (1 << 16),
	//	D-Pad B Right
	PDD_DEV_SUPPORT_KRB = (1 << 15),
	//	D-Pad B Left
	PDD_DEV_SUPPORT_KLB = (1 << 14),
	//	D-Pad B Down
	PDD_DEV_SUPPORT_KDB = (1 << 13),
	//	D-Pad B Up
	PDD_DEV_SUPPORT_KUB = (1 << 12),
	//	D button
	PDD_DEV_SUPPORT_TD = (1 << 11),
	//	X button
	PDD_DEV_SUPPORT_TX = (1 << 10),
	//	Y button
	PDD_DEV_SUPPORT_TY = (1 << 9),
	//	Z button
	PDD_DEV_SUPPORT_TZ = (1 << 8),
	//	D-Pad A Right
	PDD_DEV_SUPPORT_KR = (1 << 7),
	//	D-Pad A Left
	PDD_DEV_SUPPORT_KL = (1 << 6),
	//	D-Pad A Down
	PDD_DEV_SUPPORT_KD = (1 << 5),
	//	D-Pad A Up
	PDD_DEV_SUPPORT_KU = (1 << 4),
	//	Start button
	PDD_DEV_SUPPORT_ST = (1 << 3),
	//	A button
	PDD_DEV_SUPPORT_TA = (1 << 2),
	//	B button
	PDD_DEV_SUPPORT_TB = (1 << 1),
	//	C button
	PDD_DEV_SUPPORT_TC = (1 << 0),
};
