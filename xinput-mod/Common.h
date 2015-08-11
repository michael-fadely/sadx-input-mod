#pragma once
// TODO: move
#include <Windows.h>	// Required for XInput
#include <XInput.h>		// XUSER_MAX_COUNT

#include <limits>		// for SHRT_MAX
#include "minmax.h"		// for safe min, max, clamp
#include "typedefs.h"

// Re-defining so it can be changed easily, and because XUSER_MAX_COUNT is far too long.
// TODO: move
#define XPAD_COUNT XUSER_MAX_COUNT
