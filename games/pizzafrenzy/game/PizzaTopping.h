#ifndef __PIZZAFRENZY_PIZZATOPPING_H__
#define __PIZZAFRENZY_PIZZATOPPING_H__

// A topping on a pizza of the bonus round (0x19c bytes, constructor 0x419950, vtable 0x4fa8e0): a button with the
// topping's picture, which the round puts on the target pizza and on the player's. A marked one shows its over image.
//
// It can be dragged (the mouse slots 0xa0 and 0xd0 keep where it was grabbed); the toppings of the round and of the
// pizza editor are not enabled, so that nothing reaches them.

#include "Button.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class PizzaTopping : public engine::ImageButton
{
public:
	PizzaTopping() : mMarked(false), mDragOffset(0, 0) {}  // 0x419950

	// 0x170: 0x419a30: the picture centred, no sound on entering and the menu's click, and the topping
	void setTopping(Topping* theTopping);
	// 0x174
	Topping* getTopping() { return mTopping; }
	// 0xa0: 0x4197c0: where the mouse took the topping is kept, as its place less the mouse's
	void mouseDownEvent(engine::MouseEvent& e) override;
	// 0xd0: 0x419760: while the topping is pressed, it follows the mouse
	void mouseMoveEvent(engine::MouseEvent& e) override;

	// 0x178: 0x419830: a marked topping shows its over image
	void setMarked(bool theMarked);
	// 0x17c
	bool isMarked() const { return mMarked; }

private:
	engine::Ref<Topping> mTopping;  // +0x184
	bool mMarked;                   // +0x190
	engine::Point mDragOffset;      // +0x188
};

} // namespace pizzafrenzy

#endif
