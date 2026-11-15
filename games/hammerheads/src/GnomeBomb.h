#ifndef __HAMMERHEADS_GNOMEBOMB_H__
#define __HAMMERHEADS_GNOMEBOMB_H__

// The bomb (kind 3; 0x435b70-0x435f40): a gnome with a lit fuse that costs the player health when it is hit.

#include "Creature.h"

namespace Sexy
{

class GnomeBomb : public Creature
{
public:
	double			mPulse;						// +0xc0: counts up half a step at a time; its whole part is the cel of the lit fuse
	double			mCenterX;					// +0xc8: where it stands, the point its explosion is centred on
	double			mCenterY;					// +0xd0: (that point's height has the creature's height taken off)
	double			mFuse;						// +0xd8: runs down from 1; at 0 the fuse's sound plays again

public:
	GnomeBomb();								// 0x435b70
	virtual ~GnomeBomb();						// 0x435bc0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x435c80
	virtual void	Update(const double thePos[2]) override;			// 0x435bd0
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43b2b0
	virtual bool	Hit() override;										// 0x435cc0
	virtual std::string SaveExtra() override;							// 0x435e90
	virtual void	LoadExtra(const std::string& theData) override;		// 0x435de0
};

}

#endif
