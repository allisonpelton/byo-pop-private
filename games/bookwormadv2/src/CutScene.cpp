#include "CutScene.h"

#include "CurtainTransition.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
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
	if (!mSkipped && !mFlash->mVisible && mApp->mMainMenu == nullptr)
		mFlash->SetVisible(true);
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
		g->SetColor(Color(255, 0, 0));
		g->WriteString(GT(0x7abf98), 0, 585, mWidth, 0);  // "(Click anywhere to skip.)"
	}
}

void CutScene::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (mSkipped || !mCanSkip || mEnded || mWaitingForCurtain || mApp->IsCurtainMoving() ||
	    mApp->GetDialog(GameApp::DIALOG_SKIP_MOVIE) != nullptr)
		return;
	mFlash->Pause();
	// "Are you sure?" / "Are you sure you want to skip the movie?"
	mApp->DoConfirmDialog(GameApp::DIALOG_SKIP_MOVIE, GT(0x7ac278), GT(0x7ac24c), nullptr, "");
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
