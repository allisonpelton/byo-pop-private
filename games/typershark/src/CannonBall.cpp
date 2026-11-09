#include "CannonBall.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the destructors): 0x40e5b0 (HitsTarget), 0x40e5c0 (SetState), 0x40e5d0 (Draw)
// (jump to the functions below with the pointer adjusted), 0x40ebb0 (deleting destructor), 0x40f310 (destructor of the
// whole object; both are shared with MechaTorpedo and SquidCannonBall by the linker)

// 0x40e400: Torpedo (0x40c1a0) first. The frame size stays that of the torpedo's swim picture, which the Torpedo
// constructor took
CannonBall::CannonBall(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer) :
	Torpedo(theApp, theHitTarget, thePlayer)
{
	mApp = theApp;
	mImages[0] = mApp->mImages[IMAGE_CANNONBALL];
	mImages[6] = mApp->mImages[IMAGE_CANNONBALL_TURN];
	mImages[3] = mApp->mCannonballDim;
	mNumFrames = 12;
}

CannonBall::~CannonBall()
{
}

// 0x401370: a shot flying right (mVelX > 0) has hit its target when the target's x + 25 does not pass the shot's right
// edge, the target's x + 159 reaches the shot's x, the target's y + 33 does not pass the shot's bottom and the target's
// y + 324 reaches the shot's y
bool CannonBall::HitsTarget()
{
	if (mVelX > 0.0)
	{
		if (mHitTarget->mX + 25.0 <= (double)mFrameWidth + mX)
		{
			if (mHitTarget->mX + 159.0 >= mX)
			{
				if (mHitTarget->mY + 33.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 324.0 >= mY)
						return true;
				}
			}
		}
	}
	return false;
}

// 0x407a90: like Torpedo::Draw, with the ball's pictures: a flying ball (0, 4) is the frame mFrame of 74x44 pixels of
// the first picture (frames 44 pixels apart) at the last position, with the launch flash while it lasts; the same frame
// of the mirrored picture (1) and of the turn picture (6); an exploding ball (3) as the torpedo's explosion. The word
// of a flying ball (0) is drawn in the projectile font, left aligned 18 to the right of the position and 8 below the
// middle, the typed part green, the rest black after it and 5 below the middle
void CannonBall::Draw(Graphics* g)
{
	Projectile::Draw(g);

	if (mState == 0 || mState == 4)
	{
		Rect aSrc(mFrame * 44, 0, 74, 44);
		g->DrawImage(mImages[0], (int)mLastX, (int)mLastY, aSrc);
		if (mLaunchStep < 8)
		{
			Rect aFlash(mLaunchStep * 200, 0, 200, 100);
			g->DrawImage(mApp->mImages[IMAGE_TORPEDO_LAUNCH], (int)(mLastX - 30.0), (int)(mLastY - 30.0), aFlash);
		}
	}
	else if (mState == 1)
	{
		Rect aSrc(mFrame * 44, 0, 74, 44);
		g->DrawImage(mImages[3], (int)mLastX, (int)mLastY, aSrc);
	}
	else if (mState == 6)
	{
		Rect aSrc(mFrame * 44, 0, 74, 44);
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
				std::string aTyped = mWord.substr(0, mNumTyped);
				int aY = (int)((double)(mFrameHeight >> 1) + mLastY + 8.0);
				int aX = (int)(mLastX + 18.0);
				g->DrawString(aTyped, aX, aY);

				g->SetColor(Color(0, 0, 0));
				std::string aTyped2 = mWord.substr(0, mNumTyped);
				Font* aFont = g->GetFont();
				std::string aRemaining = mWord.substr(mNumTyped, mWord.size());
				int aY2 = (int)((double)(mFrameHeight >> 1) + mLastY + 5.0);
				int aRemainingX = (int)((double)aFont->StringWidth(aTyped2) + mLastX + 18.0);
				g->DrawString(aRemaining, aRemainingX, aY2);
			}
			else
			{
				g->SetColor(Color(0, 0, 0));
				int aY = (int)((double)(mFrameHeight >> 1) + mLastY + 8.0);
				int aX = (int)(mLastX + 18.0);
				g->DrawString(mWord, aX, aY);
			}
		}
	}
}

// 0x404030: Projectile::SetState, then the size of a frame in the picture of the new state, as in Torpedo::SetState with
// 5 frames for the turn and 12 for the ball. Flying right (4) takes the whole width of the picture and a twelfth of
// its height as the frame (the machine code divides the height here, where Torpedo::SetState divides the width)
void CannonBall::SetState(int theState)
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
		mNumFrames = 5;
		return;
	}
	if (theState == 1)
	{
		mFrameHeight = mImages[3]->GetHeight();
		mFrameWidth = mImages[3]->GetWidth();
		mFrame = 0;
		mNumFrames = 12;
		return;
	}
	if (theState == 4)
	{
		mNumFrames = 12;
		mFrameWidth = mImages[0]->GetWidth();
		mFrameHeight = mImages[0]->GetHeight() / mNumFrames;
		mFrame = 0;
	}
}
