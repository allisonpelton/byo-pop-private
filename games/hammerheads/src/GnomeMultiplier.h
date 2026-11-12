#ifndef __HAMMERHEADS_GNOMEMULTIPLIER_H__
#define __HAMMERHEADS_GNOMEMULTIPLIER_H__

// The mole with the flag (kind 8; 0x43c130-0x43c690): he waves a flag now and then; when he is hit his x5 sign flies to the
// bonus tray and for twenty time units every score counts five times (Gameplay's mFiveTimesTimer). He is not taken away at
// once: when he leaves, his second row of cels plays first.

#include "Creature.h"

namespace Sexy
{

class GnomeMultiplier : public Creature
{
public:
	enum
	{
		STAGE_WAVING = 0,		// row 0: nine cels, round and round
		STAGE_LEAVING = 1		// row 1: five cels, once (a fifth of a step in 0.15)
	};

	double			mFrame;						// +0xc0: the cel; goes up by mFrameSpeed
	double			mFrameSpeed;				// +0xc8: 0.1 while he waves, 0.15 when he leaves
	double			mCenterX;					// +0xd0: where Update last found him
	double			mCenterY;					// +0xd8
	double			mWaveTimer;					// +0xe0: runs down by 0.01; at 0 (and not frozen) he waves and it is set to 1.2828
	bool			mHasRisen;					// +0xe8: his cubby has been up; saved
	int				mStage;						// +0xec

public:
	GnomeMultiplier();							// 0x43c130
	virtual ~GnomeMultiplier();					// 0x43c1a0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43c350
	virtual void	Update(const double thePos[2]) override;			// 0x43c1b0
	virtual bool	Hit() override;										// 0x43c3b0
	virtual void	Remove() override;									// 0x43c310
	virtual std::string SaveExtra() override;							// 0x43c4d0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43c5c0
};

}

#endif
