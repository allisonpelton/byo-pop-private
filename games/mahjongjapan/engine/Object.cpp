#include "Object.h"

#include "Display.h"
#include "EngineRes.h"
#include "Font.h"
#include "Layer.h"
#include "Manager.h"

namespace jb
{

// 0x4207f0: one rect moved by (-theX, -theY)
static RegionRect* CopyRegionRect(const RegionRect* theRect, int theX, int theY)
{
	Rect aRect = theRect->mRect;
	OffsetRect(aRect, -theX, -theY);
	RegionRect* aCopy = new RegionRect;
	aCopy->mRect = aRect;
	aCopy->mNext = nullptr;
	return aCopy;
}

// 0x4207a0: returns the first rect of the copy, linked through mNext
RegionRect* CopyRegion(RegionRect* theRegion, int theX, int theY, RegionRect** theFirst)
{
	RegionRect* aFirst = CopyRegionRect(theRegion, theX, theY);
	RegionRect* aLast = aFirst;
	for (RegionRect* aRect = theRegion->mNext; aRect != nullptr; aRect = aRect->mNext)
	{
		RegionRect* aCopy = CopyRegionRect(aRect, theX, theY);
		aLast->mNext = aCopy;
		aLast = aCopy;
	}
	if (theFirst != nullptr)
		*theFirst = aFirst;
	return aFirst;
}

// 0x41ec10
Object::Object()
{
	SetRect(mDirty, 0, 0, 0, 0);
	CrtSrand(TickCount());
}

// 0x41ed00
Object::~Object()
{
}

// 0x41ed40
void Object::DestroyTree()
{
	Clear();
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		Object* aChild = mChildren[i];
		aChild->DestroyTree();
		delete aChild;
	}
	mChildren.clear();
	for (size_t i = 0; i < mAttachments.size(); i++)
		delete mAttachments[i];
	mAttachments.clear();
	RegHover(false);
	RegLayerCmd(false);
	RegClick(false);
	RegMove(false);
	RegKey(false);
	RegChar(false);
	RegUpdate(false);
	while (mTimers != nullptr)
	{
		ListNode* aNode = mTimers;
		ObjTimer* aTimer = (ObjTimer*)aNode->mObj;
		mTimers = aNode->mNext;
		if (aNode->mNext != nullptr)
			aNode->mNext->mPrev = nullptr;
		delete aNode;
		delete aTimer;
	}
	FreeClipOwn();
}

// 0x41eea0
void Object::FreeClipOwn()
{
	RegionRect* aRect = mClipOwn;
	while (aRect != nullptr)
	{
		RegionRect* aNext = aRect->mNext;
		delete aRect;
		aRect = aNext;
	}
	mClipOwn = nullptr;
}

// 0x41eed0
void Object::SetVisible(bool theVisible)
{
	if (mVisible != theVisible)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
			mChildren[i]->SetVisibleByParent(theVisible);
		mVisible = theVisible;
		if (!theVisible)
			SetRect(mDirty, 0, 0, 0, 0);
		mManager->RedoHover();
		OnVisibleChanged();
		Invalidate();
	}
}

// 0x41ef30
void Object::SetVisibleByParent(bool theVisible)
{
	mVisibleByParent = theVisible;
	SetRect(mDirty, 0, 0, 0, 0);
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->SetVisibleByParent(theVisible);
	OnVisibleChanged();
}

// 0x41ef80
void Object::SetEnabled(bool theEnabled)
{
	if (mEnabled != theEnabled)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
			mChildren[i]->SetEnabledByParent(theEnabled);
		mEnabled = theEnabled;
		PauseClock(!theEnabled);
		mManager->RedoHover();
		OnEnabledChanged();
		Invalidate();
	}
}

// 0x41efe0
void Object::SetEnabledByParent(bool theEnabled)
{
	mEnabledByParent = theEnabled;
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->SetEnabledByParent(theEnabled);
	PauseClock(!theEnabled);
	OnEnabledChanged();
}

// 0x41f020
void Object::PauseClock(bool thePaused)
{
	for (size_t i = 0; i < mAttachments.size(); i++)
		mAttachments[i]->SetPaused(thePaused);
	if (!mPaused)
	{
		if (thePaused)
		{
			mPauseStart = ManagerTime();
			mPaused = true;
		}
		if (!mPaused)
			return;
	}
	if (!thePaused)
	{
		int aNow = ManagerTime();
		mPaused = false;
		mStart = mStart + (aNow - mPauseStart);
	}
}

// 0x41f090
void Object::SetCursorKind(int theCursor, bool theForce)
{
	if (mCursor != theCursor)
	{
		mCursor = theCursor;
		mManager->SetObjectCursor(this, theCursor, theForce);
	}
}

// 0x41f0b0
void Object::SetClip(RegionRect* theRegion)
{
	FreeClipOwn();
	if (mUseClip)
	{
		if (theRegion != nullptr)
			mClipOwn = CopyRegion(theRegion, mX, mY, nullptr);
		if (mClipTail == nullptr)
			mClip = mClipOwn;
		else
			mClipTail->mNext = mClipOwn;
	}
	for (size_t i = 0; i < mChildren.size(); i++)
		mChildren[i]->SetClip(theRegion);
}

// 0x41f190
void Object::DrawImage(int theX, int theY, int theImageId, const Rect* theSrc, bool theRelative, int theScaleX,
	int theScaleY, int theAlpha)
{
	Image* anImage = mManager->GetImage(theImageId, Module::DLL);
	if (anImage != nullptr)
		DrawWorker(0, theX, theY, anImage, theRelative, 0, theSrc, theScaleX, theScaleY, theAlpha);
}

// 0x41f1e0
void Object::DrawImagePtr(int theX, int theY, Image* theImage, const Rect* theSrc, bool theRelative, int theScaleX,
	int theScaleY, int theAlpha)
{
	DrawWorker(0, theX, theY, theImage, theRelative, 0, theSrc, theScaleX, theScaleY, theAlpha);
}

// 0x41f220
void Object::DrawImageKeyed(int theX, int theY, int theImageId, uint32_t theKey, const Rect* theSrc,
	bool theRelative, int theScaleX, int theScaleY, int theAlpha)
{
	Image* anImage = mManager->GetImage(theImageId, Module::DLL);
	if (anImage != nullptr)
		DrawWorker(1, theX, theY, anImage, theRelative, theKey, theSrc, theScaleX, theScaleY, theAlpha);
}

// 0x41f270. Without a clip region the alpha is multiplied by the layer's and the object's own; with one it is
// not (the original passes it through unchanged on that path).
void Object::DrawWorker(int theMode, int theX, int theY, Image* theImage, bool theRelative, uint32_t theKey,
	const Rect* theSrc, int theScaleX, int theScaleY, int theAlpha)
{
	int anOffset[2] = {0, 0};
	if (theRelative)
		GetScreenPos(anOffset);
	Rect anOut;
	if (mClip == nullptr)
	{
		int aLayerAlpha = mLayer->mAlpha;
		if (aLayerAlpha != 0xff)
			theAlpha = (aLayerAlpha * theAlpha) / 0xff;
		if (mAlpha != 0xff)
			theAlpha = (mAlpha * theAlpha) / 0xff;
		if (theMode == 0)
			mManager->DrawImage(anOffset[0] + theX, anOffset[1] + theY, theImage, theSrc, theScaleX, theScaleY,
				theAlpha, anOut);
		else if (theMode == 1)
			mManager->DrawImageKeyed(anOffset[0] + theX, anOffset[1] + theY, theImage, theKey, theSrc, theScaleX,
				theScaleY, theAlpha, anOut);
		AddDirty(anOut);
		return;
	}

	const Rect* aSrc = theSrc;
	Rect aWhole;
	if (theSrc == nullptr)
	{
		ImageInfo anInfo;
		theImage->GetInfo(anInfo);
		SetRect(aWhole, 0, 0, anInfo.mWidth, anInfo.mHeight);
		aSrc = &aWhole;
	}
	Rect aDest;
	if (theScaleX == 100 && theScaleY == 100)
	{
		aDest.left = theX;
		aDest.right = (theX - aSrc->left) + aSrc->right;
		aDest.top = theY;
		aDest.bottom = (aSrc->bottom - aSrc->top) + theY;
	}
	else
	{
		int aHeight = MulDiv(aSrc->bottom - aSrc->top, theScaleY, 100);
		int aWidth = MulDiv(aSrc->right - aSrc->left, theScaleX, 100);
		SetRect(aDest, theX, theY, aWidth + theX, aHeight + theY);
	}
	for (RegionRect* aClip = mClip; aClip != nullptr; aClip = aClip->mNext)
	{
		Rect aPart;
		if (!IntersectRect(aPart, aClip->mRect, aDest))
			continue;
		Rect aPartSrc = *aSrc;
		if (theScaleX == 100 && theScaleY == 100)
		{
			aPartSrc.left = aPartSrc.left + (aPart.left - aDest.left);
			aPartSrc.top = aPartSrc.top + (aPart.top - aDest.top);
			aPartSrc.right = (aPart.right - aPart.left) + aPartSrc.left;
			aPartSrc.bottom = (aPart.bottom - aPart.top) + aPartSrc.top;
		}
		else
		{
			aPartSrc.left = aPartSrc.left + MulDiv(aPart.left - aDest.left, 100, theScaleX);
			aPartSrc.top = aPartSrc.top + MulDiv(aPart.top - aDest.top, 100, theScaleY);
			aPartSrc.right = MulDiv(aPart.right - aPart.left, 100, theScaleX) + aPartSrc.left;
			int aBottom = MulDiv(aPart.bottom - aPart.top, 100, theScaleY);
			if (aSrc->right < aPartSrc.right)
				aPartSrc.right = aSrc->right;
			aPartSrc.bottom = aBottom + aPartSrc.top;
			if (aSrc->bottom < aBottom + aPartSrc.top)
				aPartSrc.bottom = aSrc->bottom;
		}
		if (theMode == 0)
			mManager->DrawImage(aPart.left + anOffset[0], aPart.top + anOffset[1], theImage, &aPartSrc, theScaleX,
				theScaleY, theAlpha, anOut);
		else if (theMode == 1)
			mManager->DrawImageKeyed(aPart.left + anOffset[0], aPart.top + anOffset[1], theImage, theKey, &aPartSrc,
				theScaleX, theScaleY, theAlpha, anOut);
		AddDirty(anOut);
	}
}

// 0x41f6a0
void Object::FillRect(Rect theRect, uint32_t theColour, bool theRelative)
{
	if (theRelative)
	{
		int aPos[2];
		GetScreenPos(aPos);
		OffsetRect(theRect, aPos[0], aPos[1]);
	}
	RegionRect* aClip = mClip;
	if (aClip == nullptr)
	{
		int aLayerAlpha = mLayer->mAlpha;
		if (aLayerAlpha == 0xff)
		{
			jb::FillRect(&theRect, theColour);
			AddDirty(theRect);
			return;
		}
		jb::FillRectAlpha(theRect, aLayerAlpha, theColour);
		AddDirty(theRect);
		return;
	}
	do
	{
		Rect aPart;
		if (IntersectRect(aPart, aClip->mRect, theRect))
		{
			jb::FillRect(&aPart, theColour);
			AddDirty(aPart);
		}
		aClip = aClip->mNext;
	} while (aClip != nullptr);
}

// 0x41f770
void Object::FillRectAlpha(Rect theRect, int theAlpha, uint32_t theColour, bool theRelative)
{
	if (theRelative)
	{
		int aPos[2];
		GetScreenPos(aPos);
		OffsetRect(theRect, aPos[0], aPos[1]);
	}
	RegionRect* aClip = mClip;
	if (aClip == nullptr)
	{
		jb::FillRectAlpha(theRect, theAlpha, theColour);
		AddDirty(theRect);
		return;
	}
	do
	{
		Rect aPart;
		if (IntersectRect(aPart, aClip->mRect, theRect))
		{
			jb::FillRectAlpha(aPart, theAlpha, theColour);
			AddDirty(aPart);
		}
		aClip = aClip->mNext;
	} while (aClip != nullptr);
}

// 0x41f810. A line that is neither horizontal nor vertical draws whatever the uninitialised rect held in the
// original; the engine draws only horizontal and vertical ones.
void Object::DrawLine(int theX1, int theY1, int theX2, int theY2, uint32_t theColour, bool theRelative)
{
	Rect aRect;
	if (theX1 == theX2)
	{
		theX2 = theX2 + 1;
		SetRect(aRect, theX1, theY1, theX2, theY2);
	}
	else
	{
		if (theY1 != theY2)
			return;
		theY2 = theY2 + 1;
		SetRect(aRect, theX1, theY1, theX2, theY2);
	}
	int aLayerAlpha = mLayer->mAlpha;
	if (aLayerAlpha != 0xff)
	{
		FillRectAlpha(aRect, aLayerAlpha, theColour, theRelative);
		return;
	}
	FillRect(aRect, theColour, theRelative);
}

// 0x41f8a0
void Object::DrawFrame(const Rect& theRect, uint32_t theColour, bool theRelative)
{
	DrawLine(theRect.left, theRect.top, theRect.right, theRect.top, theColour, theRelative);
	DrawLine(theRect.right, theRect.top, theRect.right, theRect.bottom, theColour, theRelative);
	DrawLine(theRect.left, theRect.bottom, theRect.right + 1, theRect.bottom, theColour, theRelative);
	DrawLine(theRect.left, theRect.top, theRect.left, theRect.bottom, theColour, theRelative);
}

// 0x41f910
void Object::AddDirty(const Rect& theRect)
{
	if (mTrackDirty)
		UnionRect(mDirty, mDirty, theRect);
}

// 0x41f930
void Object::Create(Manager* theManager, Layer* theLayer, int theX, int theY, Object* theParent)
{
	mLayer = theLayer;
	mManager = theManager;
	mParent = theParent;
	mX = theX;
	mY = theY;
	mStart = ManagerTime();
	Init();
}

// 0x41f970
void Object::AddChild(Object* theChild, int theX, int theY)
{
	theChild->Create(mManager, mLayer, theX, theY, this);
	mChildren.push_back(theChild);
}

// 0x41fb30
void Object::Attach(Attachment* theAttachment)
{
	theAttachment->mOwner = this;
	theAttachment->Load();
	mAttachments.push_back(theAttachment);
}

// 0x41fcf0
void Object::DrawTree()
{
	SetRect(mDirty, 0, 0, 0, 0);
	if (mVisible && mVisibleByParent)
	{
		Draw();
		for (size_t i = 0; i < mAttachments.size(); i++)
			mAttachments[i]->Draw();
		for (size_t i = 0; i < mChildren.size(); i++)
			mChildren[i]->DrawTree();
	}
}

// 0x41fd70
void Object::RegHover(bool theOn)
{
	if (mHoverReg != theOn)
	{
		mHoverReg = theOn;
		if (theOn)
		{
			mManager->AddHover(this);
			return;
		}
		mManager->RemoveHover(this);
	}
}

// 0x41fda0
void Object::RegLayerCmd(bool theOn)
{
	if (mLayerCmdReg != theOn)
	{
		mLayerCmdReg = theOn;
		if (theOn)
		{
			mLayer->AddCmdListener(this);
			return;
		}
		mLayer->RemoveCmdListener(this);
	}
}

// 0x41fdd0
void Object::RegClick(bool theOn)
{
	if (mClickReg != theOn)
	{
		mClickReg = theOn;
		if (theOn)
		{
			mManager->AddClick(this);
			return;
		}
		mManager->RemoveClick(this);
	}
}

// 0x41fe00
void Object::RegMove(bool theOn)
{
	if (mMoveReg != theOn)
	{
		mMoveReg = theOn;
		if (theOn)
		{
			mManager->AddMove(this);
			return;
		}
		mManager->RemoveMove(this);
	}
}

// 0x41fe30
void Object::RegKey(bool theOn)
{
	if (mKeyReg != theOn)
	{
		mKeyReg = theOn;
		if (theOn)
		{
			mManager->AddKey(this);
			return;
		}
		mManager->RemoveKey(this);
	}
}

// 0x41fe60
void Object::RegChar(bool theOn)
{
	if (mCharReg != theOn)
	{
		mCharReg = theOn;
		if (theOn)
		{
			mManager->AddChar(this);
			return;
		}
		mManager->RemoveChar(this);
	}
}

// 0x41fe90
void Object::RegUpdate(bool theOn)
{
	if (mUpdateReg != theOn)
	{
		mUpdateReg = theOn;
		if (theOn)
		{
			mLastUpdate = GetTime();
			mManager->AddUpdater(this);
			mDelta = 0;
			return;
		}
		mManager->RemoveUpdater(this);
		mDelta = 0;
	}
}

// 0x41fee0
void Object::GetScreenPos(int* thePos)
{
	Object* aParent = mParent;
	mLayer->GetPos(thePos);
	for (; aParent != nullptr; aParent = aParent->mParent)
	{
		int aPos[2];
		aParent->GetPos(aPos);
		thePos[0] = thePos[0] + aPos[0];
		thePos[1] = thePos[1] + aPos[1];
	}
	thePos[0] = thePos[0] + mX;
	thePos[1] = thePos[1] + mY;
}

// 0x41ff90
void Object::GetTreeDirty(Rect& theRect)
{
	Rect aRect = mDirty;
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		Rect aChild;
		mChildren[i]->GetTreeDirty(aChild);
		UnionRect(aRect, aRect, aChild);
	}
	theRect = aRect;
}

// 0x420000
bool Object::DispatchMouse(int theMsg, int theFlags, int theX, int theY)
{
	int aPos[2];
	GetScreenPos(aPos);
	int aX = theX - aPos[0];
	int aY = theY - aPos[1];
	if (!TakesInput())
		return false;
	switch (theMsg - 0x201)
	{
	case 0:
		return OnLButtonDown(theFlags, aX, aY);
	case 1:
		return OnLButtonUp(theFlags, aX, aY);
	case 2:
		return OnLButtonDblClk(theFlags, aX, aY);
	case 3:
		return OnRButtonDown(theFlags, aX, aY);
	case 4:
		return OnRButtonUp(theFlags, aX, aY);
	case 5:
		return OnRButtonDblClk(theFlags, aX, aY);
	}
	return false;
}

// 0x420100
void Object::DispatchMouseMove(int theFlags, int theX, int theY)
{
	if (TakesInput())
	{
		int aPos[2];
		GetScreenPos(aPos);
		OnMouseMove(theFlags, theX - aPos[0], theY - aPos[1]);
	}
}

// 0x420150
void Object::DispatchDragEnd(int theFlags, int theX, int theY)
{
	int aPos[2];
	GetScreenPos(aPos);
	OnDragEnd(theFlags, theX - aPos[0], theY - aPos[1]);
}

// 0x420190
void Object::DispatchDrag(int theFlags, int theX, int theY)
{
	int aPos[2];
	GetScreenPos(aPos);
	OnDrag(theFlags, theX - aPos[0], theY - aPos[1]);
}

// 0x4201d0
int Object::ManagerTime()
{
	return mManager->GetClock();
}

// 0x4201e0
int Object::GetTime()
{
	if (IsEnabled())
		return ManagerTime() - mStart;
	return mPauseStart - mStart;
}

// 0x420220
void Object::Capture(bool theOn)
{
	mManager->SetCapture(this, theOn);
}

// 0x420240
bool Object::DoUpdate(int theClock)
{
	if (!TakesInput())
		return false;
	bool aChanged = false;
	mDelta = (theClock - mLastUpdate) - mStart;
	mLastUpdate = theClock - mStart;
	ListNode* aNode = mTimers;
	while (!gLayerDeleted && aNode != nullptr)
	{
		ObjTimer* aTimer = (ObjTimer*)aNode->mObj;
		aNode = aNode->mNext;
		if (StepTimer(aTimer, theClock - mStart))
		{
			ListRemove(mTimers, aTimer);
			delete aTimer;
		}
	}
	if (gLayerDeleted)
		return aChanged;
	for (size_t i = 0; i < mAttachments.size(); i++)
	{
		if (mAttachments[i]->Update(theClock))
			aChanged = true;
		if (gLayerDeleted)
			return aChanged;
	}
	if (Update())
		aChanged = true;
	return aChanged;
}

// 0x420360
void Object::AddTimer(int theId, int thePeriod, int theCount, int thePerTick)
{
	ObjTimer* aTimer = new ObjTimer;
	aTimer->mId = theId;
	aTimer->mLast = GetTime();
	aTimer->mPeriod = thePeriod;
	aTimer->mCount = theCount;
	aTimer->mLeft = theCount;
	aTimer->mPerTick = thePerTick;
	ListAdd(mTimers, aTimer);
}

// 0x4203b0 (the period is compared unsigned, as the original)
bool Object::StepTimer(ObjTimer* theTimer, int theTime)
{
	if ((uint32_t)theTimer->mPeriod < (uint32_t)(theTime - theTimer->mLast))
	{
		theTimer->mLast = theTimer->mPeriod + theTimer->mLast;
		int i = 0;
		if (0 < theTimer->mPerTick)
		{
			do
			{
				if (FireTimer(theTimer))
					return true;
				i = i + 1;
			} while (i < theTimer->mPerTick);
		}
	}
	return false;
}

// 0x420400
bool Object::FireTimer(ObjTimer* theTimer)
{
	int aLeft = theTimer->mLeft;
	theTimer->mLeft = aLeft - 1;
	if (!OnTimer(theTimer->mId, (theTimer->mCount - (aLeft - 1)) - 1))
		theTimer->mLeft = 0;
	return theTimer->mLeft == 0;
}

// 0x420570
bool Object::DispatchCommand(int theCmd)
{
	if (TakesInput())
		return OnCommand(theCmd);
	return false;
}

// 0x4205a0
void Object::DispatchHover(bool theIn, int theFlags)
{
	if (TakesInput())
		OnHover(theIn, theFlags);
}

// 0x4205d0
bool Object::DispatchKey(int theKey, int theRepeat, int theScan)
{
	if (TakesInput())
		return OnKeyDown(theKey, theRepeat, theScan);
	return false;
}

// 0x420600
void Object::DispatchChar(int theChar, int theRepeat, int theScan)
{
	if (TakesInput())
		OnChar(theChar, theRepeat, theScan);
}

// 0x4206c0
bool Object::TakesInput() const
{
	if (!IsActive() && mIgnoreWhenDisabled)
		return false;
	return true;
}

// 0x40a4e0
void Object::SetPos(int theX, int theY)
{
	mX = theX;
	mY = theY;
}

// 0x420650
void LoadSound(int theId, bool theLoop)
{
	gManager->LoadSound(theId, theLoop);
}

// 0x420670
void PlaySound(int theId)
{
	gManager->PlaySound(theId);
}

// 0x420680
void StopSound(int theId)
{
	gManager->StopSound(theId);
}

// 0x4206a0
int Random(int theRange)
{
	if (theRange == 0)
		return 0;
	return CrtRand() % theRange;
}

// 0x41f680
void GetImageInfo(int theImageId, ImageInfo& theInfo)
{
	gManager->GetImageInfo(theImageId, theInfo);
}

// 0x41f120
Font* GetExeFont(int theImageId, int theTextId)
{
	return gManager->GetFont(theImageId, theTextId, Module::EXE);
}

// 0x41f150
Font* GetDllFont(int theImageId, int theTextId)
{
	return gManager->GetFont(theImageId, theTextId, Module::DLL);
}

// 0x41f180
void FreeImage(int theImageId)
{
	gManager->FreeImage(theImageId);
}

// 0x420440
Image* CreateImage(int theWidth, int theHeight)
{
	return gManager->CreateImage(theWidth, theHeight);
}

// 0x420460
Image* LoadDllImage(int theImageId)
{
	return Display::LoadDllImage(theImageId);
}

// 0x420470
Image* LoadDllImageKeyed(int theImageId, uint32_t theKey)
{
	return Display::LoadDllImageKeyed(theImageId, theKey);
}

// ---------------------------------------------------------------------------------------------------------------

// 0x41e240
Button::Button(int theImage, int theFrames, bool theFixedSize)
	: mImage(theImage), mCmd(theImage), mFrames(theFrames), mFixedSize(theFixedSize)
{
}

// 0x41e310
Button::~Button()
{
}

// 0x41e3c0
void Button::Init()
{
	if (!mFixedSize)
	{
		ImageInfo anInfo;
		GetImageInfo(mImage, anInfo);
		mHeight = anInfo.mHeight;
		mFrameWidth = anInfo.mWidth / mFrames;
	}
	SetCursorKind(1, false);
	RegHover(true);
	RegClick(true);
	OnInit();
}

// 0x41e430
void Button::SetLabel(int theStringId, int theFontImage, int theFontText, int theX, int theRight, int theY,
	int thePressDX, int thePressDY, int theDisabledAlpha)
{
	mLabel = LoadDllString(theStringId);
	mFont = GetDllFont(theFontImage, theFontText);
	mLabelX = theX;
	mLabelRight = theRight;
	mLabelY = theY;
	mPressDX = thePressDX;
	mPressDY = thePressDY;
	mDisabledAlpha = theDisabledAlpha;
	mLabelLines = 0;
}

// 0x41e4b0
void Button::Draw()
{
	if (IsVisible())
	{
		if (!IsEnabled())
		{
			DrawDisabled();
			return;
		}
		if (!mHover)
		{
			DrawNormal();
		}
		else
		{
			if (mPressed)
			{
				DrawPressed();
				return;
			}
			DrawHover();
		}
	}
}

// 0x41e560
void Button::DrawNormal()
{
	Rect aSrc;
	aSrc.right = mFrameWidth;
	if (0 < aSrc.right)
	{
		aSrc.bottom = mHeight;
		aSrc.left = 0;
		aSrc.top = 0;
		DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
		DrawLabel();
	}
}

// 0x41e5c0
void Button::DrawLabel()
{
	if (mFont != nullptr)
	{
		if (mLabelLines == 0)
		{
			mFont->DrawCentred(this, mLabelX, mLabelY, mLabel.c_str(), mLabelRight - mLabelX, 100, 100, 0xff);
			return;
		}
		mFont->DrawLinesCentred(this, mLabelX, mLabelY, mLabel.c_str(), mLabelRight - mLabelX, mLabelLines, 100, 100,
			0xff);
	}
}

// 0x41e660
void Button::DrawHover()
{
	Rect aSrc;
	aSrc.left = mFrameWidth;
	if (0 < aSrc.left && 1 < mFrames)
	{
		aSrc.right = aSrc.left * 2;
		aSrc.bottom = mHeight;
		aSrc.top = 0;
		DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
		DrawLabel();
		return;
	}
	DrawNormal();
}

// 0x41e6d0
void Button::DrawPressed()
{
	Rect aSrc;
	aSrc.right = mFrameWidth;
	if (0 < aSrc.right && 2 < mFrames)
	{
		aSrc.left = aSrc.right * 2;
		aSrc.right = aSrc.right * 3;
		aSrc.bottom = mHeight;
		aSrc.top = 0;
		DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
		DrawLabelPressed();
		return;
	}
	DrawNormal();
}

// 0x41e750
void Button::DrawLabelPressed()
{
	if (mFont != nullptr)
	{
		if (mLabelLines == 0)
		{
			mFont->DrawCentred(this, mPressDX + mLabelX, mPressDY + mLabelY, mLabel.c_str(), mLabelRight - mLabelX,
				100, 100, 0xff);
			return;
		}
		mFont->DrawLinesCentred(this, mPressDX + mLabelX, mPressDY + mLabelY, mLabel.c_str(), mLabelRight - mLabelX,
			mLabelLines, 100, 100, 0xff);
	}
}

// 0x41e810
void Button::DrawDisabled()
{
	Rect aSrc;
	if (mShowDisabled && (aSrc.right = mFrameWidth, 0 < aSrc.right) && 3 < mFrames)
	{
		aSrc.left = aSrc.right * 3;
		aSrc.right = aSrc.right * 4;
		aSrc.bottom = mHeight;
		aSrc.top = 0;
		DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
		DrawLabelDisabled();
		return;
	}
	DrawNormal();
}

// 0x41e890
void Button::DrawLabelDisabled()
{
	if (mFont != nullptr)
	{
		if (mLabelLines == 0)
		{
			mFont->DrawCentred(this, mLabelX, mLabelY, mLabel.c_str(), mLabelRight - mLabelX, 100, 100,
				mDisabledAlpha);
			return;
		}
		mFont->DrawLinesCentred(this, mLabelX, mLabelY, mLabel.c_str(), mLabelRight - mLabelX, mLabelLines, 100, 100,
			mDisabledAlpha);
	}
}

// 0x41e930
bool Button::OnLButtonDown(int theFlags, int theX, int theY)
{
	mPressed = true;
	PlaySound(SOUND_CLICK);
	Capture(true);
	Invalidate();
	return true;
}

// 0x41e960
bool Button::OnLButtonUp(int theFlags, int theX, int theY)
{
	mPressed = false;
	Capture(false);
	if (mHover)
		OnClick();
	Invalidate();
	return true;
}

// 0x41e990
void Button::OnHover(bool theIn, int theFlags)
{
	if (mHover != theIn)
	{
		mHover = theIn;
		Invalidate();
		if (theIn && mHoverSound != 0)
			PlaySound(mHoverSound);
	}
}

// 0x41e9d0
void Button::OnEnabledChanged()
{
	if (!IsEnabled())
	{
		mHover = false;
		mPressed = false;
	}
}

// 0x41e9f0
void Button::OnVisibleChanged()
{
	if (!IsVisible())
	{
		mHover = false;
		mPressed = false;
	}
}

// 0x41ea10
void Button::OnRelease()
{
	mPressed = false;
}

// 0x41ea30
void Button::OnClick()
{
	mLayer->Command(mCmd);
}

// ---------------------------------------------------------------------------------------------------------------

// 0x41ea40
Toggle::Toggle(int theImage, RadioGroup* theGroup, int theId)
	: Button(theImage, 2, false), mGroup(theGroup), mId(theId)
{
	theGroup->Add(this);
}

// 0x41eaf0
void Toggle::DrawNormal()
{
	if (0 < mFrameWidth)
	{
		Rect aSrc;
		if (!IsChecked())
		{
			aSrc.right = mFrameWidth;
			aSrc.bottom = mHeight;
			aSrc.left = 0;
		}
		else
		{
			aSrc.left = mFrameWidth;
			aSrc.right = aSrc.left * 2;
			aSrc.bottom = mHeight;
		}
		aSrc.top = 0;
		DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
		if (mFont != nullptr)
			mFont->DrawLeft(this, mLabelX, mLabelY, mLabel.c_str(), -1, 100, 100, 0xff);
	}
}

// 0x41ebc0
void Toggle::DrawHover()
{
	DrawNormal();
}

// 0x41ebd0
void Toggle::OnClick()
{
	mGroup->Select(mId);
	Button::OnClick();
}

// 0x422260
void RadioGroup::Select(int theId)
{
	if (mSelected != theId)
	{
		mSelected = theId;
		for (size_t i = 0; i < mToggles.size(); i++)
		{
			Toggle* aToggle = mToggles[i];
			aToggle->SetChecked(aToggle->GetId() == theId);
		}
	}
}

// 0x424ae0
CheckBox::CheckBox(int theImage, int theFrames) : Button(theImage, theFrames, false)
{
}

// 0x424b40
void SetCheckLabel(Button* theButton, int theStringId, int theFontImage, int theFontText, int theX, int theY,
	int theDisabledAlpha)
{
	theButton->mLabel = LoadDllString(theStringId);
	theButton->mFont = GetDllFont(theFontImage, theFontText);
	theButton->mLabelX = theX;
	theButton->mLabelY = theY;
	theButton->mDisabledAlpha = theDisabledAlpha;
}

// 0x424b90
void CheckBox::DrawNormal()
{
	if (0 < mFrameWidth)
	{
		if (mChecked)
		{
			int aFrame = 3;
			if (mFrames < 3)
			{
				DrawFrame(1);
				return;
			}
			if (mFrames < 5)
				aFrame = 2;
			DrawFrame(aFrame);
			return;
		}
		DrawFrame(0);
	}
}

// 0x424be0
void CheckBox::DrawHover()
{
	if (mFrames < 3)
	{
		DrawNormal();
		return;
	}
	if (mChecked)
	{
		DrawFrame((4 < mFrames) + 3);
		return;
	}
	DrawFrame(1);
}

// 0x424c20
void CheckBox::DrawPressed()
{
	DrawHover();
}

// 0x424c30
void CheckBox::DrawDisabled()
{
	if (4 < mFrames && mShowDisabled)
	{
		if (mChecked)
		{
			DrawFrame(5);
			return;
		}
		DrawFrame(2);
		return;
	}
	DrawNormal();
}

// 0x424c70
void CheckBox::OnClick()
{
	mChecked = !mChecked;
	Button::OnClick();
}

// 0x424c90
void CheckBox::DrawFrame(int theFrame)
{
	Rect aSrc;
	aSrc.left = mFrameWidth * theFrame;
	aSrc.right = mFrameWidth + aSrc.left;
	aSrc.bottom = mHeight;
	aSrc.top = 0;
	DrawImage(0, 0, mImage, &aSrc, true, 100, 100, 0xff);
	if (mFont != nullptr)
		mFont->DrawLeft(this, mLabelX, mLabelY, mLabel.c_str(), -1, 100, 100, 0xff);
}

} // namespace jb
