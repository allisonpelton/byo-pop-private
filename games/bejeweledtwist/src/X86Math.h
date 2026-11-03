// port: the original's float-to-int conversion (_ftol2, 0x77dfe0) truncates and gives 0x80000000 for NaN and for
// values out of range. C++ leaves those undefined (and WebAssembly may trap), so the port converts through Ftol.
#pragma once

#include <climits>

namespace Sexy {

inline int Ftol(double theValue)
{
	if (!(theValue > -2147483649.0 && theValue < 2147483648.0))
		return INT_MIN;
	return (int)theValue;
}

}  // namespace Sexy
