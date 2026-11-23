#ifndef __FEEDINGFRENZY2_GAMEHUD_H__
#define __FEEDINGFRENZY2_GAMEHUD_H__

// The play's heads-up display (GameHud, 0x260 bytes, vtables 0x5565c4, 0x556720, 0x556738 and 0x556740
// (feedingfrenzy2); its constructor 0x4036d0 (feedingfrenzy2)), the stage's +0x2e0: a screen layout read from
// resource/HUD/hudLayout.xml with the shell's theme (the band along the top), its parts found by name. It shows the
// score, the lives, the stage's name and time, the growth meter with an arrow at each of the player's phases, the
// suck's meter, the frenzy's word lit letter by letter and its multiplier, and the bonus stage's counter. The stage
// puts it in its container when a stage with players is set up (its slot 0x34). Ghidra names its base DebugHud, whose
// table it shares but for the update; the fields it reads are a screen layout's and its own.

#include "Container.h"
#include "ScreenLayout.h"
#include "Splat.h"
#include "TextItem.h"
#include "Theme.h"

#include <string>

class PlayerSpawn;
class Stage;

// A bar filled from the left in its colour (GrowMeter, 0xb4 bytes, vtables 0x556a7c and 0x556b70 (feedingfrenzy2); its
// constructor 0x48ce40 (feedingfrenzy2)): the growth's and the suck's, each inside the layout's bar
class GrowMeter : public engine::Component
{
public:
	// 0x48ce40 (feedingfrenzy2): empty, visible, of the bar's size
	GrowMeter(float theWidth, float theHeight);

	// 0xa0 (0x45faf0 (feedingfrenzy2)): the filled part in the colour (the alpha the state's)
	void paint(engine::DrawContext* theContext) override;
	// 0xa8 (0x4960e0 (feedingfrenzy2)): the whole bar's size at the position
	void computeBounds() override;

	void setMaximum(float theMaximum) { mMaximum = theMaximum; }
	float getMaximum() const { return mMaximum; }
	// Inlined at 0x44b9b0 and 0x455a50 (feedingfrenzy2): the value kept between 0 and the maximum, and the filled
	// part its share of the width
	void setValue(float theValue);

private:
	float mMaximum;      // +0x98
	float mValue;        // +0x9c
	float mWidth;        // +0xa0
	float mHeight;       // +0xa4
	float mFilledWidth;  // +0xa8
};

class GameHud : public engine::ScreenLayout
{
public:
	// 0x4036d0 (feedingfrenzy2): empty, for the stage
	explicit GameHud(Stage* theStage);

	// 0xa4 (0x434a20 (feedingfrenzy2)): the layout's update, then the queues of texts shown one after another
	// (0x4301d0 over the global 0x5acc20 (feedingfrenzy2): the points' bubbles every 0.09 seconds and the others'
	// every 0.3)
	void update(engine::UpdateContext& theContext) override;

	// 0x482f20 (feedingfrenzy2): the layout read and its parts found; the growth's and the suck's meters made in their
	// bars, and the arrows' and the texts' containers added
	void setup(engine::Theme* theStageTheme);
	// 0x48b0e0 (feedingfrenzy2): made ready for a stage: the score shown, the time's warning forgotten, the jump's
	// height hidden, the points' bubbles taken out, the growth's or the boss's caption and, in the timed mode, the
	// lives' plug
	void reset();
	// 0x404f10 (feedingfrenzy2): the growth meter's maximum the last phase's goal, and for a fish with more than one
	// phase (not on a boss's stage) an arrow along the bar at each goal but the last
	void setGrowthGoals(PlayerSpawn* theSpawn);
	// 0x49bcc0 (feedingfrenzy2): the score shown, with a comma between each three digits
	void showScore(int theScore);
	// 0x410740 (feedingfrenzy2): a points bubble's points added to the score shown when it reaches the score
	void addShownScore(int thePoints);
	// 0x44c910 (feedingfrenzy2): the time left as minutes and seconds; under 11 seconds, each second less splats the
	// timeWarning at the clock and plays the stage theme's timerWarning (the sound not yet translated)
	void showTime(int theSeconds);
	// 0x494620 (feedingfrenzy2): the clock shown or hidden
	void showTimer(bool isShown) { mTimerGroup->setVisible(isShown); }
	// 0x402550 (feedingfrenzy2): the lives, at most 99
	void showLives(int theLives);
	// 0x4050d0 (feedingfrenzy2) as the stage's start uses it: the stage's number and title
	void showStageName(const std::string& theName) { mStageNameText->setText(theName); }
	// 0x49c010 (feedingfrenzy2): the bonus stage's counter shown or hidden
	void showCounter(bool isShown) { mCounterGroup->setVisible(isShown); }
	// 0x48a910 (feedingfrenzy2): the suck's meter and its caption, or the plug in their place
	void showSuck(bool canSuck);
	// 0x455a50 (feedingfrenzy2): the suck's charge (0 to 1) on its meter, red below a half and yellow from it
	void showSuckCharge(float theCharge);
	// 0x4605d0 (feedingfrenzy2), the player's: the frenzy's word lit a letter for each fish of the count's seven (all
	// seven at a full seven), the lit letters in the multiplier's colour and the rest in the one before, and the
	// multiplier from two
	void showFrenzy(int theCount, int theMultiplier);
	// 0x41cdb0 (feedingfrenzy2): a chomp's burst at the point, one of the three variants drawn, its size by the
	// biggest rank eaten (chompSplatMini, chompSplatSmall, then chompSplatLarge for the rest)
	void splatChomp(int theRank, const engine::Vector2f& thePoint);
	// 0x40d970 (feedingfrenzy2): the frenzy's word of the step (the strings' frenzy1 and on) splatted at the window's
	// centre, coloured as the multiplier one more than the step
	void splatFrenzy(int theStep);
	// 0x408df0 (feedingfrenzy2): the text rising from the point on the screen in green, in the shell's frenzyFontBold,
	// queued among the texts that are not points
	void showPlacedText(const std::string& theText, const engine::Vector2f& thePoint);
	// 0x494510 (feedingfrenzy2): the strings' timeSplat with the three seconds a time bubble gives, rising from the
	// point
	void showTimeBonus(const engine::Vector2f& thePoint);
	SplatFactory* getSplats() { return mSplats; }

	GrowMeter* getGrowthMeter() { return mGrowthMeter.get(); }
	engine::TextItem* getScoreText() { return mScoreText.get(); }
	int getShownScore() const { return mShownScore; }
	engine::Container* getPointsBubbles() { return mPointsBubbles.get(); }
	GrowMeter* getSuckMeter() { return mSuckMeter.get(); }

private:
	engine::Ref<GrowMeter> mGrowthMeter;                // +0x174: in growthBar
	engine::Ref<engine::Container> mGrowthArrows;      // +0x178: at growthBar's place
	engine::Ref<engine::Container> mGrowthBar;         // +0x17c
	engine::Ref<GrowMeter> mSuckMeter;                 // +0x180: in specialBar, hidden until the stage allows the suck
	engine::Ref<engine::TextItem> mScoreText;          // +0x188
	int mShownScore;                                   // +0x18c: the score the text shows
	engine::Ref<engine::Component> mTimerGroup;        // +0x190
	engine::Ref<engine::TextItem> mTimerText;          // +0x194
	engine::Ref<engine::TextItem> mLifeCountText;      // +0x198
	engine::Ref<engine::Component> mCounterGroup;      // +0x19c
	engine::Ref<engine::TextItem> mCounterTextA;       // +0x1a0
	engine::Ref<engine::TextItem> mCounterTextB;       // +0x1a4
	engine::Ref<engine::TextItem> mStageNameText;      // +0x1a8
	engine::Ref<engine::Component> mBonusGroup;        // +0x1ac
	engine::Ref<engine::Component> mBonusBubble2X;     // +0x1b0
	int mBonus2XShown;                                 // +0x1b4: 0 from the setup; read by code not yet translated
	engine::Ref<engine::Component> mBonusBubbleSpeed;  // +0x1b8
	int mBonusSpeedShown;                              // +0x1bc: likewise
	// +0x1c4 to +0x1d0, the menu's four images (menuImg0 to menuImg3), each in a selector (Selector, 0xd4 bytes) at
	// +0x1d4 to +0x1e0: not yet translated
	int mShownSeconds;                                 // +0x1f4: the time shown last, 0 from the reset
	SplatFactory* mSplats;                             // +0x1f8: from resource/HUD/hudSplats.xml, a child
	Stage* mStage;                                     // +0x1fc
	engine::Ref<engine::Component> mJumpHeightGroup;   // +0x200
	engine::Ref<engine::Component> mJumpHeight;        // +0x204
	// +0x208 to +0x220, the jump's marks, +0x230, the jump's height effect (InOutFx, 0x2f0 bytes), and +0x234 and
	// +0x238, the medals (medalPersonalBest and medalNewRecord): not yet translated
	engine::Ref<engine::Container> mPlacedTexts;       // +0x23c: the texts shown at a place's (0x408df0 (feedingfrenzy2))
	engine::Ref<engine::Container> mPointsBubbles;     // +0x240: the points' bubbles' (0x427250 (feedingfrenzy2)),
	                                                   // emptied by the reset and the stage's slot 0x10
	engine::Ref<engine::Component> mAbilityText;       // +0x244: the suck's caption
	engine::Ref<engine::Component> mAbilityPlug;       // +0x248: in its place when the stage has no suck
	engine::Ref<engine::Component> mLifePlug;          // +0x24c: over the lives in the timed mode
	engine::Ref<engine::Component> mGrowthText;        // +0x250
	engine::Ref<engine::Component> mBossText;          // +0x254: in the growth's caption's place on a boss's stage
};

#endif
