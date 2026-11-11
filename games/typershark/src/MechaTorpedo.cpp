#include "MechaTorpedo.h"

using namespace Sexy;

// Not translated (vtordisp thunks and the destructors): 0x40eb80 (HitsTarget), 0x40eb90 (SetState) and 0x40eba0 (Draw)
// (jump to the functions below or to Torpedo's 0x405080 and 0x4085e0 with the pointer adjusted), 0x40ebb0 (deleting
// destructor, shared with CannonBall), 0x40f310 (destructor of the whole object)

// 0x40ea50: Torpedo (0x40c1a0) first; nothing else (the export cuts it in two at 0x40ea73)
MechaTorpedo::MechaTorpedo(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer) :
	Torpedo(theApp, theHitTarget, thePlayer)
{
}

MechaTorpedo::~MechaTorpedo()
{
}

// 0x401430: Torpedo::HitsTarget (0x401b90) with other numbers. The first box: the shot's right edge reaches the
// target's x + 28, the shot's x does not pass x + 28 + 156, the target's y + 65 does not pass the shot's bottom and the
// target's y + 210 reaches the shot's y. The second: the target's x + 111 does not pass the shot's right edge, x + 156
// reaches the shot's x, the target's y + 4 does not pass the shot's bottom and y + 70 reaches the shot's y
bool MechaTorpedo::HitsTarget()
{
	if (mVelX > 0.0)
	{
		double aTargetX = mHitTarget->mX + 28.0;
		if ((double)mFrameWidth + mX >= aTargetX)
		{
			if (mX <= 156.0 + aTargetX)
			{
				if (mHitTarget->mY + 65.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 210.0 >= mY)
						return true;
				}
			}
		}
	}

	if (mVelX > 0.0)
	{
		if (mHitTarget->mX + 111.0 <= (double)mFrameWidth + mX)
		{
			if (mHitTarget->mX + 156.0 >= mX)
			{
				if (mHitTarget->mY + 4.0 <= (double)mFrameHeight + mY)
				{
					if (mHitTarget->mY + 70.0 >= mY)
						return true;
				}
			}
		}
	}
	return false;
}
