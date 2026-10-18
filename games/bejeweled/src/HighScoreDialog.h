#ifndef __HIGHSCOREDIALOG_H__
#define __HIGHSCOREDIALOG_H__

// HighScoreDialog.cpp: modal "HIGH SCORES" dialog (id 13) opened from the Options dialog.
// Original: class Sexy::HighScoreDialog : public Dialog, sizeof 0x158, vftable 0x50686c
// (ButtonListener 0x50684c, inherited Dialog behaviour). Offsets are absolute; Ghidra's
// HighScoreDialog_data.offset_N is absolute N+0x100.
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (HighScoreDialog section).

#include "Dialog.h"

namespace Sexy
{

class WinDMApp;
class Graphics;
class HighScoreWidget;

class HighScoreDialog : public Dialog
{
public:
	enum
	{
		PREFERRED_HEIGHT = 326
	};

	HighScoreWidget* mHighScoreWidget;      // +0x150 table owner (loads hiscores.dat; its dtor re-saves it); never added to the WidgetManager
	WinDMApp*		mApp;                   // +0x154

public:
	virtual ~HighScoreDialog();                                     // 0x4ce44f (scalar deleting dtor 0x4ce49d)
	virtual int		GetPreferredHeight(int theWidth);               // 0x4ce495 slot 73 (returns 326)
	HighScoreDialog(WinDMApp* theApp);                              // 0x4ce4bc
	virtual void	Draw(Graphics* g);                              // 0x4ce60b slot 26
};

}

#endif
