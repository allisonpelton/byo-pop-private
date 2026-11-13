#ifndef __HAMMERHEADS_GNOMECUPIDO_H__
#define __HAMMERHEADS_GNOMECUPIDO_H__

// Cupid (kind 9; 0x438fe0-0x4393d0): when he is hit every ordinary gnome that is up turns into a heart. He is never destroyed
// at once: he flies off first (the stage after the hit).

#include "Creature.h"

namespace Sexy
{

class GnomeCupido : public Creature
{
public:
	double			mFrame;						// +0xc0: the flap of his wings; its whole part, eleven at a time round, is the cel
	double			mFrameSpeed;				// +0xc8: 0.1
	double			mCupidX;					// +0xd0: where he stands
	double			mCupidY;					// +0xd8
	int				mStage;						// +0xe0: 0 waiting, 1 flying off (the second row of cels), 2 gone
	double			mTimer;						// +0xe8: counts down from 1 while he flies off
	bool			mHasCalled;					// +0xf0: he has said his "love" once

public:
	GnomeCupido();								// 0x438fe0
	virtual ~GnomeCupido();						// 0x439040

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x439180
	virtual void	Update(const double thePos[2]) override;			// 0x439070
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x439050
	virtual bool	Hit() override;										// 0x4393d0
	virtual void	Remove() override;									// 0x439140
	virtual std::string SaveExtra() override;							// 0x4391f0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x4392f0
};

}

#endif
