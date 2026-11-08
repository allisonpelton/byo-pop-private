#ifndef __HIGHSCORENAMEDIALOG_H__
#define __HIGHSCORENAMEDIALOG_H__

// HighScoreNameDialog.h: the dialog that asks for the name of a new high score (Sexy::RecordScoreDialog in the original,
// 0x5d31e4-0x5d34c8 and the constructor 0x5d3382; the object is 0x190 bytes). It is a PodDialog (id 10) with an edit
// field below the text; Enter in the field presses the dialog's OK button. Nothing in the original's code opens it any
// more: WinDMApp::FUN_005dcdf0 (0x5dcdf0, WinDMApp::DoHighScoreNameDialog here) makes one, but has no callers, and the
// high score table takes the name of the profile (HighScoreWidget::NameEntered).

#include "PodDialog.h"
#include "EditListener.h"

namespace Sexy
{

class EditWidget;

class HighScoreNameDialog : public PodDialog, public EditListener
{
public:
	// (the original keeps a second copy of the app pointer at +0x188: the one of PodDialog, at +0x150, is used here)
	EditWidget*		mEdit;              // +0x18c

public:
	HighScoreNameDialog(WinDMApp* theApp, const SexyString& theLines);      // 0x5d3382
	virtual ~HighScoreNameDialog();                                         // 0x5d31e4

	virtual int		GetPreferredHeight(int theWidth) override;              // 0x5d323b
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5d3265
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5d3293
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5d32b6
	virtual void	Draw(Graphics* g) override;                             // 0x5d34c8
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;    // 0x5d334d
};

}

#endif
