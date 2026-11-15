#ifndef __PIZZAFRENZY_HUD_H__
#define __PIZZAFRENZY_HUD_H__

// What the player sees over the game (0x2b4 bytes, constructor 0x438ac0, vtable 0x4fd888; the application keeps it at
// +0x118): the orders waiting, the progress, the tip jar and the stars of the happiness, from the layout
// res\screenLayouts\hudScreen.xml. What the game's play does with them (showing the counts, the tip jar's filling,
// the stars' bursts, the pictures of the vehicles that can be bought at +0x244, slots 0x180 to 0x190 and the
// functions at 0x439650, 0x4397d0 and 0x439b00) is translated with the game.
//
// Left out: the two particle systems (the tip jar's coins at +0x284 and the stars' burst at +0x214), which the
// signal build's particle documents (0x47bc40) make and are not translated, and the sound of the tip jar's roll
// (+0x294, the sound named "tip_jarRollup").

#include "GameStats.h"
#include "GenericScreen.h"
#include "ImageNode.h"

namespace pizzafrenzy
{

class Hud : public engine::GenericScreen
{
public:
	Hud();  // 0x438ac0

	// slot 0x17c: 0x439ba0; the layout is loaded, its nodes found and the stars made
	void setup(GameStats* theStats);
	// slot 0x190: a frame of the hud; translated next
	void updateHud(engine::UpdateInfo& theInfo);
	// 0x43a5b0: the vehicles that can be bought are shown (0x439b00 for each of the map's, with the number 200);
	// translated next
	void showVehicles();
	// 0x438270: the tip jar's roll sound stops with the game paused and goes on with it; left out with the sound
	void pauseTipJarSound(bool thePaused);

private:
	enum { STAR_COUNT = 5 };

	engine::Ref<engine::Node> mOrderCount;                       // +0x1d4: "orderCount"
	engine::Ref<engine::Node> mProgressText;                     // +0x1d8: "progressText"
	engine::Ref<GameStats> mStats;                               // +0x1dc
	engine::Ref<engine::ImageNode> mTipJarContents;              // +0x1e0: "tipJarContents", a part of its picture
	engine::Ref<engine::Node> mTipJarCount;                      // +0x1e4: "tipJarCount"
	engine::Ref<engine::Node> mTipJarMax;                        // +0x1e8: "tipJarMax"
	engine::Ref<engine::Node> mProgressStack;                    // +0x1f0: "progressStack"
	engine::Ref<engine::Node> mScoreBackground;                  // +0x1f4: "scoreBkg", not shown
	engine::Ref<engine::Container> mWarningStars;                // +0x1fc: the stars of the warning, not shown
	engine::Ref<engine::ImageNode> mStars[STAR_COUNT];           // +0x200
	engine::Rect mTipJarRect;                                    // +0x250: the bounds of the group "tipJar"
	engine::Rect mStarsRect;                                     // +0x260: the bounds of the group "stars"
	engine::Point mTipJarCountPosition;                          // +0x29c
};

} // namespace pizzafrenzy

#endif
