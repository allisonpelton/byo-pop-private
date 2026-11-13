#ifndef __HAMMERHEADS_HEARTS_H__
#define __HAMMERHEADS_HEARTS_H__

// The player's health (0x449990-0x44a680): a row of hearts at the top of the screen, filled by the health in the stats and
// drawn in the middle of the window. A heart the player picks up flies to the row first; the row only fills when it gets
// there, so each such gain keeps the health the row shows back for 0.75 of a time unit.

#include <vector>

#include "Object.h"

namespace Sexy
{

class Image;

enum HealthChange
{
	HEALTH_MISS = 0,		// a swing that hit nothing costs a third of a heart
	HEALTH_ESCAPE = 1,		// a gnome got away: half its escape penalty, and the combo drops
	HEALTH_GAIN = 2,		// a heart was picked up: half the amount given is added
	HEALTH_NEW_HEART = 3	// the capacity goes up by a heart
};

// A health gain on its way: until the time is up the row shows the health it had before
struct HealthFlight
{
	double			mHealth;
	double			mTime;
};

class Hearts : public Object
{
public:
	std::vector<HealthFlight>	mFlights;			// +0x80
	double			mCapacityFlash;					// +0x90: after a heart is added to the capacity, the new one is held back while this runs down
	double			mShownHealth;					// +0x98: the health as the row shows it, which follows the real one by a little a step (-1: none yet)

public:
	Hearts();										// 0x449b50 (made by MakeHearts)
	virtual ~Hearts();								// 0x449ba0

	OBJECT_TYPE(Hearts)

	virtual void	Update(float theFrac) override;	// 0x44a000
	virtual void	Draw(Graphics* g) override;		// 0x449c40

	void			ChangeHealth(int theChange, double theAmount);	// 0x44a270
	// 0x449db0: where the end of the health in the row is, in the middle of its hearts' height (a heart that is lost breaks there)
	void			GetHealthEndPos(double* thePos);
	void			GetNewHeartPos(double* thePos);	// 0x449e60: where the next heart to be added goes
};

// 0x449990: the hearts on screen (null when the level has none)
Hearts* GetHearts();
// 0x44a190: makes the hearts and remembers them
Hearts* MakeHearts();

void SetHealth(double theHealth);					// 0x4499c0
double GetPlayerHealth();							// 0x4499e0
double GetHealthCapacity();							// 0x4499f0
// 0x449bd0: where the row of hearts starts across (centred in the window; 100 in the shop)
int GetHeartsX();

// 0x4497e0: a row of theCapacity hearts from the point, theHealth of them full (the resurrect page draws the same row, small)
void DrawHeartRow(Graphics* g, int theCapacity, double theHealth, int theX, int theY, int theTick, Image* theHearts, int theOverlap);

}

#endif
