#include "stdafx.h"
#include "KeyboardRemap.h"

Uint32 KButton_A = 0;
Uint32 KButton_B = 0;
Uint32 KButton_X = 0;
Uint32 KButton_Y = 0;
Uint32 KButton_Z = 0;
Uint32 KButton_C = 0;
Uint32 KButton_D = 0;
Uint32 KButton_Start = 0;
Uint32 KButton_L = 0;
Uint32 KButton_R = 0;
Uint32 KButton_Up = 0;
Uint32 KButton_Down = 0;
Uint32 KButton_Left = 0;
Uint32 KButton_Right = 0;
Uint32 KButton_DPadUp = 0;
Uint32 KButton_DPadDown = 0;
Uint32 KButton_DPadLeft = 0;
Uint32 KButton_DPadRight = 0;
Uint32 KButton_Center = 0;

Uint32 KButton2_A = 0;
Uint32 KButton2_B = 0;
Uint32 KButton2_X = 0;
Uint32 KButton2_Y = 0;
Uint32 KButton2_Z = 0;
Uint32 KButton2_C = 0;
Uint32 KButton2_D = 0;
Uint32 KButton2_Start = 0;
Uint32 KButton2_L = 0;
Uint32 KButton2_R = 0;
Uint32 KButton2_Up = 0;
Uint32 KButton2_Down = 0;
Uint32 KButton2_Left = 0;
Uint32 KButton2_Right = 0;
Uint32 KButton2_DPadUp = 0;
Uint32 KButton2_DPadDown = 0;
Uint32 KButton2_DPadLeft = 0;
Uint32 KButton2_DPadRight = 0;
Uint32 KButton2_Center = 0;

Uint32 KButton3_A = 0;
Uint32 KButton3_B = 0;
Uint32 KButton3_X = 0;
Uint32 KButton3_Y = 0;
Uint32 KButton3_Z = 0;
Uint32 KButton3_C = 0;
Uint32 KButton3_D = 0;
Uint32 KButton3_Start = 0;
Uint32 KButton3_L = 0;
Uint32 KButton3_R = 0;
Uint32 KButton3_Up = 0;
Uint32 KButton3_Down = 0;
Uint32 KButton3_Left = 0;
Uint32 KButton3_Right = 0;
Uint32 KButton3_DPadUp = 0;
Uint32 KButton3_DPadDown = 0;
Uint32 KButton3_DPadLeft = 0;
Uint32 KButton3_DPadRight = 0;
Uint32 KButton3_Center = 0;

bool CenterKey = false;

Uint32 FindKey(std::string KeyString)
{
	for (int i = 0; i < LengthOfArray(KeyArray); i++)
	{
		if (KeyArray[i].KeyConfigName == KeyString) return KeyArray[i].WindowsCode;
	}
	if (KeyString == "None") return 0;
	PrintDebug("Incorrect key '%s'\n", KeyString.c_str());
	return 0;
}

char GetEKey(char index)
{
	if (CenterKey) return 1;
	else return 0;
}

void ClearVanillaKeys()
{
	for (int i = 0; i < LengthOfArray(KeyArray); i++)
	{
		KeyArray[i].VanillaKeyPointer.pressed = 0;
	}
}

void SetVanillaSADXKey(Uint32 key, bool down)
{
	for (int i = 0; i < LengthOfArray(KeyArray); i++)
	{
		if (key == KeyArray[i].WindowsCode)
		{
			if (input::debug) PrintDebug("Key match: %s to code %X, previous state: %d, new state: %d", KeyArray[i].KeyConfigName.c_str(), KeyArray[i].WindowsCode, KeyArray[i].VanillaKeyPointer.old, KeyArray[i].VanillaKeyPointer.held);
			KeyArray[i].VanillaKeyPointer.old = KeyArray[i].VanillaKeyPointer.held;
			if (down && KeyArray[i].VanillaKeyPointer.old == 0)
			{
				KeyArray[i].VanillaKeyPointer.pressed = 1;
				if (input::debug) PrintDebug(" (pressed)\n");
			}
			else
			{
				KeyArray[i].VanillaKeyPointer.pressed = 0;
				if (input::debug) PrintDebug(" (held)\n");
			}
			KeyArray[i].VanillaKeyPointer.held = down;
		}
	}
}
