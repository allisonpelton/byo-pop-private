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
	int			mX; // the spark's x across the beam, from 0 to the beam's width less one cel of the particle picture
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
	int			mHue; // +0x10 the hue of the text's colour, 7 more each update (mod 255) while the text fades
	int			mLife;          // +0x14 updates left (counts down once the text fades)
	int			mAlphaDelay; // +0x18 counts down before mAlpha does (constructed 0, so the alpha falls from the first update of a text that waits for ever)
	bool		mDone;          // +0x1c
	bool		mNoFade;        // +0x1d
	std::string	mText;          // +0x20
	Font*		mFont;          // +0x3c
	float		mOriginX; // +0x40 the x the text was given (-1 for the plain rising text, which Update tests with mSpeed and mTargetY)
	float		mTargetY;       // +0x44 the height it moves to (-1: it just floats up)
	int			mHold;          // +0x48 updates it waits there (500000000: for ever)
	int			mHoldStart;     // +0x4c the update the wait began
	int			mAlpha; // +0x50 the alpha of the text, 255 down by 2 an update once mAlphaDelay has run out
	float		mSpeed;         // +0x54

	FloatText(const std::string& theText, Font* theFont, float theX, float theY, bool theFlag);  // 0x4107e0
	FloatText(const std::string& theText, Font* theFont, float theX, float theTargetY, int theHold, float theSpeed);  // 0x4108e0
	void		Update();       // 0x408890
	void		Draw(Graphics* g);      // 0x4089a0
};

// A hint box (class of 0x68 bytes, constructor 0x456660, vtable 0x565440): lines of text in a panel that slides in
// and out of view
class HintBox
{
public:
	int			mPhaseTimer; // +0x08 counts the updates of the phase while mTimerRunning: the wait before it slides in, then the time it stays in view
	int			mX;             // +0x0c
	int			mY;             // +0x10
	int			mWidth;         // +0x14 (0x43c700)
	int			mHeight;        // +0x18 (0x436490)
	bool		mTimerRunning; // +0x1c the phase timer runs
	bool		mInView; // +0x1d the box is on the screen now
	bool		mHidden; // +0x1e set by SetShown(false), cleared by SetShown(true); Draw skips a hidden box (constructed set)
	bool		mDrawAllowed; // +0x1f cleared by SetShown(false), set by SetShown(true); Draw needs it (constructed set)
	bool		mSlidIn; // +0x20 the box has come in and is counting mHideTime
	bool		mShown; // +0x21 the box was asked to show itself (SetShown)
	Font*		mFont;          // +0x24
	std::vector<std::string>	mLines;     // +0x28
	Color		mFillColor; // +0x38 the panel's colour
	Color		mTextColor; // +0x48 the text's colour
	int			mShowTime;      // +0x58 updates the box stays out of view before it slides in
	int			mHideTime;      // +0x5c updates it stays in view
	int			mCellColumn; // +0x60 the column of the field cell the box is about (set by the pointer's cell hint; HideCellHint compares it)
	int			mCellRow; // +0x64 the row of the field cell the box is about

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

// The wipe that takes a picture off the screen when a game ends (class of 0x28 bytes, constructor 0x426230; the Board
// keeps one at +0x310, over the picture it captured into the image at +0x2f4). It makes the picture transparent bit by
// bit, one of four ways: 0 pixels in a random order, 1 a ragged edge sweeping from the right to the left, 2 the same
// from the left to the right, 3 rows from the top down by two and back up by two.
class ScreenWipe
{
public:
	struct Point
	{
		int			mX;
		int			mY;
	};

	Image*				mImage;         // +0x00
	int					mKind;          // +0x04
	int					mCounter;       // +0x08 updates so far
	int					mPos;           // +0x0c kinds 1 and 2: the column the edge is at
	int					mRow;           // +0x10 kind 3: the row
	bool				mDone;          // +0x14
	bool				mOwnsImage;     // +0x15 the picture is a copy
	bool				mGoingDown;     // +0x16 kind 3 (constructed true)
	std::vector<Point>	mPoints;        // +0x18 kind 0: the pixels still to clear

	ScreenWipe();                                           // 0x426230
	~ScreenWipe();
	void		SetImage(Image* theImage, bool theCopy);    // 0x40b280
	void		SetKind(int theKind);                       // 0x426250
	bool		Check(int theSteps);                        // 0x4264a0: runs theSteps steps; true when the wipe is over
	void		Draw(Graphics* g, int theX, int theY);      // 0x40b5e0

private:
	bool		StepRandom(int theSteps);                   // 0x426370
	bool		StepToLeft(int theSteps);                   // 0x40b340
	bool		StepToRight(int theSteps);                  // 0x40b430
	bool		StepRows(int theSteps);                     // 0x40b540
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
	int			mActive; // +0x08 the picture is showing; 0 once it has faded out or shrunk to nothing
	int			mStartDelay; // +0x0c updates left before the picture's size starts to move (constructed 30)
	int			mBigHoldTime; // +0x10 updates left of the stretch to the big size 1.75 (constructed 100); the picture shrinks away after it
	int			mSmallHoldTime; // +0x14 updates left of a first stretch to the size 1.2; constructed 0 and never raised, so Update never takes that branch
	int			mAlphaRiseTime; // +0x18 updates the alpha still rises (the y it was given plus 190); once at 0 the alpha falls
	float		mSpringRamp; // +0x1c grows from 0 to 1 by 0.04 an update; makes the spring of mStretch stiffer and its damping stronger
	float		mStretch; // +0x20 the spring position, 1 down to 0: the picture is wide and short while it is large (DrawAt)
	float		mStretchSpeed; // +0x24 the speed of mStretch
	float		mScale; // +0x28 the picture's size, 1 at the start, aimed at 1.75 and then at 0
	float		mScaleSpeed; // +0x2c the speed of mScale (constructed 0.025)
	float		mAlpha; // +0x30 the picture's alpha, 0 to 1
	float		mBaseScale; // +0x34 the constant factor of the picture's size (0.73)
	Image*		mImage;         // +0x38

	LevelUpText(Image* theImage, int theY);     // 0x40b600
	void		Update();                                // 0x40b6a0
	void		Draw(Graphics* g);                       // 0x40dab0 (at the middle of the screen, 0x182, 0xf0)
	void		DrawAt(Graphics* g, int theX, int theY); // 0x40c8a0
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
