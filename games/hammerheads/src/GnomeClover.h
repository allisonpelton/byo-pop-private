#ifndef __HAMMERHEADS_GNOMECLOVER_H__
#define __HAMMERHEADS_GNOMECLOVER_H__

// The lucky clover (kind 0x12; 0x437d80-0x438190): glows while it stands there; when it is hit its picture flies to the sign of
// the power-ups and for a while the creatures that carry nothing may drop coins and hearts.

#include "Creature.h"

namespace Sexy
{

class GnomeClover : public Creature
{
public:
	double			mDrawX;						// +0xc0: the top left of the picture
	double			mDrawY;						// +0xc8
	double			mPulse;						// +0xd0: how bright the glow is, 0 to 1, a slow wave of its age

public:
	GnomeClover();								// 0x437d80
	virtual ~GnomeClover();						// 0x437dc0

	virtual void	Update(const double thePos[2]) override;			// 0x43c6e0 (shared with the power-up)
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x437dd0
	virtual bool	Hit() override;										// 0x4380f0
	virtual std::string SaveExtra() override;							// 0x437f90
	virtual void	LoadExtra(const std::string& theData) override;		// 0x438040
};

// 0x437f30: the clover's power starts: its timer is set to 25, and the delay before it starts to run is the argument (the
// clover gives 0.75)
void StartCloverPower(double theDelay);

}

#endif
