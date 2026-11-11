#include "Manager.h"

#include "App.h"
#include "Display.h"
#include "Host.h"
#include "Layer.h"
#include "Object.h"
#include "Sound.h"

namespace jb
{

Manager* gManager = nullptr;
bool gLayerDeleted = false;
bool gInvalidated = false;
// 0x5d8ea0: the pauses asked for (0x42c110)
static int gPauseRequests = 0;
// 0x527ba8: the full screen's "windowed" button (image 0x35 at 785, 4)
static const Rect kWindowedButton = {784, 0, 800, 16};

static bool InList(ListNode* theHead, void* theObj)
{
	return ListContains(theHead, theObj);
}

// 0x42a0d0
Manager::Manager()
{
	mSound = new Sound();
	mDisplay = new Display();
}

// 0x42a230
Manager::~Manager()
{
	if (mRunning)
		Stop();
	delete mSound;
	mSound = nullptr;
	delete mDisplay;
	mDisplay = nullptr;
	if (gManager == this)
		gManager = nullptr;
}

// 0x42a2c0
void Manager::Stop()
{
	mDisplay->Release();
	mPaused = true;
	mRunning = false;
	ClearLayers(false);
	delete mNextLayer;
	mNextLayer = nullptr;
	mSound->Shutdown();
	ListClear(mAnimators);
}

// 0x42a340
bool Manager::IsAppActive()
{
	return mDisplay->mActive;
}

// 0x42a5c0
void Manager::Shutdown()
{
	for (size_t i = 0; i < mLayers.size(); i++)
		mLayers[i]->OnRemove();
	Stop();
}

// 0x42a5f0 (the cursors are the host's; the window's timer is the host's frame)
bool Manager::Init(bool theFullScreen, int thePeriod)
{
	mPeriod = thePeriod;
	mLastFrame = TickCount();
	if (!SetMode(theFullScreen, true))
		return false;
	PreloadExeImage(0x35);
	PreloadExeImage(0x46);
	PreloadExeImage(0x47);
	PreloadExeImage(0x49);
	PreloadExeImage(0x48);
	PreloadExeImage(0x4a);
	mSound->Init();
	mStart = TickCount();
	mPaceBase = TickCount();
	mPaceCount = 0;
	mPaused = false;
	mRunning = true;
	gManager = this;
	return true;
}

// 0x42c300 (with theUseSetting the registry's HardwareAcceleration, else the display's state, decides whether a
// failed mode is tried again without acceleration)
bool Manager::SetMode(bool theFullScreen, bool theUseSetting)
{
	bool aRetry = theFullScreen && (theUseSetting ? gHardwareAccel : mDisplay->mAccelerated);
	int aResult = mDisplay->SetMode(theFullScreen, false);
	if (aResult < 0 && aRetry)
		aResult = mDisplay->SetMode(theFullScreen, true);
	return 0 <= aResult;
}

// 0x42a800
void Manager::PresentDone()
{
	mDisplay->PresentDone();
	mLastPresent = TickCount();
}

// 0x42a820 (the software cursor of the accelerated display is the host's cursor in the port)
void Manager::Draw()
{
	if (mRunning && !mPaused)
	{
		if (!gInvalidated)
			return;
		if (mDisplay->BeginFrame())
		{
			if (mFade != 3)
			{
				size_t aFirst = 0;
				for (size_t i = 0; i < mLayers.size(); i++)
				{
					if (mLayers[i]->IsOpaque())
						aFirst = i;
				}
				for (size_t i = aFirst; i < mLayers.size(); i++)
					mLayers[i]->Draw();
			}
			if (mFade != 0)
			{
				Rect aRect;
				aRect.left = 0;
				aRect.top = 0;
				aRect.right = 800;
				aRect.bottom = 600;
				if (mFadeAlpha == 0)
					FillRect(&aRect, 0);
				else
					FillRectAlpha(aRect, 0xff - mFadeAlpha, 0);
			}
			if (gFullScreen)
			{
				Rect anOut;
				mDisplay->DrawById(0x311, 4, 0x35, nullptr, 100, 100, 0xff, anOut);
			}
			mDisplay->Present();
			PresentDone();
			gInvalidated = false;
			if (mRedoHover)
			{
				mRedoHover = false;
				int aX, aY;
				bool anOutside;
				GetHost()->GetMousePos(aX, aY, anOutside);
				MouseMove(GetHost()->InputFlags(), aX, aY);
			}
		}
	}
}

// 0x42a9f0
void Manager::Push(Layer* theLayer, bool theOverlay, bool theDarken, bool theAnimate)
{
	if (!theOverlay)
		theLayer->SetOpaque(true);
	if (mCapture != nullptr)
	{
		mCapture->OnRelease();
		mCapture = nullptr;
	}
	mHover = nullptr;
	SetCursor(4);
	DeactivateTop(theDarken);
	theLayer->SetManager(this);
	mLayers.push_back(theLayer);
	if (theAnimate)
	{
		ListAdd(mTransitions, theLayer);
		theLayer->StartFade(2);
	}
	RedoHover();
	Invalidate();
}

// 0x42ac00
void Manager::DeactivateTop(bool theDarken)
{
	size_t i = mLayers.size();
	while (i != 0)
	{
		Layer* aLayer = mLayers[i - 1];
		if (!InList(mTransitions, aLayer) || !aLayer->IsFadingOut())
		{
			aLayer->SetActive(false, theDarken);
			return;
		}
		i = i - 1;
	}
}

// 0x42ac60
Layer* Manager::Pop(bool theAnimate, int theCount)
{
	mHover = nullptr;
	mCapture = nullptr;
	Layer* aTop;
	if (!theAnimate)
		aTop = PopNow(theCount);
	else
		aTop = PopAnimated(theCount);
	if (aTop != nullptr)
		aTop->SetActive(true, true);
	Invalidate();
	return aTop;
}

// 0x42acb0 (with an empty stack the original read past it)
Layer* Manager::PopNow(int theCount)
{
	if (mLayers.empty())
		return nullptr;
	int aCount = 0;
	int p = (int)mLayers.size() - 1;
	if (0 < theCount)
	{
		do
		{
			if (p < 0)
				return nullptr;
			Layer* aLayer = mLayers[p];
			if (!InList(mTransitions, aLayer) || !aLayer->IsFadingOut())
			{
				DeleteLayer(aLayer, true);
				mLayers.erase(mLayers.begin() + p);
				aCount = aCount + 1;
			}
			p = p - 1;
		} while (aCount < theCount);
	}
	while (0 <= p)
	{
		Layer* aLayer = mLayers[p];
		if (!InList(mTransitions, aLayer) || !aLayer->IsFadingOut())
			return aLayer;
		p = p - 1;
	}
	return nullptr;
}

// 0x42adb0
Layer* Manager::PopAnimated(int theCount)
{
	int aCount = 0;
	size_t p = mLayers.size();
	if (0 < theCount)
	{
		do
		{
			if (p == 0)
				return nullptr;
			Layer* aLayer = mLayers[p - 1];
			if (!InList(mTransitions, aLayer) || !aLayer->IsFadingOut())
			{
				aLayer->SetActive(false, false);
				if (!InList(mTransitions, aLayer))
					ListAdd(mTransitions, aLayer);
				aLayer->StartFade(1);
				aCount = aCount + 1;
			}
			p = p - 1;
		} while (aCount < theCount);
	}
	while (p != 0)
	{
		Layer* aLayer = mLayers[p - 1];
		if (!InList(mTransitions, aLayer) || !aLayer->IsFadingOut())
			return aLayer;
		p = p - 1;
	}
	return nullptr;
}

bool gFastStart = false;

// 0x42ae80
void Manager::SwitchTo(Layer* theLayer)
{
	mCapture = nullptr;
	mHover = nullptr;
	SetCursor(4);
	DeactivateTop(false);
	mFade = 1;
	mFadeAlpha = 0xff;
	mNextLayer = theLayer;
	// port: the first screen has nothing to fade out of, and the original lets 0.77 s of black pass before it
	if (gFastStart && mLayers.empty())
	{
		mFade = 3;
		mFadeAlpha = 0;
	}
}

// 0x42aed0
void Manager::SwitchNow()
{
	if (mNextLayer != nullptr)
	{
		ClearLayers(true);
		mNextLayer->SetManager(this);
		mLayers.push_back(mNextLayer);
		RedoHover();
		mFade = 2;
		mFadeAlpha = 0;
		mFadeSkip = true;
		mNextLayer->OnEntered();
		mNextLayer = nullptr;
	}
}

// 0x42b0b0
void Manager::FadeDone()
{
	mFade = 0;
	EndWait();
	mLayers.back()->OnShown();
}

// 0x42b0d0
void Manager::Replace(Layer* theLayer, bool theAnimateOut, bool theAnimateIn)
{
	mCapture = nullptr;
	mHover = nullptr;
	SetCursor(4);
	if (!theAnimateOut)
		PopNow(1);
	else
		PopAnimated(1);
	theLayer->SetManager(this);
	mLayers.push_back(theLayer);
	if (theAnimateIn)
	{
		ListAdd(mTransitions, theLayer);
		theLayer->StartFade(2);
	}
	RedoHover();
	Invalidate();
}

// 0x42b2d0
void Manager::DeleteLayer(Layer* theLayer, bool theNotify)
{
	ListRemove(mTransitions, theLayer);
	if (theNotify)
		theLayer->OnRemove();
	delete theLayer;
	gLayerDeleted = true;
}

// 0x42b340
void Manager::ClearLayers(bool theNotify)
{
	for (size_t i = 0; i < mLayers.size(); i++)
		DeleteLayer(mLayers[i], theNotify);
	mLayers.clear();
}

// 0x42b3b0 (0x42cb90's mapping of a stretched full screen to 800x600 is the host's)
void Manager::MouseMove(int theFlags, int theX, int theY)
{
	mCursorLocked = false;
	if (mDrag != nullptr)
	{
		if ((mDragFlags & 1) != 0)
			DragTo(theX, theY);
		if ((mDragFlags & 2) != 0)
			mDrag->DispatchDrag(theFlags, theX, theY);
		if ((uint32_t)(mPeriod * 2) < TickCount() - mLastFrame)
		{
			Frame();
			Draw();
			return;
		}
		Draw();
		return;
	}
	bool aOnButton = false;
	bool aButton = false;
	if (!gFullScreen || !PtInRect(kWindowedButton, theX, theY))
	{
		aOnButton = aButton;
		if (mCapture == nullptr)
		{
			for (ListNode* aNode = mMoves; aNode != nullptr; aNode = aNode->mNext)
			{
				Object* anObject = (Object*)aNode->mObj;
				if (PtInRect(anObject->mDirty, theX, theY))
				{
					anObject->DispatchMouseMove(theFlags, theX, theY);
					break;
				}
			}
		}
	}
	else if (mCapture == nullptr)
	{
		SetCursor(1);
		aButton = true;
		aOnButton = aButton;
		for (ListNode* aNode = mMoves; aNode != nullptr; aNode = aNode->mNext)
		{
			Object* anObject = (Object*)aNode->mObj;
			if (PtInRect(anObject->mDirty, theX, theY))
			{
				anObject->DispatchMouseMove(theFlags, theX, theY);
				break;
			}
		}
	}
	for (ListNode* aNode = mHovers; aNode != nullptr; aNode = aNode->mNext)
	{
		Object* anObject = (Object*)aNode->mObj;
		if (PtInRect(anObject->mDirty, theX, theY) && anObject->IsActive())
		{
			Object* aHover = mHover;
			if (aHover != anObject)
			{
				if (aHover != nullptr)
				{
					mNextHover = anObject;
					if (aHover->IsActive())
						SendHover(false, theFlags);
				}
				mHover = anObject;
				SendHover(true, theFlags);
			}
			if ((mCapture == nullptr || mHover == mCapture) && !mCursorLocked && !aOnButton)
			{
				SetCursor(mHover->mCursor);
				Draw();
				return;
			}
			Draw();
			return;
		}
	}
	if (!mCursorLocked && !aOnButton)
		SetCursor(0);
	if (mHover != nullptr)
	{
		mNextHover = nullptr;
		if (mHover->IsActive())
			SendHover(false, theFlags);
		mHover = nullptr;
	}
	Draw();
}

// 0x42b5c0
void Manager::SendHover(bool theIn, int theFlags)
{
	if (mCapture == nullptr || mHover == mCapture)
		mHover->DispatchHover(theIn, theFlags);
}

// 0x42b5f0
void Manager::SetObjectCursor(Object* theObject, int theCursor, bool theLock)
{
	if (mHover == theObject || theLock)
		SetCursor(theCursor);
	mCursorLocked = theLock;
}

// 0x42b620
void Manager::LButtonDown(int theFlags, int theX, int theY)
{
	if (mCapture == nullptr)
	{
		ListNode* aNode = mClicks;
		while (aNode != nullptr && (!PtInRect(((Object*)aNode->mObj)->mDirty, theX, theY) ||
			!((Object*)aNode->mObj)->DispatchMouse(0x201, theFlags, theX, theY)))
			aNode = aNode->mNext;
		if (gFullScreen && PtInRect(kWindowedButton, theX, theY))
			SetFullScreen(false, true);
		Draw();
		return;
	}
	mCapture->DispatchMouse(0x201, theFlags, theX, theY);
	Draw();
}

// 0x42b6e0
void Manager::LButtonDbl(int theFlags, int theX, int theY)
{
	if (mCapture != nullptr)
	{
		mCapture->DispatchMouse(0x203, theFlags, theX, theY);
		Draw();
		return;
	}
	for (ListNode* aNode = mClicks; aNode != nullptr; aNode = aNode->mNext)
	{
		Object* anObject = (Object*)aNode->mObj;
		if (PtInRect(anObject->mDirty, theX, theY) && anObject->GetUserFlag() &&
			anObject->DispatchMouse(0x203, theFlags, theX, theY))
		{
			Draw();
			return;
		}
	}
	LButtonDown(theFlags, theX, theY);
	Draw();
}

// 0x42b7a0
void Manager::StartDrag(Object* theObject, int theFlags, int theLeft, int theTop, int theRight, int theBottom)
{
	if (mCapture != nullptr)
	{
		mCapture->OnRelease();
		mCapture = nullptr;
	}
	GetHost()->CaptureMouse(true);
	mDrag = theObject;
	mDragFlags = theFlags;
	int aPos[2];
	theObject->GetPos(aPos);
	mDragOrigin.x = aPos[0];
	mDragOrigin.y = aPos[1];
	mDragPos = mDragOrigin;
	bool anOutside;
	GetHost()->GetMousePos(mDragStart.x, mDragStart.y, anOutside);
	SetRect(mDragClamp, theLeft, theTop, theRight, theBottom);
}

// 0x42b840
void Manager::LButtonUp(int theFlags, int theX, int theY)
{
	if (mCapture == nullptr)
	{
		if (mDrag != nullptr)
		{
			GetHost()->CaptureMouse(false);
			mDrag->DispatchDragEnd(theFlags, theX, theY);
			mDrag = nullptr;
		}
		ListNode* aNode = mClicks;
		while (aNode != nullptr && (!PtInRect(((Object*)aNode->mObj)->mDirty, theX, theY) ||
			!((Object*)aNode->mObj)->DispatchMouse(0x202, theFlags, theX, theY)))
			aNode = aNode->mNext;
		Draw();
		return;
	}
	mCapture->DispatchMouse(0x202, theFlags, theX, theY);
	Draw();
}

// 0x42b910
void Manager::RButtonDown(int theFlags, int theX, int theY)
{
	if (mCapture == nullptr)
	{
		ListNode* aNode = mClicks;
		while (aNode != nullptr && (!PtInRect(((Object*)aNode->mObj)->mDirty, theX, theY) ||
			!((Object*)aNode->mObj)->DispatchMouse(0x204, theFlags, theX, theY)))
			aNode = aNode->mNext;
		Draw();
		return;
	}
	mCapture->DispatchMouse(0x204, theFlags, theX, theY);
	Draw();
}

// 0x42b9b0
void Manager::RButtonDbl(int theFlags, int theX, int theY)
{
	if (mCapture != nullptr)
	{
		mCapture->DispatchMouse(0x206, theFlags, theX, theY);
		Draw();
		return;
	}
	for (ListNode* aNode = mClicks; aNode != nullptr; aNode = aNode->mNext)
	{
		Object* anObject = (Object*)aNode->mObj;
		if (PtInRect(anObject->mDirty, theX, theY) && anObject->GetUserFlag() &&
			anObject->DispatchMouse(0x206, theFlags, theX, theY))
		{
			Draw();
			return;
		}
	}
	RButtonDown(theFlags, theX, theY);
	Draw();
}

// 0x42ba70
void Manager::RButtonUp(int theFlags, int theX, int theY)
{
	if (mCapture == nullptr)
	{
		ListNode* aNode = mClicks;
		while (aNode != nullptr && (!PtInRect(((Object*)aNode->mObj)->mDirty, theX, theY) ||
			!((Object*)aNode->mObj)->DispatchMouse(0x205, theFlags, theX, theY)))
			aNode = aNode->mNext;
		Draw();
		return;
	}
	mCapture->DispatchMouse(0x205, theFlags, theX, theY);
	Draw();
}

// 0x42bb10
void Manager::DragTo(int theX, int theY)
{
	int aX = (mDragOrigin.x - mDragStart.x) + theX;
	int aY = (mDragOrigin.y - mDragStart.y) + theY;
	int aClamp = mDragClamp.left;
	if (aX < mDragClamp.left || (aClamp = mDragClamp.right, mDragClamp.right < aX))
		aX = aClamp;
	aClamp = mDragClamp.top;
	if (aY < mDragClamp.top || (aClamp = mDragClamp.bottom, mDragClamp.bottom < aY))
		aY = aClamp;
	if (mDragPos.x != aX || mDragPos.y != aY)
	{
		mDragPos.x = aX;
		mDragPos.y = aY;
		mDrag->SetPos(aX, aY);
		Invalidate();
	}
}

// 0x42bb90
void Manager::Activate(bool theActive)
{
	if (mActive != theActive)
	{
		mActive = theActive;
		if (!theActive)
		{
			if (mHover != nullptr)
			{
				SetCursor(0);
				if (mHovers != nullptr)
					SendHover(false, GetHost()->InputFlags());
				mHover = nullptr;
			}
			if (mCapture != nullptr)
			{
				mCapture->OnRelease();
				mCapture = nullptr;
			}
		}
		if (!mLayers.empty())
		{
			size_t i = mLayers.size();
			do
			{
				Layer* aLayer = mLayers[i - 1];
				if (!aLayer->IsFadingOut())
					aLayer->AppActivate(theActive);
				i = i - 1;
			} while (i != 0);
		}
		mSound->Activate(theActive);
		Draw();
	}
}

// 0x42bc60
void Manager::Key(int theKey, int theRepeat, int theScan)
{
	ListNode* aNode = mKeys;
	while (aNode != nullptr)
	{
		Object* anObject = (Object*)aNode->mObj;
		aNode = aNode->mNext;
		if (anObject->DispatchKey(theKey, theRepeat, theScan))
			break;
	}
	Draw();
}

// 0x42bcb0
void Manager::Char(int theChar, int theRepeat, int theScan)
{
	ListNode* aNode = mChars;
	while (aNode != nullptr)
	{
		Object* anObject = (Object*)aNode->mObj;
		aNode = aNode->mNext;
		anObject->DispatchChar(theChar, theRepeat, theScan);
	}
	if (theChar == 9)
	{
		Layer* aTop = mLayers.back();
		if ((GetHost()->InputFlags() & 4) != 0)
		{
			aTop->FocusPrev();
			Draw();
			return;
		}
		aTop->FocusNext();
	}
	Draw();
}

// 0x42bd40
void Manager::Frame()
{
	if (mRunning && !mPaused)
	{
		uint32_t aNow = TickCount();
		int aClock = (int)(aNow - mStart);
		mClock = aClock;
		Update(aClock, aNow - mLastFrame);
		mLastFrame = aNow;
		Draw();
		if (1000 < aNow - mLastPresent)
			PresentDone();
		mPaceCount = mPaceCount + 1;
		if (mPaceCount == 0)
		{
			mPaceBase = aNow;
			mPaceCount = 0;
		}
		uint32_t aTime = TickCount();
		uint32_t aTarget = mPeriod * (uint32_t)mPaceCount + mPaceBase;
		if (aTime <= aTarget)
		{
			GetHost()->Sleep(aTarget - aTime);
			return;
		}
		mPaceBase = aNow;
		mPaceCount = 0;
	}
}

// 0x42be10
void Manager::Update(int theClock, uint32_t theDelta)
{
	gLayerDeleted = false;
	int aFade = mFade;
	if (aFade != 0)
	{
		if (aFade == 1)
		{
			int anAlpha = mFadeAlpha - (int)(theDelta / 3);
			mFadeAlpha = anAlpha;
			if (anAlpha < 1)
			{
				mFadeAlpha = 0;
				mFade = 3;
			}
		}
		else if (aFade == 3)
		{
			SwitchNow();
		}
		else if (aFade == 2)
		{
			if (!mFadeSkip)
			{
				int anAlpha = mFadeAlpha + (int)(theDelta / 3);
				mFadeAlpha = anAlpha;
				if (0xfe < anAlpha)
				{
					mFadeAlpha = 0xff;
					FadeDone();
				}
			}
			else
			{
				mFadeSkip = false;
			}
		}
		Invalidate();
	}
	ListNode* aNode = mAnimators;
	while (!gLayerDeleted && aNode != nullptr)
	{
		Layer* aLayer = (Layer*)aNode->mObj;
		aNode = aNode->mNext;
		if (!aLayer->StepDarken((int)(theDelta / 3)))
		{
			ListRemove(mAnimators, aLayer);
			if (mAnimators == nullptr)
				EndWait();
		}
		Invalidate();
	}
	aNode = mTransitions;
	while (!gLayerDeleted && aNode != nullptr)
	{
		Layer* aLayer = (Layer*)aNode->mObj;
		aNode = aNode->mNext;
		bool aDelete;
		if (!aLayer->StepFade((int)((theDelta * 3) / 5), &aDelete))
		{
			ListRemove(mTransitions, aLayer);
			if (aDelete)
			{
				DeleteLayer(aLayer, true);
				for (size_t i = 0; i < mLayers.size(); i++)
				{
					if (mLayers[i] == aLayer)
					{
						mLayers.erase(mLayers.begin() + i);
						break;
					}
				}
			}
		}
		Invalidate();
	}
	aNode = mUpdaters;
	bool aChanged = false;
	if (!gLayerDeleted)
	{
		do
		{
			if (aNode == nullptr)
				break;
			Object* anObject = (Object*)aNode->mObj;
			aNode = aNode->mNext;
			if (anObject->DoUpdate(theClock))
				aChanged = true;
		} while (!gLayerDeleted);
		if (aChanged)
			Invalidate();
	}
}

// 0x42c110
void Manager::Pause(bool thePause)
{
	gPauseRequests = gPauseRequests + (thePause ? 1 : -1);
	mPaused = gPauseRequests != 0;
	PauseClock(thePause);
}

// 0x42c140
void Manager::PauseClock(bool thePause)
{
	if (thePause)
	{
		if (mPauseCount == 0)
			mPauseStart = TickCount();
		mPauseCount = mPauseCount + 1;
		return;
	}
	mPauseCount = mPauseCount - 1;
	if (mPauseCount == 0)
		mStart = mStart + (TickCount() - mPauseStart);
}

// 0x42c190
bool Manager::SetHardwareAccel(bool theOn)
{
	if (!IsAppActive())
		return false;
	if (gHardwareAccel != theOn)
	{
		gHardwareAccel = theOn;
		Pause(true);
		if (!SetMode(gFullScreen, true))
		{
			gHardwareAccel = false;
			gApp->Quit();
			return false;
		}
		Pause(false);
	}
	return true;
}

// 0x42c220
bool Manager::SetFullScreen(bool theOn, bool theReset)
{
	if (gFullScreen != theOn)
	{
		Pause(true);
		if (theReset)
		{
			if (!SetMode(theOn, false))
			{
				gApp->Quit();
				return false;
			}
		}
		Pause(false);
	}
	return true;
}

// 0x42c2a0
bool Manager::SetWideScreen(bool theOn)
{
	if (gWideScreen != theOn)
	{
		gWideScreen = theOn;
		Pause(true);
		if (!SetMode(gFullScreen, false))
		{
			gApp->Quit();
			return false;
		}
		Pause(false);
	}
	return true;
}

// 0x42c370
void Manager::LoadSound(int theId, bool theLoop)
{
	mSound->Load(theId, theLoop);
}

// 0x42c390
void Manager::PlaySound(int theId)
{
	mSound->Play(theId);
}

// 0x42c3b0
void Manager::StopSound(int theId)
{
	mSound->Stop(theId);
}

// 0x42c3d0
void Manager::StopSoundsExcept(int theId)
{
	mSound->StopAllExcept(theId);
}

// 0x42c3f0
void Manager::PauseSounds(bool thePause, bool theLoopingOnly, int theExcept)
{
	mSound->Pause(thePause, theLoopingOnly, theExcept);
}

// 0x42c410
void Manager::LoadMusic(int theId)
{
	mSound->LoadMusic(theId);
}

// 0x42c430
void Manager::PlayMusic(int theId, bool theRestart, int theFadeMs, int theOrder)
{
	mSound->PlayMusic(theId, theRestart, theFadeMs, theOrder);
}

// 0x42c460
void Manager::PauseMusic(bool thePause)
{
	mSound->PauseMusic(thePause);
}

// 0x42c480
void Manager::SetSoundVolume(int theVolume)
{
	gSoundsVolume = theVolume;
	if (gSoundsOn)
	{
		mSound->SetVolume(theVolume);
		return;
	}
	mSound->SetVolume(0);
}

// 0x42c4c0
void Manager::SetMusicVolume(int theVolume)
{
	gMusicVolume = theVolume;
	if (gMusicOn)
	{
		mSound->SetMusicVolume(theVolume);
		return;
	}
	mSound->SetMusicVolume(0);
}

// 0x42c500
void Manager::SlideMusic(int theVolume, int theMs)
{
	if (gMusicOn)
		mSound->SlideMusic(theVolume, theMs);
}

// 0x42c530
void Manager::SetMusicLoop(int theRowStep, int theOrder)
{
	mSound->SetMusicLoop(theRowStep, theOrder);
}

// 0x42c550
bool Manager::GetClipboard(std::string& theText)
{
	return GetHost()->GetClipboard(theText);
}

// 0x42c5c0
bool Manager::SetClipboard(const char* theText)
{
	return GetHost()->SetClipboard(theText);
}

// 0x42c6b0
bool Manager::LoadFileImage(int theId, const char* thePath)
{
	return mDisplay->LoadFileImage(theId, thePath);
}

// 0x42c6d0
void Manager::PreloadExeImage(int theId)
{
	mDisplay->Preload(theId, Module::Exe);
}

// 0x42c6f0
void Manager::PreloadDllImage(int theId)
{
	mDisplay->Preload(theId, Module::Dll);
}

// 0x42c710
void Manager::FreeImage(int theId)
{
	mDisplay->Free(theId);
}

// 0x42c720
Image* Manager::CreateImage(int theWidth, int theHeight)
{
	return mDisplay->CreateImage(theWidth, theHeight);
}

// 0x42c770
Image* Manager::GetImage(int theId, Module theModule)
{
	return mDisplay->GetImage(theId, theModule);
}

// 0x42c790
void Manager::DrawImage(int theX, int theY, Image* theImage, const Rect* theSrc, int theScaleX, int theScaleY,
	int theAlpha, Rect& theOut)
{
	Display::DrawImage(theX, theY, theImage, theSrc, theScaleX, theScaleY, theAlpha, theOut);
}

// 0x42c7d0
void Manager::DrawImageKeyed(int theX, int theY, Image* theImage, uint32_t theKey, const Rect* theSrc,
	int theScaleX, int theScaleY, int theAlpha, Rect& theOut)
{
	Display::DrawImageKeyed(theX, theY, theImage, theKey, theSrc, theScaleX, theScaleY, theAlpha, theOut);
}

// 0x42c850
void Manager::GetImageInfo(int theId, ImageInfo& theInfo)
{
	mDisplay->GetImageInfo(theId, theInfo);
}

// 0x42c870
Font* Manager::GetFont(int theImageId, int theTextId, Module theModule)
{
	return mDisplay->GetFont(theImageId, theTextId, theModule);
}

// 0x42c890
void Manager::SetCapture(Object* theObject, bool theOn)
{
	if (theOn)
	{
		mCapture = theObject;
		return;
	}
	if (mCapture != nullptr)
	{
		mCapture->OnRelease();
		mCapture = nullptr;
	}
	mHover = nullptr;
	RedoHover();
}

// 0x42c8d0 (at 0 the original told the display (0x434dc0) about +0xdc; nothing in the exe raises the count from
// its 0, so it never reaches 0 again)
void Manager::EndWait()
{
	mWaitCount = mWaitCount - 1;
}

// 0x42c8f0 (the accelerated display's software cursor is the host's cursor too)
void Manager::SetCursor(int theKind)
{
	mCursor = theKind;
	GetHost()->SetCursor(theKind);
}

// 0x42c810
void FillRectAlpha(Rect& theRect, int theAlpha, uint32_t theColour)
{
	gManager->mDisplay->FillRectAlpha(theRect, theAlpha, theColour);
}

// 0x42c830
void FillRect(const Rect* theRect, uint32_t theColour)
{
	gManager->mDisplay->FillRect(theRect, theColour);
}

} // namespace jb
