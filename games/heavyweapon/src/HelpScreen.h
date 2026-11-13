#ifndef __HEAVYWEAPON_HELPSCREEN_H__
#define __HEAVYWEAPON_HELPSCREEN_H__

// HelpScreen: "HOW TO PLAY": a board that plays itself. In four turns the tank follows the pointer and fires at a plane and a
// helicopter, dodges the helicopter's shot, collects a power-up that the friendly white helicopter drops, and launches a
// nuke; a line of text says what to do in each, and the player's real pointer is not used. An OK button closes it (or, when
// a listener was given, reports to the listener with id 0x3f), and a checkbox keeps it from showing at the start of the game.
// Original: a Board (0x398 bytes: the board's 0x368 and a CheckboxListener at +0x368 that the board does not have), constructor
// 0x421410, vtable 0x52d408 (button listener 0x52d3e8, checkbox listener 0x52d3e4), destructor body 0x4209b0 (deleting
// 0x420d80). The vtable gives the board's keys and mouse the Widget's own (do nothing) and replaces MouseEnter and
// MouseLeave with empty functions (0x420b00, 0x420af0).

#include "Board.h"
#include "CheckboxListener.h"

namespace Sexy
{

class BracketButton;
class Checkbox;
class HeavyWeaponApp;
class PropFighter;
class SupplyCopter;
class SmallCopter;

class HelpScreen : public Board, public CheckboxListener
{
public:
	enum
	{
		BUTTON_OK = 0x3f,				// HeavyWeaponApp::BUTTON_HELP_PLAY
		CHECKBOX_DONT_SHOW = 0,
	};

	// The turns (mPhase) in the order they play, each from tick 0
	enum
	{
		PHASE_FIRE,						// the tank follows the pointer and fires
		PHASE_DODGE,					// a plane and a helicopter come in; the helicopter shoots
		PHASE_POWER_UP,					// the friendly helicopter drops a power-up
		PHASE_NUKE						// a stream of planes, and a nuke
	};

	BracketButton*	mOkButton;			// +0x36c id 0x3f
	Checkbox*		mCheckbox;			// +0x370 "Do not show this screen at startup"
	int				mPhase;				// +0x374 PHASE_
	int				mTick;				// +0x378 updates in the turn
	double			mAimAngle;			// +0x380 the angle of the sine that swings the pointer in the first turn
	int				mNukeIcon;			// +0x388 1 draws the nuke icon (a power-up has been collected, until the nuke goes)
	PropFighter*	mPropFighter;		// +0x38c the plane of the second turn
	SmallCopter*	mSmallCopter;		// +0x390 the helicopter of the second turn
	SupplyCopter*	mSupplyCopter;			// +0x394 the friendly helicopter of the third turn

public:
	HelpScreen(HeavyWeaponApp* theApp, ButtonListener* theListener);					// 0x421410
	virtual ~HelpScreen();																// 0x420d80 (body 0x4209b0)

	virtual void	AddedToManager(WidgetManager* theManager) override;				// 0x420b10 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;			// 0x420b50 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x420b90 slot 13
	virtual void	Draw(Graphics* g) override;										// 0x421620 slot 16
	virtual void	Update() override;													// 0x4212a0 slot 17
	virtual void	UpdateF(float theFrac) override;									// 0x420db0 slot 18
	virtual void	KeyChar(SexyChar theChar) override;								// 0x471be0 slot 21: nothing
	virtual void	KeyDown(KeyCode theKey) override;									// 0x471be0 slot 22: nothing
	virtual void	KeyUp(KeyCode theKey) override;									// 0x471be0 slot 23: nothing
	virtual void	MouseEnter() override;												// 0x420b00 slot 24: nothing
	virtual void	MouseLeave() override;												// 0x420af0 slot 25: nothing
	virtual void	MouseMove(int x, int y) override;									// 0x45fa60 slot 26: nothing
	virtual void	MouseDrag(int x, int y) override;									// 0x45fa60 slot 32: nothing
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override; // 0x45fa50 slot 27: nothing
	virtual void	MouseUp(int x, int y, int theBtnNum, int theClickCount) override;	// 0x45fa50 slot 29: nothing
	virtual void	ButtonDepress(int theId) override;									// 0x420bf0 listener slot 5
	virtual void	CheckboxChecked(int theId, bool checked) override;					// 0x4213c0 checkbox listener slot 0

	using Board::Resize;
	using Widget::MouseDown;
	using Widget::MouseUp;

private:
	void			UpdateFirePhase();													// 0x420c30
	void			UpdateDodgePhase();													// 0x420e10
	void			UpdatePowerUpPhase();												// 0x421160
	void			UpdateNukePhase();													// 0x420cd0
};

}

#endif
