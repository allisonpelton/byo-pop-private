// Sexy::ChallengeMenu and Sexy::ChallengeButton (ChallengeMenu.h).
#include "ChallengeMenu.h"

#include "Board.h"
#include "Buffer.h"
#include "Challenge.h"
#include "Common.h"
#include "DDInterface.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Menu.h"
#include "PlayButton.h"
#include "Profile.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// The globals of the menu's camera and sun (ChallengeMenu.h)
CurvedVal Sexy::gSunCurve;
Transform3D Sexy::gMenuCamera;
float Sexy::gUnknown85C508 = 0;
Color Sexy::gUnknown85C50C;
Color Sexy::gSunGlowColor;
float Sexy::gAspect = 0;
float Sexy::gAspect45 = 0;

namespace {

// The planets in rows of this many, as far apart as these
const int PLANET_COLUMNS = 5;
const int PLANET_SIZE = 100;
const int PLANET_SPACING_X = 150;
const int PLANET_SPACING_Y = 130;

SexyString Number(int theNumber)
{
	return StringToSexyString(std::to_string(theNumber));
}

// port: a stand-in colour for each planet (the original's come from its pictures)
Color PlanetColor(int theCategory)
{
	double anAngle = (double)theCategory * 0.9;
	return Color(Ftol(150.0 + 100.0 * std::sin(anAngle)), Ftol(150.0 + 100.0 * std::sin(anAngle + 2.1)),
		Ftol(150.0 + 100.0 * std::sin(anAngle + 4.2)));
}

// 0x7570c1: the colour a blended towards b by theT (a's alpha kept)
Color BlendColors(const Color& a, const Color& b, float theT)
{
	Color aColor = a;
	aColor.mRed = Ftol((double)(b.mRed - a.mRed) * (double)theT + (double)aColor.mRed);
	aColor.mGreen = Ftol((double)(b.mGreen - a.mGreen) * (double)theT + (double)aColor.mGreen);
	aColor.mBlue = Ftol((double)(b.mBlue - a.mBlue) * (double)theT + (double)aColor.mBlue);
	return aColor;
}

// The camera curves are in degrees: the radians as a float, as 0x6e2318 and 0x6e3f41 make them
float CurveRadians(CurvedVal& theCurve)
{
	float aDegrees = (float)(theCurve.GetOutVal() + 0.0);
	return (float)((double)aDegrees * 3.1415927410125732 / 180.0);
}

}  // namespace

// 0x6e5337: the planet's id is its category's number (the original also gives it the category's pictures, its place
// in the 3D world, its text and the clouds; those are 3D only)
ChallengeButton::ChallengeButton(int theCategory, ChallengeMenu* theMenu) : ButtonWidget(theCategory, theMenu)
{
	mMenu = theMenu;
	mCategory = theCategory;
	mDoFinger = true;
	mHasAlpha = true;
	mHasTransparencies = true;
	Resize(0, 0, PLANET_SIZE, PLANET_SIZE);
}

void ChallengeButton::Draw(Graphics* g)
{
	// port: the planet as a flat disc (the original's is the software-lit sphere of 0x6dde98)
	int anAlpha = Ftol((double)mMenu->PlaceholderAmount() * 255.0);
	if (anAlpha <= 0)
		return;
	Color aColor = mOpen ? PlanetColor(mCategory) : Color(70, 70, 70);
	if (mEclipse)
		aColor = Color(110, 60, 150);
	int aBright = mIsOver && mOpen ? 40 : 0;
	g->SetColor(Color(aColor.mRed + aBright > 255 ? 255 : aColor.mRed + aBright,
		aColor.mGreen + aBright > 255 ? 255 : aColor.mGreen + aBright,
		aColor.mBlue + aBright > 255 ? 255 : aColor.mBlue + aBright, anAlpha));
	int aRadius = mWidth / 2;
	for (int y = -aRadius; y < aRadius; y++) {
		int aHalf = Ftol(std::sqrt((double)(aRadius * aRadius - y * y)));
		g->FillRect(aRadius - aHalf, aRadius + y, aHalf * 2, 1);
	}
	g->SetFont(FONT_DIALOG_BUTTONS);
	g->SetColor(Color(255, 255, 255, anAlpha));
	ChallengeInfo* anInfo = gApp->mChallengeInfo;
	SexyString anId = anInfo->GetCategoryId(mCategory);
	g->DrawString(anId, aRadius - g->GetFont()->StringWidth(anId) / 2, aRadius - g->GetFont()->GetHeight() / 2);
	if (mOpen) {
		int aCount = anInfo->GetChallengeCount(mCategory);
		int aStage = mEclipse ? aCount + 1 : (mStage < aCount ? mStage : aCount);
		SexyString aProgress = Number(aStage) + StringToSexyString("/") + Number(aCount);
		g->DrawString(aProgress, aRadius - g->GetFont()->StringWidth(aProgress) / 2,
			aRadius + g->GetFont()->GetHeight() / 2 + 2);
	}
}

// 0x6e5c3e
ChallengeMenu::ChallengeMenu()
{
	Resize(0, 0, gApp->mWidth, gApp->mHeight);
	mVisible = false;
	mHover = -1;
	mCategory = 0;
	mStageSel = 0;
	mStarted = 0;
	mHoverFade = 0;
	mUnknown9BC = false;
	mDragging = false;
	mCameraStackIndex = 0;
	mStartDelay = 0;
	mUnknown15C = 0;
	// The camera starts at (0, -12, 0) (+0x630) and the camera stack comes from its file (0x6e5a8d)
	mCameraPos[0] = 0;
	mCameraPos[1] = -12.0f;
	mCameraPos[2] = 0;
	LoadCameraStack();
	// The planets, one for each category (the original's also take their pictures and titles)
	for (unsigned int aCategory = 0; aCategory < (unsigned int)gApp->mChallengeInfo->mCategories.size(); aCategory++)
		mButtons.push_back(new ChallengeButton((int)aCategory, this));
	UpdateProgress(false, false);
	// The two buttons below: back to the play menu and to the main menu (0x754172 makes them)
	mPlayMenuButton = new TwistButton(BUTTON_PLAY_MENU, IMAGE_SIZEABLEBUTTON, this);
	mPlayMenuButton->mLabel = gApp->GetString(GTS(0x7e0cd0), GT(0x7e0cbc));
	mPlayMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	mMainMenuButton = new TwistButton(BUTTON_MAIN_MENU, IMAGE_SIZEABLEBUTTON, this);
	mMainMenuButton->mLabel = gApp->GetString(GTS(0x7e0cb0), GT(0x7e0c9c));
	mMainMenuButton->SetFont(FONT_DIALOG_BUTTONS);
	// Two particle managers (0x6f9f59): +0x1b8 is a widget of the menu, +0x1bc the sun's glow
	mFXManager1B8 = new EffectsManager();
	mFXManager1BC = new EffectsManager();
	TwistButton* aButtons[2] = {mPlayMenuButton, mMainMenuButton};
	for (TwistButton* aButton : aButtons) {
		// as wide as the label needs, with a tenth to spare, but not narrower than the picture
		double aLabelWidth = (double)FONT_DIALOG_BUTTONS->StringWidth(aButton->mLabel) * 1.1000000238418579;
		int aWidth = (double)aButton->mButtonImage->GetCelWidth() >= aLabelWidth ? aButton->mButtonImage->GetCelWidth()
		                                                                           : Ftol(aLabelWidth);
		aButton->Resize(0, 0, aWidth, aButton->mButtonImage->GetCelHeight());
	}
	SetState(0, false);
}

ChallengeMenu::~ChallengeMenu()
{
	for (ChallengeButton* aButton : mButtons) {
		RemoveWidget(aButton);
		delete aButton;
	}
	RemoveWidget(mPlayMenuButton);
	delete mPlayMenuButton;
	RemoveWidget(mMainMenuButton);
	delete mMainMenuButton;
	RemoveWidget(mFXManager1B8);
	delete mFXManager1B8;
	delete mFXManager1BC;
}

void ChallengeMenu::Layout()
{
	int aCount = (int)mButtons.size();
	int aColumns = aCount < PLANET_COLUMNS ? aCount : PLANET_COLUMNS;
	int aRows = aColumns == 0 ? 0 : (aCount + aColumns - 1) / aColumns;
	int aStartX = mWidth / 2 - ((aColumns - 1) * PLANET_SPACING_X) / 2;
	int aStartY = (mHeight - 90 - (aRows - 1) * PLANET_SPACING_Y) / 2;
	for (int i = 0; i < aCount; i++) {
		int aCenterX = aStartX + (i % aColumns) * PLANET_SPACING_X;
		int aCenterY = aStartY + (i / aColumns) * PLANET_SPACING_Y;
		mButtons[i]->Resize(aCenterX - PLANET_SIZE / 2, aCenterY - PLANET_SIZE / 2, PLANET_SIZE, PLANET_SIZE);
	}
	mPlayMenuButton->Move(mWidth / 2 - mPlayMenuButton->mWidth - 20, mHeight - 80);
	mMainMenuButton->Move(mWidth / 2 + 20, mHeight - 80);
}

// 0x6dfc49
void ChallengeMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	Layout();
	for (ChallengeButton* aButton : mButtons)
		AddWidget(aButton);
	AddWidget(mPlayMenuButton);
	AddWidget(mMainMenuButton);
	AddWidget(mFXManager1B8);
	mFXManager1BC->Resize(0, 0, mWidth, mHeight);
}

// 0x6dd2cd
void ChallengeMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	TwistWidget::RemovedFromManager(theWidgetManager);
	for (ChallengeButton* aButton : mButtons)
		RemoveWidget(aButton);
	RemoveWidget(mPlayMenuButton);
	RemoveWidget(mMainMenuButton);
	RemoveWidget(mFXManager1B8);
}

// 0x6e30d5
void ChallengeMenu::Show(int theMode, bool theNow)
{
	mUnknown9BC = false;
	mFXManager1BC->Clear();
	mVisible = true;
	UpdateProgress(false, false);
	SetState(theMode, theNow);
	if (!theNow) {
		for (ChallengeButton* aButton : mButtons)
			aButton->mMouseVisible = false;
	}
}

// 0x6dd42e
bool ChallengeMenu::IsOpenCurveRunning() const
{
	return 0.0 != mOpen.mIncRate && mOpen.mInMax != mOpen.mInVal;
}

// 0x6e3142: nothing happens while the open curve runs backwards
void ChallengeMenu::Close(int theState)
{
	if (IsOpenCurveRunning() && mOpen.mIncRate < 0.0)
		return;
	mMouseVisible = false;
	if (theState == 2)
		gApp->mMainMenu->PlayMusic(0);
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	for (ChallengeButton* aButton : mButtons) {
		if (theState == 2)
			aButton->mUnknown5C4 = false;
		aButton->mMouseVisible = false;
	}
	SetState(theState, false);
}

// 0x6e2318: the curves of the camera, the sun, the buttons and the moon for each state. All but the first state's are
// linked to the open curve (mOpen), which is what moves them. A state 1 or 2 menu flies the camera in or out; the camera's
// start for the zoom into a picked planet (mZoomPos) comes from the picked planet.
void ChallengeMenu::SetState(int theState, bool theNow)
{
	switch (theState) {
	case 0:
		// away
		mOpen.SetConstant(0.0);
		mButtonSlide.SetConstant(0.0);
		mStartDelay = 0.0f;
		gSunCurve.SetConstant(0.0);
		break;
	case 1: {
		// shown by the play menu: the camera flies in from the side
		gApp->mMainMenu->PlayMusic(0x130);
		mOpen.SetCurve(GTS(0x7e06a8), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e06a8), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e068c), nullptr);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), nullptr);
		mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		if (theNow) {
			// at once: one step before the ends
			mOpen.mInVal = mOpen.mInMax - 0.0010000000474974513;
			mUnknown6B0.mInVal = mUnknown6B0.mInMax - 0.0010000000474974513;
			mUnknown640.mInVal = mUnknown640.mInMax - 0.0010000000474974513;
		}
		mCameraRotX.SetCurve(GTS(0x7e066c), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0648), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0620), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e05fc), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e05dc), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e05bc), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		// The turn the camera starts at (the three axes' curves at 0), turned 0.625 of the way to the identity, and
		// its transpose (0x4c1670 with the identity for the other, then 0x6dda5c): mCameraTarget
		Mat3 aRotation;
		aRotation.RotateZ(CurveRadians(mCameraRotZ));
		aRotation.RotateX(CurveRadians(mCameraRotX));
		aRotation.RotateY(CurveRadians(mCameraRotY));
		Mat3 anIdentity;
		mCameraTarget = SlerpMat3(aRotation, anIdentity, 0.625f, false).Transposed();
		break;
	}
	case 2:
		// leaving for a menu: the camera flies out the other way
		mOpen.SetCurve(GTS(0x7e0828), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e07fc), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e07dc), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mCameraRotX.SetCurve(GTS(0x7e07bc), &mOpen);
		mCameraRotY.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraRotZ.SetCurve(GTS(0x7e0798), &mOpen);
		mCameraDistance.SetCurve(GTS(0x7e0778), &mOpen);
		mCameraRight.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraUp.SetCurve(GTS(0x7e0758), &mOpen);
		mCameraShiftX.SetCurve(GTS(0x7e0738), &mOpen);
		mCameraShiftY.SetCurve(GTS(0x7e0718), &mOpen);
		mUnknown720.SetCurve(GTS(0x7e06f0), &mOpen);
		mCameraTurn.SetCurve(GTS(0x7e06c4), &mOpen);
		break;
	case 3: {
		// leaving for a challenge: the camera flies to the picked planet
		float aWas = (float)mOpen.mInVal;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e089c), &mOpen);
		mStartDelay = 0.0f;
		gSunCurve.SetCurve(GTS(0x7e087c), &mOpen);
		mUnknown6B0.SetCurve(GTS(0x7e0858), &mOpen);
		mUnknown640.SetCurve(GTS(0x7e0908), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e0940), &mOpen);
		for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++)
			mButtons[i]->mUnknown5C5 = i != (unsigned int)mCategory;
		if (mState == 4) {
			// the menu was still coming in: the way out begins where the way in stood
			float aLeft = (float)(1.0 - (double)aWas);
			if (0.00999999977648258f < aLeft)
				mOpen.mInVal = (double)aLeft;
		}
		break;
	}
	case 4: {
		// shown at once (after a game): the camera flies from the picked planet
		gApp->mMainMenu->PlayMusic(0x130);
		mVisible = true;
		mOpen.SetCurve(GTS(0x7e0940), nullptr);
		mButtonSlide.SetCurve(GTS(0x7e0940), &mOpen);
		mStartDelay = 0.0f;
		if (mState != 5) {
			gSunCurve.SetCurve(GTS(0x7e0924), &mOpen);
			mUnknown640.SetCurve(GTS(0x7e0908), nullptr);
		}
		mUnknown6B0.SetCurve(GTS(0x7e08e4), &mOpen);
		mUnknown720.SetConstant(0.0);
		mCameraTurn.SetConstant(1.0);
		ChallengeButton* aPicked = mButtons[mCategory];
		mZoomPos[0] = aPicked->mWorldPos[0];
		mZoomPos[1] = aPicked->mWorldPos[1];
		mZoomPos[2] = aPicked->mWorldPos[2];
		mZoom.SetCurve(GTS(0x7e08c0), &mOpen);
		UpdateProgress(false, false);
		break;
	}
	case 5:
		// only shown: the sun's glow all the way
		gSunCurve.SetConstant(1.0);
		mVisible = true;
		break;
	}
	mState = theState;
	MarkDirty();
}

// 0x6e5a8d: properties/camerastack.txt, two lines of three numbers each for each camera (a blank line between)
void ChallengeMenu::LoadCameraStack()
{
	mCameraStack.clear();
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile("properties/camerastack.txt", &aBuffer, true))
		return;
	// the lines (the original reads them with fgets in text mode)
	std::vector<std::string> aLines;
	std::string aLine;
	for (unsigned char aChar : aBuffer.mData) {
		if (aChar == '\n') {
			aLines.push_back(aLine);
			aLine.clear();
		} else if (aChar != '\r') {
			aLine += (char)aChar;
		}
	}
	if (!aLine.empty())
		aLines.push_back(aLine);
	for (size_t i = 0; i < aLines.size(); i++) {
		if (Trim(aLines[i]).empty())
			continue;
		CameraStackEntry anEntry = {{0, 0, 0}, {0, 0, 0}};
		std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mPos[0], &anEntry.mPos[1], &anEntry.mPos[2]);
		i++;
		if (i < aLines.size())
			std::sscanf(aLines[i].c_str(), "%f,%f,%f\n", &anEntry.mRot[0], &anEntry.mRot[1], &anEntry.mRot[2]);
		mCameraStack.push_back(anEntry);
	}
}

// port: the temporary planets fade with the open curve: in while the menu comes in (states 1, 4, 5), out while it goes
// out (2, 3), none while it is away
float ChallengeMenu::PlaceholderAmount()
{
	if (mState == 5)
		return 1.0f;
	double anAmount = mOpen.GetOutVal();
	if (mState == 1 || mState == 4)
		return (float)anAmount;
	if (mState == 2 || mState == 3)
		return (float)(1.0 - anAmount);
	return 0.0f;
}

// 0x6e0ddb
int ChallengeMenu::UpdateProgress(bool theAnimate, bool theClearTips)
{
	bool aAdvanced = false;
	bool anOpened = false;
	ChallengeInfo* anInfo = gApp->mChallengeInfo;
	Profile* aProfile = gApp->mProfile;
	for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
		ChallengeButton* aButton = mButtons[i];
		int aStage = aProfile->GetChallengeStage((int)i);
		// (theClearTips: the original also deletes the planet's tip widget at +0x650)
		(void)theClearTips;
		if (!aButton->mEclipse) {
			if (aButton->mStage != aStage) {
				int aCount = anInfo->GetChallengeCount(i);
				if (aCount < aStage) {
					// every challenge done: the planet stays at the last one
					aButton->mOpen = true;
					aButton->mStage = aCount;
					if (theAnimate)
						aAdvanced = true;
				} else {
					// port: the original, animating (and past the first stage), moves the planet on after a glow
					// (the new stage then shows); the port moves it at once
					if (theAnimate && aStage != 1)
						aAdvanced = true;
					aButton->mStage = aStage;
				}
			}
		}
		aButton->mProgress = (int)i < (int)aProfile->mInts204.size() ? aProfile->mInts204[i] : 0;
		aButton->mEclipse = anInfo->GetChallengeCount(i) < aStage;
		bool anOpen = aProfile->IsChallengeCategoryOpen((int)i);
		if (!anOpen) {
			aButton->mOpen = false;
			aButton->mMouseVisible = false;
		} else if (!aButton->mOpen) {
			if (theAnimate)
				anOpened = true;
			aButton->mOpen = true;
			aButton->mMouseVisible = true;
		}
	}
	if (mHover >= 0)
		ButtonMouseEnter(mHover);
	if (anOpened)
		return 0xFA;
	return aAdvanced ? 200 : 0;
}

// 0x6e3f41
void ChallengeMenu::Update()
{
	TwistWidget::Update();
	gSunCurve.IncInVal();
	mUnknown6B0.IncInVal();
	mUnknown640.IncInVal();
	// 1 - mUnknown6B0, but not below 0.25
	float aFraction = 0.25f;
	if (0.25 <= 1.0 - mUnknown6B0.GetOutVal())
		aFraction = (float)(1.0 - mUnknown6B0.GetOutVal());
	gUnknown85C508 = aFraction;
	if (mHover == -1) {
		mHoverFade = (float)((double)mHoverFade - 0.03999999910593033);
		if (mHoverFade < 0.0f)
			mHoverFade = 0.0f;
		mUnknown9D0 = (float)((double)mUnknown9D0 + 0.009999999776482582);
		if (0.5f < mUnknown9D0) {
			mUnknown9D4 = (float)((double)mUnknown9D4 + 0.019999999552965164);
			if (1.0f < mUnknown9D4)
				mUnknown9D4 = 1.0f;
		}
	} else {
		mHoverFade = (float)((double)mHoverFade + 0.02500000037252903);
		if (1.0f < mHoverFade)
			mHoverFade = 1.0f;
		mUnknown9D0 = 0;
		mUnknown9D4 = 0;
	}
	// The glow colours follow mUnknown928 (0x7570c1 blends two colours, the first's alpha kept)
	mUnknown928.IncInVal();
	float aBlend = (float)mUnknown928.GetOutVal();
	gSunGlowColor = BlendColors(Color(255, 255, 255), Color(255, 128, 255), aBlend);
	aBlend = (float)mUnknown928.GetOutVal();
	gUnknown85C50C = BlendColors(Color(128, 32, 128), Color(0, 0, 128), aBlend);
	mFXManager1BC->Update();
	// The mouse drag changes the camera stack's entry mCameraStackIndex: sideways it turns about y (the right button,
	// about z), up and down about x
	WidgetManager* aWidgetManager = gApp->mWidgetManager;
	int aMouseX = aWidgetManager->mLastMouseX;
	int aMouseY = aWidgetManager->mLastMouseY;
	if ((aWidgetManager->mDownButtons & 1) == 0) {
		mDragging = false;
	} else if (!mDragging) {
		mDragStartX = aMouseX;
		mDragStartY = aMouseY;
		mDragging = true;
	} else {
		int aDeltaX = aMouseX - mLastMouseX;
		int aDeltaY = aMouseY - mLastMouseY;
		// (with no camera stack the original would stop on its bounds check; the file ships with the game)
		if ((size_t)mCameraStackIndex < mCameraStack.size()) {
			CameraStackEntry& anEntry = mCameraStack[mCameraStackIndex];
			if ((aWidgetManager->mDownButtons & 2) == 0) {
				anEntry.mRot[1] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[1]);
				anEntry.mRot[0] = (float)((double)aDeltaY * 0.0020000000949949026 + (double)anEntry.mRot[0]);
			} else {
				anEntry.mRot[2] = (float)((double)aDeltaX * 0.0020000000949949026 + (double)anEntry.mRot[2]);
			}
		}
	}
	mLastMouseX = aMouseX;
	mLastMouseY = aMouseY;
	if (IsOpenCurveRunning()) {
		if (0.0f < mStartDelay) {
			mStartDelay = (float)((double)mStartDelay - 0.009999999776482582);
		} else if (!mOpen.IncInVal()) {
			// the curve has ended
			mUnknown9D4 = 0;
			mHoverFade = 0;
			bool anIn = mOpen.GetOutVal() == 1.0;
			mMouseVisible = anIn;
			mMainMenuButton->mMouseVisible = anIn;
			mPlayMenuButton->mMouseVisible = anIn;
			if (mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4))
				mUnknown640.SetCurve(GTS(0x7e0b40), nullptr);
			for (unsigned int i = 0; i < (unsigned int)mButtons.size(); i++) {
				if (mState == 1)
					mButtons[i]->mUnknown5C4 = mMouseVisible && mButtons[i]->mOpen;
				mButtons[i]->mMouseVisible = mMouseVisible && mButtons[i]->mOpen;
			}
		}
	}
	// The camera (a Transform3D, kept for the planets' projection): states 1 and 2 follow the camera curves, the others
	// fly from the start to the picked planet (mZoom); state 5 leaves it as it is
	if (mState != 5) {
		Transform3D aCamera;
		if (mState == 1 || mState == 2) {
			aCamera.mRot.RotateZ(CurveRadians(mCameraRotZ));
			aCamera.mRot.RotateX(CurveRadians(mCameraRotX));
			aCamera.mRot.RotateY(CurveRadians(mCameraRotY));
			float aDistance = (float)(mCameraDistance.GetOutVal() + 16.149999618530273);
			Vec3 aRow0 = aCamera.mRot.Row(0);
			Vec3 aRow1 = aCamera.mRot.Row(1);
			Vec3 aRow2 = aCamera.mRot.Row(2);
			// the start, a distance along the third axis, some along the first and the second, and the shifts
			float aX = aRow2.x * aDistance + mCameraPos[0];
			float aY = mCameraPos[1] + aRow2.y * aDistance;
			float aZ = mCameraPos[2] + aRow2.z * aDistance;
			float aRight = (float)mCameraRight.GetOutVal();
			aCamera.mPos.x = aRow0.x * aRight + aX;
			aCamera.mPos.y = aRow0.y * aRight + aY;
			aCamera.mPos.z = aRow0.z * aRight + aZ;
			float anUp = (float)mCameraUp.GetOutVal();
			aCamera.mPos.x = aRow1.x * anUp + aCamera.mPos.x;
			aCamera.mPos.y = aRow1.y * anUp + aCamera.mPos.y;
			aCamera.mPos.z = aRow1.z * anUp + aCamera.mPos.z;
			float aShiftX = (float)mCameraShiftX.GetOutVal();
			aCamera.mPos.x = aShiftX + aCamera.mPos.x;
			float aShiftY = (float)mCameraShiftY.GetOutVal();
			aCamera.mPos.y = aShiftY + aCamera.mPos.y;
		} else {
			float aFly = (float)mZoom.GetOutVal();
			float aFromX = (float)((double)mCameraPos[0] + 0.0);
			float aFromY = (float)((double)mCameraPos[1] + 0.0);
			float aFromZ = (float)((double)mCameraPos[2] + 16.149999618530273);
			float aToX = (float)((double)mZoomPos[0] + 0.0);
			float aToY = (float)((double)mZoomPos[1] + 0.15000000596046448);
			float aToZ = (float)((double)mZoomPos[2] + 7.5);
			aCamera.mPos.x = (float)(((double)aToX - (double)aFromX) * (double)aFly + (double)aFromX);
			aCamera.mPos.y = (float)(((double)aToY - (double)aFromY) * (double)aFly + (double)aFromY);
			aCamera.mPos.z = (float)(((double)aToZ - (double)aFromZ) * (double)aFly + (double)aFromZ);
		}
		gMenuCamera = aCamera;
	}
	// The hyperspace's 3D camera turns from the identity to the menu's by mCameraTurn
	if (gApp->mHyperspace != nullptr) {
		float aTurn = (float)mCameraTurn.GetOutVal();
		Mat3 anIdentity;
		gApp->mHyperspace->mUnknown1548 = SlerpMat3(anIdentity, mCameraTarget, aTurn, false);
	}
	// The buttons rise with mButtonSlide (0x34 is the buttons' y)
	mPlayMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	mMainMenuButton->mY = Ftol(((1.0 - mButtonSlide.GetOutVal()) * 0.5 + 0.9250000119209290) * (double)mHeight);
	// The glow's particles ride down with the open curve
	double anOpen = mOpen.GetOutVal();
	int aGlowShift = Ftol((double)gApp->mScale * 1500.0);
	mFXManager1BC->mY = Ftol((double)aGlowShift * (1.0 - anOpen));
	mFXManager1BC->mAlpha = (mState == 1 || mState == 4 || mState == 5) ? (float)gSunCurve.GetOutVal() : 0.0f;
	gAspect = (float)((double)gApp->mDDInterface->mWidth / (double)gApp->mDDInterface->mHeight);
	gAspect45 = (float)((double)gAspect * 45.0);
	// The sun's glow throws particles about while the menu is all the way in
	if (mStarted == 0 && mOpen.GetOutVal() == 1.0 && (mState == 1 || mState == 4)) {
		float aRadius = (float)((double)IMAGE_ECLIPSE_SUN->mWidth * 0.30000001192092896);
		int aCount;
		if (gApp->Is3DAccelerated()) {
			NotPorted("ChallengeMenu::Update 0x6e47a2: two glow particles each update with Direct3D");
			aCount = 0;
		} else {
			aCount = (int)(Rand() % 100) / 80;
		}
		while (0 < aCount) {
			Effect* anEffect = mFXManager1BC->AllocEffect(0);
			anEffect->mImage = GetImageById((Rand() & 1) == 0 ? IMAGE_ECLIPSE_FLARE1_ID : IMAGE_ECLIPSE_FLARE2_ID);
			float aSpeed = (float)((double)gApp->mScale * 0.30000001192092896);
			aSpeed = (float)std::fabs(RandSigned() * aSpeed);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mX = (float)((double)(std::cos(anAngle) * aRadius) + (double)(mWidth / 2));
			anEffect->mY = (float)((double)(std::sin(anAngle) * aRadius) + (double)(mHeight / 2));
			anEffect->mDX = std::cos(anAngle) * aSpeed;
			anEffect->mDY = std::sin(anAngle) * aSpeed;
			anEffect->mAlpha = 0;
			anEffect->mDAlpha = 0.0024999999441206455f;
			anEffect->mMaxAlpha = 0.25f;
			anEffect->mFlags |= Effect::FLAG_ALPHA_BOUNCE;
			anEffect->mScale = (float)((double)RandSigned() * 0.10000000149011612 + 0.5);
			anEffect->mDScale = 0.004999999888241291f;
			anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.0024999999441206455);
			int aGrey = (int)(Rand() % 15) + 240;
			anEffect->mColor = Color(aGrey, aGrey, aGrey);
			anEffect->mIsAdditive = false;
			mFXManager1BC->AddEffect(anEffect);
			aCount--;
		}
	}
	if (0.0f < mUnknown15C) {
		float aLeft = (float)((double)mUnknown15C - 0.029999999329447746);
		mUnknown15C = 0.0f < aLeft ? aLeft : 0.0f;
	}
	// NotPorted (0x6e4989-0x6e4a1f): the original also steps a widget at +0x9dc (the board's code sets it, 0x6b3412;
	// an unlock message whose id is 250): it adds to a float of the widget's owner and, once the widget's flag at +0xc9 is
	// set, removes and deletes it and goes from state 5 to state 4. Its type is not identified
	MarkDirty();
}

// 0x6dd322 (the sun, the camera's world and the planets' tips are 3D; here the picked planet's text)
void ChallengeMenu::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	if (mHover < 0 || mHover >= (int)mButtons.size() || mHoverFade <= 0.0f || PlaceholderAmount() <= 0.0f)
		return;
	ChallengeButton* aButton = mButtons[mHover];
	int aNumber = aButton->mStage + (aButton->mEclipse ? 1 : 0);
	Challenge* aChallenge = gApp->mChallengeInfo->GetChallenge(aButton->mCategory, aNumber);
	if (aChallenge == nullptr)
		return;
	float aFraction = 0;
	SexyString aText = gApp->mChallengeInfo->GetText(aChallenge, false, false, &aFraction);
	int anAlpha = Ftol((double)mHoverFade * (double)PlaceholderAmount() * 255.0);
	g->SetFont(FONT_DIALOG_BUTTONS);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->DrawString(aText, mWidth / 2 - g->GetFont()->StringWidth(aText) / 2, mHeight - 120);
}

// 0x6dfe2a: the camera's zoom
void ChallengeMenu::MouseWheel(int theDelta)
{
	(void)theDelta;
}

// 0x6e54c3
void ChallengeMenu::ButtonDepress(int theId)
{
	int aSound = SOUND_BUTTONPRESS2;
	if (theId == BUTTON_PLAY_MENU) {
		Close(2);
		gApp->ShowModeSelect(0);
		PlayButton* aPlayButton = gApp->mMainMenu->mPlayButton;
		aPlayButton->mAppear.SetCurve(GTS(0x7e0bb0));
		aPlayButton->mAlpha.SetCurve(GTS(0x7e0b88));
	} else if (theId == BUTTON_MAIN_MENU) {
		Close(2);
		gApp->ShowMainMenu(false, false, 0.0f);
	} else if (!mUnknown9BC) {
		ChallengeButton* aButton = mButtons[theId];
		mCategory = aButton->mCategory;
		mStageSel = aButton->mStage;
		if (aButton->mEclipse)
			mStageSel++;
		if (!gApp->mRegistered && (mStageSel > 3 || mCategory > 2)) {
			// The trial: a click sound, and the planet shakes (its curves at +0x758 and on, 3D only)
			aSound = SOUND_TRIALCLICK;
		} else if (!gApp->SaveExists(2, mCategory)) {
			Start(true);
		} else {
			TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 0, true, GT(0x80e8a4),
				gApp->GetString(GTS(0x7e0c44), GT(0x7e0bf0)), GT(0x80d9a4), 7);
			aDialog->mYesLabel = gApp->GetString(GTS(0x80f054), GT(0x7e0bdc));
			aDialog->mNoLabel = gApp->GetString(GTS(0x7e0bd8), GT(0x7e0bd0));
			aDialog->PopUp();
			aDialog->mTwistListener = this;
			double aMinWidth = (double)gGemWidth * 6.75;
			double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
			aDialog->mWidth = Ftol(aWidth);
			aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
			gApp->AddDialog(aDialog);
			aDialog->mX = (mWidth / 2 - aDialog->mWidth / 2) + mX;
			aDialog->mY = (mHeight / 2 - aDialog->mHeight / 2) + mY;
		}
	}
	gApp->PlaySample(aSound);
}

// 0x6e58f7: Continue goes on with the saved game, the other button gives it up
void ChallengeMenu::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId == 0) {
		bool aNewGame = true;
		if (theButtonId == Dialog::ID_YES) {
			aNewGame = false;
		} else if (theButtonId == Dialog::ID_NO) {
			aNewGame = true;
			gApp->DeleteSave(2, mCategory);
		}
		Start(aNewGame);
	}
	gApp->KillDialog(theDialogId);
}

// 0x6e665e: the planet's text shows (the original builds its two pictures here)
void ChallengeMenu::ButtonMouseEnter(int theId)
{
	if (!mMouseVisible || theId >= 100)
		return;
	mHover = theId;
}

// 0x6dd418
void ChallengeMenu::ButtonMouseLeave(int theId)
{
	if (mHover == theId)
		mHover = -1;
}

// 0x6e4ab6
void ChallengeMenu::Start(bool theNewGame)
{
	if (gApp->mHyperspace == nullptr)
		gApp->ShowHyperspace();
	gApp->NewGame(2, theNewGame, mCategory);
	if (theNewGame)
		gBoard->SetChallenge(gApp->mChallengeInfo->GetChallenge(mCategory, mStageSel));
	mStarted = 1;
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	mMouseVisible = false;
	// (the planet's place in the 3D world, at +0x5b4, aims the 3D camera: unused in 2D)
	gApp->mBoard->mHyperspace->StartChallenge();
	gApp->mBoard->SetUnknown3924(0.0f);
	gApp->mBoard->ApplyUnknown3924();
	Close(3);
}
