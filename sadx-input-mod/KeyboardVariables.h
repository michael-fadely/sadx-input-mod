#pragma once

extern Uint32 KButton_A;
extern Uint32 KButton_B;
extern Uint32 KButton_X;
extern Uint32 KButton_Y;
extern Uint32 KButton_Z;
extern Uint32 KButton_C;
extern Uint32 KButton_D;
extern Uint32 KButton_Start;
extern Uint32 KButton_L;
extern Uint32 KButton_R;
extern Uint32 KButton_Up;
extern Uint32 KButton_Down;
extern Uint32 KButton_Left;
extern Uint32 KButton_Right;
extern Uint32 KButton_DPadUp;
extern Uint32 KButton_DPadDown;
extern Uint32 KButton_DPadLeft;
extern Uint32 KButton_DPadRight;
extern Uint32 KButton_Center;

extern Uint32 KButton2_A;
extern Uint32 KButton2_B;
extern Uint32 KButton2_X;
extern Uint32 KButton2_Y;
extern Uint32 KButton2_Z;
extern Uint32 KButton2_C;
extern Uint32 KButton2_D;
extern Uint32 KButton2_Start;
extern Uint32 KButton2_L;
extern Uint32 KButton2_R;
extern Uint32 KButton2_Up;
extern Uint32 KButton2_Down;
extern Uint32 KButton2_Left;
extern Uint32 KButton2_Right;
extern Uint32 KButton2_DPadUp;
extern Uint32 KButton2_DPadDown;
extern Uint32 KButton2_DPadLeft;
extern Uint32 KButton2_DPadRight;
extern Uint32 KButton2_Center;

extern Uint32 KButton3_A;
extern Uint32 KButton3_B;
extern Uint32 KButton3_X;
extern Uint32 KButton3_Y;
extern Uint32 KButton3_Z;
extern Uint32 KButton3_C;
extern Uint32 KButton3_D;
extern Uint32 KButton3_Start;
extern Uint32 KButton3_L;
extern Uint32 KButton3_R;
extern Uint32 KButton3_Up;
extern Uint32 KButton3_Down;
extern Uint32 KButton3_Left;
extern Uint32 KButton3_Right;
extern Uint32 KButton3_DPadUp;
extern Uint32 KButton3_DPadDown;
extern Uint32 KButton3_DPadLeft;
extern Uint32 KButton3_DPadRight;
extern Uint32 KButton3_Center;

extern bool CenterKey;

Uint32 FindKey(std::string KeyString);
void SetVanillaSADXKey(Uint32 key, bool down);
char GetEKey(char index);
void ClearVanillaKeys();