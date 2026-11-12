#ifndef __SIDEBAR_H__
#define __SIDEBAR_H__

// The side bar (vtable 0x4fcb98 and 0x4fcb84 for its ButtonListener, 0x8c bytes; 0x434dc0-0x435b00): the left
// 150 pixels beside the menu and the board, with six picture buttons, the score, the mode and skill pictures, and
// the small Inchy. (A third base at +0x5c, vtable 0x4fcb80, has one empty slot; the port leaves it out.)

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class CellButton;
class FossilizedApp;
class InchySmall;

class SideBar : public Widget, public ButtonListener
{
public:
	FossilizedApp*		mApp;				// +0x60
	int					mScoreShown;		// +0x64
	int					mScore;				// +0x68
	InchySmall*			mInchy;				// +0x6c
	CellButton*			mMenuButton;		// +0x70 id 0
	CellButton*			mOptionsButton;		// +0x74 id 1
	CellButton*			mExitButton;		// +0x78 id 2
	CellButton*			mPauseButton;		// +0x7c id 3
	CellButton*			mSaveButton;		// +0x80 id 4
	CellButton*			mMuseumButton;		// +0x84 id 5, the museum (only in the game type 2)
	bool				mShowTime;			// +0x88 the score is a time (minutes:seconds)

	SideBar(FossilizedApp* theApp);			// 0x435420
	virtual ~SideBar();						// 0x434dc0

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x435020
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4350e0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x435140
	virtual void		Draw(Graphics* g) override;										// 0x4357d0
	virtual void		Update() override;												// 0x434ec0
	virtual void		ButtonPress(int theId) override;								// 0x435210
	virtual void		ButtonDepress(int theId) override;								// 0x435920

	void				DrawScore();						// 0x434f00
	void				DisableGameButtons();				// 0x435230
	void				SetPauseState(int theMode);			// 0x435270: 0 toggle, 1 pause, 2 resume
	void				GameStarted();						// 0x435890
};

}

#endif
