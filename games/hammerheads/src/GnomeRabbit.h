#ifndef __HAMMERHEADS_GNOMERABBIT_H__
#define __HAMMERHEADS_GNOMERABBIT_H__

// The rabbit (kind 0x16; 0x43cb10-0x43d000): a hit gives the player two and a half hearts. Like the flag mole it is not taken
// away at once: when it leaves, its first row of cels plays backwards first.

#include "Creature.h"

namespace Sexy
{

class GnomeRabbit : public Creature
{
public:
	enum
	{
		STAGE_HOPPING = 0,		// row 1: ten cels, round and round
		STAGE_LEAVING = 1		// row 0: five cels, once, the last first
	};

	double			mFrame;						// +0xc0: the cel; goes up by mFrameSpeed
	double			mFrameSpeed;				// +0xc8: 0.075 to 0.125 for each rabbit, 0.15 when it leaves
	double			mCenterX;					// +0xd0: where Update last found it
	double			mCenterY;					// +0xd8
	bool			mHasRisen;					// +0xe0: its cubby has been up; saved
	int				mStage;						// +0xe4

public:
	GnomeRabbit();								// 0x43cb10
	virtual ~GnomeRabbit();						// 0x43cbc0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43cde0
	virtual void	Update(const double thePos[2]) override;			// 0x43cc00
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43cbd0
	virtual bool	Hit() override;										// 0x43cd00
	virtual void	Remove() override;									// 0x43cda0
	virtual std::string SaveExtra() override;							// 0x43ce40
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43cf30
};

}

#endif
