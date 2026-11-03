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
	int mUnknown2c;                      // +0x2c (-1; copied, not read here)
	int mUnknown30;                      // +0x30 (copied, not read here)
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
	bool mUnknown98;                     // +0x98
	bool mUnknown99;                     // +0x99

	Animator();                                  // 0x6ade80
	Animator(const Animator& theOther);          // 0x6adf30
	~Animator();                                 // 0x6adfd0
	Animator& operator=(const Animator& theOther);  // vf 0x64 (0x6ae200)

	// vf 0x04 (0x6ae300): one update; true when the cel changed (never for a single cel). theCountLoops: whether
	// a pass through the cels counts towards mMaxLoops
	bool Update(bool theCountLoops);
	void SetStopped(bool theStopped) { mStopped = theStopped; }      // vf 0x08 (0x6ae1f0)
	void Restart();                                                   // vf 0x0c (0x6ae6e0)
	void Init();                                                      // vf 0x10 (0x6ae040): the defaults
	void SetRange(int theStart, int theEnd);                          // vf 0x14 (0x6ae6a0)
	void SetDirection(int theDirection);                              // vf 0x18 (0x6ae6c0)
	void SetStopAfter(int theUpdates) { mStopAfter = theUpdates; mStopCounter = 0; }  // vf 0x1c (0x6add80)
	void SetAdditiveColor(const Sexy::Color& theColor);              // vf 0x20 (0x6adda0)
	void ClearAdditive() { mAdditive = false; }                       // vf 0x24 (0x6addd0)
	void SetColorizeColor(const Sexy::Color& theColor);              // vf 0x28 (0x6adde0)
	void ClearColorize() { mColorize = false; }                       // vf 0x2c (0x6ade10)
	void SetNumFrames(int theNumFrames);                              // vf 0x30 (0x6ae140)
	void SetImage(Sexy::Image* theImage);                             // vf 0x34 (0x6ae100): a cel a row or column
	// vf 0x38 (0x6ae9c0): at (mX + x, mY + y), at float positions when theFloat (the cel's part of the image),
	// else rounded
	void DrawF(Sexy::Graphics* g, float theX, float theY, bool theFloat);
	void Draw(Sexy::Graphics* g, int theX, int theY);                 // vf 0x3c (0x6ae7c0)
	void DrawStretched(Sexy::Graphics* g, float theX, float theY, int theWidth, int theHeight);  // vf 0x40 (0x6aeef0)
	void DrawScaled(Sexy::Graphics* g, float theX, float theY, float theScale);  // vf 0x44 (0x6aeca0)
	// vf 0x48 (0x6af100): turned by theRot radians about (theCenterX, theCenterY)
	void DrawRotated(Sexy::Graphics* g, float theX, float theY, float theRot, bool theFloat, float theCenterX,
					 float theCenterY);
	void SetRandomOrder(bool theRandom);                              // vf 0x4c (0x6af420)
	void SetMaxLoops(int theLoops) { mMaxLoops = theLoops; mLoopCount = 0; }  // vf 0x50 (0x6ade20)
	void SetFrame(int theFrame) { mFrame = theFrame; }                // vf 0x54 (0x6ae0f0)
	void SetFrameDelay(int theDelay, int theFrame);                   // vf 0x58 (0x6ae710): also mDelay
	void SetDelay(int theDelay);                                      // vf 0x5c (0x6ae750): every cel's
	bool TakeFrameChanged();                                          // vf 0x60 (0x6af410): read and cleared

private:
	void UpdateFade();                   // 0x6ae620
	int DrawCel() const { return mNumFrames == 1 ? 0 : mFrame; }
	Sexy::Rect CelSrcRect() const;
	void SetDrawColor(Sexy::Graphics* g) const;
};

}  // namespace JeffLib
