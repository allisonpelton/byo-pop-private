// Sexy::GoodTube (0x770 bytes, vtable 0x7e3d64 with 71 slots, a TwistWidget and a ButtonListener; 0x702736-0x70a9bb):
// the tube beside the board that fills with the level's progress, with the level's number, the multiplier's arc of
// lights, the blitz clock, the bonus challenge's bar and the menu button. Being ported (docs/NOTES.md, "The board").
#pragma once

#include "ButtonListener.h"
#include "ColorCycle.h"
#include "CurvedVal.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Board;
class GoodTube;
class EffectsManager;
class Font;
class Graphics;
class Image;
class TextImage;
class TwistButton;

// 0x6a3403: the pixel of theImage (0 outside it)
unsigned int GetPixelAt(Image* theImage, int theX, int theY);

// 0x84 bytes (0x707960 its constructor, 0x7073fc its update, 0x707476 its drawing): a number that counts up to a
// target and shows as a text (the challenge's count and its best)
struct Ticker {
	SexyString mText;                 // +0x00: the value as a text (0x7073be)
	float mAlpha = 1.0f;              // +0x1c
	float mFade = 0;                  // +0x20: the glow's strength; 1 while the value moves, then it fades
	int mTarget = 0;                  // +0x24
	int mValue = 0;                   // +0x28
	int mTimer = 0;                   // +0x2c: counts the updates
	SexyString mPrefixText;           // +0x30: an empty string, never read
	SexyString mSuffixText;           // +0x4c: an empty string, never read
	Font* mLabelFont = nullptr;       // +0x68: FONT_TOOLTIP, never read
	Font* mSmallLabelFont = nullptr;  // +0x6c: FONT_TOOLTIP, never read
	Font* mFont = nullptr;            // +0x70: FONT_SCORE
	Font* mGlowFont = nullptr;        // +0x74: FONT_SCOREGLOW
	Font* mSmallFont = nullptr;       // +0x78: FONT_SCORE_SMALL
	Font* mSmallGlowFont = nullptr;   // +0x7c: FONT_SCOREGLOW_SMALL
	bool mUseSmallFont = false;       // +0x80: always the small font (never set)

	Ticker();                                                                     // 0x707a98, 0x707960
	void Update();                                                                // 0x7073fc
	void UpdateText();                                                            // 0x7073be
	void Draw(Graphics* g, int theX, int theY, bool theCentre, float theAlpha);  // 0x707476
};

// The player's rank badge in the tube's head (Sexy::RankBadge, 0x158 bytes)
class RankBadge : public TwistWidget {
public:
	bool mOver = false;            // +0x148: the mouse is over it
	GoodTube* mTube = nullptr;     // +0x14c
	float mTipTimer = 0;           // +0x150: steps of 0.01 while the mouse is over it; at 2 the next page
	int mTipPage = 0;              // +0x154: the tip's page (0 the rank, 1 the next rank's progress, 2 none)

	explicit RankBadge(GoodTube* theTube);                  // 0x701efb
	void Draw(Graphics* g) override;                        // 0x701f3a
	void Update() override;                                 // 0x70cbfa: the rank's tip on the tube while the mouse is over it
	void MouseEnter() override;                             // 0x701fbb (the base class' is not called)
	void MouseLeave() override;                             // 0x702007
	void MouseDown(int x, int y, int theClickCount) override;  // 0x70200f
	using TwistWidget::MouseDown;
	static int GetSize();                                   // 0x701fa5: its width and height
};

class GoodTube : public TwistWidget, public ButtonListener {
public:
	// +0x1a0: 16 of them, each a random phase (0 to 0.099)
	struct Bubble {
		float mPhaseStep = 0;
		float mPhase = 0;  // +0x4
	};

	// 0x34 bytes (0x701ea4): a speck of light rising in the tube (+0x1b4)
	struct Speck {
		float mX = 0;                  // +0x00
		float mY = 0;                  // +0x04
		float mDX = 0;                 // +0x08
		float mDY = 0;                 // +0x0c
		float mAlpha = 1.0f;           // +0x10
		float mDAlpha = 0.01f;         // +0x14 (0x3c23d70a)
		float mDDY = 0;                // +0x18
		Color mColor = Color::White;   // +0x1c
		float mScale = 1.0f;           // +0x2c
		Image* mImage = nullptr;       // +0x30
	};

	bool mShown = false;                 // +0x14c: passed on to +0x32c's +0x54 and +0x55 (0x702ad3)
	bool mLevelSoundOn = false;          // +0x14d: a new level's number comes with a sound; off in a challenge (0x702ac3)
	int mBestSoFar = 0;                  // +0x150: the highest of the board's +0x2634 so far
	Board* mBoard = nullptr;             // +0x154
	int mTubeKind = 0;                   // +0x158: 4 in a challenge (0x702b87), 0 in the other modes
	int mIsBlitzTube = 0;                // +0x15c: 1 in blitz (0x702bae)
	int mChallengeStyle = 0;             // +0x160: 1, 2 in a challenge (0x702bbe)
	int mGlassBottom = 0;                // +0x164: the glass's bottom (0x70318f)
	int mGlassX = 0;                     // +0x168: the glass's x (0x70318f)
	int mGlassHeight = 0;                // +0x16c: the glass's height over the bottom (0x7030ec)
	Image* mTopImage = nullptr;          // +0x178: the challenge's top in a challenge
	Image* mBottomImage = nullptr;       // +0x17c
	Image* mGlassImage = nullptr;        // +0x180
	Image* mFillImage = nullptr;         // +0x184
	Image* mFillImage2 = nullptr;        // +0x188
	float mTurnSlow = 0;                 // +0x18c: turns by 2 pi / 200 an update
	float mTurnSlower = 0;               // +0x190: turns by 2 pi / 400 an update
	float mFillTopShown = 0;             // +0x194: the fill's top as drawn, easing to mFillTopGoal
	float mFillTopGoal = 0;              // +0x198: where the fill's top should be
	float mProgress = 0;                 // +0x19c: how full, 0 to 1
	std::vector<Bubble*> mBubbles;       // +0x1a0
	int mSpeckWait = 0;                  // +0x1b0: updates to the next speck
	std::vector<Speck*> mSpecks;         // +0x1b4
	float mLightFade = 0;                // +0x1c4: runs down by 0.008 an update
	float mLevelChangeFlash = 0;         // +0x1c8: 1 when the level changes (0x702a88)
	// +0x1cc, +0x1d0: two text popups (0xd4 bytes, constructor 0x725732), made by 0x707103 and 0x707215; nothing in
	// the exe calls those two or holds their address, so the popups never exist
	void* mLevelPopup = nullptr;         // +0x1cc: a text popup that never exists (see above)
	void* mScorePopup = nullptr;         // +0x1d0: a text popup that never exists (see above)
	TextImage* mLevelText = nullptr;     // +0x1d4: "LEVEL" in FONT_INGAME (the exe's 0x74e701; nothing draws it)
	int mFlashBrightness = 0;            // +0x1d8: a flash's brightness, up to 255 (0x703431, 0x7033d4)
	int mGlowAlpha = 0;                  // +0x1dc: the glow's alpha, from +0x1d8
	float mFlashLevel = 0;               // +0x1e0: 0.5 with the flash
	float mLightFlash[16] = {};          // +0x1e4: each light's flash
	SexyString mTip;                     // +0x224: a tip text; empty (never set by ported code)
	int mTipWidth = 0;                   // +0x240: its width
	float mTipFade = 0;                  // +0x244: its fade
	float mTipTime = 0;                  // +0x248: how long the tip stays; runs down by 0.01 an update
	int mTipColor[4] = {};               // +0x24c: its colour
	int mLightsLit = 0;                  // +0x25c: the lights lit
	int mLightsLitBefore = 0;            // +0x260: and at the last update
	int mLastLightRow = 0;               // +0x264: the row the last speck's light was in
	int mFillBottom = 0;                 // +0x268: where the fill starts
	int mFillTop = 0;                    // +0x26c: where a full tube's fill ends
	TwistButton* mMenuButton = nullptr;  // +0x270 (MENU_BUTTON)
	int mChainSteps = 0;                 // +0x274: the chain's steps into the level (0x7033d4)
	float mShownSteps = 0;               // +0x278: the steps shown; 0 on a level up, the level's steps on a drop
	int mShownLevel = 1;                 // +0x27c: the level (the multiplier) shown
	int mLevelCel = 1;                   // +0x280: the same, for the level badge's cel
	int mLevel = 0;                      // +0x288 (0x702aa6)
	EffectsManager* mFXManager = nullptr;  // +0x28c
	float mEaseGoal = 0;                 // +0x290: +0x294 eases to it
	float mEaseValue = 0;                // +0x294: eases to mEaseGoal by 0.01 an update
	float mLevelFlash = 0;               // +0x29c: 1 when the level changes, halved by a drop
	int mLastX = 0;                      // +0x2a0: the widget's x at the last update
	float mSparkBurst = 0;               // +0x2a4: a burst of sparks while above 0
	float mGlowCut = 0;                  // +0x2ac: less of the glow's alpha is drawn as it grows
	float mTopSkew = 0;                  // +0x2b0: grows by mTopSkewStep when the falling top hits the floor
	float mTopSkewStep = 0;              // +0x2b4: added to mTopSkew each time
	float mGlassGone = 0;                // +0x2b8: how much of the glass is gone (the fill falling away)
	float mTopTurn = 0;                  // +0x2c0: the top's turn
	float mTopTurnSpeed = 0;             // +0x2c4: added to the top's turn each update
	float mTopFall = 0;                  // +0x2c8: how far the top has fallen
	float mTopFallSpeed = 0;             // +0x2cc: the top's falling speed, up by 0.075 an update
	float mTopBounce = 0;                // +0x2d0: how far the top went past the floor
	float mShardShare = 0;               // +0x2d8: the share of the glass the shards have reached
	float mRandomPhase = 0;              // +0x2e0: a random 0 to 1 made at construction, never read
	int mFirstLitRow = -1;               // +0x2e4: -1, never read
	int mLastLitRow = -1;                // +0x2e8: -1, never read
	std::vector<int> mShardRows;         // +0x2f0: filled by 0x70c7ec, which nothing in the exe calls (always empty)
	int mBreakState = 0;                 // +0x300: 1 while the glass breaks, 2 once the top has fallen out of the board; the fill is drawn full while above 0
	float mBaseStep = 1.0f;              // +0x308: updates a frame
	float mAddedSum = 0;                 // +0x30c: what has been added
	SexyString mText;                    // +0x310: the text the board gives it (0x7070f4)
	RankBadge* mRankBadge = nullptr;     // +0x32c
	float mBonusShown = 0;               // +0x330: 1 when a loaded game has a bonus challenge
	float mBonusFlash = 0;               // +0x334: set to 1 when the bonus is shown, runs down by 0.02 an update
	CurvedVal mBonusSwap;                // +0x338: the curve that swaps the bonus numbers
	int mBonusShownCount = 0;            // +0x3a8: the bonus count shown
	int mBonusTarget = 0;                // +0x3ac: the bonus count it is moving to (the board's +0x2660)
	int mBonusState = 0;                 // +0x3b0: the bonus display's state, 0 to 4
	float mBonusTimer = 0;               // +0x3b4: a countdown within a bonus state
	float mBonusIntro = 0;               // +0x3b8: set to 1 when a bonus starts
	int mBonusNumber = 0;                // +0x3bc: the number the bonus shows (the board's +0x2658)
	int mBonusStep = 0;                  // +0x3c0: 0 when a bonus starts, never read
	float mBonusFade = 0;                // +0x3c4: a fade that rises by 0.02 an update
	float mBonusSpin = 0;                // +0x3cc: a slow turn of the bonus icon
	std::vector<float> mBonusGlowTop = std::vector<float>(10, 0.0f);  // +0x3d0 (0x6d7130 its size): each bonus glow, the first list
	std::vector<float> mBonusGlowBottom = std::vector<float>(10, 0.0f);  // +0x3e0: each bonus glow, the second list
	float mBonusGlowFade = 1.0f;         // +0x3f0: runs down by 0.01 an update
	bool mBonusListSet = false;          // +0x3f4: the glow lists have been filled
	float mBonusSparkTimer = 0;          // +0x3f8: 8 and up by 4
	int mBonusStepsLeft = 0;             // +0x3fc: the steps to the bonus, less the board's +0x818 and 1
	CurvedVal mBlitzTimer;               // +0x400: set going by 0x707329
	ColorCycle mColorCycle;              // +0x470: white, blitz's colours by the time left (0x70c8b6)
	float mBlitzPulse = 0;               // +0x4a4: blitz's pulse, up by 0.05 and down by 0.03 an update
	bool mShowTenths = false;            // +0x4a8 (0x702bce): the time shows tenths
	bool mDrawTime = false;              // +0x4a9 (0x707329)
	int mTimeKind = 0;                   // +0x4ac: 1, or 4 (0x707329)
	float mTimePulseStep = 0;            // +0x4b0: 0, never read
	int mTimeLimit = 0;                  // +0x4b4: blitz's time (0x7036eb)
	int mTimeShown = 0;                  // +0x4b8: blitz's time, of mTimeFull
	int mTimeFull = 0;                   // +0x4bc (0x702b6a): blitz's 30000
	float mTimePulse = 0;                // +0x4c0
	bool mChallenge = false;             // +0x4c4 (0x70371c)
	CurvedVal mGoalMove;                 // +0x4c8: the challenge's goal moving
	int mGoalShown = 0;                  // +0x538: the challenge's goal shown
	// +0x53c to +0x56c: the challenge's goals as the pattern and on-board displays last got them (0x6db0ec, in
	// DrawChallenge): each rule's value now, its goal, its comparison and its stat
	std::vector<int> mRuleValues;        // +0x53c: each rule's value now
	std::vector<int> mRuleStats;         // +0x54c: each rule's stat
	std::vector<int> mRuleComparisons;   // +0x55c: each rule's comparison
	std::vector<int> mRuleGoals;         // +0x56c: each rule's goal
	CurvedVal mBestFade;                 // +0x580: the best's fade
	CurvedVal mBonusGlowCurve;           // +0x5f0: set by the board's bonus challenge (0x6bbfe4)
	Ticker mCountTicker;                   // +0x660: the board's +0x2638 (the challenge's count)
	Ticker mBestTicker;                   // +0x6e4: the board's +0x2634 (the challenge's best)
	bool mIsTubeEnabled = true;          // +0x764: true, never read
	bool mShowBestEver = false;          // +0x768: the night challenge's best-ever line is shown (never set)
	float mBestPulse = 0;                // +0x680: no ported code sets it
	float mBestFadeOut = 0;              // +0x700: +0x580's output in a challenge
	float mBestPulseOut = 0;             // +0x704 (the same as +0x680)

	explicit GoodTube(Board* theBoard);  // 0x707b7f
	~GoodTube() override;                // 0x707941, 0x704bfd

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x703582
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x702a4a
	void Update() override;                                             // 0x702736: UpdateTick, mBaseStep a frame
	void Draw(Graphics* g) override;                                    // 0x70a974
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x70623f
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x70318f
	using Widget::Resize;
	void DrawOverlay(Graphics* g) override;                             // 0x7049e5
	using Widget::DrawOverlay;
	void GotFocus() override;                                           // 0x7029ae
	void ButtonPress(int theId, int theClickCount) override;            // 0x7029c1
	using ButtonListener::ButtonPress;
	void ButtonDepress(int theId) override;                             // 0x7029d8
	virtual void UpdateTick();                                          // 0x70a9bb (slot 70, the tube's own)

	void SetShown(bool theShown);                  // 0x702ad3
	void SetLevelSoundOn(bool on) { mLevelSoundOn = on; }        // 0x702ac3
	void SetTubeKind(int theValue);              // 0x702b87
	void SetBlitzTube(int theValue) { mIsBlitzTube = theValue; }  // 0x702bae
	void SetChallengeStyle(int theValue) { mChallengeStyle = theValue; }  // 0x702bbe
	void SetChallenge(bool on);                    // 0x70371c
	void SetLevel(int theLevel) { mLevel = theLevel; }        // 0x702aa6
	void SetTimeFull(int theValue) { mTimeFull = theValue; }  // 0x702b6a
	void SetShowTenths(bool on) { mShowTenths = on; }         // 0x702bce
	bool IsIdle() { return mBonusTimer <= 0.0f && !(0.0f < mBestPulse) && !(0.0f < mBestPulseOut); }  // 0x702bde
	bool IsTimerStartedFull() { return mTimeKind == 4; }         // 0x702b7a
	void SetTime(int theTime, bool theAtOnce);     // 0x7036eb: blitz's time; theAtOnce shows it at once
	void StartTimer(bool theFull, bool theDrawTime);  // 0x707329
	void SetColor(const Color& theColor, bool theAtOnce);   // 0x70c8b6: the cycle runs from the last colour to it
	void SetTip(const SexyString& theText, const Color& theColor, float theSpeed, int theMaxWidth);  // 0x70c94a
	void UpdateImages();                           // 0x7030ec
	int GetTubeHeight();                           // 0x702af7: 0 until the images are set
	int GetTubeWidth();                            // 0x702b36: the wider light glow's width
	int GetGlassTop();                             // 0x702921
	int GetBottomTop();                            // 0x70295d
	int GetTopX();                                 // 0x7027fb: where the top image goes
	int GetBottomX();                              // 0x702837
	int GetTopY();                                 // 0x702873
	int GetScoreY();                               // 0x7028b7: the score's row
	void SetProgress(float theProgress, bool theAtOnce);  // 0x703731
	void UpdateFill(bool theAtOnce);               // 0x702c1b: the fill eases to the progress unless theAtOnce
	void MarkLevelChange() { mLevelFlash = 1.0f; mLevelChangeFlash = 1.0f; }  // 0x702a88
	void SetLevelSteps(int theSteps);              // 0x7033d4
	void ResetBonus();                             // 0x7035dc: a loaded game's bonus challenge starts afresh
	void StartBonus();                             // 0x703642: the bonus challenge's bar starts
	void SetLevelShown(int theLevel);              // 0x703431
	// 0x703334: where the level's light theStep is (its image's offset, its centre with theCentre)
	void GetStepPoint(int* thePoint, int theStep, bool theCentre);

	void DrawBonus(Graphics* g);                   // 0x703bdf: the bonus challenge's bar
	void DrawGlow(Graphics* g);                    // 0x704637: the top's door and glow
	void DrawFill(Graphics* g, bool theFront);     // 0x7051a4: the fill behind (false) or the ring in front (true)
	void DrawLight(Graphics* g, int theStep, float theFrac);  // 0x70675e: the light theStep, lit theFrac of the way
	void DrawBody(Graphics* g);                    // 0x7081c9: the top, the lights, the score and the level
	void DrawChallenge(Graphics* g, float theTipFade);  // part of 0x7081c9: the challenge's goals, count and best
	void DrawTime(Graphics* g, int theTime, int theX, int theY, const Color& theColor, const Color& theColorTop,
		float theScale, bool theHundredths);       // 0x704dde: blitz's clock
};

}  // namespace Sexy
