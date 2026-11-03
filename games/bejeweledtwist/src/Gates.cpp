// Sexy::Gates, TopGate and BottomGate (Gates.h).
#include "Gates.h"

#include "EffectsManager.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "LoaderScreen.h"
#include "Logo.h"
#include "Menu.h"
#include "MenuButtonWidget.h"
#include "MenuPanels.h"
#include "Profile.h"
#include "Res.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

TopGate::TopGate()
{
	mMouseVisible = false;
	mHasAlpha = true;
}

BottomGate::BottomGate()
{
	mMouseVisible = false;
	mHasAlpha = true;
}

Gates::Gates()
{
	mOffset = (float)Ftol((double)gApp->mScale * -750.0);
	mOpen = true;
	mMouseVisible = false;
	mSpeed = 0;
	mHasAlpha = true;
	mUnknown394 = 0;
	mFlash = 0;
	mUnknown390 = 0;
	mTop = new TopGate();
	mTop->mClip = false;
	mBottom = new BottomGate();
	mBottom->mClip = false;

	// Without a MoreGamesURL property (the port's case) the first button shows how to play
	SexyString aMoreGamesURL = gApp->GetString(GTS(0x7e3200), _S(""));
	Image* aFirstImage;
	int aFirstId;
	SexyString aFirstTip;
	if (aMoreGamesURL.empty()) {
		aFirstTip = gApp->GetString(GTS(0x7e31f0), GT(0x7e31d0));
		aFirstId = 3;
		aFirstImage = IMAGE_MAINMENU_HELPBUTTON;
	} else {
		aFirstTip = gApp->GetString(GTS(0x7e31b0), GT(0x7e3174));
		aFirstId = 2;
		aFirstImage = IMAGE_MAINMENU_MOREGAMEBUTTON;
	}
	mHelpButton = gApp->MakeMenuButton(aFirstImage, IMAGE_MENU_BUTTONCOVER1, aFirstId, 0, 0, _S(""), aFirstTip);
	mHelpButton->mButtonListener = this;
	mProfileButton = gApp->MakeMenuButton(IMAGE_MAINMENU_PROFILEBUTTON, IMAGE_MENU_BUTTONCOVER2, 1, 0, 0, _S(""),
		gApp->GetString(GTS(0x7e315c), GT(0x7e312c)));
	mProfileButton->mButtonListener = this;
	mProfileButton->SetFlipImage(IMAGE_MAINMENU_CONTINUEBTN);
	mScoresButton = gApp->MakeMenuButton(IMAGE_MAINMENU_SCORESBUTTON, IMAGE_MENU_BUTTONCOVER1, 4, 0, 0, _S(""),
		gApp->GetString(GTS(0x7e3110), GT(0x7e30dc)));
	mScoresButton->mButtonListener = this;
	mOptionsButton = gApp->MakeMenuButton(IMAGE_MAINMENU_OPTIONSBUTTON, IMAGE_MENU_BUTTONCOVER1, 5, 0, 0, _S(""),
		gApp->GetString(GTS(0x7e30c4), GT(0x7e3098)));
	mOptionsButton->mButtonListener = this;
	mQuitButton = gApp->MakeMenuButton(IMAGE_MAINMENU_QUITBUTTON, IMAGE_MENU_BUTTONCOVER1, 6, 0, 0, _S(""),
		gApp->GetString(GTS(0x7e3078), GT(0x7e303c)));
	mQuitButton->mButtonListener = this;

	// The title comes from the loader when there is one
	if (gApp->mLoaderScreen == nullptr) {
		mLogo = new Logo(gApp->mFXManager);
	} else {
		mLogo = gApp->mLoaderScreen->mLogo;
		gApp->mWidgetManager->RemoveWidget(mLogo);
		gApp->mLoaderScreen->mLogo = nullptr;
	}
	mFadeLogo = false;
	Profile* aProfile = gApp->mProfile;
	mButtonsOn = aProfile->mName != GT(0x7da070) && !aProfile->mNames.empty();
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
}

Gates::~Gates()
{
	delete mTop;
	delete mBottom;
	delete mHelpButton;
	delete mProfileButton;
	delete mScoresButton;
	delete mOptionsButton;
	delete mQuitButton;
	delete mLogo;
}

void Gates::AddedToManager(WidgetManager* theWidgetManager)
{
	(void)theWidgetManager;
	AddWidget(mBottom);
	AddWidget(mTop);
	mBottom->AddWidget(mHelpButton);
	mBottom->AddWidget(mProfileButton);
	mBottom->AddWidget(mScoresButton);
	mBottom->AddWidget(mOptionsButton);
	mBottom->AddWidget(mQuitButton);
	AddWidget(mLogo);
}

void Gates::RemovedFromManager(WidgetManager* theWidgetManager)
{
	(void)theWidgetManager;
	RemoveAllWidgets(false, true);
}

void Gates::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	float anArtScale = (float)(gApp->mArtRes / 1200.0);
	mTop->mGateImage = IMAGE_MAINMENU_UI_MERGED;
	mTopY = 0;
	Image* anImage = mTop->mGateImage;
	mTop->Resize(mWidth / 2 - anImage->mWidth / 2, 0, anImage->mWidth, anImage->mHeight);
	mBottom->mGateImage = IMAGE_MAINMENU_UI_MERGED;
	mBottomY = mHeight - IMAGE_MAINMENU_UI_MERGED->GetCelHeight();
	anImage = mBottom->mGateImage;
	mBottom->Resize(mWidth / 2 - anImage->mWidth / 2, mBottomY, anImage->mWidth, anImage->mHeight);

	struct {
		MenuButtonWidget* mButton;
		int mId;
	} aPlaces[] = {{mScoresButton, IMAGE_MAINMENU_SCORESBUTTON_ID}, {mHelpButton, IMAGE_MAINMENU_MOREGAMEBUTTON_ID},
		{mOptionsButton, IMAGE_MAINMENU_OPTIONSBUTTON_ID}, {mQuitButton, IMAGE_MAINMENU_QUITBUTTON_ID},
		{mProfileButton, IMAGE_MAINMENU_PROFILEBUTTON_ID}};
	for (auto& aPlace : aPlaces) {
		aPlace.mButton->mX = Ftol((double)GetImageOffsetX(aPlace.mId) * anArtScale);
		aPlace.mButton->mY = Ftol((double)GetImageOffsetY(aPlace.mId) * anArtScale - mBottom->mY);
	}
	// The original nudges a few by a pixel at the smaller art sizes
	if (gApp->mArtRes == 600) {
		mScoresButton->mX += 1;
		mScoresButton->mY += 1;
		mHelpButton->mX -= 1;
		mHelpButton->mY -= 1;
		mOptionsButton->mX += 1;
		mOptionsButton->mY += 1;
		mQuitButton->mX += 1;
	} else if (gApp->mArtRes == 1200) {
		mHelpButton->mX -= 4;
		mQuitButton->mX += 2;
	}
	mLogo->Resize(0, 0, mWidth, mHeight);
}

int Gates::GetClosedTopY()
{
	return -10 - IMAGE_MAINMENU_UI_MERGED->GetCelHeight();
}

void Gates::Update()
{
	float anOldSpeed = mSpeed;
	Menu* aMenu = gApp->mMainMenu;
	if (mOpen) {
		float aTarget = (float)GetClosedTopY();
		bool aPosDone = !mPos.IncInVal();
		if (mLogo != nullptr && mFadeLogo) {
			float anAlpha = (float)(mLogo->mAlpha - 0.019999999552965164);
			mLogo->mAlpha = anAlpha < 0.0f ? 0.0f : anAlpha;
		}

		// Sparks along the seams: their number is random each update
		double aSparks = mSparks.GetOutVal();
		int aCount = Ftol(std::fabs(RandSigned()) * aSparks);
		for (; aCount > 0; aCount--) {
			int aSeam = Rand() % 4;
			int aLower = Rand() % 2;
			Effect* anEffect = gApp->mFXManager->AllocEffect(11);
			float anAngle = (float)(RandSigned() * 3.1415927410125732 * 0.15000000596046448 + 1.5707963705062866);
			float aSpeed = (float)(std::fabs(RandSigned()) * 0.019999999552965164 + 0.4000000059604645);
			anEffect->mImage = IMAGE_GATEPARTICLE;
			anEffect->mAngle = (float)(RandSigned() * 0.2);
			anEffect->mDX = std::cos(anAngle) * aSpeed;
			anEffect->mDY = std::sin(anAngle) * aSpeed;
			anEffect->mAlpha = (float)((mSparks.GetOutVal() + 30.0) / 300.0);
			anEffect->mIsAdditive = true;
			anEffect->mDAlpha = (float)(200.0 / (mSparks.GetOutVal() + 100.0) * -0.003000000026077032);
			anEffect->mScale = 0.3f;
			anEffect->mDScale = (float)((mSparks.GetOutVal() + 80.0) / 200.0 * 0.05000000074505806);
			if (aLower == 1)
				anEffect->mDY = -anEffect->mDY;
			int aSlide = Ftol(mSlide.GetOutVal() * Ftol((double)gApp->mScale * 90.0));
			static const double kSeamX[4] = {-201.0, -292.0, -1624.0, -1716.0};
			anEffect->mX = (float)(mTop->mX - Ftol((double)gApp->mScale * kSeamX[aSeam]));
			int aJitter = Ftol((double)gApp->mScale * 10.0);
			anEffect->mX = (float)(RandSigned() * aJitter + anEffect->mX);
			if (aSeam == 0 || aSeam == 3)
				anEffect->mColor = Color(255, 200, 255);
			else
				anEffect->mColor = Color(180, 160, 255);
			int aSeamY = Ftol((double)gApp->mScale * -573.0);
			int aY;
			if (aLower == 0)
				aY = mTop->mY - aSeamY + aSlide;
			else
				aY = -mTop->mY - aSeamY - aSlide;
			anEffect->mY = (float)aY;
			gApp->mFXManager->AddEffect(anEffect);
		}

		if (aPosDone) {
			float aSpeedNow = (float)(mSpeed - 0.8799999952316284);
			mSpeed = aSpeedNow;
			float anOffset = aSpeedNow + mOffset;
			mOffset = anOffset;
			if (!(aTarget < anOffset) && gApp->mBoard != nullptr) {
				mSpeed = 0;
				mOffset = aTarget;
				if (mParent != nullptr)
					mParent->RemoveWidget(this);
			}
		}
	} else {
		mProfileButton->SetGlowOn(aMenu->mNewUserPanel == nullptr && !aMenu->mNoProfile &&
			(aMenu->mPanel == 0 || aMenu->mPanel == 1));
		if (mLogo != nullptr && mLogo->mAlpha < 1.0f && !mOpen) {
			mLogo->mAlpha = (float)(mLogo->mAlpha + 0.009999999776482582);
			if (mLogo->mAlpha >= 1.0f)
				mLogo->mAlpha = 1.0f;
		}
		mPos.IncInVal();
		bool aMoving = mPos.GetOutVal() != 0.0;
		if (mPos.CheckInThreshold(0.33))
			gApp->PlaySample(SOUND_LOCK1);
		if (!aMoving && mSpeed != 0.0f) {
			mOffset = 0;
			mSpeed = 0;
			if (mButtonsOn && aMenu->mPanel == 0)
				SetButtonsOn(true);
		}
	}

	double aPos = mPos.GetOutVal();
	if (!(0.0 < aPos))
		aPos = 0.0;
	else
		aPos = mPos.GetOutVal();
	mOffset = (float)(Ftol((double)gApp->mScale * -750.0) * aPos);
	mTop->mY = Ftol(mTopY + (double)mOffset);
	mBottom->mY = Ftol(mBottomY - (double)mOffset);
	if (aMenu != nullptr) {
		if (aMenu->mProfilePanel != nullptr)
			aMenu->mProfilePanel->mY = mBottom->mY - Ftol((double)gApp->mScale * -1550.0);
		// The original then tells the menu (0x699bbc, empty) when the halves moved
		(void)anOldSpeed;
	}
	if (0.0f < mFlash) {
		float aFlash = (float)(mFlash - 0.014999999664723873);
		mFlash = aFlash < 0.0f ? 0.0f : aFlash;
	}
	MarkDirty();
}

void Gates::Draw(Graphics* g)
{
	if (0.0f < mFlash) {
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, Ftol(mFlash * 255.0)));
	}
	float aScale = gApp->mScale;
	int aSlideMax = Ftol((double)aScale * 90.0);
	int aSlide = Ftol(mSlide.GetOutVal() * aSlideMax);
	auto GapNow = [&]() {
		int aGap = -(mTop->mY + aSlide);
		return aGap < 1 ? 0 : aGap;
	};

	int aGap = GapNow();
	g->DrawImage(IMAGE_MAINMENU_UI_BOTTOM2, mBottom->mX,
		Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_BOTTOM2_ID)) + aGap);
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mInnerGlow.GetOutVal() * 255.0)));
	g->SetColorizeImages(true);
	aGap = GapNow();
	int aY = aGap + Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_BOTTOM2_GLOW_ID));
	g->DrawImage(IMAGE_MAINMENU_UI_BOTTOM2_GLOW,
		Ftol((double)aScale * GetImageOffsetX(IMAGE_MAINMENU_UI_BOTTOM2_GLOW_ID)) + mTop->mX, aY);
	g->PopState();

	aGap = GapNow();
	g->DrawImage(IMAGE_MAINMENU_UI_TOP2, mTop->mX,
		Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_TOP2_ID)) - aGap);
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mInnerGlow.GetOutVal() * 255.0)));
	g->SetColorizeImages(true);
	aGap = GapNow();
	aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_TOP2_GLOW_ID)) - aGap;
	g->DrawImage(IMAGE_MAINMENU_UI_TOP2_GLOW,
		Ftol((double)aScale * GetImageOffsetX(IMAGE_MAINMENU_UI_TOP2_GLOW_ID)) + mTop->mX, aY);
	g->PopState();

	g->DrawImageCel(IMAGE_MAINMENU_UI_MERGED, mBottom->mX, mBottom->mY, 1);
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mOuterGlow.GetOutVal() * 255.0)));
	g->SetColorizeImages(true);
	aY = Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_BOTTOM1_GLOW_ID)) - mTop->mY;
	g->DrawImage(IMAGE_MAINMENU_UI_BOTTOM1_GLOW,
		Ftol((double)aScale * GetImageOffsetX(IMAGE_MAINMENU_UI_BOTTOM1_GLOW_ID)) + mTop->mX, aY);
	g->PopState();

	g->DrawImageCel(IMAGE_MAINMENU_UI_MERGED, mTop->mX, mTop->mY, 0);
	g->PushState();
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(255, 255, 255, Ftol(mOuterGlow.GetOutVal() * 255.0)));
	g->SetColorizeImages(true);
	aY = mTop->mY + Ftol((double)aScale * GetImageOffsetY(IMAGE_MAINMENU_UI_TOP1_GLOW_ID));
	g->DrawImage(IMAGE_MAINMENU_UI_TOP1_GLOW,
		Ftol((double)aScale * GetImageOffsetX(IMAGE_MAINMENU_UI_TOP1_GLOW_ID)) + mTop->mX, aY);
	g->PopState();
}

void Gates::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	// The menu's zoom transform (+0x480) is pushed around the drawing while the menu's zoom is between 0 and 1
	Menu* aMenu = gApp->mMainMenu;
	if (aMenu->mZoom.GetOutVal() != 0.0 && aMenu->mZoom.GetOutVal() != 1.0)
		gD3DInterface->PushTransform(aMenu->mZoomMatrix, true);  // slot 0x14
	Widget::DrawAll(theFlags, g);
	if (aMenu->mZoom.GetOutVal() != 0.0 && aMenu->mZoom.GetOutVal() != 1.0)
		gD3DInterface->PopTransform();  // slot 0x18
}

void Gates::ButtonPress(int theId, int theClickCount)
{
	if (gApp->mMainMenu != nullptr)
		static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonPress(theId, theClickCount);
}

void Gates::ButtonDepress(int theId)
{
	if (gApp->mMainMenu != nullptr)
		static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonDepress(theId);
}

void Gates::SetButtonsOn(bool on)
{
	mHelpButton->SetGlowOn(on);
	mProfileButton->SetGlowOn(on);
	mScoresButton->SetGlowOn(on);
	mOptionsButton->SetGlowOn(on);
	mQuitButton->SetGlowOn(on);
	mButtonsOn = on;
}

void Gates::SetOpen(bool theOpen, bool theNow)
{
	if (theOpen == mOpen)
		return;
	mPos.SetInVal(0);
	if (!theOpen) {
		mSpeed = 1.0f;
		mPos.SetCurve(GTS(0x7e2e78));
		mSlide.SetCurve(GTS(0x7e2e64), &mPos);
		mSparks.SetCurve(GTS(0x7e2e50), &mPos);
		mOuterGlow.SetCurve(GTS(0x7e2e34), &mPos);
		mInnerGlow.SetCurve(GTS(0x7e2e1c), &mPos);
		mFadeLogo = false;
	} else {
		mPos.SetCurve(GTS(0x7e2f00));
		mSlide.SetCurve(GTS(0x7e2ee8), &mPos);
		mSparks.SetCurve(GTS(0x7e2ec8), &mPos);
		mOuterGlow.SetCurve(GTS(0x7e2eb0), &mPos);
		mInnerGlow.SetCurve(GTS(0x7e2e98), &mPos);
		if (!theNow)
			mFadeLogo = true;
	}
	mOpen = theOpen;
	bool aMouse = !theOpen;
	mHelpButton->mMouseVisible = aMouse;
	mProfileButton->mMouseVisible = aMouse;
	mScoresButton->mMouseVisible = aMouse;
	mOptionsButton->mMouseVisible = aMouse;
	mQuitButton->mMouseVisible = aMouse;
	if (!mOpen) {
		mVisible = true;
		if (!theNow)
			goto done;
		mOffset = 0;
		mSpeed = 0;
		mPos.SetInVal(1.0);
	} else if (!theNow) {
		mSpeed = mSpeed < 0.0f ? mSpeed : 0.0f;
	} else {
		mOffset = (float)GetClosedTopY();
		mSpeed = 0;
		mPos.SetInVal(mPos.mInMax);
		mSlide.SetInVal(mSlide.mInMax);
		mSparks.SetInVal(mSparks.mInMax);
		mOuterGlow.SetInVal(mOuterGlow.mInMax);
		mInnerGlow.SetInVal(mInnerGlow.mInMax);
		mLogo->mAlpha = 0;
	}
	// theNow: the original also tells the menu (0x699bbc, empty)
done:
	if (mOffset == 0.0f)
		mUnknown394 = 0.85f;
}
