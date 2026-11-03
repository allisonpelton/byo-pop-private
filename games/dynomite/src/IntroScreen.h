#ifndef __INTROSCREEN_H__
#define __INTROSCREEN_H__

// The main menu (IntroScreen, vtable 0x4fb970 and 0x4fb95c for its ButtonListener, 0x284 bytes; docs/GAME_NOTES.md
// section 3a): pages of CellButtons for the start (play, load, ...), the four modes, the skill, the saved games and
// the puzzle list. (A third base at +0x5c, vtable 0x4fb958, has one empty slot; the port leaves it out.)

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class Cell;
class CellButton;
class FossilizedApp;
class MenuTalker;
class Pic;
class PuzzleListWidget;
class ScrollbarWidget;

class IntroScreen : public Widget, public ButtonListener
{
public:
	FossilizedApp*		mApp;					// +0x60
	int					mField64;				// +0x64
	int					mStartChoice;			// +0x68 the start page's choice (ids 10-12 give 0-2)
	Pic*				mSlotPics[8];			// +0x70 the saved games' pictures
	Cell*				mSlotCells[8];			// +0x90
	CellButton*			mSlotButtons[8];		// +0xb0 ids 900 + n
	int					mSlotCount;				// +0xd0
	int					mChosenSlot;			// +0xd4 -1 none
	char				mSlotModes[8][0x14];	// +0xd8 the mode's name in each saved game
	char				mSlotInfos[8][0x14];	// +0x178
	int					mSlotNumbers[8];		// +0x218 n of SLOTn.SAV
	int					mField238;				// +0x238
	CellButton*			mStartButtons[3];		// +0x23c ids 10-12
	CellButton*			mSkillButtons[3];		// +0x248 ids 0-2
	CellButton*			mModeButtons[4];		// +0x254 ids 3-6
	CellButton*			mOkButton;				// +0x264 id 7
	CellButton*			mBackButton;			// +0x268 id 9
	ScrollbarWidget*	mScrollbar;				// +0x26c
	PuzzleListWidget*	mPuzzleList;			// +0x270
	int					mField274;				// +0x274
	MenuTalker*			mTalker;				// +0x278
	int					mPage;					// +0x27c 0 start, 1 modes, 2 skill, 3 saved games, 5 puzzles, 6 fossil skill
	bool				mField280;				// +0x280

	IntroScreen(FossilizedApp* theApp);			// 0x424e90
	virtual ~IntroScreen();						// 0x425570

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x423d30
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x423e40
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x423f50
	virtual void		Draw(Graphics* g) override;										// 0x425790
	virtual void		Update() override;												// 0x423d10
	virtual void		ButtonPress(int theId) override;								// 0x424740: a choice
	virtual void		ButtonDepress(int theId) override;								// 0x42af50

	void				FreeSlots();							// 0x424970
	void				ReadSlots();							// 0x424a20
	void				PlaceOkButton();						// 0x424cc0
	void				LoadChosenSlot();						// 0x424d10
};

// 0x52b320: set while the menu comes back from a game (no music change)
extern bool gIntroReturning;

}

#endif
