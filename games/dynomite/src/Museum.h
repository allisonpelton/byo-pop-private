#ifndef __MUSEUM_H__
#define __MUSEUM_H__

// The fossil challenge's museum (vtable 0x4fc2bc, a ButtonListener; 0x38 bytes; 0x42c030-0x42e5a0): a long gallery
// over the board with the fifteen fossils, scrolled with two arrow buttons. M (or the side bar's button) opens it
// at any time; at a round's end it opens by itself, the new fragment fades in over its fossil's dark shape and the
// close button continues to the next round. After the last fragment its button says "beware" and the tyrannosaurus
// breaks through the wall (RexAttack).

#include "ButtonListener.h"

namespace Sexy
{

class ButtonWidget;
class Cell;
class CellButton;
class DynomiteFossil;
class FossilizedApp;
class GameSound;
class Museum;
class Pic;

// The museum's +0x34 (0x78 bytes, no vtable; 0x42c070-0x42d9a0): four footsteps, a roar, the cracks, and the
// tyrannosaurus coming out of the wall; when he is out the score gets 10,000 and the button ends the game
class RexAttack
{
public:
	Museum*				mMuseum;			// +0
	int					mDelay;				// +4 frames before the first step (100)
	GameSound*			mSndFootstep;		// +8 sounds\footstep
	GameSound*			mSndRoar;			// +0xc sounds\roar
	Pic*				mPicCrack;			// +0x10 images\fossils\rexcrack
	Pic*				mPicHole;			// +0x14 images\fossils\rexhole
	Pic*				mPicRex;			// +0x18 images\fossils\tyrannosaurusattack
	Cell*				mCells[19];			// +0x1c his frames: 0-9, then 3, 2, 1 and 10-13, 12, 11
	int					mStepTimer;			// +0x68 frames to the next step
	int					mSteps;				// +0x6c steps still to come (4)
	float				mFrame;				// +0x70
	bool				mDone;				// +0x74 he is out and the points are in

	RexAttack(Museum* theMuseum);			// 0x42c070
	~RexAttack();							// 0x42c420

	void				Update();			// 0x42d3f0
	void				Draw();				// 0x42d620
};

class Museum : public ButtonListener
{
public:
	FossilizedApp*		mApp;				// +4
	DynomiteFossil*		mBoard;				// +8
	int					mScroll;			// +0xc the gallery's left, 0 to 4924
	ButtonWidget*		mCloseButton;		// +0x10 (1) close, continue, beware, end the game
	CellButton*			mLeftButton;		// +0x14 (2) held down it scrolls
	CellButton*			mRightButton;		// +0x18 (2)
	bool				mRoundEnd;			// +0x1c opened at a round's end: the new fragment fades in
	int					mFragmentAlpha;		// +0x20
	int					mGlowAlpha;			// +0x24
	int					mWait;				// +0x28 frames before the fragment fades in (64)
	int					mY;					// +0x2c comes up from 480
	bool				mKeepMusic;			// +0x30 the destructor leaves the music (the board is going)
	RexAttack*			mRex;				// +0x34

	Museum(FossilizedApp* theApp, DynomiteFossil* theBoard);	// 0x42cae0
	virtual ~Museum();											// 0x42c520

	virtual void		ButtonPress(int theId) override;		// 0x42c030: the close button clicks
	// 0x42ce70: the close button; "beware" brings the tyrannosaurus
	virtual void		ButtonDepress(int theId) override;

	void				SetScroll(int theScroll);				// 0x42c050
	// 0x42d000: at a round's end: the scroll to the fossil, and the close button's label
	void				Start();
	void				Update();								// 0x42e460
	void				Draw();									// 0x42e300
	void				DrawFossil(int theFossil);				// 0x42c730: theFossil 1-20
};

}

#endif
