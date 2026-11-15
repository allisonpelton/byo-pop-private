#ifndef __HAMMERHEADS_GNOMESHOP_H__
#define __HAMMERHEADS_GNOMESHOP_H__

// The shop sign (kind 0xf; 0x43d3d0-0x43d630): a neon sign that buzzes now and then; hitting it opens the shop.

#include "Creature.h"

namespace Sexy
{

class GnomeShop : public Creature
{
public:
	double			mCenterX;					// +0xc0: where Update last found it
	double			mCenterY;					// +0xc8

public:
	GnomeShop();								// 0x43d3d0
	virtual ~GnomeShop();						// 0x43d400

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43d460
	virtual void	Update(const double thePos[2]) override;			// 0x43d410
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43b2b0
	virtual void	Remove() override;								// 0x437620
	virtual bool	Hit() override;										// 0x43d490
	virtual std::string SaveExtra() override;							// 0x43d4f0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43d590
};

}

#endif
