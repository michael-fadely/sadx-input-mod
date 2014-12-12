#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>

#include <SADXModLoader.h>
#include <G:\Libraries\LazyTypedefs.h>

#include "UpdateControllersXInput.h"

DataArray(ControllerData, Controller_Data_0, 0x03B0E9C8, 8);
DataPointer(int, rumble_related_3B2A2E4, 0x3B2A2E4);
DataPointer(char, enableRumble, 0x00913B10);

const short DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
bool rumble = false;
const uint rumble_timer = 15; // TODO: Take framerate setting into consideration.
uint rumble_elapsed = 0;

// Returns analog if it exceeds the deadzone, otherwise 0.
// TODO: Configurable deadzone, split for each analog stick
short deadzone(short analog, short dz = DEADZONE)
{
	return (analog < -dz || analog > dz) ? analog : 0;
}
// Converts wButtons in controller to Sonic Adventure compatible buttons and returns the value.
int XInputToDreamcast(const XINPUT_GAMEPAD& controller)
{
	int result = 0;
	int buttons = controller.wButtons;

	if (buttons & XINPUT_GAMEPAD_A)
		result |= Buttons_A;
	if (buttons & XINPUT_GAMEPAD_B)
		result |= Buttons_B;
	if (buttons & XINPUT_GAMEPAD_X)
		result |= Buttons_X;
	if (buttons & XINPUT_GAMEPAD_Y)
		result |= Buttons_Y;
	if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
		result |= Buttons_Z;

	// TODO: Configurable trigger threshold
	if (controller.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		result |= Buttons_L;
	if (controller.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		result |= Buttons_R;

	if (buttons & XINPUT_GAMEPAD_START)
		result |= Buttons_Start;

	if (buttons & XINPUT_GAMEPAD_DPAD_UP)
		result |= Buttons_Up;
	if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
		result |= Buttons_Down;
	if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
		result |= Buttons_Left;
	if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
		result |= Buttons_Right;

	return result;
}

// TODO: Mouse
void __cdecl UpdateControllersXInput()
{
	for (uint i = 0; i < 4; i++)
	{
		ControllerData* pad = &Controller_Data_0[i];
		XINPUT_STATE state = {};
		XInputGetState(i, &state);
		XINPUT_GAMEPAD controller = state.Gamepad;

		// Not sure what this is for
		pad->Support = 0x3F07FEu;

		// L Analog
		pad->LeftStickX = deadzone(controller.sThumbLX);
		pad->LeftStickY = deadzone(-controller.sThumbLY);

		// R Analog
		pad->RightStickX = deadzone(controller.sThumbRX);
		pad->RightStickY = deadzone(-controller.sThumbRY);

		// Trigger pressure
		pad->LTriggerPressure = controller.bLeftTrigger;
		pad->RTriggerPressure = controller.bRightTrigger;

		// Now set the released buttons to the pressed buttons from last frame.
		pad->ReleasedButtons = pad->PressedButtons;

		// Now, get the new buttons from the XInput controller
		pad->HeldButtons = XInputToDreamcast(controller);
		pad->NotHeldButtons = pad->HeldButtons;

		// Do some fancy math to "press" only the necessary buttons
		pad->PressedButtons = pad->HeldButtons;
		pad->PressedButtons &= ~pad->Old;

		// Set the "last held" to held
		pad->Old = pad->HeldButtons;
	}

	// Disable rumble if the timer says it's a good idea.
	if (rumble && ++rumble_elapsed == rumble_timer)
	{
		Rumble(0);
		rumble_elapsed = 0;
		rumble = false;
	}
}

void __cdecl Rumble(int a1)
{
	rumble = true;
	// TODO: Automatic scaling from byte to short value
	a1 *= 32767;

	XINPUT_VIBRATION vibration = { (a1 & 0x0000FFFF), (a1 & 0xFFFF0000) };

	for (uint i = 0; i < 4; i++)
		XInputSetState(i, &vibration);
}
void __cdecl RumbleA(int a1, signed int a2)
{
	int intensity; // eax@4

	if (!rumble_related_3B2A2E4)
	{
		if (enableRumble)
		{
			if (!a1)
			{
				intensity = a2;
				if (a2 <= 255)
				{
					if (a2 < 0 || a2 < 1)
						intensity = 1;
					Rumble(intensity);
				}
				else
				{
					Rumble(255);
				}
			}
		}
	}
}
void __cdecl RumbleB(int a1, signed int a2, signed int a3, int a4)
{
	signed int v4; // ecx@4
	signed int v5; // eax@12
	signed int intensity; // eax@16

	if (!rumble_related_3B2A2E4)
	{
		if (enableRumble)
		{
			if (!a1)
			{
				v4 = a2;
				if (a2 <= 4)
				{
					if (a2 >= -4)
					{
						if (a2 == 1)
						{
							v4 = 2;
						}
						else
						{
							if (a2 == -1)
								v4 = -2;
						}
					}
					else
					{
						v4 = -4;
					}
				}
				else
				{
					v4 = 4;
				}
				v5 = a3;
				if (a3 <= 59)
				{
					if (a3 < 7)
						v5 = 7;
				}
				else
				{
					v5 = 59;
				}
				intensity = a4 * v5 / (4 * v4);
				if (intensity <= 0)
					intensity = 1;
				Rumble(intensity);
			}
		}
	}
}

#pragma region help me
/*
DataPointer(CharObj1*, RAM_dword_ptr_P1_Obj1, 0x03B42E10);
DataPointer(ControllerData*, Controller_Data_0_ptr, 0x03B0E77C);
DataPointer(WORD, RAM_word_Game_Status, 0x03B22DE4);

DataPointer(LPVOID, dword_3B2C480, 0x3B2C480);
DataPointer(_UNKNOWN, unk_909FB8, 0x909FB8);
DataArray(char, byte_3B0E9A4, 0x3B0E9A4, 0);
DataPointer(char, byte_3B0E3EC, 0x3B0E3EC);
DataPointer(char, byte_3B0E3F2, 0x3B0E3F2);
DataPointer(char, byte_3B0E3F5, 0x3B0E3F5);
DataPointer(char, byte_3B0E3FB, 0x3B0E3FB);
DataPointer(char, byte_3B0E404, 0x3B0E404);
DataPointer(char, byte_3B0E407, 0x3B0E407);
DataPointer(char, byte_3B0E40A, 0x3B0E40A);
DataPointer(char, byte_3B0E410, 0x3B0E410);
DataPointer(char, byte_3B0E41C, 0x3B0E41C);
DataPointer(char, byte_3B0E41F, 0x3B0E41F);
DataPointer(char, byte_3B0E422, 0x3B0E422);
DataPointer(char, byte_3B0E42B, 0x3B0E42B);
DataPointer(char, byte_3B0E42E, 0x3B0E42E);
DataPointer(char, byte_3B0E431, 0x3B0E431);
DataPointer(char, byte_3B0E437, 0x3B0E437);
DataPointer(char, byte_3B0E458, 0x3B0E458);
DataPointer(char, byte_3B0E45B, 0x3B0E45B);
DataPointer(char, byte_3B0E464, 0x3B0E464);
DataPointer(char, byte_3B0E4BE, 0x3B0E4BE);
DataArray(int, dword_3B0E344, 0x3B0E344, 0);
DataArray(int, dword_3B0E768, 0x3B0E768, 0);
DataArray(int, dword_3B0E9B8, 0x3B0E9B8, 0);
DataArray(int, dword_3B0EBFC, 0x3B0EBFC, 0);
DataArray(int, dword_3B36DD0, 0x3B36DD0, 0);
DataArray(int, dword_8929D4, 0x8929D4, 0);
DataArray(int, dword_909FE0, 0x909FE0, 0);
DataArray(int, dword_90A000, 0x90A000, 0);
DataArray(int, dword_90A014, 0x90A014, 0);
DataArray(int, dword_90A01C, 0x90A01C, 0);
DataArray(int, dword_90A020, 0x90A020, 0);
DataPointer(int, dword_3B0E340, 0x3B0E340);
DataPointer(int, dword_3B0E764, 0x3B0E764);
DataPointer(int, dword_3B0E778, 0x3B0E778);
DataPointer(int, dword_3B0E990, 0x3B0E990);
DataPointer(int, dword_3B0E994, 0x3B0E994);
DataPointer(int, dword_3B0E998, 0x3B0E998);
DataPointer(int, dword_3B0E99C, 0x3B0E99C);
DataPointer(int, dword_3B0E9A0, 0x3B0E9A0);
DataPointer(int, dword_3B0EBF0, 0x3B0EBF0);
DataPointer(int, dword_3B0EBF4, 0x3B0EBF4);
DataPointer(int, dword_3B0EBF8, 0x3B0EBF8);
DataPointer(int, dword_3B0EC0C, 0x3B0EC0C);
DataPointer(int, dword_3B0EC10, 0x3B0EC10);
DataPointer(int, dword_3B0EC14, 0x3B0EC14);
DataPointer(int, dword_3B0EC18, 0x3B0EC18);
DataPointer(int, dword_3B0EC24, 0x3B0EC24);
DataPointer(int, dword_3B2A2E4, 0x3B2A2E4);
DataPointer(int, dword_3B2A2E8, 0x3B2A2E8);
DataPointer(int, dword_3B2C470, 0x3B2C470);
DataPointer(int, dword_3B2C474, 0x3B2C474);
DataPointer(int, dword_3B36D34, 0x3B36D34);
DataPointer(int, dword_909FC8, 0x909FC8);
DataPointer(int, dword_90A010, 0x90A010);
DataPointer(int, dword_7DCC9C, 0x7DCC9C);
DataPointer(int, dword_7DCCB0, 0x7DCCB0);
DataPointer(__int64, qword_3B0EC1C, 0x3B0EC1C);
DataArray(short, word_3B0E6E0, 0x3B0E6E0, 0);
DataPointer(short, word_3B2C460, 0x3B2C460);
DataPointer(short, word_3B2C464, 0x3B2C464);

FunctionPointer(char, GetDInputMouseState, (void), 0x0040BB80);
FunctionPointer(char*, sub_77F120, (int a1), 0x77F120);
FunctionPointer(char, sub_40EE10, (void), 0x40EE10);
FunctionPointer(int, sub_77EFA0, (int a1), 0x77EFA0);
FunctionPointer(int, sub_77EFB0, (int a1), 0x77EFB0);
FunctionPointer(int, sub_77F060, (int a1), 0x77F060);
FunctionPointer(void, sub_40E900, (void), 0x40E900);
*/
#pragma endregion

/*
void __cdecl WriteControllerData()
{
	int v0; // eax@4
	int iterator_maybe; // ecx@6
	int v2; // eax@8
	int zerobutgetsreused; // ebx@8
	int iterator_dupe; // edi@8
	ControllerData *controller_ptr; // edi@10
	int v6; // ebp@13
	signed int v7; // esi@32
	signed int v8; // eax@33
	int v9; // ecx@33
	int v10; // eax@36
	int v11; // ecx@42
	CharObj2 *v12; // esi@42
	long double v13; // st7@50
	__int16 v14; // ax@56
	char *v15; // esi@56
	char *v16; // eax@56
	__int16 v17; // ax@61
	__int16 v18; // ax@66
	__int16 v19; // ax@71
	int v20; // ebx@76
	char v21; // cl@76
	char dinput_mouse; // dl@84
	int v23; // eax@85
	char v24; // cl@86
	char v25; // dl@88
	char v26; // zf@88
	int v27; // ecx@89
	int v28; // ebp@122
	_UNKNOWN *v29; // esi@122
	int charID; // eax@123
	int v31; // ecx@129
	int v32; // eax@130
	int v33; // edx@135
	signed int v34; // eax@136
	signed int v35; // ecx@136
	signed __int16 v36; // ax@147
	signed __int16 v37; // cx@155
	signed __int16 v38; // ax@159
	int held; // edx@163
	int useless_1; // eax@163
	int useless_2; // ecx@163
	int held_dupe; // ecx@163
	int useless_3; // eax@163
	int pressed; // edx@163
	int v45; // ebx@164
	unsigned __int8 v46; // sf@169
	unsigned __int8 v47; // of@169
	LPVOID v48; // eax@176
	int held_something; // edx@178
	char dinput_mouse_dupe; // [sp+3h] [bp-21h]@84
	signed int v51; // [sp+4h] [bp-20h]@13
	int v52; // [sp+4h] [bp-20h]@76
	int v53; // [sp+8h] [bp-1Ch]@42
	int v54; // [sp+8h] [bp-1Ch]@76
	int v55; // [sp+Ch] [bp-18h]@6
	int v56; // [sp+14h] [bp-10h]@13
	int v57; // [sp+14h] [bp-10h]@76
	int v58; // [sp+18h] [bp-Ch]@13
	int v59; // [sp+18h] [bp-Ch]@76
	int v60; // [sp+1Ch] [bp-8h]@10
	signed int v61; // [sp+20h] [bp-4h]@76

	dword_3B0E340 = sub_77F060(0);
	if (dword_8929D4[0])
	{
		dword_3B0E764 = sub_77EFB0(0);
		dword_3B0E778 = sub_77EFA0(0);
	}
	else
	{
		dword_3B0E764 = (int)&dword_7DCC9C;
		dword_3B0E778 = (int)&dword_7DCCB0;
	}
	sub_40E900();
	v0 = dword_3B0E340;
	if (!dword_8929D4[0])
	{
		*(_DWORD *)(dword_3B0E340 + 2) = 0;
		*(_WORD *)(v0 + 6) = 0;
		*(_BYTE *)v0 = 0;
	}
	sub_40EE10();
	iterator_maybe = 0;
	dword_3B0EC24 = 0;
	v55 = 0;
	while (1)
	{
		v2 = v55;
		zerobutgetsreused = 0;
		iterator_dupe = iterator_maybe;
		if (v55)
			iterator_dupe = v55;
		v60 = iterator_dupe;
		controller_ptr = &Controller_Data_0[iterator_dupe];
		if (iterator_maybe)
		{
			if (v55 == iterator_maybe)
				goto LABEL_169;
			v2 = v55;
			zerobutgetsreused = 0;
		}
		v6 = 0;
		controller_ptr->RightStickY = zerobutgetsreused;
		controller_ptr->RightStickX = zerobutgetsreused;
		controller_ptr->LeftStickY = zerobutgetsreused;
		controller_ptr->LeftStickX = zerobutgetsreused;
		v51 = zerobutgetsreused;
		v56 = zerobutgetsreused;
		v58 = zerobutgetsreused;
		if (v2 == zerobutgetsreused)
		{
			if (byte_3B0E464 || byte_3B0E431)
				v51 = 4;
			if (byte_3B0E437 || byte_3B0E42B || byte_3B0E45B)
				v51 |= 2u;
			if (byte_3B0E458 || byte_3B0E4BE)
				v51 |= 8u;
			if (byte_3B0E3EC)
				v51 |= 0x400u;
			if (byte_3B0E422)
				v51 |= 0x200u;
			if (byte_3B0E41C)
				v51 |= 0x20000u;
			if (byte_3B0E42E)
				v51 |= 0x10000u;
			v7 = 0;
			do
			{
				v9 = *(_BYTE *)(dword_3B0E340 + v7 + 2);
				v8 = v9 - 89;
				if (v9 - 89 < 0 || v8 >= 9)
				{
					v10 = v9 - 79;
					if (v9 - 79 >= 0)
					{
						if (v10 < 4)
						{
							if (v10 >= 2)
								zerobutgetsreused += dword_90A014[v10 & 1];
							else
								v6 += dword_90A014[v10];
						}
					}
				}
				else
				{
					v6 += dword_90A01C[v8 % 3];
					zerobutgetsreused -= dword_90A01C[v8 / 3];
				}
				++v7;
			} while (v7 < 6);
			zerobutgetsreused += dword_90A020[(unsigned __int8)byte_3B0E3F2] - dword_90A020[(unsigned __int8)byte_3B0E41F];
			v56 = dword_90A020[(unsigned __int8)byte_3B0E40A] - dword_90A020[(unsigned __int8)byte_3B0E407];
			v61 = v7;
			v53 = dword_90A020[(unsigned __int8)byte_3B0E3FB] - dword_90A020[(unsigned __int8)byte_3B0E3F5] + v6;
			v58 = dword_90A020[(unsigned __int8)byte_3B0E404] - dword_90A020[(unsigned __int8)byte_3B0E410];
			v12 = GetCharObj2(0);
			v11 = (unsigned __int64)(atan2((double)v53, (double)zerobutgetsreused) * 65536.0 * 0.1591549762031479);
			if (v53 | zerobutgetsreused && qword_3B0EC1C)
			{
				if (dword_3B0EC18)
				{
					--dword_3B0EC18;
					v53 = qword_3B0EC1C;
					zerobutgetsreused = HIDWORD(qword_3B0EC1C);
				}
				else
				{
					if (v11 - dword_3B0EC14 <= (signed int)0x4000u || dword_3B0EC14 - v11 <= (signed int)0x4000u)
					{
						if (v12)
						{
							if (dword_3B0EC10)
								v13 = v12->PhysicsData.Run1;
							else
								v13 = v12->PhysicsData.RollEnd;
							if (fabs(*(float *)(dword_3B36DD0[0] + 56) * 5.0 + v12->HSpeed) >= v13)
							{
								v53 = ((_DWORD)qword_3B0EC1C + v53) / 2;
								zerobutgetsreused = (zerobutgetsreused + HIDWORD(qword_3B0EC1C)) / 2;
								dword_3B0EC18 = dword_90A010;
							}
						}
					}
				}
			}
			else
			{
				dword_3B0EC18 = 0;
			}
			dword_3B0EC14 = v11;
			qword_3B0EC1C = __PAIR__(zerobutgetsreused, v53);
			v6 = v53;
		}
		v16 = sub_77F120(dword_90A000[v55]);
		v15 = v16;
		v14 = *((_WORD *)v16 + 14);
		if (v14 > -60)
		{
			if (v14 < 60)
				*((_WORD *)v15 + 14) = (unsigned __int64)((double)v14 * 2.133333444595337);
			else
				*((_WORD *)v15 + 14) = 127;
		}
		else
		{
			*((_WORD *)v15 + 14) = -128;
		}
		v17 = *((_WORD *)v15 + 15);
		if (v17 > -60)
		{
			if (v17 < 60)
				*((_WORD *)v15 + 15) = (unsigned __int64)((double)v17 * 2.133333444595337);
			else
				*((_WORD *)v15 + 15) = 127;
		}
		else
		{
			*((_WORD *)v15 + 15) = -128;
		}
		v18 = *((_WORD *)v15 + 16);
		if (v18 > -60)
		{
			if (v18 < 60)
				*((_WORD *)v15 + 16) = (unsigned __int64)((double)v18 * 2.133333444595337);
			else
				*((_WORD *)v15 + 16) = 127;
		}
		else
		{
			*((_WORD *)v15 + 16) = -128;
		}
		v19 = *((_WORD *)v15 + 17);
		if (v19 > -60)
		{
			if (v19 < 60)
				*((_WORD *)v15 + 17) = (unsigned __int64)((double)v19 * 2.133333444595337);
			else
				*((_WORD *)v15 + 17) = 127;
		}
		else
		{
			*((_WORD *)v15 + 17) = -128;
		}
		v52 = *((_DWORD *)v15 + 2) | v51;
		v54 = *((_WORD *)v15 + 14) + v6;
		v20 = *((_WORD *)v15 + 15) + zerobutgetsreused;
		v21 = byte_3B0E9A4[v61];
		v57 = *((_WORD *)v15 + 16) + v56;
		v59 = *((_WORD *)v15 + 17) + v58;
		controller_ptr->Support = 0x3F07FEu;
		if (v21)
		{
			controller_ptr->HeldButtons = dword_3B0E9B8[v61];
			byte_3B0E9A4[v61] = 0;
		}
		if (v55)
			goto LABEL_147;
		if (v54 || v20)
		{
			dword_3B0E998 = 0;
			dword_3B0E99C = 0;
			dword_3B0E9A0 = 0;
			dword_3B0E990 = 0;
			dword_3B0E994 = 0;
		}
		else
		{
			if (dword_3B0E998 > 0)
			{
				v20 = dword_3B0E9A0;
				v54 = -dword_3B0E99C;
			}
		}
		dinput_mouse = GetDInputMouseState();
		dinput_mouse_dupe = dinput_mouse;
		if (dinput_mouse)
		{
			v24 = dinput_mouse;
			if (dword_3B0EBF8 == (unsigned __int8)dinput_mouse)
			{
				v23 = dword_3B0EC0C;
				dword_3B0EBFC[dword_3B0EC0C] = dword_3B0EBFC[(dword_3B0EC0C - 1) & 3];
			}
			else
			{
				v26 = ((unsigned __int8)dinput_mouse & (unsigned __int8)(dinput_mouse ^ dword_3B0EBF8)) == 0;
				v25 = dinput_mouse & (dinput_mouse ^ dword_3B0EBF8);
				v23 = dword_3B0EC0C;
				if (v26)
				{
					if (v24 & 1)
					{
						dword_3B0EBFC[dword_3B0EC0C] = 1;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 2)
					{
						dword_3B0EBFC[v23] = 2;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 4)
					{
						dword_3B0EBFC[v23] = 3;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 8)
					{
						dword_3B0EBFC[v23] = 4;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 0x10)
					{
						dword_3B0EBFC[v23] = 5;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 0x20)
					{
						dword_3B0EBFC[v23] = 6;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 & 0x40)
					{
						dword_3B0EBFC[v23] = 7;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v24 >= 0)
						goto LABEL_122;
					dword_3B0EBFC[v23] = 8;
				}
				else
				{
					v27 = dword_3B0EBFC[(dword_3B0EC0C - 1) & 3] << 8;
					dword_3B0EBFC[dword_3B0EC0C] = v27;
					if (v25 & 1)
					{
						dword_3B0EBFC[v23] = v27 | 1;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 2)
					{
						dword_3B0EBFC[v23] |= 2u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 4)
					{
						dword_3B0EBFC[v23] |= 3u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 8)
					{
						dword_3B0EBFC[v23] |= 4u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 0x10)
					{
						dword_3B0EBFC[v23] |= 5u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 0x20)
					{
						dword_3B0EBFC[v23] |= 6u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 & 0x40)
					{
						dword_3B0EBFC[v23] |= 7u;
						v23 = (v23 + 1) & 3;
						dword_3B0EC0C = v23;
					}
					if (v25 >= 0)
						goto LABEL_122;
					dword_3B0EBFC[v23] |= 8u;
				}
			}
		}
		else
		{
			v23 = dword_3B0EC0C;
			dword_3B0EBFC[dword_3B0EC0C] = 0;
		}
		v23 = (v23 + 1) & 3;
		dword_3B0EC0C = v23;
	LABEL_122:
		v28 = *(_DWORD *)&Controller_Data_0[5].RTriggerPressure;
		v29 = (_UNKNOWN *)word_3B0E6E0;
		if (RAM_dword_ptr_P1_Obj1)
		{
			charID = GetCharacterID(0);
			if (charID >= 0)
			{
				if (charID < 8)
				{
					if (RAM_dword_ptr_P1_Obj1->Action == dword_909FE0[charID])
					{
						if (RAM_dword_ptr_P1_Obj1->NextAction == 18)
						{
							v28 = dword_909FC8;
							v29 = &unk_909FB8;
							dword_3B36D34 = 1;
						}
					}
				}
			}
			v23 = dword_3B0EC0C;
		}
		v31 = 0;
		if (v28 > 0)
		{
			v32 = dword_3B0EBFC[(v23 - 1) & 3];
			while (v32 != *((_WORD *)v29 + 2 * v31))
			{
				++v31;
				if (v31 >= v28)
					goto LABEL_135;
			}
			v52 |= *((_WORD *)v29 + 2 * v31 + 1);
		}
	LABEL_135:
		v33 = *(_DWORD *)dword_3B0E778;
		dword_3B0EBF8 = (unsigned __int8)dinput_mouse_dupe;
		if (v33 & 0x20000)
		{
			v35 = 20;
			v34 = 1;
			dword_3B0EBF4 = 20;
			dword_3B0EBF0 = 1;
		}
		else
		{
			v35 = dword_3B0EBF4;
			v34 = dword_3B0EBF0;
		}
		if (v33 & 0x10000)
		{
			v35 = 20;
			dword_3B0EBF0 = 2;
			goto LABEL_144;
		}
		if (!v35)
		{
			dword_3B0EBF0 = 0;
			goto LABEL_147;
		}
		if (v34 != 1)
		{
			if (v34 == 2)
			LABEL_144:
			v52 |= 0x10000u;
			dword_3B0EBF4 = v35 - 1;
			goto LABEL_147;
		}
		v52 |= 0x20000u;
		dword_3B0EBF4 = v35 - 1;
	LABEL_147:
		v36 = v54;
		if (v54 >= -127)
		{
			if (v54 > 127)
				v36 = 127;
		}
		else
		{
			v36 = -127;
		}
		if (v20 >= -127)
		{
			if (v20 > 127)
				LOWORD(v20) = 127;
		}
		else
		{
			LOWORD(v20) = -127;
		}
		v37 = v57;
		controller_ptr->LeftStickX = v36;
		controller_ptr->LeftStickY = v20;
		if (v57 >= -127)
		{
			if (v57 > 127)
				v37 = 127;
		}
		else
		{
			v37 = -127;
		}
		v38 = v59;
		if (v59 >= -127)
		{
			if (v59 > 127)
				v38 = 127;
		}
		else
		{
			v38 = -127;
		}
		held = controller_ptr->HeldButtons;
		controller_ptr->RightStickY = v38;
		controller_ptr->RightStickX = v37;
		useless_1 = v52;
		useless_2 = v52;
		controller_ptr->LTriggerPressure = (useless_1 & 0x20000) != 0 ? 0xFF : 0;
		controller_ptr->NotHeldButtons = ~v52;
		controller_ptr->RTriggerPressure = (useless_2 & 0x10000) != 0 ? 0xFF : 0;
		held_dupe = held;
		useless_3 = v52 ^ held;
		controller_ptr->Old = held;
		pressed = v52 & (v52 ^ held);
		controller_ptr->ReleasedButtons = held_dupe & useless_3;
		controller_ptr->HeldButtons = v52;
		controller_ptr->PressedButtons = pressed;
		v61 = v60;
		dword_3B0E344[v60] = pressed;
		if (v52 == held_dupe)
		{
			v45 = dword_3B0E768[v60] + 1;
			dword_3B0E768[v60] = v45;
			if (v45 == 15)
			{
				dword_3B0E344[v60] = v52;
			}
			else
			{
				if (v45 == 19)
				{
					dword_3B0E344[v60] = v52;
					dword_3B0E768[v60] = 15;
				}
			}
		}
		else
		{
			dword_3B0E768[v60] = 0;
		}
	LABEL_169:
		v47 = __SETO__(v55 + 1, 4);
		v46 = v55++ - 3 < 0;
		if (!(v46 ^ v47))
			break;
		iterator_maybe = dword_3B0EC24;
	}
	if (RAM_word_Game_Status == 15)
	{
		if (dword_3B2A2E4)
		{
			if (dword_3B2C470)
			{
				if (dword_3B2A2E8 < 0)
				{
					v48 = dword_3B2C480;
					if (dword_3B2C474 == 1)
					{
						if (word_3B2C464 > word_3B2C460
							|| (held_something = *((_DWORD *)dword_3B2C480 + 6 * word_3B2C464), held_something == -1))
						{
							dword_3B2C470 = 0;
							StartLevelCutscene(6);
							++word_3B2C464;
						}
						else
						{
							Controller_Data_0_ptr->HeldButtons = held_something | Controller_Data_0_ptr->HeldButtons & 8;
							Controller_Data_0_ptr->LTriggerPressure = *((_WORD *)v48 + 12 * word_3B2C464 + 2);
							Controller_Data_0_ptr->RTriggerPressure = *((_WORD *)v48 + 12 * word_3B2C464 + 3);
							Controller_Data_0_ptr->LeftStickX = *((_WORD *)v48 + 12 * word_3B2C464 + 4);
							Controller_Data_0_ptr->LeftStickY = *((_WORD *)v48 + 12 * word_3B2C464 + 5);
							Controller_Data_0_ptr->NotHeldButtons = *((_DWORD *)v48 + 6 * word_3B2C464 + 3);
							Controller_Data_0_ptr->PressedButtons = Controller_Data_0_ptr->PressedButtons & 8 | *((_DWORD *)v48
								+ 6 * word_3B2C464
								+ 4);
							Controller_Data_0_ptr->ReleasedButtons = *((_DWORD *)v48 + 6 * word_3B2C464++ + 5);
						}
					}
				}
			}
		}
	}
}
*/