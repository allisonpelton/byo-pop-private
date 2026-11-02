// Sexy::GoodTube (0x770 bytes, vtable 0x7e3d64 with 71 slots, a TwistWidget and a ButtonListener; 0x702736-0x70a9bb):
// the tube beside the board that fills with the level's progress, with the level's number, the multiplier's arc of
// lights, the blitz clock, the bonus challenge's bar and the menu button. Being ported (docs/NOTES.md, "The board");
// its fields are named by their offset until their use is known.
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

// 0x84 bytes (0x707960 its constructor, 0x7073fc its update, 0x707476 its drawing): a number that counts up to a
// target and shows as a text (the challenge's count and its best)
struct Ticker {
	SexyString mText;                 // +0x00: the value as a text (0x7073be)
	float mAlpha = 1.0f;              // +0x1c
	float mFade = 0;                  // +0x20: the glow's strength; 1 while the value moves, then it fades
	int mTarget = 0;                  // +0x24
	int mValue = 0;                   // +0x28
	int mTimer = 0;                   // +0x2c: counts the updates
	SexyString mUnknown30;            // +0x30
	SexyString mUnknown4C;            // +0x4c
	Font* mUnknown68 = nullptr;       // +0x68: FONT_TOOLTIP
	Font* mUnknown6C = nullptr;       // +0x6c: FONT_TOOLTIP
	Font* mFont = nullptr;            // +0x70: FONT_SCORE
	Font* mGlowFont = nullptr;        // +0x74: FONT_SCOREGLOW
	Font* mSmallFont = nullptr;       // +0x78: FONT_SCORE_SMALL
	Font* mSmallGlowFont = nullptr;   // +0x7c: FONT_SCOREGLOW_SMALL
	bool mUnknown80 = false;          // +0x80: always the small font

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
	int mUnknown150 = 0;           // +0x150
	int mUnknown154 = 0;           // +0x154: 0 or 1, flipped by a click

	explicit RankBadge(GoodTube* theTube);                  // 0x701efb
	void Draw(Graphics* g) override;                        // 0x701f3a
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
		float mUnknown0 = 0;
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
	bool mUnknown14D = false;            // +0x14d: a new level's number comes with a sound; off in a challenge (0x702ac3)
	int mUnknown150 = 0;                 // +0x150: the highest of the board's +0x2634 so far
	Board* mBoard = nullptr;             // +0x154
	int mUnknown158 = 0;                 // +0x158: 4 in a challenge (0x702b87)
	int mUnknown15C = 0;                 // +0x15c: 1 in blitz (0x702bae)
	int mUnknown160 = 0;                 // +0x160: 1, 2 in a challenge (0x702bbe)
	int mUnknown164 = 0;                 // +0x164: the glass's bottom (0x70318f)
	int mUnknown168 = 0;                 // +0x168: the glass's x (0x70318f)
	int mUnknown16C = 0;                 // +0x16c: the glass's height over the bottom (0x7030ec)
	Image* mTopImage = nullptr;          // +0x178: the challenge's top in a challenge
	Image* mBottomImage = nullptr;       // +0x17c
	Image* mGlassImage = nullptr;        // +0x180
	Image* mFillImage = nullptr;         // +0x184
	Image* mFillImage2 = nullptr;        // +0x188
	float mUnknown18C = 0;               // +0x18c: turns by 2 pi / 200 an update
	float mUnknown190 = 0;               // +0x190: turns by 2 pi / 400 an update
	float mUnknown194 = 0;               // +0x194: the fill's top, easing to +0x198
	float mUnknown198 = 0;               // +0x198
	float mProgress = 0;                 // +0x19c: how full, 0 to 1
	std::vector<Bubble*> mBubbles;       // +0x1a0
	int mUnknown1B0 = 0;                 // +0x1b0: updates to the next speck
	std::vector<Speck*> mSpecks;         // +0x1b4
	float mUnknown1C4 = 0;               // +0x1c4
	float mUnknown1C8 = 0;               // +0x1c8: 1 when the level changes (0x702a88)
	void* mUnknown1CC = nullptr;         // +0x1cc: a text popup (0x707103, 0xd4 bytes, not ported; never made yet)
	void* mUnknown1D0 = nullptr;         // +0x1d0: the same (0x707215)
	TextImage* mLevelText = nullptr;     // +0x1d4: "LEVEL" in FONT_INGAME (the exe's 0x74e701; nothing draws it)
	int mUnknown1D8 = 0;                 // +0x1d8: a flash's brightness, up to 255 (0x703431, 0x7033d4)
	int mUnknown1DC = 0;                 // +0x1dc: the glow's alpha, from +0x1d8
	float mUnknown1E0 = 0;               // +0x1e0: 0.5 with the flash
	float mUnknown1E4[16] = {};          // +0x1e4: each light's flash
	SexyString mTip;                     // +0x224: a tip text; empty (never set by ported code)
	int mUnknown240 = 0;                 // +0x240: its width
	float mUnknown244 = 0;               // +0x244: its fade
	float mUnknown248 = 0;               // +0x248
	int mUnknown24C[4] = {};             // +0x24c: its colour
	int mUnknown25C = 0;                 // +0x25c: the lights lit
	int mUnknown260 = 0;                 // +0x260: and at the last update
	int mUnknown264 = 0;                 // +0x264
	int mFillBottom = 0;                 // +0x268: where the fill starts
	int mFillTop = 0;                    // +0x26c: where a full tube's fill ends
	TwistButton* mMenuButton = nullptr;  // +0x270 (MENU_BUTTON)
	int mUnknown274 = 0;                 // +0x274: the chain's steps into the level (0x7033d4)
	float mUnknown278 = 0;               // +0x278: 0 on a level up, the level's steps on a drop
	int mUnknown27C = 1;                 // +0x27c: the level (the multiplier) shown
	int mUnknown280 = 1;                 // +0x280: the same
	int mLevel = 0;                      // +0x288 (0x702aa6)
	EffectsManager* mFXManager = nullptr;  // +0x28c
	float mUnknown290 = 0;               // +0x290: +0x294 eases to it
	float mUnknown294 = 0;               // +0x294
	float mUnknown29C = 0;               // +0x29c: 1 when the level changes, halved by a drop
	int mUnknown2A0 = 0;                 // +0x2a0: the widget's x
	float mUnknown2A4 = 0;               // +0x2a4: a burst of sparks while above 0
	float mUnknown2AC = 0;               // +0x2ac
	float mUnknown2B0 = 0;               // +0x2b0
	float mUnknown2B4 = 0;               // +0x2b4
	float mUnknown2B8 = 0;               // +0x2b8: how much of the glass is gone (the fill falling away)
	float mUnknown2C0 = 0;               // +0x2c0: the top's turn
	float mUnknown2C4 = 0;               // +0x2c4
	float mUnknown2C8 = 0;               // +0x2c8: how far the top has fallen
	float mUnknown2CC = 0;               // +0x2cc
	float mUnknown2D0 = 0;               // +0x2d0
	float mUnknown2D8 = 0;               // +0x2d8
	float mUnknown2E0 = 0;               // +0x2e0: a random 0 to 1
	int mUnknown2E4 = -1;                // +0x2e4
	int mUnknown2E8 = -1;                // +0x2e8
	std::vector<int> mUnknown2F0;        // +0x2f0: filled when the glass breaks (0x70c7ec, not ported)
	int mUnknown300 = 0;                 // +0x300: the fill is drawn full while above 0
	float mUnknown308 = 1.0f;            // +0x308: updates a frame
	float mUnknown30C = 0;               // +0x30c: what has been added
	SexyString mText;                    // +0x310: the text the board gives it (0x7070f4)
	RankBadge* mRankBadge = nullptr;     // +0x32c
	float mUnknown330 = 0;               // +0x330: 1 when a loaded game has a bonus challenge
	float mUnknown334 = 0;               // +0x334
	CurvedVal mUnknown338;               // +0x338
	int mUnknown3A8 = 0;                 // +0x3a8
	int mUnknown3AC = 0;                 // +0x3ac
	int mUnknown3B0 = 0;                 // +0x3b0
	float mUnknown3B4 = 0;               // +0x3b4
	float mUnknown3B8 = 0;               // +0x3b8
	int mUnknown3BC = 0;                 // +0x3bc
	int mUnknown3C0 = 0;                 // +0x3c0
	float mUnknown3C4 = 0;               // +0x3c4
	float mUnknown3CC = 0;               // +0x3cc
	std::vector<float> mUnknown3D0 = std::vector<float>(10, 0.0f);  // +0x3d0 (0x6d7130 its size)
	std::vector<float> mUnknown3E0 = std::vector<float>(10, 0.0f);  // +0x3e0
	float mUnknown3F0 = 1.0f;            // +0x3f0
	bool mUnknown3F4 = false;            // +0x3f4
	float mUnknown3F8 = 0;               // +0x3f8
	int mUnknown3FC = 0;                 // +0x3fc
	CurvedVal mUnknown400;               // +0x400: set going by 0x707329
	ColorCycle mColorCycle;              // +0x470: white, blitz's colours by the time left (0x70c8b6)
	float mUnknown4A4 = 0;               // +0x4a4
	bool mUnknown4A8 = false;            // +0x4a8 (0x702bce)
	bool mUnknown4A9 = false;            // +0x4a9 (0x707329)
	int mUnknown4AC = 0;                 // +0x4ac: 1, or 4 (0x707329)
	float mUnknown4B0 = 0;               // +0x4b0
	int mUnknown4B4 = 0;                 // +0x4b4: blitz's time (0x7036eb)
	int mUnknown4B8 = 0;                 // +0x4b8: blitz's time, of mUnknown4BC
	int mUnknown4BC = 0;                 // +0x4bc (0x702b6a): blitz's 30000
	float mUnknown4C0 = 0;               // +0x4c0
	bool mChallenge = false;             // +0x4c4 (0x70371c)
	CurvedVal mUnknown4C8;               // +0x4c8: the challenge's goal moving
	int mUnknown538 = 0;                 // +0x538: the challenge's goal shown
	// port: +0x53c, +0x54c, +0x55c and +0x56c (four vectors the challenge's drawing fills) wait for the challenge
	CurvedVal mUnknown580;               // +0x580
	CurvedVal mUnknown5F0;               // +0x5f0: set by the board's bonus challenge (0x6bbfe4)
	Ticker mTicker660;                   // +0x660: the board's +0x2638 (the challenge's count)
	Ticker mTicker6E4;                   // +0x6e4: the board's +0x2634 (the challenge's best)
	bool mUnknown764 = true;             // +0x764
	bool mUnknown768 = false;            // +0x768
	float mUnknown680 = 0;               // +0x680 (not yet known; no ported code sets it)
	float mUnknown700 = 0;               // +0x700: +0x580's output in a challenge
	float mUnknown704 = 0;               // +0x704 (the same as +0x680)

	explicit GoodTube(Board* theBoard);  // 0x707b7f
	~GoodTube() override;                // 0x707941, 0x704bfd

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x703582
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x702a4a
	void Update() override;                                             // 0x702736: UpdateTick, mUnknown308 a frame
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
	void SetUnknown14D(bool on) { mUnknown14D = on; }        // 0x702ac3
	void SetUnknown158(int theValue);              // 0x702b87
	void SetUnknown15C(int theValue) { mUnknown15C = theValue; }  // 0x702bae
	void SetUnknown160(int theValue) { mUnknown160 = theValue; }  // 0x702bbe
	void SetChallenge(bool on);                    // 0x70371c
	void SetLevel(int theLevel) { mLevel = theLevel; }        // 0x702aa6
	void SetUnknown4BC(int theValue) { mUnknown4BC = theValue; }  // 0x702b6a
	void SetUnknown4A8(bool on) { mUnknown4A8 = on; }         // 0x702bce
	bool IsIdle() { return mUnknown3B4 <= 0.0f && !(0.0f < mUnknown680) && !(0.0f < mUnknown704); }  // 0x702bde
	bool IsUnknown4AC4() { return mUnknown4AC == 4; }         // 0x702b7a
	void SetTime(int theTime, bool theAtOnce);     // 0x7036eb: blitz's time; theAtOnce shows it at once
	void StartUnknown4AC(bool theFour, bool theUnknown4A9);  // 0x707329
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
	void MarkLevelChange() { mUnknown29C = 1.0f; mUnknown1C8 = 1.0f; }  // 0x702a88
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
	void DrawTime(Graphics* g, int theTime, int theX, int theY, const Color& theColor, const Color& theColorTop,
		float theScale, bool theHundredths);       // 0x704dde: blitz's clock
};

}  // namespace Sexy
