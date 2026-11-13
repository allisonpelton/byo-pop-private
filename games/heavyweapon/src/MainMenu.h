#ifndef __HEAVYWEAPON_MAINMENU_H__
#define __HEAVYWEAPON_MAINMENU_H__

// MainMenu: the first screen after the intro: the player's name, a flickering lamp and six picture buttons (campaign,
// survival, high scores, options, how to play, quit). Pointing at a button fades in a line that describes it. The
// buttons report to the menu, except OPTIONS, which reports to the app. Original: a Widget with a ButtonListener at
// +0x58, 0x88 bytes, constructor 0x425e20, vtable 0x52dfb0 (listener 0x52df90), destructor body 0x425d40.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class FlickerLight;
class HeavyWeaponApp;
class HyperlinkWidget;
class MenuButton;

class MainMenu : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_BUTTONS = 6,
		BUTTON_CAMPAIGN = 0,
		BUTTON_SURVIVAL = 1,
		BUTTON_HIGH_SCORES = 2,
		BUTTON_QUIT = 5,
		BUTTON_HELP = 4,
		BUTTON_USER = 100,			// "If this isn't you, click here"
		BUTTON_QUIT_YES = 0x23,		// the answers of the quit box
		BUTTON_QUIT_NO = 0x24,
		SURVIVAL_STAGES_NEEDED = 3	// the stages of the campaign that unlock survival
	};

	// The buttons are in this order in mButtons (and their descriptions in the texts); their ids are the ones above, except
	// that OPTIONS is HeavyWeaponApp::BUTTON_OPTIONS
	int				mGlowAlpha;						// +0x5c the glow picture's alpha: rises while no button is under the mouse
	HeavyWeaponApp*	mApp;							// +0x60
	HyperlinkWidget*	mUserButton;				// +0x64 id 100
	MenuButton*		mButtons[NUM_BUTTONS];			// +0x68..+0x7c
	FlickerLight*	mFlickerLight;					// +0x80
	int				mState;						// +0x84 0 while the menu is shown

public:
	MainMenu(HeavyWeaponApp* theApp);							// 0x425e20
	virtual ~MainMenu();										// 0x426a4a (body 0x425d40)

	virtual void	AddedToManager(WidgetManager* theManager) override;			// 0x425b00 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;		// 0x425b60 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x425c40 slot 13
	virtual void	Draw(Graphics* g) override;									// 0x426140 slot 16
	virtual void	Update() override;											// 0x425ba0 slot 17
	virtual void	ButtonPress(int theId) override;							// 0x425d20 listener slot 4
	virtual void	ButtonDepress(int theId) override;							// 0x4265c0 listener slot 5

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
