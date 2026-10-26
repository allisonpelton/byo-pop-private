#ifndef __ALCHEMY_EFFECTS_H__
#define __ALCHEMY_EFFECTS_H__

// The effects overlay (class 0x4e3ab0, ctor 0x40858e, size 0x78): a transparent widget over the whole screen that
// draws the score numbers that float up from a cleared piece and the sparkles. The board adds to it (0x408938 and
// 0x408970); it also shows the help arrow while the help text is up.

#include "Widget.h"
#include <list>

namespace Sexy
{

class AlchemyApp;

struct ScorePopup
{
	int			mX;         // +8
	int			mY;         // +0xc rises until it is near the top
	int			mLife;      // +0x10 updates left (starts 110); fades over the last 18
	int			mValue;     // +0x14 the number shown
	double		mAlpha;     // +0x18

	ScorePopup() : mX(0), mY(0), mLife(0), mValue(0), mAlpha(1.0) {}
};

struct Sparkle
{
	int			mX;         // +8
	int			mY;         // +0xc
	int			mFrame;     // +0x10 0..13, one per 3 updates
	bool		mFalls;     // +0x14 sinks one pixel every 4 updates
	bool		mBig;       // +0x15 the 20-pixel picture (smsparkle) instead of the 10-pixel one (tinysparkle)

	Sparkle() : mX(0), mY(0), mFrame(0), mFalls(false), mBig(false) {}
};

class EffectsWidget : public Widget
{
public:
	std::list<ScorePopup>	mPopups;        // +0x58
	std::list<Sparkle>		mSparkles;      // +0x64
	AlchemyApp*				mApp;           // +0x70
	bool					mShowHelpArrow; // +0x74

	EffectsWidget(AlchemyApp* theApp);      // 0x40858e

	void					AddScorePopup(int theX, int theY, int theValue);        // 0x408938
	void					AddSparkle(int theX, int theY, bool isFalling, bool isBig); // 0x408970
	void					Clear();

	virtual void			Draw(Graphics* g) override;     // 0x4086e6
	virtual void			Update() override;              // 0x40844f
};

}

#endif
