// Sexy::FlyingText (0x1f8 bytes, vtable 0x7e238c, 0x6fc3a9-0x6fd22c): a word that grows in the middle of the board
// and fades, such as the praise after a good move (0x6ba73a). The board keeps them at +0x1f98, updates the first one
// still waiting and the ones after it (0x6d090f), and draws them over everything (0x6b4a39). Kind 0 is an image;
// kind 1 flies in letters (+0x16c) that come with the texts that use them.
#pragma once

#include "ColorCycle.h"
#include "TwistWidget.h"

namespace Sexy {

class Image;

class FlyingText : public TwistWidget {
public:
	int mKind = 0;                   // +0x148: 0 an image, 1 letters
	int mSoundId = 0;                // +0x14c: a voice, said on the first update after the delay
	int mX = 0;                      // +0x150: the middle
	int mY = 0;                      // +0x154
	float mScale = 0.1f;             // +0x158
	float mFadeScale = 0.5f;         // +0x15c: from this scale it fades
	float mSlowScale = 0.5f;         // +0x160: from this scale it grows slowly
	float mFadeStep = -0.0075f;      // +0x164
	Image* mImageOverride = nullptr; // +0x198: its own, drawn in place of mTextImage when set
	float mTextAlpha = 1.0f;         // +0x19c
	float mDelay = 1.0f;             // +0x1a0: falls by 0.01 an update while mState is 0
	int mState = 0;                  // +0x1a4: 0 waiting, 1 growing, 2 growing slowly
	ColorCycle mCycle;               // +0x1a8: its colour while mCycling
	bool mCycling = false;           // +0x1dc
	Image* mTextImage = nullptr;     // +0x1e0
	bool mFading = false;            // +0x1e4: a click starts the fade at once
	bool mQueued = true;             // +0x1e5: the board leaves it alone until the ones before it are gone
	float mGrowFast = 0.04f;         // +0x1e8
	float mGrowSlow = 0.0025f;       // +0x1ec
	bool mSaid = false;              // +0x1f0: its voice is the app's one voice

	// 0x6fd22c: theX, theY below 0 take the window's middle, a delay of 0 or less none
	FlyingText(int theKind, Image* theImage, int theX, int theY, float theUnused, int theSoundId, float theDelay);
	~FlyingText() override;          // 0x6fccb4
	void Update() override;          // 0x6fcd48
	void Draw(Graphics* g) override; // 0x6fc840
	void MouseDown(int x, int y, int theClickCount) override;  // 0x6fc436
	using Widget::MouseDown;
};

}  // namespace Sexy
