#pragma once

#include <SADXStructs.h>

struct KeyboardKey
{
	char held;
	char old;
	char pressed;
};

DataArray(KeyboardKey, KeyboardKeys, 0x03B0E3E0, 256);

struct KeyData
{
	Uint32 WindowsCode;
	Uint32 SADX2004Code;
};

KeyData SADX2004Keys[] = {
	{ VK_ESCAPE, 41 },
	{ VK_F1, 58 },
	{ VK_F2, 59 },
	{ VK_F3, 60 },
	{ VK_F4, 61 },
	{ VK_F5, 62 },
	{ VK_F6, 63 },
	{ VK_F7, 64 },
	{ VK_F8, 65 },
	{ VK_F9, 66 },
	{ VK_F10, 67 },
	{ VK_F11, 68 },
	{ VK_F12, 69 },
	{ VK_SNAPSHOT, 70 },
	{ VK_SCROLL, 71 },
	{ VK_PAUSE, 72 },
	{ 0xC0, 140 }, //~ (doesn't exist in vanilla SADX)
	{ 0x31, 30 }, //1
	{ 0x32, 31 }, //2
	{ 0x33, 32 }, //3
	{ 0x34, 33 }, //4
	{ 0x35, 34 }, //5
	{ 0x36, 35 }, //6
	{ 0x37, 36 }, //7
	{ 0x38, 37 }, //8
	{ 0x39, 38 }, //9
	{ 0x30, 39 }, //0
	{ VK_OEM_MINUS, 45 }, //-
	{ 0xBB, 46 }, //=
	{ VK_BACK, 42 }, //Backspace
	{ VK_INSERT, 73 },
	{ VK_HOME, 74 },
	{ VK_PRIOR, 75 }, //Page Up
	{ VK_NUMLOCK, 83 }, //Num Lock
	{ 0x6F, 84 }, //Num slash
	{ 0x6A, 85 }, //Num asterisk
	{ 0x6D, 86 }, //Num minus
	{ VK_TAB, 43 },
	{ 0x51, 20 }, //Q
	{ 0x57, 26 }, //W
	{ 0x45, 8 }, //E
	{ 0x52, 21 }, //R
	{ 0x54, 23 }, //T
	{ 0x59, 28 }, //Y
	{ 0x55, 24 }, //U
	{ 0x49, 12 }, //I
	{ 0x4F, 18 }, //O
	{ 0x50, 19 }, //P
	{ 219, 141 }, //[ (doesn't exist in vanilla SADX)
	{ 221, 142 }, //] (doesn't exist in vanilla SADX)
	{ 220, 135 }, //Backslash
	{ VK_DELETE, 76 },
	{ VK_END, 77 },
	{ VK_NEXT, 78 }, //Page Down
	{ VK_NUMPAD7, 95 }, //Num 7 (Home)
	{ VK_NUMPAD8, 96 }, //Num 8 (Up)
	{ VK_NUMPAD9, 97 }, //Num 9 (Page Up)
	{ VK_ADD, 87 }, //Num Plus
	{ VK_CAPITAL, 143 }, //Caps Lock (doesn't exist in vanilla SADX)
	{ 0x41, 4 }, //A
	{ 0x53, 22 }, //S
	{ 0x44, 7 }, //D
	{ 0x46, 9 }, //F
	{ 0x47, 10 }, //G
	{ 0x48, 11 }, //H
	{ 0x4A, 13 }, //J
	{ 0x4B, 14 }, //K
	{ 0x4C, 15 }, //L
	{ VK_OEM_1, 51 }, //Semicolon
	{ VK_OEM_7, 52 }, //'
	{ VK_RETURN, 40 }, //Enter
	{ VK_NUMPAD4, 92 }, //Num 4 (Left)
	{ VK_NUMPAD5, 93 }, //Num 5
	{ VK_NUMPAD6, 94 }, //Num 6 (Right)
	{ VK_LSHIFT }, //Left Shift (doesn't exist in vanilla SADX)
	{ 0x5A, 29 }, //Z
	{ 0x58, 27 }, //X
	{ 0x43, 6 }, //C
	{ 0x56, 25 }, //V
	{ 0x42, 5 }, //B
	{ 0x4E, 17 }, //N
	{ 0x4D, 16 }, //M
	{ VK_OEM_COMMA, 54 },
	{ VK_OEM_PERIOD, 55 },
	{ VK_OEM_2, 56 }, //Slash
	{ VK_RSHIFT, 144 }, //Right Shift (doesn't exist in vanilla SADX)
	{ VK_UP, 82 }, //Up
	{ VK_NUMPAD1, 89 }, //Num 1 (End)
	{ VK_NUMPAD2, 90 }, //Num 2 (Down)
	{ VK_NUMPAD3, 91 }, //Num 3 (Page Down)
	{ VK_LCONTROL, 145 }, //Left Control (doesn't exist in vanilla SADX)
	{ VK_LMENU, 146 }, //Left Alt (doesn't exist in vanilla SADX)
	{ VK_SPACE, 44 },
	{ VK_RMENU, 147 }, //Right Alt (doesn't exist in vanilla SADX)
	{ VK_APPS, 148 }, //Menu (doesn't exist in vanilla SADX)
	{ VK_RCONTROL, 149 }, //Right Control (doesn't exist in vanilla SADX)
	{ VK_LEFT, 80 },
	{ VK_DOWN, 81 },
	{ VK_RIGHT, 79 },
	{ VK_NUMPAD0, 98 },
	{ 110, 150 }, //Numpad Delete (doesn't exist in vanilla SADX)
	{ 256, 151 }, //Numpad Enter (doesn't exist in vanilla SADX)
};
