#ifndef __ASTROPOP_CHARSELECTOR_H__
#define __ASTROPOP_CHARSELECTOR_H__

// CharSelector: the pilot choice screen (original class, constructor 0x4556c0, sizeof 0xb8, vftable 0x564b90 with 47
// slots and a ButtonListener at +0x58 with vftable 0x564b6c; destructor body 0x446720). Four pilots stand side by side
// (Vector, Vixx, Sprocket and Turbot), each with a button that lights up on a hover, a picture of what the pilot can do
// (closed under a lock while the pilot is not open for the player), and under it how far the player has got with the
// pilot. A fifth button at the bottom goes back to the game selector.
// Left out of the port: the registration locks (the lock pictures of Sprocket and Turbot's second cover, shown while the
// copy is not registered).
// Offsets in the comments are the original's.

#include "Widget.h"
#include "ButtonListener.h"
#include <vector>

namespace Sexy
{

class AstroPopApp;
class AstroButton;
class Image;

class CharSelector : public Widget, public ButtonListener
{
public:
	enum
	{
		PILOT_VECTOR = 0,
		PILOT_SPROCKET = 1,
		PILOT_TURBOT = 2,
		PILOT_VIXX = 3,
		BUTTON_BACK = 4,
		NUM_PILOTS = 4
	};

	std::vector<AstroButton*>	mButtons;           // +0x60 (begin) the four pilots, then the back button
	std::vector<int>			mStats;             // +0x70 (begin) how far the pilot's upgrades go (0 .. 8), by pilot
	AstroPopApp*				mApp;               // +0x8c
	bool						mTurbotLocked;      // +0x90 the player has not opened Turbot yet
	bool						mVixxLocked;        // +0x91 the player has not opened Vixx yet
	int							mPercent[NUM_PILOTS];   // +0x94 the percent shown in the second mode (0 .. 100), by pilot
	int							mBest[NUM_PILOTS];      // +0xa4 the best level of the first mode (a multiple of four), by pilot
	int							mMode;              // +0xb4 0: the first mode (the title shows ARCADE), 1 and 2: the second mode

public:
	CharSelector(AstroPopApp* theApp, int theMode);     // 0x4556c0
	virtual ~CharSelector();                            // 0x44bf50 (body 0x446720)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;       // 0x43c2d0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x43c310
	virtual void	Draw(Graphics* g) override;             // 0x43e300

	virtual void	ButtonPress(int theId) override;        // 0x434fd0 (plays SOUND_BUTTON_DOWN)
	virtual void	ButtonDepress(int theId) override;      // 0x434ff0

private:
	void			DrawPilotLabel(Graphics* g, int thePilot, int theX);   // the best level, the percent or the best time under a pilot
	void			DrawStatBar(Graphics* g, int thePilot, Image* theFill); // the bar of how far the pilot's upgrades go
};

}

#endif
