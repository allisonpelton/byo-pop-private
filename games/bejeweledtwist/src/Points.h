// Sexy::Points (0x1b8 bytes, vtable 0x7e9bf8, 0x7319e5-0x733033) and Sexy::PointsManager (0x158 bytes, vtable
// 0x7e9adc, 0x73248b-0x7329b7): the points that float up from a match. The board keeps one manager (+0x21fc) as a
// child widget; AddPoints makes a Points for a match, or adds to the one of the same serial (a chain of the same
// move), and recolours it by how big the points have grown.
#pragma once

#include "Color.h"
#include "ColorCycle.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Font;
class Graphics;
class WinTwistApp;

class Points {
public:
	float mX = 0;                    // +0x4: the text's centre
	float mY = 0;                    // +0x8
	float mVY = 0;                   // +0xc: upward, slowing as it nears the top
	bool mStopHigh = true;           // +0x10: it stops 75 (scaled) from the top
	int mFading = 0;                 // +0x14
	int mChainSum = 0;               // +0x18: the points times +0x2580, summed over a chain
	int mChainScaled = 0;            // +0x1c: the same scaled by the multiplier's 0.45th power
	WinTwistApp* mApp = nullptr;     // +0x28
	float mLife = 1.0f;              // +0x2c: falls by 0.01 an update; at 0 it fades
	SexyString mText;                // +0x30
	Font* mFont = nullptr;           // +0x4c
	int mFontHeight = 0;             // +0x50: 0, set by the constructor, never read
	int mLayerOffsets[5] = {};       // +0x54: five zeros set by the constructor, never read
	int mLayers = 0;                 // +0x68: see Points.cpp
	int mDelay = 0;                  // +0x6c: updates before it shows
	float mScale = 0;                // +0x70
	float mTargetScale = 0;          // +0x74
	float mScaleVel = 0;             // +0x78
	float mDamping = 0;              // +0x7c
	float mSpring = 0;               // +0x80
	ColorCycle mCycles[5];           // +0x84: the font's layers' colours
	bool mUseLastLayerCycle = false;  // +0x188: the last layer takes its colour from its cycle (false for every points text the port makes)
	int mUpdates = 0;                // +0x18c
	Color mColor;                    // +0x190
	float mPhase = 0;                // +0x1a0: turns by 0.03 of a circle an update (not drawn)
	float mFlash = 0;                // +0x1a4
	float mAlpha = 1.0f;             // +0x1a8
	int mSerial = -1;                // +0x1ac
	int mPoints = 0;                 // +0x1b0
	bool mDrawn = false;             // +0x1b4
	bool mDone = false;              // +0x1b5: the manager deletes it

	// 0x732ce7: theKind 0 to 2 give the second and third layers colours of their own; the board's floaters pass -1. The original's
	// also takes an int after theLife (0 from its one caller) and never reads it: left out
	Points(WinTwistApp* theApp, Font* theFont, const SexyString& theText, int theX, int theY, float theLife,
		const Color& theColor, int theKind);
	virtual ~Points() {}             // 0x732927, 0x7326cf: its body, inline here
	virtual void Update();           // 0x731d06
	virtual void Draw(Graphics* g);  // 0x7319fa
	void Flash() { mFlash = 0.3f; }  // 0x7319ed
};

class PointsManager : public TwistWidget {
public:
	std::vector<Points*> mPoints;    // +0x148

	PointsManager();                 // 0x7329b7
	~PointsManager() override;       // 0x732658, 0x7325ff: its loop that deletes the points, inline here
	void Update() override;          // 0x73248b
	void Draw(Graphics* g) override {}  // 0x731f61
	void DrawOverlay(Graphics* g, int thePriority) override;  // 0x732534
	void AddPoints(Points* thePoints) { mPoints.push_back(thePoints); }  // 0x733033
};

}  // namespace Sexy
