#ifndef __COSMICBUGS_CBOBJ_H__
#define __COSMICBUGS_CBOBJ_H__

// The base every game object starts with (0x40eac0..0x40f0f0), 0xb0 bytes. Objects of every kind sit in lists of
// Obj and are deleted through the base (0x407430 runs only the base's destructor), so the kinds that extend it
// add no members that need destroying.

#include "CBGame.h"

namespace cb
{

struct Obj
{
	Obj* mNext;                 // +0x00
	Obj* mPrev;                 // +0x04
	int mKind;                  // +0x08
	int mState;                 // +0x0c  2: fading out (Obj_Update)
	float mX;                   // +0x10
	float mY;                   // +0x14
	int mDepth;                 // +0x18  the draw list's
	float mOldX;                // +0x1c  before the last move
	float mOldY;                // +0x20
	float mVx;                  // +0x24
	float mVy;                  // +0x28
	float mSpeed;               // +0x2c  the game file's
	int mR;                     // +0x30  the draw list's colour and alpha
	int mG;                     // +0x34
	int mB;                     // +0x38
	int mAlpha;                 // +0x3c
	int mTint;                  // +0x40  0: the animation's colours, -1 or no animation: none, else tints[tint]
	Image* mImage;              // +0x44
	int mDrawKind;              // +0x48  the draw list's (0 blit .. 6 additive)
	int mDirty;                 // +0x4c  also marks the dirty grid
	float mAge;                 // +0x50  updates since made
	float mLifetime;            // +0x54  0: for ever
	List<Obj> mEmitters;        // +0x58  (deleted as objects too)
	List<Obj> mChildren;        // +0x64
	Animation* mAnim;           // +0x70
	int mFrame;                 // +0x74
	int mFrameCount;            // +0x78
	int mFrameTime;             // +0x7c  updates a frame
	int mFrameTimer;            // +0x80
	int mFrameStep;             // +0x84
	int mLoop;                  // +0x88  0: the last frame starts the fade out
	float mFade;                // +0x8c
	int mShadow;                // +0x90  a dark copy 5 pixels down and right, a depth below
	Animation* mDieAnim;        // +0x94  Obj_Explode plays it
	int mDieFrameCount;         // +0x98
	int mDieFrameTime;          // +0x9c
	int mOneShot;               // +0xa0  the animation's end (either way) finishes the object
	int mExploSound;            // +0xa4  -1: none
	int mAppearSound;           // +0xa8  a text's, played with a burst of bubbles as it fades in (0: none)
	int mMidHandle;             // +0xac  each frame is drawn about its middle
};

void Obj_Init(Obj* theObj);                                      // 0x40f060
void Obj_Destroy(Obj* theObj);                                   // 0x40f0f0
// 0x40ee80: 0 when the object is done (its lifetime is up, or a one-shot animation reached an end)
int Obj_Update(Obj* theObj);
// 0x40eac0: queues the object (and its shadow) and its children on the draw list
void Obj_DrawTo(Obj* theObj, uint8_t* theDst, int theDPitch);
void Obj_Draw(Obj* theObj);                                      // 0x40ec70: onto the screen
void Obj_UpdateEmitters(Obj* theObj);                            // 0x40ee40

// 0x40e3c0: the fields a copy of an object takes (not the links, lists, tint, age, frame step or loop)
void Obj_Copy(Obj* theObj, const Obj* theFrom);

// 0x40b7d0 (CBEmitter.cpp; its result, the new particle system, goes unused)
void Emitter_Update(Obj* theEmitter);

// The screen the game draws on (gScreenBits 0x46e3a4 and its pitch 0x46e3a0), set by the host each frame
extern uint8_t* gScreenBits;
extern int gScreenPitch;

} // namespace cb

#endif
