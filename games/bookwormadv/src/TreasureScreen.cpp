#include "TreasureScreen.h"

#include "BookManager.h"
#include "FadeButton.h"
#include "GameApp.h"
#include "LuaBind.h"
#include "LuaSupport.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;
using namespace LuaLib;

TreasureScreen::TreasureScreen(GameApp* theApp) : mApp(theApp)
{
	RegisterLua();
	mEnums["ButtonId_Back"] = BUTTON_BACK;
	mEnums["ButtonId_Continue"] = BUTTON_CONTINUE;
	mEnums["ButtonId_Options"] = BUTTON_OPTIONS;
	mEnums["State_Normal"] = STATE_NORMAL;
	mEnums["State_SlidingOut"] = STATE_SLIDING_OUT;
	mEnums["State_SlidingIn"] = STATE_SLIDING_IN;
}

TreasureScreen::~TreasureScreen()
{
	RemoveAllWidgets(true, true);
	CallLinkedMethod("Destructor", 0);
}

// The original registered the class in the constructor (0x4a5340), every time
void TreasureScreen::RegisterLua()
{
	static bool sRegistered = false;
	if (sRegistered)
		return;
	sRegistered = true;
	Class<TreasureScreen>("TreasureScreen", false)
		.Base<LuaWidget>()
		.Method<&TreasureScreen::PositionButton>("PositionButton")
		.Method<&TreasureScreen::SetButtonState>("SetButtonState")
		.Method<&TreasureScreen::GetButtonX>("GetButtonX")
		.Method<&TreasureScreen::GetButtonY>("GetButtonY")
		.Method<&TreasureScreen::GetButtonW>("GetButtonW")
		.Method<&TreasureScreen::GetButtonH>("GetButtonH")
		.Method<&TreasureScreen::PreventRightClick>("PreventRightClick")
		.Method<&TreasureScreen::DoChapterTransition>("DoChapterTransition")
		.Method<&TreasureScreen::GetState>("GetState")
		.Method<&TreasureScreen::SetState>("SetState");
}

void TreasureScreen::PositionButton(int theId, int theX, int theY)
{
	mButtons[theId]->Move(theX, theY);
}

void TreasureScreen::SetButtonState(int theId, bool theVisible, bool theEnabled)
{
	FadeButton* aButton = mButtons[theId];
	bool wasEnabled = !aButton->mDisabled;
	aButton->SetVisible(theVisible);
	if (wasEnabled != theEnabled)
		aButton->SetDisabled(!theEnabled);
	// the continue button's glow plays from the start when it becomes enabled, and goes off when it is disabled
	if (theId == BUTTON_CONTINUE && wasEnabled != theEnabled) {
		if (!theEnabled)
			aButton->mEffectOff = true;
		else
			aButton->EnableEffect(true, true);
	}
}

int TreasureScreen::GetButtonRect(int theId, int theField)
{
	if (theId != BUTTON_BACK && theId != BUTTON_CONTINUE && theId != BUTTON_OPTIONS)
		return -1;
	FadeButton* aButton = mButtons[theId];
	switch (theField) {
	case 0: return aButton->mX;
	case 1: return aButton->mY;
	case 2: return aButton->mWidth;
	default: return aButton->mHeight;
	}
}

int TreasureScreen::GetButtonX(int theId) { return GetButtonRect(theId, 0); }
int TreasureScreen::GetButtonY(int theId) { return GetButtonRect(theId, 1); }
int TreasureScreen::GetButtonW(int theId) { return GetButtonRect(theId, 2); }
int TreasureScreen::GetButtonH(int theId) { return GetButtonRect(theId, 3); }

void TreasureScreen::DoChapterTransition()
{
	BookManager* aBook = mApp->mBookManager;
	aBook->SetVisible(true);
	aBook->SetDisabled(false);
	for (int i = 0; i < 3; i++)
		aBook->PositionButton(i, mButtons[BUTTON_BACK + i]->mX, mButtons[BUTTON_BACK + i]->mY);
	for (int anId = BUTTON_BACK; anId <= BUTTON_OPTIONS; anId++) {
		FadeButton* aButton = mButtons[anId];
		bool wasEnabled = !aButton->mDisabled;
		aButton->SetVisible(false);
		if (wasEnabled)
			aButton->SetDisabled(true);
	}
	mApp->mWidgetManager->SetFocus(aBook);
	aBook->CallLinkedMethod("DoChapterTransition", 0);
}

void TreasureScreen::StartFullyDisplayed()
{
	PositionButton(BUTTON_BACK, 36, 489);
	PositionButton(BUTTON_OPTIONS, 664, 489);
	PositionButton(BUTTON_CONTINUE, 289, 555);
	lua_pushboolean(LuaLib::gLuaSupport->L, false);
	CallLinkedMethod("StartFullyDisplayed", 1);
}

void TreasureScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	static Image* const* const kImages[] = {&IMAGE_UI_BOOK_BACK_BUTTON, &IMAGE_UI_BOOK_TS_CONTINUE_BUTTON,
	                                        &IMAGE_UI_BOOK_OPTIONS_BUTTON};
	for (int i = 0; i < 3; i++) {
		FadeButton* aButton = new FadeButton(BUTTON_BACK + i, mApp);  // the app hears these
		aButton->SetupImage(*kImages[i]);
		mButtons[BUTTON_BACK + i] = aButton;
		AddWidget(aButton);
		aButton->SetVisible(false);
		aButton->SetDisabled(true);
		if (BUTTON_BACK + i == BUTTON_CONTINUE) {
			// the continue button's glow: the attack button's animation, clipped to 308 by 90
			aButton->SetEffect("images/pam/fx_attackbutton.pam", -21, "EFFECT");
			aButton->mWidgetFlagsMod.mRemoveFlags &= WIDGETFLAGS_CLIP;
			aButton->mEffectRect = Rect(0, 0, 308, 90);
			aButton->mEffectOff = true;
		}
	}
	CallLinkedMethod("Setup", 0);
	theWidgetManager->SetFocus(this);
}

void TreasureScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	lua_State* L = gLuaSupport->L;
	lua_pushnil(L);
	lua_setglobal(L, "gTreasureScreen");
}

bool TreasureScreen::IsAsleep(bool theCheckState) const
{
	if (!mVisible && mDisabled && (!theCheckState || mState != STATE_NORMAL))
		return true;
	return mApp->mLoadingChapter && mApp->mChapterLoadScreen != nullptr;
}

void TreasureScreen::Update()
{
	if (!IsAsleep(false))
		LuaWidget::Update();
}

void TreasureScreen::Draw(Graphics* g)
{
	if (IsAsleep(false))
		return;
	LuaWidget::Draw(g);
	// in the timed mode, over its dialog 0
	if (mApp->GetDialogCount() > 0 && mApp->mTimedMode && mApp->GetDialog(GameApp::DIALOG_OK) != nullptr)
		DeferOverlay(2);
}

void TreasureScreen::DrawOverlay(Graphics* g)
{
	if (!IsAsleep(false))
		LuaWidget::DrawOverlay(g);
}

void TreasureScreen::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_RETURN && !mButtons[BUTTON_CONTINUE]->mDisabled && mApp->mCredits == nullptr &&
	    !mApp->IsCurtainMoving())
		mApp->ButtonDepress(BUTTON_CONTINUE);
}

void TreasureScreen::MouseMove(int x, int y)
{
	if (!IsAsleep(true))
		LuaWidget::MouseMove(x, y);
}

void TreasureScreen::MouseDrag(int x, int y)
{
	if (!IsAsleep(true))
		LuaWidget::MouseMove(x, y);  // (as the original: the Lua object hears a drag as a move)
}

void TreasureScreen::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (IsAsleep(true) || (mPreventRightClick && theBtnNum > 0))
		return;
	LuaWidget::MouseDown(x, y, theBtnNum, theClickCount);
}
