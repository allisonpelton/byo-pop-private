// The jewel: the coloured stone the shooter throws at the slots. It waits in the shooter's row, is promoted to the one ready to
// throw, and flies on a curve (up and then down again) until it fills a slot, falls into the water or is picked up again by the
// shooter. A slot throws the jewel it holds back out as one that falls.

#ifndef VENICE_JEWEL_H
#define VENICE_JEWEL_H

#include "SceneObject.h"

class Slot;

// The kinds that are a colour are 1 to 99; 100 and up are the wild jewels
const int WildJewelKind = 999;
const int FirstWildJewelKind = 100;

// 0x194 bytes in the original
class Jewel : public SceneObject
{
public:
	int mKind = 0; // 0x148: the colour (picture jewel<kind>.png), 999 for a wild one
	int mPowerShot = 0; // 0x14c: the jewel passes through the bumpers
	int mComboShot = 0; // 0x150: filling a slot sets off the cascade
	int mShot = 1; // 0x154: 1 for a jewel the shooter threw, 0 for one a slot threw back
	int mBumpable = 1; // 0x158: the bumpers bounce the jewel (0 for one that a filled slot threw, but 1 for the tosser plate's)
	int mSlotsFilled = 0; // 0x15c: the slots the jewel has filled
	int mPlatesCompleted = 0; // 0x160: the plates the jewel has completed
	int mBounces = 0; // 0x164: the walls and bumpers the jewel has bounced off
	int mWarpCount = 0; // 0x168: the warps the jewel has gone through (the ninth ends it)
	int mAge = 0; // 0x16c: the updates since the last bounce
	int mNoPickupScore = 0; // 0x170: the jewel came out of a basket, which has scored for it
	int mHitBumperWithCode = 0; // 0x174: the jewel has bounced off a bumper that runs code, which gives the shot back
	float mStartX = 0; // 0x178: where the jewel was thrown from
	float mStartY = 0; // 0x17c
	float mTint[3] = {1.0f, 1.0f, 1.0f}; // 0x180: the colour of the beam that the shooter shows for this jewel
	Slot* mTarget = nullptr; // 0x18c: the slot a jewel that was thrown back is flying to
	Slot* mLastSlot = nullptr; // 0x190: the slot the jewel last bounced off

	explicit Jewel(int theKind); // 0x409fa0
	bool Update() override; // 0x40a6b0

	void SetKind(int theKind); // 0x40a390: the picture of the colour

	// 0x428ec0: the jewel bounces off a bumper whose outline is the edges (each a start and end point, before the bumper's
	// turn and place are applied; the call leaves them in the world's frame) when it comes nearer than the radius to one;
	// true when it bounced
	bool CollideWithEdges(SceneObject* theBumper, float (*theEdges)[4], int theEdgeCount, float theRadius);

private:
	bool KeepInBounds(); // 0x40aea0: the walls bounce the jewel, and one that has gone out of the bottom is lost
	bool FlyToTarget(); // 0x40a860: the jewel that was thrown back flies to the slot it was aimed at
	void ExplodeAfterBumps(); // 0x40aa60
	void Spin(); // 0x40adb0
	void ShotSparkle(); // 0x40acb0: the sparkles that follow a power shot
	void Trail(); // 0x40ab70: the glow that follows a jewel that was thrown back
	bool PickedUp(); // 0x40bc60: the shooter picks up a jewel that has come back down to it
	void EmitReadySparkle(); // 0x40b370: the sparkles of the jewel that is ready to throw
	void LandInWater(); // 0x40b5e0: the jewel falls into the water (or the cupid saves it)
	void CountBounce(); // a bounce counts only when the jewel has flown for five updates since the last one
};

#endif
