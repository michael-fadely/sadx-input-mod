#pragma once
#include "typedefs.h"

namespace xinput
{
	enum Motor : int8
	{
		None,
		Left,
		Right,
		Both
	};

	/// <summary>
	/// Sets active motor(s) to rumble.
	/// </summary>
	/// <param name="id">Controller ID.</param>
	/// <param name="motor">Motor(s) to rumble.</param>
	/// <param name="magnitude">Rumble magnitude.</param>
	void SetActiveMotor(ushort id, Motor motor, short magnitude);

	/// <summary>
	/// Gets the rumble state for the specified ID.
	/// </summary>
	/// <param name="id">Controller ID.</param>
	/// <returns>The motor associated with the specified ID.</returns>
	Motor GetActiveMotor(ushort id);

	/// <summary>
	/// Checks if the rumble duration has exceeded the threshold and disables the motor in question.
	/// </summary>
	/// <param name="id">Controller ID.</param>
	void UpdateMotor(ushort id);
}
