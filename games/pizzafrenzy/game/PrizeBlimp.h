#ifndef __PIZZAFRENZY_PRIZEBLIMP_H__
#define __PIZZAFRENZY_PRIZEBLIMP_H__

// The prize blimp (0x1b8 bytes, constructor 0x431330, vtable 0x4fc910): the vehicle that ends a level in which every
// pizza ordered was delivered. It flies across the city's sky over its shadow, and drops three balloon tips (Tip.h)
// every second or so while it is over the middle of the screen, until it has flown off the other side. A dot matrix
// sign under it scrolls a message with the player's name.

#include "ImageNode.h"
#include "Sound.h"
#include "Van.h"

namespace pizzafrenzy
{

class PrizeBlimp : public Van
{
public:
	PrizeBlimp();  // 0x431330

	// slot 0x140: 0x430d50: the pictures and the speed of the vehicle type "PrizeBlimp", its shadow, and the waves
	// that rock the two
	void loadType() override;
	// slot 0x144: 0x430cc0: a frame: the blimp flies on, and drops its tips while it is over the screen
	void updateVehicle(engine::UpdateInfo& theInfo) override;
	// slot 0x164: 0x430b30: the picture faces the way the blimp flies (the cell is not looked at)
	void face(const engine::IntPair& theCell) override;
	// slot 0x168: 0x431110: the blimp stands on the map at the point, and its engine runs
	void start(CityMap* theMap, const engine::Point& thePosition);
	// slot 0x16c: 0x430b10: the blimp has flown off the screen
	bool isDone() const { return mFlownOff; }
	// slot 0x170: 0x430ac0: the blimp flies along the direction at the speed of its type, and faces that way
	void setDirection(const engine::Point& theDirection);
	// slot 0x174: 0x431220: three balloon tips are dropped under the blimp
	void dropTips();

private:
	// 0x430970: the blimp flies on for the seconds, and is let go once it is off the screen
	void fly(engine::UpdateInfo& theInfo);
	// 0x430b90: the sign's text scrolls on through its window
	void scrollSign(engine::UpdateInfo& theInfo);

	engine::Point mVelocity;     // +0x194
	float mDropTimer;            // +0x19c: the seconds before the next drop
	engine::Sound* mEngine;      // +0x1a0: the voice "bonus_blimpEngine"
	bool mFlownOff;              // +0x1a4: the blimp is off the screen
	engine::Ref<engine::ImageNode> mSign;  // +0x1a8: the dot matrix sign
	float mSignTimer;            // +0x1ac: the seconds left of the sign's pass
};

} // namespace pizzafrenzy

#endif
