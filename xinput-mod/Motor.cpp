#include "Common.h"
#include "Ingame.h"

#include "Motor.h"

namespace xinput
{
	XINPUT_VIBRATION	vibration[XPAD_COUNT];
	Motor				rumbleState[XPAD_COUNT];
	uint				rumbleTime_L[XPAD_COUNT];
	uint				rumbleTime_R[XPAD_COUNT];

	const uint rumbleDur_L = 250;	// milliseconds
	const uint rumbleDur_R = 1000;	// milliseconds

	void SetActiveMotor(ushort id, Motor motor, short magnitude)
	{
		float f = settings[id].rumbleFactor;

		if (motor & Motor::Left)
		{
			vibration[id].wLeftMotorSpeed = (short)min(SHRT_MAX, (int)(magnitude * f));
			rumbleTime_L[id] = GetTickCount();
			rumbleState[id] = (Motor)((magnitude > 0) ? rumbleState[id] | motor : rumbleState[id] & ~Motor::Left);
		}

		if (motor & Motor::Right)
		{
			vibration[id].wRightMotorSpeed = (short)min(SHRT_MAX, (int)(magnitude * (2 + f)));
			rumbleTime_R[id] = GetTickCount();
			rumbleState[id] = (Motor)((magnitude > 0) ? rumbleState[id] | motor : rumbleState[id] & ~Motor::Right);
		}

		//XInputSetState(id, &vibration[id]);
	}

	Motor GetActiveMotor(ushort id)
	{
		return rumbleState[id % XPAD_COUNT];
	}

	void UpdateMotor(ushort id)
	{
		if (rumbleState[id] == Motor::None)
			return;

		Motor result = Motor::None;
		uint now = GetTickCount();

		if (now - rumbleTime_L[id] >= rumbleDur_L)
			result = (Motor)(result | Motor::Left);

		if (now - rumbleTime_R[id] >= rumbleDur_R)
			result = (Motor)(result | Motor::Right);

		if (result != Motor::None)
			Rumble(id, 0, result);
	}
}