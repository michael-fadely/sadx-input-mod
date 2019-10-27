#pragma once

#include <cstdint>

struct Point2I
{
	int16_t x;
	int16_t y;
};

enum PDD_DEV_SUPPORT : uint32_t
{
	//	Right stick Y
	PDD_DEV_SUPPORT_AY2 = (1 << 21),
	//	Right stick X
	PDD_DEV_SUPPORT_AX2 = (1 << 20),
	//	Left stick Y
	PDD_DEV_SUPPORT_AY1 = (1 << 19),
	//	Left stick X
	PDD_DEV_SUPPORT_AX1 = (1 << 18),
	//	Analog trigger L
	PDD_DEV_SUPPORT_AL = (1 << 17),
	//	Analog trigger R
	PDD_DEV_SUPPORT_AR = (1 << 16),
	//	D-Pad B Right
	PDD_DEV_SUPPORT_KRB = (1 << 15),
	//	D-Pad B Left
	PDD_DEV_SUPPORT_KLB = (1 << 14),
	//	D-Pad B Down
	PDD_DEV_SUPPORT_KDB = (1 << 13),
	//	D-Pad B Up
	PDD_DEV_SUPPORT_KUB = (1 << 12),
	//	D button
	PDD_DEV_SUPPORT_TD = (1 << 11),
	//	X button
	PDD_DEV_SUPPORT_TX = (1 << 10),
	//	Y button
	PDD_DEV_SUPPORT_TY = (1 << 9),
	//	Z button
	PDD_DEV_SUPPORT_TZ = (1 << 8),
	//	D-Pad A Right
	PDD_DEV_SUPPORT_KR = (1 << 7),
	//	D-Pad A Left
	PDD_DEV_SUPPORT_KL = (1 << 6),
	//	D-Pad A Down
	PDD_DEV_SUPPORT_KD = (1 << 5),
	//	D-Pad A Up
	PDD_DEV_SUPPORT_KU = (1 << 4),
	//	Start button
	PDD_DEV_SUPPORT_ST = (1 << 3),
	//	A button
	PDD_DEV_SUPPORT_TA = (1 << 2),
	//	B button
	PDD_DEV_SUPPORT_TB = (1 << 1),
	//	C button
	PDD_DEV_SUPPORT_TC = (1 << 0),
};

enum PDD_DGT
{
	PDD_DGT_TL  = (1 << 17),    /* L button (emulation) */
	PDD_DGT_TR  = (1 << 16),    /* R button (emulation) */
	PDD_DGT_KRB = (1 << 15),    /* Right button B       */
	PDD_DGT_KLB = (1 << 14),    /* Left button B        */
	PDD_DGT_KDB = (1 << 13),    /* Down button B        */
	PDD_DGT_KUB = (1 << 12),    /* Up button B          */
	PDD_DGT_TD  = (1 << 11),    /* D button             */
	PDD_DGT_TX  = (1 << 10),    /* X button             */
	PDD_DGT_TY  = (1 << 9),     /* Y button             */
	PDD_DGT_TZ  = (1 << 8),     /* Z button             */
	PDD_DGT_KR  = (1 << 7),     /* Right button A       */
	PDD_DGT_KL  = (1 << 6),     /* Left button A        */
	PDD_DGT_KD  = (1 << 5),     /* Down button A        */
	PDD_DGT_KU  = (1 << 4),     /* Up button A          */
	PDD_DGT_ST  = (1 << 3),     /* Start button         */
	PDD_DGT_TA  = (1 << 2),     /* A button             */
	PDD_DGT_TB  = (1 << 1),     /* B button             */
	PDD_DGT_TC  = (1 << 0),     /* C button             */
};

struct DCControllerData
{
	uint32_t ID;
	uint32_t Support;
	uint32_t HeldButtons;
	uint32_t NotHeldButtons;
	uint32_t PressedButtons;
	uint32_t ReleasedButtons;
	uint16_t RTriggerPressure;
	uint16_t LTriggerPressure;
	int16_t  LeftStickX;
	int16_t  LeftStickY;
	int16_t  RightStickX;
	int16_t  RightStickY;
	char*    Name;
	void*    Extend;
	uint32_t Old;
	void*    Info;
};
