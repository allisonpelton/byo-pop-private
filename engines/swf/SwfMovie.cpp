#include "SwfMovie.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "SwfRaster.h"
#include "SwfTypes.h"
#include "platform/VFS.h"

using namespace Sexy;

namespace {

struct Inst;

// A timeline that is playing: the movie's own, or that of a sprite on the display list
struct Timeline {
	const SwfTimelineDef* mDef = nullptr;
	int mFrame = -1;  // the frame whose tags were applied last; -1 before the first
	bool mPlaying = true;
	std::vector<Inst> mList;  // the display list, by depth
};

// An object on a display list
struct Inst {
	uint32_t mId = 0;        // told apart from every other object that was ever placed, to find it in the next picture
	int mDepth = 0;
	const SwfCharacter* mChar = nullptr;
	int mCharId = 0;
	SwfMatrix mMatrix;
	SwfCXForm mCX;
	int mRatio = 0;
	int mClipDepth = 0;
	int mPlaceFrame = 0;     // the frame of the timeline that placed it
	unsigned mBorn = 0;      // the tick that placed it: a sprite does not advance in the tick it appears in
	std::unique_ptr<Timeline> mTL;  // a sprite's own timeline
};

const int kMaxGroups = 6;  // clip layers that apply to one object at once (more are ignored)

// One thing to draw in a picture: a shape, morph shape or text with everything that decides how it looks, and where
// that is. A picture is the list of these in drawing order; two pictures are compared by the list.
struct Op {
	uint32_t mId = 0;
	const SwfCharacter* mChar = nullptr;
	SwfMatrix mMatrix;
	SwfCXForm mCX;
	int mRatio = 0;
	int mMaskGroup = -1;  // for an object of a clip layer: which one; the object is drawn into its mask
	int mNumGroups = 0;   // the clip layers that clip it
	int mGroups[kMaxGroups];
	SwfRect mBounds;
	uint64_t mSig = 0;
};

struct OpKey {
	uint32_t mId;
	uint64_t mSig;
	SwfRect mBounds;
};

struct GroupList {
	int mCount = 0;
	int mIds[kMaxGroups];
};

inline uint64_t Mix(uint64_t theHash, uint32_t theValue)
{
	theHash ^= theValue;
	theHash *= 0x100000001b3ULL;
	return theHash ^ (theHash >> 31);
}

inline uint64_t MixFloat(uint64_t theHash, float theValue)
{
	uint32_t v;
	memcpy(&v, &theValue, sizeof(v));
	return Mix(theHash, v);
}

const char kFSCommand[] = "FSCommand:";

}  // namespace

struct SwfMovie::Impl {
	SwfData mData;
	std::string mError;
	bool mLoaded = false;
	bool mStarted = false;  // the first NextFrame (or a GotoFrame) has shown a frame
	unsigned mTick = 0;
	uint32_t mLastId = 0;
	Timeline mRoot;
	std::vector<std::pair<std::string, std::string> > mPending;  // FSCommands not yet told to the listener

	// drawing
	SwfRaster mRaster;
	SwfRawShape mMorphRaw;  // scratch of the morph shapes
	SwfShape mMorphShape;
	std::vector<uint32_t> mBack;  // the last picture, which only what changed is drawn over
	int mBackWidth = 0, mBackHeight = 0;
	bool mBackValid = false;
	std::vector<Op> mOps;
	std::vector<uint64_t> mGroupSig;  // for each clip layer of the picture, what its objects look like
	std::vector<OpKey> mPrevKeys, mKeys;
	std::vector<SwfRect> mDamage;
	SwfRect mLastDamage;  // all of mDamage in one rectangle, as of the last Render
	std::vector<SwfMask*> mGroupMask;  // for each clip layer, its mask in the rectangle being drawn

	// the display lists
	void Apply(Timeline& theTL, const SwfPlaceCmd& theCmd, int theFrame, std::vector<Inst>* theOld);
	void ApplyFrame(Timeline& theTL, int theFrame, std::vector<Inst>* theOld);
	void StartSprite(Inst& theInst);
	void RunActions(Timeline& theTL, int theFrame);
	void Rewind(Timeline& theTL, int theTarget);
	void GotoInternal(Timeline& theTL, int theTarget);
	void Tick(Timeline& theTL);

	void TellListener(SwfListener* theListener);

	// diagnostics
	void Dump(const Timeline& theTL, int theIndent, std::string& theOut) const;

	// drawing
	void CollectTimeline(const Timeline& theTL, const SwfMatrix& theParent, const SwfCXForm& theCX, const GroupList& theInherited, int theMaskGroup);
	void CollectInst(const Inst& theInst, const SwfMatrix& theParent, const SwfCXForm& theCX, const GroupList& theGroups, int theMaskGroup);
	void FindDamage(int theWidth, int theHeight);
	void AddDamage(const SwfRect& theRect);
	void DrawRect(const SwfRect& theRect);
	void DrawOp(const Op& theOp);
	SwfMask* GroupMask(int theGroup);
};

namespace {

std::vector<Inst>::iterator FindDepth(std::vector<Inst>& theList, int theDepth)
{
	return std::lower_bound(theList.begin(), theList.end(), theDepth, [](const Inst& i, int d) { return i.mDepth < d; });
}

}  // namespace

// ---- the display lists ----

// A new sprite instance starts at its frame 0
void SwfMovie::Impl::StartSprite(Inst& theInst)
{
	theInst.mTL.reset(new Timeline());
	Timeline& aTL = *theInst.mTL;
	aTL.mDef = &theInst.mChar->mSprite;
	if (aTL.mDef->mFrames.empty())
		return;
	ApplyFrame(aTL, 0, nullptr);
	aTL.mFrame = 0;
	RunActions(aTL, 0);
}

// One PlaceObject2 or RemoveObject2. theOld is the display list a rewound timeline had before: an object it placed
// in this very frame is taken over from there, so that a sprite in it keeps playing.
void SwfMovie::Impl::Apply(Timeline& theTL, const SwfPlaceCmd& c, int theFrame, std::vector<Inst>* theOld)
{
	std::vector<Inst>& aList = theTL.mList;
	std::vector<Inst>::iterator anItr = FindDepth(aList, c.mDepth);
	bool aFound = anItr != aList.end() && anItr->mDepth == c.mDepth;
	if (c.mRemove) {
		if (aFound)
			aList.erase(anItr);
		return;
	}

	if (c.mHasChar && !(c.mMove && aFound)) {
		// A new object at the depth, in place of any that is there
		Inst anInst;
		bool aTaken = false;
		if (theOld != nullptr) {
			std::vector<Inst>::iterator anOld = FindDepth(*theOld, c.mDepth);
			if (anOld != theOld->end() && anOld->mDepth == c.mDepth && anOld->mCharId == c.mCharId && anOld->mPlaceFrame == theFrame) {
				anInst = std::move(*anOld);
				theOld->erase(anOld);
				aTaken = true;
			}
		}
		anInst.mDepth = c.mDepth;
		anInst.mCharId = c.mCharId;
		anInst.mChar = mData.Find(c.mCharId);
		if (!aTaken) {
			anInst.mId = ++mLastId;
			anInst.mPlaceFrame = theFrame;
			anInst.mBorn = mTick;
		}
		anInst.mMatrix = c.mHasMatrix ? c.mMatrix : SwfMatrix();
		anInst.mCX = c.mHasCX ? c.mCX : SwfCXForm();
		anInst.mRatio = c.mHasRatio ? c.mRatio : 0;
		anInst.mClipDepth = c.mHasClip ? c.mClipDepth : 0;
		if (aFound)
			*anItr = std::move(anInst);
		else
			anItr = aList.insert(anItr, std::move(anInst));
		if (!aTaken && anItr->mChar != nullptr && anItr->mChar->mType == SwfCharacter::kSprite)
			StartSprite(*anItr);
		return;
	}

	if (!aFound)
		return;
	Inst& anInst = *anItr;
	if (c.mHasChar) {
		// The character is replaced; the rest stays as it was but for what the placement names
		anInst.mCharId = c.mCharId;
		anInst.mChar = mData.Find(c.mCharId);
		anInst.mPlaceFrame = theFrame;
		anInst.mBorn = mTick;
		anInst.mTL.reset();
		if (anInst.mChar != nullptr && anInst.mChar->mType == SwfCharacter::kSprite)
			StartSprite(anInst);
	}
	if (c.mHasMatrix)
		anInst.mMatrix = c.mMatrix;
	if (c.mHasCX)
		anInst.mCX = c.mCX;
	if (c.mHasRatio)
		anInst.mRatio = c.mRatio;
	if (c.mHasClip)
		anInst.mClipDepth = c.mClipDepth;
}

void SwfMovie::Impl::ApplyFrame(Timeline& theTL, int theFrame, std::vector<Inst>* theOld)
{
	const SwfFrame& aFrame = theTL.mDef->mFrames[theFrame];
	for (size_t i = 0; i < aFrame.mCmds.size(); i++)
		Apply(theTL, aFrame.mCmds[i], theFrame, theOld);
}

void SwfMovie::Impl::RunActions(Timeline& theTL, int theFrame)
{
	const SwfFrame& aFrame = theTL.mDef->mFrames[theFrame];
	for (size_t i = 0; i < aFrame.mActions.size(); i++) {
		const SwfAction& a = aFrame.mActions[i];
		if (a.mType == SwfAction::kPlay)
			theTL.mPlaying = true;
		else if (a.mType == SwfAction::kStop)
			theTL.mPlaying = false;
		else if (a.mURL.compare(0, sizeof(kFSCommand) - 1, kFSCommand) == 0)
			mPending.push_back(std::make_pair(a.mURL.substr(sizeof(kFSCommand) - 1), a.mTarget));
		else
			mData.mUnsupported["GetURL of " + a.mURL]++;
	}
}

// The display list as it is after frame theTarget, built again from frame 0
void SwfMovie::Impl::Rewind(Timeline& theTL, int theTarget)
{
	std::vector<Inst> anOld;
	anOld.swap(theTL.mList);
	for (int f = 0; f <= theTarget; f++)
		ApplyFrame(theTL, f, &anOld);
}

void SwfMovie::Impl::GotoInternal(Timeline& theTL, int theTarget)
{
	int aCount = (int)theTL.mDef->mFrames.size();
	if (aCount == 0)
		return;
	theTarget = std::min(std::max(theTarget, 0), aCount - 1);
	if (theTL.mFrame >= 0 && theTarget > theTL.mFrame) {
		for (int f = theTL.mFrame + 1; f <= theTarget; f++)
			ApplyFrame(theTL, f, nullptr);
	}
	else
		Rewind(theTL, theTarget);
	theTL.mFrame = theTarget;
	RunActions(theTL, theTarget);
}

// One tick of a timeline and of the sprites that were on its display list before the tick
void SwfMovie::Impl::Tick(Timeline& theTL)
{
	int aCount = (int)theTL.mDef->mFrames.size();
	// A movie of one frame has nowhere to go
	if (theTL.mPlaying && aCount > 1) {
		int aNext = theTL.mFrame + 1;
		if (aNext >= aCount) {
			Rewind(theTL, 0);
			aNext = 0;
		}
		else
			ApplyFrame(theTL, aNext, nullptr);
		theTL.mFrame = aNext;
		RunActions(theTL, aNext);
	}
	for (size_t i = 0; i < theTL.mList.size(); i++) {
		Inst& anInst = theTL.mList[i];
		if (anInst.mTL && anInst.mBorn != mTick)
			Tick(*anInst.mTL);
	}
}

// The host hears of the commands once the movie is in its new state, so that it may Play or GotoFrame in answer
void SwfMovie::Impl::TellListener(SwfListener* theListener)
{
	if (mPending.empty())
		return;
	std::vector<std::pair<std::string, std::string> > aCommands;
	aCommands.swap(mPending);
	for (size_t i = 0; i < aCommands.size() && theListener != nullptr; i++)
		theListener->SwfCommand(aCommands[i].first, aCommands[i].second);
}

// ---- diagnostics ----

void SwfMovie::Impl::Dump(const Timeline& theTL, int theIndent, std::string& theOut) const
{
	static const char* kTypes[] = {"none", "shape", "morph", "sprite", "text", "font"};
	char aBuf[256];
	for (size_t i = 0; i < theTL.mList.size(); i++) {
		const Inst& anInst = theTL.mList[i];
		snprintf(aBuf, sizeof(aBuf), "%*sdepth %d: %s %d m=[%.3f %.3f %.3f %.3f %.0f %.0f] cx=[*%.2f %.2f %.2f %.2f +%.0f %.0f %.0f %.0f]", theIndent * 2, "",
		         anInst.mDepth, anInst.mChar ? kTypes[anInst.mChar->mType] : "missing", anInst.mCharId, anInst.mMatrix.a, anInst.mMatrix.b,
		         anInst.mMatrix.c, anInst.mMatrix.d, anInst.mMatrix.tx, anInst.mMatrix.ty, anInst.mCX.mMul[0], anInst.mCX.mMul[1],
		         anInst.mCX.mMul[2], anInst.mCX.mMul[3], anInst.mCX.mAdd[0], anInst.mCX.mAdd[1], anInst.mCX.mAdd[2], anInst.mCX.mAdd[3]);
		theOut += aBuf;
		if (anInst.mClipDepth > 0) {
			snprintf(aBuf, sizeof(aBuf), " clip to %d", anInst.mClipDepth);
			theOut += aBuf;
		}
		if (anInst.mChar && anInst.mChar->mType == SwfCharacter::kMorph) {
			snprintf(aBuf, sizeof(aBuf), " ratio %d", anInst.mRatio);
			theOut += aBuf;
		}
		if (anInst.mTL) {
			snprintf(aBuf, sizeof(aBuf), " frame %d of %d %s", anInst.mTL->mFrame, (int)anInst.mTL->mDef->mFrames.size(),
			         anInst.mTL->mPlaying ? "playing" : "stopped");
			theOut += aBuf;
		}
		theOut += '\n';
		if (anInst.mTL)
			Dump(*anInst.mTL, theIndent + 1, theOut);
	}
}

// ---- drawing ----
//
// A picture is made in three steps. CollectTimeline walks the display lists and lists what is to be drawn, in
// drawing order, each with its matrix and color transform worked out down the tree, where it will land, and a
// signature of everything that decides how it looks. FindDamage compares the list with the one of the picture
// before (the objects are told apart by their ids) and so finds the rectangles that differ. DrawRect draws the
// list again within each rectangle, over the background, which makes those pixels what a drawing of the whole
// picture would; the rest of the picture is not touched.

// The objects of a display list that are to be drawn, from the lowest depth up. An object with a clip depth is not
// drawn itself: it becomes a clip layer, whose coverage clips the objects above it up to its clip depth. Clip layers
// that overlap each other all apply where they do.
void SwfMovie::Impl::CollectTimeline(const Timeline& theTL, const SwfMatrix& theParent, const SwfCXForm& theCX, const GroupList& theInherited,
                                     int theMaskGroup)
{
	struct Active {
		int mEnd;
		int mGroup;
	};
	const int kMaxActive = 16;
	Active anActive[kMaxActive];
	int aNumActive = 0;

	for (size_t i = 0; i < theTL.mList.size(); i++) {
		const Inst& anInst = theTL.mList[i];
		// Clip layers whose range ended below this object stop applying
		int aKept = 0;
		for (int k = 0; k < aNumActive; k++) {
			if (anActive[k].mEnd >= anInst.mDepth)
				anActive[aKept++] = anActive[k];
		}
		aNumActive = aKept;

		if (anInst.mClipDepth > 0 && theMaskGroup < 0) {
			if (anInst.mChar == nullptr || aNumActive >= kMaxActive)
				continue;
			int aGroup = (int)mGroupSig.size();
			mGroupSig.push_back(1469598103934665603ULL);
			GroupList aNone;
			CollectInst(anInst, theParent, theCX, aNone, aGroup);
			anActive[aNumActive].mEnd = anInst.mClipDepth;
			anActive[aNumActive].mGroup = aGroup;
			aNumActive++;
			continue;
		}
		GroupList aGroups = theInherited;
		for (int k = 0; k < aNumActive && aGroups.mCount < kMaxGroups; k++)
			aGroups.mIds[aGroups.mCount++] = anActive[k].mGroup;
		CollectInst(anInst, theParent, theCX, aGroups, theMaskGroup);
	}
}

void SwfMovie::Impl::CollectInst(const Inst& theInst, const SwfMatrix& theParent, const SwfCXForm& theCX, const GroupList& theGroups, int theMaskGroup)
{
	if (theInst.mChar == nullptr)
		return;
	SwfMatrix m = SwfMatrix::Concat(theParent, theInst.mMatrix);
	SwfCXForm cx = SwfCXForm::Concat(theCX, theInst.mCX);
	// What is wholly transparent, with everything on it, cannot show (but a clip layer does not mind)
	if (theMaskGroup < 0 && cx.mMul[3] <= 0 && cx.mAdd[3] <= 0)
		return;

	const SwfCharacter& aChar = *theInst.mChar;
	if (aChar.mType == SwfCharacter::kSprite) {
		if (theInst.mTL)
			CollectTimeline(*theInst.mTL, m, cx, theGroups, theMaskGroup);
		return;
	}

	Op anOp;
	bool aHasBounds = false;
	if (aChar.mType == SwfCharacter::kShape)
		aHasBounds = SwfRaster::ShapeBounds(aChar.mShape, m, anOp.mBounds);
	else if (aChar.mType == SwfCharacter::kMorph)
		aHasBounds = !aChar.mMorph.mEmpty &&
		             SwfRaster::TransformBounds(aChar.mMorph.mMinX, aChar.mMorph.mMinY, aChar.mMorph.mMaxX, aChar.mMorph.mMaxY, m, anOp.mBounds);
	else if (aChar.mType == SwfCharacter::kText)
		aHasBounds = SwfRaster::TextBounds(aChar.mText, mData, m, anOp.mBounds);
	if (!aHasBounds)
		return;

	anOp.mId = theInst.mId;
	anOp.mChar = &aChar;
	anOp.mMatrix = m;
	anOp.mCX = cx;
	anOp.mRatio = theInst.mRatio;
	anOp.mMaskGroup = theMaskGroup;
	anOp.mNumGroups = theGroups.mCount;
	for (int k = 0; k < theGroups.mCount; k++)
		anOp.mGroups[k] = theGroups.mIds[k];

	uint64_t aSig = 1469598103934665603ULL;
	aSig = Mix(aSig, theInst.mId);
	aSig = Mix(aSig, (uint32_t)(uintptr_t)&aChar);
	aSig = Mix(aSig, (uint32_t)((uint64_t)(uintptr_t)&aChar >> 32));
	aSig = MixFloat(aSig, m.a);
	aSig = MixFloat(aSig, m.b);
	aSig = MixFloat(aSig, m.c);
	aSig = MixFloat(aSig, m.d);
	aSig = MixFloat(aSig, m.tx);
	aSig = MixFloat(aSig, m.ty);
	for (int k = 0; k < 4; k++) {
		aSig = MixFloat(aSig, cx.mMul[k]);
		aSig = MixFloat(aSig, cx.mAdd[k]);
	}
	if (aChar.mType == SwfCharacter::kMorph)
		aSig = Mix(aSig, (uint32_t)theInst.mRatio);
	aSig = Mix(aSig, theMaskGroup >= 0 ? 1u : 0u);
	for (int k = 0; k < theGroups.mCount; k++) {
		aSig = Mix(aSig, (uint32_t)mGroupSig[theGroups.mIds[k]]);
		aSig = Mix(aSig, (uint32_t)(mGroupSig[theGroups.mIds[k]] >> 32));
	}
	anOp.mSig = aSig;
	if (theMaskGroup >= 0) {
		mGroupSig[theMaskGroup] = Mix(mGroupSig[theMaskGroup], (uint32_t)aSig);
		mGroupSig[theMaskGroup] = Mix(mGroupSig[theMaskGroup], (uint32_t)(aSig >> 32));
		mGroupSig[theMaskGroup] = Mix(mGroupSig[theMaskGroup], (uint32_t)anOp.mBounds.mX0);
		mGroupSig[theMaskGroup] = Mix(mGroupSig[theMaskGroup], (uint32_t)anOp.mBounds.mY1);
	}
	mOps.push_back(anOp);
}

// Adds a rectangle to those to be drawn again, joining it to the ones it meets
void SwfMovie::Impl::AddDamage(const SwfRect& theRect)
{
	SwfRect aRect = theRect;
	SwfRect aTarget;
	aTarget.mX1 = mBackWidth;
	aTarget.mY1 = mBackHeight;
	aRect.Intersect(aTarget);
	if (aRect.IsEmpty())
		return;
	// Rectangles that overlap, or whose union is hardly bigger than the two, are one
	for (size_t i = 0; i < mDamage.size();) {
		SwfRect aUnion = mDamage[i];
		aUnion.Union(aRect);
		long long aSeparate = (long long)(mDamage[i].mX1 - mDamage[i].mX0) * (mDamage[i].mY1 - mDamage[i].mY0) +
		                      (long long)(aRect.mX1 - aRect.mX0) * (aRect.mY1 - aRect.mY0);
		long long aTogether = (long long)(aUnion.mX1 - aUnion.mX0) * (aUnion.mY1 - aUnion.mY0);
		if (mDamage[i].Intersects(aRect) || aTogether <= aSeparate + aSeparate / 4) {
			aRect = aUnion;
			mDamage[i] = mDamage.back();
			mDamage.pop_back();
			i = 0;
		}
		else
			i++;
	}
	mDamage.push_back(aRect);
}

// Compares the picture just listed with the one before, which sets mDamage to the rectangles that differ
void SwfMovie::Impl::FindDamage(int theWidth, int theHeight)
{
	mDamage.clear();
	SwfRect aAll;
	aAll.mX1 = theWidth;
	aAll.mY1 = theHeight;

	mKeys.clear();
	for (size_t i = 0; i < mOps.size(); i++) {
		OpKey aKey = {mOps[i].mId, mOps[i].mSig, mOps[i].mBounds};
		mKeys.push_back(aKey);
	}
	std::sort(mKeys.begin(), mKeys.end(), [](const OpKey& l, const OpKey& r) { return l.mId < r.mId; });

	if (!mBackValid)
		mDamage.push_back(aAll);
	else {
		size_t p = 0, c = 0;
		while (p < mPrevKeys.size() || c < mKeys.size()) {
			if (c == mKeys.size() || (p < mPrevKeys.size() && mPrevKeys[p].mId < mKeys[c].mId)) {
				AddDamage(mPrevKeys[p].mBounds);
				p++;
			}
			else if (p == mPrevKeys.size() || mKeys[c].mId < mPrevKeys[p].mId) {
				AddDamage(mKeys[c].mBounds);
				c++;
			}
			else {
				if (mPrevKeys[p].mSig != mKeys[c].mSig) {
					AddDamage(mPrevKeys[p].mBounds);
					AddDamage(mKeys[c].mBounds);
				}
				p++;
				c++;
			}
		}
		// A picture that is mostly to be drawn again is better drawn at once
		long long anArea = 0;
		for (size_t i = 0; i < mDamage.size(); i++)
			anArea += (long long)(mDamage[i].mX1 - mDamage[i].mX0) * (mDamage[i].mY1 - mDamage[i].mY0);
		if (mDamage.size() > 8 || anArea * 10 > (long long)theWidth * theHeight * 7) {
			mDamage.clear();
			mDamage.push_back(aAll);
		}
	}
	mPrevKeys.swap(mKeys);
}

SwfMask* SwfMovie::Impl::GroupMask(int theGroup)
{
	if (mGroupMask[theGroup] == nullptr)
		mGroupMask[theGroup] = mRaster.AllocMask();
	return mGroupMask[theGroup];
}

void SwfMovie::Impl::DrawOp(const Op& theOp)
{
	const SwfCharacter& aChar = *theOp.mChar;
	switch (aChar.mType) {
	case SwfCharacter::kShape:
		mRaster.DrawShape(aChar.mShape, theOp.mMatrix, theOp.mCX, true);
		break;
	case SwfCharacter::kMorph:
		SwfBuildMorph(aChar.mMorph, theOp.mRatio, mMorphRaw);
		SwfCompileShape(mMorphRaw, mMorphShape);
		mRaster.DrawShape(mMorphShape, theOp.mMatrix, theOp.mCX, false);
		break;
	case SwfCharacter::kText:
		mRaster.DrawText(aChar.mText, mData, theOp.mMatrix, theOp.mCX);
		break;
	default:
		break;
	}
}

// Draws the picture again within a rectangle
void SwfMovie::Impl::DrawRect(const SwfRect& theRect)
{
	mRaster.SetTarget(mBack.data(), mBackWidth, mBackHeight, mBackWidth);
	mRaster.SetDamage(theRect);
	const SwfColor& aBack = mData.mBackground;
	mRaster.Fill(((uint32_t)aBack.r << 16) | ((uint32_t)aBack.g << 8) | aBack.b);

	mGroupMask.assign(mGroupSig.size(), nullptr);
	const SwfMask* anEffective = nullptr;
	int aLastGroups[kMaxGroups];
	int aNumLastGroups = 0;
	for (size_t i = 0; i < mOps.size(); i++) {
		const Op& anOp = mOps[i];
		if (!anOp.mBounds.Intersects(theRect))
			continue;
		if (anOp.mMaskGroup >= 0) {
			mRaster.SetMaskTarget(GroupMask(anOp.mMaskGroup));
			DrawOp(anOp);
			mRaster.SetMaskTarget(nullptr);
			continue;
		}
		if (anOp.mNumGroups == 0)
			anEffective = nullptr;
		else if (anOp.mNumGroups != aNumLastGroups || memcmp(anOp.mGroups, aLastGroups, anOp.mNumGroups * sizeof(int)) != 0) {
			anEffective = GroupMask(anOp.mGroups[0]);
			for (int k = 1; k < anOp.mNumGroups; k++)
				anEffective = mRaster.Intersect(anEffective, GroupMask(anOp.mGroups[k]));
		}
		aNumLastGroups = anOp.mNumGroups;
		memcpy(aLastGroups, anOp.mGroups, anOp.mNumGroups * sizeof(int));
		if (anOp.mNumGroups > 0 && anEffective->IsEmpty())
			continue;
		mRaster.SetClip(anEffective);
		DrawOp(anOp);
	}
	mRaster.SetClip(nullptr);
}

// ---- the public class ----

SwfMovie::SwfMovie() : mImpl(new Impl()) {}

SwfMovie::~SwfMovie() {}

bool SwfMovie::Load(const std::string& theGamePath)
{
	mImpl.reset(new Impl());
	Impl& I = *mImpl;
	std::vector<uint8_t> aBytes;
	if (!Platform::VFS::Get().ReadAll(theGamePath, aBytes)) {
		I.mError = "cannot read " + theGamePath;
		return false;
	}
	if (!SwfParse(aBytes.data(), aBytes.size(), I.mData, I.mError))
		return false;
	if (I.mData.mRoot.mFrames.empty()) {
		I.mError = "the movie has no frames";
		return false;
	}
	I.mRoot.mDef = &I.mData.mRoot;
	I.mLoaded = true;
	return true;
}

const std::string& SwfMovie::GetError() const
{
	return mImpl->mError;
}

int SwfMovie::GetWidth() const
{
	return (int)((mImpl->mData.mXMax - mImpl->mData.mXMin) / 20 + 0.5f);
}

int SwfMovie::GetHeight() const
{
	return (int)((mImpl->mData.mYMax - mImpl->mData.mYMin) / 20 + 0.5f);
}

float SwfMovie::GetFrameRate() const
{
	return mImpl->mData.mFrameRate;
}

int SwfMovie::GetFrameCount() const
{
	return (int)mImpl->mData.mRoot.mFrames.size();
}

int SwfMovie::GetCurrentFrame() const
{
	return std::max(0, mImpl->mRoot.mFrame);
}

bool SwfMovie::IsPlaying() const
{
	return mImpl->mRoot.mPlaying;
}

void SwfMovie::Play()
{
	mImpl->mRoot.mPlaying = true;
}

void SwfMovie::Stop()
{
	mImpl->mRoot.mPlaying = false;
}

void SwfMovie::GotoFrame(int theFrame)
{
	Impl& I = *mImpl;
	if (!I.mLoaded)
		return;
	I.mStarted = true;
	I.GotoInternal(I.mRoot, theFrame);
	I.TellListener(mListener);
}

std::string SwfMovie::GetCurrentLabel() const
{
	const std::vector<SwfFrame>& aFrames = mImpl->mData.mRoot.mFrames;
	for (int i = std::min(GetCurrentFrame(), (int)aFrames.size() - 1); i >= 0; i--)
		if (!aFrames[i].mLabel.empty())
			return aFrames[i].mLabel;
	return std::string();
}

bool SwfMovie::GotoLabel(const std::string& theLabel)
{
	const std::vector<SwfFrame>& aFrames = mImpl->mData.mRoot.mFrames;
	for (size_t i = 0; i < aFrames.size(); i++) {
		if (aFrames[i].mLabel == theLabel) {
			GotoFrame((int)i);
			return true;
		}
	}
	return false;
}

void SwfMovie::NextFrame()
{
	Impl& I = *mImpl;
	if (!I.mLoaded)
		return;
	I.mTick++;
	if (!I.mStarted) {
		I.mStarted = true;
		I.GotoInternal(I.mRoot, 0);
	}
	else
		I.Tick(I.mRoot);
	I.TellListener(mListener);
}

void SwfMovie::Invalidate()
{
	mImpl->mBackValid = false;
}

bool SwfMovie::GetDamage(int& theX, int& theY, int& theWidth, int& theHeight) const
{
	const SwfRect& r = mImpl->mLastDamage;
	theX = r.mX0;
	theY = r.mY0;
	theWidth = r.IsEmpty() ? 0 : r.mX1 - r.mX0;
	theHeight = r.IsEmpty() ? 0 : r.mY1 - r.mY0;
	return !r.IsEmpty();
}

void SwfMovie::Render(uint32_t* theBits, int theWidth, int theHeight, int theStride)
{
	Impl& I = *mImpl;
	if (!I.mLoaded || theWidth <= 0 || theHeight <= 0)
		return;
	if (theWidth != I.mBackWidth || theHeight != I.mBackHeight) {
		I.mBack.assign((size_t)theWidth * theHeight, 0xFF000000u);
		I.mBackWidth = theWidth;
		I.mBackHeight = theHeight;
		I.mBackValid = false;
	}

	// The picture, as a list of what to draw; twips of the movie become pixels of the target
	I.mOps.clear();
	I.mGroupSig.clear();
	if (I.mStarted) {
		SwfMatrix aBase;
		aBase.a = theWidth / (I.mData.mXMax - I.mData.mXMin);
		aBase.d = theHeight / (I.mData.mYMax - I.mData.mYMin);
		aBase.tx = -I.mData.mXMin * aBase.a;
		aBase.ty = -I.mData.mYMin * aBase.d;
		GroupList aNone;
		I.CollectTimeline(I.mRoot, aBase, SwfCXForm(), aNone, -1);
	}
	I.FindDamage(theWidth, theHeight);
	I.mLastDamage = SwfRect();
	for (size_t i = 0; i < I.mDamage.size(); i++) {
		I.DrawRect(I.mDamage[i]);
		I.mLastDamage.Union(I.mDamage[i]);
	}
	I.mBackValid = true;

	for (int y = 0; y < theHeight; y++)
		memcpy(theBits + (size_t)y * theStride, &I.mBack[(size_t)y * theWidth], (size_t)theWidth * sizeof(uint32_t));
}

const std::map<std::string, int>& SwfMovie::GetUnsupported() const
{
	return mImpl->mData.mUnsupported;
}

std::string SwfMovie::DumpDisplayList() const
{
	std::string aResult;
	mImpl->Dump(mImpl->mRoot, 0, aResult);
	return aResult;
}
