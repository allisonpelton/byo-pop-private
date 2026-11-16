// JeffLib::Animator (0x9c bytes, code 0x6add80-0x6af4b0): plays an image strip's cels, forwards, backwards, to and
// fro or in a random order, each cel for a number of updates, optionally a number of times, within a range of cels,
// tinted (additively or by colourizing) and faded in and out. The game draws its portraits and many small
// animations with it.
#pragma once

#include <vector>

#include "Color.h"
#include "Rect.h"

namespace Sexy {
class Graphics;
class Image;
}

namespace JeffLib {

class Animator {
public:
	// +0x74: the alpha's pulse: down to mMin and up to mMax, mCount changes of direction (each call of Update),
	// after which it stops (and the animation is done, if mDoneWhenFinished)
	struct Fade {
		int mState = 0;                  // +0x00: 0 none, 1 fading down, 2 fading up
		int mDownSpeed = 0;              // +0x04
		int mUpSpeed = 0;                // +0x08
		int mMin = 0;                    // +0x0c
		int mMax = 0;                    // +0x10
		int mAlpha = 0;                  // +0x14
		int mCount = 0;                  // +0x18
		bool mDoneWhenFinished = true;   // +0x1c
	};

	int mUpdateCount;                    // +0x04
	int mFrame;                          // +0x08
	int mNumFrames;                      // +0x0c
	int mDelay;                          // +0x10: updates a cel
	int mMaxLoops;                       // +0x14: 0 for no end
	int mLoopCount;                      // +0x18
	int mRangeStart;                     // +0x1c
	int mRangeEnd;                       // +0x20
	int mDirection;                      // +0x24: below 0 backwards
	int mStep;                           // +0x28: cels a change
	int mStopAfter;                      // +0x34: updates until it stops, 0 or less for never
	int mStopCounter;                    // +0x38
	float mX;                            // +0x3c
	float mY;                            // +0x40
	bool mStopped;                       // +0x44
	bool mPingPong;                      // +0x45
	bool mGoingBack;                     // +0x46
	bool mOnce;                          // +0x47
	bool mDone;                          // +0x48
	bool mFrameChanged;                  // +0x49
	bool mUseRange;                      // +0x4a
	bool mAdditive;                      // +0x4b
	bool mColorize;                      // +0x4c
	bool mRandom;                        // +0x4d
	Sexy::Color mAdditiveColor;          // +0x50
	Sexy::Color mColorizeColor;          // +0x60
	Sexy::Image* mImage;                 // +0x70
	Fade* mFade = nullptr;               // +0x74
	std::vector<int> mFrameDelays;       // +0x78: a delay a cel
	std::vector<int> mRandomOrder;       // +0x88: the cels not yet shown this loop

	Animator();                                  // 0x6ade80
	Animator(const Animator&) = delete;  // 0x6adf30 and operator= (vf 0x64, 0x6ae200): nothing copies one
	~Animator();                                 // 0x6adfd0
	Animator& operator=(const Animator&) = delete;

	// vf 0x04 (0x6ae300): one update; true when the cel changed (never for a single cel). theCountLoops: whether
	// a pass through the cels counts towards mMaxLoops
	bool Update(bool theCountLoops);
	void SetStopped(bool theStopped) { mStopped = theStopped; }      // vf 0x08 (0x6ae1f0)
	void Init();                                                      // vf 0x10 (0x6ae040): the defaults
	void SetNumFrames(int theNumFrames);                              // vf 0x30 (0x6ae140)
	void SetImage(Sexy::Image* theImage);                             // vf 0x34 (0x6ae100): a cel a row or column
	void Draw(Sexy::Graphics* g, int theX, int theY);                 // vf 0x3c (0x6ae7c0)
	void SetDelay(int theDelay);                                      // vf 0x5c (0x6ae750): every cel's
	// The original's other virtuals (Restart, SetRange, SetDirection, the tints, the other draws, SetRandomOrder,
	// SetStopAfter, SetMaxLoops, SetFrame, SetFrameDelay, TakeFrameChanged) have no caller; left out (README,
	// "Unneeded code"). Update and the draw keep the original's branches for what only they turned on

private:
	void UpdateFade();                   // 0x6ae620
	int DrawCel() const { return mNumFrames == 1 ? 0 : mFrame; }
	void SetDrawColor(Sexy::Graphics* g) const;
};

}  // namespace JeffLib
