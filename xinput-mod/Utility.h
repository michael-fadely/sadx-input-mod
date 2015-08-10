#pragma once

// Utility functions
// TODO: Separate utility functions into separate file.
void ConvertAxes(float scaleFactor, short dest[2], short source[2], short deadzone = 0, bool radial = true);
int ConvertButtons(short buttons);
