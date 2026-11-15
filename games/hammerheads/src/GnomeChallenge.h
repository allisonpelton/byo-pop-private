#ifndef __HAMMERHEADS_GNOMECHALLENGE_H__
#define __HAMMERHEADS_GNOMECHALLENGE_H__

// The sign that leads to the challenge screen (kind 0x10; 0x437500-0x437790): a neon sign that flickers through its cels, and
// when it is hit opens the challenge.

#include "Creature.h"

namespace Sexy
{

class GnomeChallenge : public Creature
{
public:
	double			mX;							// +0xc0: where it stands
	double			mY;							// +0xc8

public:
	GnomeChallenge();							// 0x437500
	virtual ~GnomeChallenge();					// 0x437530

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x4375a0
	virtual void	Update(const double thePos[2]) override;			// 0x437540
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43b2b0
	virtual void	Remove() override;								// 0x437620
	virtual bool	Hit() override;										// 0x4375e0
	virtual std::string SaveExtra() override;							// 0x437650
	virtual void	LoadExtra(const std::string& theData) override;		// 0x4376f0
};

}

#endif
