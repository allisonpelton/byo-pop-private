#ifndef __HAMMERHEADS_GNOMEPOWERUP_H__
#define __HAMMERHEADS_GNOMEPOWERUP_H__

// The power-up bottle (kind 0x1a; 0x43c690-0x43cb10): it glows like the clover; when it is hit its picture flies to the sign
// of the power-ups and for twenty time units the hammer's blows count double (Gameplay's mDoubleHitTimer).

#include "Creature.h"

namespace Sexy
{

class GnomePowerUp : public Creature
{
public:
	double			mDrawX;						// +0xc0: the top left of the picture
	double			mDrawY;						// +0xc8
	double			mPulse;						// +0xd0: how bright the glow is, 0 to 1, a slow wave of its age

public:
	GnomePowerUp();								// 0x43c690
	virtual ~GnomePowerUp();					// 0x43c6d0

	virtual void	Update(const double thePos[2]) override;			// 0x43c6e0 (the same code as the clover's)
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x43c730
	virtual bool	Hit() override;										// 0x43c8b0
	virtual std::string SaveExtra() override;							// 0x43c9b0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43ca60
};

}

#endif
