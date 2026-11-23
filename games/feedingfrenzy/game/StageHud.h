#ifndef __FEEDINGFRENZY_STAGEHUD_H__
#define __FEEDINGFRENZY_STAGEHUD_H__

// The display over a stage (StageHud, 0x60 bytes, constructor 0x431f00): the score, the time left, the fish left, the
// meter of the player's growth with a mark at each size, the meter of the power, the icons of the bonuses that last,
// the frenzy's words and the pictures of the fish the player can eat. Its layout is the stage's HUD file; the stage
// updates it in each frame.

#include "GrowMeter.h"
#include "ImageFilter.h"
#include "ScreenLayout.h"
#include "SplatFactory.h"
#include "TextItem.h"

#include <string>

namespace feedingfrenzy
{

class StageMgr;

// How many pictures of the fish the player can eat the HUD has room for
const int HUD_MENU_BUBBLES = 4;

class StageHud
{
public:
	StageHud();                                                      // 0x431f00
	void load(const std::string& theFile, StageMgr* theStage);       // 0x431f30
	// 0x432550: the picture of a kind of fish is put in the bubble of its place; it shows until the player is above its
	// level
	void setMenuBubble(int theIndex, const std::string& theImage, float theLevel, engine::HsvShiftFilter* theShift);
	void update(engine::UpdateInfo& theInfo);                        // 0x4327c0
	engine::ScreenLayout* getRoot() { return mRoot; }

private:
	// 0x4327c0: a bonus icon is off while there is none, on while it has two seconds or more, and blinks in its last
	void blinkBonusIcon(engine::Node* theIcon, float theSecondsLeft, float& theBlinkSeconds, float theSeconds);

	engine::Ref<engine::ScreenLayout> mRoot;  // +0x00
	GrowMeter* mGrowMeter;                    // +0x04
	GrowMeter* mSpecialMeter;                 // +0x08
	engine::TextItem* mScoreText;             // +0x0c
	engine::TextItem* mTimerText;             // +0x10
	engine::TextItem* mLifeCountText;         // +0x14
	engine::TextItem* mCounterTextEaten;      // +0x18: the fish eaten, in a bonus stage
	engine::TextItem* mCounterTextToEat;      // +0x1c: the fish to eat
	engine::Node* mBonusGroup;                // +0x20: shows while a bonus lasts
	engine::Node* mDoubleScoreIcon;           // +0x24
	engine::Node* mSpeedBoostIcon;            // +0x28
	float mSpeedBoostBlinkSeconds;            // +0x2c: the seconds until the icon shows or hides again
	float mDoubleScoreBlinkSeconds;           // +0x30
	float mGrowthTotal;                       // +0x34: the score of the biggest size, or -1 when there are two sizes or fewer
	engine::Node* mMenuBubbles[HUD_MENU_BUBBLES];  // +0x38
	float mBubbleLevels[HUD_MENU_BUBBLES];    // +0x48: the bubble shows while the player's level is no higher
	float mWarnSeconds;                       // +0x58: the whole seconds the last warning was given at
	engine::SplatFactory* mSplats;            // +0x5c: the time's warning
};

} // namespace feedingfrenzy

#endif
