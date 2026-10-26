#ifndef __ROCKETMANIA_DIALOGS_H__
#define __ROCKETMANIA_DIALOGS_H__

// The dialogs of Rocket Mania Deluxe. All of them are 2003 dialogs (id + 2000 / + 3000 for the dialog's own buttons,
// docs/FRAMEWORK_2003.md) drawn by the game itself: the dialog picture with a title in the Showcard font, the
// game's own buttons (DialogButton, the dark button picture) and an app-wide click sound.

#include "Compat2004.h"
#include "DialogButton.h"

namespace Sexy
{

class RocketApp;

// 0x404380: id 0xc "Difficulty": EASY, MEDIUM, HARD and CANCEL (the ids 0 to 3)
class DifficultyDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	DialogButton*		mButtons[4];            // +0xf0 easy, +0xf4 medium, +0xf8 hard, +0xfc cancel

public:
	DifficultyDialog(RocketApp* theApp);
	virtual ~DifficultyDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x402630: id 0xf "Continue": a saved game of the mode exists; NEW GAME or CONTINUE SAVED GAME
class ContinueDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0x100
	int					mMode;                  // +0xec the mode whose saved game it is
	DialogButton*		mNewButton;             // +0x104 id 0
	DialogButton*		mContinueButton;        // +0x108 id 1
	int					mSavedDifficulty;       // +0xf4 read from the save file (1 to 3)
	int					mSavedScore;            // +0xf8
	int					mSavedLevel;            // +0xfc the level index (shown plus one)

public:
	ContinueDialog(RocketApp* theApp, int theMode);
	virtual ~ContinueDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

}

#endif
