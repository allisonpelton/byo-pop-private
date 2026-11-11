#include "Torpedo.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the destructors): 0x402270 (deleting destructor of the GameObject part), 0x402280
// (HitsTarget), 0x4065a0 (SetState), 0x4098d0 (Draw), 0x40e0b0 (Update), 0x40e0c0 (TypeLetter) (jump to the functions
// below with the pointer adjusted), 0x409b10 (deleting destructor), 0x409b40 (destructor of the whole object), 0x401ae0
// (resets the vftables to the Torpedo's: the first step of the destructors of the classes derived from it)

// 0x40c1a0: Projectile (0x40acc0; GameObject 0x4611b0 and Enemy 0x40a3f0 first), then the Torpedo part. The Enemy
// constructor makes six places in mImages; this one makes it eight
Torpedo::Torpedo(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer) :
	Projectile(theApp, theHitTarget, thePlayer),
	mLaunchStep(0)
{
	mImages.resize(8, (Image*)NULL);
	mLaunchStep = 0;
	mImages[0] = mApp->mImages[IMAGE_TORPEDO_SWIM];
	mImages[2] = mApp->mImages[IMAGE_TORPEDO_EXPLODE];
	mImages[6] = mApp->mImages[IMAGE_TORPEDO_REVERSE];
	mImages[3] = mApp->mTorpedoSwimDim;
	mImages[7] = mApp->mImages[IMAGE_TORPEDO_EXPLODE];
	mFrameWidth = mImages[0]->GetWidth();
	mFrameHeight = mImages[0]->GetHeight();
	mFrameDelay = 4;
	mNumFrames = 14;
	mDamage = 10;
	mTick = 0;
	mVelX = 0.0;
}

Torpedo::~Torpedo()
{
}

// 0x401b90: a shot flying right (mVelX > 0) has hit its target when its box meets one of two boxes at the target's
// position. The first: the shot's right edge reaches the target's x + 25, the shot's x does not pass x + 25 + 193, the
// target's y + 53 does not pass the shot's bottom and the target's y + 153 reaches the shot's y. The second box is
// further right and lower: the target's x + 80 does not pass the shot's right edge, x + 155 reaches the shot's x, the
// target's y + 1 does not pass the shot's bottom and y + 46 reaches the shot's y
bool Torpedo::HitsTarget()
{
	if (mVelX > 0.0)
	{
		double aTargetX = mHitTarget->mX + 25.0;
		if ((double)mFrameWidth + mX >= aTargetX)
		{
			if (mX <= 193.0 + aTargetX)
			{
				if (mHitTarget->mY + 53.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 153.0 >= mY)
						return true;
				}
			}
		}
	}

	if (mVelX > 0.0)
	{
		if (mHitTarget->mX + 80.0 <= (double)mFrameWidth + mX)
		{
			if (mHitTarget->mX + 155.0 >= mX)
			{
				if (mHitTarget->mY + 1.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 46.0 >= mY)
						return true;
				}
			}
		}
	}
	return false;
}

// 0x40c3d0: the frame advances every mFrameDelay ticks (the tick count is that of the previous Update: Projectile::Update
// counts it). Past the last frame a dying torpedo (3) stays on frame 7 and pushes 6 and 5 once, a turning one (6)
// goes on flying back as state 1, any other starts again at frame 0. The launch flash counts up every fifth tick in
// states 0 and 4. Then Projectile::Update, then the speed of a returning torpedo (1) grows by 0.05 to 3.0, and states
// 0, 1, 4 and 6 move it by mVelX
void Torpedo::Update()
{
	if (mTick % mFrameDelay == 0)
	{
		mFrame++;
		if (mFrame < mNumFrames || mState == 3 || mState == 6)
		{
			if (mState == 3 && mFrame >= mNumFrames)
			{
				mFrame = 7;
				if (!mDeathFinished)
				{
					mDeathFinished = true;
					mEvents.push_back(6);
					mEvents.push_back(5);
				}
			}
			else if (mState == 6 && mFrame >= mNumFrames)
				SetState(1);
		}
		else
			mFrame = 0;
	}

	if ((mState == 0 || mState == 4) && mTick % 5 == 0)
	{
		mLaunchStep++;
		if (mLaunchStep > 8)
			mLaunchStep = 8;
	}

	Projectile::Update();

	if (mState == 1)
	{
		mVelX = mVelX + 0.05;
		if (mVelX >= 3.0)
			mVelX = 3.0;
	}

	if (mState == 0 || mState == 1 || mState == 6 || mState == 4)
		mX = mVelX + mX;
}

// 0x4085e0: the picture of the state at the last position (mLastX, mLastY, set by Projectile::Draw). Flying right (0, 4):
// the torpedo, the rotor 113 to the right (11 wide frames of the app's rotor picture) and, while the launch flash
// lasts, the flash 30 up and left; flying back (1): the mirrored torpedo and the app's darkened rotor; turning (6): the
// picture of the turn; exploding (3): the explosion picture drawn left of the position when the torpedo flies left,
// right of it when it flies right (nothing when it stands still). The word goes over a flying torpedo (0) in the
// projectile font, centred, in white at first: the typed part in green, the rest in black, 5 pixels below the middle
void Torpedo::Draw(Graphics* g)
{
	Projectile::Draw(g);

	if (mState == 0 || mState == 4)
	{
		g->DrawImage(mImages[0], (int)mLastX, (int)mLastY);
		Rect aRotor(mFrame * 11, 0, 11, mApp->mImages[IMAGE_ROTOR_TORPEDO]->GetHeight());
		g->DrawImage(mApp->mImages[IMAGE_ROTOR_TORPEDO], (int)(mLastX + 113.0), (int)mLastY, aRotor);
		if (mLaunchStep < 8)
		{
			Rect aFlash(mLaunchStep * 200, 0, 200, 100);
			g->DrawImage(mApp->mImages[IMAGE_TORPEDO_LAUNCH], (int)(mLastX - 30.0), (int)(mLastY - 30.0), aFlash);
		}
	}
	else if (mState == 1)
	{
		g->DrawImage(mImages[3], (int)mLastX, (int)mLastY);
		Rect aRotor(mFrame * 11, 0, 11, mApp->mImages[IMAGE_ROTOR_TORPEDO]->GetHeight());
		g->DrawImage(mApp->mRotorTorpedoDim, (int)mLastX, (int)mLastY, aRotor);
	}
	else if (mState == 6)
	{
		Rect aSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
		g->DrawImage(mImages[6], (int)mLastX, (int)mLastY, aSrc);
	}
	else if (mState == 3)
	{
		if (mVelX < 0.0)
		{
			Rect aSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
			g->DrawImage(mImages[2], (int)((mLastX - (double)mFrameWidth) + 20.0), (int)(mLastY - 55.0), aSrc);
		}
		else if (mVelX > 0.0)
		{
			Rect aSrc(mFrame * mFrameWidth, 0, mFrameWidth, mFrameHeight);
			g->DrawImage(mImages[7], (int)(mLastX + 23.0), (int)(mLastY - 55.0), aSrc);
		}
	}

	if (mState == 0)
	{
		g->SetFont(mProjFont);
		g->SetColor(Color(255, 255, 255));
		Board* aBoard = mApp->mBoard;
		if (aBoard == NULL || !aBoard->mPaused)
		{
			if (mNumTyped > 0)
			{
				g->SetColor(Color(0x11, 0xa1, 0x11));
				Font* aFont = g->GetFont();
				std::string aTyped = mWord.substr(0, mNumTyped);
				int aY = (int)((double)(mFrameHeight >> 1) + mLastY + 5.0);
				double aCenterX = (double)(mFrameWidth >> 1) + mLastX;
				int aTypedX = (int)(aCenterX - (double)(aFont->StringWidth(mWord) >> 1) - 5.0);
				g->DrawString(aTyped, aTypedX, aY);

				g->SetColor(Color(0, 0, 0));
				std::string aTyped2 = mWord.substr(0, mNumTyped);
				Font* aFont2 = g->GetFont();
				Font* aFont3 = g->GetFont();
				std::string aRemaining = mWord.substr(mNumTyped, mWord.size());
				int aY2 = (int)((double)(mFrameHeight >> 1) + mLastY + 5.0);
				int aTypedWidth = aFont2->StringWidth(aTyped2);
				double aRemainingX = (double)aTypedWidth + (double)(mFrameWidth >> 1) + mLastX;
				int aRemainingDrawX = (int)(aRemainingX - (double)(aFont3->StringWidth(mWord) / 2) - 5.0);
				g->DrawString(aRemaining, aRemainingDrawX, aY2);
			}
			else
			{
				g->SetColor(Color(0, 0, 0));
				Font* aFont = g->GetFont();
				int aY = (int)((double)(mFrameHeight >> 1) + mLastY + 5.0);
				int aWordX = (int)(((double)(mFrameWidth >> 1) + mLastX) - (double)(aFont->StringWidth(mWord) >> 1) - 5.0);
				g->DrawString(mWord, aWordX, aY);
			}
		}
	}
}

// 0x405080: Projectile::SetState, then the size of a frame in the picture of the new state. Exploding (3) plays the
// explosion sample (31) from 8 frames of the explosion picture, 3 ticks each (the original stores theState, which is 3,
// as the delay); turning (6) takes 6 frames of the turn picture; flying back (1) one picture of 14 frames the whole
// size of the mirrored torpedo's; flying right (4) 14 frames of the swim picture
void Torpedo::SetState(int theState)
{
	Projectile::SetState(theState);

	if (theState == 3)
	{
		mApp->PlaySample(SOUND_EXPLOSION);
		mFrameHeight = mImages[2]->GetHeight();
		mFrameWidth = mImages[2]->GetWidth() / 8;
		mFrame = 0;
		mFrameDelay = 3;
		mNumFrames = 8;
		return;
	}
	if (theState == 6)
	{
		mFrameHeight = mImages[6]->GetHeight();
		mFrameWidth = mImages[6]->GetWidth() / 6;
		mFrame = 0;
		mNumFrames = 6;
		return;
	}
	if (theState == 1)
	{
		mFrameHeight = mImages[3]->GetHeight();
		mFrameWidth = mImages[3]->GetWidth();
		mFrame = 0;
		mNumFrames = 14;
		return;
	}
	if (theState == 4)
	{
		mNumFrames = 14;
		mFrameWidth = mImages[0]->GetWidth() / 14;
		mFrameHeight = mImages[0]->GetHeight();
		mFrame = 0;
	}
}

// 0x40c590: the last letter does not count as typed (as in Enemy::TypeLetter): event 1 is pushed, the torpedo turns
// round (state 6) and its speed changes sign. mTyped and mRemaining stay as they are
void Torpedo::TypeLetter()
{
	mNumTyped++;
	if (mNumTyped == (int)mWord.size())
	{
		mNumTyped--;
		mEvents.push_back(1);
		SetState(6);
		mVelX = mVelX * -1.0;
	}
}
