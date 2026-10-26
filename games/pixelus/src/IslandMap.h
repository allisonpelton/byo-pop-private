#ifndef __PIXELUS_ISLANDMAP_H__
#define __PIXELUS_ISLANDMAP_H__

// IslandMap.cpp: the map of the island with its ten temples (Pixelus.exe 0x4176a0 constructor, 0x4163e0 Draw,
// 0x4170d0 buttons; vftable 0x50df08, listener 0x50dee8). A temple shows once the player has solved enough
// puzzles; one with puzzles left glows and has an arrow over it, a finished one shows its second picture, and a
// medal when every puzzle was solved the best way. Each temple is a button (ids 2-11), the back button has id 1.
//
// port: the lightning, the birds and the particle bursts of a temple that has just opened are not ported; the sea
// and the smoke of the volcano move as in the original. The note that names the temple under the mouse is the
// port's own.

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
	int				mTicks;
	int				mSeaFrame;					// +0xbc
	int				mSmokeFrame;				// +0xc8
	int				mHover;						// the temple under the mouse, -1 none

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

private:
	void				DrawTemples(Graphics* g);
	void				DrawLabel(Graphics* g, int theTemple);
};

}

#endif
