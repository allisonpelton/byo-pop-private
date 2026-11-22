#ifndef __SPROUT2_PIXELFX_H__
#define __SPROUT2_PIXELFX_H__

// An effect computed per pixel as a component's images are drawn (engine::PixelFx, vtable 0x55c0dc (feedingfrenzy2):
// ColorFx, RainbowShiftFx and HSVShiftFx), set on a component (Component::setPixelFx) and animated by its update.

#include "Geometry.h"
#include "Object.h"

namespace engine
{

class PixelFx : public virtual Interface
{
public:
	// 0x04: the effect advanced by the frame's time
	virtual void update(float theElapsed) = 0;
};

// How a ColorFx changes a pixel's colour (its +0x18)
enum ColorFxMode
{
	COLOR_FX_NONE = 0,       // the pixel left as it is, and any other value
	COLOR_FX_HSV = 1,        // 0x509364 (feedingfrenzy2): not yet translated
	COLOR_FX_MULTIPLY = 2,   // each channel, alpha too, times the colour's
	COLOR_FX_BLEND = 3       // the colour laid over by its alpha, weighted by the pixel's alpha; the alpha kept
};

// A colour put on the pixels (ColorFx, vtables 0x55c16c, 0x55c178 and 0x55c188, its constructor 0x40e820
// (feedingfrenzy2))
class ColorFx : public Object, public PixelFx
{
public:
	ColorFx(const Color& theColor, int theMode) : mColor(theColor), mMode(theMode) {}

	void update(float theElapsed) override {}

	Color mColor;  // +0x08
	int mMode;     // +0x18: a ColorFxMode
};

} // namespace engine

#endif
