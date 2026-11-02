// Sexy::ChallengeMenu and Sexy::ChallengeButton (ChallengeMenu.h).
#include "ChallengeMenu.h"

#include "Board.h"
#include "Challenge.h"
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

using namespace Sexy;

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
	int anAlpha = Ftol((double)mMenu->mAmount * 255.0);
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
}

// 0x6dd2cd
void ChallengeMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	TwistWidget::RemovedFromManager(theWidgetManager);
	for (ChallengeButton* aButton : mButtons)
		RemoveWidget(aButton);
	RemoveWidget(mPlayMenuButton);
	RemoveWidget(mMainMenuButton);
}

// 0x6e30d5
void ChallengeMenu::Show(int theMode, bool theNow)
{
	mUnknown9BC = false;
	// (the original clears an EffectsManager of the menu's here)
	mVisible = true;
	UpdateProgress(false, false);
	SetState(theMode, theNow);
	if (!theNow) {
		for (ChallengeButton* aButton : mButtons)
			aButton->mMouseVisible = false;
	}
}

// 0x6e3142
void ChallengeMenu::Close(int theState)
{
	mMouseVisible = false;
	mMainMenuButton->mMouseVisible = false;
	mPlayMenuButton->mMouseVisible = false;
	for (ChallengeButton* aButton : mButtons)
		aButton->mMouseVisible = false;
	if (theState == 2)
		gApp->mMainMenu->PlayMusic(0);
	SetState(theState, false);
}

// 0x6e2318: PLACEHOLDER. The original sets the camera's, the sun's, the moon's and the planets' curves for each state
// (the CurvedVals at +0x1c8 to +0x830 and the sun's at 0x85c558); the port only keeps which state it is in and the music
void ChallengeMenu::SetState(int theState, bool theNow)
{
	// The original's switch (0x6e232e): 0 away; 1, 4 and 5 show it, 4 and 1 with the menu's music (0x6e237f, 0x6e2b91 are the
	// only PlayMusic calls; 2, 3 and 5 play none); 2 and 3 are the ways out. 5 only shows the menu (0x6e2361)
	mState = theState;
	if (theState == 5)
		mVisible = true;
	if (theState == 1 || theState == 4)
		gApp->mMainMenu->PlayMusic(0x130);
	if (theNow)
		mAmount = (theState == 1 || theState == 4) ? 1.0f : 0.0f;
	if (theState == 4)
		mAmount = 1.0f;
	if (theState == 0)
		mAmount = 0.0f;
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

// 0x6e3f41 (the part that does not move the 3D camera)
void ChallengeMenu::Update()
{
	TwistWidget::Update();
	if (mHover == -1) {
		mHoverFade -= 0.04f;
		if (mHoverFade < 0.0f)
			mHoverFade = 0.0f;
	} else {
		mHoverFade += 0.025f;
		if (mHoverFade > 1.0f)
			mHoverFade = 1.0f;
	}
	// port: the menu comes in and goes out by a plain fade (the original's curves at +0x1c8 and +0x238 run the camera
	// and the planets in and out)
	if (mState == 1 || mState == 4) {
		mAmount += 0.04f;
		if (mAmount > 1.0f)
			mAmount = 1.0f;
	} else {
		mAmount -= 0.04f;
		if (mAmount <= 0.0f) {
			mAmount = 0.0f;
			if (mState != 0)
				mVisible = false;
		}
	}
	bool anOpen = mAmount >= 1.0f && (mState == 1 || mState == 4);
	mMouseVisible = anOpen;
	mPlayMenuButton->mMouseVisible = anOpen;
	mMainMenuButton->mMouseVisible = anOpen;
	for (ChallengeButton* aButton : mButtons)
		aButton->mMouseVisible = anOpen && aButton->mOpen;
	MarkDirty();
}

// 0x6dd322 (the sun, the camera's world and the planets' tips are 3D; here the picked planet's text)
void ChallengeMenu::Draw(Graphics* g)
{
	TwistWidget::Draw(g);
	if (mHover < 0 || mHover >= (int)mButtons.size() || mHoverFade <= 0.0f || mAmount <= 0.0f)
		return;
	ChallengeButton* aButton = mButtons[mHover];
	int aNumber = aButton->mStage + (aButton->mEclipse ? 1 : 0);
	Challenge* aChallenge = gApp->mChallengeInfo->GetChallenge(aButton->mCategory, aNumber);
	if (aChallenge == nullptr)
		return;
	float aFraction = 0;
	SexyString aText = gApp->mChallengeInfo->GetText(aChallenge, false, false, &aFraction);
	int anAlpha = Ftol((double)mHoverFade * (double)mAmount * 255.0);
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
