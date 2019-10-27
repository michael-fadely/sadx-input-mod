#pragma once

struct KeyboardStick : Point2I
{
	uint32_t directions;
	static constexpr auto amount = 8192;

	void update();
};

class KeyboardMouse
{
public:
	static const DCControllerData& dreamcast_data()
	{
		return pad;
	}
	static float normalized_l()
	{
		return normalized_l_;
	}
	static float normalized_r()
	{
		return normalized_r_;
	}

	static void poll();
	static void update_keyboard_buttons(uint32_t key, bool down);
	static void update_cursor(int32_t xrel, int32_t yrel);
	static void reset_cursor();
	static void update_mouse_buttons(uint32_t button, bool down);
	static LRESULT read_window_message(HWND handle, UINT Msg, WPARAM wParam, LPARAM lParam);
	static void hook_wnd_proc();

private:
	static DCControllerData pad;
	static float normalized_l_, normalized_r_;
	static bool mouse_active;
	static bool left_button;
	static bool right_button;
	static bool half_press;
	static Point2I cursor;
	static KeyboardStick sticks[2];
	static int16_t mouse_x;
	static int16_t mouse_y;
	static WNDPROC lpPrevWndFunc;
};
