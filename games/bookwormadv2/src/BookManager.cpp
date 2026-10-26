#include "BookManager.h"

#include <algorithm>

#include "FadeButton.h"
#include "GameApp.h"
#include "LuaBind.h"
#include "Res.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

BookManager::BookManager(GameApp* theApp) : mApp(theApp)
{
	RegisterLua();
	mEnums["ButtonId_Back"] = BUTTON_BACK;
	mEnums["ButtonId_Continue"] = BUTTON_CONTINUE;
	mEnums["ButtonId_Options"] = BUTTON_OPTIONS;
	mEnums["State_Normal"] = STATE_NORMAL;
	mEnums["State_BookSlideOut"] = STATE_BOOK_SLIDE_OUT;
	mEnums["State_BookSlideIn"] = STATE_BOOK_SLIDE_IN;
	mEnums["State_MapSlideOut"] = STATE_MAP_SLIDE_OUT;
	mEnums["State_MapSlideIn"] = STATE_MAP_SLIDE_IN;
}

BookManager::~BookManager()
{
	RemoveAllWidgets(true, true);
}

// The original registered the class in the constructor (0x42b6f0), every time
void BookManager::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<BookManager>("BookManager", false)
		.Base<LuaWidget>()
		.Method<&BookManager::PositionButton>("PositionButton")
		.Method<&BookManager::SetButtonState>("SetButtonState")
		.Method<&BookManager::GetButtonX>("GetButtonX")
		.Method<&BookManager::GetButtonY>("GetButtonY")
		.Method<&BookManager::GetButtonW>("GetButtonW")
		.Method<&BookManager::GetButtonH>("GetButtonH")
		.Method<&BookManager::MakeMGButton>("MakeMGButton")
		.Method<&BookManager::GiveKeyboardFocus>("GiveKeyboardFocus")
		.Method<&BookManager::RemoveMGButtons>("RemoveMGButtons")
		.Method<&BookManager::AddOverlayCallback>("AddOverlayCallback")
		.Method<&BookManager::DisableMGButtons>("DisableMGButtons")
		.Method<&BookManager::RemoveOverlayCallback>("RemoveOverlayCallback")
		.Method<&BookManager::DoTreasureTransition>("DoTreasureTransition")
		.Method<&BookManager::DisableContinueButton>("DisableContinueButton")
		.Method<&BookManager::GetState>("GetState")
		.Method<&BookManager::SetState>("SetState");
}

FadeButton* BookManager::FindButton(int theId)
{
	auto anIt = mButtons.find(theId);
	if (anIt != mButtons.end())
		return anIt->second;
	for (FadeButton* aButton : mMGButtons)
		if (aButton->mId == theId)
			return aButton;
	return nullptr;
}

void BookManager::PositionButton(int theId, int theX, int theY)
{
	if (FadeButton* aButton = FindButton(theId))
		aButton->Move(theX, theY);
}

void BookManager::SetButtonState(int theId, bool theVisible, bool theEnabled)
{
	FadeButton* aButton = mButtons[theId];
	aButton->SetVisible(theVisible);
	aButton->SetDisabled(!theEnabled);
}

int BookManager::GetButtonX(int theId)
{
	return theId >= BUTTON_BACK && theId <= BUTTON_OPTIONS ? mButtons[theId]->mX : -1;
}

int BookManager::GetButtonY(int theId)
{
	return theId >= BUTTON_BACK && theId <= BUTTON_OPTIONS ? mButtons[theId]->mY : -1;
}

int BookManager::GetButtonW(int theId)
{
	return theId >= BUTTON_BACK && theId <= BUTTON_OPTIONS ? mButtons[theId]->mWidth : -1;
}

int BookManager::GetButtonH(int theId)
{
	return theId >= BUTTON_BACK && theId <= BUTTON_OPTIONS ? mButtons[theId]->mHeight : -1;
}

void BookManager::MakeMGButton(int theId)
{
	// its presses go to the Lua object (ButtonDepress)
	FadeButton* aButton = new FadeButton(theId, static_cast<LuaWidgetTplBase*>(this));
	aButton->mOwner = this;
	aButton->SetupImage(IMAGE_TUT_MG_BUTTON);
	aButton->mDisabledRect = aButton->mNormalRect;
	mMGButtons.push_back(aButton);
	AddWidget(aButton);
}

void BookManager::GiveKeyboardFocus()
{
	mApp->mWidgetManager->SetFocus(this);
}

void BookManager::RemoveMGButtons()
{
	for (FadeButton* aButton : mMGButtons) {
		mApp->SafeDeleteWidget(aButton);
		RemoveWidget(aButton);
	}
	mMGButtons.clear();
}

void BookManager::AddOverlayCallback(std::string theFunction)
{
	mOverlayCallbacks.push_back(theFunction);
}

void BookManager::DisableMGButtons()
{
	for (FadeButton* aButton : mMGButtons)
		aButton->SetDisabled(true);
}

void BookManager::RemoveOverlayCallback(std::string theFunction)
{
	auto anIt = std::find(mOverlayCallbacks.begin(), mOverlayCallbacks.end(), theFunction);
	if (anIt != mOverlayCallbacks.end())
		mOverlayCallbacks.erase(anIt);
}

void BookManager::DoTreasureTransition()
{
	TreasureScreen* aTreasure = mApp->mTreasureScreen;
	aTreasure->SetVisible(true);
	aTreasure->SetDisabled(false);
	// its buttons 10, 11 and 12 where the back, continue and options buttons are
	for (int i = BUTTON_BACK; i <= BUTTON_OPTIONS; i++)
		aTreasure->mButtons[10 + i]->Move(mButtons[i]->mX, mButtons[i]->mY);
	for (int i = BUTTON_BACK; i <= BUTTON_OPTIONS; i++) {
		mButtons[i]->SetVisible(false);
		mButtons[i]->SetDisabled(true);
	}
	SetVisible(false);
	SetDisabled(true);
	mApp->mWidgetManager->SetFocus(aTreasure);
	aTreasure->CallLinkedMethod(mApp->mTimedMode ? "StartFullyDisplayed" : "SlideIn", 0);
}

void BookManager::DisableContinueButton()
{
	mButtons[BUTTON_CONTINUE]->SetDisabled(true);
}

bool BookManager::IsContinueEnabled()
{
	return !mButtons[BUTTON_CONTINUE]->mDisabled;
}

void BookManager::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	static Image* const* const kImages[] = {&IMAGE_UI_BOOK_BACK_BUTTON, &IMAGE_UI_BOOK_ENTER_BUTTON,
	                                        &IMAGE_UI_BOOK_OPTIONS_BUTTON};
	static const int kPos[][2] = {{36, 489}, {289, 555}, {664, 489}};
	for (int i = BUTTON_BACK; i <= BUTTON_OPTIONS; i++) {
		FadeButton* aButton = new FadeButton(i, mApp);  // the app hears these
		aButton->SetupImage(*kImages[i]);
		mButtons[i] = aButton;
		AddWidget(aButton);
		if (i == BUTTON_CONTINUE)
			aButton->SetDisabled(true);
		aButton->Move(kPos[i][0], kPos[i][1]);
	}
	CallLinkedMethod("Setup", 0);
	theWidgetManager->SetFocus(this);
	if (mApp->mArrowHintManager != nullptr)
		theWidgetManager->BringToFront(mApp->mArrowHintManager);
}

void BookManager::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	lua_State* L = gLuaSupport->L;
	lua_pushnil(L);
	lua_setglobal(L, "gBookManager");
}

bool BookManager::IsAsleep(bool theCheckState) const
{
	if (!mVisible && mDisabled && (!theCheckState || mState != STATE_NORMAL))
		return true;
	return mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr;
}

void BookManager::Update()
{
	if (IsAsleep(false) || mApp->GetDialogCount() > 0)
		return;
	LuaWidget::Update();
	if (!mApp->mVSyncUpdates)
		UpdateF(1.0f);
}

void BookManager::UpdateF(float theFrac)
{
	if (IsAsleep(false) || mApp->GetDialogCount() > 0)
		return;
	lua_pushnumber(gLuaSupport->L, theFrac);
	CallLinkedMethod("UpdateF", 1);
}

void BookManager::Draw(Graphics* g)
{
	if (IsAsleep(false))
		return;
	LuaWidget::Draw(g);
	for (size_t i = 0; i < mOverlayCallbacks.size(); i++) {
		PushGraphics(g);
		gLuaSupport->Call(mOverlayCallbacks[i], 1, 0);
		gLuaSupport->ObjectDeleted(g);
	}
}

void BookManager::OrderInManagerChanged()
{
	for (FadeButton* aButton : mMGButtons)
		mApp->mWidgetManager->PutInfront(aButton, this);
	if (mApp->mArrowHintManager != nullptr)
		mApp->mWidgetManager->BringToFront(mApp->mArrowHintManager);
}

void BookManager::KeyDown(KeyCode theKey)
{
	if ((mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr) || mApp->mCredits != nullptr ||
	    mApp->IsCurtainMoving())
		return;
	if (theKey == KEYCODE_RETURN)
		mApp->ButtonDepress(BUTTON_CONTINUE);
	LuaWidget::KeyDown(theKey);
}

void BookManager::MouseMove(int x, int y)
{
	if (IsAsleep(true))
		return;
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->MouseMove(x, y);
	LuaWidget::MouseMove(x, y);
	mApp->UpdateTooltipHover(x, y);
}

void BookManager::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if ((mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr) || mApp->mQueuedHydraLockout)
		return;
	// (not the Lua object's: the arrow hints hear it)
	if (mApp->mArrowHintManager != nullptr)
		mApp->mArrowHintManager->MouseDown(x, y, theClickCount);
}

void BookManager::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (IsAsleep(true) || mApp->mQueuedHydraLockout)
		return;
	LuaWidget::MouseUp(x, y, theBtnNum, theClickCount);
}
