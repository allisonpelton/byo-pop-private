#ifndef __HIGHSCOREWIDGET_H__
#define __HIGHSCOREWIDGET_H__

// HighScoreWidget.h: the panel of high scores that slides in after a game (Sexy::HighScoreWidget, 0x5c352e-0x5c530a).
// TODO: a stub until the board is ported.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>

namespace Sexy
{

class WinDMApp;

class HighScoreWidget : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c

public:
	HighScoreWidget(WinDMApp* theApp) { mApp = theApp; }
	virtual void	ButtonDepress(int theId) override {}
	void			SetHighScoreName(const std::string& theName) {}
	void			ResetHighScores() {}
};

}

#endif
