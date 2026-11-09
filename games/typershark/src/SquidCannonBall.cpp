#include "SquidCannonBall.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the destructors): 0x40f2a0 (HitsTarget), 0x40f2b0 and 0x40f2c0 (SetState, to
// CannonBall's 0x404030 with the pointer adjusted), 0x40f2e0 and 0x40f2f0 (Draw, to CannonBall's 0x407a90), 0x40ebb0
// (deleting destructor, shared with CannonBall), 0x40f310 (destructor of the whole object)

// 0x40f150: CannonBall (0x40e400) first; nothing else
SquidCannonBall::SquidCannonBall(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer) :
	CannonBall(theApp, theHitTarget, thePlayer)
{
}

SquidCannonBall::~SquidCannonBall()
{
}

// 0x401590: a shot flying right (mVelX > 0) has hit its target when the boxes meet in one of two ways. The first: the
// target's x + 32 does not pass the shot's right edge, the target's x + 132 reaches the shot's x, the shot's bottom
// reaches the target's y and the target's y + 144 reaches the shot's y. The second, further right and lower: the
// target's x + 120 does not pass the shot's right edge, the target's right edge (x + GetFrameWidth) reaches the shot's
// x, the target's y + 48 does not pass the shot's bottom and the target's y + 106 reaches the shot's y
bool SquidCannonBall::HitsTarget()
{
	if (mVelX > 0.0)
	{
		if (mHitTarget->mX + 32.0 <= (double)mFrameWidth + mX)
		{
			if (mHitTarget->mX + 132.0 >= mX)
			{
				if ((double)mFrameHeight + mY >= mHitTarget->mY)
				{
					if (mHitTarget->mY + 144.0 >= mY)
						return true;
				}
			}
		}
	}

	if (mVelX > 0.0)
	{
		if (mHitTarget->mX + 120.0 <= (double)mFrameWidth + mX)
		{
			if ((double)mHitTarget->GetFrameWidth() + mHitTarget->mX >= mX)
			{
				if (mHitTarget->mY + 48.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 106.0 >= mY)
						return true;
				}
			}
		}
	}
	return false;
}
