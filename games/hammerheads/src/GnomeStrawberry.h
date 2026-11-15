#ifndef __HAMMERHEADS_GNOMESTRAWBERRY_H__
#define __HAMMERHEADS_GNOMESTRAWBERRY_H__

// The strawberry (kind 0x15; 0x43de80-0x43e2d0): it glows like the clover; when it is hit its picture flies to the sign of the
// power-ups and for fifteen time units a click that misses costs nothing (Gameplay's mStrawberryTimer).

#include "Creature.h"

namespace Sexy
{

class GnomeStrawberry : public Creature
{
public:
	double			mDrawX;						// +0xc0: the top left of the picture
	double			mDrawY;						// +0xc8
	double			mPulse;						// +0xd0: how bright the glow is, 0 to 1, a slow wave of its age

public:
	GnomeStrawberry();							// 0x43de80
	virtual ~GnomeStrawberry();					// 0x43dec0

	virtual void	Update(const double thePos[2]) override;			// 0x43c6e0 (the same code as the clover's)
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x43ded0
	virtual bool	Hit() override;										// 0x43e0a0
	virtual std::string SaveExtra() override;							// 0x43e170
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43e220
};

// 0x43e030: the strawberry's power starts: its timer is set to theTimer, and the delay before it starts to run is theDelay
void StartStrawberryPower(double theDelay, double theTimer);

}

#endif
