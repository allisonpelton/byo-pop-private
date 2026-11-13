// The plates: the shapes that hold the slots the player fills with jewels. A plate rises with the level, rotates when jewels
// hit it, and pops (and the combo counts) when every one of its slots is filled. The kinds of plate (PlateTypes.h) differ in
// how their slots are placed, how a jewel's hit moves them, and a few of them in how they behave.

#ifndef VENICE_PLATE_H
#define VENICE_PLATE_H

#include "SceneObject.h"

class Jewel;
class Slot;

// 0x178 bytes in the original (0x17c for the kinds that keep one more number)
class Plate : public SceneObject
{
public:
	int mSlotCount = 0; // 0x14c: the number of slots ("num_jewels" in the data file)
	float mRadius = 0; // 0x150: how far from its middle the slots sit ("radius")
	int mTumbleLevel = 0; // 0x154: the plates that the jewel which completed this plate has completed (1 for the first)
	char mShapeName[32] = {}; // 0x158: the kind of plate (the name of its type in the data files)

	Plate(); // 0x412800
	void Load(const char* theText, SceneTag* theTag) override; // 0x4128a0
	void OnObjectClosed() override; // 0x412920
	bool Update() override; // 0x412960

	// slots 36 to 40 of the original's virtual table: what each kind of plate does differently
	virtual void PopEffect(); // 0x413250 (the block plate has none, the key plate's also sucks in the baskets)
	virtual void OnJewelHit(Slot* theSlot, Jewel* theJewel) {} // 0x413230: a jewel hit one of the plate's slots
	virtual void SetUp() = 0; // the kind's name, and its slots made and placed
	virtual void OnSlotFilled() {} // 0x413240: one of the plate's slots was just filled
	virtual void PositionSlot(Slot* theSlot, int theIndex) = 0; // 0x414b20: where the slot goes on the plate

	int CountFilledSlots(); // 0x414df0
	bool AllSlotsFilled(); // 0x414b80

protected:
	void SetUpSlots(); // 0x414ab0
	void PlaceSlot(Slot* theSlot, float theX, float theY); // 0x414b30
	// the slot goes on the circle of the plate's radius, at an angle in degrees (0x414f00, 0x415280, 0x415390)
	void PlaceOnCircle(Slot* theSlot, float theAngle);
	// 0x412f00: a jewel that hit a slot knocks the plate down and back up
	void Rattle(Jewel* theJewel, Slot* theSlot, float theScale);
	// 0x412f90: the plate wobbles from side to side
	void Wobble(float theAmplitude, float thePeriod);
	// 0x413150: a jewel that hit a slot turns the plate by how far off the plate's middle the hit was
	void Twist(Jewel* theJewel, Slot* theSlot, float theScale, float theDecay);
	// 0x413010: the angle the jewel's hit turns the plate through, in radians
	double TwistAmount(Jewel* theJewel, Slot* theSlot);
	// 0x414e40: the plate that is turning turns faster
	void BoostSpin();
	// 0x4155b0, 0x415f20, 0x416280: a hit off the plate's middle, or by a jewel that moves sideways, makes it wobble
	void WobbleOnHit(Slot* theSlot, Jewel* theJewel, float theAmplitude, float thePeriod, float theRattle);
	void SuckBaskets(bool theByVictory); // 0x412c20: the baskets and bumpers near the plate are pulled into it

private:
	Slot* CreateSlot(int theIndex); // 0x414c60: a slot of a colour that does not repeat that of the one before it
	void WarnOfDanger(); // 0x414050
	void CheckDanger(); // 0x413f90
};

#endif
