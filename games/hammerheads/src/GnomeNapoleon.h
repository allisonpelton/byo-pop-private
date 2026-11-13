#ifndef __HAMMERHEADS_GNOMENAPOLEON_H__
#define __HAMMERHEADS_GNOMENAPOLEON_H__

// The gnome with the big hat (kind 0x19; the exe's class is called GnomeSimple2; 0x43db40-0x43de80). Every hit takes a cel off
// the sheet's row (the hat gets smaller), and the cel that is drawn catches up with the hit ones a step at a time.

#include "Creature.h"

namespace Sexy
{

class GnomeNapoleon : public Creature
{
public:
	int				mHitStage;					// +0xc0: the cel the hits have brought him to (the sheet's columns minus the health, less 1)
	int				mShownStage;				// +0xc4: the cel that is drawn; goes up by one every 8 steps while it is behind mHitStage
	double			mCenterX;					// +0xc8: where Update last found him
	double			mCenterY;					// +0xd0
	bool			mHasRisen;					// +0xd8: his cubby has been up; saved

public:
	GnomeNapoleon();							// 0x43db40
	virtual ~GnomeNapoleon();					// 0x43db80

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43dca0
	virtual void	Update(const double thePos[2]) override;			// 0x43db90
	virtual bool	Hit() override;										// 0x43dc00
	virtual std::string SaveExtra() override;							// 0x43dcf0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43ddc0
};

}

#endif
