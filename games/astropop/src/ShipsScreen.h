#ifndef __ASTROPOP_SHIPSSCREEN_H__
#define __ASTROPOP_SHIPSSCREEN_H__

// ShipsScreen: the hidden surprise of the main menu (original class of 0x70 bytes, constructor 0x434ac0, vftable 0x563f00).
// When the six buttons have been touched in the secret order and the last one is left, the four ships of the pilots fly up
// the screen over a background that runs through the colours, each on its flame, with the word "GO" over them, and the menu
// has its way back when the ships are gone. Offsets in the comments are the original's.

#include "Widget.h"

namespace Sexy
{

class LevelUpText;
class MainMenu;

class ShipsScreen : public Widget
{
public:
	int				mHue;               // +0x58 the background's colour (0 .. 255, 4 more each update)
	int				mY;                 // +0x5c where the ships are (they start below the screen and rise by 4)
	int				mGlow;              // +0x60 how bright the flames' second layer is (0 .. 255)
	int				mGlowStep;          // +0x64 how it changes each update (16, and the other way at either end)
	MainMenu*		mMenu;              // +0x68
	LevelUpText*	mGo;                // +0x6c the picture of the word

public:
	explicit ShipsScreen(MainMenu* theMenu);        // 0x434ac0
	virtual ~ShipsScreen();                         // 0x436240

	virtual void	Draw(Graphics* g) override;     // 0x434bc0
	virtual void	Update() override;              // 0x436270
};

}

#endif
