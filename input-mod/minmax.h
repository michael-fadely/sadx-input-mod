#pragma once

// Disable preprocessor macro min/max
#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

// Force std::min/max instead for type consistency
#include <algorithm>

using std::min;
using std::max;

#define clamp(value, low, high) min(max(low, value), high)
