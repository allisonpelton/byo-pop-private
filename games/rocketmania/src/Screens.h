#ifndef __ROCKETMANIA_SCREENS_H__
#define __ROCKETMANIA_SCREENS_H__

// The full-screen widgets of Rocket Mania Deluxe: LoadingScreen (RocketManiaGame 0x4104b0-0x411860) and the
// game's other screens. Each is a top-level widget (the 2003 framework has no child widgets) that routes its
// buttons to itself.

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include "DialogButton.h"
#include "Effects.h"
#include <string>

namespace Sexy
{

class RocketApp;

// 0x4104b0: the loading picture, a bar that fills as the game's files load, and the link that starts the game.
// port: the shareware branches (the trial counters, the register link, "You must register online now") are not
// ported; the Steam build's partner.xml says NoReg and the screen always takes the registered path.
class LoadingScreen : public Widget, public ButtonListener
{
public:
	RocketApp*			mApp;                   // +0x5c
	double				mProgress;              // +0x60 the bar's fill, 0..1
	bool				mLinkShown;             // +0x70 the play link has been shown
	HyperlinkWidget*	mPlayLink;              // +0x94 id 1 "click here to play"

public:
	LoadingScreen(RocketApp* theApp);
	virtual ~LoadingScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
};

// 0x404e30: the menu (picture "select back"): the three game modes, the high scores, the options and the quit button.
// A mode button opens the difficulty dialog (or, with a saved game of that mode, the continue dialog); the
// difficulty dialog starts the game with StartGame.
// port: the talking head that introduces the first game (0x4161a0) is not translated yet; its calls are marked "not
// ported" below. Over the picture blow the leaves (an effect of kind 2 that never ends, 0x41a830).
class GameSelector : public Widget, public ButtonListener
{
public:
	enum
	{
		BUTTON_ARCADE = 0,
		BUTTON_CLASSIC = 1,
		BUTTON_STRATEGY = 2,
		BUTTON_QUIT = 3,
		BUTTON_HIGH_SCORES = 4,
		BUTTON_OPTIONS = 5
	};

	RocketApp*			mApp;                   // +0x80
	DialogButton*		mModeButtons[3];        // +0x5c arcade (id 0), +0x60 classic (id 1), +0x64 strategy (id 2)
	DialogButton*		mHighScoresButton;      // +0x68 id 4
	DialogButton*		mOptionsButton;         // +0x6c id 5
	DialogButton*		mQuitButton;            // +0x70 id 3
	EffectManager*		mEffects;               // +0x7c the leaves that blow over the picture
	int					mAlpha;                 // +0x74 the hint text's fade-in, 0 to 255 in steps of 10
	int					mHint;                  // +0x78 which hint text shows: 0 none hovered, 1-3 a mode, 4-6 the lower buttons
	int					mDifficulty;            // +0x84 the chosen difficulty, 1 easy to 3 hard (-1 none)
	int					mMode;                  // +0x88 the chosen mode, 0 arcade, 1 classic, 2 strategy (-1 none)
	int					mYOffset;               // +0x8c the picture's vertical offset (always 0)

public:
	GameSelector(RocketApp* theApp);
	virtual ~GameSelector();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;

	void				ShowDifficultyDialog();     // 0x405fb0
	void				ShowContinueDialog();       // 0x4060d0
	void				StartGame(bool theContinuing);  // slot 44, 0x405ed0
	void				ResetHead();                // 0x405950 / 0x405980: the talking head's place (not ported)
};

}

#endif
