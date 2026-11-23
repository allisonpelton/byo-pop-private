#ifndef __SPROUT_GRAPHICS_H__
#define __SPROUT_GRAPHICS_H__

// The engine's render context (ENGINE.md, "The render context"): a stack of render states and the drawing calls that
// use the top one. The base class (vtable 0x484cf8 (wordharmony)) keeps the states; the Windows layer's subclass
// (0x45dcb0 (wordharmony), vtable 0x48ac14 (wordharmony)) drew over DirectDraw with Sprout's blitters. The port's
// subclass draws with the framework (Display.cpp).

#include "BltFx.h"
#include "Build.h"
#include "Color4.h"
#include "Geometry.h"
#include "Object.h"
#include <cstdint>
#include <deque>

namespace engine
{

class Font;
class Graphics;
class PixelFx;

// The alpha modes of an image (its slot 0x18)
enum
{
	ALPHA_NONE = 0,   // drawn opaque (JPEGs, PNGs without alpha)
	ALPHA_TEST = 1,   // PNGs with alpha (threshold 0), or alphaType="alphaTest" in a screen
	ALPHA_BLEND = 2   // images with a mask, or alphaType="alphaBlend"
};

// An image: the interface the Windows layer's image (0x457410 (wordharmony), 0x48 bytes) keeps at +0x0c (vtable
// 0x48aba8 (wordharmony); its base 0x45e670 (wordharmony) holds the pivot and the alpha mode). The host's image
// implements the size and the pixels.
class Image : public Object
{
public:
	// 0x45e670 (wordharmony)
	Image() : mAlphaMode(ALPHA_NONE), mAlphaThreshold(0) {}
	virtual int getWidth() = 0;   // slot 0x00: 0x4572a0 (wordharmony)
	virtual int getHeight() = 0;  // slot 0x04: 0x4572b0 (wordharmony)
	// slot 0x0c: 0x45e5d0 (wordharmony), 0x4cf090 (pizzafrenzy), 1 puts the pivot at the centre (in whole pixels);
	// other values leave it, except in the signal build, where they put it back at the corner
	void setPivot(int theMode)
	{
		if (theMode == 1)
			setPivot(getWidth() / 2, getHeight() / 2);
		else if (gSignalBuild)
			setPivot(0, 0);
	}
	// The signal build's image operations (Pizza Frenzy's image, vtable 0x50aa98 (pizzafrenzy)); the host's image
	// implements them, and the defaults are an image that cannot be copied.
	// Slot 0x34 (0x4bea40 (pizzafrenzy)): a new image with the same pixels, alpha mode, threshold and pivot
	virtual Image* clone() { return nullptr; }
	// Slot 0x38 (0x4becb0 (pizzafrenzy)): the image resampled in place to the size the factors (their sizes) give, and
	// its pivot scaled with it
	virtual void scale(float theScaleX, float theScaleY) {}
	// Slot 0x2c (0x4be780 (pizzafrenzy)): a copy of the image scaled by the factors (its clone, scaled in place); an
	// image that has no clone gives itself back
	virtual Image* createScaled(float theScaleX, float theScaleY)
	{
		Image* aCopy = clone();
		if (aCopy == nullptr)
			return this;
		aCopy->scale(theScaleX, theScaleY);
		return aCopy;
	}
	// Slot 0x20 (0x4beec0 (pizzafrenzy)): every pixel the colour
	virtual void fill(const Color4& theColor) {}
	// Slot 0x3c (0x4be830 (pizzafrenzy)): every pixel replaced by what the filter makes of it
	virtual void applyFilter(PixelFx& theFilter) {}
	// slot 0x10: 0x45e5c0 (wordharmony), 0x4cf080 (pizzafrenzy)
	void setPivot(int x, int y) { mPivotX = x; mPivotY = y; }
	int getPivotX() const { return mPivotX; }                  // slot 0x14: 0x45e600 (wordharmony)
	int getPivotY() const { return mPivotY; }
	// slot 0x18: 0x45e610 (wordharmony)
	void setAlphaMode(int theMode, uint8_t theThreshold)
	{
		mAlphaMode = theMode;
		mAlphaThreshold = theThreshold;
	}
	int getAlphaMode() const { return mAlphaMode; }            // slot 0x1c: 0x45e630 (wordharmony)
	uint8_t getAlphaThreshold() const { return mAlphaThreshold; }

private:
	int mAlphaMode;            // +0x08
	uint8_t mAlphaThreshold;   // +0x0c
	int mPivotX = 0;           // +0x10
	int mPivotY = 0;
};

// A render state (0x40 bytes in the original, whose last two words nothing reads; 0x43c470 (wordharmony) makes the
// first)
struct RenderState
{
	Point mTranslation;      // +0x00, added to
	Point mScale;            // +0x08, multiplied (1, 1)
	float mRotation;         // +0x10, added to
	float mOpacity;          // +0x14, multiplied (1)
	Color mColor;            // +0x18, set
	Color mColorOffset;      // +0x24, added to
	Ref<Font> mFont;         // +0x30; +0x28 (pizzafrenzy)
	unsigned int mFlags;     // +0x34: the alignment bits below; +0x40 (pizzafrenzy)
	// The signal build's state (Pizza Frenzy's, 0x44 bytes, made by 0x477a70 (pizzafrenzy); it has no colour offset
	// and no snap field). Its flags hold the alignments in the same bits and the colour mode (bits 4 and 5) and the
	// smooth type (bit 6) above them; the port keeps those two apart. Its +0x30 is the clip rectangle, which is the
	// screen's.
	int mColorMode;          // +0x40 bits 4-5 (pizzafrenzy): 0 none, 1 add the colour, 2 multiply by the colour
	int mSmoothType;         // +0x40 bit 6 (pizzafrenzy): 1 draws without snapping the position to whole pixels
	Ref<BltFx> mBltFx;       // +0x2c (pizzafrenzy)
	float mColorAlpha;       // +0x24 (pizzafrenzy): the colour's alpha, set only with it (0x477ef0); the fill reads it

	RenderState() : mScale(1, 1), mRotation(0), mOpacity(1), mFlags(0), mColorMode(0), mSmoothType(0), mColorAlpha(1)
	{
	}
};

// The bit fields of RenderState::mFlags (masks and shifts at 0x497200..0x49721c (wordharmony))
enum
{
	SNAP_BOTH = 0,      // the position is rounded to whole pixels
	SNAP_Y = 1,         // only y is rounded
	SNAP_X = 2,         // only x is rounded
	SNAP_NONE = 3,
};

class Graphics
{
public:
	Graphics();          // 0x43d0a0 (wordharmony): one state, the defaults
	virtual ~Graphics(); // 0x43cb10 (wordharmony)

	// Slot 0: the image's theSrc part at (x, y), with the state's transform, opacity and colours
	virtual void drawImage(Image* theImage, const IntRect& theSrc, float x, float y) = 0;
	// Slot 2 (0x45dd70 (wordharmony)): the whole image at (0, 0)
	virtual void drawImage(Image* theImage);
	// Slot 3 (0x43cba0 (wordharmony)): a string with the state's font, glyph by glyph
	virtual void drawString(const char* theString);
	// Slot 4 (0x43c310 (wordharmony)): one character
	virtual void drawChar(char theChar);
	// Slot 7: a line in the state's colour
	virtual void drawLine(const Point& theFrom, const Point& theTo) = 0;
	// Slot 8 (0x4bfc20 (pizzafrenzy)): a filled rectangle in the state's colour (port: one line per pixel row)
	virtual void fillRect(const Rect& theRect);
	// 0x478320 (pizzafrenzy): a string at (x, y) (port: the text drawn at the origin of a translated state)
	void drawString(const char* theString, float x, float y);

	// 0x43d120 (wordharmony), 0x4788d0 (pizzafrenzy): pushes a copy of the top state
	void pushState();
	// 0x43ce10 (wordharmony), 0x4785a0 (pizzafrenzy)
	void popState();
	RenderState& state() { return mStates.back(); }  // 0x43c660 (wordharmony)

	// 0x43c5c0 (wordharmony), 0x477bc0 (pizzafrenzy)
	void setTranslation(float x, float y)
	{
		state().mTranslation.x = x;
		state().mTranslation.y = y;
	}
	void translate(float x, float y);       // 0x43c610 (wordharmony), 0x477c10 (pizzafrenzy)
	void scale(float sx, float sy);         // 0x43c690 (wordharmony), 0x477ce0 (pizzafrenzy)
	float getScaleX();                      // 0x43c6e0 (wordharmony), 0x477d30 (pizzafrenzy)
	float getScaleY();                      // 0x43c710 (wordharmony), 0x477d60 (pizzafrenzy)
	void rotate(float theAngle);            // 0x43c740 (wordharmony), 0x477d90 (pizzafrenzy)
	float getRotation();                    // 0x43c770 (wordharmony), 0x477dc0 (pizzafrenzy)
	void setOpacity(float theOpacity);      // 0x43c7a0 (wordharmony), 0x477df0 (pizzafrenzy)
	void multiplyOpacity(float theOpacity); // 0x43c7d0 (wordharmony), 0x477e20 (pizzafrenzy)
	float getOpacity();                     // 0x43c800 (wordharmony), 0x477e50 (pizzafrenzy)
	void setColor(float theRed, float theGreen, float theBlue);  // 0x43c830 (wordharmony), 0x477ec0 (pizzafrenzy)
	const Color& getColorOffset();                   // 0x43c890 (wordharmony)
	void addColorOffset(float theRed, float theGreen, float theBlue);  // 0x43c8c0 (wordharmony)
	void setFont(Font* theFont);                     // 0x43c980 (wordharmony), 0x477fd0 (pizzafrenzy)
	void setHAlign(int theAlign);           // 0x43c9e0 (wordharmony), 0x4780f0 (pizzafrenzy)
	void setVAlign(int theAlign);           // 0x43ca30 (wordharmony), 0x478140 (pizzafrenzy)
	void setSnap(int theSnap);              // 0x43ca80 (wordharmony)
	int getHAlign();
	int getVAlign();
	int getSnap();                          // 0x43cad0 (wordharmony)

	// The signal build's state (Pizza Frenzy's draw context, vtable 0x50ab0c (pizzafrenzy)
	// and base 0x502b70 (pizzafrenzy))
	// 0x477ef0 (pizzafrenzy): the colour with an alpha (the three-value form, 0x477ec0 (pizzafrenzy), leaves the alpha
	// as it was); of the draws only the fill reads the alpha
	void setColor(float theRed, float theGreen, float theBlue, float theAlpha)
	{
		setColor(theRed, theGreen, theBlue);
		state().mColorAlpha = theAlpha;
	}
	const Color& getColor() { return state().mColor; }                        // 0x477f20 (pizzafrenzy)
	void setColorMode(int theMode);                                           // 0x477f50 (pizzafrenzy)
	int getColorMode() { return state().mColorMode; }                         // 0x477fa0 (pizzafrenzy)
	void setSmoothType(int theSmoothType);                                    // 0x478190 (pizzafrenzy)
	int getSmoothType() { return state().mSmoothType; }                       // 0x4781e0 (pizzafrenzy)
	void setBltFx(BltFx* theBltFx);                                           // 0x478060 (pizzafrenzy)
	BltFx* getBltFx() { return state().mBltFx; }                              // 0x4780c0 (pizzafrenzy)

protected:
	std::deque<RenderState> mStates;
};

} // namespace engine

#endif
