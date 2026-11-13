#ifndef __HEAVYWEAPON_HIGHSCORESSCREEN_H__
#define __HEAVYWEAPON_HIGHSCORESSCREEN_H__

// HighScoresScreen: the ten best campaign scores or the ten longest survivals, over a screen of rising glowing bubbles.
// Buttons: MAIN (back to the main menu), MISSION and SURVIVAL (the list shown) and RESET (asks to put the shown list back
// to its defaults). When a game has just ended, the line that player got blinks. Original: a Widget with a ButtonListener
// at +0x58, 0x8c bytes, constructor 0x4224e0, vtable 0x52d888 (listener 0x52d868), destructor body 0x422410 (deleting
// 0x4224b0).

#include "Widget.h"
#include "ButtonListener.h"
#include <list>

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;

// A glowing bubble drifting up the screen (0x20 bytes, kept in a list)
struct HighScoreBubble
{
	double	mX;			// +0x00
	double	mY;			// +0x08
	double	mLife;		// +0x10 1.0 at the start; its colour fades with it
	double	mSpeed;		// +0x18 pixels up each update
};

class HighScoresScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		BUTTON_MAIN = 0,
		BUTTON_MISSION = 1,
		BUTTON_SURVIVAL = 2,
		BUTTON_RESET = 3,
		BUTTON_RESET_SURVIVAL_YES = 4,	// the answers of the box that asks before a reset
		BUTTON_RESET_MISSION_YES = 5,
		BUTTON_RESET_NO = 0x22,
		NO_HIGHLIGHT = 10				// mHighlightRank when no line is to blink
	};

	HeavyWeaponApp*			mApp;				// +0x5c
	BracketButton*			mMainButton;		// +0x60 id 0
	BracketButton*			mMissionButton;		// +0x64 id 1
	BracketButton*			mSurvivalButton;	// +0x68 id 2
	BracketButton*			mResetButton;		// +0x6c id 3
	std::list<HighScoreBubble>	mBubbles;		// +0x70
	bool					mShowSurvival;		// +0x80 the survival list is shown
	int						mHighlightRank;		// +0x84 the line to blink (0-based; NO_HIGHLIGHT for none)
	bool					mHighlightSurvival;	// +0x88 the line to blink is in the survival list

public:
	HighScoresScreen(HeavyWeaponApp* theApp, bool theShowSurvival, int theHighlightRank);	// 0x4224e0
	virtual ~HighScoresScreen();								// 0x4224b0 (body 0x422410)

	virtual void	AddedToManager(WidgetManager* theManager) override;			// 0x422190 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;		// 0x4221e0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x422230 slot 13
	virtual void	Draw(Graphics* g) override;									// 0x422780 slot 16
	virtual void	Update() override;											// 0x4222b0 slot 17
	virtual void	ButtonPress(int theId) override;							// 0x410670 listener slot 4
	virtual void	ButtonDepress(int theId) override;							// 0x423450 listener slot 5

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
