#ifndef __PIZZAFRENZY_TOPPINGBOOKSCREEN_H__
#define __PIZZAFRENZY_TOPPINGBOOKSCREEN_H__

// The topping book (0x2a0 bytes, constructor 0x445b20, vtable 0x4fe6d0; the application keeps it at +0xe0, from
// res/screenLayouts/toppingBook.xml): a book of two pages that opens from the main menu and from the pause menu. Its
// first left page lists the score titles the player has reached; every other page shows one topping, in the order of
// the tile manifest: what the player has of it, what its upgrade is worth and what the next one needs. A topping the
// player has not met is shown dark. The arrows turn two pages; its other actions go on to the application.

#include "GenericScreen.h"
#include "ImageFilter.h"
#include "ImageNode.h"
#include "TextItem.h"
#include "TileEntries.h"

#include <map>

namespace pizzafrenzy
{

class ToppingBookScreen : public engine::GenericScreen
{
public:
	ToppingBookScreen() : mPage(0) {}  // 0x445b20

	// 0x17c: 0x444db0: the layout's nodes (a left and a right page), the dark picture of each topping, and the first
	// pages; the screen hears its own actions
	void setup();
	// 0x180: 0x4446c0: the pages at the book's place are shown: the titles and the first topping, or two toppings
	void showPages();
	// 0x184: 0x443d80 (the action's slot): the arrows turn the pages; any other action goes on
	void onAction(std::string theAction);
	// 0x188: 0x4455d0: the page of the side (0 left, 1 right) shows the topping, with the level the player has of it
	// (-1 for none), under the page's number
	void showPage(Topping* theTopping, int theLevel, int theSide, int thePage);
	// 0x18c: 0x443e30: the left page lists the titles the player's points reach, the last in red
	void showTitles();

private:
	engine::Ref<engine::Container> mTitles;               // +0x1d4: "leftTitles"
	engine::Ref<engine::Node> mPages[2];                  // +0x1d8: "leftPage", "rightPage"
	engine::Ref<engine::ImageNode> mToppingBgs[2];        // +0x1e0: the coupon behind the topping
	engine::Ref<engine::ImageNode> mToppingPictures[2];   // +0x1e8
	engine::Ref<engine::Node> mLockedOverlays[2];         // +0x1f0: shown over a topping that is new to the player
	engine::Ref<engine::TextItem> mCaptions[2];           // +0x1f8: the upgrade's name
	engine::Ref<engine::TextItem> mValues[2];             // +0x200
	engine::Ref<engine::TextItem> mLevels[2];             // +0x208
	engine::Ref<engine::TextItem> mNextAts[2];            // +0x210: what the next upgrade needs
	engine::Ref<engine::TextItem> mDescriptions[2];       // +0x218
	engine::Ref<engine::TextItem> mPageNumbers[2];        // +0x220
	engine::Ref<engine::ImageButton> mTurnButtons[2];     // +0x228: the arrows
	ToppingMap<engine::Ref<engine::Image>> mDarkImages;  // +0x230: a topping's picture with its colours gone
	engine::HsvShiftFilter mDarkFilter;                   // +0x23c: a counted object in the original
	std::string mValueTemplate;                           // +0x240: the left value's text as the layout has it
	std::string mLevelTemplate;                           // +0x25c
	std::string mPageTemplate;                            // +0x278
	int mPage;                                            // +0x294: the page on the left (0 for the titles)
};

} // namespace pizzafrenzy

#endif
