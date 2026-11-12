#ifndef __HAMMERHEADS_GNOMEHIDE_H__
#define __HAMMERHEADS_GNOMEHIDE_H__

// The gnome with the bucket (kind 5; 0x43b200-0x43b800): he puts a bucket on his head and takes it off again. A hit while the
// bucket is on costs the player health; a hit while it is off or on its way counts. The bucket's pictures are rows 1 and 2 of
// the sheet (4 and 5 for the second set, shown after a hit); row 0 (or 3) is the bare gnome.

#include "Creature.h"

namespace Sexy
{

class GnomeHide : public Creature
{
public:
	enum
	{
		STAGE_BARE = 0,			// waits for the timer, then puts the bucket on
		STAGE_PUTTING_ON = 1,	// row 1: the bucket comes down over ten cels and stays there until the timer is out
		STAGE_TAKING_OFF = 2	// row 2: eleven cels, then back to bare
	};

	double			mFrame;						// +0xc0: counts up by mFrameSpeed; its whole part is the cel
	double			mFrameSpeed;				// +0xc8: 0.18 while the bucket moves
	int				mStage;						// +0xd0
	double			mTimer;						// +0xd8: runs down by 0.01 a step; at 0 the stage changes
	double			mHurtFlash;					// +0xe0: set to 1 by a hit that counts, runs down by 0.01; while it is above 0 the second set of rows is drawn

public:
	GnomeHide();								// 0x43b200
	virtual ~GnomeHide();						// 0x43b2a0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43b5d0
	virtual void	Update(const double thePos[2]) override;			// 0x43b2c0
	virtual bool	Hit() override;										// 0x43b4b0
	virtual std::string SaveExtra() override;							// 0x43b6a0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43b780
};

}

#endif
