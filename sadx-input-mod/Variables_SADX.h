#pragma once

DataPointer(HWND, hWnd, 0x3D0FD30);
DataPointer(char, SoftResetByte, 0x3B0EAA0);
DataPointer(int, Demo_Enabled, 0x3B2C470);
DataPointer(int, Demo_Cutscene, 0x3B2A2E8);
DataPointer(int, Demo_ControlMode, 0x3B2C474);
DataPointer(__int16, Demo_Frame, 0x3B2C464);
DataPointer(__int16, Demo_MaxFrame, 0x3B2C460);
DataArray(KeyboardKey, KeyboardKeys, 0x03B0E3E0, 256);
DataPointer(KeyboardInput*, KeyboardInputPointer, 0x3B0E340);