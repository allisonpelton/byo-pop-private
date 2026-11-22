// CollisionMgr.cpp: see CollisionMgr.h.

#include "CollisionMgr.h"

#include "Ball.h"
#include "NotTranslated.h"
#include "PhysObj.h"

using namespace Sexy;

// 0x44d870
CollisionMgr::CollisionMgr()
{
	mUnknown30 = 0;
	mUnknown34 = 0;
	mUnknown3C = 0;
	mUnknown40 = 0;
	mUnknown44 = 0;
	mUnknown4C = 0;
	mUnknown50 = 0;
	mUnknown54 = 0;
	mUnknown6C = 0;
	mUnknown70 = 0;
	mUnknown74 = 0;
	mUnknown00 = false;
	mUnknown78 = false;
}

// 0x446560
void CollisionMgr::Clear()
{
	mPairs.clear();
	mEdgesX.clear();
	mEdgesY.clear();
}

// 0x448be0 with the comparison of its set
bool CollisionPairLess::operator()(const CollisionPair& theLeft, const CollisionPair& theRight) const
{
	if (theLeft.mFirst == theRight.mFirst)
	{
		if (theLeft.mSecond->mKind != theRight.mSecond->mKind)
			return theLeft.mSecond->mKind < theRight.mSecond->mKind;
		return theLeft.mSecond->mSerialNumber < theRight.mSecond->mSerialNumber;
	}
	return theLeft.mFirst->mSerialNumber < theRight.mFirst->mSerialNumber;
}

// 0x4326e0: the two boxes touch or cross
static bool BoxesMeet(const PhysObj* theFirst, const PhysObj* theSecond)
{
	return theFirst->mLeft <= theSecond->mRight && theSecond->mLeft <= theFirst->mRight && theFirst->mTop <= theSecond->mBottom &&
		   theSecond->mTop <= theFirst->mBottom;
}

// 0x43f7c0: the first edge that is not left of the new one (the lists are searched as if they were in order, even when a piece
// has moved since they were)
static size_t FindEdgePlace(const std::vector<CollisionEdge>& theEdges, float theValue)
{
	size_t aFirst = 0;
	size_t aCount = theEdges.size();
	while (aCount > 0)
	{
		size_t aHalf = aCount / 2;
		if (*theEdges[aFirst + aHalf].mPosition < theValue)
		{
			aFirst = aFirst + aHalf + 1;
			aCount = aCount - aHalf - 1;
		}
		else
			aCount = aHalf;
	}
	return aFirst;
}

// 0x44d920: the start edge goes in first, and then the end edge
static void InsertEdges(std::vector<CollisionEdge>& theEdges, PhysObj* theObj, const float* theStart, const float* theEnd)
{
	CollisionEdge aStartEdge = {theObj, theStart, true};
	theEdges.insert(theEdges.begin() + FindEdgePlace(theEdges, *theStart), aStartEdge);
	CollisionEdge anEndEdge = {theObj, theEnd, false};
	theEdges.insert(theEdges.begin() + FindEdgePlace(theEdges, *theEnd), anEndEdge);
}

// 0x44d9d0
void CollisionMgr::AddObj(PhysObj* theObj)
{
	for (size_t i = 0; i < mEdgesX.size(); i++)
	{
		if (!mEdgesX[i].mIsStart)
			continue;
		PhysObj* anOther = mEdgesX[i].mObject;
		if (theObj->mKind == KIND_SHOT_BALL)
		{
			bool aCanMeet = anOther->mKind != KIND_SHOT_BALL ||
							(static_cast<Ball*>(anOther)->mUnknown163 && static_cast<Ball*>(theObj)->mUnknown163);
			if (aCanMeet && BoxesMeet(anOther, theObj))
			{
				CollisionPair aPair = {theObj, anOther};
				if (anOther->mKind == KIND_SHOT_BALL && !(theObj->mSerialNumber < anOther->mSerialNumber))
				{
					aPair.mFirst = anOther;
					aPair.mSecond = theObj;
				}
				mPairs.insert(aPair);
			}
		}
		else if (anOther->mKind == KIND_SHOT_BALL && BoxesMeet(anOther, theObj))
		{
			CollisionPair aPair = {anOther, theObj};
			mPairs.insert(aPair);
		}
	}

	InsertEdges(mEdgesX, theObj, &theObj->mLeft, &theObj->mRight);
	InsertEdges(mEdgesY, theObj, &theObj->mTop, &theObj->mBottom);
}

// 0x45add0
void CollisionMgr::Update(bool theDebug)
{
	NOT_TRANSLATED("CollisionMgr::Update");
}

// 0x454f90
void CollisionMgr::UpdateOneBall()
{
	NOT_TRANSLATED("CollisionMgr::UpdateOneBall");
}
