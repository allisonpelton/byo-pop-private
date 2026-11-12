// The cloud (0x409f40, 0x405090).

#include "Cloud.h"

#include "VeniceMath.h"

Cloud::Cloud()
{
	mType = ObjectTypeCloud;
}

// The cloud keeps the height that the data file gave its first stage (a random one if the file gave it none), and its stages are
// replaced by one that does nothing but hold the cloud's place. A random speed to the left moves it, and a cloud that has passed
// 500 to the left of the middle is moved to 500 to the right of it.
void Cloud::OnObjectClosed()
{
	float aHeight;
	if (mStages.mHead == nullptr)
		aHeight = (float)RandomFloat(-300.0f, 20.0f);
	else
		aHeight = mStages.mHead->mStart[AttributeY];
	mLocal[AttributeY] = aHeight;
	mStages.Clear();
	AddStage(1000.0f, 1, 1, 1, nullptr);
	mLocal[AttributeX] = (float)RandomFloat(-400.0f, 400.0f);
	mStages.mCursor->mAttributeMask = mStages.mCursor->mAttributeMask & ~4u;
	mStages.mCursor->mFilters.Clear();
	AddFilter(FilterVelocity, AttributeX, (float)RandomFloat(-0.035f, -0.01f), 0.0f, 0.0f);
	mStages.mCursor->mEvents.Clear();
	AddEvent(TriggerLessThan, ActionMove, "x", "-500", "x", "500", "");
}
