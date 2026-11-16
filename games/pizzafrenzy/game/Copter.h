#ifndef __PIZZAFRENZY_COPTER_H__
#define __PIZZAFRENZY_COPTER_H__

// The copter (0x190 bytes, constructor 0x431dc0, vtable 0x4fcd70): a van, for the kitchens whose vehicle type is of
// the class "copter", that flies over the houses instead of driving the roads. It waits parked at its kitchen's road,
// then, sent to a house, lifts off (half a second in which its pictures rise 100 pixels), flies in a straight line to
// the house's road cell in the air group, comes down the same way in the ground group, hovers there for a second and
// finishes the house. The original keeps these as state objects (parked 0x4fcef4, lifting 0x4fcee0, flying 0x4fcf08,
// arrived 0x4fcf1c); the port keeps them as phases of the van's.
//
// Left out: the lifting state's exit has a second branch for a copter that has landed (the path is let go, the home
// tile takes the copter back and the sound "copterReturn" plays); the one flag that selects it is false wherever the
// state is made, so nothing reaches it.

#include "Van.h"

namespace pizzafrenzy
{

class Copter : public Van
{
public:
	explicit Copter(const std::string& theTypeName);  // 0x431dc0

	// slot 0x140: 0x432670: the pictures are put in a container of their own that the lift moves, with the hovering
	// and parked pictures after the three of the van's
	void loadType() override;
	// slot 0x144: 0x42f780 (the van's): the lift's frame, or the van's
	void updateVehicle(engine::UpdateInfo& theInfo) override;
	// slot 0x154: 0x4323f0: the copter sets out for the house, with a sound (not when it is on its way, or at the
	// house's road already)
	void goTo(CustomerTile* theTile) override;
	// slot 0x158: 0x4328d0: the way is the straight line, from the copter's cell to the cell
	std::vector<engine::IntPair> findWay(const engine::IntPair& theCell) override;
	// slot 0x164: 0x431e40
	void face(const engine::IntPair& theCell) override;

protected:
	// 0x431f20 (the parked state's entry): the parked picture, at its kitchen's road
	void park() override;
	// 0x432320 (the flying state's message "arrived"): the copter comes down
	void reachGoal() override;

private:
	// 0x432060 (the lifting state's entry): the lift begins, and a copter that lands is put in the ground group
	void beginLift(bool theTakeOff);
	// 0x431d00 (the lifting state's update): the lift's frame, and at its end the flight or the arrival
	void lift(engine::UpdateInfo& theInfo);
	// 0x432550 (the lifting state's exit): a copter that took off faces its way and is put in the air group
	void endLift();

	engine::Ref<engine::Container> mBody;  // +0x184: holds the pictures and the effect; the lift moves it
	float mLiftTime;                        // the seconds the lift has left
};

} // namespace pizzafrenzy

#endif
