#ifndef __SPROUT_COLOR4_H__
#define __SPROUT_COLOR4_H__

// The signal build's colour (Pizza Frenzy's): red, green, blue and alpha as floats, 0 to 1 (16 bytes, default
// 0x47e4b0 (pizzafrenzy): all four 1). The layout files give them as bytes, and a colour attribute reads
// "r, g, b" or "r, g, b, a" (Layout.h).

namespace engine
{

struct Color4
{
	float r, g, b, a;

	Color4() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}  // 0x47e4b0 (pizzafrenzy)
	Color4(float theR, float theG, float theB, float theA) : r(theR), g(theG), b(theB), a(theA) {}
	// 0x47e5a0 (pizzafrenzy): the bytes over 255, the alpha 1
	static Color4 fromBytes(int theR, int theG, int theB)
	{
		return Color4((float)theR * 0.003921569f, (float)theG * 0.003921569f, (float)theB * 0.003921569f, 1.0f);
	}
};

} // namespace engine

#endif
