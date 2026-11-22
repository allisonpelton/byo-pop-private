#include "LoadGame.h"

#include "Anim.h"
#include "Color.h"
#include "GameApp.h"
#include "Graphics.h"

using namespace Sexy;

enum LoadGameEvent
{
	LOADGAME_EVENT_SHOWN = 1,
	LOADGAME_EVENT_GONE = 2,
};

// 0x43f430
LoadGame::LoadGame()
{
	SetParent(nullptr, nullptr);
	BuildTracks();
	MakeBarCorners();
	mProgress = 0.0f;
	mPhase = LOADGAME_FADING_IN;
	mProgressPending = false;
}

// 0x43f4d0
LoadGame::~LoadGame()
{
	if (mAnim != nullptr)
		delete mAnim;
	mAnim = nullptr;
}

// One bar of 0x43f550: from the first corner, as wide as the progress
void LoadGame::FillBar(Graphics* g, ObjBase* theTopLeft, ObjBase* theBottomRight)
{
	float aHeight = (float)((double)theBottomRight->mDrawn[STATE_Y] - (double)theTopLeft->mDrawn[STATE_Y] + 1.0);
	float aWidth = (float)(((double)theBottomRight->mDrawn[STATE_X] - (double)theTopLeft->mDrawn[STATE_X] + 1.0) *
		(double)mProgress);
	g->FillRect((int)theTopLeft->mDrawn[STATE_X], (int)theTopLeft->mDrawn[STATE_Y], (int)aWidth, (int)aHeight);
}

// 0x43f550: the background, and once the picture is fully in, the two bars at the progress's length
void LoadGame::Draw(Graphics* g)
{
	g->SetColor(Color(20, 30, 18));
	g->FillRect(0, 0, gApp->mWidth, gApp->mHeight);
	float anAlpha = mState[STATE_ALPHA];
	if (1.0f <= anAlpha) {
		mProgressPending = false;
		g->SetColor(Color(225, 150, 0));
		FillBar(g, mUpperBarTopLeft, mUpperBarBottomRight);
		g->SetColor(Color(245, 170, 0));
		FillBar(g, mLowerBarTopLeft, mLowerBarBottomRight);
	}
	ObjBase::Draw(g);
}

// 0x43f6f0
void LoadGame::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == LOADGAME_EVENT_SHOWN)
		mPhase = LOADGAME_SHOWN;
	else if (anEvent == LOADGAME_EVENT_GONE)
		mPhase = LOADGAME_DONE;
}

// 0x43f730
void LoadGame::SetProgress(int thePercent)
{
	if (thePercent > 100)
		thePercent = 100;
	else if (thePercent < 0)
		thePercent = 0;
	mProgressPending = true;
	mProgress = (float)((double)thePercent / 100.0);
}

// 0x43f770: shown, with the last step's bar drawn
bool LoadGame::IsReady()
{
	if (mProgressPending)
		return false;
	return mPhase == LOADGAME_SHOWN;
}

// 0x43f7a0: fades in over 35 frames; the second track fades it out over 30 and removes it
void LoadGame::BuildTracks()
{
	mAnim = Anim::Get("images/init/loadscreen", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 0);
	AnimKey* aFadeIn = new AnimKey();
	aFadeIn->SetDuration(35);
	aFadeIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aShown = new AnimKey();
	aShown->mEventId = LOADGAME_EVENT_SHOWN;
	aShown->SetDuration(2);
	aShown->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	AnimTrack* aShowTrack = new AnimTrack();
	aShowTrack->AddKey(aFadeIn, nullptr);
	aShowTrack->AddKey(aShown, aShown);
	AddTrack(aShowTrack, nullptr);

	AnimKey* aFadeOut = new AnimKey();
	aFadeOut->SetDuration(30);
	aFadeOut->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aFadeOut->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aFadeOut->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = LOADGAME_EVENT_GONE;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aGone->SetRemoveAtEnd();
	AnimTrack* aHideTrack = new AnimTrack();
	aHideTrack->AddKey(aFadeOut, nullptr);
	aHideTrack->AddKey(aGone, aGone);
	AddTrack(aHideTrack, nullptr);
	Activate(this, false);
}

// 0x43fa30: four points in the picture's design space, children of the screen, whose drawn positions give the bars
void LoadGame::MakeBarCorners()
{
	mUpperBarTopLeft = new ObjBase();
	mUpperBarTopLeft->SetStateInt(STATE_X, -722);
	mUpperBarTopLeft->SetStateInt(STATE_Y, 54);
	mUpperBarTopLeft->SetStateInt(STATE_Z, 10);
	mUpperBarTopLeft->SetParent(this, nullptr);
	Activate(mUpperBarTopLeft, false);
	mUpperBarBottomRight = new ObjBase();
	mUpperBarBottomRight->SetStateInt(STATE_X, 693);
	mUpperBarBottomRight->SetStateInt(STATE_Y, 364);
	mUpperBarBottomRight->SetStateInt(STATE_Z, 10);
	mUpperBarBottomRight->SetParent(this, nullptr);
	Activate(mUpperBarBottomRight, false);
	mLowerBarTopLeft = new ObjBase();
	mLowerBarTopLeft->SetStateInt(STATE_X, -571);
	mLowerBarTopLeft->SetStateInt(STATE_Y, 478);
	mLowerBarTopLeft->SetStateInt(STATE_Z, 10);
	mLowerBarTopLeft->SetParent(this, nullptr);
	Activate(mLowerBarTopLeft, false);
	mLowerBarBottomRight = new ObjBase();
	mLowerBarBottomRight->SetStateInt(STATE_X, 578);
	mLowerBarBottomRight->SetStateInt(STATE_Y, 576);
	mLowerBarBottomRight->SetStateInt(STATE_Z, 10);
	mLowerBarBottomRight->SetParent(this, nullptr);
	Activate(mLowerBarBottomRight, false);
}
