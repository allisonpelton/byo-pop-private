#ifndef __ASTROPOP_MAINMENU_H__
#define __ASTROPOP_MAINMENU_H__

// MainMenu: the game selector (original class, constructor 0x454ee0, sizeof 0xd8, vftable 0x5650d8 with 47 slots and
// a ButtonListener at +0x58 with vftable 0x5650b4; destructor body 0x44e5c0). It shows the projector screen with five
// image buttons that light up on a hover (Classic, Survival, Hall of Fame, Options, Quit) and the player's name
// above them. In 3D mode glowing particles rise from the projector. A hidden sequence of hovers over the buttons
// (see HoverStep) shows a screen of ships flying off.
// Left out of the port: the "More Games" button (id 0xfad, index 4), a link out of the game (its rectangle and
// highlight stay in the tables and are never used).
// Offsets in the comments are the original's.

#include "Text2004.h"
#include "Widget.h"
#include "ButtonListener.h"
#include "FrameAnim.h"
#include <vector>

namespace Sexy
{

class AstroPopApp;
class AstroButton;

// One rising particle (0x4c bytes: an animation of IMAGE_PARTICLE_RADIAL's cels and its position)
struct MenuParticle
{
	FrameAnim	mAnim;      // +0x00
	int			mAlpha;     // +0x40
	int			mX;         // +0x44
	int			mY;         // +0x48
};

class MainMenu : public Text2004<Widget>, public ButtonListener
{
public:
	enum
	{
		NUM_BUTTONS = 6,
		BUTTON_ID_BASE = 0xfa9,         // buttons 0xfa9 .. 0xfae
		BUTTON_ID_CLASSIC = 0xfa9,
		BUTTON_ID_SURVIVAL = 0xfaa,
		BUTTON_ID_HALL_OF_FAME = 0xfab,
		BUTTON_ID_OPTIONS = 0xfac,
		BUTTON_ID_MORE_GAMES = 0xfad,
		BUTTON_ID_QUIT = 0xfae,
		BUTTON_ID_USER = 0xfaf,         // the player-name button above the projector
		NUM_PANELS = 7                  // the centre piece and one highlight for each button
	};

	std::vector<AstroButton*>	mButtons;           // +0x60 (begin), six entries; index 4 (More Games) stays NULL in the port
	std::vector<MenuParticle*>	mParticles;         // +0x70
	AstroButton*				mUserButton;        // +0x7c id 0xfaf
	AstroPopApp*				mApp;               // +0x80
	int							mSpeed[NUM_PANELS]; // +0x84 how fast the panel's alpha moves (12 up, -8 down)
	int							mAlpha[NUM_PANELS]; // +0xa0 the panel's alpha: 0 centre piece shown, the others fade in on a hover
	int							mParticleLevel;     // +0xc0 how many particles are made each time (0 .. 5), rises while a button is hovered
	int							mHoverStep;         // +0xc4 the hidden hover sequence: -5 for none, 0 .. 8 steps done
	int							mEasterTimer;       // +0xc8 updates since the sequence was finished
	bool						mHovering;          // +0xcc the pointer is over one of the buttons
	bool						mEasterPending;     // +0xcd the sequence is finished: the ships fly off when mEasterTimer reaches 50
	Widget*						mGoScreen;          // +0xd0 the screen of the flying ships (ShipsScreen) while it is up
	bool						mInactive;          // +0xd4 constructed false

public:
	MainMenu(AstroPopApp* theApp);              // 0x454ee0
	virtual ~MainMenu();                        // 0x4552b0 (body 0x44e5c0)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;       // 0x43c230
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x43c290
	virtual void	Draw(Graphics* g) override;             // 0x4461b0
	virtual void	Update() override;                      // 0x4552e0

	virtual void	ButtonPress(int theId) override;        // 0x434f60 (plays SOUND_BUTTON_DOWN)
	void			CloseGoScreen();                        // 0x434f80
	virtual void	ButtonDepress(int theId) override;      // 0x446410
	virtual void	ButtonMouseEnter(int theId) override;   // 0x434dc0
	virtual void	ButtonMouseLeave(int theId) override;   // 0x434ef0

	using Widget::KeyDown;
};

}

#endif
