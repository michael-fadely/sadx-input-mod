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

template <typename T>
T clamp(T value, T low, T high)
{
	return min(max(low, value), high);
}
