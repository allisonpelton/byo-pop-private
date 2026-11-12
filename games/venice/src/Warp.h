// The warp: a pair (or more) of portals. A jewel that flies into one comes out of the nearest other one, in a burst of sparks. A
// jewel that has gone through nine of them is lost. The warp shows a trail of arrows to its partner while the shooter is near.

#ifndef VENICE_WARP_H
#define VENICE_WARP_H

#include "Obstacle.h"

#include <vector>

class ImageRecord;
class Jewel;

// 0x160 bytes in the original
class Warp : public Obstacle
{
public:
	ImageRecord* mArrowImage = nullptr; // 0x150

	Warp(); // 0x409dc0
	void OnObjectClosed() override; // 0x402ad0
	bool Update() override; // 0x402be0

private:
	// 0x154: the jewels that came out of this warp and have not left its circle yet (they are not taken in again)
	std::vector<Jewel*> mJewelsInside;

	bool HasJewelInside(Jewel* theJewel) const; // 0x402b30
	void ForgetJewel(Jewel* theJewel); // 0x402b80
	void Transport(Jewel* theJewel); // 0x402ce0: the jewel goes to the nearest other warp
	void Burst(); // 0x402dc0: the ring of sparks that goes out when a jewel comes in or out
	void ShowArrows(); // 0x402f50: the arrows that run to the other warp
};

#endif
