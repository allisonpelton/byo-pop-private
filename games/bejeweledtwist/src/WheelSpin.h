// Sexy::WheelSpin (0x13b0 bytes, a TwistWidget and ButtonListener; 0x759d99-0x75e877): the second chance when a
// bomb's counter reaches zero. The board zooms in on the bomb, a wheel of eight slots drops in (a skull for each
// wheel spun before in this game, the bomb's colour in the others), and the player stops it with the button. A skull
// under the pointer ends the game with the nuke, the bomb's colour defuses it (its counter goes back to 20 and every
// other counter on the board gains up to six). The outcome is decided when the wheel comes up (mLose): the button's
// press simulates the spin with ever later stops until one lands on the decided slot.
#pragma once

#include "ButtonListener.h"
#include "Color.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "TwistWidget.h"

#include <string>

namespace Sexy {

class Board;
class EffectsManager;
class Graphics;
class MemoryImage;
class Piece;
class SoundInstance;
class TwistButton;

class WheelSpin : public TwistWidget, public ButtonListener {
public:
	bool mLose;                     // +0x14c: the board's +0x21e0 was 1 or less (the original logs "FTL" or "FTW")
	MemoryImage* mCache = nullptr;  // +0x150: the screen behind the wheel, while the wheel stands (2D mode)
	Board* mBoard;                  // +0x154
	int mMouseX = 0;                // +0x158 (nothing reads it)
	int mMouseY = 0;                // +0x15c
	bool mPaused = false;           // +0x160: the board was paused by AddedToManager
	float mUnknown164 = 0;          // +0x164: rises to 1 (nothing reads it)
	int mUnknown168 = 0;            // +0x168
	bool mUnknown16C = false;       // +0x16c
	int mState = 0;                 // +0x170: see UpdateState
	float mAngle = 0;               // +0x174: the wheel's turn
	float mSpeed = 0;               // +0x178: added to mAngle each update
	float mTargetAngle = 0;         // +0x17c: where it settles after it stops
	float mBrake = 0;               // +0x180: how hard it slows, set by the simulation
	float mCoast = 0;               // +0x184: how long it keeps its speed after the press
	float mSavedAngle = 0;          // +0x188: the angle the simulation starts from
	float mUnknown18C = 0;          // +0x18c: a random turn (nothing reads it)
	int mCounter = 0;               // +0x190: counts updates in several states
	std::string mUnknown194;        // +0x194
	EffectsManager* mFXManager;     // +0x1b0: the sparkles and sparks
	float mUnknown1B4 = 0;          // +0x1b4: the skulls' share before this spin (nothing reads it)
	float mUnknown1B8 = 0;          // +0x1b8: and after it
	Piece* mPiece;                  // +0x1bc: the bomb
	float mGemScale = 1.0f;         // +0x1c0: the bomb's wobble after a win
	int mCenterX = 0;               // +0x1c4
	int mCenterY = 0;               // +0x1c8
	float mUnknown1CC = 0;          // +0x1cc
	float mUnknown1D0 = 1.0f;       // +0x1d0: the darkness round the hole, out of 255/200
	bool mSimulating = false;       // +0x1d4: ButtonDepress runs the spin ahead
	float mPanX = 0;                // +0x1d8: the board's shift that brings the bomb to the middle
	float mPanY = 0;                // +0x1dc
	CurvedVal mZoomIn;              // +0x1e0: the board's zoom on the bomb, and out again
	float mArms = 0;                // +0x250: the arms close round the wheel
	float mUnknown254 = 0;          // +0x254
	float mUnknown258 = 2.0f;       // +0x258: falls to 1 before the skulls are added
	float mUnknown25C = 0;          // +0x25c
	MTRand mRand;                   // +0x260: the simulation's brakes
	SoundInstance* mSiren;          // +0xc24
	SoundInstance* mUnknownC28 = nullptr;  // +0xc28: never set
	int mSlots[8];                  // +0xc2c: -1 a skull, -2 the slot picked, else a gem colour
	bool mSkullAdded[8];            // +0xc4c: the skull this spin added (drawn with mSkullAdd)
	bool mUnknownC54[8];            // +0xc54
	float mUnknownC5C[8];           // +0xc5c: falls to 0
	float mUnknownC7C[8];           // +0xc7c
	Color mSlotColors[8];           // +0xc9c
	Image* mLeftArm;                // +0xd1c
	Image* mRightArm;               // +0xd20
	int mUnknownD24 = 0;            // +0xd24: the arms' places (the 3D draw's)
	int mUnknownD28 = 0;            // +0xd28
	int mUnknownD2C;                // +0xd2c
	int mUnknownD30;                // +0xd30
	int mOppositeSlot = 0;          // +0xd34
	int mPickedSlot = 0;            // +0xd38: the slot under the pointer when it stopped
	float mGemGap = 0;              // +0xd3c: the bomb's two halves apart after a win
	float mWobble = 0;              // +0xd40
	float mWobbleAngle = 0;         // +0xd44
	int mResult = 0;                // +0xd48: 9 won, 13 lost
	float mUnknownD4C = 0;          // +0xd4c
	float mUnknownD50 = 0;          // +0xd50
	float mUnknownD58 = 4.0f;       // +0xd58
	float mUnknownD5C = 0;          // +0xd5c
	float mUnknownD60 = 0;          // +0xd60
	bool mUnknownD64 = false;       // +0xd64
	float mUnknownD68 = 1.0f;       // +0xd68: a slow pulse while it spins
	int mSkulls = 0;                // +0xd6c: the skulls on the wheel
	float mUnknownD70 = 1.0f;       // +0xd70
	float mUnknownD74 = 0;          // +0xd74
	float mUnknownD78 = 1.0f;       // +0xd78
	float mTickDelay = 0;           // +0xd7c: until the next timpani tick as it slows
	int mTicks = 0;                 // +0xd80
	CurvedVal mArrive;              // +0xd88: the arms and the wheel come in (1) and go (the others link to it)
	CurvedVal mUnknownDF8;          // +0xdf8: the button's and the bottom arm's drop
	CurvedVal mUnknownE68;          // +0xe68
	CurvedVal mGlow;                // +0xed8: the bomb's glow (its +0x29c), the sparks after a loss
	CurvedVal mUnknownF48;          // +0xf48
	CurvedVal mUnfold;              // +0xfb8: the slots fan out to their places
	CurvedVal mWheelAlpha;          // +0x1028
	CurvedVal mAlert;               // +0x1098: the alert ring's colour
	CurvedVal mIconAlpha;           // +0x1108
	CurvedVal mSkullAdd;            // +0x1178: a new skull's fade
	CurvedVal mSkullScale;          // +0x11e8: the picked skull's size after a loss
	CurvedVal mSkullColor;          // +0x1258
	CurvedVal mSirenVolume;         // +0x12c8
	CurvedVal mOpen;                // +0x1338: the hole opens round the wheel
	TwistButton* mButton = nullptr; // +0x13a8: "click to stop"

	WheelSpin(Board* theBoard, Piece* thePiece);  // 0x75c4de
	~WheelSpin() override;                        // 0x75c41c

	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x759db8
	using Widget::Resize;
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x75cd9c
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x759e1b
	void Update() override;                                             // 0x75e479
	void Draw(Graphics* g) override;                                    // 0x75a004
	void DrawOverlay(Graphics* g, int thePriority) override;            // 0x75a717
	using TwistWidget::DrawOverlay;
	void MouseMove(int x, int y) override;                              // 0x75c3ce
	void ButtonDepress(int theId) override;                             // 0x75e39a

	void Reset();        // 0x75cc55: the state AddedToManager starts from
	void StartSpin();    // 0x75c3eb: state 6, the button off
	// 0x75cdda: the wheel's states: 0 the board zooms in, 1 the wheel comes in, 2 to 4 the skulls are added, 5 spins,
	// 6 slows down, 7 settles, 8 the result; won: 9 and 10 the bomb splits and wobbles, 11 everything goes, 12 the
	// zoom out, 14 the counters count up; lost: 13, 11, 12, then 15 while the nuke goes off
	void UpdateState();
	void MakeCache();    // 0x75a02d: the screen behind the wheel into mCache
	// 0x7607d9: the screen darkened (theDark of 1) but for a hole about theX, theY, smaller as theSize grows
	void DrawHole(Graphics* g, int theX, int theY, float theDark, float theSize);
};

extern WheelSpin* gWheelSpin;  // 0x921688: the wheel while it is up

}  // namespace Sexy
