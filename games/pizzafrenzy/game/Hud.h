#ifndef __PIZZAFRENZY_HUD_H__
#define __PIZZAFRENZY_HUD_H__

// What the player sees over the game (0x2b4 bytes, constructor 0x438ac0, vtable 0x4fd888; the application keeps it at
// +0x118): the orders waiting, the progress, the tip jar and the stars of the happiness, from the layout
// res\screenLayouts\hudScreen.xml, and the coupon buttons of the toppings that can be bought. It shows the game's
// numbers (the stats') and changes as they change: the mode's frame calls updateHud.
//
// Left out: the two particle systems (the tip jar's coins at +0x284 and the stars' burst at +0x214), which the
// signal build's particle documents (0x47bc40) make and are not translated, and the splat of the full happiness
// (0x4397d0, the splat "happinessBonus").

#include "CouponButton.h"
#include "GameStats.h"
#include "GenericScreen.h"
#include "ImageNode.h"
#include "Modifiers.h"
#include "Sound.h"
#include "TextItem.h"

#include <list>

namespace pizzafrenzy
{

class Hud : public engine::GenericScreen
{
public:
	Hud();  // 0x438ac0

	// slot 0x17c: 0x439ba0; the layout is loaded, its nodes found and the stars made
	void setup(GameStats* theStats);
	// the point the tip jar's count is at (+0x29c), where the money of a tip flies to
	const engine::Point& getTipJarCountPosition() const { return mTipJarCountPosition; }
	// slot 0x190: 0x438fd0: a frame of the hud: the tip jar (its money rolls to the stats' at the goal's size in a
	// second), the orders, the happiness (its stars fade to the new number, with a sound), the coupon buttons, and
	// the screen's own update
	void updateHud(engine::UpdateInfo& theInfo);
	// slot 0x180: 0x4382d0: the stars show the happiness, 20 for each, the one that fades from the last number to the
	// stats' by the fade's share
	void updateStars();
	// slot 0x184: 0x4393f0: the tip jar's contents are as high as its money (the rolling number while it rolls) is
	// of the goal, and the money is written
	void updateTipJar();
	// slot 0x188: 0x4388a0: each coupon button shows whether the tips buy it
	void updateCoupons();
	// slot 0x18c: 0x439650: the number of orders to go (none shown at 0) and a slip for each 15, rounded up
	void updateOrders();
	// 0x43a5b0: a coupon button for each of the kitchens' toppings, with the price 200
	void showCoupons();
	// 0x4392e0: the hud's counters start over with a level, and its coupon buttons go
	void resetHud();
	// 0x4397d0: the happiness shows (the stars' share is the update's); a full one gives 100 in tips with a sound
	void updateHappiness();
	// 0x438610: the warning stars of the low happiness are shown (flashing) or hidden
	void setWarning(bool theShown);
	// 0x438270: the tip jar's roll sound stops with the game paused and goes on with it
	void pauseTipJarSound(bool thePaused);
	// 0x4387f0: the tip jar's roll sound stops, and its coins' emitter (not translated) with it
	void stopTipJar();
	// 0x4388d0: the coupon buttons slide out of the screen to the left
	void slideCouponsOut();
	// 0x438570: the money's backdrop flashes for two seconds: the tips do not buy what was clicked
	void flashScoreBackground();
	// 0x4399d0: the coupon button is put at the right of the row (a place for each one that is not gone) and shown
	void addCoupon(CouponButton* theCoupon);
	// 0x438970: the coupon button is let go, and the others close the gap
	void removeCoupon(CouponButton* theCoupon);

	// +0x244: the coupon buttons, in the order they were added
	std::list<engine::Ref<CouponButton>> mCoupons;

private:
	enum { STAR_COUNT = 5 };

	// 0x438720: the rolling money goes on for the seconds
	void rollTipJar(engine::UpdateInfo& theInfo);
	// 0x4386d0: the money rolls from the number
	void startRoll(int theFrom);
	// 0x439b00: a coupon button for the topping (a kitchen's, drawn, for none), shown at the end of the row
	void makeCoupon(Topping* theTopping, int thePrice);
	// 0x438a40: the coupon buttons are let go
	void clearCoupons();
	// the y of the star that is the step-th of the tenths of the stars' height from the bottom
	float getStarY(int theStep) const;

	engine::Ref<engine::TextItem> mOrderCount;                   // +0x1d4: "orderCount"
	engine::Ref<engine::Node> mProgressText;                     // +0x1d8: "progressText"
	engine::Ref<GameStats> mStats;                               // +0x1dc
	engine::Ref<engine::ImageNode> mTipJarContents;              // +0x1e0: "tipJarContents", a part of its picture
	engine::Ref<engine::TextItem> mTipJarCount;                  // +0x1e4: "tipJarCount"
	engine::Ref<engine::TextItem> mTipJarMax;                    // +0x1e8: "tipJarMax"
	engine::Ref<engine::Container> mProgressStack;               // +0x1f0: "progressStack"
	engine::Ref<engine::Node> mScoreBackground;                  // +0x1f4: "scoreBkg", shown by the flash
	engine::Ref<engine::Container> mWarningStars;                // +0x1fc: the stars of the warning, not shown
	engine::Ref<engine::ImageNode> mStars[STAR_COUNT];           // +0x200
	float mScoreFlashTime;                                       // +0x240: the seconds the backdrop's flash has left
	engine::Rect mTipJarRect;                                    // +0x250: the bounds of the group "tipJar"
	engine::Rect mStarsRect;                                     // +0x260: the bounds of the group "stars"
	int mShownOrderCount;                                        // +0x270: the orders to go the hud wrote last
	int mShownTipMoney;                                          // +0x274: the money the roll goes to
	int mShownHappiness;                                         // +0x278: the happiness the stars go to
	int mShownPickCount;                                         // +0x280: the orders picked the coupons show for
	bool mTipJarRolling;                                         // +0x288: the money rolls to the stats'
	int mRollingTipMoney;                                        // +0x28c: the money the jar shows while it rolls
	engine::Sound* mRollSound;                                   // +0x294: the voice "tip_jarRollup"
	engine::Point mTipJarCountPosition;                          // +0x29c
	float mHappinessFade;                                        // +0x2a4: 1 when the happiness changed, down to 0
	int mPreviousHappiness;                                      // +0x2a8: the happiness the fade starts from
};

} // namespace pizzafrenzy

#endif
