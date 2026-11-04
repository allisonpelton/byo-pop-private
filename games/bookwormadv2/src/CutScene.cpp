#include "CutScene.h"

#include "CurtainTransition.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "OptionsDialog.h"
#include "PopLua.h"
#include "Res.h"
#include "WidgetManager.h"

using namespace Sexy;

CutScene::CutScene(GameApp* theApp) : mApp(theApp)
{
	mFlash = new FlashWidget(0, this);
	mFlash->mMouseOwner = this;
	AddWidget(mFlash);
	Resize(0, 0, mApp->mWidth, mApp->mHeight);
	mFlash->Resize(mApp->mWidth / 2 - 320, mApp->mHeight / 2 - 240, 640, 480);
	mFlash->SetVisible(false);
	mSkipButton = MakeDialogButton(static_cast<ButtonListener*>(this), GT(0x6fa1f4));
	AddWidget(mSkipButton);
	int aWidth = IMAGE_DIALOG_BUTTON->GetCelWidth();
	int aHeight = IMAGE_DIALOG_BUTTON->GetCelHeight();
	mSkipButton->Resize(mWidth / 2 - aWidth / 2, mHeight - aHeight - 10, aWidth, aHeight);
	mSkipButton->SetVisible(false);
	mSkipButton->SetDisabled(true);
	mWidgetFlagsMod.mAddFlags |= WIDGETFLAGS_MARK_DIRTY;
}

CutScene::~CutScene()
{
	RemoveAllWidgets(true, false);
}

void CutScene::Load(const std::string& theFileName, const std::string& theCallback, const std::string& theCaption)
{
	mFlash->StartAnimation(theFileName);
	mCallback = theCallback;
	mCaption = theCaption;
}

void CutScene::End()
{
	mFlash->Pause();
	mEnded = true;
	mFlash->mRedraw = false;
	mWidgetFlagsMod.mAddFlags |= ~WIDGETFLAGS_MARK_DIRTY;  // (as the original: |=, so it stays redrawn)
	if (mApp->mTransitionState == GameApp::TRANSITION_CUTSCENE) {
		mApp->mTransitionState = GameApp::TRANSITION_OPENING;
		mApp->mCurtain->StartClosing();
	}
	if (mApp->mKillCutSceneWhenDone && mApp->mCutScene != nullptr) {
		CutScene* aCutScene = mApp->mCutScene;
		mApp->mCutScene = nullptr;
		mApp->mWidgetManager->RemoveWidget(aCutScene);
		mApp->SafeDeleteWidget(aCutScene);
	}
	if (!mCallback.empty()) {
		lua_State* L = mApp->mLua->L;
		int aTop = lua_gettop(L);
		mApp->mLua->Call(mCallback, 0, 0);
		lua_settop(L, aTop);
	}
}

void CutScene::Skip()
{
	mSkipped = true;
	mFlash->mState = FlashWidget::STATE_STOPPED;
	End();
}

void CutScene::Update()
{
	Widget::Update();
	if (mSkipped)
		return;
	if (!mFlash->mVisible && mApp->mMainMenu == nullptr)
		mFlash->SetVisible(true);
	CurtainTransition* aCurtain = mApp->mCurtain;
	if ((aCurtain == nullptr || (aCurtain->mDone && aCurtain->mDrawn)) && !mSkipButtonEnabled) {
		mSkipButtonEnabled = true;
		mSkipButton->SetVisible(true);  // (the original shows it once the curtain is open: its movies are skipped by it)
		mSkipButton->SetDisabled(false);
	}
}

void CutScene::Draw(Graphics* g)
{
	if (mApp->mMainMenu == nullptr) {
		g->SetColor(Color::Black);
		g->FillRect(0, 0, mWidth, mHeight);
	}
	if (mWaitingForCurtain)
		return;
	g->DrawImage(IMAGE_CUTSCENE_TOP, 0, 0);
	g->DrawImage(IMAGE_CUTSCENE_BOTTOM, 0, mHeight - IMAGE_CUTSCENE_BOTTOM->GetCelHeight());
	g->SetFont(FONT_HUGE);
	g->SetColor(Color::White);
	g->DrawString(mCaption, mWidth / 2 - FONT_HUGE->StringWidth(mCaption) / 2 + 15, FONT_HUGE->GetAscent() + 5);
	if (mCanSkip) {
		g->SetFont(FONT_MEDIUM);
		g->SetColor(Color(255, 0, 0));  // (the hint is no longer written)
	}
}

void CutScene::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (mSkipped || !mCanSkip || mEnded || mWaitingForCurtain || mApp->IsCurtainMoving() ||
	    mApp->GetDialog(GameApp::DIALOG_SKIP_MOVIE) != nullptr)
		return;
	// the scene's number is the first character of the label the movie is at ("2start": 2)
	std::string aScene = mFlash->GetCurrentLabel().substr(0, 1);
	mFlash->GotoLabel(aScene + "end");
}

void CutScene::ButtonDepress(int theId)
{
	if (theId == 1) {
		mSkipped = true;
		mFlash->mState = FlashWidget::STATE_STOPPED;
		End();
		mSkipButton->SetDisabled(true);
	}
}

void CutScene::FlashAnimEnded(int theId)
{
	mEnded = true;
}

void CutScene::FlashCommand(int theId, const std::string& theCommand, const std::string& theParam)
{
	if (theCommand == "done" && !mSkipped)
		End();
}
