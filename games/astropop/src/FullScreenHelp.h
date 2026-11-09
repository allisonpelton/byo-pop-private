#ifndef __ASTROPOP_FULLSCREENHELP_H__
#define __ASTROPOP_FULLSCREENHELP_H__

// FullScreenHelp: the picture of how to play that a game opens with (original class of 0xf8 bytes, constructor 0x470a60,
// destructor body 0x46c040, vftable 0x5660c0). Three little scenes play over and over, each in a window of the picture:
// on the left the ship takes a column of bricks and shoots it up again, on the right it takes bricks of a kind from two
// columns, shoots them up the third and a group of four breaks up, and under them the supa weapon charges, its button
// is pressed and a target goes to a column. A tick box turns the help off for the next games and a button closes it
// (the application does that: button 0xfb4, CloseFullScreenHelp). Offsets in the comments are the original's.

#include "Color.h"
#include "FrameAnim.h"
#include "Widget.h"
#include <vector>

namespace Sexy
{

class AstroButton;
class AstroPopApp;
class Checkbox;
class Image;

class FullScreenHelp : public Widget
{
public:
	// a brick of the pictures (0x58 bytes: the position, the speed and the row it goes to, the picture and, for the
	// ones that break up, the picture of that and its frame counter)
	struct Cell
	{
		int			mX;             // +0x00
		int			mY;             // +0x04
		int			mSpeed;         // +0x08 pixels an update
		int			mTargetY;       // +0x0c the row it stops at
		Image*		mImage;         // +0x10
		Image*		mDeathImage;    // +0x14 NULL for a brick that does not break up
		FrameAnim	mAnim;          // +0x18

		Cell() : mX(0), mY(0), mSpeed(0), mTargetY(0), mImage(NULL), mDeathImage(NULL) {}
	};

	std::vector<std::vector<Cell*> >	mLeft;          // +0x58 the scene on the left: two rows of seven
	std::vector<std::vector<Cell*> >	mRight;         // +0x68 the scene on the right: four rows of seven
	std::vector<int>					mState;         // +0x78 where each of the three scenes is
	Color			mLeftBeamColor;     // +0x88
	int				mLeftShipX;         // +0x98
	int				mLeftCount;         // +0xd0 updates a step has been going on
	int				mLeftMouse;         // +0xa0 which mouse picture is drawn (0 and 1 the left button, 2 the right)
	bool			mLeftBlink;         // +0xa4
	Color			mRightBeamColor;    // +0xa8
	int				mRightShipX;        // +0xb8
	int				mRightColumn;       // +0xbc
	int				mRightMouse;        // +0xc0
	bool			mRightBlink;        // +0xc4
	AstroButton*	mContinueButton;    // +0xc8
	AstroPopApp*	mApp;               // +0xcc
	int				mRightCount;        // +0xd4
	int				mSupaCount;         // +0xd8
	int				mSupaCharge;        // +0xdc pixels of the charged strip that show
	bool			mSupaBlink;         // +0xe0
	int				mRightPointerX;     // +0xe4
	int				mPointerY;          // +0xe8
	int				mSupaPointerX;      // +0xec
	int				mSupaPointerY;      // +0xf0
	Checkbox*		mNoMoreCheck;       // +0xf4

public:
	explicit FullScreenHelp(AstroPopApp* theApp);                       // 0x470a60
	virtual ~FullScreenHelp();                                          // 0x471030 (body 0x46c040)

	virtual void	Draw(Graphics* g) override;                         // 0x43c770 slot 16
	virtual void	Update() override;                                  // 0x471060 slot 17
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x435aa0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x435b20 slot 2

private:
	void			FillRight();
	void			ClearRight();
};

}

#endif
