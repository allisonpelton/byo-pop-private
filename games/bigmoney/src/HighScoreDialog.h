#ifndef __HIGHSCOREDIALOG_H__
#define __HIGHSCOREDIALOG_H__

// HighScoreDialog.cpp: the modal per-mode high score tables (dialog 13, Options > Scores).
// Original: class Sexy::HighScoreDialog : public MoneyDialog, sizeof 0x160, vftable 0x4f60fc
// (60 slots), ButtonListener 0x4f60d8 (Dialog's handlers). Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/dialogs.md (section 2).

#include "MoneyDialog.h"

namespace Sexy
{

class WinBMApp;
class Graphics;
class HighScoreWidget;

class HighScoreDialog : public MoneyDialog
{
public:
	enum
	{
		PREFERRED_HEIGHT = 396,
		TABLE_Y = 67,               // row 0 of table 0; the title is 19 above
		TABLE_PITCH = 94,
		ROW_PITCH = 15,
		COLUMN_INSET = 48           // names at x 48, scores right-aligned at mWidth - 48
	};

	HighScoreWidget* mHighScoreWidget;      // +0x158 private table owner: loads hiscores.dat, its dtor re-saves it; never added to a manager
	WinBMApp*		mApp;                   // +0x15c

public:
	virtual ~HighScoreDialog();                                     // 0x412280 (scalar deleting dtor 0x4122f0 slot 0)
	HighScoreDialog(WinBMApp* theApp);                              // 0x412310
	virtual void	Draw(Graphics* g);                              // 0x4125c0 slot 13
	virtual int		GetPreferredHeight(int theWidth);               // 0x40ee80 slot 57 (one body with CreditsDialog's: 396)
};

}

#endif
