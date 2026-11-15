#ifndef __HAMMERHEADS_GNOMESWITCHER_H__
#define __HAMMERHEADS_GNOMESWITCHER_H__

// The switcher (kind 0x1b; 0x43eb20-0x43f5f0): he puts on one of two costumes and takes it off again. In the first, a hit
// while it is almost on sends three coins flying to the counter; in the second a bomb's fuse hisses, and a hit while it is
// on is a bomb going off (it costs health). A hit that is not one of those only takes health off him (he takes more than one).

#include "Creature.h"

namespace Sexy
{

class SoundInstance;

class GnomeSwitcher : public Creature
{
public:
	enum
	{
		STAGE_BARE = 0,				// waits for the timer, then draws which costume at random
		STAGE_COINS_ON = 1,			// rows 0 and 1 (2 and 3 after a hit): the coin costume goes on, ten cels, and stays until the timer is out
		STAGE_COINS_OFF = 2,		// row 1: it comes off again, nine cels
		STAGE_BOMB_ON = 3,			// rows 4 (or 6) and 8: the bomb costume goes on; the fuse hisses
		STAGE_BOMB_OFF = 4			// row 5 (or 7): it comes off again, nine cels
	};

	double			mFrame;						// +0xc0: the cel; goes up by mFrameSpeed
	double			mFrameSpeed;				// +0xc8: 0.18 while a costume goes on or off, 0 while it is on
	int				mStage;						// +0xd0
	double			mTimer;						// +0xd8: runs down by 0.01 a step; at 0 the stage changes (99999 while one goes on or off)
	double			mHurtFlash;					// +0xe0: set to 1 by a hit, runs down by 0.01; while it is above 0 the second set of rows is drawn
	int				mFlicker;					// +0xe8: 0 to 3, drawn at random every step; the last cels of the bomb costume flicker through them (not saved)
	double			mCenterX;					// +0xf0: where Update last found him
	double			mCenterY;					// +0xf8: and his height above the platform taken off
	SoundInstance*	mFuseSound;					// +0x100: the bomb's fuse, played over and over while the bomb costume is on
	double			mFuseTimer;					// +0x108: counts down while the fuse plays; at 0 it plays again (not saved)

public:
	GnomeSwitcher();							// 0x43eb20
	virtual ~GnomeSwitcher();					// 0x43ebf0

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43ef90
	virtual void	Update(const double thePos[2]) override;			// 0x43ec50
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43b2b0
	virtual bool	Hit() override;										// 0x43f210
	virtual std::string SaveExtra() override;							// 0x43f410
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43f510

private:
	void			ThrowCoin(double theDelay);							// 0x43f110
};

}

#endif
