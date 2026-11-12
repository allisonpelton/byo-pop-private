#ifndef __HAMMERHEADS_GNOMESILVER_H__
#define __HAMMERHEADS_GNOMESILVER_H__

// The silver gnome (kind 7; 0x43d630-0x43db40): six hits break it, a piece of its armor at a time (one cel of the sheet for each
// hit taken); each hit moves the spot where the hammer lands, since the armor that is left is somewhere else. When the last
// piece is gone it fades out.

#include "Creature.h"

namespace Sexy
{

class GnomeSilver : public Creature
{
public:
	int				mStage;						// +0xc0: the hits taken (6 minus the health), the cel of the sheet; 6 is the last, fading out
	double			mCenterX;					// +0xc8: where Update last found it
	double			mCenterY;					// +0xd0
	bool			mHasRisen;					// +0xd8: its cubby has been up; saved
	double			mFade;						// +0xe0: 1 until the last stage; then it runs down by 0.1 a step to 0 (and the gnome is finished)

public:
	GnomeSilver();								// 0x43d630
	virtual ~GnomeSilver();						// 0x43d680

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43d7f0
	virtual void	Update(const double thePos[2]) override;			// 0x43d840
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x43d8e0
	virtual bool	Hit() override;										// 0x43d690
	virtual void	Remove() override;									// 0x43d7b0
	virtual std::string SaveExtra() override;							// 0x43d9d0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43da90

	// 0x43d7e0: the last stage, the fade; a gnome in it never flashes (read by Creature::FlashAlpha)
	bool			IsFading() const;
};

}

#endif
