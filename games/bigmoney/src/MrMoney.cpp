#include "MrMoney.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Compat12.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

// Animation table @0x4f6818 (.rdata, [50][50] steps of {frame, duration}); the steps after the
// {-1, 0} terminator are zero.
static const MrMoneyAnimStep gMrMoneyAnims[NUM_MRMONEY_ANIMS][MrMoney::NUM_ANIM_STEPS] =
{
	/*  0 */ {{0,3}, {1,7}, {4,9}, {1,10}, {2,8}, {-1,0}},
	/*  1 */ {{3,4}, {4,6}, {0,5}, {3,5}, {4,5}, {0,6}, {2,9}, {-1,0}},
	/*  2 */ {{2,2}, {1,8}, {2,2}, {4,7}, {3,6}, {0,5}, {2,13}, {-1,0}},
	/*  3 */ {{0,3}, {1,8}, {2,5}, {3,6}, {4,10}, {2,6}, {1,5}, {3,8}, {4,6}, {2,5}, {1,5}, {4,6}, {4,10}, {2,25}, {-1,0}},
	/*  4 */ {{2,3}, {1,5}, {2,2}, {3,11}, {2,6}, {-1,0}},
	/*  5 */ {{1,11}, {2,2}, {3,3}, {2,6}, {-1,0}},
	/*  6 */ {{2,3}, {1,15}, {3,3}, {2,6}, {-1,0}},
	/*  7 */ {{0,3}, {1,16}, {2,7}, {3,4}, {4,4}, {3,4}, {2,11}, {-1,0}},
	/*  8 */ {{0,1}, {2,3}, {1,17}, {4,7}, {2,14}, {3,7}, {4,15}, {0,6}, {2,25}, {-1,0}},
	/*  9 */ {{0,1}, {1,9}, {2,4}, {0,4}, {4,5}, {3,9}, {2,4}, {0,4}, {1,7}, {2,5}, {3,4}, {0,10}, {4,10}, {0,4}, {1,4}, {3,3}, {4,4}, {0,4}, {2,9}, {4,6}, {0,9}, {2,19}, {-1,0}},
	/* 10 */ {{1,1}, {2,4}, {1,10}, {2,4}, {3,5}, {4,7}, {0,5}, {2,16}, {-1,0}},
	/* 11 */ {{4,7}, {3,5}, {2,4}, {1,6}, {3,6}, {0,3}, {3,6}, {2,17}, {-1,0}},
	/* 12 */ {{3,3}, {1,8}, {3,4}, {4,9}, {3,3}, {0,3}, {2,4}, {1,11}, {2,43}, {-1,0}},
	/* 13 */ {{2,5}, {1,10}, {2,9}, {0,6}, {3,4}, {4,5}, {0,4}, {-1,0}},
	/* 14 */ {{2,4}, {3,4}, {4,3}, {1,15}, {4,3}, {0,4}, {2,21}, {-1,0}},
	/* 15 */ {{1,22}, {2,11}, {3,5}, {1,10}, {4,3}, {3,3}, {2,18}, {-1,0}},
	/* 16 */ {{2,4}, {1,8}, {2,9}, {3,5}, {4,6}, {2,7}, {0,4}, {3,5}, {1,6}, {0,5}, {2,22}, {-1,0}},
	/* 17 */ {{0,1}, {2,2}, {0,2}, {1,6}, {2,2}, {3,4}, {4,5}, {0,4}, {2,11}, {-1,0}},
	/* 18 */ {{2,8}, {1,9}, {2,5}, {3,6}, {1,10}, {2,7}, {1,7}, {4,9}, {3,10}, {0,7}, {2,3}, {-1,0}},
	/* 19 */ {{3,3}, {4,4}, {1,10}, {3,4}, {0,3}, {2,3}, {3,2}, {4,2}, {1,6}, {0,3}, {4,6}, {0,2}, {2,15}, {-1,0}},
	/* 20 */ {{2,2}, {1,4}, {3,2}, {2,2}, {1,5}, {0,3}, {2,2}, {1,5}, {3,4}, {0,2}, {2,5}, {3,7}, {0,2}, {2,19}, {-1,0}},
	/* 21 */ {{1,7}, {2,4}, {1,6}, {2,3}, {3,3}, {4,3}, {3,3}, {0,3}, {1,6}, {0,2}, {2,6}, {-1,0}},
	/* 22 */ {{2,3}, {1,6}, {0,3}, {3,3}, {0,3}, {2,7}, {-1,0}},
	/* 23 */ {{1,7}, {3,5}, {4,6}, {1,10}, {2,6}, {1,10}, {2,7}, {1,7}, {3,6}, {4,9}, {0,8}, {1,6}, {3,7}, {4,7}, {0,5}, {2,29}, {-1,0}},
	/* 24 */ {{5,5}, {-1,0}},
	/* 25 */ {{5,5}, {0,5}, {5,5}, {-1,0}},
	/* 26 */ {{5,5}, {2,82}, {5,5}, {-1,0}},
	/* 27 */ {{5,5}, {4,2}, {3,2}, {2,9}, {3,2}, {4,2}, {5,3}, {-1,0}},
	/* 28 */ {{3,8}, {4,13}, {5,8}, {-1,0}},
	/* 29 */ {{1,18}, {2,18}, {3,10}, {5,6}, {-1,0}},
	/* 30 */ {{3,6}, {2,20}, {1,11}, {3,14}, {5,5}, {-1,0}},
	/* 31 */ {{3,39}, {5,8}, {-1,0}},
	/* 32 */ {{4,3}, {1,6}, {4,2}, {5,4}, {-1,0}},
	/* 33 */ {{3,35}, {2,26}, {5,7}, {-1,0}},
	/* 34 */ {{3,55}, {2,45}, {5,7}, {0,6}, {5,5}, {0,19}, {2,48}, {5,6}, {3,5}, {3,3}, {-1,0}},
	/* 35 */ {{5,21}, {4,11}, {-1,0}},
	/* 36 */ {{4,3}, {3,3}, {2,26}, {3,3}, {4,3}, {5,4}, {-1,0}},
	/* 37 */ {{1,50}, {2,15}, {5,5}, {-1,0}},
	/* 38 */ {{5,5}, {0,3}, {5,4}, {0,3}, {5,4}, {-1,0}},
	/* 39 */ {{5,25}, {1,6}, {2,26}, {5,4}, {0,3}, {5,4}, {0,3}, {5,4}, {-1,0}},
	/* 40 */ {{2,119}, {4,2}, {0,2}, {4,2}, {5,4}, {0,3}, {5,4}, {-1,0}},
	/* 41 */ {{4,14}, {3,20}, {5,5}, {-1,0}},
	/* 42 */ {{5,3}, {0,2}, {5,3}, {0,2}, {5,3}, {0,2}, {5,3}, {3,28}, {-1,0}},
	/* 43 */ {{1,9}, {0,18}, {4,6}, {5,3}, {-1,0}},
	/* 44 */ {{3,55}, {5,5}, {0,3}, {5,4}, {0,3}, {5,3}, {-1,0}},
	/* 45 */ {{3,7}, {1,17}, {3,5}, {2,6}, {5,3}, {0,2}, {5,2}, {-1,0}},
	/* 46 */ {{5,15}, {0,20}, {5,16}, {-1,0}},
	/* 47 */ {{1,7}, {2,20}, {5,12}, {-1,0}},
	/* 48 */ {{5,29}, {0,3}, {5,11}, {0,4}, {5,7}, {0,5}, {5,5}, {0,2}, {5,4}, {0,6}, {5,2}, {-1,0}},
	/* 49 */ {{5,9}, {0,5}, {5,3}, {0,2}, {5,3}, {-1,0}},
};

// 0x415140
MrMoney::MrMoney(WinBMApp* theApp)
{
	mApp = theApp;
	mRot = 0.0;
	mPosX = 0.0;
	mPosY = 0.0;
	mVelX = 0.0;
	mVelY = 0.0;
	mTargetX = 0;
	mTargetY = 0;
	mAnimTick = 0;
	mAnimStep = 0;
	mFrame = 0;
	mMouseVisible = false;
	mHasAlpha = true;
	mHasTransparencies = true;
	mIdleWoke = true;
	mRotVel = 0.004;
	mAnim = -1;
}

// 0x4151f0
MrMoney::~MrMoney()
{
}

// 0x415200
void MrMoney::PlayAnim(int theAnim)
{
	mAnim = theAnim;
	mAnimTick = 0;
	mAnimStep = 0;
	mFrame = gMrMoneyAnims[theAnim][0].mFrame;
	MarkDirty();
}

// 0x415240
// Teleport: the spring target and position jump to the new place (the velocities are kept).
void MrMoney::Move(int theNewX, int theNewY)
{
	Widget::Move(theNewX, theNewY);
	mTargetX = mX;
	mTargetY = mY;
	mPosX = mX;
	mPosY = mY;
}

// 0x415290
void MrMoney::Update()
{
	Widget::Update();

	// Wake-up line after 120 s without input (once per idle stretch; mIdleWoke starts true, so the
	// first stretch after the title screen needs an input first).
	if ((unsigned int)(mWidgetManager->mUpdateCnt - mWidgetManager->mLastInputUpdateCnt) > IDLE_WAKE_TICKS)
	{
		if (!mIdleWoke)
		{
			mApp->PlaySample(SOUND_WAKEUP);
			PlayAnim(MRANIM_WAKEUP);
			mIdleWoke = true;
		}
	}
	else
		mIdleWoke = false;

	// Springs. The rotation swings with its own spring and follows the horizontal acceleration.
	mRot = mRot + mRotVel;
	mRotVel = mRotVel - mRot * 0.0004;

	double anAccelX = (mTargetX - mPosX) * 0.0025;
	double aVelX = (mVelX + anAccelX) * 0.94;
	mVelX = aVelX;
	mRot = mRot * 0.99 + anAccelX * 0.01;
	mVelY = (mVelY + (mTargetY - mPosY) * 0.0025) * 0.94;
	mPosX = aVelX + mPosX;
	mPosY = mVelY + mPosY;

	if ((mApp->mHasFocus && mUpdateCnt % 8 == 0) || fabs(aVelX) > 1.0 || fabs(mVelY) > 1.0)
		MarkDirtyFull();

	Resize((int)mPosX, (int)mPosY, mWidth, mHeight);

	if (mAnim != -1)
	{
		mAnimTick++;
		if (mAnimTick >= gMrMoneyAnims[mAnim][mAnimStep].mDuration * 2)
		{
			mAnimStep++;
			mAnimTick = 0;
			mFrame = gMrMoneyAnims[mAnim][mAnimStep].mFrame;
			if (mFrame == -1)
			{
				mAnim = -1;
				mFrame = 0;
			}
			MarkDirty();
		}
	}

	// Idle animations from the C runtime rand(): two draws every update, a third when the second hits.
	if (CrtRand() % 3000 == 0 && mAnim == -1)
		PlayAnim(MRANIM_BLINK);
	if (CrtRand() % 6000 == 0 && mAnim == -1)
		PlayAnim(CrtRand() % 25 + MRANIM_IDLE_FIRST);
}

// 0x4154d0
void MrMoney::Draw(Graphics* g)
{
	if (fabs(mRot) < 0.005)
		g->DrawImage(mApp->mMrMoneyImages[mFrame], 0, 0);
	else
	{
		// port: explicit int centre (1.2 rotates about (w/2, h/2) in ints, the port's NULL-rect overload
		// about (w/2.0f, h/2.0f)); the same (92, 68) for the 184x136 frames
		Image* anImage = mApp->mMrMoneyImages[mFrame];
		g->DrawImageRotated(anImage, 0, 0, mRot, anImage->GetWidth()/2, anImage->GetHeight()/2, NULL);
	}
}
