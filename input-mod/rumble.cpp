#include "stdafx.h"

#include <SADXModLoader.h>

#include "input.h"
#include "rumble.h"

// HACK: Update mod loader
#define UnknownA_ptr field_28
#define UnknownB_ptr field_2C

namespace rumble
{
	Sint32 __cdecl pdVibMxStop_hook(Uint32 port)
	{
		for (Uint32 i = 0; i < GAMEPAD_COUNT; i++)
			DreamPad::Controllers[i].SetActiveMotor(Motor::Both, 0);
		return 0;
	}

	void __cdecl Rumble_Main_hook(ObjectMaster* _this)
	{
		ObjUnknownB* v1 = (ObjUnknownB*)_this->UnknownB_ptr;
		PDS_VIBPARAM* param = (PDS_VIBPARAM*)_this->UnknownA_ptr;

		if (v1->field_0-- <= 0)
		{
			DreamPad::Controllers[param->unit].SetActiveMotor((Motor)param->reserved[0], 0);
			DeleteObject_(_this);
		}
		else
		{
			DreamPad::Controllers[param->unit].SetActiveMotor((Motor)param->reserved[0], 1000);
		}
	}

	void __cdecl Rumble_Load_hook(Uint32 port, Uint32 time, Motor motor)
	{
		if (port >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
				Rumble_Load_hook(i, time, motor);

			return;
		}

		ObjectMaster* _this = LoadObject(LoadObj_UnknownB, 0, Rumble_Main_hook);

		if (_this == nullptr)
			return;

		((ObjUnknownB*)_this->UnknownB_ptr)->field_0 = 4 * time;
		PDS_VIBPARAM* param = (PDS_VIBPARAM*)AllocateMemory(sizeof(PDS_VIBPARAM));

		if (param)
		{
			param->unit = port;
			param->flag = PDD_VIB_FLAG_CONTINUOUS;
			param->power = 10;
			param->freq = 50;
			param->inc = 0;

			// hax
			param->reserved[0] = motor;

			_this->UnknownA_ptr = param;
		}
		else
		{
			DeleteObject_(_this);
		}
	}

	void __cdecl RumbleA(Uint32 port, Uint32 time)
	{
		if (!CutscenePlaying && RumbleEnabled && input::_ControllerEnabled[port])
			Rumble_Load_hook(port, clamp(time, 1u, 255u), Motor::Large);
	}

	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4)
	{
		Uint32 idk; // ecx@4
		int _a3; // eax@12
		int _time; // eax@16

		if (!CutscenePlaying && RumbleEnabled && input::_ControllerEnabled[port])
		{
			idk = time;
			if ((signed int)time <= 4)
			{
				if ((signed int)time >= -4)
				{
					if (time == 1)
					{
						idk = 2;
					}
					else if (time == -1)
					{
						idk = -2;
					}
				}
				else
				{
					idk = -4;
				}
			}
			else
			{
				idk = 4;
			}
			_a3 = a3;
			if (a3 <= 59)
			{
				if (a3 < 7)
				{
					_a3 = 7;
				}
			}
			else
			{
				_a3 = 59;
			}
			_time = a4 * _a3 / (signed int)(4 * idk);
			if (_time <= 0)
			{
				_time = 1;
			}
			Rumble_Load_hook(port, _time, Motor::Small);
		}
	}
}