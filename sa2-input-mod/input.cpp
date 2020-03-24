#include "stdafx.h"

#include <SA2ModLoader.h>
#include <algorithm>

#include "typedefs.h"
#include "input.h"
#include "DreamPad.h"

/*
 * TODO: hook up to VibTask
 * TODO: handle controller enabled states
 * TODO: don't call redirect_raw_controllers every poll
 */

namespace input
{
	PDS_PERIPHERAL raw_input[GAMEPAD_COUNT] {};
	bool controller_enabled[GAMEPAD_COUNT] {};
	bool debug = false;

	inline void poll_sdl()
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				default:
					break;

				case SDL_JOYDEVICEADDED:
				{
					const int which = event.cdevice.which;

					for (auto& controller : DreamPad::controllers)
					{
						// Checking for both in cases like the DualShock 4 and (e.g.) DS4Windows where the controller might be
						// "connected" twice with the same ID. DreamPad::open automatically closes if already open.
						if (!controller.connected() || controller.controller_id() == which)
						{
							controller.open(which);
							break;
						}
					}
					break;
				}

				case SDL_JOYDEVICEREMOVED:
				{
					const int which = event.cdevice.which;

					for (auto& controller : DreamPad::controllers)
					{
						if (controller.controller_id() == which)
						{
							controller.close();
							break;
						}
					}
					break;
				}
			}
		}

		SDL_GameControllerUpdate();
	}

	void redirect_raw_controllers();
	
	void poll_controllers()
	{
		redirect_raw_controllers();
		poll_sdl();

		//KeyboardMouse::poll();

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			DreamPad& dream_pad = DreamPad::controllers[i];

			dream_pad.poll();
			raw_input[i] = *reinterpret_cast<const PDS_PERIPHERAL*>(&dream_pad.dreamcast_data());

			// Compatibility for mods which use ControllersRaw directly.
			// This will only copy the first four controllers.
			if (i < ControllersRaw_Length)
			{
				auto& game_raw = ControllersRaw[i];
				auto& mod_raw = raw_input[i];

				const auto name   = game_raw.name;
				const auto info   = game_raw.info;
				const auto extend = game_raw.extend;

				mod_raw.name   = name;
				mod_raw.info   = info;
				mod_raw.extend = extend;

				game_raw = *reinterpret_cast<PDS_PERIPHERAL*>(&mod_raw);
			}

		#if 0 // UNDONE
			if (debug && raw_input[i].HeldButtons & Buttons_C)
			{
				const DCControllerData& pad = raw_input[i];

			#if 0
				Motor m = DreamPad::controllers[i].active_motor();

				DisplayDebugStringFormatted(NJM_LOCATION(0, 8 + (3 * i)), "P%d  B: %08X LT/RT: %03d/%03d V: %d%d", (i + 1),
				                            pad.HeldButtons, pad.LTriggerPressure, pad.RTriggerPressure, (m & Motor::large), (m & Motor::small) >> 1);
				DisplayDebugStringFormatted(NJM_LOCATION(4, 9 + (3 * i)), "LS: %4d/%4d (%f) RS: %4d/%4d (%f)",
				                            pad.LeftStickX, pad.LeftStickY, dreampad.normalized_l(), pad.RightStickX, pad.RightStickY,
				                            dreampad.normalized_r());
			#endif

				if (pad.HeldButtons & Buttons_Z)
				{
					const int pressed = pad.PressedButtons;
					if (pressed & Buttons_Up)
					{
						dreampad.settings.rumble_factor += 0.125f;
					}
					else if (pressed & Buttons_Down)
					{
						dreampad.settings.rumble_factor -= 0.125f;
					}
					else if (pressed & Buttons_Left)
					{
						//rumble::RumbleA_r(i, 0);
					}
					else if (pressed & Buttons_Right)
					{
						//rumble::RumbleB_r(i, 7, 59, 6);
					}

				#if 0
					DisplayDebugStringFormatted(NJM_LOCATION(4, 10 + (3 * i)),
					                            "Rumble factor (U/D): %f (L/R to test)", dreampad.settings.rumble_factor);
				#endif
				}
			}
		#endif
		}
	}

	DataPointer(bool, ControllersEnabled, 0x0174affe);

	void WriteAnalogs_c()
	{
		if (!ControllersEnabled)
		{
			return;
		}

		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			if (!controller_enabled[i])
			{
				continue;
			}

			const DreamPad& dream_pad = DreamPad::controllers[i];

			if (dream_pad.connected() || (dream_pad.settings.allow_keyboard && !i))
			{
				const DCControllerData& pad = dream_pad.dreamcast_data();

				// SA2's internal deadzone is 14 of 127. It doesn't set the relative forward direction
				// unless this is exceeded in WriteAnalogs(), so the analog shouldn't be set otherwise.
				if (abs(pad.LeftStickX) > 14 || abs(pad.LeftStickY) > 14)
				{
					AnalogThings[i].magnitude = dream_pad.normalized_l();
				}
			}
		}
	}

	static void redirect_raw_controllers()
	{
		for (uint i = 0; i < GAMEPAD_COUNT; i++)
		{
			auto& ptr = ControllerPointers[i];
			ptr = reinterpret_cast<PDS_PERIPHERAL*>(&raw_input[i]);
		}
	}

	void EnableController_r(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			controller_enabled[0] = true;
			controller_enabled[1] = true;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < std::min(index, static_cast<Uint8>(GAMEPAD_COUNT)); i++)
			{
				EnableController_r(i);
			}
		}
		else
		{
			controller_enabled[index] = true;
		}
	}

	void DisableController_r(Uint8 index)
	{
		// default behavior 
		if (index > 1)
		{
			controller_enabled[0] = false;
			controller_enabled[1] = false;
		}

		if (index > GAMEPAD_COUNT)
		{
			for (Uint32 i = 0; i < std::min(index, static_cast<Uint8>(GAMEPAD_COUNT)); i++)
			{
				DisableController_r(i);
			}
		}
		else
		{
			controller_enabled[index] = false;
		}
	}
}
