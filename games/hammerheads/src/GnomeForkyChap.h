#ifndef __HAMMERHEADS_GNOMEFORKYCHAP_H__
#define __HAMMERHEADS_GNOMEFORKYCHAP_H__

// The gnome with the fork (kind 6; 0x43a450-0x43b190): a loop of poses, one row of the sheet each. A hit costs the player
// health while he is not in the way of the hammer (idle or on his way back), and only counts while he is up on his toes with
// the fork out.

#include "Creature.h"

namespace Sexy
{

class GnomeForkyChap : public Creature
{
public:
	enum
	{
		STAGE_IDLE = 0,			// row 0, four cels, a few turns (the count in mRepeats); then he raises the fork
		STAGE_RAISE = 1,		// row 1, three cels (a hit in its second half counts, a hit before that sends him back down)
		STAGE_STAB = 2,			// row 2, seven cels, six to twelve turns; a hit counts
		STAGE_RECOVER = 3,		// row 3, five cels: after a hit, then back to idle
		STAGE_LOWER = 4,		// row 4, four cels: down again, then idle
		STAGE_HURT = 5			// row 5, four cels: the hit he has taken
	};

	double			mFrame;						// +0xc0: counts up by mFrameSpeed; its whole part is the cel of the row
	double			mFrameSpeed;				// +0xc8: 0.07 to 0.13, drawn at random for each gnome
	int				mStage;						// +0xd0
	int				mRepeats;					// +0xd4: turns of the stage's loop left
	double			mCenterX;					// +0xd8: where Update last found him
	double			mCenterY;					// +0xe0

public:
	GnomeForkyChap();							// 0x43aa90
	virtual ~GnomeForkyChap();					// 0x43a450

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43aa50
	virtual void	Update(const double thePos[2]) override;			// 0x43a4b0
	virtual bool	Hit() override;										// 0x43a760
	virtual std::string SaveExtra() override;							// 0x43aba0
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43ac90

private:
	void			BackToIdle();
};

}

#endif
