#ifndef __PIXELUS_ISLANDMAP_H__
#define __PIXELUS_ISLANDMAP_H__

// IslandMap.cpp: the map of the island with its ten temples (Pixelus.exe 0x4176a0 constructor, 0x4163e0 Draw,
// 0x4170d0 buttons; vftable 0x50df08, listener 0x50dee8). A temple shows once the player has solved enough
// puzzles; one with puzzles left glows and has an arrow over it, a finished one shows its second picture, and a
// medal when every puzzle was solved the best way. Each temple is a button (ids 2-11), the back button has id 1.
//
// The panel at the bottom left (IslandPanel, 0x414750 Update, 0x414bf0 Draw) fades between the island's own text
// and the page of the temple under the mouse. The sea and the smoke cross-fade, lightning flashes now and then and
// the gulls and surf are heard at random (0x4163e0).
//
// port: the particle bursts of a temple that has just opened and the follower of the pointer are not ported yet.

#include "Widget.h"
#include "ButtonListener.h"
#include "Temples.h"

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class ButtonWidget;
class MemoryImage;

class IslandMap;

// the panel at the bottom left of the island (vftable 0x50dc38)
class IslandPanel : public Widget
{
public:
	PixelusApp*		mApp;
	IslandMap*		mIsland;
	int				mIndex;			// +0x60 the temple shown, -1 none
	int				mIndex2;		// +0x64
	int				mFadeState;		// +0x84 0 = fading the temple page in, 1 = fading it out
	int				mFade;			// +0x88 0-255

public:
	IslandPanel(PixelusApp* theApp, IslandMap* theIsland);
	virtual void		Update() override;			// 0x414750
	virtual void		Draw(Graphics* g) override;	// 0x414bf0
	void				Set(int theIndex, int theIndex2);	// 0x4147e0
	void				Clear();					// 0x414710

private:
	void				DrawBar(Graphics* g, int theY, int theSolved, int theCount, const char* theFormat);	// 0x414910
	void				DrawCentered(Graphics* g, int theFont, const std::string& theText, int theY);
};

class IslandMap : public Widget, public ButtonListener
{
public:
	enum
	{
		ID_BACK = 1,
		SEA_FRAMES = 4,
		SMOKE_FRAMES = 3,
		SEA_TICKS = 100,		// updates per frame of the sea
		APPEAR_TICKS = 110		// 1.1 s: the temples that are open come in after this
	};
	enum { STATE_HIDDEN = 0, STATE_OPEN = 1, STATE_NEW = 2, STATE_DONE = 4, STATE_MAXED = 5 };

	PixelusApp*		mApp;						// +0x5c
	DialogButton*	mBackButton;				// +0x84
	ButtonWidget*	mHotspots[NUM_TEMPLES];		// +0x68
	int				mState[NUM_TEMPLES];		// +0x78
	MemoryImage*	mBackground;				// +0xa8 sky and island
	IslandPanel*	mPanel;						// +0x88
	int				mTicks;
	int				mSeaFrame;					// +0xbc
	int				mSmokeFrame;				// +0xc8
	double			mSeaBlend;					// +0xc0
	double			mPhase;						// +0xd8
	double			mGlow[NUM_TEMPLES];			// the hover glow of each temple
	int				mCandidate;					// +0x90 the temple under the mouse, -1 none
	int				mDwell;						// +0x94 updates the mouse has stayed on it
	int				mShown;						// +0x8c the temple the panel shows
	double			mLightning;					// +0xd0
	int				mLightningImage;			// +0xcc
	double			mSinPhase;					// +0xa0
	bool			mActive;					// +0x98 a temple waits to be entered
	int				mWaveFlag;					// +0xe8
	int				mNextGull1, mNextGull2;		// +0xe0, +0xe4
	int				mSolvedTotal;				// +0x14c

public:
	IslandMap(PixelusApp* theApp);				// 0x4176a0
	virtual ~IslandMap();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x416300
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x416370
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x416280
	virtual void		Draw(Graphics* g) override;										// 0x4163e0
	virtual void		Update() override;
	virtual void		ButtonDepress(int theId) override;								// 0x4170d0
	virtual void		ButtonMouseEnter(int theId) override;							// 0x416180
	virtual void		ButtonMouseLeave(int theId) override;

	using Widget::Resize;

	double				Rand01();
};

}

#endif
