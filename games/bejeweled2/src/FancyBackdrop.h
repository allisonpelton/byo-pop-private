#ifndef __FANCYBACKDROP_H__
#define __FANCYBACKDROP_H__

// FancyBackdrop.h: the space picture behind the title screen and the main menu (Sexy::FancyBackdrop, 0x5b95ff-0x5ba3cb;
// the object is 0xd0 bytes): the background, the scrolling star field and planet, the sparkles, the comets and the fog.
// Its updates draw random numbers (the sparkles and the comets), so the stream of the game's RNG depends on it.

#include "Widget.h"
#include <vector>

namespace Sexy
{

class WinDMApp;

struct Comet
{
	float			mX;             // +0x00
	float			mY;             // +0x04
	float			mVX;            // +0x08
	float			mVY;            // +0x0c
	double			mLife;          // +0x10 0 to 1
};

class FancyBackdrop : public Widget
{
public:
	WinDMApp*		mApp;               // +0x88
	bool			mShowing;           // +0x8c the comets fly (the main menu is up)
	float			mCometX;            // +0x90 where the comets start
	float			mCometY;            // +0x94
	float			mCometVX;           // +0x98
	float			mCometVY;           // +0x9c
	std::vector<Comet*> mComets;        // +0xa0
	bool			mSparkle;           // +0xb0 a star sparkle shows
	int				mSparkleX;          // +0xb4
	int				mSparkleY;          // +0xb8
	int				mSparkleFrame;      // +0xbc
	float			mStarScroll;        // +0xc0 the star field's scroll
	float			mPlanetScroll;      // +0xc4
	float			mFogX;              // +0xc8 the fog bank behind the foreground
	float			mFogFgX;            // +0xcc the fog bank in front

public:
	FancyBackdrop(WinDMApp* theApp);                                // 0x5ba2d7
	virtual ~FancyBackdrop();                                       // 0x5ba030, 0x5b9fe5 (the destructor proper)

	virtual void	Update() override;                              // 0x5ba3cb
	virtual void	Draw(Graphics* g) override;                     // 0x5b9d60
	void			DrawTop(Graphics* g);                           // 0x5b95ff the foreground and the fog in front
	void			DrawBottom(Graphics* g);                        // 0x5b9983 the background, stars, comets and planet
};

}

#endif
