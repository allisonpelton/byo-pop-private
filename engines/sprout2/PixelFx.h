#ifndef __SPROUT2_PIXELFX_H__
#define __SPROUT2_PIXELFX_H__

// An effect computed per pixel (engine::PixelFx, vtable 0x55c0dc (feedingfrenzy2): ColorFx, RainbowShiftFx and
// HSVShiftFx), put on a surface's pixels when it is copied (the surface's slots 0x34, 0x38 and 0x54), or set on a
// component (Component::setPixelFx) and animated by its update.

#include "Geometry.h"
#include "Object.h"

#include <cstdint>
#include <string>

namespace engine
{

// One pixel's channels as bytes
struct PixelColor
{
	uint8_t mRed;
	uint8_t mGreen;
	uint8_t mBlue;
	uint8_t mAlpha;
};

class PixelFx : public virtual Interface
{
public:
	// The pixel at the column and row changed (the effects' slot 0x00). By default left as it is
	virtual void shadePixel(int theColumn, int theRow, PixelColor& thePixel) const {}
	// The effect's name, for the names of the surfaces it makes (the effects' slot 0x04)
	virtual std::string getFxName() const { return ""; }
	// A component's effect advanced by the frame's time (the BltFx's slot 0x04; the pixel effects have none)
	virtual void update(float theElapsed) {}
};

// How a ColorFx changes a pixel's colour (its +0x18)
enum ColorFxMode
{
	COLOR_FX_NONE = 0,       // the pixel left as it is, and any other value
	COLOR_FX_ADD = 1,        // 0x509364 (feedingfrenzy2): each channel, alpha too, plus the colour's times 255,
	                         // truncated and held to 0 to 255
	COLOR_FX_MULTIPLY = 2,   // each channel, alpha too, times the colour's
	COLOR_FX_BLEND = 3       // the colour laid over by its alpha, weighted by the pixel's alpha; the alpha kept
};

// A colour put on the pixels (ColorFx, vtables 0x55c16c, 0x55c178 and 0x55c188, its constructor 0x40e820
// (feedingfrenzy2))
class ColorFx : public Object, public PixelFx
{
public:
	ColorFx(const Color& theColor, int theMode) : mColor(theColor), mMode(theMode) {}

	// 0x00 (0x43e2e0 (feedingfrenzy2), and as the surface's slots 0x38 and 0x50 inline it): each channel's sum at
	// the x87's double precision, truncated
	void shadePixel(int theColumn, int theRow, PixelColor& thePixel) const override;
	// 0x04 (0x40cf00 (feedingfrenzy2))
	std::string getFxName() const override { return "ColorFx"; }

	Color mColor;  // +0x08
	int mMode;     // +0x18: a ColorFxMode
};

// The pixels' hue, saturation and value shifted (HSVShiftFx, vtables 0x55c134, 0x55c140 and 0x55c150, 0x14 bytes;
// its constructor 0x484060 (feedingfrenzy2)), for a visual's colour effect hsvAdd
class HSVShiftFx : public Object, public PixelFx
{
public:
	// 0x484060 and 0x412db0 (feedingfrenzy2)
	HSVShiftFx(float theHue, float theSaturation, float theValue)
	    : mHue(theHue), mSaturation(theSaturation), mValue(theValue)
	{
	}

	// 0x00 (0x43eed0 (feedingfrenzy2), as the surface's slot 0x38 inlines it, 0x43e927 (feedingfrenzy2)): the colour
	// taken out of the alpha, turned to its hue (in degrees), saturation and value, the shifts added (the hue brought
	// back into 0 to 360, the others held to 0 to 1), and turned back, times the alpha again. A pixel of alpha 0 is
	// left as it is, its alpha 0. Its sextant 0 gives the colours of the sextant 5 (red, then the least, then the
	// falling one) where it would give red, the rising one and the least
	void shadePixel(int theColumn, int theRow, PixelColor& thePixel) const override;
	// 0x04 (0x4840e0 (feedingfrenzy2))
	std::string getFxName() const override { return "HSVShiftFx"; }

	float mHue;          // +0x08: degrees added
	float mSaturation;   // +0x0c
	float mValue;        // +0x10
};

// The pixels given a hue that turns along the columns (RainbowShiftFx, vtables 0x55c0f8, 0x55c104 and 0x55c114, 0x10
// bytes; its constructor 0x4118d0 (feedingfrenzy2)), keeping each pixel's saturation and value, for a visual's colour
// effect rainbow
class RainbowShiftFx : public Object, public PixelFx
{
public:
	// 0x4118d0 and 0x408c80 (feedingfrenzy2)
	RainbowShiftFx(float theHue, float theHuePerColumn) : mHue(theHue), mHuePerColumn(theHuePerColumn) {}

	// 0x00 (0x43f2b0 (feedingfrenzy2), as the surface's slot 0x38 inlines it, 0x43e7f0 (feedingfrenzy2)): the hue
	// the column times the turn plus the start (brought into 0 to 360); the alpha kept
	void shadePixel(int theColumn, int theRow, PixelColor& thePixel) const override;
	// 0x04 (0x4118b0 (feedingfrenzy2))
	std::string getFxName() const override { return "RainbowShiftFx"; }

	float mHue;            // +0x08: degrees at the first column
	float mHuePerColumn;   // +0x0c
};

} // namespace engine

#endif
