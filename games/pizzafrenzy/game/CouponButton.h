#ifndef __PIZZAFRENZY_COUPONBUTTON_H__
#define __PIZZAFRENZY_COUPONBUTTON_H__

// A coupon button of the hud (0x180 bytes, constructor 0x44fa80, vtable 0x4ffc88): a topping that the tips buy for the
// price, which the orders picked (or the next customer clicked) then ask for. A click takes it off the hud's row and
// gives it to the mode (GameMode::pickCoupon), which holds it on the cursor until it is used or let go (putBack
// puts it back in the row).

#include "ImageNode.h"
#include "Node.h"
#include "Ramp.h"
#include "Selector.h"
#include "State.h"
#include "TextItem.h"
#include "TileEntries.h"

namespace pizzafrenzy
{

class CouponButton : public engine::Container
{
public:
	CouponButton();            // 0x44fa80
	~CouponButton() override;  // 0x44f160 (0x44ef50)

	// slot 0x12c: 0x44f500: the topping's pictures on the frame, its price, and its name for the caption
	virtual void setup(Topping* theTopping, int thePrice);
	// slot 0x130: 0x44eaa0: a frame of the button: its slide
	virtual void updateCoupon(engine::UpdateInfo& theInfo);
	// slot 0x134: 0x44f3f0: the tips buy it (the price for each order picked): it leaves the row and the mode takes
	// it; else the money's backdrop flashes with a sound
	virtual void buy();
	// slot 0x138: 0x44ed20: the coupon goes back in the hud's row
	virtual void putBack();
	// slot 0x13c: 0x44ed50: the coupon leaves the hud's row
	virtual void leaveHud();
	// slot 0x140: 0x44f0d0: the button slides to the point in the seconds (a button that goes left is gone)
	virtual void moveTo(const engine::Point& thePoint, float theSeconds);
	// slot 0x144: 0x44eab0
	virtual Topping* getTopping() { return mTopping; }
	// slot 0x148: 0x44f080
	virtual int getPrice() { return mPrice; }
	// slot 0x14c: 0x44f180: the price is written on the button
	virtual void setPrice(int thePrice);
	// slot 0x150: 0x44f090: the button has gone left, to be let go of when it is off the screen
	virtual bool isGone() { return mGone; }
	// 0x44eac0: the button is dimmed unless the tips buy it for each order picked
	void updateAffordable(int theTipMoney);

	// slot 0x98: 0x44eca0: the button's bounds are its frame's
	void layout() override;

protected:
	// slot 0xa8: 0x44f210
	void onMouseDownEvent(engine::MouseEvent& theEvent) override;
	// slot 0xe0: 0x44f330: the frame lights and its caption shows, when the tips buy it
	void onMouseEnterEvent(engine::MouseEvent& theEvent) override;
	// slot 0xec: 0x44ec70
	void onMouseExitEvent(engine::MouseEvent& theEvent) override;

private:
	engine::Ref<engine::Selector> mFrame;      // +0x128: the frame, off and lit
	engine::Ref<engine::TextItem> mPriceText;  // +0x12c: the price
	engine::Ref<engine::TextItem> mNameText;   // +0x130: the caption's text
	engine::Ref<engine::Container> mCaption;   // +0x134: the caption under the button, shown with the mouse over
	int mPrice;                                // +0x138
	engine::StateMgr mStates;                  // +0x13c: the slide
	std::string mCaptionText;                  // +0x154: the text of the caption
	bool mAffordable;                          // +0x170: the tips buy it
	bool mGone;                                // +0x171
	Topping* mTopping;                         // +0x174
};

} // namespace pizzafrenzy

#endif
