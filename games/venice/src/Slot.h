// The slots: the places on a plate that a jewel of the slot's colour fills. A slot checks every jewel in flight, fills when one of
// its colour reaches it (and sets off a cascade of the slots of the same colour near it when the jewel is a combo shot), and
// throws the jewel back out when the game's tuning has a filled slot toss it.

#ifndef VENICE_SLOT_H
#define VENICE_SLOT_H

#include "SceneObject.h"

class Jewel;

// 0x168 bytes in the original
class Slot : public SceneObject
{
public:
	int mKind = 0; // 0x148: the colour (3 and up; 5 is the one colour when there is only one kind of slot, 999 is a wild one)
	int mFilled = 0; // 0x14c
	bool mLinked = false; // 0x150: a cascade is on its way to fill this slot
	int mTossSearches = 0; // 0x154: the slots that were thrown to this one's place this update (the jewel that is thrown goes to the least)
	int mSlotsFilled = 0; // 0x158: the slots the jewel that filled this slot had filled, with this one
	int mPlatesCompleted = 0; // 0x15c: the plates the jewel that filled this slot had completed, with this one's plate
	float mSpin = 0; // 0x160: the sideways speed of the jewel that filled this slot, which the jewel it throws back out keeps

	Slot(); // 0x41c040
	~Slot() override; // 0x41c130
	void Load(const char* theText, SceneTag* theTag) override; // 0x41c270
	bool Update() override; // 0x41c3b0
	void OnCustomEvent() override; // 0x41fc40

	// 0x41c350: the picture of the slot's colour
	void ShowKind();
	// 0x41c450: a ring of light that fades round the slot (the ring's picture is the corona's unless one is given)
	void Glow(float theDuration, float theScale, const char* theImageName);
	// 0x41cbe0: the slot is filled by the jewel (none when a cascade fills it); a slot filled by a cascade does not move the
	// scroll as one filled by a jewel does
	void FillSlot(Jewel* theJewel, bool theCascaded);
	// 0x41ef80: the slot's jewel is thrown out again, with the sideways speed, to land on another slot of its colour (a free toss
	// keeps the speed it is given, is not aimed at a slot, and is bounced by the bumpers)
	void TossJewel(int theSlotsFilled, int thePlatesCompleted, float theXVelocity, bool theFreeToss);

private:
	static int ChooseKind(); // 0x41c1c0
	void CheckJewel(Jewel* theJewel); // 0x41c5a0
	void Cascade(bool theCascaded); // 0x41f500: the slots of the same colour near this filled one are filled from it, one after another
	void ShowPopup(int thePoints, const char* theLabel); // 0x41d600
	void ShowSuperTumble(int thePoints); // 0x41de50
	void ShowTumblePopup(Jewel* theJewel); // 0x41eb20: the message that goes with the score of a jewel that filled a slot
	// 0x41ec00: the sideways speed that takes a thrown jewel to the nearest slot of its colour below it
	float FindTossTarget(float theXVelocity, Slot** theTarget);
};

// The line that runs from a filled slot to a slot of the same colour that the filling cascades to (0x154 bytes in the original)
class CascadeLink : public SceneObject
{
public:
	float mStartX = 0; // 0x148: where the cascade started
	float mStartY = 0; // 0x14c
	Slot* mSlot = nullptr; // 0x150: the slot it fills

	CascadeLink(Slot* theSource, Slot* theTarget, int theDelay); // 0x41fc90
	bool Update() override; // 0x41fe90
};

#endif
