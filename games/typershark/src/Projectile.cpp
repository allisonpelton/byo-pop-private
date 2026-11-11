#include "Projectile.h"
#include "WinWSApp.h"
#include "Graphics.h"

using namespace Sexy;

// Not translated (vtordisp thunks): 0x4021d0 (deleting destructor of the GameObject part), 0x40dfe0 (Update), 0x4021e0
// (Draw), 0x40dff0 (TypeLetter: Enemy's own, the thunk only finds the Enemy part), 0x4021f0 (SetState, a jump to 0x4017a0
// with the pointer adjusted), 0x401270 (resets the three vftables to the Projectile's: the first step of the destructors
// of the classes derived from it, nothing for the port)

// 0x40acc0: Enemy (0x40a3f0) first; the flags and the speed of the animation are the same for every projectile (a
// projectile has one frame, so it is not animated)
Projectile::Projectile(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer) :
	Enemy(theApp)
{
	mPlayer = thePlayer;
	mHitTarget = theHitTarget;
	mDamage = 0;	// port: the original leaves +0x14 uninitialised
	mSlot = 0;	// port: the original leaves +0x18 uninitialised
	mProjFont = mApp->mFontCourierFinalDraft15Bold;
	mNumFrames = 1;
	mFrameDelay = 11;
	mState = 0;
	mAlwaysTrueA = true;
	mAlwaysTrueB = true;
	mAlwaysTrueC = true;
}

Projectile::~Projectile()
{
}

// 0x401820: a shot flying left (mVelX < 0) has hit the player when the player is in state 9 and the shot's box meets
// a box at the player's position: in x the player's x + 29 reaches the shot's x and the shot's x + width reaches the
// player's x; in y the player's y + 4 does not pass the shot's y + height and the player's y + 78 reaches the shot's y
bool Projectile::HitsPlayer()
{
	if (mPlayer->mState == 9)
	{
		if (mVelX < 0.0)
		{
			if (mPlayer->mX + 29.0 >= mX)
			{
				if ((double)mFrameWidth + mX >= mPlayer->mX)
				{
					if (mPlayer->mY + 4.0 <= (double)mFrameHeight + mY)
					{
						if (mPlayer->mY + 78.0 >= mY)
							return true;
					}
				}
			}
		}
	}
	return false;
}

// HitsTarget (slot 1, 0x4018c0, a shot flying right has hit its target when the boxes overlap) is not translated: no play of the
// original reaches it, and Torpedo and the shots below it (CannonBall, MechaTorpedo, SquidCannonBall) each have their own, so
// Projectile leaves it pure.

// 0x40adf0: nothing while invisible. State 3 (hit) does nothing more, state 4 (a shot knocked back: the speed falls
// by 0.05 a tick) only watches the edges; any other state tests, in this order, the hit on the player (event 0x13,
// the state becomes 3 and the animation slows to 6), the hit on the target (event 0x14, state 3, unless the target is
// itself in state 3), the left edge (the whole shot gone: x <= -width) and the right edge (x >= the window's width);
// leaving pushes 0x15 and then 5
void Projectile::Update()
{
	if (!mVisible)
		return;

	IncTick();
	if (mState == 3)
		return;

	if (mState == 4)
	{
		mVelX = mVelX - 0.05;
		int aWidth = mFrameWidth;
		if (!((double)-aWidth < mX))
		{
			mEvents.push_back(0x15);
			mEvents.push_back(5);
			return;
		}
		if ((double)mApp->mWidth <= (double)aWidth + mX)
		{
			mEvents.push_back(0x15);
			mEvents.push_back(5);
		}
		return;
	}

	if (HitsPlayer())
	{
		mEvents.push_back(0x13);
		SetState(3);
		mFrameDelay = 6;
		return;
	}
	if (HitsTarget() && mHitTarget->mState != 3)
	{
		mEvents.push_back(0x14);
		SetState(3);
		return;
	}
	if (!((double)-mFrameWidth < mX))
	{
		mEvents.push_back(0x15);
		mEvents.push_back(5);
		return;
	}
	if ((double)mApp->mWidth <= mX)
	{
		mEvents.push_back(0x15);
		mEvents.push_back(5);
	}
}

// 0x4017f0: only the history of the position; a projectile draws nothing itself (Torpedo and the others draw first)
void Projectile::Draw(Graphics* g)
{
	if (mVisible)
		GameObject::Draw(g);
}

// 0x4017a0: state 3 (hit) plays a quicker animation from its first frame
void Projectile::SetState(int theState)
{
	mState = theState;
	if (theState == 3)
	{
		mFrameDelay = 5;
		mFrame = 0;
	}
}
