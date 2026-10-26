#ifndef __ATOMICA_HINTBOX_H__
#define __ATOMICA_HINTBOX_H__

// HintBox.cpp: the tutorial boxes of Atomica Deluxe (Atomica.exe 0x42a310-0x42fb30) and the field's help popups. A box is a
// green-edged window on the field that fades in, holds a title and some lines (some words lit in white), arrows that
// point at what it talks about, a Continue button and, for a tutorial box, a check box that keeps the hint from being
// shown again. Hints 0 to 0xf and 0x17 to 0x1b come up by themselves as the first levels go (Grid::CheckHints); 0x11 to
// 0x16 are the popups the board opens when the player clicks Dr. Atomicus, the bomb and so on. While a box is up the
// game is paused and the box takes the mouse.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"

namespace Sexy
{

class AtomicaApp;
class Grid;
class WidgetManager;

class HintBox : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;			// +0x60
	Grid*			mGrid;			// +0x64
	int				mHint;			// +0x68 which of the 28 hints
	int				mBoxX;			// +0x6c the box's place
	int				mBoxY;			// +0x70
	int				mBoxWidth;		// +0x74
	int				mBoxHeight;		// +0x78
	int				mAlpha;			// +0x7c fades in by 8 an update
	ButtonWidget*	mContinue;		// +0x80 id 0
	ButtonWidget*	mCheck;			// +0x84 id 1 the check box
	bool			mNever;			// +0x88 the check box is ticked: do not show this hint again
	bool			mGone;			// +0x89 the box is to go at the next update
	bool			mHasCheck;		// +0x8a a tutorial box has the check box, a popup has not

	HintBox(AtomicaApp* theApp, Grid* theGrid, int theHint);	// 0x42a310
	virtual ~HintBox();											// 0x42a0b0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x42a1e0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x42a270
	virtual void	Update() override;							// 0x42a180
	virtual void	Draw(Graphics* g) override;					// 0x42a810
	virtual void	ButtonPress(int theId) override;			// 0x42a2e0
	virtual void	ButtonDepress(int theId) override;			// 0x42a2c0
};

}

#endif
