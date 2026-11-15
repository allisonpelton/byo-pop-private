#ifndef __HAMMERHEADS_GNOMECOIN_H__
#define __HAMMERHEADS_GNOMECOIN_H__

// The coins that pop up from the cubbies (kinds 0xd the single coin and 0xe the three coins; 0x4382c0-0x438e20). A coin spins
// until it is hit, then flies to the coin counter (or, in a challenge, a triple coin to the next cell of the challenge's row).
// The first coin of the first level carries the hint that says to pick the coins up.

#include <string>

#include "Creature.h"

namespace Sexy
{

class GnomeCoin : public Creature
{
public:
	double			mFrame;						// +0xc0: the spin; its whole part, 20 at a time round, is the cel
	double			mFrameSpeed;				// +0xc8: 0.55 at first, slowing to 0.15
	int				mUnusedInt;					// +0xd0: never set, saved and loaded all the same
	double			mUnusedDouble;				// +0xd8: the same
	double			mDrawX;						// +0xe0: the top left of the picture
	double			mDrawY;						// +0xe8
	int				mCoinKind;					// +0xf0: 0xd or 0xe
	bool			mHintShown;					// +0xf4: this is the coin that has put the hint up
	std::string		mHintName;					// +0xf8: the name of the hint's floater

public:
	explicit GnomeCoin(int theKind);			// 0x438a30
	virtual ~GnomeCoin();						// 0x438d30

	virtual void	Update(const double thePos[2]) override;			// 0x438e20
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x4382c0
	virtual bool	PixelHit(const double thePos[2], int theX, int theY) override;	// 0x4384f0
	virtual bool	MaskHit(const double thePos[2], int theX, int theY, const std::vector<Point>& theMask, int theRectX, int theRectY, int theRectW, int theRectH) override;	// 0x4388f0
	virtual bool	Hit() override;										// 0x4385d0
	virtual std::string SaveExtra() override;							// 0x438b30
	virtual void	LoadExtra(const std::string& theData) override;		// 0x438c40
	virtual void	SetFrozen(bool theFrozen) override;					// 0x438580
};

}

#endif
