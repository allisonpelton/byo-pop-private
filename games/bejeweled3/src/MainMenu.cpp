// Sexy::MainMenu: see the header.
#include "MainMenu.h"

#include "Bej3App.h"
#include "DDImage.h"
#include "EncodingParser.h"
#include "Font.h"
#include "Bej3Music.h"
#include "Graphics.h"
#include "X86Math.h"
#include "PlayMenu.h"
#include "Profile.h"
#include "WidgetManager.h"
#include "Res.h"

#include <SDL.h>
#include <algorithm>

using namespace Sexy;

namespace {

// 0x475e30: the text of a numbered string in default.xml (its ids are the numbers 0x11d and up, in decimal there)
std::string GetTextById(int theId, const std::string& theDefault)
{
	return gApp->GetString(std::to_string(theId), theDefault);
}

}  // namespace

// 0x6a54b0 (bejeweled3): see the header
MainMenu::MainMenu()
{
	// 0x6a54b0: the buttons, made in the order OPTIONS, RECORDS, PLAY; the texts are string ids 0x11d, 0x11e, 0x11f
	// with the English words as their default
	// with the ids 0, 1 and 2 and the menu as their listener
	mOptionsButton = new CrystalBall(0, this, GetTextById(0x11d, "OPTIONS"));
	mRecordsButton = new CrystalBall(1, this, GetTextById(0x11e, "RECORDS"));
	mPlayButton = new CrystalBall(2, this, GetTextById(0x11f, "PLAY"));
	AddWidget(mOptionsButton);
	AddWidget(mRecordsButton);
	AddWidget(mPlayButton);
	// the PlayMenu (0x680760), the app's art size 1200 by 1920 scaled, a child of the menu (made after PLAY)
	mPlayMenu = new PlayMenu(this);
	mPlayMenu->Resize(0, 0, gApp->mArtRes * 0x4b0 / 0x4b0, gApp->mArtRes * 0x780 / 0x4b0);
	AddWidget(mPlayMenu);

	// 0x6a54b0: the menu's curves; +0x190 is a constant 0. This is the branch of 0x59b0a0 being false (the cache message
	// the port never has); the other branch sets +0x2e8, +0x518, +0x588 and +0x5f8 to 1 and leaves the rest
	mTurnACurve.SetConstant(0.0);
	mDistanceCurve.SetConstant(1.0);
	mBackgroundFadeCurve.SetConstant(0.0);
	mIntroCurve1.SetCurve("b;0,1,0.00125,1,~### Q~###        O####");
	mIntroCurve2.SetConstant(1.0);
	mIntroCurve3.SetCurve("b;0,1,0.005,1,####         ~~###");
	mIntroCurve4.SetCurve("b;0,1,0.005,1,####         ~~###");
	mIntroCurve7.SetCurve("b;0,1,0.002,1,#### O####    R~###   }~###");
	mIntroCurve5.SetConstant(1.0);
	mIntroCurve6.SetConstant(0.0);
	mSideFadeCurve.SetConstant(0.0);
	mNameCurve.SetConstant(0.0);
	// 0x6a54b0: the logos of partner.xml (NumLogos, Logo<n>File and Logo<n>HoldTime; SexyApp::Init has read the file)
	int aNumLogos = gApp->GetInteger("NumLogos", 0);
	for (int aLogoNum = 1; aLogoNum <= aNumLogos; aLogoNum++) {
		mBlackFade = 1.0f;
		MenuLogo aLogo;
		aLogo.mImage = gApp->GetImage(gApp->GetString(StrFormat("Logo%dFile", aLogoNum), ""));
		aLogo.mHold = gApp->GetInteger(StrFormat("Logo%dHoldTime", aLogoNum), 0);
		aLogo.mHoldLeft = aLogo.mHold;
		aLogo.mAlpha = 0;
		// a logo whose picture does not load is left out (0x422950 only under the image's test); the black fade is
		// still set. Steam's partner.xml names properties\partner_logo, which its install does not have
		if (aLogo.mImage != nullptr)
			mLogos.push_back(aLogo);
	}

	// slot 0x7c (0x46a780) is Widget::SetVisible: all three are hidden here, and the intro shows them
	mPlayButton->SetVisible(false);
	mOptionsButton->SetVisible(false);
	mRecordsButton->SetVisible(false);
}

// port: the button stands in for the crystal ball (see the header)
MainMenu::~MainMenu()
{
	RemoveAllWidgets(true, true);
}

// 0x6a0ca0 (bejeweled3)
void MainMenu::Draw(Graphics* g)
{
	// 0x6a0ca0: a black cover while a logo shows and as it fades away
	if (mBlackFade > 0.0f) {
		g->SetColor(Color(0, 0, 0, Ftol(mBlackFade * 255.0f)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	// the logo, stretched over the screen (the menu starts art size x 160 / 1200 left of it), and nothing else
	if (!mLogos.empty()) {
		int anArtRes = gApp->mArtRes;
		g->PushState();
		if (mLogos.front().mAlpha != 255)
			g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, mLogos.front().mAlpha));
		if (mLogos.front().mImage != nullptr)
			g->DrawImage(mLogos.front().mImage, anArtRes * 160 / 1200, 0, anArtRes * 1600 / 1200, anArtRes);
		g->PopState();
		return;
	}
	// 0x6a2a20 makes the 2D mode's background (images\NonResize\2donly\mainmenu\background) 1920 wide at 1200,
	// stretched over the menu; 0x6a0ca0 draws it first
	if (IMAGE_2DONLY_MAINMENU_BACKGROUND != nullptr)
		g->DrawImage(IMAGE_2DONLY_MAINMENU_BACKGROUND, 0, 0, mWidth, mHeight);
	// 0x6a0ca0: the foreground picture (the 2donly mainmenu foreground) rises from below while the cameras pull in
	// (curve +0x718) and slides with the turn (curve +0x190); it is drawn at its own size
	if (IMAGE_2DONLY_MAINMENU_FOREGROUND != nullptr) {
		int aBelow = gApp->mArtRes * 1500 / 1200;
		int aTop = Ftol((double)aBelow * mDistanceCurve.GetOutVal());
		int aLeft = Ftol(-600.0 - mTurnACurve.GetOutVal() * 600.0);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 255));
		g->DrawImage(IMAGE_2DONLY_MAINMENU_FOREGROUND, aLeft, aTop);
		g->SetColorizeImages(false);
	}
	// port: the stars, the stars' backdrop, the particles and the tips

	// 0x6a0ca0: the loading line at y = art size * 0x352 / 1200 (string id 0x122, "Loading..."), while the loading thread runs
	if (!gApp->mLoadingThreadCompleted && FONT_PLAYBUTTONS != nullptr) {
		std::string aText = GetTextById(0x122, "Loading...");
		g->SetFont(FONT_PLAYBUTTONS);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aText, (mWidth - FONT_PLAYBUTTONS->StringWidth(aText)) / 2, gApp->mArtRes * 0x352 / 1200);
	}
	// the player's name and "Click here to change user" (string id 0x120)
	if (gApp->mProfile != nullptr && !gApp->mProfile->mName.empty() && FONT_PLAYBUTTONS != nullptr) {
		g->SetFont(FONT_PLAYBUTTONS);
		g->SetColor(Color(255, 255, 255));
		std::string aName;
		for (wchar_t aChar : gApp->mProfile->mName)
			aName = AppendUTF8(aName, aChar);
		g->DrawString(aName, 10, mHeight - 10);
	}
}

// 0x6a2de0 (bejeweled3): a ball was pressed and let go over itself. Nothing works before the intro has started, nor
// while the PlayMenu is up. PLAY (id 2) plays the click, makes the PLAY ball grow to cover the screen and the PlayMenu
// fade in, and gives the PlayMenu the focus. OPTIONS (0) and RECORDS (1) turn the camera to one side
// (curve +0x190 to -1 or 1 and back), fade the side balls (+0x588) and the name (+0x5f8) and disable every ball while
// it turns: they show the widgets at +0x6f0 to +0x6f8, which are not translated, so here they only log. Ids 3, 4 and 7
// (those widgets' quit, user and help buttons) are not made.
void MainMenu::ButtonDepress(int theId)
{
	if (!mIntroStarted)
		return;
	if (mPlayMenu != nullptr && mPlayMenu->IsActive())
		return;
	switch (theId) {
	case 0:
	case 1:
		SDL_Log("MainMenu: ball %d pressed (the side screens are not translated)", theId);
		break;
	case 2:
		if (!mPlayButton->mScaleCurve.IsDoingCurve()) {
			gApp->PlaySoundEx(SOUND_CLICKFLYIN, 1.0, 0, 0.0);
			// port: the PopAnim background's curve (+0x6e0 object's +0x1a8: "b;0,1,0.002,1,####         ~~###") is left out
			mPlayMenu->mFadeCurve.SetCurve("b?0,1,0.01,1,####    C#.Z[    r~###j~###");
			mPlayButton->mBlendCurve.SetCurve("b;0,1,0.01,1,####         ~~###");
			mPlayButton->mScaleCurve.SetCurve("b;0.17,0.93,0.01,1,####         ~~Q(j");
			mWidgetManager->SetFocus(mPlayMenu);
		}
		break;
	}
}

// 0x6a0860 (bejeweled3): the mouse comes over a ball: SOUND_BUTTON_MOUSEOVER
void MainMenu::ButtonMouseEnter(int theId)
{
	gApp->PlaySoundEx(SOUND_BUTTON_MOUSEOVER, 1.0, 0, 0.0);
}

// 0x6a3530 (bejeweled3). port: left out: the logo phase, the widgets +0x6f0 to +0x6f8, the overlay widgets +0x700 and
// +0x704 (made when the intro starts), the user dialogs, the loader's unloading, the stars, the background widget
// (+0x6d8) and its Resize, ResizeButtons (0x6a09e0) when curve +0x190 is triggered.
void MainMenu::Update()
{
	Widget::Update();
	// 0x6a3530: the menu is hidden once the PlayMenu's +0x490 curve has reached 1 (the exe also hides it for the screen at
	// +0xbe8 and while +0xba4 is set: the port has neither yet), and not "shown" once the PlayMenu's fade has reached 1
	mVisible = !(mPlayMenu != nullptr && mPlayMenu->mHideCurve.GetOutVal() >= 1.0);
	mShown = mVisible && !(mPlayMenu != nullptr && mPlayMenu->mFadeCurve.GetOutVal() >= 1.0);
	mPlayButton->mVisible = mShown;
	if (!mShown)
		mPlayButton->mCover = SharedImageRef();   // 0x46a420 on +0x168
	if (mIntroStarted) {
		mOptionsButton->mVisible = mShown;
		mRecordsButton->mVisible = mShown;
	}
	if (!mShown)
		return;

	// 0x6a3530: while a logo shows nothing else runs: it fades in by 5 an update, holds, and fades out
	if (!mLogos.empty()) {
		mBlackFade = 1.0f;
		MenuLogo& aLogo = mLogos.front();
		if (aLogo.mAlpha < 255 && aLogo.mHoldLeft == aLogo.mHold) {
			aLogo.mAlpha += 5;
			if (aLogo.mAlpha > 254) {
				aLogo.mAlpha = 255;
				MarkDirty();
				return;
			}
		}
		else {
			aLogo.mHoldLeft--;
			if (aLogo.mHoldLeft < 1) {
				aLogo.mAlpha -= 5;
				if (aLogo.mAlpha < 1) {
					delete aLogo.mImage;
					mLogos.erase(mLogos.begin());
				}
			}
		}
		MarkDirty();
		return;
	}
	mBlackFade = std::max(mBlackFade - 0.05f, 0.0f);

	mDistanceCurve.IncInVal();

	// the loading progress, smoothed by 0.02 of the way each update
	double aProgress = gApp->GetLoadingThreadProgress();
	float aSmooth = (float)((aProgress - mLoadProgress) * 0.02 + mLoadProgress);
	mLoadProgress = aSmooth;
	if (aProgress >= 1.0 && !mIntroStarted && aSmooth >= 0.995f) {
		mIntroCurve7.SetCurveMult("b;0,1,0.005,1,~###   R~###      N####");
		mIntroCurve2.SetCurve("b;0,1,0.0025,1,~###b~###         ?####");
		mIntroCurve3.SetCurve("b;0,1,0.006667,1,~###         ~####");
		mDistanceCurve.SetCurve("b+0,1,0.001818,1,~###         ~#P##");
		mTurnBCurve.SetCurveMult("b;0,1,0.00303,1,~###         ~####");
		mIntroCurve5.SetCurve("b;0,1,0.005,1,~###         ~####");
		mIntroCurve4.SetCurve("b;0,1,0.01,1,~###         ~####");
		mIntroCurve6.SetCurve("b;0,1,0.006667,1,####         ~~###");
		mSideFadeCurve.SetCurve("b;0,1,0.001429,1,####      A#### |~### b~###");
		if (gApp->mProfile != nullptr && !gApp->mProfile->mName.empty())
			mNameCurve.SetCurve("b;0,1,0.001429,1,####       s#### Q~###Z~###");
		mOptionsButton->mScaleCurve.SetCurve("b;0,0.22,0.003333,1,####       -####  tP###");
		if (!mMusicSwitched) {
			mMusicSwitched = true;
			if (gApp->mBej3Music != nullptr)
				gApp->mBej3Music->QueueEvent("Switch", "", false);
		}
		mIntroStarted = true;
		mOptionsButton->SetVisible(true);
		mRecordsButton->SetVisible(true);
		mPlayButton->SetVisible(true);
	}

	// the background's fade in starts at a fifth of the loading
	if (!mBackgroundAdded && aProgress >= 0.2) {
		mBackgroundAdded = true;
		mBackgroundFadeCurve.SetCurve("b;0,1,0.005,1,####         ~~###");
	}

	// the two cameras: the same eye and target, turned by the curves
	float anAspect = (float)gApp->mWidth / (float)gApp->mHeight;
	double aBack = mDistanceCurve.GetOutVal();
	Vec3 anEye{0.0f, 0.0f, (float)(aBack + aBack + 0.25)};
	Vec3 aTarget{0.0f, 10.0f, (float)(aBack * 7.0 + 2.3499999046325684)};
	Vec3 anUp{0.0f, 0.0f, 1.0f};
	mCameraA.SetFov(anAspect * 38.5f, anAspect, 0.1f, 1000.0f);
	mCameraA.LookAt(anEye, aTarget, anUp);
	mCameraB.SetFov(anAspect * 38.5f, anAspect, 0.1f, 1000.0f);
	mCameraB.LookAt(anEye, aTarget, anUp);

	Transform3D aMove;
	aMove.mPos = Vec3{0.0f, -0.4f, 0.0f};
	mCameraA.mTransform = Compose(aMove, mCameraA.mTransform);
	mCameraB.mTransform = Compose(aMove, mCameraB.mTransform);
	Transform3D aTurn;
	aTurn.mRot = Mat3::TurnZ((float)(mTurnACurve.GetOutVal() * -0.7799999713897705));
	mCameraA.mTransform = Compose(aTurn, mCameraA.mTransform);
	aTurn.mRot = Mat3::TurnZ((float)mTurnBCurve.GetOutVal());
	mCameraB.mTransform = Compose(aTurn, mCameraB.mTransform);

	int anArtRes = gApp->mArtRes;
	double aWidth = (double)gApp->mWidth;
	double aHeight = (double)gApp->mHeight;
	// port: the app's viewport offset ([0x1c5]) is 0 here
	double aBallOffset = (double)(anArtRes * 0xa0 / 0x4b0);

	// PLAY
	int aPlaySize = anArtRes * 0xb4 / 0x4b0;
	Vec3 aPlay = mCameraB.ProjectWorld(Vec3{0.0f, 2.0f, 0.7f});
	double aPlayX = aWidth * aPlay.x + aBallOffset;
	double aPlayY = aHeight * aPlay.y;
	double aKeep = 1.0 - mPlayButton->mBlendCurve.GetOutVal();
	aPlayX *= aKeep;
	aPlayY *= aKeep;
	double aToMiddle = mPlayButton->mBlendCurve.GetOutVal();
	aPlayX += (double)(mWidth / 2) * aToMiddle;
	aPlayY += (double)(mHeight / 2) * aToMiddle;
	int aPlayX0 = Ftol(aPlayX);
	int aPlayY0 = Ftol(aPlayY);
	mPlayButton->mSubpixelX = aPlayX - (double)aPlayX0;
	mPlayButton->mSubpixelY = aPlayY - (double)aPlayY0;
	mPlayButton->Resize(aPlayX0 - aPlaySize / 2, aPlayY0 - aPlaySize / 2, aPlaySize, aPlaySize);
	mPlayButton->mDepth = aPlay.z;

	// OPTIONS and RECORDS
	int aSideSize = anArtRes * 0x78 / 0x4b0;
	bool aSidesTouchable = !mTurnACurve.IsDoingCurve() && !mDistanceCurve.IsDoingCurve();
	Vec3 anOptions = mCameraB.ProjectWorld(Vec3{-0.802f, 1.93f, 0.64f});
	mOptionsButton->Resize(Ftol(aWidth * anOptions.x + aBallOffset) - aSideSize / 2,
		Ftol(aHeight * anOptions.y) - aSideSize / 2, aSideSize, aSideSize);
	mOptionsButton->mDepth = anOptions.z;
	mOptionsButton->mMouseVisible = aSidesTouchable;
	if (mTurnACurve.GetOutVal() < 0.0)
		mOptionsButton->mFade = (float)mSideFadeCurve.GetOutVal();

	Vec3 aRecords = mCameraB.ProjectWorld(Vec3{0.802f, 1.93f, 0.64f});
	mRecordsButton->Resize(Ftol(aWidth * aRecords.x + aBallOffset) - aSideSize / 2,
		Ftol(aHeight * aRecords.y) - aSideSize / 2, aSideSize, aSideSize);
	mRecordsButton->mScaleCurve.SetConstant(mOptionsButton->mScaleCurve.GetOutVal());
	mRecordsButton->mDepth = aRecords.z;
	mRecordsButton->mMouseVisible = aSidesTouchable;
	if (mTurnACurve.GetOutVal() > 0.0)
		mRecordsButton->mFade = (float)mSideFadeCurve.GetOutVal();

	MarkDirty();
}
