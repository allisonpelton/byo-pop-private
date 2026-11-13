#ifndef __HAMMERHEADS_GNOMECHEST_H__
#define __HAMMERHEADS_GNOMECHEST_H__

// The treasure chest (kind 0x11; 0x437790-0x437d80): when it is hit it opens and throws coins for two seconds' worth of time,
// one every fifth of it, then closes and goes.

#include "Creature.h"

namespace Sexy
{

class GnomeChest : public Creature
{
public:
	enum
	{
		STAGE_CLOSED = 0,
		STAGE_THROWING = 1,		// open, coins flying
		STAGE_CLOSING = 2		// the last half second
	};

	double			mX;							// +0xc0: where it stands
	double			mY;							// +0xc8
	double			mOpenX;						// +0xd0: where it stood when it was opened (the coins start there)
	double			mOpenY;						// +0xd8
	int				mStage;						// +0xe0
	double			mTimer;						// +0xe8: counts down from 2 while it is open

public:
	GnomeChest();								// 0x437790
	virtual ~GnomeChest();						// 0x4377f0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x4378d0
	virtual void	Update(const double thePos[2]) override;			// 0x437a70
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x437820
	virtual void	Remove() override;									// 0x437b60: the chest opens
	virtual std::string SaveExtra() override;							// 0x437bc0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x437cb0

	void			ThrowCoin();										// 0x437950: a coin flies to the counter
};

}

#endif
