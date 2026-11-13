#ifndef __HAMMERHEADS_GNOMESAVE_H__
#define __HAMMERHEADS_GNOMESAVE_H__

// The photographer (kind 0x18; 0x43d000-0x43d3d0): hitting him takes a snapshot of the game, which is kept in the player's
// profile for the mode (a saved game to come back to), and shows the photograph. He goes when the hit has been made (a second
// after, in the timer).

#include "Creature.h"

namespace Sexy
{

class GnomeSave : public Creature
{
public:
	double			mCenterX;					// +0xc0: where he stands
	double			mCenterY;					// +0xc8: and his height above the platform taken off
	double			mTimer;						// +0xd0: 0 until Remove sets it to 1; then it runs down by 0.01 and at 0 he is spent

public:
	GnomeSave();								// 0x43d000
	virtual ~GnomeSave();						// 0x43d040

	virtual void	Update(const double thePos[2]) override;			// 0x43d050
	virtual void	UpdateWithoutCubby(const double thePos[2]) override;	// 0x430030
	virtual bool	Hit() override;										// 0x43d260
	virtual void	Remove() override;									// 0x43d0c0
	virtual std::string SaveExtra() override;							// 0x43d100
	virtual void	LoadExtra(const std::string& theData) override;		// 0x43d1b0
};

}

#endif
