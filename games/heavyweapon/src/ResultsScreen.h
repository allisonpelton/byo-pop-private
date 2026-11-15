#ifndef __HEAVYWEAPON_RESULTSSCREEN_H__
#define __HEAVYWEAPON_RESULTSSCREEN_H__

// ResultsScreen: the debriefing after a campaign level's boss. It darkens the board and lists, a line every 50 updates,
// the total score, the enemies destroyed and the kill percentage, then counts up the bonuses into the score: no
// friendly unit destroyed, no tank lost, the destruction bonus for a kill percentage of 90 or more; the reinforcements
// (tanks lost) appear as icons. The right panel shows the player's rank insignia and name ("promotion!" blinking when
// the level was the furthest yet). The bonuses go into the score when it opens, and the campaign is saved as if the
// next level had begun. ADVANCE goes on to the armory, or after the 9th and the last level to their cutscenes.
// Original: a Widget of 0x9c bytes with a ButtonListener at +0x58, constructor 0x426e60, vtable 0x52e488 (listener
// 0x52e468), destructor 0x426af0 (deleting 0x426c50). +0x8c is never written or read.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class Board;
class DialogButton;
class HeavyWeaponApp;

class ResultsScreen : public Widget, public ButtonListener
{
public:
	HeavyWeaponApp*	mApp;						// +0x5c
	int				mFade;						// +0x60 the darkening's alpha, 4 an update up to 128
	int				mTime;						// +0x64 updates since it opened (a click skips to the next line)
	int				mKillPercentage;			// +0x68
	int				mDestructionBonusShown;		// +0x6c the bonuses as counted up so far, and in full
	int				mDestructionBonus;			// +0x70
	int				mFriendlyFireBonusShown;	// +0x74
	int				mFriendlyFireBonus;			// +0x78
	int				mSurvivalBonusShown;		// +0x7c
	int				mSurvivalBonus;				// +0x80
	int				mReinforcementsShown;		// +0x84 the tanks lost drawn so far, and in all
	int				mReinforcements;			// +0x88
	int				mScoreShown;				// +0x90 the total score as counted up so far
	DialogButton*	mAdvanceButton;				// +0x94
	Board*			mBoard;						// +0x98 the app's board when it opened

public:
	ResultsScreen(HeavyWeaponApp* theApp);										// 0x426e60
	virtual ~ResultsScreen();													// 0x426af0 (deleting 0x426c50)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x426b50 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x426b90 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x426bc0 slot 13
	virtual void	Draw(Graphics* g) override;									// 0x427460 slot 16
	virtual void	Update() override;											// 0x426c80 slot 17
	virtual void	MouseUp(int x, int y) override;								// 0x426c00 slot 31
	virtual void	MarkDirty() override;										// 0x426c30 slot 47
	virtual void	ButtonPress(int theId) override;							// 0x410670 (listener slot 4, the board's code)
	virtual void	ButtonDepress(int theId) override;							// 0x427260 (listener slot 5)

	using Widget::Resize;
	using Widget::MouseUp;
	using ButtonListener::ButtonPress;
};

}

#endif
