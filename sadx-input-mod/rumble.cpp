#include "stdafx.h"

#include <SADXModLoader.h>

#include "minmax.h"
#include "input.h"
#include "rumble.h"

namespace rumble
{
	static ObjectMaster* Instances[GAMEPAD_COUNT][2] = {};

	Sint32 __cdecl pdVibMxStop_hook(Uint32 port)
	{
		for (Uint32 i = 0; i < GAMEPAD_COUNT; i++)
		{
			auto& pad = DreamPad::Controllers[i];
			if (pad.GetActiveMotor() != Motor::None)
				pad.SetActiveMotor(Motor::Both, false);
		}

		return 0;
	}

	static void __cdecl Rumble_Main_hook(ObjectMaster* _this)
	{
		auto v1    = (ObjUnknownB*)_this->UnknownB_ptr;
		auto param = (PDS_VIBPARAM*)_this->UnknownA_ptr;
		auto motor = (Motor)param->reserved[0];
		auto& pad  = DreamPad::Controllers[param->unit];

		if (!v1->Mode)
		{
			pad.SetActiveMotor(motor, true);
			v1->Mode = 1;
			Instances[param->unit][(int)motor - 1] = _this;
		}

		if (v1->Time-- <= 0)
		{
			DeleteObject_(_this);
			pad.SetActiveMotor(motor, false);
		}
	}

	static void __cdecl Rumble_Delete(ObjectMaster* _this)
	{
		auto param = (PDS_VIBPARAM*)_this->UnknownA_ptr;
		auto motor = (Motor)param->reserved[0];
		auto i = (int)motor - 1;

		if (Instances[param->unit][i] == _this)
			Instances[param->unit][i] = nullptr;
	}

	static void __cdecl Rumble_Load_hook(Uint32 port, Uint32 time, Motor motor)
	{
		if (port >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
				Rumble_Load_hook(i, time, motor);
			return;
		}

		auto _this = LoadObject(LoadObj_UnknownB, 0, Rumble_Main_hook);

		if (_this == nullptr)
			return;

		Uint32 time_scaled = max(4 * time, (uint)(DreamPad::Controllers[port].settings.rumbleMinTime / (1000.0f / 60.0f)));
		((ObjUnknownB*)_this->UnknownB_ptr)->Time = time_scaled;
		auto param = (PDS_VIBPARAM*)AllocateMemory(sizeof(PDS_VIBPARAM));

		if (param)
		{
			param->unit  = port;
			param->flag  = PDD_VIB_FLAG_CONTINUOUS;
			param->power = 10;
			param->freq  = 50;
			param->inc   = 0;

			param->reserved[0] = motor;

			_this->DeleteSub = Rumble_Delete;
			_this->UnknownA_ptr = param;

			// HACK: Fixes tornado in Windy Valley, allowing it to queue rumble requests and pulse the motor.
			auto i = (int)motor - 1;
			if (motor & Motor::Large && Instances[i] != nullptr)
				DeleteObject_(Instances[port][i]);

			if (input::debug)
			{
				auto time_ms = (Uint32)(time_scaled * (1000.0f / (60.0f / (float)FrameIncrement)));
				PrintDebug("[Input] [%u] Rumble %u: %s, %u frames (%ums)\n",
					FrameCounter, param->unit, (motor == Motor::Small ? "R" : "L"), time_scaled, time_ms);
			}
		}
		else
		{
			DeleteObject_(_this);
		}
	}

	/// <summary>
	/// Rumbles the large motor. Used for things like explosions, springs, dash panels, etc.
	/// </summary>
	/// <param name="port">Controller port.></param>
	/// <param name="time">Time to rumble.</param>
	void __cdecl RumbleA(Uint32 port, Uint32 time)
	{
		if (RumbleEnabled && !DemoPlaying && input::_ControllerEnabled[port])
			Rumble_Load_hook(port, clamp(time, 1u, 255u), Motor::Large);
	}

	/// <summary>
	/// Rumbles the small motor. Used for things like taking damage.
	/// </summary>
	/// <param name="port">Controller port.</param>
	/// <param name="time">Time to rumble.</param>
	/// <param name="a3">Unknown.</param>
	/// <param name="a4">Unknown.</param>
	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4)
	{
		Uint32 idk; // ecx@4
		int _a3; // eax@12
		int _time; // eax@16

		if (RumbleEnabled && !DemoPlaying && input::_ControllerEnabled[port])
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

	static const void* loc_0042D534 = (const void*)0x0042D534;
	/// <summary>
	/// Enables rumble by default when a new save file is created.
	/// </summary>
	void __declspec(naked) DefaultRumble()
	{
		__asm
		{
			mov [esp + 26Ah], 1
			jmp loc_0042D534
		}
	}
}