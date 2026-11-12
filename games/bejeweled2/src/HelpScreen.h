#ifndef __HELPSCREEN_H__
#define __HELPSCREEN_H__

// HelpScreen.h: the "How To Play" pages (Sexy::HelpScreen, 0x5c1c7a-0x5c267d; 0x278 bytes). A PodDialog that slides in
// over the game and shows four pages of gem demonstrations one after the other (swap, multiple sets, power gem, hyper
// cube). Each page fades in, plays its swaps, fades out to 0.5 brightness and the next page begins; the pages repeat
// until the player closes the screen. The offsets in the comments are the original's, from the start of the object.

#include "PodDialog.h"
#include "CurvedVal.h"

namespace Sexy
{

class Graphics;
class WinDMApp;

class HelpScreen : public PodDialog
{
public:
	enum { STATE_SLIDING_IN = 0, STATE_SHOWN = 1, STATE_SLIDING_OUT = 2, STATE_HIDDEN = 3 };

	int				mPage;              // +0x184 the page that plays now, 0 to 3 (-1 until InitialReset)
	int				mDrawPage;          // +0x188 the page Draw is drawing (DrawGem, DrawArrow and DrawSwapGem read the fades of it)
	float			mArrowAlpha[4];     // +0x18c the alpha of the arrow of each page (it fades out after a delay)
	float			mPageAlpha[4];      // +0x19c the alpha of the gems of each page
	float			mPageBright[4];     // +0x1ac the brightness (colorize) of each page: 1.0 for the page that plays, 0.5 for the rest
	int				mState;             // +0x1bc STATE_SLIDING_IN ... STATE_HIDDEN
	int				mDelay;             // +0x1c0 updates to wait before the slide starts
	CurvedVal		mSlide;             // +0x1c8 the slide in and out, in pixels (at 1024x768)
	int				mSwapDelay1;        // +0x1f8 updates before mSwapAlpha1 starts to rise
	float			mSwapAlpha1;        // +0x1fc the alpha of the selection box on the swap of the second row
	int				mSwapDelay2;        // +0x200 updates before mSwapAlpha2 starts to rise
	float			mSwapAlpha2;        // +0x204 the alpha of the selection box on the swap of the first row
	int				mSwapTime;          // +0x208 updates that the selection boxes rise (when it has run out they fade)
	int				mFadeInTime;        // +0x20c updates that the page fades in (then it fades out)
	int				mPageTime;          // +0x210 updates until the next page (Reset)
	int				mAngleDelay;        // +0x214 updates before the swap angles start to turn
	float			mSwapAngle[4];      // +0x218 how far each page's swap has gone, 0 to a little over pi/2
	double			mPulse;             // +0x228 0 to 1, the star's flicker
	double			mStarAngle;         // +0x230 the turn of the first star, 0 to 2 pi
	double			mStarAngle2;        // +0x238 the turn of the second star (counts down without limit)
	int				mEffectDelay;       // +0x240 updates before mEffectFade starts to rise
	float			mEffectFade[4];     // +0x244 the fade of the star or the wild card of each page
	int				mGemFadeDelay;      // +0x254 updates before mGemFade starts to fall
	float			mGemFade[4];        // +0x258 the shrinking of the gems that are cleared (1.0 whole, 0 gone)
	int				mArrowTimer;        // +0x268 (set to 0 on page 0, only counted down)
	int				mArrowDelay;        // +0x26c updates before mArrowAlpha starts to fall
	bool			mNeverShowAgain;    // +0x270 port only (the original asks mNoButton != 0): the screen has the "Don't Show Again" button

public:
	HelpScreen(WinDMApp* theApp, bool theBackOnly);         // 0x5c252e
	virtual ~HelpScreen() {}                                // 0x5c2349, 0x5c1c65 (the destructor proper: both vtables, then PodDialog's)

	void			Reset();                                // 0x5c1c95
	void			InitialReset();                         // 0x5c1daa
	void			Show();                                 // 0x5c1dfc
	void			Hide();                                 // 0x5c1e50
	void			DrawArrow(Graphics* g, int theX, int theY);     // 0x5c1e97
	void			DrawGem(Graphics* g, int theX, int theY, int theColor, float theFrac);      // 0x5c2368
	void			DrawSwapGem(Graphics* g, int theX, int theY, int theDX, int theDY, int theColor, float theFrac, float theSelectAlpha);   // 0x5c245d

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5c1c7a
	virtual void	AddedToManager(WidgetManager* theManager) override;                 // 0x5c1c83
	virtual void	RemovedFromManager(WidgetManager* theManager) override;             // 0x5c1c8c
	virtual void	Update() override;                      // 0x5c1f1d
	virtual void	Draw(Graphics* g) override;             // 0x5c267d
};

}

#endif
