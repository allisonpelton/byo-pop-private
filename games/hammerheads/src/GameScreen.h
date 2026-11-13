#ifndef __HAMMERHEADS_GAMESCREEN_H__
#define __HAMMERHEADS_GAMESCREEN_H__

// The screen a game is played on (0x450b50-0x453290): the board of the level (the cubbies and the stickers of a board file),
// the level's pace (how many gnomes may be up, how long they stay, how fast a cubby sends one up) and the order in which the
// kinds of gnome come. The original's GameScreen is an editor widget (ToolWidget) with the game on top; the port leaves the
// editor out and keeps the widget.

#include <string>
#include <vector>

#include "BoardFile.h"
#include "ButtonListener.h"
#include "Object.h"
#include "Widget.h"

namespace Sexy
{

class NcButton;

class GameScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_SHUFFLED_BOARDS = 11		// the order of the boards of a mode, shuffled (0x4527c0)
	};

	bool			mOver;				// +0x1bc: the game is over (the health ran out, or the last level was won)
	ObjectRef		mAvatar;			// +0x1c4: the hammer
	std::vector<int> mKindList;			// +0x1ec: the kinds of gnome that come, in order (0x452b10 makes it, 0x452ec0 shuffles it)
	int				mKindIndex;			// +0x1fc: where in the list the next one is
	int				mLevel;				// +0x200: the level being played, from 1
	int				mBonusRoundsDone;	// +0x204: the bonus rounds taken so far in this game (0x450b70 adds one when a round ends); the first one is explained
	int				mShuffledBoards[NUM_SHUFFLED_BOARDS];	// +0x208: 0..10 in a random order (0x4527c0)
	int				mMaxBoard;			// +0x234: the highest board number of the mode's files (0x4521a0)
	int				mCycle;				// +0x238: how many times the list of kinds has been made for this level
	int				mMode;				// +0x23c: the game mode
	double			mLevelTime;			// +0x240: seconds the level has run
	NcButton*		mMenuButton;		// +0x248
	std::vector<BoardObject> mBoardObjects;	// the objects of the board file that LoadBoard read, until Start makes them

public:
	GameScreen();										// 0x4527c0
	virtual ~GameScreen();								// 0x451a30

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x450b80
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x450bf0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x450c50: moves the menu button along
	virtual void	Update() override;										// 0x451070
	virtual void	UpdateF(float theFrac) override;						// 0x4510d0
	virtual void	Draw(Graphics* g) override;								// 0x451d90
	virtual void	GotFocus() override;									// 0x450f80
	virtual void	LostFocus() override;									// 0x450fc0
	virtual void	MouseDrag(int x, int y) override;						// 0x450d10: the hammer follows the pointer
	virtual void	MouseMove(int x, int y) override;						// 0x451200
	virtual void	MouseDown(int x, int y, int theClickCount) override;	// 0x451320
	virtual void	KeyDown(KeyCode theKey) override;						// 0x451410
	virtual void	ButtonPress(int theId) override;						// 0x450e10 (the menu button)

	// 0x4525c0: starts a game in a mode: loads the board, makes the objects of the level
	void			Start(int theMode);
	void			LoadBoard();											// 0x452140: the board of the level (and 0x451ce0, its file name)
	std::string		GetBoardFileName() const;								// 0x451ce0
	void			FindMaxBoard();											// 0x4521a0: the highest number among the files in boards/
	void			NextLevel();											// 0x4519b0
	void			SetLevel(int theLevel, int theCycle);					// 0x450b50
	void			AddBonusRoundDone();									// 0x450b70

	bool			IsGameInProgress() const;								// 0x450bb0: some health, and not over
	bool			IsInPlay() const;										// 0x450ca0: this is the focused widget, and no menu, no confirmation and no intro is up
	bool			IsPaused() const;										// 0x451040: the app is paused and nothing else is showing
	bool			IsLevelRunning() const;									// 0x450d80: the level's time runs (not in the free seconds of a wave or when the last king stands)
	void			Pause();												// 0x450d60
	void			UpdateHover();											// 0x450f30: the crosshair off
	void			ShowCrosshair();										// 0x450ec0: the crosshair on

	// The pace of the level (the sheet's columns, 0x451630-0x451c20)
	int				GetMaxCreatures() const;								// 0x450e50
	double			GetUpTime(int theKind) const;							// 0x451630: seconds a gnome of the kind stays up
	double			GetPopDelay() const;									// 0x451ae0: the wait between one gnome coming up and the next
	double			GetRiseSpeed() const;									// 0x451b80
	double			GetPitchValue() const;									// 0x451c20
	int				PickCreatureKind();										// 0x452fc0: the kind that comes next
	void			MakeKindList();											// 0x452b10: the kinds of the level, each as often as the sheet says
	void			ShuffleKindList();										// 0x452ec0
	void			AddKindToList(int theKind, int theCount);				// 0x452a80
};

}

#endif
