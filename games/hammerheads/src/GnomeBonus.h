#ifndef __HAMMERHEADS_GNOMEBONUS_H__
#define __HAMMERHEADS_GNOMEBONUS_H__

// The bonus creatures (kinds 0x13 the ice cream and 0x14 the duck; 0x435f40-0x4361d0): the ice cream freezes everything for a
// while and flies to the sign of the power-ups, the duck only quacks.

#include "Creature.h"

namespace Sexy
{

class GnomeBonus : public Creature
{
public:
	double			mDrawX;						// +0xc0: the top left of the picture, where the ice cream starts when it flies off
	double			mDrawY;						// +0xc8

public:
	explicit GnomeBonus(int theKind);			// 0x435f40
	virtual ~GnomeBonus();						// 0x435f80

	virtual void	Update(const double thePos[2]) override;			// 0x435f90
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x435f90
	virtual bool	Hit() override;										// 0x436040
	virtual std::string SaveExtra() override;							// 0x436130
	virtual void	LoadExtra(const std::string& theData) override;		// 0x4361d0
};

// 0x435fd0: everything freezes, and the freeze timer is set to the time given (the ice cream gives 5)
void StartIceCreamPower(double theSeconds);

}

#endif
