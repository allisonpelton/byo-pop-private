#ifndef __HAMMERHEADS_GNOMEKING_H__
#define __HAMMERHEADS_GNOMEKING_H__

// The Gnome King (kind 0x17; 0x43b840-0x43c130): he rises out of a cubby with a crown that opens over a few seconds. When it
// is open the cubbies around him pop up and a crystal glows; hitting him breaks every other creature on screen and the glow
// goes out. What the cubbies bring up while he is out is chosen by the item he carries (GetItemKind, read by the screen that
// chooses the next creature to send).

#include "Creature.h"

namespace Sexy
{

class GnomeKing : public Creature
{
public:
	enum
	{
		STAGE_WAITING = 0,		// the timer (3) runs down; then the crown opens
		STAGE_OPENING = 1,		// the cels of the sheet's first row play forwards; when the last is reached the cubbies pop up
		STAGE_OPEN = 2,			// the last cel, for the timer (3)
		STAGE_CLOSING = 3		// the cels backwards; then waiting again
	};

	double			mFrame;						// +0xc0: the cel of the crown; goes up and down by 0.07 a step
	double			mFrameSpeed;				// +0xc8: never changed (0), saved all the same
	int				mStage;						// +0xd0
	double			mTimer;						// +0xd8: runs down by 0.01 a step
	double			mCenterX;					// +0xe0: where Update last found him
	double			mCenterY;					// +0xe8
	int				mItemCount;					// +0xf0: 9, the length of the table of items
	int				mItemIndex;					// +0xf4: which of them he carries, drawn at random when the crown has opened (not saved)
	double			mFlash;						// +0xf8: set to 1 by a hit, runs down by 0.01; while it is above 0 the second row of cels is drawn
	bool			mHasRisen;					// +0x100: his cubby has been up; saved

public:
	explicit GnomeKing(bool theForIntro = false);	// 0x43b840: theForIntro is the king an intro page shows (made without the arrival's count, timer and sound)
	virtual ~GnomeKing();						// 0x43b930

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43b970
	virtual void	Update(const double thePos[2]) override;			// 0x43bca0
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x430030
	virtual bool	Hit() override;										// 0x43bf90
	virtual std::string SaveExtra() override;							// 0x43baa0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43bbb0

	// 0x43ba10: the kind of the creature the screen sends next while he is out: the table's entry for mItemIndex, or when
	// that is -1 one of the four bonuses at random
	int				GetItemKind() const;
};

}

#endif
