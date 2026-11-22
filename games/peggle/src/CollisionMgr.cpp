// CollisionMgr.cpp: see CollisionMgr.h.

#include "CollisionMgr.h"

#include "Ball.h"
#include "Hole.h"
#include "NotTranslated.h"
#include "PhysObj.h"
#include "SexyVector.h"

using namespace Sexy;

static const int LOCAL_MARGIN = 75; // ModValInt CollisionMgr.cpp 81,266: how far from the ball a piece may be to be kept

CollisionMgr* Sexy::gContactRecorder = nullptr; // 0x650a70

// 0x44d870
CollisionMgr::CollisionMgr()
{
	mUnknown30 = 0;
	mUnknown34 = 0;
	mRegionLeft = 0;
	mRegionRight = 0;
	mRegionTop = 0;
	mRegionBottom = 0;
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

// 0x44b1e0: an insertion sort of the edges (an edge goes in front of the ones that are not less than it, so equal ones are
// turned round). Where a start edge goes in front of an end edge the two boxes have begun to meet along this axis, so the pair
// is added when the boxes meet along the other axis as well; where an end edge goes in front of a start edge they no longer
// meet, so the pair is taken away. Only pairs with a ball in play are kept, made as AddObj makes them.
void CollisionMgr::SortEdges(std::vector<CollisionEdge>& theEdges)
{
	size_t aCount = theEdges.size();
	for (size_t i = 1; i < aCount; i++)
	{
		for (size_t j = i; j > 0; j--)
		{
			CollisionEdge& aPrev = theEdges[j - 1];
			CollisionEdge& aCur = theEdges[j];
			if (!(*aCur.mPosition <= *aPrev.mPosition))
				break;
			PhysObj* anA = aPrev.mObject;
			PhysObj* aB = aCur.mObject;
			if (!aPrev.mIsStart)
			{
				if (aCur.mIsStart)
				{
					if (anA->mKind == KIND_SHOT_BALL)
					{
						bool aCanMeet = aB->mKind != KIND_SHOT_BALL ||
										(static_cast<Ball*>(aB)->mUnknown163 && static_cast<Ball*>(anA)->mUnknown163);
						if (aCanMeet && BoxesMeet(aB, anA))
						{
							CollisionPair aPair = {anA, aB};
							if (aB->mKind == KIND_SHOT_BALL && !(anA->mSerialNumber < aB->mSerialNumber))
							{
								aPair.mFirst = aB;
								aPair.mSecond = anA;
							}
							mPairs.insert(aPair);
						}
					}
					else if (aB->mKind == KIND_SHOT_BALL && BoxesMeet(aB, anA))
					{
						CollisionPair aPair = {aB, anA};
						mPairs.insert(aPair);
					}
				}
			}
			else if (!aCur.mIsStart)
			{
				if (anA->mKind == KIND_SHOT_BALL)
				{
					if (aB->mKind == KIND_SHOT_BALL)
					{
						if (static_cast<Ball*>(aB)->mUnknown163 && static_cast<Ball*>(anA)->mUnknown163)
						{
							CollisionPair aPair = {anA, aB};
							if (!(anA->mSerialNumber < aB->mSerialNumber))
							{
								aPair.mFirst = aB;
								aPair.mSecond = anA;
							}
							mPairs.erase(aPair);
						}
					}
					else
					{
						CollisionPair aPair = {anA, aB};
						mPairs.erase(aPair);
					}
				}
				else if (aB->mKind == KIND_SHOT_BALL)
				{
					CollisionPair aPair = {aB, anA};
					mPairs.erase(aPair);
				}
			}
			std::swap(aPrev, aCur);
		}
	}
}

// 0x45ad30
void Sexy::NoteContact(PhysObj* theFirst, PhysObj* theSecond)
{
	if (gContactRecorder == nullptr)
		return;
	if (theFirst->mKind == KIND_SHOT_BALL || theSecond->mKind == KIND_SHOT_BALL)
	{
		CollisionContact aContact;
		if (theFirst->mKind == KIND_SHOT_BALL)
		{
			aContact.mBall = theFirst;
			aContact.mOther = theSecond;
		}
		else
		{
			aContact.mBall = theSecond;
			aContact.mOther = theFirst;
		}
		gContactRecorder->mContacts.push_back(aContact);
	}
}

// 0x443660: every edge of the piece is taken out of the list
static void RemoveEdges(std::vector<CollisionEdge>& theEdges, PhysObj* theObj)
{
	size_t i = 0;
	while (i < theEdges.size())
	{
		if (theEdges[i].mObject == theObj)
			theEdges.erase(theEdges.begin() + i);
		else
			i++;
	}
}

// 0x4465a0
void CollisionMgr::RemoveObj(PhysObj* theObj)
{
	RemoveEdges(mEdgesX, theObj);
	RemoveEdges(mEdgesY, theObj);
	std::set<CollisionPair, CollisionPairLess>::iterator anItr = mPairs.begin();
	while (anItr != mPairs.end())
	{
		if (anItr->mFirst == theObj || anItr->mSecond == theObj)
			mPairs.erase(anItr++);
		else
			++anItr;
	}
}

// 0x45add0: the edges are put in order (which adds and takes away the pairs of boxes that meet), then the pairs are gone through in
// their order: a ball in play looks at the pieces it may hit, and when the next pair is another ball's (or there are no more)
// the ball moves for the rest of the update and hits the pieces collected for it (Ball::DoCollideUpdate). A ball meeting a ball
// bounces off it at once.
void CollisionMgr::Update(bool theNoteContacts)
{
	mCandidates.clear();
	if (theNoteContacts)
	{
		gContactRecorder = this;
		mContacts.clear();
	}
	SortEdges(mEdgesX);
	SortEdges(mEdgesY);
	PhysObj* aBall = nullptr;
	for (std::set<CollisionPair, CollisionPairLess>::iterator anItr = mPairs.begin(); anItr != mPairs.end(); ++anItr)
	{
		PhysObj* aFirst = anItr->mFirst;
		PhysObj* anOther = anItr->mSecond;
		if (aFirst != aBall)
		{
			if (aBall != nullptr)
			{
				static_cast<Ball*>(aBall)->DoCollideUpdate(mCandidates, nullptr);
				mCandidates.clear();
			}
			aBall = aFirst;
		}
		if (!aFirst->IsCollidableWith(anOther))
			continue;
		switch (anOther->mKind)
		{
		case KIND_HOLE:
			static_cast<Hole*>(anOther)->CheckBall(static_cast<Ball*>(aFirst));
			break;
		case KIND_SHOT_BALL:
			static_cast<Ball*>(aFirst)->CollideWithBall(static_cast<Ball*>(anOther));
			break;
		case KIND_PEG:
		case KIND_LINE:
		case KIND_POLY:
			mCandidates.push_back(anOther);
			break;
		}
	}
	if (aBall != nullptr)
	{
		static_cast<Ball*>(aBall)->DoCollideUpdate(mCandidates, nullptr);
		mCandidates.clear();
	}
	gContactRecorder = nullptr;
}

// 0x432740: a ball in play comes first in a pair; of two balls in play the one made first
static CollisionPair OrderedPair(PhysObj* theBall, PhysObj* theOther)
{
	CollisionPair aPair = {theBall, theOther};
	if (!(theBall->mKind == KIND_SHOT_BALL && (theOther->mKind != KIND_SHOT_BALL || theBall->mSerialNumber < theOther->mSerialNumber)))
	{
		aPair.mFirst = theOther;
		aPair.mSecond = theBall;
	}
	return aPair;
}

// 0x454dc0: the pieces whose boxes are within a margin of the ball's box are kept, in the order of the x list's end edges (when mUnknown78
// is set, as for a fireball's guide, the pieces that are pegs are left out)
void CollisionMgr::BuildLocalPieces(Ball* theBall)
{
	float aMargin = (float)LOCAL_MARGIN;
	float aLeft = theBall->mLeft - aMargin;
	float aRight = theBall->mRight + aMargin;
	float aTop = theBall->mTop - aMargin;
	float aBottom = aMargin + theBall->mBottom;
	mLocalPieces.clear();
	mRegionLeft = aLeft;
	mRegionRight = aRight;
	mRegionTop = aTop;
	mRegionBottom = aBottom;
	for (size_t i = 0; i < mEdgesX.size(); i++)
	{
		if (mEdgesX[i].mIsStart)
			continue;
		PhysObj* aPiece = mEdgesX[i].mObject;
		if (mUnknown78 && aPiece->mPegInfo.get() != nullptr)
			continue;
		if (aPiece->mLeft <= aRight && aLeft <= aPiece->mRight && aPiece->mTop <= aBottom && aTop <= aPiece->mBottom)
			mLocalPieces.push_back(aPiece);
	}
}

// 0x454f90: a ball goes through the rest of its update. A hole it falls in that has an exit puts it there at once; the other pieces
// it may meet (pegs, lines and polygons) are put in the order of the pair set and hit in that order. The result is true when
// something that moves was touched (the contact is then where the ball was).
bool CollisionMgr::UpdateOneBall(Ball* theBall, SexyVector2* theFirstContact)
{
	bool aTouchedMover = false;
	mCandidates.clear();
	if (mRegionRight < theBall->mRight || theBall->mLeft < mRegionLeft || theBall->mTop < mRegionTop || mRegionBottom < theBall->mBottom)
		BuildLocalPieces(theBall);

	std::set<CollisionPair, CollisionPairLess> aPieces;
	for (size_t i = 0; i < mLocalPieces.size(); i++)
	{
		PhysObj* aPiece = mLocalPieces[i];
		if (!aPiece->mCollidable || !(aPiece->mLeft <= theBall->mRight && theBall->mLeft <= aPiece->mRight &&
									  aPiece->mTop <= theBall->mBottom && theBall->mTop <= aPiece->mBottom))
			continue;
		switch (aPiece->mKind)
		{
		case KIND_HOLE:
		{
			Hole* aHole = static_cast<Hole*>(aPiece);
			if (aHole->mExitPiece.get() != nullptr && aHole->CheckBall(theBall))
			{
				mUnknown00 = true;
				PhysObj* anExit = aHole->mExitPiece.get();
				theBall->SetHoleOut(anExit, 0);
				if ((aHole->mMover.get() != nullptr && aHole->mMoving) || (anExit->mMover.get() != nullptr && anExit->mMoving))
				{
					aTouchedMover = true;
					if (theFirstContact != nullptr)
					{
						theFirstContact->x = theBall->mX;
						theFirstContact->y = theBall->mY;
					}
				}
			}
			break;
		}
		case KIND_PEG:
		case KIND_LINE:
		case KIND_POLY:
			aPieces.insert(OrderedPair(theBall, aPiece));
			break;
		}
	}
	for (std::set<CollisionPair, CollisionPairLess>::iterator anIt = aPieces.begin(); anIt != aPieces.end(); ++anIt)
		mCandidates.push_back(anIt->mSecond);

	if (!mCandidates.empty() && theBall->DoCollideUpdate(mCandidates, theFirstContact))
		aTouchedMover = true;
	if (!theBall->mUnknown141)
		theBall->DoNonCollideUpdate();
	return aTouchedMover;
}
