// Sexy::FruitAwardScreen (0x228 bytes, vtable 0x7e24c4, 0x6fd535-0x6fe15b): the fruit's award (Board::AwardFruit,
// 0x6ba5b4). It waits in the board's dialog queue; when it comes up the screen darkens and fruits fly from the tube to
// the gem picked for it, which then becomes the fruit (Board::MakeFruitGem), and the screen closes.
//
// Only state 4 runs: Init (0x6fd5ca) sets it and nothing sets another. States 0 to 3 (0x6fd7c6-0x6fd95c and
// 0x6fdb0f-0x6fdcaf: sparkles round a ring and down from mImage16C, a fade of the gem's colour and, in Draw, state 3's
// fonts) never run and are left out; so is mImage16C, which nothing sets.
#pragma once

#include "ColorCycle.h"
#include "TwistWidget.h"

namespace Sexy {

class Board;
class EffectsManager;

class FruitAwardScreen : public TwistWidget {
public:
	int mColor = 0;                  // +0x148: the gem's colour (the fruits' cel)
	float mUnknown14C = 0;           // +0x14c (states 2 and 3)
	float mUnknown150 = 0;           // +0x150 (state 2)
	float mUnknown154 = 0;           // +0x154: raised by state 2 only, so always 0
	Board* mBoard;                   // +0x158
	int mMouseX = 0;                 // +0x15c (nothing reads it)
	int mMouseY = 0;                 // +0x160
	float mDim = 0;                  // +0x164: the screen's darkness, falls by 0.03 an update
	int mState = 0;                  // +0x168
	Image* mImage16C = nullptr;      // +0x16c: never set
	float mUnknown170 = 0;           // +0x170 (states 0 and 1)
	int mUnknown178 = 0;             // +0x178: mImage16C's offset (state 1)
	float mUnknown17C = 0;           // +0x17c
	EffectsManager* mFXManager;      // +0x180: a child widget (nothing adds effects to it)
	int mCenterX = 0;                // +0x184
	int mCenterY = 0;                // +0x188
	int mFromX = 0;                  // +0x18c: the tube's top
	int mFromY = 0;                  // +0x190: its bottom while the fruit dance runs
	int mToX = 0;                    // +0x194: the gem's centre
	int mToY = 0;                    // +0x198
	float mFromScale = 0;            // +0x19c: the fruits' scale (doubled) at the tube
	float mToScale = 0;              // +0x1a0: and at the gem
	float mProgress = 0;             // +0x1a4: 0 to 1 by 0.015 an update
	float mUnknown1A8 = 0;           // +0x1a8
	float mUnknown1AC = 0;           // +0x1ac
	float mUnknown1B0 = 0;           // +0x1b0
	int mPieceId = 0;                // +0x1b4
	int mRow = 0;                    // +0x1b8: the gem's place when it was picked
	int mCol = 0;                    // +0x1bc
	ColorCycle mCycles[2];           // +0x1c0 (nothing draws them)

	explicit FruitAwardScreen(Board* theBoard);  // 0x6fe00d
	~FruitAwardScreen() override;                // 0x6fe0f8

	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6fd535
	using Widget::Resize;
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6fd6be
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6fd72b
	void Update() override;                                             // 0x6fd752
	void Draw(Graphics* g) override;                                    // 0x6fde55
	void MouseLeave() override {}                                       // 0x6fde51
	void MouseMove(int x, int y) override;                              // 0x6fde38
	void MouseDown(int x, int y, int theClickCount) override {}         // 0x6fde52
	using Widget::MouseDown;

	void SetPiece(int thePieceId);   // 0x6fd598: the gem's id and place
	void Init(int theColor);         // 0x6fd5ca: from AddedToManager, with the gem's colour
};

}  // namespace Sexy
