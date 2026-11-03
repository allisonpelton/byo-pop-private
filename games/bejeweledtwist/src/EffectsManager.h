// Sexy::Effect (0xc0 bytes, 0x6f4a05-0x6f5a90) and Sexy::EffectsManager (a widget of 0xbc bytes, 0x6f55a7-0x6fa00c):
// the game's particles. The app keeps one manager over the whole screen (app +0xf78); the board has its own. An
// effect moves by its velocity, fades by mDAlpha and grows by mDScale each update; its type picks how it is set up,
// moves and is drawn.
#pragma once

#include "Color.h"
#include "TwistD3D.h"
#include "Widget.h"

#include <list>
#include <map>

namespace Sexy {

class EffectsManager;
class Graphics;
class Image;

class Effect {
public:
	enum {
		// The scale bounces at mMaxScale instead of stopping there
		FLAG_SCALE_BOUNCE = 0x01,
		// The alpha bounces at mMaxAlpha instead of ending the effect there
		FLAG_ALPHA_BOUNCE = 0x02,
		// The effect waits mDelay before it fades or grows
		FLAG_DELAY = 0x04,
	};

	int mType;                    // +0x04
	bool mOverlay = false;        // +0x08: drawn in the manager's overlay pass
	float mX = 0;                 // +0x0c
	float mY = 0;                 // +0x10
	float mDX = 0;                // +0x14
	float mDY = 0;                // +0x18
	int mPieceId = -1;            // +0x1c: drawn relative to this piece of the board
	float mDXScalar = 1.0f;       // +0x20: mDX is multiplied by it each update
	float mDYScalar = 1.0f;       // +0x24
	float mGravity = 0;           // +0x28: added to mDY each update
	int mFrame = 0;               // +0x2c: the image's cel
	float mDelay = 0;             // +0x30
	float mUnknown38 = 0;         // +0x38: the shards' damping of their velocity
	float mAlpha = 1.0f;          // +0x3c
	float mDAlpha = -0.01f;       // +0x40
	float mMaxAlpha = 1.0f;       // +0x44
	float mScale = 1.0f;          // +0x48
	float mDScale = 0;            // +0x4c
	float mMinScale = 0;          // +0x50: below it the effect ends
	float mMaxScale = 100.0f;     // +0x54
	Color mColor;                 // +0x58
	Color mColor2;                // +0x68
	float mAngle = 0;             // +0x78
	float mDAngle = 0;            // +0x7c
	float mUnknown80 = 0;         // +0x80
	float mUnknown84 = 0;         // +0x84
	Image* mImage = nullptr;      // +0x88
	int mUpdateDiv = 1;           // +0x8c: moves by a 1/mUpdateDiv of its velocity, changes every mUpdateDiv updates
	bool mIsCyclingColor = false; // +0x90: the hue turns
	int mCurHue = 0;              // +0x94
	bool mIsAdditive = false;     // +0x98
	// +0x9c to +0xa8: type 26's speed below which it fades, that fade and its damping; the shards' two tumbling
	// angles and their turns
	float mUnknown9C = 0;         // +0x9c
	float mUnknownA0 = 0;         // +0xa0
	float mUnknownA4 = 0;         // +0xa4
	float mUnknownA8 = 0;         // +0xa8
	int mFlags = 0;               // +0xb0
	bool mDeleteMe = false;       // +0xb4
	EffectsManager* mFXManager = nullptr;  // +0xb8
	bool mOwnsImage = false;      // +0xbc: mImage is deleted with the effect

	explicit Effect(int theType);  // 0x6f4a05
	virtual ~Effect();             // 0x6f553e
	// Type 44 (an effect class of its own) moves and draws itself; the base does nothing
	virtual void Update() {}       // 0x6f553a
	virtual void Draw(Graphics* g) {}  // 0x6f553b
};

// A heat wave (Direct3D only): a ring of distortion that spreads from a point (0x40 bytes in the manager's tree at
// +0x94, made by 0x6f5a46, added by 0x6f9efe, aged by 0x6f8792, drawn by 0x6f7ab4)
struct HeatWave {
	int mUnknown00 = 0;           // +0x00
	double mX = 0;                // +0x08
	double mY = 0;                // +0x10
	double mEndX = 0;             // +0x18: where the ring ends (0x6f7ab4 moves its box by progress times it)
	double mEndY = 0;             // +0x20
	float mGrowth = 0.025f;       // +0x28: added to mProgress each update (0x6f5a46: 0.025; 0x6f9efe halves it)
	float mOpacity = 1.0f;        // +0x2c: its opacity in 0x6f7ab4
	float mStartSize = 0;         // +0x30
	float mEndSize = 0;           // +0x34: its size at the end: the app's scale times 400 (0x6f5a46), times the strength
	float mProgress = 0;          // +0x38: the wave is gone once it passes 1.0
};

class EffectsManager : public Widget {
public:
	// The effects ordered by their type, each type in the order they were added
	std::multimap<int, Effect*> mEffects;  // +0x88
	std::list<HeatWave> mHeatWaves;        // +0x94 (a tree of them in the original, all alike in order of adding)
	bool mUnknownA0 = false;               // +0xa0: drawn under the board's transform (0x6f7a45)
	bool mUnknownA1 = false;               // +0xa1: the depth test goes off first (0x6f6451)
	bool mIs3D = false;                    // +0xa3: the last update ran with Direct3D
	bool mUnknownB4 = false;               // +0xb4: set once a heat wave has been drawn (0x6f80a4)
	bool mUnknownB5 = false;               // +0xb5
	bool mUnknownB6 = false;               // +0xb6: set by the board's constructor for the first manager, read by nothing
	DDImage* mHeightImage = nullptr;       // +0xb0: the heat waves' height image (a half-size target), 0 in 2D mode
	TargetHold mHeightHold;                // port: the pool's hold of it (the original's is a hidden global, never released)
	float mAlpha = 1.0f;                   // +0xb8: every effect's opacity is multiplied by it

	EffectsManager();            // 0x6f9f59
	~EffectsManager() override;  // 0x6f86e7

	void Update() override;                  // 0x6f8792
	void Draw(Graphics* g) override;         // 0x6f7a45
	void DrawOverlay(Graphics* g) override;  // 0x6f80a4
	using Widget::DrawOverlay;

	void Clear();  // 0x6f63ee: every effect deleted
	// 0x6f555e: a new effect of the type, not yet added
	Effect* AllocEffect(int theType);
	// 0x6f875d
	void AddEffect(Effect* theEffect);
	// 0x6fa00c: a bomb going off at (theX, theY): smoke, drips, steam and shards of theColor
	void AddBombFlash(float theX, float theY, const Color& theColor);
	// 0x6f9efe: a heat wave at (theX, theY), its +0x34 times theStrength, its +0x28 times 0.5
	HeatWave* AddHeatWave(float theX, float theY, float theStrength);
	void MakeHeightImage();                    // 0x6f5ea6
	DDImage* GetHeightImage();                 // 0x6f5efc: made if need be, and +0xb4 set
	void ClearHeightImage(Graphics* g);        // 0x6f55b0: grey, then drawing goes back to g's image
	void DrawDistortion(Graphics* g);          // 0x6f7ab4
	// 0x6f6451: the effects whose mOverlay is theOverlay
	void DrawEffects(Graphics* g, bool theOverlay);
};

}  // namespace Sexy
