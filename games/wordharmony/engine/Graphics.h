#ifndef __WORDHARMONY_ENGINE_GRAPHICS_H__
#define __WORDHARMONY_ENGINE_GRAPHICS_H__

// The engine's render context (ENGINE.md, "The render context"): a stack of render states and the drawing calls that
// use the top one. The base class (vtable 0x484cf8) keeps the states; the Windows layer's subclass (0x45dcb0, vtable
// 0x48ac14) drew over DirectDraw with Sprout's blitters. The port's subclass draws with the framework (Display.cpp).

#include "Geometry.h"
#include "Object.h"
#include <cstdint>
#include <deque>

namespace engine
{

class Font;

// The alpha modes of an image (its slot 0x18)
enum
{
	ALPHA_NONE = 0,   // drawn opaque (JPEGs, PNGs without alpha)
	ALPHA_TEST = 1,   // PNGs with alpha (threshold 0), or alphaType="alphaTest" in a screen
	ALPHA_BLEND = 2   // images with a mask, or alphaType="alphaBlend"
};

// An image: the interface the Windows layer's image (0x457410, 0x48 bytes) keeps at +0x0c (vtable 0x48aba8; its base
// 0x45e670 holds the pivot and the alpha mode). The host's image implements the size and the pixels.
class Image : public Object
{
public:
	// 0x45e670
	Image() : mAlphaMode(ALPHA_NONE), mAlphaThreshold(0) {}
	virtual int getWidth() = 0;   // slot 0x00: 0x4572a0
	virtual int getHeight() = 0;  // slot 0x04: 0x4572b0
	// slot 0x0c: 0x45e5d0, 1 puts the pivot at the centre (in whole pixels); other values leave it
	void setPivot(int theMode)
	{
		if (theMode == 1)
			setPivot(getWidth() / 2, getHeight() / 2);
	}
	void setPivot(int x, int y) { mPivotX = x; mPivotY = y; }  // slot 0x10: 0x45e5c0
	int getPivotX() const { return mPivotX; }                  // slot 0x14: 0x45e600
	int getPivotY() const { return mPivotY; }
	// slot 0x18: 0x45e610
	void setAlphaMode(int theMode, uint8_t theThreshold)
	{
		mAlphaMode = theMode;
		mAlphaThreshold = theThreshold;
	}
	int getAlphaMode() const { return mAlphaMode; }            // slot 0x1c: 0x45e630
	uint8_t getAlphaThreshold() const { return mAlphaThreshold; }

private:
	int mAlphaMode;            // +0x08
	uint8_t mAlphaThreshold;   // +0x0c
	int mPivotX = 0;           // +0x10
	int mPivotY = 0;
};

// A render state (0x40 bytes; 0x43c470 makes the first)
struct RenderState
{
	Point mTranslation;      // +0x00, added to
	Point mScale;            // +0x08, multiplied (1, 1)
	float mRotation;         // +0x10, added to
	float mOpacity;          // +0x14, multiplied (1)
	Color mColor;            // +0x18, set
	Color mColorOffset;      // +0x24, added to
	Ref<Font> mFont;         // +0x30
	unsigned int mFlags;     // +0x34: the alignment bits below
	int mUnused38;
	int mUnused3c;

	RenderState() : mScale(1, 1), mRotation(0), mOpacity(1), mFlags(0), mUnused38(0), mUnused3c(0) {}
};

// The bit fields of RenderState::mFlags (masks and shifts at 0x497200..0x49721c)
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
	Graphics();          // 0x43d0a0: one state, the defaults
	virtual ~Graphics(); // 0x43cb10

	// Slot 0: the image's theSrc part at (x, y), with the state's transform, opacity and colours
	virtual void drawImage(Image* theImage, const IntRect& theSrc, float x, float y) = 0;
	// Slot 1 (0x45ddf0): the whole image at (x, y)
	virtual void drawImage(Image* theImage, float x, float y);
	// Slot 2 (0x45dd70): the whole image at (0, 0)
	virtual void drawImage(Image* theImage);
	// Slot 3 (0x43cba0): a string with the state's font, glyph by glyph
	virtual void drawString(const char* theString);
	// Slot 4 (0x43c310): one character
	virtual void drawChar(char theChar);
	// Slots 5 and 6: a rectangle filled with the state's colour and opacity
	virtual void fillRect(const Rect& theRect) = 0;
	virtual void fillRectAdditive(const Rect& theRect) = 0;
	// Slot 7: a line in the state's colour
	virtual void drawLine(const Point& theFrom, const Point& theTo) = 0;

	// 0x43d120: pushes a copy of the top state
	void pushState();
	// 0x43ce10
	void popState();
	RenderState& state() { return mStates.back(); }  // 0x43c660

	// 0x43c5c0
	void setTranslation(float x, float y)
	{
		state().mTranslation.x = x;
		state().mTranslation.y = y;
	}
	void translate(float x, float y);       // 0x43c610
	void scale(float sx, float sy);         // 0x43c690
	float getScaleX();                      // 0x43c6e0
	float getScaleY();                      // 0x43c710
	void rotate(float theAngle);            // 0x43c740
	float getRotation();                    // 0x43c770
	void setOpacity(float theOpacity);      // 0x43c7a0
	void multiplyOpacity(float theOpacity); // 0x43c7d0
	float getOpacity();                     // 0x43c800
	void setColor(float r, float g, float b);        // 0x43c830
	const Color& getColor();                         // 0x43c860
	const Color& getColorOffset();                   // 0x43c890
	void addColorOffset(float r, float g, float b);  // 0x43c8c0
	void setFont(Font* theFont);                     // 0x43c980
	void setHAlign(int theAlign);           // 0x43c9e0
	void setVAlign(int theAlign);           // 0x43ca30
	void setSnap(int theSnap);              // 0x43ca80
	int getHAlign();
	int getVAlign();
	int getSnap();                          // 0x43cad0

protected:
	std::deque<RenderState> mStates;
};

} // namespace engine

#endif
