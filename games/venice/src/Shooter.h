// The shooter: the object at the bottom of the screen that follows the pointer, holds the jewels in its row, throws the one that is
// ready when the player clicks, and keeps the power-ups the player has collected (the wings, the wild jewels, the power shots, the
// combo shot and the score multiplier). It also sinks when the level is lost and brings the cupid when the player waits too long.

#ifndef VENICE_SHOOTER_H
#define VENICE_SHOOTER_H

#include "SceneObject.h"

class Jewel;
class Slot;

// 0x178 bytes in the original
class Shooter : public SceneObject
{
public:
	int mCanShoot = 0; // 0x148: the level's panel sets it on every update (1 unless the pointer is on one of its buttons, or the tutorial runs)
	int mPowerShots = 0; // 0x14c: the shots left that pass through the bumpers
	int mWingsTicks = 0; // 0x150: the updates the wings have left; the shooter flies after the pointer while there are any
	int mComboShot = 0; // 0x154: the next shot sets off a cascade where it fills a slot
	int mWildShots = 0; // 0x158: the jewels that are to become wild ones as they are loaded
	int mShotsFired = 0; // 0x15c
	int mScoreMultiplier = 1; // 0x160
	int mSinkCount = 0; // 0x164: the times the shooter has sunk (nothing reads it)
	Slot* mTargetSlot = nullptr; // 0x168: the slot the jewel in flight is aimed at, which a jewel that fills it clears
	float mMoveX = 0; // 0x16c: how far the shooter moved sideways on the latest update (the jewels in its row follow)
	float mMoveY = 0; // 0x170
	float mCupidWait = 0; // 0x174: the updates since the shooter was last safe, which bring the cupid when they run out

	Shooter(); // 0x419790
	bool Update() override; // 0x4199e0
	void Draw() override; // 0x41bad0

	void Sink(); // 0x419ee0: the shooter goes down into the water and the jewels in the air fade
	void ActivateWild(); // 0x41a250: the jewels in the row that are not wild yet become wild ones, as many as the wild shots left
	void ActivateWings(); // 0x41a2e0: the wings (2000 updates more of them) and the sound they come with
	Jewel* GetReadyJewel(); // 0x41ba10: the jewel in the row that is ready to throw

private:
	void TrackMouse(); // 0x419c80
	void FollowMouse(); // 0x419890
	void Straighten(); // 0x419950
	void UpdateCupid(); // 0x419d30
	void UpdateWings(); // 0x41a710
	void AimSparkles(); // 0x41abb0: the ready jewel of a combo shot sparkles
	void SwapJewels(); // 0x41ad40: the right button swaps the two jewels in the row
	int CountJewelsOfKind(int theKind); // 0x41ae60
	void UpdateJewelColours(); // 0x41afa0: the jewels in the row change colour to one that the plates still need
	int PickColour(Jewel* theExcluded); // 0x41b170: a colour for a jewel that is not the colour of the other one
	void Reload(); // 0x41b3f0
	void Fire(); // 0x41b620
	void EmitPowerSparkle(); // the sparkle that rises from a shooter with power shots (a part of 0x4199e0)
	void UpdateBeam(); // the beam of the power shot (a part of 0x4199e0)
};

// port: the slot counts the colour choice reads (0x5ba3f4, eleven of them, and the number of colours at 0x5ba420), for a test's trace
void GetSlotKindCountsForTest(int* theCounts);

#endif
