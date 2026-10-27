#ifndef __TUTORIAL_H__
#define __TUTORIAL_H__

// The tutorial (vtable 0x4fd078, ButtonListener 0x4fd068 at +0x58; 0xc8 bytes; 0x439ee0-0x440f30; the app's
// +0x10bc): Inchy's box over a board, with his text, an arrow at what he talks about and a continue button. Its
// steps (0 to 0x6f) set up a board and its grid, and some wait for the player to do something; the links of
// step 3 jump to each mode's part.

#include "ButtonListener.h"
#include "Widget.h"
#include <string>

namespace Sexy
{

class Cell;
class CellButton;
class FossilizedApp;
class Graphics;
class HyperlinkWidget;
class Pic;
class WidgetManager;
struct GridPoint;

class Tutorial : public Widget, public ButtonListener
{
public:
	FossilizedApp*		mApp;				// +0x5c
	int					mStep;				// +0x60 (-1)
	int					mShownStep;			// +0x64 the step on show (-1)
	int					mTimer;				// +0x68 step 0's wait
	Pic*				mInchy;				// +0x6c images\TutorInchy
	Pic*				mContinuePic;		// +0x70 images\continuebuttonsheet
	Cell*				mContinueCell;		// +0x74 its first cell
	Cell*				mContinueOverCell;	// +0x78 and its second
	CellButton*			mContinueButton;	// +0x7c id 100
	std::string			mText;				// +0x80
	GridPoint*			mArrow;				// +0x9c where the arrow points, on the screen (NULL)
	int					mBoxX;				// +0xa0 the box's offsets in the widget
	int					mBoxY;				// +0xa4
	int					mContinueY;			// +0xa8 the continue button's, below the text
	int					mDelay;				// +0xac updates before the next step shows
	bool				mNoContinue;		// +0xb0 the step hides the continue button
	HyperlinkWidget*	mLinks[4];			// +0xb4 ids 1000-1003: Endless, Stomped, Fossil, Time Trial
	bool				mOldLockCursor;		// +0xc4 the app's, put back at the end
	bool				mLinksShown;		// +0xc5

	Tutorial(FossilizedApp* theApp);		// 0x43ac40
	virtual ~Tutorial();					// 0x43aad0

	void				Start();								// 0x43a530
	void				SetupBoard(int theMode);				// 0x439fc0: 1 Stomped, 2 Endless, 3 Fossil, 4 Time Trial
	int					ContinueY(int theWidth);				// 0x439f00
	void				ShowLinks(bool show);					// 0x43a3f0
	void				DrawArrow(Graphics* g, int* theFrom, int* theTo);	// 0x43a580: a filled arrow, its tip at theTo
	void				Step();									// 0x43b2c0: shows mStep

	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x43a2e0
	using Widget::Resize;
	virtual void		Draw(Graphics* g) override;				// 0x43a720
	virtual void		Update() override;						// 0x4408b0
	virtual void		MouseMove(int x, int y) override;		// 0x43a3b0: passed on to the board
	virtual void		MouseDown(int x, int y, int theClickCount) override;	// 0x43a360: likewise
	using Widget::MouseDown;
	virtual void		MarkDirty() override;					// 0x439ee0: the side bar too
	using Widget::MarkDirty;
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x43a240
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x43a280

	virtual void		ButtonPress(int theId) override;		// 0x43a9e0: continue (id 100, or 0 from Update), a link
	using ButtonListener::ButtonPress;
};

}

#endif
