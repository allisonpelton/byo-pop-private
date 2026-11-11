#ifndef __TYPERSHARK_BACKGROUNDFISH_H__
#define __TYPERSHARK_BACKGROUNDFISH_H__

// BackgroundFish.cpp: the small fish that swim across the back of the board for decoration. The board makes a school
// every 400 ticks (a leader from the first constructor, the others copy it with the second) and removes a fish that
// has passed the right edge. A fish is a colourised picture of the app (mColorizedFish), 20 frames side by side.
// Original: class Sexy::BackgroundFish : GameObject (the only base, so the GameObject vptr is at +0 and the slots are
// direct), vftable 0x5296fc (deleting destructor 0x461e80 shared with Bubble and Player, Update 0x401f40, Draw 0x402d70,
// the rest GameObject's), sizeof 0xf8; constructors 0x405fc0 (a new fish) and 0x401df0 (a copy of another one). The
// field comments give the offsets in the original's object.

#include "GameObject.h"

namespace Sexy
{

class Graphics;
class Image;
class WinWSApp;

class BackgroundFish : public GameObject
{
public:
	WinWSApp*				mApp;					// +0xf0 constructor argument 1
	Image*					mImage;					// +0xf4 the colourised picture (chosen by the speed, or the template's)
	bool					mFollower;				// port: made by the copy constructor (the fixes mod's schools count only the leaders)

public:
	BackgroundFish(WinWSApp* theApp);										// 0x405fc0
	BackgroundFish(WinWSApp* theApp, BackgroundFish* theTemplate);			// 0x401df0
	virtual ~BackgroundFish();												// deleting destructor 0x461e80

	virtual void			Update() override;								// slot 1, 0x401f40
	virtual void			Draw(Graphics* g) override;						// slot 2, 0x402d70
};

}

#endif
