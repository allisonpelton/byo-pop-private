#ifndef __SPROUT2_DRAWCONTEXT_H__
#define __SPROUT2_DRAWCONTEXT_H__

// The draw context: what a frame's drawing goes through (engine::DrawContext, vtable 0x55ff3c (feedingfrenzy2); its
// Windows implementation dxdriver::DXDrawContext draws into a surface with the engine's own blitters). It keeps a stack
// of states, the transform and the look that a component passes on to its children (Component::applyTransform), and
// a stack of clip rectangles.

#include "Font.h"
#include "Geometry.h"
#include "Object.h"
#include "PixelFx.h"

#include <cstdint>
#include <vector>

namespace engine
{

class Surface;

// How a state's colour applies to what is drawn (the colour mode bits of the state's flags)
enum ColorMode
{
	COLOR_MODE_NONE = 0,       // the image as it is (a component's 0 leaves its parent's mode)
	COLOR_MODE_ADD = 1,        // the state's colour added to the image's (a negative one darkens it)
	COLOR_MODE_MULTIPLY = 2,   // the image's colours multiplied by the state's colour
	COLOR_MODE_PIXEL_FX = 3    // the image's colours through the state's pixel effect
};

// How an image is placed (the filter mode bits of the state's flags)
enum FilterMode
{
	FILTER_MODE_SNAP = 0,     // at whole pixels, the position rounded
	FILTER_MODE_SMOOTH = 1    // at the exact position, filtered
};

// Where text is placed across its position (the horizontal alignment bits of the state's flags)
enum TextAlignH
{
	TEXT_ALIGN_LEFT = 0,
	TEXT_ALIGN_CENTER = 1,
	TEXT_ALIGN_RIGHT = 2
};

// And down from it (the vertical alignment bits)
enum TextAlignV
{
	TEXT_ALIGN_TOP = 0,      // the glyphs' tops, the font's top overlap above the position
	TEXT_ALIGN_MIDDLE = 1,
	TEXT_ALIGN_BOTTOM = 2
};

// A state of the stack (0x38 bytes in the original)
struct DrawState
{
	Vector2f mPosition;         // +0x00
	Vector2f mScale;            // +0x08
	float mRotation;            // +0x10
	float mOpacity;             // +0x14
	Color mColor;               // +0x18
	Ref<Font> mFont;            // +0x28
	Ref<PixelFx> mPixelFx;      // +0x2c
	int mClipCount;             // +0x30: the clip stack's size when the state was pushed
	ColorMode mColorMode;       // +0x34: the flags' colour mode bits
	FilterMode mFilterMode;     // +0x34: the flags' filter mode bits
	TextAlignH mTextAlignH;     // +0x34: the flags' horizontal alignment bits
	TextAlignV mTextAlignV;     // +0x34: the flags' vertical alignment bits

	// The first state (the frame's, 0x438700 (feedingfrenzy2)): at the origin, unscaled, opaque, black, no font
	DrawState()
		: mPosition(0, 0), mScale(1, 1), mRotation(0), mOpacity(1), mColor(0, 0, 0, 1), mClipCount(0),
		  mColorMode(COLOR_MODE_NONE), mFilterMode(FILTER_MODE_SNAP), mTextAlignH(TEXT_ALIGN_LEFT),
		  mTextAlignV(TEXT_ALIGN_TOP)
	{
	}
};

class DrawContext : public Object
{
public:
	DrawContext();
	~DrawContext() override;

	// 0x00: a part of an image (theSource, in the image's pixels) drawn with its anchor at (x, y) in the state's space
	virtual void drawImage(Surface* theImage, const RectI& theSource, float x, float y) = 0;
	// 0x04 (DXDrawContext's 0x434870 (feedingfrenzy2)): the whole image
	virtual void drawImage(Surface* theImage, float x, float y);
	// 0x08 (DXDrawContext's 0x443770 (feedingfrenzy2)): the whole image at the state's origin
	virtual void drawImage(Surface* theImage);
	// 0x0c (0x4441f0 (feedingfrenzy2)): a line of text in the state's font, placed at (x, y) by the state's alignment
	virtual void drawText(const char* theText, float x, float y);
	// 0x10 (0x44e560 (feedingfrenzy2)): text at the state's origin
	virtual void drawText(const char* theText);
	// 0x14 (0x4ff716 (feedingfrenzy2)): one character at the state's origin
	virtual void drawChar(char theChar);
	// 0x18 (0x513cd5 (feedingfrenzy2)): a rectangle filled with the state's colour
	virtual void fillRect(const RectI& theRect) = 0;
	// 0x1c (DXDrawContext's 0x507b7d (feedingfrenzy2)): a rectangle of the size, truncated, at the state's origin
	virtual void fillRect(float theWidth, float theHeight);
	// 0x20 (0x44ef30 (feedingfrenzy2)): a rectangle's outline in the state's colour
	virtual void drawRect(const RectI& theRect) = 0;
	// 0x24 (DXDrawContext's 0x45fbd0 (feedingfrenzy2)): an outline of the size, truncated, at the state's origin
	virtual void drawRect(float theWidth, float theHeight);
	// 0x28 (0x46c520 (feedingfrenzy2)): a line in the state's colour
	virtual void drawLine(const Vector2f& theFrom, const Vector2f& theTo) = 0;
	// 0x2c (0x51b5c8 (feedingfrenzy2)): a circle's outline in the state's colour
	virtual void drawCircle(const Vector2f& theCenter, float theRadius) = 0;
	// 0x34 (DXDrawContext's 0x513cb9 (feedingfrenzy2)): the target filled with the state's colour
	virtual void clear() = 0;

	// The state stack (a std::deque of states in the original)
	DrawState& back() { return mStates.back(); }
	// 0x437b40 (feedingfrenzy2)
	void pushState(const DrawState& theState) { mStates.push_back(theState); }
	// Container::paint (0x443850 (feedingfrenzy2)): a copy of the state on top
	void pushState() { mStates.push_back(mStates.back()); }
	void popState() { mStates.pop_back(); }

	// The clip stack (0x14-byte entries in the original, a rectangle and a field not yet seen read)
	int getClipCount() const { return (int)mClips.size(); }
	// 0x46ccd0 (feedingfrenzy2): the clip stack cut back to the count a state had
	void restoreClips(int theCount);

protected:
	std::vector<RectI> mClips;        // +0x0c
	std::vector<DrawState> mStates;   // +0x38
};

} // namespace engine

#endif
