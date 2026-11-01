// Sexy::Effect (0xc0 bytes, 0x6f4a05-0x6f5a90) and Sexy::EffectsManager (a widget of 0xbc bytes, 0x6f55a7-0x6fa00c):
// the game's particles. The app keeps one manager over the whole screen (app +0xf78); the board has its own. An
// effect moves by its velocity, fades by mDAlpha and grows by mDScale each update; its type picks how it is set up,
// moves and is drawn.
#pragma once

#include "Color.h"
#include "Widget.h"

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

class EffectsManager : public Widget {
public:
	// The effects ordered by their type, each type in the order they were added
	std::multimap<int, Effect*> mEffects;  // +0x88
	bool mIs3D = false;                    // +0xa3: the last update ran with Direct3D
	float mAlpha = 1.0f;                   // +0xb8: every effect's opacity is multiplied by it

	EffectsManager();            // 0x6f9f59
	~EffectsManager() override;  // 0x6f86e7

	void Update() override;                  // 0x6f8792
	void Draw(Graphics* g) override;         // 0x6f7a45
	void DrawOverlay(Graphics* g) override;  // 0x6f80a4
	using Widget::DrawOverlay;

	// 0x6f555e: a new effect of the type, not yet added
	Effect* AllocEffect(int theType);
	// 0x6f875d
	void AddEffect(Effect* theEffect);
	// 0x6f6451: the effects whose mOverlay is theOverlay
	void DrawEffects(Graphics* g, bool theOverlay);
};

}  // namespace Sexy
