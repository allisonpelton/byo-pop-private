#ifndef __HAMMERHEADS_GNOMESTRONG_H__
#define __HAMMERHEADS_GNOMESTRONG_H__

// The strong gnome (kind 2; 0x43e2d0-0x43eb20): flexes, and when it has a moment steals coins from the counter, one at a time,
// which it carries off with it. It takes more than one hit.

#include "Creature.h"

namespace Sexy
{

class GnomeStrong : public Creature
{
public:
	enum
	{
		STAGE_IDLE = 0,			// waits (a few turns of the frame count), then starts to steal if the player has coins and it is up
		STAGE_FLEX = 1,			// the second row of cels: the coins leave the counter for it
		STAGE_REST = 2,			// the third row
		STAGE_FLEX_AGAIN = 3,	// the fourth row (it never gets here by itself; the save may bring it back)
		STAGE_HIT = 4			// hit but still standing
	};

	double			mFrame;						// +0xc0: counts up by mFrameSpeed a step; its whole part is the cel of the row
	double			mFrameSpeed;				// +0xc8: 0.18
	int				mStage;						// +0xd0
	int				mRepeats;					// +0xd4: how many times the stage's row goes round before the next stage
	double			mCenterX;					// +0xd8: where it stands (the coins fly to this point)
	double			mCenterY;					// +0xe0
	int				mCoinsToTake;				// +0xe8: coins it still takes from the counter
	int				mCoinTimer;					// +0xec: steps until the next coin is taken

public:
	GnomeStrong();								// 0x43e410
	virtual ~GnomeStrong();						// 0x43e2d0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43e330
	virtual void	Update(const double thePos[2]) override;			// 0x43e4d0
	virtual bool	Hit() override;										// 0x43e8a0
	virtual std::string SaveExtra() override;							// 0x43e920
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43ea30
};

}

#endif
