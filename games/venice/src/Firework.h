// The firework: a spark that rises from below, bursts into a ring of sparks of its colour when its rise ends, and starts again
// from a new place. The victory screens have them.

#ifndef VENICE_FIREWORK_H
#define VENICE_FIREWORK_H

#include "SceneObject.h"

// 0x148 bytes in the original
class Firework : public SceneObject
{
public:
	Firework(); // 0x404580

	// 0x4045e0: the spark's stages (a wait, and a rise that ends where the burst is) and its colour. The original also calls it
	// from the constructor, so a firework draws its random values twice before it first rises.
	void OnFirstUpdate() override;

	// 0x4048b0: the rise ended, the ring of sparks flies out, and the firework starts over
	void OnCustomEvent() override;
};

#endif
