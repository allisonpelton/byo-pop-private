#include "WalkThru.h"

#include "Anim.h"
#include "LevelObj.h"

#include <cstdio>

using namespace Sexy;

WalkThru* WalkThru::sInstance = nullptr;
Anim* WalkThru::sTextAnims[9] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

// The key events the hint's tracks raise (AnimKey::mEventId)
enum WalkEvent
{
	WALK_EVENT_SHOWN = 98,           // the page has swelled in: it may be left now
	WALK_EVENT_HIDDEN = 99,          // the page has swelled out: the next shows, or the hint ends
};

// The depth the hint is at
const int WALK_Z = 0xd1;

// Frames the picture takes to swell in, how long it is held at full size, and how long it takes to swell out
const int WALK_SWELL_IN = 6;
const int WALK_SHOWN_HOLD = 15;
const int WALK_SWELL_OUT = 8;
// The swell in overshoots: a sine wave of this rate and amount is added to the size on the way
const float WALK_OVERSHOOT_RATE = 1.0f;
const float WALK_OVERSHOOT_AMOUNT = 1.1f;
// Frames a page with a timeout transition waits
const int WALK_TIMEOUT_FRAMES = 200;
// A page that was never given a timeout (the original starts the counter at -99)
const int WALK_NO_TIMEOUT = -99;

// Where the first level's pages stand: far above the middle of the play area, and 200 either side of it
const float WALK_FIRST_Y = -420.0f;
const double WALK_FIRST_SIDE = 200.0;
// The third level's pages stand up and left of the middle
const double WALK_THIRD_LEFT = 250.0;
const double WALK_THIRD_UP = 200.0;

// The text pictures the steps use (the index into sTextAnims; the eighth is never used)
enum WalkText
{
	WALK_TEXT_1 = 0,
	WALK_TEXT_2 = 1,
	WALK_TEXT_3 = 2,
	WALK_TEXT_4 = 3,
	WALK_TEXT_5 = 4,
	WALK_TEXT_6 = 5,
	WALK_TEXT_7 = 6,
	WALK_TEXT_9 = 8,
};

// 0x475db0
WalkPage::WalkPage()
{
	mPicture = nullptr;
	mEnded = false;
	mFinal = true;
	mX = 0.0f;
	mY = 0.0f;
	mTimer = WALK_NO_TIMEOUT;
	for (int i = 0; i < WALK_TRIGGER_COUNT; i++) {
		mHasTransition[i] = false;
		mNext[i] = nullptr;
	}
}

// 0x475e00: a page that waits for a timeout counts its 200 frames down, and raises the trigger on each frame after
void WalkPage::Tick()
{
	if (mHasTransition[WALK_TRIGGER_TIMEOUT]) {
		if (mTimer < 1) {
			WalkThru::OnTimeout();
			return;
		}
		mTimer = mTimer - 1;
	}
}

// 0x475e20
void WalkPage::SetPlace(float theX, float theY)
{
	mX = theX;
	mY = theY;
}

// 0x475e40: the trigger leads to theNext (none ends the hint); a timeout starts its 200 frames
void WalkPage::AddTransition(WalkPage* theNext, int theTrigger)
{
	mHasTransition[theTrigger] = true;
	mNext[theTrigger] = theNext;
	if (theNext != nullptr)
		mFinal = false;
	if (theTrigger == WALK_TRIGGER_TIMEOUT)
		mTimer = WALK_TIMEOUT_FRAMES;
}

// 0x475e70
WalkPage* WalkPage::GetNext(int theTrigger)
{
	if (theTrigger >= 0) {
		if (mHasTransition[theTrigger]) {
			if (mNext[theTrigger] == nullptr)
				mEnded = true;
			return mNext[theTrigger];
		}
		mEnded = false;
	}
	return nullptr;
}

// 0x474f00: invisible (swollen to nothing) until a page starts, in front of everything at 209
WalkThru::WalkThru()
{
	mPage = nullptr;
	mFirstPage = nullptr;
	mTrigger = -1;
	mDismissed = false;
	mPageCount = 0;
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_Z, WALK_Z);
	BuildTracks();
	ClearTriggers();
	Deactivate(this);
	StartTrack(mShowTrack);
}

// 0x474fe0 (the body of the destructor 0x474fc0)
WalkThru::~WalkThru()
{
	sInstance = nullptr;
}

// 0x4752b0: the two tracks. Showing: 6 frames growing from nothing with an overshoot (a sine wave added to the size on
// its way), then 15 held at full size, which raise event 98. Hiding: 8 frames from full size to nothing, then a frame
// held at nothing, which raises event 99.
void WalkThru::BuildTracks()
{
	AnimKey* aSwellIn = new AnimKey();
	aSwellIn->SetDuration(WALK_SWELL_IN);
	aSwellIn->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwellIn->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, WALK_OVERSHOOT_RATE, WALK_OVERSHOOT_AMOUNT, 0.0f,
		nullptr);
	aSwellIn->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwellIn->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, WALK_OVERSHOOT_RATE, WALK_OVERSHOOT_AMOUNT, 0.0f,
		nullptr);
	aSwellIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aShown = new AnimKey();
	aShown->mEventId = WALK_EVENT_SHOWN;
	aShown->SetDuration(WALK_SHOWN_HOLD);
	aShown->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aShown->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aShown->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aShowTrack = new AnimTrack();
	aShowTrack->AddKey(aSwellIn, nullptr);
	aShowTrack->AddKey(aShown, aShown);
	mShowTrack = AddTrack(aShowTrack, nullptr);

	AnimKey* aSwellOut = new AnimKey();
	aSwellOut->SetDuration(WALK_SWELL_OUT);
	aSwellOut->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSwellOut->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aSwellOut->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aHidden = new AnimKey();
	aHidden->mEventId = WALK_EVENT_HIDDEN;
	aHidden->SetDuration(1);
	aHidden->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aHidden->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aHidden->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	AnimTrack* aHideTrack = new AnimTrack();
	aHideTrack->AddKey(aSwellOut, nullptr);
	aHideTrack->AddKey(aHidden, nullptr);
	mHideTrack = AddTrack(aHideTrack, nullptr);
}

// 0x475180
void WalkThru::ClearTriggers()
{
	for (int i = 0; i < WALK_TRIGGER_COUNT; i++)
		mTriggered[i] = false;
}

// 0x4751c0: the page is the first and the one shown; its picture swells in at its place. The original takes the
// picture without a reference of its own and gives one back when it goes (Notify): the pictures are never freed, so
// nothing shows.
void WalkThru::SetFirstPage(WalkPage* thePage)
{
	mPage = thePage;
	mFirstPage = thePage;
	mAnim = thePage->mPicture;
	StartTrack(mShowTrack);
	SetState(STATE_X, mPage->mX);
	SetState(STATE_Y, mPage->mY);
}

// 0x475210
void WalkThru::AddPage(WalkPage* thePage)
{
	for (int i = 0; i < mPageCount; i++) {
		if (mPages[i] == thePage)
			return;
	}
	mPages[mPageCount] = thePage;
	mPageCount = mPageCount + 1;
}

// 0x475250
void WalkThru::FreePages()
{
	for (int i = 0; i < mPageCount; i++) {
		delete mPages[i];
		mPages[i] = nullptr;
	}
	mFirstPage = nullptr;
	mPage = nullptr;
	mPageCount = 0;
}

// 0x475000: a key of the hint's tracks ended. Shown: the page starts to wait for a trigger. Hidden: the page the trigger
// leads to shows; with none (or after Dismiss) the hint is over.
void WalkThru::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == WALK_EVENT_SHOWN) {
		ClearTriggers();
		mTrigger = -1;
	} else if (anEvent == WALK_EVENT_HIDDEN) {
		if (mPage == nullptr) {
			Delete();
			return;
		}
		mPage = mPage->GetNext(mTrigger);
		if (mPage != nullptr && !mDismissed) {
			if (mAnim != nullptr)
				mAnim->Release();
			mAnim = mPage->mPicture;
			StartTrack(mShowTrack);
			SetState(STATE_X, mPage->mX);
			SetState(STATE_Y, mPage->mY);
			return;
		}
		if (mAnim != nullptr)
			mAnim->Release();
		FreePages();
		Delete();
		mDismissed = false;
		sInstance = nullptr;
	}
}

// 0x475100: a page that is waiting counts its timeout, and leaves by the first trigger it has a transition for that
// has happened (the swell out starts; the triggers are cleared, so only one is taken)
bool WalkThru::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mPage != nullptr && mTrigger == -1) {
		mPage->Tick();
		for (int i = 0; i < WALK_TRIGGER_COUNT; i++) {
			if (mPage->mHasTransition[i] && mTriggered[i]) {
				ClearTriggers();
				mTrigger = i;
				StartTrack(mHideTrack);
			}
		}
	}
	return true;
}

// What happened, for the page that waits (nothing when no hint is showing)
void WalkThru::Raise(int theTrigger)
{
	if (sInstance != nullptr)
		sInstance->mTriggered[theTrigger] = true;
}

void WalkThru::OnTimeout()
{
	Raise(WALK_TRIGGER_TIMEOUT);
}

void WalkThru::OnShoot()
{
	Raise(WALK_TRIGGER_SHOOT);
}

void WalkThru::OnLoseLife()
{
	Raise(WALK_TRIGGER_LOSE_LIFE);
}

void WalkThru::OnAreaFilled()
{
	Raise(WALK_TRIGGER_AREA_FILLED);
}

void WalkThru::OnTurn()
{
	Raise(WALK_TRIGGER_TURN);
}

void WalkThru::OnBugZapped()
{
	Raise(WALK_TRIGGER_BUG_ZAPPED);
}

// 0x4756b0: the level is over. The hint is told it is dismissed (a page ends the hint instead of showing the next when
// it is next left, and no page waits for the dismiss trigger itself, so a page with no timeout stays up).
void WalkThru::Dismiss()
{
	if (sInstance != nullptr) {
		sInstance->mTriggered[WALK_TRIGGER_DISMISS] = true;
		sInstance->mDismissed = true;
	}
}

// 0x4756d0
void Sexy::PreloadWalkThru()
{
	for (int i = 0; i < 9; i++) {
		char aName[32];
		snprintf(aName, sizeof(aName), "images/walkthru/text%02d", i + 1);
		WalkThru::sTextAnims[i] = Anim::Get(aName, ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	}
}

// 0x4757d0: the first level. The first text (above the play area) is taken down by the first shot; then nothing is shown
// until an area is filled; the second text (200 left) is taken down by the next area filled, the third (200 right) by
// the next shot, which ends the hint.
void WalkThru::ShowFirstStep()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	WalkPage* aFirst = new WalkPage();
	aFirst->mPicture = sTextAnims[WALK_TEXT_1];
	aFirst->SetPlace(aLevel->mScreenCenterX, WALK_FIRST_Y);
	WalkPage* aBlank = new WalkPage();
	aBlank->mPicture = nullptr;
	aBlank->SetPlace((float)((double)aLevel->mScreenCenterX - WALK_FIRST_SIDE), WALK_FIRST_Y);
	WalkPage* aSecond = new WalkPage();
	aSecond->mPicture = sTextAnims[WALK_TEXT_2];
	aSecond->SetPlace((float)((double)aLevel->mScreenCenterX - WALK_FIRST_SIDE), WALK_FIRST_Y);
	WalkPage* aThird = new WalkPage();
	aThird->mPicture = sTextAnims[WALK_TEXT_3];
	aThird->SetPlace((float)((double)aLevel->mScreenCenterX + WALK_FIRST_SIDE), WALK_FIRST_Y);
	aThird->AddTransition(nullptr, WALK_TRIGGER_SHOOT);
	aFirst->AddTransition(aBlank, WALK_TRIGGER_SHOOT);
	aBlank->AddTransition(aSecond, WALK_TRIGGER_AREA_FILLED);
	aSecond->AddTransition(aThird, WALK_TRIGGER_AREA_FILLED);
	WalkThru* aWalk = new WalkThru();
	aWalk->SetFirstPage(aFirst);
	aWalk->AddPage(aFirst);
	aWalk->AddPage(aBlank);
	aWalk->AddPage(aSecond);
	aWalk->AddPage(aThird);
	sInstance = aWalk;
	aWalk->Activate(aWalk, false);
}

// 0x4759e0: the second level; every page is in the middle of the play area. Nothing is shown for 200 frames, then the
// fourth text, which a shot answers with the fifth, a lost life with the seventh and a bug zapped with the ninth; the
// fifth is also answered by a bug zapped (with the ninth); the seventh gives way to the fourth again after 200 frames;
// the ninth ends the hint after 200 frames or a shot.
void WalkThru::ShowSecondStep()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	WalkPage* aBlank = new WalkPage();
	aBlank->mPicture = nullptr;
	aBlank->SetPlace(aLevel->mScreenCenterX, aLevel->mScreenCenterY);
	WalkPage* aAsk = new WalkPage();
	aAsk->mPicture = sTextAnims[WALK_TEXT_4];
	aAsk->SetPlace(aLevel->mScreenCenterX, aLevel->mScreenCenterY);
	WalkPage* aShot = new WalkPage();
	aShot->mPicture = sTextAnims[WALK_TEXT_5];
	aShot->SetPlace(aLevel->mScreenCenterX, aLevel->mScreenCenterY);
	WalkPage* aLost = new WalkPage();
	aLost->mPicture = sTextAnims[WALK_TEXT_7];
	aLost->SetPlace(aLevel->mScreenCenterX, aLevel->mScreenCenterY);
	aLost->AddTransition(aAsk, WALK_TRIGGER_TIMEOUT);
	WalkPage* aEnd = new WalkPage();
	aEnd->mPicture = sTextAnims[WALK_TEXT_9];
	aEnd->SetPlace(aLevel->mScreenCenterX, aLevel->mScreenCenterY);
	aEnd->AddTransition(nullptr, WALK_TRIGGER_TIMEOUT);
	aEnd->AddTransition(nullptr, WALK_TRIGGER_SHOOT);
	aBlank->AddTransition(aAsk, WALK_TRIGGER_TIMEOUT);
	aAsk->AddTransition(aShot, WALK_TRIGGER_SHOOT);
	aAsk->AddTransition(aLost, WALK_TRIGGER_LOSE_LIFE);
	aAsk->AddTransition(aEnd, WALK_TRIGGER_BUG_ZAPPED);
	aShot->AddTransition(aEnd, WALK_TRIGGER_BUG_ZAPPED);
	WalkThru* aWalk = new WalkThru();
	aWalk->SetFirstPage(aBlank);
	aWalk->AddPage(aBlank);
	aWalk->AddPage(aAsk);
	aWalk->AddPage(aShot);
	aWalk->AddPage(aLost);
	// port (leak): the original leaves the last page out of the list of pages to free
	aWalk->AddPage(aEnd);
	sInstance = aWalk;
	aWalk->Activate(aWalk, false);
}

// 0x475c40: the third level. The sixth text (up and left of the middle) gives way to the seventh, at the same place,
// when the rope is turned or after 200 frames; the seventh ends the hint after 200 frames.
void WalkThru::ShowThirdStep()
{
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	float aX = (float)((double)aLevel->mScreenCenterX - WALK_THIRD_LEFT);
	float aY = (float)((double)aLevel->mScreenCenterY - WALK_THIRD_UP);
	WalkPage* aFirst = new WalkPage();
	aFirst->mPicture = sTextAnims[WALK_TEXT_6];
	aFirst->SetPlace(aX, aY);
	WalkPage* aSecond = new WalkPage();
	aSecond->mPicture = sTextAnims[WALK_TEXT_7];
	aSecond->SetPlace(aX, aY);
	aSecond->AddTransition(nullptr, WALK_TRIGGER_TIMEOUT);
	aFirst->AddTransition(aSecond, WALK_TRIGGER_TURN);
	aFirst->AddTransition(aSecond, WALK_TRIGGER_TIMEOUT);
	WalkThru* aWalk = new WalkThru();
	aWalk->SetFirstPage(aFirst);
	aWalk->AddPage(aFirst);
	aWalk->AddPage(aSecond);
	sInstance = aWalk;
	aWalk->Activate(aWalk, false);
}
