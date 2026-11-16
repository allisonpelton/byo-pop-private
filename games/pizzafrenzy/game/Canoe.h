#ifndef __PIZZAFRENZY_CANOE_H__
#define __PIZZAFRENZY_CANOE_H__

// The canoe (0x18c bytes, constructor 0x431ad0, vtable 0x4fcc08): a van, for the kitchens whose vehicle type is of the
// class "canoe", that drives the same roads and parks the same way (its loadType, slot 0x140 0x431b50, is the van's)
// and differs in two things: it leaves with no sound, and it turns to face the way it goes (a line along the
// water is not one of the road's four ways).

#include "Van.h"

namespace pizzafrenzy
{

class Canoe : public Van
{
public:
	explicit Canoe(const std::string& theTypeName);  // 0x431ad0

	// slot 0x154: 0x431c10: the canoe sets out for the house, with no sound (not when it is on its way, or at the
	// house's road already)
	void goTo(CustomerTile* theTile) override;
	// slot 0x164: 0x431e40
	void face(const engine::IntPair& theCell) override;
};

} // namespace pizzafrenzy

#endif
