#ifndef __HAMMERHEADS_GAMEOVERDIALOG_H__
#define __HAMMERHEADS_GAMEOVERDIALOG_H__

// "Too bad...": the box a game ends with (dialog 0xf), telling how far the player got. "Try Again" starts a new game and "Main
// Menu" leaves for the menu. A classic game on a level that introduces a gnome (gnomedata\<level>.txt) also shows that gnome
// as a dark shape under a question mark, and the box is taller for it.
// Original: class Sexy::GameOverDialog : public NcMovingDialog (0x170 bytes; the constructor is 0x44ff70, the destructor 0x44fc70).

#include "NcMovingDialog.h"

namespace Sexy
{

class Creature;

class GameOverDialog : public NcMovingDialog
{
public:
	Creature*		mCreature;			// +0x168: the gnome of the next level (dark), or null

public:
	GameOverDialog();														// 0x44ff70
	virtual ~GameOverDialog();												// 0x44fc70

	virtual void	Update() override;										// 0x44fe10
	virtual void	Draw(Graphics* g) override;								// 0x4502e0
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x44ff10
	virtual void	ButtonDepress(int theId) override;						// 0x44fe50: the code the quit box has too
	virtual void	Closed() override;										// 0x44fee0: "Try Again" or "Main Menu" is answered

private:
	void			DrawQuestionMark(Graphics* g, int theTime, int theOffsetY, double theSwaySpeed, double theBobSpeed);	// 0x44fd00
};

// 0x450aa0: the box is made, sized and added as dialog 0xf
GameOverDialog* MakeGameOverDialog();

}

#endif
