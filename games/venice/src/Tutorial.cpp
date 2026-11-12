// The tutorial (0x420340-0x421980).

#include "Tutorial.h"

#include "GameText.h"
#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "SceneTag.h"
#include "Shooter.h"
#include "Slot.h"
#include "TextObject.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

#include <cstdio>
#include <cstring>

namespace
{
const float Unchanged = 99999.0f; // a stage attribute that keeps the object's value
const int TipLineWidth = 18;
const float TipLineHeight = 24.0f;

// the cupid bobs while he waits
void AddBob(SceneObject* theObject, float thePeriod)
{
	theObject->AddFilter(FilterPingPongAnim, AttributeFrameNumber, 4.0f, thePeriod, 1.0f);
}

void AddEase(SceneObject* theObject)
{
	theObject->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0.0f, 0.0f);
}

void AddCustomEventOn(SceneObject* theObject, int theTrigger, const char* theStageName = "")
{
	theObject->AddEvent(theTrigger, ActionCustom, theStageName, "", "", "", "");
}
}

// The lessons come as stages of the cupid himself: he flies in, waits for a click, goes on to the rules, then waits for the first
// shot (1st_idle: he points at the slot the ready jewel fits), and after each of the next lessons (2nd to 4th) he says what comes
// next; the stages that end or start on a custom event change the tip that the scroll shows.
Tutorial::Tutorial()
{
	mType = ObjectTypeTutorial;
	SetImage("gfx\\cupid.png", 0, 0, 0, 0);

	AddStage(1204.0f, 0, 0, 0, "initial_idle");
	AddBob(this, 10.0f);

	AddStage(700.0f, 0, 0, 1, "enter");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, -700.0f, -200.0f);
	SetAttribute(AttributeY, 0.0f, 20.0f);
	AddEase(this);

	AddStage(204.0f, 0, 0, 1, "enter_wait");
	AddBob(this, 10.0f);

	AddStage(3004.0f, 0, 0, 1, "enter_idle");
	AddBob(this, 10.0f);
	AddEvent(TriggerClickAnywhere, ActionNextStage, "", "", "", "", "");

	AddStage(604.0f, 0, 0, 1, "rules_1");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, -230.0f);
	SetAttribute(AttributeY, Unchanged, -190.0f);
	AddEase(this);
	AddCustomEventOn(this, TriggerEnd);

	AddStage(604.0f, 1, 1, 1, "rules_2");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, Unchanged);
	SetAttribute(AttributeY, Unchanged, Unchanged);
	AddEase(this);
	AddEvent(TriggerClickAnywhere, ActionNextStage, "", "", "", "", "");
	AddCustomEventOn(this, TriggerClickAnywhere);

	AddStage(604.0f, 1, 1, 1, "rules_2");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, Unchanged);
	SetAttribute(AttributeY, Unchanged, Unchanged);
	AddEase(this);
	AddEvent(TriggerClickAnywhere, ActionNextStage, "", "", "", "", "");

	AddStage(6000.0f, 0, 0, 1, "pre_first_idle");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, Unchanged);
	SetAttribute(AttributeY, Unchanged, Unchanged);
	AddEase(this);
	AddCustomEventOn(this, TriggerStart);
	AddEvent(TriggerClickAnywhere, ActionGoToStage, "1st_idle", "", "", "", "");

	AddStage(604.0f, 1, 1, 1, "1st_idle");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, -500.0f, -230.0f);
	SetAttribute(AttributeY, -90.0f, -90.0f);
	AddEase(this);

	AddStage(1200.0f, 0, 0, 1, "post_first_idle");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, -200.0f);
	SetAttribute(AttributeY, Unchanged, 20.0f);
	AddEase(this);

	AddStage(604.0f, 0, 0, 1, "pre_second_idle");
	SetAttribute(AttributeX, Unchanged, -78.0f);
	SetAttribute(AttributeY, Unchanged, -252.0f);
	AddBob(this, 20.0f);
	AddEase(this);
	AddCustomEventOn(this, TriggerEnd);

	AddStage(604.0f, 1, 1, 1, "2nd_idle");
	AddBob(this, 20.0f);
	AddEase(this);

	AddStage(1000.0f, 0, 0, 1, "post_second_idle");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, -200.0f);
	SetAttribute(AttributeY, Unchanged, 20.0f);
	AddEase(this);
	AddCustomEventOn(this, TriggerEnd);

	AddStage(604.0f, 1, 1, 1, "3rd_idle");
	AddBob(this, 20.0f);

	AddStage(604.0f, 0, 0, 1, "post_third_idle");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, -200.0f);
	SetAttribute(AttributeY, Unchanged, 20.0f);
	AddEase(this);

	AddStage(2200.0f, 0, 0, 1, "post_third_idle2");
	AddBob(this, 10.0f);
	SetAttribute(AttributeX, Unchanged, -200.0f);
	SetAttribute(AttributeY, Unchanged, 20.0f);
	AddEase(this);
	AddCustomEventOn(this, TriggerEnd);
	AddCustomEventOn(this, TriggerClickAnywhere, "4th_idle");
	AddEvent(TriggerClickAnywhere, ActionGoToStage, "4th_idle", "", "", "", "");

	AddStage(604.0f, 1, 1, 1, "4th_idle");
	AddBob(this, 20.0f);

	AddStage(604.0f, 1, 1, 1, "5th_idle");
	AddEvent(TriggerClickAnywhere, ActionInsertLevel, "loading", "", "", "", "");
	AddBob(this, 20.0f);

	// the scroll the cupid reads from, and the arrow that points at where to shoot
	SceneObject* aScroll = NewChild(nullptr);
	aScroll->AddStage(1000.0f, 1, 1, 1, nullptr);
	aScroll->SetImage("gfx\\scroll.png", 0, 0, 0, 0);
	aScroll->SetAttribute(AttributeX, 146.0f, 146.0f);
	aScroll->SetAttribute(AttributeY, -5.0f, -5.0f);
	aScroll->SetZOrder(0.0f);

	SceneObject* anArrow = NewChild(nullptr);
	anArrow->AddStage(1200.0f, 1, 1, 0, nullptr);
	anArrow->SetImage("gfx\\shoothere_arrow.png", 0, 0, 0, 0);
	anArrow->SetAttribute(AttributeX, 120.0f, 120.0f);
	anArrow->SetAttribute(AttributeY, 50.0f, 50.0f);
	anArrow->SetAttribute(AttributeRotation, 180.0f, 180.0f);
	anArrow->AddFilter(FilterSined, AttributeY, 12.0f, 1.0f, 0.0f);
	anArrow->SetZOrder(0.0f);
}

// The tips are the data file's tip1, tip2 ... in the game's words; the first one is shown at once
void Tutorial::Load(const char* theText, SceneTag* theTag)
{
	for (int aTip = 1;; aTip++) {
		char aName[20];
		sprintf(aName, "tip%d", aTip);
		if (!theTag->Has(aName))
			break;
		mTips.push_back(TranslateText(theTag->GetString(aName)));
	}
	mTipIndex = 0;
	ShowTip();
	SceneObject::Load(theText, theTag);
}

void Tutorial::OnCustomEvent()
{
	ShowTip();
}

bool Tutorial::Update()
{
	mSelectable = 0;
	mBilinear = 1;
	gShooter->mCanShoot = 0;
	UpdateFirstLesson();
	UpdateSecondLesson();
	UpdateThirdLesson();
	UpdateFourthLesson();
	// the pointer on the quit button is not a shot
	SceneObject* aQuitButton = gActiveLevel->FindChildByName("quit_button", nullptr);
	if (aQuitButton != nullptr && aQuitButton->mIsHot)
		gShooter->mCanShoot = 0;
	return SceneObject::Update();
}

// The scroll (the first child) says the tip in lines of at most 18 letters, broken at spaces, and the next tip is the one after
// (the first again after the last).
void Tutorial::ShowTip()
{
	SceneObject* aScroll = mChildren.mHead;
	if (aScroll == nullptr || mTips.empty())
		return;
	aScroll->mChildren.Clear();
	const std::string& aTip = mTips[mTipIndex];
	int aTipLength = (int)aTip.size();
	float aLineY = -24.0f;
	int aLineStart = 0;
	while (aLineStart < aTipLength) {
		int aLineEnd = aLineStart + TipLineWidth < aTipLength ? aLineStart + TipLineWidth : aTipLength;
		if (aLineEnd < aTipLength) {
			while (aLineStart < aLineEnd && aTip[aLineEnd] != ' ')
				aLineEnd--;
		}
		std::string aLine = aTip.substr(aLineStart, aLineEnd - aLineStart);
		aLineStart = aLineEnd + 1;
		TextObject* aLineText = new TextObject();
		aScroll->NewChild(aLineText);
		aLineText->SetText(aLine.c_str(), "gfx\\font1_small.png", -1.0f, 0, JustifyLeft);
		aLineText->AddStage(1000.0f, 1, 1, 1, nullptr);
		aLineText->SetAttribute(AttributeX, -96.0f, -96.0f);
		aLineText->SetAttribute(AttributeY, aLineY, aLineY);
		aLineY = aLineY + TipLineHeight;
	}
	mTipIndex = mTipIndex + 1 < mTips.size() ? mTipIndex + 1 : 0;
}

bool Tutorial::PlateIsGone(SceneObject* thePlate)
{
	return thePlate == nullptr || thePlate->mWorld[AttributeY] < 0.0f;
}

// The middle layer gets a 2 s stage that scrolls it to the next plates (and ends in the stage where it scrolls on its own), and
// the cupid goes to his stage after the lesson
void Tutorial::StartNextLesson(const char* theScrollStageName, float theScrollTo, const char* theCupidStageName)
{
	gMiddleLayer->AddStage(2000.0f, 0, 0, 1, theScrollStageName);
	gMiddleLayer->SetAttribute(AttributeY, Unchanged, theScrollTo);
	gMiddleLayer->AddEvent(TriggerEnd, ActionGoToStage, "scrolling", "", "", "", "");
	gMiddleLayer->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0.0f, 0.0f);
	gMiddleLayer->GoToStageByName(theScrollStageName);
	GoToStageByName(theCupidStageName);
}

// The first lesson: the cupid points at the slot of the plate that the ready jewel fits (while it turns, he points at one he
// has found), and when the plate has gone the level scrolls on. The tip is shown again on every update until a slot is found.
void Tutorial::UpdateFirstLesson()
{
	if (strstr(mStages.mCursor->mName, "1st_idle") == nullptr || strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)
		return;
	if (mTargetSlot == nullptr)
		ShowTip();
	gShooter->mCanShoot = GetRoot()->mNext == nullptr;
	mChildren.mHead->mStages.mCursor->mVisible = true;
	mChildren.mTail->mStages.mCursor->mVisible = true;
	SceneObject* aPlate = FindHighestOfType(ObjectTypePlate, nullptr);
	if (PlateIsGone(aPlate)) {
		StartNextLesson("tut_next1", 224.0f, "post_first_idle");
		mChildren.mTail->mStages.mCursor->mVisible = false;
		ShowTip();
	} else if (mStages.mCursor->mFrame == mStages.mCursor->mLength) {
		mStages.mCursor->mStart[AttributeX] = mStages.mCursor->mEnd[AttributeX];
		mStages.mCursor->mStart[AttributeY] = mStages.mCursor->mEnd[AttributeY];
	} else if (gShooter->GetReadyJewel() != nullptr && VeniceAbs(aPlate->mLocal[AttributeRotation]) < (double)1.0f) {
		SceneObject* aSlot = nullptr;
		while ((aSlot = aPlate->FindChildByType(ObjectTypeSlot, aSlot)) != nullptr) {
			if (mTargetSlot != aSlot && static_cast<Slot*>(aSlot)->mKind == gShooter->GetReadyJewel()->mKind) {
				// the cupid's stage now slides from where he is to a little way off the slot
				mStages.mCursor->mStart[AttributeX] = mLocal[AttributeX];
				mStages.mCursor->mStart[AttributeY] = mLocal[AttributeY];
				mStages.mCursor->mEnd[AttributeX] = GetLocalX(aSlot->mWorld[AttributeX], aSlot->mWorld[AttributeY]) - 120.0f;
				mStages.mCursor->mEnd[AttributeY] = GetLocalY(aSlot->mWorld[AttributeX], aSlot->mWorld[AttributeY]) - 120.0f;
				mStages.mCursor->mFrame = 1.0f;
				mTargetSlot = aSlot;
			}
		}
	}
}

void Tutorial::UpdateSecondLesson()
{
	if (strstr(mStages.mCursor->mName, "2nd_idle") == nullptr || strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)
		return;
	gShooter->mCanShoot = GetRoot()->mNext == nullptr;
	SceneObject* aPlate = FindHighestOfType(ObjectTypePlate, nullptr);
	mChildren.mTail->mStages.mCursor->mVisible = true;
	if (PlateIsGone(aPlate)) {
		StartNextLesson("tut_next2", 410.0f, "post_second_idle");
		mChildren.mTail->mStages.mCursor->mVisible = false;
		ShowTip();
	}
}

void Tutorial::UpdateThirdLesson()
{
	if (strstr(mStages.mCursor->mName, "3rd_idle") == nullptr || strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)
		return;
	gShooter->mCanShoot = GetRoot()->mNext == nullptr;
	if (PlateIsGone(FindHighestOfType(ObjectTypePlate, nullptr))) {
		StartNextLesson("tut_next3", 580.0f, "post_third_idle");
		ShowTip();
	}
}

void Tutorial::UpdateFourthLesson()
{
	if (strstr(mStages.mCursor->mName, "4th_idle") == nullptr || strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)
		return;
	gShooter->mCanShoot = GetRoot()->mNext == nullptr;
	if (PlateIsGone(FindHighestOfType(ObjectTypePlate, nullptr))) {
		StartNextLesson("tut_next4", Unchanged, "5th_idle");
		ShowTip();
	}
}
