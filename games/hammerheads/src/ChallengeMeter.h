#ifndef __HAMMERHEADS_CHALLENGEMETER_H__
#define __HAMMERHEADS_CHALLENGEMETER_H__

// The row of cells at the top left of a challenge (0x443860-0x443d60): one empty cell for each gnome of the goal, and a full
// cell for each one the player has dealt with. A count that goes up keeps the cells the row shows back for a time (the same
// way the hearts hold a gain back until the flying heart arrives), so the new cell fills when the gnome's flight ends.

#include <vector>

#include "Object.h"

namespace Sexy
{

// A count that has gone up and is still on its way to the row: until the time is up the row shows the count it had before
struct CountFlight
{
	int				mCount;
	double			mTime;
};

class ChallengeMeter : public Object
{
public:
	std::vector<CountFlight>	mFlights;			// +0x80

public:
	ChallengeMeter();								// 0x443d60 (made by MakeChallengeMeter)
	virtual ~ChallengeMeter();						// 0x443a30

	OBJECT_TYPE(ChallengeMeter)

	virtual void	Update(float theFrac) override;	// 0x443a60 (shared with Coins)
	virtual void	Draw(Graphics* g) override;		// 0x443b00

	// 0x443cc0: a kind of count goes up; only kind 0 (gnomes dealt with in a challenge) is shown, and it stops at the goal
	void			AddCount(int theKind, int theAmount);
};

// 0x4438d0: the meter on screen (null when there is none)
ChallengeMeter* GetChallengeMeter();
// 0x443d60: makes the meter and remembers it
ChallengeMeter* MakeChallengeMeter();
// 0x443900: where the next cell of the row goes (to the right of the full ones)
void GetNextCellPos(double* thePos);

}

#endif
