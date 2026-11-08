#ifndef __ASTROPOP_SMALLOBJECTS_H__
#define __ASTROPOP_SMALLOBJECTS_H__

// The small objects the Board keeps lists of: the demo's steps, the blinking pictures, the texts that float up the
// screen, the hint boxes and the check of a level's goal.

#include "Color.h"
#include "FrameAnim.h"
#include <string>
#include <vector>

namespace Sexy
{

class Font;
class Graphics;
class Image;

// A step of the demo that the title plays (8 bytes each, the vector at +0x2d0): action 0 waits mArg updates, 1 and 2 click
// (right and left), 3 .. 12 move the pointer to one of ten columns
struct DemoStep
{
	int		mAction;
	int		mArg;
};

// One of the 40 blinking pictures of the backdrop (0x48 bytes, the array at +0xd8): the counter runs down to -20 and
// restarts at 400, the animation steps along
struct BlinkRec
{
	int			m00;
	int			mCounter;       // +0x04
	FrameAnim	mAnim;          // +0x08
};

// A text that floats up the screen (a class of at least 0x58 bytes, Update 0x408890): it moves to a height, waits
// there, moves on and ends when it is above the screen or its life has run out
class FloatText
{
public:
	float		mX;             // +0x00
	float		mY;             // +0x04
	int			mAge;           // +0x08 updates so far
	int			mWidth;         // +0x0c the text's width
	int			m10;            // +0x10 colour phase, +7 each update (mod 255)
	int			mLife;          // +0x14 updates left (counts down once the text fades)
	int			m18;            // +0x18 counts down before field 0x50 does
	bool		mDone;          // +0x1c
	bool		mNoFade;        // +0x1d
	std::string	mText;          // +0x20
	Font*		mFont;          // +0x3c
	float		m40;            // +0x40
	float		mTargetY;       // +0x44 the height it moves to (-1: it just floats up)
	int			mHold;          // +0x48 updates it waits there (500000000: for ever)
	int			mHoldStart;     // +0x4c the update the wait began
	int			m50;            // +0x50
	float		mSpeed;         // +0x54

	FloatText(const std::string& theText, Font* theFont, float theX, float theY, bool theFlag);  // 0x4107e0
	FloatText(const std::string& theText, Font* theFont, float theX, float theTargetY, int theHold, float theSpeed);  // 0x4108e0
	void		Update();       // 0x408890
};

// A hint box (class of 0x68 bytes, constructor 0x456660, vtable 0x565440): lines of text in a panel that slides in
// and out of view
class HintBox
{
public:
	int			m04;            // +0x04 updates so far
	int			m08;            // +0x08 timer of the phase
	int			mX;             // +0x0c
	int			mY;             // +0x10
	int			mWidth;         // +0x14 (0x43c700)
	int			mHeight;        // +0x18 (0x436490)
	bool		m1c;            // +0x1c the timer runs
	bool		m1d;            // +0x1d
	bool		m1e;            // +0x1e
	bool		m1f;            // +0x1f
	bool		m20;            // +0x20
	bool		m21;            // +0x21 shown
	Font*		mFont;          // +0x24
	std::vector<std::string>	mLines;     // +0x28
	Color		m38;            // +0x38 the panel's colour
	Color		m48;            // +0x48 the text's colour
	int			mShowTime;      // +0x58 updates the box stays out of view before it slides in
	int			mHideTime;      // +0x5c updates it stays in view
	int			m60;            // +0x60
	int			m64;            // +0x64

	HintBox();                              // 0x456660
	virtual ~HintBox() {}                   // 0x456750
	virtual void	Update();               // 0x435a40
	virtual void	Draw(Graphics* g);      // 0x43c5d0
	void			SetShown(bool theShow); // 0x4359f0
	void			SetFont(Font* theFont); // 0x43d990
	int				ComputeWidth();         // 0x43c700
	int				ComputeHeight();        // 0x436490
	void			AddLine(const std::string& theLine);    // 0x460a70
};

// The check of a level's goal (the Board keeps one at +0x310): the kind at +4 picks which of four checks runs. Each
// update counts (+8).
class GoalChecker
{
public:
	int			mKind;          // +0x04
	int			mCounter;       // +0x08

	bool		Check(int theValue);                // 0x4264a0
};

// A text that the Board draws at a place with a font (0x28 bytes, the vector at +0x270; drawn by Draw)
struct DrawnText
{
	int			mX;             // +0x00
	int			mY;             // +0x04
	std::string	mText;          // +0x08
	Font*		mFont;          // +0x24
};

// The picture that announces a level (class of 0x3c bytes, constructor 0x40b600, destructor 0x40c870, no other virtual)
class LevelUpText
{
public:
	virtual ~LevelUpText() {}                   // 0x40c870
	bool		m04;            // +0x04
	int			m08;            // +0x08
	int			m0c;            // +0x0c
	int			m10;            // +0x10
	int			m14;            // +0x14
	int			m18;            // +0x18
	float		m1c;            // +0x1c
	float		m20;            // +0x20
	float		m24;            // +0x24
	float		m28;            // +0x28
	float		m2c;            // +0x2c
	float		m30;            // +0x30
	float		m34;            // +0x34
	Image*		mImage;         // +0x38

	LevelUpText(Image* theImage, int theY);     // 0x40b600
	void		Update();                                // 0x40b6a0
};

// A spark of the sun that drifts over the end of a game (0x50 bytes, the vector at +0xe8): it moves by its speed each
// update while its animation runs (the animation's stopped flag at +0x38 is what the Board's update tests)
struct SunParticle
{
	float		mX;             // +0x00
	float		mY;             // +0x04
	float		mVX;            // +0x08
	float		mVY;            // +0x0c
	FrameAnim	mAnim;          // +0x10
};

// A fading panel of the end of a game (12 bytes, the vector at +0x1e0): the alpha value runs up by mStep to 255 and then
// down by 4 to nothing, one panel after the other
struct FadeRec
{
	int			mValue;         // +0x00 alpha
	int			mStep;          // +0x04
	bool		mRising;        // +0x08
};

}

#endif
