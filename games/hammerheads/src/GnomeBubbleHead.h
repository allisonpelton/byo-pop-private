#ifndef __HAMMERHEADS_GNOMEBUBBLEHEAD_H__
#define __HAMMERHEADS_GNOMEBUBBLEHEAD_H__

// The bubble head (kind 4; 0x436f10-0x437500): a gnome whose head swells with every hit until it pops. Each hit adds the
// hammer's damage to its stage; at stage 5 it is gone. The head swells to the size of the stage and later shrinks back by one.

#include "Creature.h"

namespace Sexy
{

class GnomeBubbleHead : public Creature
{
public:
	enum
	{
		MODE_STILL = 0,
		MODE_INFLATING = 1,
		MODE_DEFLATING = 2
	};

	double			mSize;						// +0xc0: the cel of the head (0 to 20)
	double			mTargetSize;				// +0xc8: the size the head swells or shrinks to (5 for each stage)
	double			mMode;						// +0xd0: MODE_STILL, MODE_INFLATING or MODE_DEFLATING (kept as a number, as the original did)
	double			mWait;						// +0xd8: runs down from a half when the head has finished changing; at 0 it may shrink a stage
	int				mStage;						// +0xe0: the hits the head has taken, 0 to 5

public:
	GnomeBubbleHead();							// 0x436f10
	virtual ~GnomeBubbleHead();					// 0x436f60

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x437250
	virtual void	Update(const double thePos[2]) override;			// 0x4370e0
	virtual bool	Hit() override;										// 0x437280
	virtual int		GetHealth() override;								// 0x437330
	virtual std::string SaveExtra() override;							// 0x437360
	virtual void	LoadExtra(const std::string& theData) override;		// 0x437440
};

}

#endif
