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

extern KeyData SADX2004Keys[];