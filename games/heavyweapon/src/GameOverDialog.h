#ifndef __HEAVYWEAPON_GAMEOVERDIALOG_H__
#define __HEAVYWEAPON_GAMEOVERDIALOG_H__

// GameOverDialog: the box over the board when the last tank is lost (Board::DIALOG_GAME_OVER). A campaign shows the
// total score (blinking "new high score" when it placed) beside the enemies destroyed, the kill percentage and the
// friendlies destroyed; survival shows the time survived (blinking "top 10 time" when it placed) and the player's best
// time before the same counts. TRY AGAIN and QUIT report to the board (Board::BUTTON_GAME_OVER_CONTINUE and
// BUTTON_GAME_OVER_SCORES). Opening it tells the board to stop.
// Original: a Dialog of 0x124 bytes, constructor 0x41fe40, vtable 0x52d220 (listener 0x52d200), destructor body
// 0x41fc20 (deleting 0x41fca0).

#include "Compat2004.h"

namespace Sexy
{

class Board;
class BracketButton;
class HeavyWeaponApp;

class GameOverDialog : public Dialog2004
{
public:
	Board*			mBoard;				// +0x114
	HeavyWeaponApp*	mApp;				// +0x118
	BracketButton*	mTryAgainButton;	// +0x11c
	BracketButton*	mQuitButton;		// +0x120

public:
	GameOverDialog(HeavyWeaponApp* theApp);													// 0x41fe40
	virtual ~GameOverDialog();																// 0x41fca0 (body 0x41fc20)

	virtual void	AddedToManager(WidgetManager* theManager) override;						// 0x41d160 slot 1 (YesNoDialog's code)
	virtual void	RemovedFromManager(WidgetManager* theManager) override;					// 0x41d2b0 slot 2 (YesNoDialog's code)
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;			// 0x41fb80 slot 13
	virtual void	Draw(Graphics* g) override;												// 0x4201c0 slot 16
	virtual void	Update() override;														// 0x41fc90 slot 17

	using Dialog::Resize;
};

}

#endif
