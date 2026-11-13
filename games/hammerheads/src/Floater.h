#ifndef __HAMMERHEADS_FLOATER_H__
#define __HAMMERHEADS_FLOATER_H__

// Everything that floats over a level for a moment and then goes: score numbers, the hammer's "strike" and "miss" signs,
// the hints, the "Game Over" text, the wave result and level summary banners (0x444500-0x448ff0). One class, Floater, does
// twenty kinds of them by its type; the coin, heart, photo, rotating and shrinking floaters are classes of their own.
// A floater has a time left that runs down in hundredths of the time passed to Update (times its speed), and is gone at
// zero.

#include <string>

#include "Object.h"

namespace Sexy
{

class Image;
class MemoryImage;

enum FloaterType
{
	FLOATER_RISING_TEXT = 0,		// small text that drifts up and fades (the pointer's hint at a cubby)
	FLOATER_SCORE_FLY = 1,			// score text that grows, then flies to the score display
	FLOATER_SCORE_POP = 2,			// score text that pops up and fades where a gnome was hit
	FLOATER_SCORE_BIG = 3,			// larger score text that flies to the score display
	FLOATER_GAME_OVER = 4,			// "Game Over" drops in, then falls out; ending it ends the game
	FLOATER_SHATTER = 5,			// a picture that shatters into 25 tiles and flies apart; awards its points as it goes
	FLOATER_SHATTER_SIDE = 6,		// the same, the tiles also sliding sideways
	FLOATER_STRIKE = 7,				// the sign for a hammer hit
	FLOATER_STRIKE_BIG = 8,			// the same, 2.5 times as large
	FLOATER_STRIKE_WEAK = 9,		// the sign for a hit that did nothing
	FLOATER_MISS = 10,				// the sign for a miss
	FLOATER_HINT_CLICK = 11,		// the first hint: click on the cubbies
	FLOATER_HINT_DONT_MISS = 12,	// the second hint: do not miss
	FLOATER_HINT_PICK = 13,			// the third hint: pick up the coins (and hearts)
	FLOATER_BROKEN_HEART = 14,		// a heart that falls away when health is lost
	FLOATER_CUPID_HEART = 15,		// the hearts of the Cupido gnome
	FLOATER_CRYSTAL_GLOW = 16,
	FLOATER_WAVE_RESULT = 17,		// the banner at the end of a wave with its accuracy bonus
	FLOATER_LEVEL_COMPLETE = 18,	// the level summary
	FLOATER_FLASH = 19				// a flash of light
};

class Floater : public Object
{
public:
	int				mType;				// +0x80
	std::string		mText;				// +0x84
	double			mX;					// +0xa0
	double			mY;					// +0xa8
	double			mScrollDivisor;		// +0xb0: type 0 rises a screen height in this many hundredths of a time unit
	Image*			mImage;				// +0xd8: the picture (types 1-4 the text's own, types 5 and 6 the one that shatters)
	MemoryImage*	mTextImage;			// +0xdc: the picture made out of the text; deleted with the floater
	double			mTimeLeft;			// +0xe0
	double			mVelocity;			// +0xf0: type 14's fall
	int				mStyle;				// +0xf8: which font the text is in; for types 5 and 6 the points to award
	bool			mScored;			// +0xfc
	bool			mSoundDone;			// +0xfd
	double			mSpeed;				// +0x100: multiplies the time that passes
	int				mWaveMisses;		// +0x108
	int				mWaveSwings;		// +0x10c: the swings of the level; 0, nothing is rated
	int				mWaveIndex;			// +0x110: which wave of the level ended (when 0, ending the banner starts the level music)
	int				mWavePercent;		// +0x114: the wave's accuracy
	int				mWaveBonus;			// +0x118: the points of the bonus it earned
	int				mLevelSmashed;		// +0x11c: counts of the level summary
	int				mLevelChain;		// +0x120
	int				mLevelSpeed;		// +0x124
	int				mPendingPoints;		// +0x128: points added to the score when the floater ends

public:
	Floater(int theType, double theX, double theY, const char* theText, int theStyle);	// 0x444db0
	virtual ~Floater();															// 0x444ac0

	OBJECT_TYPE(Floater)

	virtual void	Update(float theFrac) override;								// 0x444500
	virtual void	Draw(Graphics* g) override;									// 0x445800

	void			SetImage(Image* theImage);									// 0x4449c0
	void			SetSpeed(double theSpeed);									// 0x4449d0
	// 0x4449e0: the accuracy is the swings that did not miss, as a percentage of the swings; thePoints is what a bonus of 20
	// percent (98 and up) or 10 percent (95 and up) is taken of (the exe's own order of arguments: misses, swings, points, wave)
	void			SetWaveResult(int theMisses, int theSwings, int thePoints, int theWave);
	void			DrawBar(Graphics* g, int theX, int theY, double thePercent, double theFill);	// 0x444c20
	void			SetLevelCounts(int theSmashed, int theChain, int theSpeed);	// 0x444a80
	void			SetPendingPoints(int thePoints);							// 0x444ab0
};

// 0x4456b0: makes a floater and gives it a name (blank: made up)
Floater* MakeFloater(int theType, double theX, double theY, const char* theText, int theStyle, const std::string& theName);

// A coin that jumps out of a cubby and flies to the coin counter (0x447cf0, made by 0x448030)
class FloaterCoin : public Object
{
public:
	double			mX0;				// +0x80
	double			mY0;				// +0x88
	double			mX1;				// +0x90
	double			mY1;				// +0x98
	double			mTimeLeft;			// +0xa0
	double			mDuration;			// +0xa8
	double			mFrame;				// +0xb0: the cel of the spinning coin, 0.7 a step
	double			mLift;				// +0xb8: how far up it jumps first
	Rect			mClip;				// +0xc0: the first part is clipped to this when it has a width
	bool			mGrow;				// +0xd0: set, the coin grows as it flies instead of shrinking
	bool			mKeepSize;			// +0xd1: set (and not mGrow), it keeps the size it starts with

public:
	FloaterCoin(double theX0, double theY0, double theX1, double theY1, double theFrame, double theLift, const Rect& theClip);
	virtual ~FloaterCoin();

	OBJECT_TYPE(FloaterCoin)

	virtual void	Update(float theFrac) override;								// 0x447c60
	virtual void	Draw(Graphics* g) override;									// 0x447dc0

	void			SetDuration(double theDuration);							// 0x447cc0
	void			SetKeepSize(bool theKeep);									// 0x447ce0
};

// 0x448030
FloaterCoin* MakeFloaterCoin(double theX0, double theY0, double theX1, double theY1, double theFrame, double theLift,
	int theClipX, int theClipY, int theClipWidth, int theClipHeight);

// A heart that flies to the health display (0x448130, made by 0x448380)
class FloaterHeart : public Object
{
public:
	double			mX0;				// +0x80
	double			mY0;				// +0x88
	double			mX1;				// +0x90
	double			mY1;				// +0x98
	double			mTimeLeft;			// +0xa0
	double			mDuration;			// +0xa8
	double			mFrame;				// +0xb0

public:
	FloaterHeart(double theX0, double theY0, double theX1, double theY1, double theFrame);
	virtual ~FloaterHeart();

	OBJECT_TYPE(FloaterHeart)

	virtual void	Update(float theFrac) override;								// 0x447c60
	virtual void	Draw(Graphics* g) override;									// 0x4481d0
};

// 0x448380
FloaterHeart* MakeFloaterHeart(double theX0, double theY0, double theX1, double theY1, double theFrame);

// The picture of a photographed gnome with "Snapshot Saved!" (0x4484e0)
class FloaterPhoto : public Object
{
public:
	double			mX;					// +0x80
	double			mY;					// +0x88
	double			mTimeLeft;			// +0x90
	Image*			mImage;				// +0x98: three frames side by side

public:
	FloaterPhoto(double theX, double theY, Image* theImage);
	virtual ~FloaterPhoto();

	OBJECT_TYPE(FloaterPhoto)

	virtual void	Update(float theFrac) override;								// 0x448470
	virtual void	Draw(Graphics* g) override;									// 0x4485c0
};

// 0x4484e0
FloaterPhoto* MakeFloaterPhoto(double theX, double theY, Image* theImage);

// A cel that spins once on its way from one point to another (0x448a70, made by 0x448c30)
class FloaterRotating : public Object
{
public:
	double			mX0;				// +0x80
	double			mY0;				// +0x88
	double			mX1;				// +0x90
	double			mY1;				// +0x98
	double			mTimeLeft;			// +0xa0
	double			mDuration;			// +0xa8
	Image*			mImage;				// +0xb8
	int				mCelRow;			// +0xbc
	int				mCelCol;			// +0xc0
	double			mStartAngle;		// +0xc8

public:
	FloaterRotating(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow, int theCelCol,
		double theStartAngle);
	virtual ~FloaterRotating();

	OBJECT_TYPE(FloaterRotating)

	virtual void	Update(float theFrac) override;								// 0x447c60 (the shared update)
	virtual void	Draw(Graphics* g) override;									// 0x448b10
};

// 0x448c30
FloaterRotating* MakeFloaterRotating(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow,
	int theCelCol, double theStartAngle);

// A cel that jumps and shrinks on its way to a point (0x448d30, made by 0x449090): the shop item or reward that flies to its
// counter
class FloaterShrinking : public Object
{
public:
	double			mX0;				// +0x80
	double			mY0;				// +0x88
	double			mX1;				// +0x90
	double			mY1;				// +0x98
	double			mTimeLeft;			// +0xa0
	double			mDuration;			// +0xa8
	double			mLift;				// +0xb8
	Rect			mClip;				// +0xc0
	int				mCelRow;			// +0xd0
	int				mCelCol;			// +0xd4
	Image*			mImage;				// +0x38 (a field the Object keeps free for it)

public:
	FloaterShrinking(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow, int theCelCol,
		double theLift, const Rect& theClip);
	virtual ~FloaterShrinking();

	OBJECT_TYPE(FloaterShrinking)

	virtual void	Update(float theFrac) override;								// 0x447c60
	virtual void	Draw(Graphics* g) override;									// 0x448e40
};

// 0x449090
FloaterShrinking* MakeFloaterShrinking(double theX0, double theY0, double theX1, double theY1, Image* theImage, int theCelRow,
	int theCelCol, double theLift, int theClipX, int theClipY, int theClipWidth, int theClipHeight);

// 0x4444b0: how far along a ramp theTime is, 1 at or above theHigh, 0 at or below theLow
double RampBetween(double theTime, double theHigh, double theLow);

}

#endif
