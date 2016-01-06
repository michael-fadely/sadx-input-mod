#pragma once
#include <SADXModLoader.h>
namespace rumble
{
	enum PDD_VIB_FLAG
	{
		PDD_VIB_FLAG_CONTINUOUS = 0x1,
		PDD_VIB_FLAG_EXHALATION = 0x8,
		PDD_VIB_FLAG_CONVERGENCE = 0x80,
	};

	typedef struct {
		Uint8 unit;                          /* ユニット番号                   */
											 /* Unit number                    */
		Uint8 flag;                          /* 振動フラグ                     */
											 /* Vibration flag                 */
		Sint8 power;                         /* 強さ                           */
											 /* Power                          */
		Uint8 freq;                          /* 振動周波数                     */
											 /* Frequency                      */
		Uint8 inc;                           /* 振動勾配周期                   */
											 /*                                */
		Uint8 reserved[3];                   /* 予約                           */
											 /* Reserved                       */
	} PDS_VIBPARAM;

	// HACK: Update mod loader
	enum LoadObj : __int8
	{
		LoadObj_CharObj2 = 0x1,
		LoadObj_CharObj1 = 0x2,
		LoadObj_UnknownA = 0x4,
		LoadObj_UnknownB = 0x8,
	};

	// HACK: Update mod loader
#pragma pack(push, 1)
	struct ObjUnknownB
	{
		int field_0;
		int Index;
		int Mode;
		int field_12;
	};
#pragma pack(pop)

	//FunctionPointer(Sint32, pdVibMxStart, (Uint32 port, const PDS_VIBPARAM *a2), 0x00785280);
	FunctionPointer(Sint32, pdVibMxStop, (Uint32 port), 0x00785330);

	Sint32 __cdecl pdVibMxStop_hook(Uint32 port);
	void __cdecl Rumble_Load_hook(Uint32 port, Uint32 time, Motor motor);
	void __cdecl RumbleA(Uint32 port, Uint32 time);
	void __cdecl RumbleB(Uint32 port, Uint32 time, int a3, int a4);
}
