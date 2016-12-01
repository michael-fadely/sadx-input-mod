#include "stdafx.h"

#include <SADXModLoader.h>

#include "minmax.h"
#include "input.h"
#include "rumble.h"

namespace rumble
{
	struct RumbleTimer
	{
		bool applied;
		Uint8 port;
		Motor motor;
		Sint32 frames;
	};

	static ObjectMaster* instances[GAMEPAD_COUNT][2] = {};

	inline ObjectMaster* get_instance(Uint32 port, Motor motor)
	{
		return instances[port][(int)motor - 1];
	}

	inline void set_instance(Uint32 port, Motor motor, ObjectMaster* ptr)
	{
		auto i = (int)motor - 1;
		instances[port][i] = ptr;
	}

	Sint32 __cdecl pdVibMxStop_hook(Uint32 port)
	{
		for (auto& i : instances)
		{
			if (i[0])
			{
				DeleteObject_(i[0]);
				i[0] = nullptr;
			}

			if (i[1])
			{
				DeleteObject_(i[1]);
				i[1] = nullptr;
			}
		}

		for (Uint32 i = 0; i < GAMEPAD_COUNT; i++)
		{
			auto& pad = DreamPad::Controllers[i];
			if (pad.GetActiveMotor() != Motor::None)
			{
				pad.SetActiveMotor(Motor::Both, false);
			}
		}

		return 0;
	}

	static void __cdecl Rumble_Main_hook(ObjectMaster* _this)
	{
		auto data = (RumbleTimer*)_this->UnknownA_ptr;
		auto& pad  = DreamPad::Controllers[data->port];

		if (data->frames-- <= 0)
		{
			DeleteObject_(_this);
			pad.SetActiveMotor(data->motor, false);
		}
		else if (!data->applied)
		{
			pad.SetActiveMotor(data->motor, true);
			data->applied = true;
		}
	}

	static void __cdecl Rumble_Delete(ObjectMaster* _this)
	{
		auto data = (RumbleTimer*)_this->UnknownA_ptr;
		_this->UnknownA_ptr = nullptr;

		auto instance = get_instance(data->port, data->motor);
		if (instance == _this)
		{
			set_instance(data->port, data->motor, nullptr);
		}

		delete data;
	}

	static void __cdecl Rumble_Load_hook(Uint32 port, Uint32 time, Motor motor)
	{
		if (port >= GAMEPAD_COUNT)
		{
			for (ushort i = 0; i < GAMEPAD_COUNT; i++)
			{
				Rumble_Load_hook(i, time, motor);
			}
			return;
		}

		auto time_scaled = max(4 * time, (Uint32)(DreamPad::Controllers[port].settings.rumbleMinTime / (1000.0f / 60.0f)));
		auto instance = get_instance(port, motor);

		// HACK: Fixes tornado in Windy Valley, allowing it to queue rumble requests and pulse the motor.
		if (!(motor & Motor::Small) && instance != nullptr)
		{
			auto data = (RumbleTimer*)instance->UnknownA_ptr;
			data->frames = time_scaled;
			data->applied = false;
			DreamPad::Controllers[port].SetActiveMotor(motor, false);
		}
		else
		{
			auto _this = LoadObject((LoadObj)0, 0, Rumble_Main_hook);

			if (_this == nullptr)
			{
				return;
			}

			set_instance(port, motor, _this);

			auto data = new RumbleTimer;

			data->applied = false;
			data->port    = port;
			data->motor   = motor;
			data->frames  = time_scaled;

			_this->DeleteSub = Rumble_Delete;
			_this->UnknownA_ptr = data;

			if (input::debug)
			{
				auto time_ms = (Uint32)(time_scaled * (1000.0f / (60.0f / (float)FrameIncrement)));
				PrintDebug("[Input] [%u] Rumble %u: %s, %u frames (%ums)\n",
					FrameCounter, data->port, (motor == Motor::Small ? "R" : "L"), time_scaled, time_ms);
			}
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
		{
			Rumble_Load_hook(port, clamp(time, 1u, 255u), Motor::Large);
		}
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
