#ifndef __WORDHARMONY_ENGINE_GRAPHICS_H__
#define __WORDHARMONY_ENGINE_GRAPHICS_H__

// The engine's render context (ENGINE.md, "The render context"): a stack of render states and the drawing calls that
// use the top one. The base class (vtable 0x484cf8) keeps the states; the Windows layer's subclass (0x45dcb0, vtable
// 0x48ac14) drew over DirectDraw with Sprout's blitters. The port's subclass draws with the framework (Display.cpp).

#include "Geometry.h"
#include "Object.h"
#include <deque>

namespace engine
{

class Font;

// An image (the Windows layer's image keeps this interface at +0x0c; its slots 0 and 1 give the size, slot 7 the
// kind: 1 colour-keyed, 2 without alpha)
class Image : public virtual Interface
{
public:
	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
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
