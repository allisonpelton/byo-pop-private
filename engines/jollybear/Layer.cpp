#include "Layer.h"

#include "Display.h"
#include "Manager.h"
#include "Object.h"
#include "Widgets.h"

namespace jb
{

// 0x4230b0 (mahjongchina), 0x420840 (mahjongjapan) (the original sets +0x08 and +0x34 from the low byte of the x)
Layer::Layer(int theX, int theY)
	: mXLowByteSet((char)theX != 0), mX(theX), mY(theY), mXLowByteSetToo((char)theX != 0)
{
}

// 0x423140 (mahjongchina), 0x4208d0 (mahjongjapan)
Layer::~Layer()
{
	DestroyObjects();
}

// 0x4231c0 (mahjongchina), 0x420950 (mahjongjapan)
void Layer::DestroyObjects()
{
	for (size_t i = 0; i < mObjects.size(); i++)
	{
		Object* anObject = mObjects[i];
		anObject->DestroyTree();
		delete anObject;
	}
	mObjects.clear();
	mFocus.clear();
}

// 0x420a20 (mahjongjapan)
bool Layer::AppActive()
{
	return mManager->GetActive();
}

// 0x423250 (mahjongchina), 0x420a90 (mahjongjapan)
void Layer::SetManager(Manager* theManager)
{
	mManager = theManager;
	Load();
}

// 0x423280 (mahjongchina), 0x420ac0 (mahjongjapan)
void Layer::Add(Object* theObject, int theX, int theY)
{
	theObject->Create(mManager, this, theX, theY, nullptr);
	mObjects.push_back(theObject);
}

// 0x423440 (mahjongchina), 0x420c80 (mahjongjapan)
void Layer::AddFocusable(Object* theObject, int theX, int theY)
{
	Add(theObject, theX, theY);
	AddFocus(theObject);
}

// 0x423470 (mahjongchina), 0x420cb0 (mahjongjapan)
void Layer::Draw()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		mObjects[i]->DrawTree();
	Rect aBounds;
	GetBounds(aBounds);
	if (!mActive && mDarkens)
		FillRectAlpha(aBounds, mDarken, 0);
}

// 0x4234d0 (mahjongchina), 0x420d10 (mahjongjapan)
bool Layer::StepDarken(int theStep)
{
	if (!mActive && mDarkens)
	{
		mDarken = mDarken + theStep;
		if (mDarken < 0x81)
			return true;
		mDarken = 0x80;
	}
	return false;
}

// 0x423510 (mahjongchina), 0x420d50 (mahjongjapan)
void Layer::StartFade(int theFade)
{
	mFade = theFade;
	if (theFade == 2)
	{
		mAlpha = 0;
		mFadeSkip = true;
		return;
	}
	mAlpha = 0xff;
	mFadeSkip = true;
}

// 0x423540 (mahjongchina), 0x420d80 (mahjongjapan)
bool Layer::StepFade(int theStep, bool* theDelete)
{
	if (!mFadeSkip)
	{
		if (mFade == 2)
		{
			mAlpha = mAlpha + theStep;
			if (0xfe < mAlpha)
			{
				mAlpha = 0xff;
				*theDelete = false;
				mFade = 0;
				return false;
			}
		}
		else
		{
			mAlpha = mAlpha - theStep;
			if (mAlpha < 1)
			{
				mAlpha = 0;
				*theDelete = true;
				mFade = 0;
				return false;
			}
		}
	}
	else
	{
		mFadeSkip = false;
	}
	return true;
}

// 0x4235b0 (mahjongchina), 0x420df0 (mahjongjapan)
void Layer::SetActive(bool theActive, bool theDarken)
{
	if (mActive != theActive)
	{
		mActive = theActive;
		mDarkens = theDarken;
		if (theDarken)
		{
			mManager->AddAnimator(this);
			mDarken = 0x1c;
		}
		if (mPauseSounds)
			mManager->PauseSounds(!theActive, false, 0x69);
		BeforeActiveChange();
		for (size_t i = 0; i < mObjects.size(); i++)
			mObjects[i]->SetEnabledByParent(theActive);
		mManager->RedoHover();
		AfterActiveChange();
	}
}

// 0x423630 (mahjongchina), 0x420e70 (mahjongjapan)
void Layer::GetBounds(Rect& theRect)
{
	Rect aRect;
	SetRect(aRect, 0, 0, 0, 0);
	for (size_t i = 0; i < mObjects.size(); i++)
	{
		Rect anObject;
		mObjects[i]->GetTreeDirty(anObject);
		UnionRect(aRect, aRect, anObject);
	}
	theRect = aRect;
}

// 0x4236a0 (mahjongchina), 0x420ee0 (mahjongjapan)
void Layer::SetClip(RegionRect* theRegion)
{
	for (size_t i = 0; i < mObjects.size(); i++)
		mObjects[i]->SetClip(theRegion);
}

// 0x4236d0 (mahjongchina), 0x420f10 (mahjongjapan)
void Layer::AppActivate(bool theActive)
{
	if (mFocusIndex != -1)
	{
		Object* aFocus = mFocus.at(mFocusIndex);
		if (aFocus->IsEnabled())
		{
			SetObjectFocus(mFocus.at(mFocusIndex), theActive);
			if (!theActive)
				Invalidate();
		}
	}
	OnAppActivate(theActive);
}

// 0x423760 (mahjongchina), 0x420fa0 (mahjongjapan)
void Layer::AddFocus(Object* theObject)
{
	mFocus.push_back(theObject);
	if (mFocusIndex == -1)
	{
		mFocusIndex = 0;
		SetObjectFocus(theObject, true);
	}
}

// 0x423920 (mahjongchina), 0x421160 (mahjongjapan)
void Layer::FocusNext()
{
	if (IsActive() && 1 < (int)mFocus.size())
	{
		int aCount = (int)mFocus.size();
		SetObjectFocus(mFocus.at(mFocusIndex), false);
		int anIndex = mFocusIndex + 1;
		if (aCount <= anIndex)
			anIndex = 0;
		mFocusIndex = anIndex;
		SetObjectFocus(mFocus.at(anIndex), true);
	}
}

// 0x4239b0 (mahjongchina), 0x4211f0 (mahjongjapan)
void Layer::FocusPrev()
{
	if (IsActive() && 1 < (int)mFocus.size())
	{
		int aCount = (int)mFocus.size();
		SetObjectFocus(mFocus.at(mFocusIndex), false);
		if (mFocusIndex == 0)
			mFocusIndex = aCount - 1;
		else
			mFocusIndex = mFocusIndex - 1;
		SetObjectFocus(mFocus.at(mFocusIndex), true);
	}
}

// 0x423a40 (mahjongchina), 0x421280 (mahjongjapan)
void Layer::SetFocus(Object* theObject)
{
	if (IsActive())
	{
		int anIndex = 0;
		size_t i = 0;
		for (; i < mFocus.size() && mFocus[i] != theObject; i++)
			anIndex = anIndex + 1;
		if (anIndex != (int)mFocus.size())
		{
			SetObjectFocus(mFocus.at(mFocusIndex), false);
			mFocusIndex = anIndex;
			SetObjectFocus(mFocus.at(anIndex), true);
		}
	}
}

// 0x423af0 (mahjongchina), 0x421330 (mahjongjapan)
void Layer::Command(int theCmd)
{
	if (!OnCommand(theCmd))
	{
		ListNode* aNode = mCmdListeners;
		while (aNode != nullptr && !((Object*)aNode->mObj)->DispatchCommand(theCmd))
			aNode = aNode->mNext;
	}
}

} // namespace jb
