#ifndef __HAMMERHEADS_GNOMEWOMAN_H__
#define __HAMMERHEADS_GNOMEWOMAN_H__

// The woman gnome (kind 1; 0x43f5f0-0x43fb10): she takes two hits. After the first she is angry: she shakes (the second row
// of cels, the frame running between 5 and 10) and smoke comes out of her ears.

#include "Creature.h"

namespace Sexy
{

class GnomeWoman : public Creature
{
public:
	enum
	{
		STAGE_CALM = 0,
		STAGE_ANGRY = 1
	};

	double			mFrame;						// +0xc0: the cel of the angry row; swings between 5 and 10
	double			mFrameSpeed;				// +0xc8: 0.18 when she gets angry; changes sign at each end of the swing
	int				mStage;						// +0xd0
	double			mSmokeTime;					// +0xd8: goes up by 0.05 a step while she is angry; the smoke's cel is its whole part, six at a time round
	bool			mHasRisen;					// +0xe0: her cubby has been up; saved

public:
	GnomeWoman();								// 0x43f5f0
	virtual ~GnomeWoman();						// 0x43f640

	virtual void	GetSheetCell(double theCell[2]) const override;		// 0x43f8d0
	virtual void	Update(const double thePos[2]) override;			// 0x43f650
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x43f720
	virtual void	Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime) override;	// 0x43f7d0
	virtual bool	Hit() override;										// 0x43f860
	virtual std::string SaveExtra() override;							// 0x43f940
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43fa10
};

}

#endif
