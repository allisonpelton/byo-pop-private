#ifndef __PUZZLELISTWIDGET_H__
#define __PUZZLELISTWIDGET_H__

// The Stomped Puzzle packs on the main menu (vtable 0x4fc750 and 0x4fc74c for its ScrollListener, 0x194 bytes;
// 0x4311f0-0x431d30): DEFAULT.EGG and STOMPED\*.EGG in two columns of pictures with their names, scrolled by the
// menu's scrollbar. A click picks a pack; a click on the picked one is the OK button.

#include "PtrList.h"
#include "ScrollListener.h"
#include "Widget.h"

namespace Sexy
{

class FossilizedApp;
class Pic;
class ScrollbarWidget;

// a pack (0x208 bytes)
struct PuzzlePack
{
	char				mName[0x100];		// +0
	char				mFile[0x100];		// +0x100
	unsigned int		mSkills;			// +0x200 bits 1, 2, 4: the skills it has puzzles for
	Pic*				mPic;				// +0x204 108 x 106
};

class PuzzleListWidget : public Widget, public ScrollListener
{
public:
	FossilizedApp*		mApp;				// +0x5c
	ScrollbarWidget*	mScrollbar;			// +0x60 (the menu sets it)
	PtrList<PuzzlePack>	mPacks;				// +0x64
	int					mScrollY;			// +0x7c
	int					mOver;				// +0x80 the pack under the mouse, -1 none
	int					mSelected;			// +0x84
	char				mOverFile[0x100];	// +0x88
	unsigned int		mOverSkills;		// +0x188
	bool				mNothingToPlay;		// +0x18c
	bool				mNeedsScrollbar;	// +0x18d
	unsigned int		mPackSkills;		// +0x190

	PuzzleListWidget(FossilizedApp* theApp);	// 0x4318f0
	virtual ~PuzzleListWidget();				// 0x4311f0

	virtual void		Draw(Graphics* g) override;									// 0x431a10
	virtual void		MouseLeave() override;										// 0x431700
	virtual void		MouseMove(int x, int y) override;							// 0x431710
	virtual void		MouseDown(int x, int y, int theClickCount) override;		// 0x431830
	virtual void		ScrollPosition(int theId, double thePosition) override;		// 0x4316e0

	void				LoadPacks();												// 0x4312b0
};

}

#endif
