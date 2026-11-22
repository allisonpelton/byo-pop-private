// Sexy::MainMenu: see the header.
#include "MainMenu.h"

#include "Bej3App.h"
#include "EncodingParser.h"
#include "Font.h"
#include "Bej3Music.h"
#include "Graphics.h"
#include "X86Math.h"
#include "Profile.h"
#include "Res.h"

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
	mOptionsButton = new CrystalBall(GetTextById(0x11d, "OPTIONS"));
	mRecordsButton = new CrystalBall(GetTextById(0x11e, "RECORDS"));
	mPlayButton = new CrystalBall(GetTextById(0x11f, "PLAY"));
	AddWidget(mOptionsButton);
	AddWidget(mRecordsButton);
	AddWidget(mPlayButton);

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
	// port: the original calls slot 0x7c on each button here (unidentified): the plates stay undrawn until the intro
	// starts (CrystalBall::mActive); the side buttons are hidden
	mOptionsButton->mVisible = false;
	mRecordsButton->mVisible = false;
}

// port: the button stands in for the crystal ball (see the header)
MainMenu::~MainMenu()
{
	RemoveAllWidgets(true, true);
}

void MainMenu::Draw(Graphics* g)
{
	// 0x6a2a20 makes the 2D mode's background (images\NonResize\2donly\mainmenu\background) 1920 wide at 1200,
	// stretched over the menu; 0x6a0ca0 draws it first
	if (IMAGE_2DONLY_MAINMENU_BACKGROUND != nullptr)
		g->DrawImage(IMAGE_2DONLY_MAINMENU_BACKGROUND, 0, 0, mWidth, mHeight);
	// port: the foreground picture (images\NonResize\2donly\mainmenu\foreground), the logos, the particles and the tips

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

// 0x6a3530 (bejeweled3). port: left out: the logo phase, the widgets +0x6f0 to +0x6f8, the overlay widgets +0x700 and
// +0x704 (made when the intro starts), the user dialogs, the loader's unloading, the stars, the background widget
// (+0x6d8) and its Resize, ResizeButtons (0x6a09e0) when curve +0x190 is triggered, and the calls of slot 0x7c
void MainMenu::Update()
{
	Widget::Update();
	mShown = true;
	mPlayButton->mVisible = mShown;
	if (mIntroStarted) {
		mOptionsButton->mVisible = mShown;
		mRecordsButton->mVisible = mShown;
	}
	if (!mShown)
		return;

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
		mPlayButton->mActive = true;
		mOptionsButton->mActive = true;
		mRecordsButton->mActive = true;
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
