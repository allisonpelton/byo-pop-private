#include "BigLex.h"
#include "BookWormApp.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

// Animation table @0x4f9590 (.rdata, [1][50] steps of {image id, duration}). Image ids: 119
// lexbig_speak_ah, 120 lexbig_speak_ee, 121 lexbig_speak_mm; 0 = closed mouth (nothing drawn).
// lexbig_speak_oo / _pb (122, 123) are loaded but never used.
const LexAnimFrame Sexy::gBigLexAnims[NUM_BIGLEX_ANIMS][LEX_ANIM_STEPS] =
{
	/*  0 */ {{120,3}, {119,4}, {121,4}, {119,4}, {0,4}, {119,4}, {0,3}, {119,7}, {121,6}, {0,8}, {-1,0}},
};

// 0x402df0
BigLex::BigLex(BookWormApp* theApp)
{
	mApp = theApp;
	mAnim = -1;
	mFrame = -1;
	mAnimLoop = false;
	mAnimStep = 0;
	mAnimTick = 0;
	mDoFinger = false;
	mHasAlpha = true;
	mHasTransparencies = true;
}

// 0x402e30
BigLex::~BigLex()
{
}

// 0x402e40
void BigLex::PlayAnimSeq(int theAnim, bool theLoop)
{
	mAnim = theAnim;
	mAnimStep = 0;
	mAnimTick = 0;
	mFrame = gBigLexAnims[theAnim][0].mImage;
	mAnimLoop = theLoop;
	MarkDirty();
}

// 0x402e80 slot 16
void BigLex::Draw(Graphics* g)
{
	if (mFrame > 0)
		g->DrawImage(mApp->mImages[mFrame], 0, 0);
}

// 0x402eb0 slot 17
// As Lex's animation step: a step of duration d shows for 2d updates. At the end of the row mAnim is -1
// and mFrame stays -1 for one update, then 0.
void BigLex::Update()
{
	Widget::Update();
	if (mAnim == -1)
	{
		mFrame = 0;  // no MarkDirty
		return;
	}

	mAnimTick++;
	if (mAnimTick >= gBigLexAnims[mAnim][mAnimStep].mDuration * 2)
	{
		mAnimStep++;
		mAnimTick = 0;
		mFrame = gBigLexAnims[mAnim][mAnimStep].mImage;
		if ((mFrame == -1) && mAnimLoop)
		{
			mAnimStep = 0;
			mFrame = gBigLexAnims[mAnim][0].mImage;
		}
		if (mFrame == -1)
			mAnim = -1;
		MarkDirty();
	}
}
