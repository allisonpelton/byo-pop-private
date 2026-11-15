#ifndef __HAMMERHEADS_GNOMEHEART_H__
#define __HAMMERHEADS_GNOMEHEART_H__

// The heart that comes up out of a cubby (kind 0xc; 0x43ad60-0x43b1d0): hit, it flies to the health row and gives half a
// heart; if it goes back down unhit it counts as a missed heart.

#include "Creature.h"

namespace Sexy
{

class GnomeHeart : public Creature
{
public:
	double			mFrame;						// +0xc0: the beat; its whole part, 11 at a time round, is the cel
	double			mFrameSpeed;				// +0xc8: 0.55 at first, slowing to 0.15
	int				mUnusedInt;					// +0xd0: never set, saved and loaded all the same
	double			mUnusedDouble;				// +0xd8: the same
	double			mHeartX;					// +0xe0: where it stands
	double			mHeartY;					// +0xe8: and its height above the platform taken off

public:
	GnomeHeart();								// 0x43ad60
	virtual ~GnomeHeart();						// 0x43ae00

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43aee0
	virtual void	Update(const double thePos[2]) override;			// 0x43ae70
	virtual bool	Hit() override;										// 0x43af10
	virtual std::string SaveExtra() override;							// 0x43b040
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43b130
};

}

#endif
