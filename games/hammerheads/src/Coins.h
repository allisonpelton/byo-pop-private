#ifndef __HAMMERHEADS_COINS_H__
#define __HAMMERHEADS_COINS_H__

// The coin counter at the top right (0x443e80-0x444390): a box with the number of coins the player holds and a coin beside it.
// A coin that is picked up flies to the counter first, so the number only goes up when it arrives: each gain keeps the number
// the counter shows back for 0.75 of a time unit.

#include <vector>

#include "Object.h"

namespace Sexy
{

// A gain on its way: until the time is up the counter shows the number it had before
struct CoinFlight
{
	int				mCoins;
	double			mTime;
};

class Coins : public Object
{
public:
	std::vector<CoinFlight>	mFlights;				// +0x80

public:
	Coins();										// 0x444390 (made by MakeCoins)
	virtual ~Coins();								// 0x444010

	OBJECT_TYPE(Coins)

	virtual void	Update(float theFrac) override;	// 0x443a60 (shared with the challenge meter)
	virtual void	Draw(Graphics* g) override;		// 0x4441a0

	void			AddCoins(int theAmount);		// 0x444150: the coins go up now, and the counter catches up when the flight is over
};

// 0x443e80: the counter on screen (null when there is none)
Coins* GetCoins();
// 0x444390: makes the counter and remembers it
Coins* MakeCoins();
// 0x443eb0: the right edge of the counter's box (it moves to the left in the shop, and out of the way of the hearts when the
// capacity is seven and the coins have five digits)
int GetCoinsX();
// 0x444040: where the coin of the counter is, and so where a coin that flies there lands
void GetCoinsPos(double* thePos);

}

#endif
