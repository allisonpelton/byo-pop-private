#ifndef __TYPERSHARK_BUBBLE_H__
#define __TYPERSHARK_BUBBLE_H__

// Bubble.cpp: an air bubble that rises through the water (the board makes them, for instance when a shark dies, with
// Board::AddBubble(x, y), which sets the position after the constructor). Each is one of the three bubble pictures
// and drifts a little sideways now and then.
// Original: class Sexy::Bubble : GameObject (the only base, so the GameObject vptr is at +0 and the slots are direct),
// vftable 0x5319d4 (deleting destructor 0x461e80 shared with BackgroundFish and Player, Update 0x461140, Draw 0x461eb0,
// the rest GameObject's), sizeof 0x100; constructor 0x461d50. The field comments give the offsets in the original's
// object. The constructor starts a bubble near the bottom of the screen; the picture is chosen by mFrame (0 to 2).

#include "GameObject.h"

namespace Sexy
{

class Graphics;
class Image;
class WinWSApp;

class Bubble : public GameObject
{
public:
	Image*					mBubbleImages[3];		// +0xf0 IMAGE_BUBBLE1, IMAGE_BUBBLE2, IMAGE_BUBBLE3 (mFrame picks one); +0xfc is padding

public:
	Bubble(WinWSApp* theApp);														// 0x461d50
	virtual ~Bubble();																// deleting destructor 0x461e80

	virtual void			Update() override;										// slot 1, 0x461140
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x461eb0
};

}

#endif
