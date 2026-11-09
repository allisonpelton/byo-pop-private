#ifndef __TYPERSHARK_GAMEOVERDIALOG_H__
#define __TYPERSHARK_GAMEOVERDIALOG_H__

// GameOverDialog.cpp: "GAME OVER" (dialog 0x16062, made by Board::PlayerEaten when the diver's last life is gone). The normal
// games show the score, the rank title, the level, the typing speed, the accuracy and the time; the abyss games show the
// score, the best score, the depth, the best depth, the same speed, level ("Abyss"), accuracy and time lines and a note
// that the progress is saved every 400 feet. The two buttons are "play again" (yes, id 0x16832) and "main menu" (no,
// id 0x16c1a), which the app's ButtonDepress takes.
// Original: class Sexy::GameOverDialog : public Dialog, vftable 0x52e61c (52 slots; own: 0 destructor, 3
// OrderInManagerChanged 0x4380a0, 16 Draw 0x43b250), ButtonListener vftable 0x52e5fc (+0x58, all Dialog's), sizeof 0x154,
// constructor 0x43adb0 (7 arguments), destructor body 0x43b200 (the scalar deleting destructor is 0x43b1d0).

#include "compat2004/Compat2004.h"
#include <string>

namespace Sexy
{

class WinWSApp;
class Player;
class Graphics;

class GameOverDialog : public Dialog2004
{
public:
	Player*					mPlayer;				// +0x110 constructor argument (the score and the level number come from it)
	WinWSApp*				mApp;					// +0x114 constructor argument
	bool					mAbyss;					// +0x118 constructor argument (the abyss game's layout is taller)
	int						mField11c;				// +0x11c the game's time in milliseconds; not set by the constructor, the board sets it after making the dialog (the port sets 0 first)
	int						mWPM;					// +0x120 the adjusted typing speed, -1 when it is not shown (the time and the level are too short)
	int						mAccuracy;				// +0x124 the percentage of the keystrokes that were right, -1 when it is not shown
	std::string				mRankTitle;				// +0x128 the board sets it from GetRankTitle (shown in the normal games only)
	int						mField144;				// +0x144 constructor argument (the score; the abyss depth of an abyss game, which the board sets again)
	int						mCharsTyped;			// +0x148 constructor argument (the characters typed right in the game)
	int						mMistakes;				// +0x14c constructor argument (the wrong keystrokes)
	int						mTime;					// +0x150 constructor argument (the time of the typing in milliseconds)

public:
	GameOverDialog(WinWSApp* theApp, Player* thePlayer, bool isAbyss, int theTime, int theCharsTyped, int theMistakes,
		int theScore);																		// 0x43adb0
	virtual ~GameOverDialog() override;														// 0x43b200 (deleting 0x43b1d0), slot 0

	virtual void			OrderInManagerChanged() override;								// 0x4380a0 slot 3
	virtual void			Draw(Graphics* g) override;										// 0x43b250 slot 16
};

}

#endif
