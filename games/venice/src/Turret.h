// The turret: a cannon in the plate layer that catches a jewel that goes up into it, and shoots it on out of its barrel at the
// speed it came in with. The turret's first child is the barrel and the barrel's first child is the muzzle.

#ifndef VENICE_TURRET_H
#define VENICE_TURRET_H

#include "SceneObject.h"

class Jewel;

// 0x14c bytes in the original
class Turret : public SceneObject
{
public:
	int mCooldown = 0; // 0x148: updates until the turret takes another jewel (it counts down two for each update)

	Turret(); // 0x409d70
	bool Update() override; // 0x402420

private:
	void Catch(Jewel* theJewel); // 0x402570: the jewel leaves the muzzle in the direction of the barrel, with a burst of sparks
};

#endif
