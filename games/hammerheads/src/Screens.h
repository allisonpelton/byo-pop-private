#ifndef __HAMMERHEADS_SCREENS_H__
#define __HAMMERHEADS_SCREENS_H__

// What Application.cpp needs of the screens and dialogs (0x44c170-0x462d00). These are declared first and written as the
// original's classes are translated, one file each: ScreenStubs.cpp has a plain stand-in of each, deleted as the real
// one arrives. A class gets its own header (named for it) when it is translated, and this header then includes it.

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "Color.h"
#include "Dialog.h"
#include "Widget.h"

namespace Sexy
{

class Image;
class GameScreen;

// 0x458ec0 (the constructor), 0x459120, 0x458b70: the bar and the pictures shown while the game's files load
class LoadingScreen : public Widget
{
public:
	LoadingScreen();
	void			Begin();						// 0x459120: called once the screen is sized, before it is added
	void			LoadingFinished();				// 0x458b70: the files are loaded (Application::LoadingThreadCompleted)
	virtual void	Draw(Graphics* g) override;
	virtual void	Update() override;
	bool			mLoaded = false;				// (stand-in)
};

// 0x459a60: the main menu
class MainMenu : public Widget, public ButtonListener
{
public:
	MainMenu();
	void			SetPlayButtonImage(Image* theImage);	// 0x4594e0: the picture of the play button
	void			UserChanged();					// 0x459540, 0x459520: the player has changed, the menu shows the new one's
	virtual void	Draw(Graphics* g) override;
	virtual void	ButtonPress(int theId) override;
	virtual void	MouseDown(int x, int y, int theClickCount) override;
};

// 0x457f80: the page that shows what is new in a level (null from MakeLevelIntro when the level has none)
class LevelIntroWidget : public Widget
{
public:
};
LevelIntroWidget* MakeLevelIntro(GameScreen* theGameScreen);	// 0x4580b0

// 0x460c00: the trophies (theMode 0 from the main menu, 1 from the game, with the ones just won)
class TrophyRoomWidget : public Widget
{
public:
	TrophyRoomWidget(int theMode, const std::vector<int>& theTrophies);
};

// 0x4534c0: the screen of a game that was won
class GameWonWidget : public Widget
{
public:
};
GameWonWidget* MakeGameWon();							// 0x4535c0

// 0x45d800: the choice of a level to start again from
class ResurrectScreenWidget : public Widget
{
public:
	ResurrectScreenWidget(bool theFlag);
};

// 0x462d00, 0x462eb0: the picture that grows over the screen between two screens
class ZoomWipeWidget : public Widget
{
public:
	ZoomWipeWidget();
	// theCallback runs when the picture covers the screen; the wipe then ends (Application::EndZoomWipe)
	void			Setup(Image* theFrom, Image* theTo, const Color& theColor, void (*theCallback)());
	virtual void	Draw(Graphics* g) override;
	virtual void	Update() override;
	void			(*mCallback)() = nullptr;		// (stand-in)
};

// 0x456c70: the how-to-play movie (skipped: it waits for the shared SWF player)
class HowToPlayWidget : public Widget
{
public:
};

// 0x456080: the high score table, shown or only updated
class HighScoreDisplayDialog : public Dialog
{
public:
	HighScoreDisplayDialog();
	void			Prepare();						// 0x456490
	void			RecordScore(const std::string& theName);	// 0x455f00
	bool			mShown;							// +0x155
};
HighScoreDisplayDialog* MakeHighScoreDisplay();			// 0x456810: made, sized and added as dialog 0xe

// 0x44e110 (the constructor), 0x44e250: every dialog the application makes is one of these (Application::NewDialog): it comes down
// from above the screen, stops at the middle, and goes back up when it is closed. theHeight is the height it settles at.
class NcMovingDialog : public Dialog
{
public:
	NcMovingDialog(int theHeight, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
		const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode);
	virtual void	Update() override;
	int				mEscapeResult = 0;				// (stand-in)
	bool			mSettled = false;				// (stand-in)
	void			SetEscapeResult(int theResult);		// what the escape key answers (Application::DialogClosed gets it; 999 for the continue box)
	int				mCloseResult = 0;				// +0x158 (stand-in)
	void			CloseWith(int theResult);		// 0x468aa0: the dialog goes back up with this result; Closed runs once it has left
	virtual void	Closed();						// vf60 (0xf0): the dialog has left the screen; the default tells the application (DialogClosed)
};

// 0x44e3a0: the width and lines of a dialog that only informs (the unlocked-Tough-Cookie box)
void LayoutInfoDialog(Dialog* theDialog, int theWidth);

// The dialogs the application opens
void MakeGameMenu();									// 0x44fba0: dialog 0
Dialog* MakeUserDialog();								// 0x462c50: the player list, dialog 4
Dialog* MakeCreateUserDialog(bool theAnother);			// 0x44e750: dialog 5, or 6 for another player
void SetCreateUserText(Dialog* theDialog, const std::string& theText);	// 0x44e2c0
std::string GetUserDialogSelection(Dialog* theDialog);	// 0x462950: the name chosen in the player list
void RefreshUserDialog(Dialog* theDialog, const std::string& theSelectedName);	// 0x4629d0: the list is made again, with this player chosen
void MakeGameOverDialog();								// 0x450aa0: dialog 0xf
void MakeReadyGoDialog(const std::string& theHeader, const std::string& theText);	// 0x45cde0: dialog 0xd

}

#endif
