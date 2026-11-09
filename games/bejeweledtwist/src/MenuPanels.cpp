// The main menu's panels (MenuPanels.h). port: the high scores, help, profile and new player panels are ported; of
// the others only what the menu needs of them.
#include "MenuPanels.h"

#include "Board.h"
#include "ColorCycle.h"
#include "Dialog.h"
#include "EffectsManager.h"
#include "EditListener.h"
#include "Font.h"
#include "GameText.h"
#include "GameOverShow.h"
#include "Gates.h"
#include "Graphics.h"
#include "Image.h"
#include "ListWidget.h"
#include "Menu.h"
#include "Piece.h"
#include "MenuButtonWidget.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistCheckbox.h"
#include "TwistD3D.h"
#include "TwistDialog.h"
#include "TwistEditWidget.h"
#include "TwistSlider.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <cwctype>
#include <set>
#include <string>

using namespace Sexy;

namespace {

// A button of a panel at its place in resources.xml, against the panel's picture
void PlaceButton(ButtonWidget* theButton, int theDialogId, float theScale)
{
	int anId = GetIdByImage(theButton->mButtonImage);
	int aHeight = theButton->mButtonImage->GetCelHeight();
	int aWidth = theButton->mButtonImage->GetCelWidth();
	int aY = Ftol((double)(GetImageOffsetY(anId) - GetImageOffsetY(theDialogId)) * theScale);
	int aX = Ftol((double)(GetImageOffsetX(anId) - GetImageOffsetX(theDialogId)) * theScale);
	theButton->Resize(aX, aY, aWidth, aHeight);
}

}  // namespace

// 0x694387: theBase to the power theExponent, by repeated squaring in single precision
static float IntPow(float theBase, int theExponent)
{
	unsigned int anExp = theExponent < 0 ? -theExponent : theExponent;
	float aResult = 1.0f;
	while (true) {
		if ((anExp & 1) != 0)
			aResult = aResult * theBase;
		anExp = anExp >> 1;
		if (anExp == 0)
			break;
		theBase = theBase * theBase;
	}
	if (theExponent < 0)
		aResult = 1.0f / aResult;
	return aResult;
}

MenuPanel::~MenuPanel() = default;

HighScoresPanel::HighScoresPanel(Menu* theMenu)
{
	mMenu = theMenu;
	mClip = false;
	mApp = theMenu->mApp;
	mImageId = IMAGE_MAINMENU_SCORESDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_MAINMENU_SCORESDIALOG_ID);
	mRevealTimer = 0;
	mVoiceDelay = 0;
	mState = 0;
	mMouseDown = false;
	Resize(0, GetImageOffsetY(IMAGE_MAINMENU_SCORESDIALOG_ID), mDialogImage->mWidth, mDialogImage->mHeight);
	mHoldTime = 5.0f;
	mIsPaused = false;
	mCategory = -99;
	mShown = -99;
	SetCategory(1, true);
	ResetNewRow();
	mFXManager = new EffectsManager();
	mStateTimer = 0;
	mReplaySlide = 0;
	mReplayWait = 0;
	mFade.SetConstant(1.0);
	mFading = false;
	// The two lists from the app's twenty high scores, ten each
	int anIndex = 0;
	std::vector<Row>* aList = mRows;
	do {
		int aCount = 10;
		do {
			Row aRow;
			aRow.mIsFilled = true;
			aRow.mRowAlpha = 1.0f;
			aRow.mRowShift = 0;
			const WinTwistApp::HighScore& aScore = mMenu->mApp->mHighScores[anIndex / 0x24];
			aRow.mName = aScore.mName;
			aRow.mLevel = aScore.mLevel;
			aRow.mScore = aScore.mScore;
			aList->push_back(aRow);
			anIndex += 0x24;
		} while (--aCount != 0);
		aList++;
	} while (anIndex < 0x2d0);
	mBar = new TwistWidget();
	mBar->mImage = IMAGE_MAINMENU_SCORESDIALOGBAR;
	mBar->mWidth = mBar->mImage->mWidth;
	mBar->mHeight = mBar->mImage->mHeight;
	mClip = false;
	mHide = false;
	mBarPressed = false;
	StartFade();
}

HighScoresPanel::~HighScoresPanel()
{
	RemoveWidget(mFXManager);
	delete mFXManager;
	mBar->RemoveAllWidgets(true, true);
	RemoveWidget(mBar);
	delete mBar;
	// port: the original leaves the view-replay buttons it never added (a replay the player does not have) undeleted;
	// the port deletes every one, as MakeReplayButtons does
	for (TwistButton* aButton : mReplayButtons) {
		RemoveWidget(aButton);
		delete aButton;
	}
	mReplayButtons.clear();
	RemoveAllWidgets(true, false);
}

void HighScoresPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	MenuPanel::AddedToManager(theWidgetManager);
	mVoiceDelay = 0;
	MakeReplayButtons();
	AddWidget(mFXManager);
}

void HighScoresPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveWidget(mFXManager);
	RemoveWidget(mBar);
}

void HighScoresPanel::Update()
{
	MenuPanel::Update();
	// -autoplay: the next game starts a while after this one's scores show
	if (mApp->mAutoPlayMode > -1 && 0.0f < mApp->mAutoPlayDelay) {
		mApp->mAutoPlayDelay = (float)((double)mApp->mAutoPlayDelay - 0.01);
		if (mApp->mAutoPlayDelay <= 0.0f)
			ButtonDepress(BUTTON_REPLAY);
	}
	if (mFading && !mFade.IncInVal()) {
		mRevealed = -1;
		mFlash = 0;
		mFade.SetConstant(1.0);
		mFading = false;
		ShowCategory();
	}
	mRevealTimer = (float)((double)mRevealTimer - 0.01);
	if (mRevealTimer <= 0.0f) {
		mRevealed++;
		mFlash = 1.0f;
		mRevealTimer = 0.025f;
	}
	float aFlash = (float)((double)mFlash - 0.04);
	if (aFlash < 0.0f)
		aFlash = 0.0f;
	mFlash = aFlash;
	float anAlpha;
	if (!mHide) {
		anAlpha = (float)((double)mAlpha + 0.02);
		if (1.0f < anAlpha)
			anAlpha = 1.0f;
	} else {
		anAlpha = (float)((double)mAlpha - 0.02);
		if (anAlpha < 0.0f)
			anAlpha = 0.0f;
	}
	mAlpha = anAlpha;

	if (mState == 0) {
		if (0.0f < mVoiceDelay) {
			mVoiceDelay = (float)((double)mVoiceDelay - 0.01);
			if (mVoiceDelay <= 0.0f)
				mApp->PlaySample(SOUND_VOICE_ANEWHIGHSCORE);
		}
		if (mBarSlide.mIncRate != 0.0) {
			mBarSlide.IncInVal();
			double anOut = mBarSlide.GetOutVal();
			int aDrop = Ftol((double)gApp->mScale * 400.0);
			mBar->mY = Ftol(anOut * (double)aDrop + (double)mHeight * 0.800000011920929);
		}
	} else if (mState == 1) {
		mStateTimer = (float)((double)mStateTimer - 0.01);
		if (mStateTimer <= 0.0f) {
			mStateTimer = 1.0f;
			mState = 2;
		}
	} else if (mState == 2) {
		mState = 3;
		mStateTimer = 0;
	} else if (mState == 3) {
		// The new score moves up its list one row at a time, each with a gem's sound, and the tenth row's name and
		// score fall off the list
		float aTimer;
		if (0.0f < mStateTimer) {
			aTimer = (float)((double)mStateTimer - 0.01);
		} else {
			mRowSlide.SetCurve(GTS(0x7d8828));
			int aRowsLeft = mNewRow < 1 ? 1 : mNewRow;
			float aRate = IntPow(0.6499999761581421f, aRowsLeft);
			if (aRate <= 0.07999999821186066)
				aRate = IntPow(0.6499999761581421f, aRowsLeft);
			else
				aRate = 0.07999999821186066f;
			mRowSlide.mIncRate = (double)aRate;
			aTimer = 100.0f;
		}
		mStateTimer = aTimer;
		int aList = WinTwistApp::ScoreList(mNewCategory);
		if (mRowSlide.mIncRate != 0.0 && !mRowSlide.IncInVal()) {
			mApp->PlaySoundEx(SOUND_GEM_COUNTDOWN_DESTROYED, 0, 3.0f, 0.0f, true, true);
			std::vector<Row>& aRows = mRows[aList];
			if (mNewRow == 10) {
				// The tenth row's name and score drop off the list, a piece of text for each character (the level
				// is formatted but not shown)
				SexyString aName = aRows[9].mName;
				SexyString aScoreText = StrFormat(GTC(0x80da2c), aRows[9].mScore);
				int aRowY = GetRowY(9);
				aRows[9] = aRows[mNewRow];
				aRows.pop_back();
				Font* aDropFont = FONT_HIGH_SCORE;
				int aX = GetNameX();
				int aChar = 0;
				if (aName.length() != 0) {
					float aY = (float)aRowY;
					do {
						SexyChar aCh = aName[aChar];
						Image* anImage = new TextImage(aDropFont, aName.substr(aChar, 1), 0, 0, -1, 0, nullptr);
						Effect* anEffect = mFXManager->AllocEffect(0);
						anEffect->mImage = anImage;
						anEffect->mX = (float)aX;
						anEffect->mY = aY;
						anEffect->mDX = (float)(((double)(float)aX - (double)(mWidth / 2)) * 0.004999999888241291);
						float aRand = fabsf(RandSigned());
						anEffect->mOwnsImage = true;
						float aSpeed = (float)((double)gApp->mScale * -5.0);
						anEffect->mDY = aSpeed * aRand;
						anEffect->mGravity = 0.07500000298023224f;
						anEffect->mAlpha = 1.0f;
						anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
						anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.009999999776482582);
						double aDAlpha = (double)fabsf(RandSigned()) * -0.004999999888241291;
						if (aDAlpha <= -0.0005000000237487257)
							aDAlpha = (double)fabsf(RandSigned()) * -0.004999999888241291;
						anEffect->mDAlpha = (float)aDAlpha;
						anEffect->mColor = mLastRowColor;
						mFXManager->AddEffect(anEffect);
						aX += aDropFont->CharWidth(aCh) + Ftol((double)(gApp->mScale + gApp->mScale));
						aChar++;
					} while (aChar < (int)aName.length());
				}
				aX = GetScoreRight() - aDropFont->StringWidth(aScoreText);
				aChar = 0;
				if (aScoreText.length() != 0) {
					float aY = (float)aRowY;
					do {
						SexyChar aCh = aScoreText[aChar];
						Image* anImage = new TextImage(aDropFont, aScoreText.substr(aChar, 1), 0, 0, -1, 0, nullptr);
						Effect* anEffect = mFXManager->AllocEffect(0);
						anEffect->mImage = anImage;
						anEffect->mX = (float)aX;
						anEffect->mY = aY;
						anEffect->mDX = (float)(((double)(float)aX - (double)(mWidth / 2)) * 0.004999999888241291);
						float aRand = fabsf(RandSigned());
						anEffect->mOwnsImage = true;
						float aSpeed = (float)((double)gApp->mScale * -5.0);
						anEffect->mDY = aSpeed * aRand;
						anEffect->mGravity = 0.07500000298023224f;
						anEffect->mAlpha = 1.0f;
						anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
						anEffect->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.009999999776482582);
						double aDAlpha = (double)fabsf(RandSigned()) * -0.004999999888241291;
						if (aDAlpha <= -0.0005000000237487257)
							aDAlpha = (double)fabsf(RandSigned()) * -0.004999999888241291;
						anEffect->mDAlpha = (float)aDAlpha;
						anEffect->mColor = Color(255, 128, 0);
						mFXManager->AddEffect(anEffect);
						aX += aDropFont->CharWidth(aCh) + Ftol((double)(gApp->mScale + gApp->mScale));
						aChar++;
					} while (aChar < (int)aScoreText.length());
				}
			} else {
				Row aMovedRow = aRows[mNewRow];
				aRows[mNewRow] = aRows[mNewRow - 1];
				aRows[mNewRow - 1] = aMovedRow;
			}
			mNewRow--;
			aRows[mNewRow].mRowAlpha = 1.0f;
			if (mNewRow == mNewPlace) {
				Gates* aGates = mApp->mGates;
				aGates->mOptionsButton->SetGlowOn(true);
				aGates->mScoresButton->SetGlowOn(true);
				aGates->mHelpButton->SetGlowOn(true);
				aGates->mQuitButton->SetGlowOn(true);
				mState = 0;
				mVoiceDelay = 1.0f;
			} else {
				float aStep = (float)((double)mNewRow * 0.02500000037252903);
				mStateTimer = (float)((double)aStep * (double)aStep);
				mRowSlide.mIncRate = 0;
				mRowSlide.mInVal = 0;
			}
		}
	}

	// A board under the scores (a replay) slides up out of the screen with the panel
	WinTwistApp* anApp = mApp;
	Board* aBoard = anApp->mBoard;
	if (aBoard != nullptr && aBoard->mIsReplay) {
		if (mMenu->mPanel != 2)
			mReplayWait = 0;
		aBoard->mBoardOffsetX = (float)Ftol((double)gApp->mScale * 0.0);
		int aOffsetY = Ftol((double)gApp->mScale * 75.0);
		anApp->mBoard->mBoardOffsetY = (float)((double)aOffsetY + (double)mMenu->mY * 2.5);
		aBoard = mApp->mBoard;
		if ((aBoard->mReplayWaiting && 0 < aBoard->mRecordedFrames) || mReplayWait != 2.0f) {
			mReplayWait = (float)((double)mReplayWait - 0.01);
			if (mReplayWait <= 0.0f) {
				int aButton = 0;
				while (aButton < (int)mReplayButtons.size()) {
					mReplayButtons[aButton]->SetDisabled(false);
					aButton++;
				}
				if (mReplaySlide == 1.0f && mMenu->mPanel == 2)
					mMenu->SetPan(0, -gApp->mHeight, false);
				mReplaySlide = (float)((double)mReplaySlide - 0.02);
				if (mMenu->mY == -gApp->mHeight) {
					mApp->mWidgetManager->RemoveWidget(mApp->mBoard);
					delete mApp->mBoard;
					mApp->mBoard = nullptr;
					mApp->mWidgetManager->SetFocus(this);
					mApp->mGates->SetButtonsOn(true);
				}
			}
		}
	}
}

void HighScoresPanel::StartNewScore(int theCategory, int thePlace, const SexyString& theName, int theLevel,
	int theScore)
{
	SetCategory(theCategory, false);
	WinTwistApp::HighScore aScore;
	aScore.mName = theName;
	aScore.mLevel = theLevel;
	aScore.mScore = theScore;
	int aList = WinTwistApp::ScoreList(theCategory);
	Row aRow;
	aRow.mIsFilled = true;
	aRow.mRowAlpha = 0;
	aRow.mRowShift = 0;
	aRow.mName = aScore.mName;
	aRow.mLevel = aScore.mLevel;
	aRow.mScore = aScore.mScore;
	mRows[aList].push_back(aRow);
	mStateTimer = 1.0f;
	mState = 3;
	mNewPlace = thePlace;
	mNewRow = (int)mRows[aList].size() - 1;
	mNewCategory = theCategory;
	Gates* aGates = mApp->mGates;
	aGates->mButtonsOn = false;
	aGates->mOptionsButton->SetGlowOn(false);
	aGates->mScoresButton->SetGlowOn(false);
	aGates->mHelpButton->SetGlowOn(false);
	aGates->mQuitButton->SetGlowOn(false);
}

void HighScoresPanel::Draw(Graphics* g)
{
	g->DrawImage(IMAGE_MAINMENU_REPLAYDIALOG, mWidth / 2 - IMAGE_MAINMENU_REPLAYDIALOG->mWidth / 2,
		Ftol((double)gApp->mScale * -1120.0));
	if (mAlpha == 0.0f)
		return;

	Image* aDialogImage = mDialogImage;
	g->SetColorizeImages(true);
	double aDialogAlpha = (double)mAlpha * 1.5;
	if (1.0 < aDialogAlpha)
		aDialogAlpha = 1.0;
	g->SetColor(Color(255, 255, 255, Ftol(aDialogAlpha * 255.0)));
	g->DrawImage(aDialogImage, 0, 0);
	double aFade = mFade.GetOutVal();
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0 * aFade)));
	int aTitleId;
	if (mShown == 1)
		aTitleId = IMAGE_MAINMENU_SCORES_TITLE_CLASSIC_ID;
	else if (mShown == 3)
		aTitleId = IMAGE_MAINMENU_SCORES_TITLE_BLITZ_ID;
	else
		aTitleId = IMAGE_MAINMENU_SCORES_TITLE_STATS_ID;
	DrawImageScaledCentered(g, GetImageById(aTitleId), (float)((double)mWidth * 0.5),
		(float)((double)mHeight * 0.20999999344348907), ALIGN_CENTER, 1.0f, 1.0f);

	Font* aFont = FONT_HIGH_SCORE;
	float aNameX = (float)GetNameX();
	ColorCycle aCycle;
	float aHue = (float)((double)(mUpdateCnt % 400) / 400.0);
	aCycle.mBrighten = 0.5f;

	if (mShown != 3 && mShown != 1) {
		// The stats page: the player's rank, points and best moves, a line each
		Profile* aProfile = mApp->mProfile;
		float aHeight = (float)mHeight;
		float aY = (float)((double)aHeight * 0.36000001430511475);
		float aLineHeight = (float)((double)aHeight * 0.05000000074505806);
		g->SetFont(aFont);
		std::vector<SexyString> aLines;
		aLines.push_back(StrFormat(GTC(0x7d8a10), aProfile->mRank + 1, mApp->mRanks->GetName(aProfile->mRank).c_str()));
		SexyString aPoints = CommaSeperate(aProfile->mTotalPoints) + GT(0x80e8a4) +
			mApp->GetString(GTS(0x7d8a0c), GT(0x7d89f0));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d89f8)).c_str(), aPoints.c_str()));
		SexyString aBestMove = CommaSeperate(aProfile->mSavedGames[2].mBest);
		if (aProfile->mSavedGames[2].mBest < 1000000)
			aBestMove += GT(0x80e8a4) + mApp->GetString(GTS(0x7d8a0c), GT(0x7d89f0));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d89e0), GT(0x7d89c4)).c_str(), aBestMove.c_str()));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d89b0), GT(0x7d8988)).c_str(),
			CommaSeperate(aProfile->mSavedGames[0].mBest).c_str()));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d8974), GT(0x7d8950)).c_str(),
			CommaSeperate(aProfile->mSavedGames[1].mBest).c_str()));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d8940), GT(0x7d8920)).c_str(),
			CommaSeperate(aProfile->mBestChain).c_str()));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d8908), GT(0x7d88d8)).c_str(),
			CommaSeperate(aProfile->mGamesPlayed).c_str()));
		aLines.push_back(StrFormat(mApp->GetString(GTS(0x7d88c0), GT(0x7d8890)).c_str(),
			CommaSeperate(aProfile->mGemsCleared).c_str()));
		int aLine = 0;
		if (0 < (int)aLines.size()) {
			do {
				if (mRevealed < aLine)
					break;
				aCycle.SetPosition(aHue);
				aHue = (float)((double)aHue - 0.05000000074505806);
				if (aHue < 0.0f)
					aHue = (float)((double)aHue + 1.0);
				Color aColor = aCycle.mColor;
				aColor.mAlpha = Ftol(mFade.GetOutVal() * 255.0);
				Font* aPassFont = aFont;
				int aPass = 0;
				do {
					g->SetFont(aPassFont);
					if (aPass == 0) {
						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
					} else {
						// The glow over the line that just showed (over every line while a category fades)
						if (aLine != mRevealed && !mFading)
							break;
						float aGlow = (float)(mFade.GetOutVal() * (double)mFlash);
						if (aGlow == 0.0f)
							break;
						float aGlowAlpha = mAlpha * aGlow;
						aColor.mAlpha = Ftol((double)aGlowAlpha * 255.0);
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					}
					g->SetColor(aColor);
					g->DrawString(aLines[aLine], Ftol((double)mWidth * 0.14000000059604645), Ftol((double)aY));
					aPass++;
					aPassFont = FONT_HIGH_SCORE_GLOW;
				} while (aPass < 2);
				aLine++;
				aY = aLineHeight + aY;
			} while (aLine < (int)aLines.size());
		}
		return;
	}

	// A top ten: the column labels, then each row's place, name, level (classic only) and score
	std::vector<Row>& aRows = mRows[WinTwistApp::ScoreList(mShown)];
	bool aNoLevel = mShown == 3;
	float aLabelY = (float)Ftol((double)mHeight * 0.26499998569488525);
	DrawImageScaledCentered(g, IMAGE_MAINMENU_SCORES_LABEL_PLAYER, (float)((double)mWidth * 0.2680000066757202),
		aLabelY, ALIGN_CENTER, 1.0f, 1.0f);
	if (!aNoLevel)
		DrawImageScaledCentered(g, IMAGE_MAINMENU_SCORES_LABEL_LEVEL, (float)((double)mWidth * 0.5600000023841858),
			aLabelY, ALIGN_CENTER, 1.0f, 1.0f);
	DrawImageScaledCentered(g, IMAGE_MAINMENU_SCORES_LABEL_SCORE, (float)((double)mWidth * 0.7699999809265137),
		aLabelY, ALIGN_CENTER, 1.0f, 1.0f);
	int aRowNum = 0;
	if (0 < (int)aRows.size()) {
		do {
			Row& aRow = aRows[aRowNum];
			aCycle.SetPosition(aHue);
			aHue = (float)((double)aHue - 0.05000000074505806);
			if (aHue < 0.0f)
				aHue = (float)((double)aHue + 1.0);
			if (aRowNum <= mRevealed) {
				float aY = (float)GetRowY(aRowNum);
				Color aNameColor;
				Color aScoreColor;
				Color aLevelColor;
				if (mState == 3) {
					// The rows below the new score move down to make room for it
					if (aRowNum == mNewRow) {
						float aShift = GetRowHeight();
						if (aRowNum == 10) {
							aShift = (float)((double)mApp->mHeight * 0.3);
							aY = (float)((double)GetRowY(9) + (double)aShift);
						}
						double aFrom = (double)aY;
						aY = (float)(aFrom - mRowSlide.GetOutVal() * (double)aShift);
					} else if (aRowNum == mNewRow - 1 && aRowNum != 9) {
						double anOut = mRowSlide.GetOutVal();
						aY = (float)((double)GetRowHeight() * anOut + (double)aY);
					}
				}
				if (mApp->mLastGameMode == mShown && aRowNum == mNewRow) {
					// The new score: white, or running through the rainbow for a new best
					Color aColor = Color::White;
					if (mNewRow == 0) {
						ColorCycle aBestCycle;
						aBestCycle.SetPosition((float)((double)(mUpdateCnt % 200) / 200.0));
						aColor = aBestCycle.mColor;
					}
					aLevelColor = aColor;
					aScoreColor = aColor;
					aNameColor = aColor;
				} else {
					aNameColor = aCycle.mColor;
					if (aRowNum == 9)
						mLastRowColor = aCycle.mColor;
					aLevelColor = Color(255, 128, 0);
					aScoreColor = Color(255, 220, 0);
				}
				g->SetFont(FONT_DIALOG_BUTTONS);
				SexyString aPlace = StrFormat(GTC(0x80da2c), aRowNum + 1);
				g->SetColor(Color(200, 32, 200, Ftol((double)mAlpha * 255.0 * mFade.GetOutVal())));
				int aPlaceX = Ftol((double)gApp->mScale * 178.0);
				int aPlaceY = Ftol((double)Ftol((double)gApp->mScale) + (double)aY);
				g->DrawString(aPlace, aPlaceX - g->GetFont()->StringWidth(aPlace) / 2, aPlaceY);
				aNameColor.mAlpha = Ftol((double)mAlpha * 255.0 * mFade.GetOutVal());
				aScoreColor.mAlpha = Ftol((double)mAlpha * 255.0 * mFade.GetOutVal());
				aLevelColor.mAlpha = Ftol((double)mAlpha * 255.0 * mFade.GetOutVal());
				Font* aPassFont = aFont;
				int aPass = 0;
				do {
					g->SetFont(aPassFont);
					if (aPass == 0) {
						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
					} else {
						// The glow: pulsing on the new score, else over the row that just showed (over every row
						// while a category fades)
						float aGlow;
						if (mApp->mLastGameMode == mShown && aRowNum == mNewRow) {
							float anAngle = (float)((double)(mUpdateCnt % 200) / 200.0 * 3.1415927410125732);
							aGlow = (float)((double)sinf(anAngle) * 0.5);
						} else {
							if (aRowNum != mRevealed && !mFading)
								break;
							aGlow = (float)(mFade.GetOutVal() * (double)mFlash);
						}
						if (aGlow == 0.0f)
							break;
						float aGlowAlpha = mAlpha * aGlow;
						int aGlowAlpha255 = Ftol((double)aGlowAlpha * 255.0);
						aLevelColor.mAlpha = aGlowAlpha255;
						aScoreColor.mAlpha = aGlowAlpha255;
						aNameColor.mAlpha = aGlowAlpha255;
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					}
					g->SetColor(aNameColor);
					SexyString aName = aRow.mName;
					int aMaxWidth = Ftol((double)gApp->mScale * 250.0);
					while (aMaxWidth < g->GetFont()->StringWidth(aName))
						aName = aName.substr(0, aName.length() - 1);
					int aRowY = Ftol((double)aY);
					g->DrawString(aName, Ftol((double)aNameX), aRowY);
					g->SetColor(aLevelColor);
					if (!aNoLevel) {
						SexyString aLevel = StrFormat(GTC(0x80da2c), aRow.mLevel);
						int aWidth = g->GetFont()->StringWidth(aLevel);
						g->DrawString(aLevel, GetLevelRight() - aWidth, aRowY);
					}
					g->SetColor(aScoreColor);
					// The score right-aligned a character at a time, each centred in a cell (narrower for commas)
					SexyString aScore = CommaSeperate(aRow.mScore);
					float aRight = (float)GetScoreRight();
					SexyChar aPrev = ' ';
					int i = (int)aScore.length() - 1;
					if (i >= 0) {
						do {
							SexyString aChar = aScore.substr(i, 1);
							float aCell;
							if (aChar[0] == ',')
								aCell = (float)((double)gApp->mScale * 12.0);
							else
								aCell = (float)((double)gApp->mScale * 22.0);
							double aCentre = (double)aRight - (double)aCell * 0.5;
							float aX = (float)(aCentre - (double)(g->GetFont()->StringWidth(aChar) / 2));
							if (aChar[0] == '1' && aPrev == ',' && i == 0)
								aX = (float)((double)Ftol((double)gApp->mScale * 4.0) + (double)aX);
							g->DrawString(aChar, Ftol((double)aX), aRowY);
							aRight = aRight - aCell;
							aPrev = aChar[0];
							i--;
						} while (i >= 0);
					}
					aPass++;
					aPassFont = FONT_HIGH_SCORE_GLOW;
				} while (aPass < 2);
			}
			aRowNum++;
		} while (aRowNum < (int)aRows.size());
	}
}

void HighScoresPanel::MouseDown(int, int, int, int)
{
	mMouseDown = true;
}

void HighScoresPanel::MouseUp(int, int, int)
{
	mMouseDown = false;
}

void HighScoresPanel::ButtonPress(int theId, int)
{
	if (theId == BUTTON_QUIT || theId == BUTTON_REPLAY)
		mMenu->mApp->PlaySample(SOUND_MENU_BUTTON_CLICK);
	else if (theId >= BUTTON_VIEW_REPLAY)
		mApp->PlaySample(SOUND_CLICK);
}

void HighScoresPanel::ButtonDepress(int theId)
{
	if (!mMenu->mPan.HasBeenTriggered())
		return;
	mVoiceDelay = 0;
	if (theId == BUTTON_QUIT) {
		if (!mBarPressed) {
			mBarPressed = true;
			mMenu->mApp->PlaySample(SOUND_MENU_SLIDE);
			mMenu->ShowMain(false);
		}
	} else if (theId == BUTTON_REPLAY) {
		if (!mBarPressed) {
			mBarPressed = true;
			mMenu->StartGame(mApp->mLastGameMode);
			HideBar();
			mApp->mGates->SetOpen(true, true);
			mHide = true;
		}
	} else if (theId >= BUTTON_VIEW_REPLAY) {
		// A best move's replay, on a classic board of its own under the scores (+0x37c4), which Update takes away
		// once it is over
		int aReplay = GameInt(GTB(0x84e550, 12) + (theId - BUTTON_VIEW_REPLAY) * 4);
		Profile::SavedGame& aGame = mApp->mProfile->mSavedGames[aReplay];
		if (!aGame.mData.mData.empty()) {
			int aButton = 0;
			while (aButton < (int)mReplayButtons.size()) {
				mReplayButtons[aButton]->SetDisabled(true);
				aButton++;
			}
			if (mApp->Is3DAccelerated())
				mMenu->SetPan(0, 0, false);  // 0x69af49: with Direct3D the menu pans to the board
			// port: the exe also sets a flag of its sound manager
			// (+0x300a0) that puts every sound started from now on through DirectSound's parametric equaliser,
			// until the board is removed; the port's sound manager has none. It also makes a Buffer it never uses.
			mApp->mScoresLoaded = false;
			mApp->mGates->SetButtonsOn(false);
			Board* aBoard = new Board(mApp, Board::MODE_CLASSIC, 8, 0);
			aBoard->Resize(0, 0, mApp->mWidth, mApp->mHeight);
			mApp->mBoard = aBoard;
			aBoard->mIsReplay = true;
			mApp->mWidgetManager->AddWidget(aBoard);
			aBoard->ReadConfig(true);
			aBoard->mAlertGlow = 1.0f;
			aBoard->Update();
			aBoard->ShowTube(true, mApp->Is3DAccelerated());
			aBoard->mGameLoaded = false;
			aBoard->SetScore(aBoard->mScore, false);
			aBoard->mBoardZoom = 0.4f;
			aBoard->mStartState = 1;
			aGame.mData.SeekFront();
			aBoard->mShakeY = 0;
			aBoard->mShakeX = 0;
			aBoard->StartReplay(aGame.mData, aGame.mReplayStart, aGame.mReplayEnd, false);
			if (gD3DInterface == nullptr) {
				aBoard->mBoardX = mApp->mWidth;
				aBoard->mReplaySlideWait = 100;
			} else {
				// with Direct3D the gems and the effects run 25 updates ahead instead
				for (int anUpdate = 25; anUpdate != 0; anUpdate--) {
					for (int aCell = 0; aCell < aBoard->mCols * aBoard->mRows; aCell++) {
						if (aBoard->mGrid[aCell] != nullptr)
							aBoard->mGrid[aCell]->Update();
					}
					aBoard->mFXManager->Update();
					aBoard->mFXManager2->Update();
				}
			}
			aBoard->mClearedOnHyperspaceStart = 0;
			aBoard->mModeFlags = 8;
			aBoard->mTubeOffsetX = 0;
			aBoard->mBarWidth = aBoard->GetGridRight();
			aBoard->mAlertGlow = 1.0f;
			aBoard->SetVisible(true);
			mReplayWait = 2.0f;
			mReplaySlide = 1.0f;
			aBoard->LayoutTube();
		}
	}
}

void HighScoresPanel::SetCategory(int theCategory, bool theNow)
{
	if (mShown != theCategory || theNow || (mCategory != -1 && mCategory != theCategory)) {
		mReplayWait = 0;
		mCategory = theCategory;
		StartFade();
		if (!theNow) {
			mApp->PlaySample(SOUND_SCREENPRINT);
		} else {
			ShowCategory();
			ShowAll();
		}
	}
}

void HighScoresPanel::Refresh()
{
	SetCategory(1, false);
	ResetNewRow();
}

void HighScoresPanel::ShowBar()
{
	mBarPressed = false;
	mBarSlide.SetCurve(GTS(0x7d8610));
	if (mBar->mParent == nullptr) {
		// port: as in the original, the bar gets a new pair of buttons each time it comes back after being removed
		// with the panel; the old pair stays under the new one, in the same place
		AddWidget(mBar);
		TwistButton* aReplay = new TwistButton(BUTTON_REPLAY, IMAGE_MAINMENU_REPLAYBUTTON, this);
		aReplay->mX = (mBar->mWidth / 2 - Ftol((double)gApp->mScale * 5.0)) - aReplay->mWidth;
		aReplay->mY = mBar->mHeight / 2 - aReplay->mHeight / 2;
		mBar->AddWidget(aReplay);
		TwistButton* aQuit = new TwistButton(BUTTON_QUIT, IMAGE_MAINMENU_OPTIONSQUITBUTTON, this);
		aQuit->mX = mBar->mWidth / 2 - Ftol((double)gApp->mScale * -5.0);
		aQuit->mY = mBar->mHeight / 2 - aQuit->mHeight / 2;
		mBar->AddWidget(aQuit);
	}
	mBar->mX = mWidth / 2 - mBar->mWidth / 2;
	mBar->mY = mApp->mHeight;
	mBar->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	mMouseVisible = true;
	if (mApp->mAutoPlayMode > -1)
		mApp->mAutoPlayDelay = 7.0f;
}

void HighScoresPanel::HideBar()
{
	mBarSlide.SetCurve(GTS(0x7d8630));
	mBar->mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	mMouseVisible = false;
}

void HighScoresPanel::ShowAll()
{
	mFlash = 0;
	mRevealed = 1000;
	mFade.SetConstant(1.0);
	mFading = false;
	ShowCategory();
}

void HighScoresPanel::StartFade()
{
	mFade.SetCurve(GTS(0x7d85e0));
	mFading = true;
}

void HighScoresPanel::ShowCategory()
{
	mShown = mCategory;
	MakeReplayButtons();
}

void HighScoresPanel::MakeReplayButtons()
{
	int i = 0;
	if (0 < (int)mReplayButtons.size()) {
		do {
			RemoveWidget(mReplayButtons[i]);
			if (mReplayButtons[i] != nullptr)
				delete mReplayButtons[i];
			i++;
		} while (i < (int)mReplayButtons.size());
	}
	mReplayButtons.clear();
	if (WinTwistApp::ScoreList(mShown) != -1)
		return;
	// The stats page: a button beside each of the three best moves, shown when its replay was kept
	int aButton = 0;
	do {
		TwistButton* aView = new TwistButton(aButton + BUTTON_VIEW_REPLAY, IMAGE_FINALSTATSUI_VIEWREPLAYBTN, this);
		aView->mWidth = IMAGE_FINALSTATSUI_VIEWREPLAYBTN->GetCelWidth();
		aView->mHeight = IMAGE_FINALSTATSUI_VIEWREPLAYBTN->GetCelHeight();
		int aMargin = Ftol((double)gApp->mScale * 115.0);
		aView->mX = (mDialogImage->mWidth - aMargin) - IMAGE_FINALSTATSUI_VIEWREPLAYBTN->GetCelWidth();
		int aHeight = mHeight;
		int aStep = aHeight * aButton;
		aView->mY = Ftol((double)aHeight * 0.4269999861717224 + (double)aStep * 0.05000000074505806);
		int aReplay = GameInt(GTB(0x84e550, 12) + aButton * 4);
		if (!mApp->mProfile->mSavedGames[aReplay].mData.mData.empty())
			AddWidget(aView);
		mReplayButtons.push_back(aView);
		aButton++;
	} while (aButton < 3);
}

void HighScoresPanel::ResetNewRow()
{
	mNewRow = -1;
	mNewCategory = -1;
}

float HighScoresPanel::GetRowHeight()
{
	return (float)((double)mDialogImage->GetCelHeight() * 0.04625);
}

int HighScoresPanel::GetRowY(int theRow)
{
	int aTop = Ftol((double)gApp->mScale * 295.0);
	return Ftol((double)GetRowHeight() * (double)theRow + (double)aTop);
}

int HighScoresPanel::GetNameX()
{
	return Ftol((double)gApp->mScale * 215.0);
}

int HighScoresPanel::GetScoreRight()
{
	return mWidth - Ftol((double)gApp->mScale * 160.0);
}

int HighScoresPanel::GetLevelRight()
{
	return mWidth / 2 - Ftol((double)gApp->mScale * -80.0);
}

namespace {

// Each help page's title and text: a property id and the text when the properties have none (0x6984b0 looks them
// up once, the titles then the texts, and keeps them)
const unsigned int kHelpTitles[HelpPanel::PAGE_COUNT][2] = {
	{GTA(0x7d9b78), GTA(0x7d9b58)}, {GTA(0x7d9b48), GTA(0x7d9b2c)}, {GTA(0x7d9b20), GTA(0x7d9b08)},
	{GTA(0x7d9af8), GTA(0x7d9ad8)}, {GTA(0x7d9ad0), GTA(0x7d9ac4)}, {GTA(0x7d9ab8), GTA(0x7d9aa0)},
	{GTA(0x7d9a94), GTA(0x7d9a80)}, {GTA(0x7d9a70), GTA(0x7d9a50)}, {GTA(0x7d9a48), GTA(0x7d9a38)},
	{GTA(0x7d9a24), GTA(0x7d9a00)}, {GTA(0x7d99f8), GTA(0x7d99ec)}, {GTA(0x7d99e4), GTA(0x7d99d8)},
	{GTA(0x7d99d0), GTA(0x7d99c4)}, {GTA(0x7d99b8), GTA(0x7d99a4)}, {GTA(0x7d9998), GTA(0x7d9980)},
	{GTA(0x7d9970), GTA(0x7d9950)},
};
const unsigned int kHelpTexts[HelpPanel::PAGE_COUNT][2] = {
	{GTA(0x7d9908), GTA(0x7d9878)}, {GTA(0x7d983c), GTA(0x7d97c8)}, {GTA(0x7d978c), GTA(0x7d9720)},
	{GTA(0x7d96e8), GTA(0x7d9678)}, {GTA(0x7d9630), GTA(0x7d95a0)}, {GTA(0x7d955c), GTA(0x7d94e0)},
	{GTA(0x7d9498), GTA(0x7d9410)}, {GTA(0x7d93c8), GTA(0x7d9340)}, {GTA(0x7d92fc), GTA(0x7d9280)},
	{GTA(0x7d9238), GTA(0x7d91a8)}, {GTA(0x7d9160), GTA(0x7d90d0)}, {GTA(0x7d9090), GTA(0x7d9018)},
	{GTA(0x7d8fc8), GTA(0x7d8f30)}, {GTA(0x7d8ef0), GTA(0x7d8e70)}, {GTA(0x7d8e28), GTA(0x7d8d98)},
	{GTA(0x7d8d50), GTA(0x7d8cc8)},
};

// The label of a help page's animation in twistpam.pam
const std::string& HelpPageLabel(int thePage)
{
	return GTS(GameInt(GTP(0x84e5b0, 16) + thePage * 4));
}

// A button of the in-game help at its place in resources.xml
void PlaceHelpButton(TwistButton* theButton, int thePlaceId)
{
	int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(thePlaceId));
	int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(thePlaceId));
	theButton->Resize(aX, aY, theButton->mButtonImage->GetCelWidth(), theButton->mButtonImage->GetCelHeight());
}

}  // namespace

HelpPanel::HelpPanel(WinTwistApp* theApp)
{
	mApp = theApp;
	TwistButton* aButton = nullptr;
	mClip = false;
	mImageId = IMAGE_HOWTOPLAY_HELPDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_HOWTOPLAY_HELPDIALOG_ID);
	mPageStart = 0;
	mMouseDown = false;
	int aSize = Ftol((double)gApp->mScale * 1000.0);
	Resize(0, 0, aSize, aSize);
	mHoldTime = 5.0f;
	mIsPaused = false;
	mFXManager = new EffectsManager();
	mPageCount = 0;
	mClip = false;
	mIsAnimDone = false;
	mAnim = nullptr;
	MakeAnim();
	if (mApp->mBoard == nullptr) {
		AddWidget(mAnim);
		mPrevButton = nullptr;
		mNextButton = nullptr;
	} else {
		// Over a game: its own buttons, and it zooms in
		aButton = new TwistButton(BUTTON_BACK, IMAGE_HOWTOPLAY_HELPBACKBUTTON, this);
		PlaceHelpButton(aButton, IMAGE_HOWTOPLAY_HELPBACKBUTTON_ID);
		AddWidget(aButton);
		aButton = new TwistButton(BUTTON_PREV, IMAGE_HOWTOPLAY_PREVBUTTON, this);
		mPrevButton = aButton;
		PlaceHelpButton(aButton, IMAGE_HOWTOPLAY_PREVBUTTON_ID);
		aButton->SetDisabled(true);
		AddWidget(aButton);
		aButton = new TwistButton(BUTTON_NEXT, IMAGE_HOWTOPLAY_NEXTBUTTON, this);
		mNextButton = aButton;
		PlaceHelpButton(aButton, IMAGE_HOWTOPLAY_NEXTBUTTON_ID);
		AddWidget(aButton);
		mZooming = true;
		mZoom.SetCurve(GTS(0x7d9bc0));
	}
	SetPage(0);
}

HelpPanel::~HelpPanel()
{
	RemoveWidget(mFXManager);
	delete mFXManager;
	RemoveWidget(mAnim);
	delete mAnim;
	RemoveAllWidgets(true, false);
}

void HelpPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	MenuPanel::AddedToManager(theWidgetManager);
	AddWidget(mFXManager);
}

void HelpPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveWidget(mFXManager);
}

void HelpPanel::Update()
{
	bool wasZooming = mZooming;
	MenuPanel::Update();
	if (!mZooming && wasZooming) {
		if (mZoom.GetOutVal() == 0.0) {
			// zoomed out over the game: gone
			mWidgetManager->RemoveBaseModal(this);
			mParent->RemoveWidget(this);
			mApp->SafeDeleteWidget(this);
			mApp->mHelpPanel = nullptr;
		} else {
			AddWidget(mAnim);
		}
	}
	int aWidth = Ftol((double)gApp->mScale * 516.0);
	int aHeight = Ftol((double)gApp->mScale * 344.0);
	int aY = Ftol((double)gApp->mScale * 265.0);
	mAnim->Resize(mWidth / 2 - aWidth / 2, aY, aWidth, aHeight);
}

void HelpPanel::Draw(Graphics* g)
{
	if (mAlpha == 0.0f)
		return;
	static bool sTitlesRead = false;
	static SexyString sTitles[PAGE_COUNT];
	if (!sTitlesRead) {
		sTitlesRead = true;
		for (int i = 0; i < PAGE_COUNT; i++)
			sTitles[i] = mApp->GetString(GTS(kHelpTitles[i][0]), GT(kHelpTitles[i][1]));
	}
	static bool sTextsRead = false;
	static SexyString sTexts[PAGE_COUNT];
	if (!sTextsRead) {
		sTextsRead = true;
		for (int i = 0; i < PAGE_COUNT; i++)
			sTexts[i] = mApp->GetString(GTS(kHelpTexts[i][0]), GT(kHelpTexts[i][1]));
	}

	Image* aDialogImage = mDialogImage;
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0)));
	int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(mImageId));
	int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(mImageId));
	g->DrawImage(aDialogImage, aX, aY);
	g->SetFont(FONT_MENU_TINY);
	g->SetColor(Color(255, 255, 255));
	g->WriteString(sTitles[mPage], mWidth / 2, Ftol((double)gApp->mScale * 250.0), 0, 0, true, 0, -1, -1);

	// The text with wider lines when it takes two lines or fewer
	int aMargin = Ftol((double)gApp->mScale * 200.0);
	int aSpacing = Ftol((double)gApp->mScale * 45.0);
	int aTextHeight = g->GetWordWrappedHeight(mWidth - aMargin * 2, sTexts[mPage], aSpacing, nullptr);
	float aLineSpacing;
	if ((float)aTextHeight <= (float)((double)gApp->mScale * 100.0))
		aLineSpacing = (float)((double)gApp->mScale * 50.0);
	else
		aLineSpacing = (float)((double)gApp->mScale * 45.0);
	int aWrapSpacing = Ftol((double)aLineSpacing);
	int aLeft = Ftol((double)gApp->mScale * 200.0);
	int aTop = Ftol((double)gApp->mScale * 622.0);
	g->WriteWordWrapped(Rect(aLeft, aTop, mWidth - aLeft * 2, 0), sTexts[mPage], aWrapSpacing, 0, nullptr, -1,
		nullptr);

	g->SetColor(Color(48, 48, 48));
	g->SetFont(FONT_INGAME);
	SexyString aCount = StrFormat(GTC(0x7d8ca8), mPage + 1, PAGE_COUNT);
	g->WriteString(aCount, Ftol((double)gApp->mScale * 820.0), Ftol((double)gApp->mScale * 240.0), 0, 1, true, 0,
		-1, -1);
	g->SetColor(Color(255, 255, 255));
	if (mApp->mBoard != nullptr) {
		int aBarY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(IMAGE_HOWTOPLAY_HELPDIALOGBARLRG_ID));
		int aBarX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(IMAGE_HOWTOPLAY_HELPDIALOGBARLRG_ID));
		g->DrawImage(IMAGE_HOWTOPLAY_HELPDIALOGBARLRG, aBarX, aBarY);
	}
}

void HelpPanel::MouseDown(int, int, int, int)
{
	mMouseDown = true;
}

void HelpPanel::MouseUp(int, int, int)
{
	mMouseDown = false;
}

void HelpPanel::ButtonPress(int, int)
{
	mApp->PlaySample(SOUND_CLICK);
}

void HelpPanel::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_CLICK_DIALOG);
	if (theId == BUTTON_BACK) {
		// zooms out, and the game's options it came from zoom back in
		mZooming = true;
		mZoom.SetCurve(GTS(0x7d8c7c));
		static_cast<TwistWidget*>(mApp->mOptionsDialog)->mZoom.SetCurve(GTS(0x7d8c58));
		return;
	}
	if (theId == BUTTON_PREV)
		PrevPage();
	else if (theId == BUTTON_NEXT)
		NextPage();
	else
		return;
	mPrevButton->SetDisabled(mPage == 0);
	mNextButton->SetDisabled(mPage == PAGE_COUNT - 1);
}

void HelpPanel::PopAnimStopped(int)
{
	mAnim->PlayLabel(HelpPageLabel(mPage));
}

void HelpPanel::MakeAnim()
{
	mAnim = new PopAnim(0, this);
	mAnim->mImageFolders.clear();
	mAnim->mImageFolders.push_back(StrFormat(GTC(0x7d9bac), mApp->mArtRes));
	mAnim->mImageScale = gApp->mScale;
	mAnim->mDrawScale = gApp->mScale;
	mAnim->LoadAnimation(GTS(0x7d9b88));
}

void HelpPanel::SetPage(int thePage)
{
	mPage = thePage;
	mAnim->PlayLabel(HelpPageLabel(thePage));
}

void HelpPanel::PrevPage()
{
	if (0 < mPage)
		SetPage(mPage - 1);
}

void HelpPanel::NextPage()
{
	if (mPage < PAGE_COUNT - 1)
		SetPage(mPage + 1);
}

ProfilePanel::ProfilePanel(Menu* theMenu)
{
	mMenu = theMenu;
	mApp = theMenu->mApp;
	mImageId = IMAGE_MAINMENU_PROFILEDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_MAINMENU_PROFILEDIALOG_ID);
	int aWidth = mDialogImage->mWidth;
	int aHeight = mDialogImage->mHeight;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	Resize(0, Ftol((double)GetImageOffsetY(IMAGE_MAINMENU_PROFILEDIALOG_ID) * aScale), aWidth, aHeight);

	mList = new ListWidget(0, FONT_MENU_TINY, this);
	mList->mJustify = ListWidget::JUSTIFY_CENTER;
	mList->SetColor(ListWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mList->SetColor(ListWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mList->SetColor(ListWidget::COLOR_TEXT, Color(0x6800b8));
	mList->SetColor(ListWidget::COLOR_HILITE, Color(0xffffff));
	mList->SetColor(ListWidget::COLOR_SELECT, Color(0xc332d7));
	mList->mDrawOutline = false;
	mList->mItemHeight = Ftol((double)gApp->mScale * 40.0);
	mMouseDown = false;
	mClicked = false;
	mCreateButton = new TwistButton(BUTTON_CREATE, IMAGE_MAINMENU_CREATEBUTTON, this);
	mDeleteButton = new TwistButton(BUTTON_DELETE, IMAGE_MAINMENU_DELETEBUTTON, this);
}

ProfilePanel::~ProfilePanel()
{
	RemoveWidget(mList);
	delete mList;
	RemoveWidget(mCreateButton);
	delete mCreateButton;
	RemoveWidget(mDeleteButton);
	delete mDeleteButton;
	RemoveAllWidgets(true, false);
}

void ProfilePanel::AddedToManager(WidgetManager* theWidgetManager)
{
	MenuPanel::AddedToManager(theWidgetManager);
	float aScale = (float)gApp->mArtRes / 1200.0f;
	int aHeight = Ftol((double)gApp->mScale * 400.0);
	int aWidth = (mWidth * 2) / 3;
	int aY = Ftol((double)aScale * 240.0);
	int aX = Ftol((double)aScale * 155.0);
	mList->Resize(aX, aY, aWidth, aHeight);
	AddWidget(mList);
	PlaceButton(mCreateButton, mImageId, aScale);
	AddWidget(mCreateButton);
	PlaceButton(mDeleteButton, mImageId, aScale);
	AddWidget(mDeleteButton);
	mRefresh = true;
}

void ProfilePanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveWidget(mList);
	RemoveWidget(mCreateButton);
	RemoveWidget(mDeleteButton);
}

void ProfilePanel::Update()
{
	MenuPanel::Update();
	bool noNewUser = mMenu->mNewUserPanel == nullptr;
	mMouseVisible = noNewUser;
	mList->mMouseVisible = noNewUser;
	if (mSlide.mIncRate > 0.0 || !gApp->mMainMenu->mPanning)
		mSlide.IncInVal();
	if (mRefresh) {
		Refresh();
		mRefresh = false;
	}
}

void ProfilePanel::Draw(Graphics* g)
{
	g->DrawImage(mDialogImage, 0, 0);
	SexyString aRank = mApp->mRanks->GetName(mApp->mProfile->mRank);
	SexyString aName = mApp->mProfile->mName;
	Font* aFont = FONT_DIALOG_CHECKBOX;
	SetFontLayerColor(aFont, 0, Color::White);
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 200));
	int aY = Ftol((double)gApp->mScale * 92.0);
	g->DrawString(aRank, mWidth / 2 - g->GetFont()->StringWidth(aRank) / 2, aY);
	g->SetFont(FONT_DIALOG_TEXT);
	SetFontLayerColor(g->GetFont(), "MAIN", Color::White);
	g->SetColor(Color(255, 255, 0));
	aY = Ftol((double)gApp->mScale * 143.0);
	g->DrawString(aName, mWidth / 2 - g->GetFont()->StringWidth(aName) / 2, aY);
}

void ProfilePanel::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	float aY = (float)(mSlide.GetOutVal() * (double)gApp->mHeight);
	g->Translate(0, Ftol((double)aY));
	MenuPanel::DrawAll(theFlags, g);
	g->Translate(0, Ftol(-(double)aY));
}

void ProfilePanel::KeyChar(SexyChar theChar)
{
	if (theChar != '\r' && theChar != 0x1b) {
		Widget::KeyChar(theChar);
		return;
	}
	static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonDepress(1);
}

void ProfilePanel::MouseDown(int, int, int, int)
{
	mMouseDown = true;
}

void ProfilePanel::MouseUp(int, int, int)
{
	mMouseDown = false;
}

void ProfilePanel::ListClicked(int, int theIdx, int theClickCount)
{
	mList->SetSelect(theIdx);
	SexyString aName = mList->GetStringAt(theIdx);
	if (aName != mApp->mProfile->mName) {
		// port: the original first writes and clears its beta dump (0x7648d2, 0x75e95b; NOTES.md), and reads it
		// again after (0x764814)
		mApp->mProfile->Select(aName);
		mMenu->RefreshPlayer();
	}
	if (theClickCount > 1)
		static_cast<ButtonListener*>(gApp->mMainMenu)->ButtonDepress(1);
}

void ProfilePanel::DialogButtonPress(int, int)
{
	mApp->PlaySample(SOUND_CLICK);
}

void ProfilePanel::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theDialogId != DIALOG_DELETE)
		return;
	mApp->KillDialog(DIALOG_DELETE);
	if (theButtonId == Dialog::ID_YES) {
		SexyString aName = mList->GetStringAt(mList->mSelectIdx);
		if (!aName.empty()) {
			mApp->mProfile->Delete(aName);
			mRefresh = true;
		}
	}
}

void ProfilePanel::ButtonPress(int, int)
{
	gApp->PlaySample(SOUND_CLICK);
}

void ProfilePanel::ButtonDepress(int theId)
{
	if (theId == BUTTON_CREATE) {
		ShowNewUser();
	} else if (theId == BUTTON_DELETE) {
		SexyString aFooter = _S("");
		SexyString aLines = mApp->GetString(GTS(0x7db980), GT(0x7db928));
		SexyString aHeader = mApp->GetString(GTS(0x7db914), GT(0x7db8f8));
		TwistDialog* aDialog = static_cast<TwistDialog*>(mApp->DoDialog(DIALOG_DELETE, true, aHeader, aLines, aFooter,
			TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO));
		aDialog->mTwistListener = this;
	}
}

void ProfilePanel::Show()
{
	if (mSlide.mIncRate <= 0.0)
		mSlide.SetCurve(GTS(0x7db9ac));
}

void ProfilePanel::Hide()
{
	if (mSlide.mIncRate >= 0.0 && mSlide.mIncRate != 0.0) {
		mSlide.mTriggered = false;
		mSlide.mIncRate = -mSlide.mIncRate;
	}
}

void ProfilePanel::Refresh()
{
	mList->RemoveAll();
	int aCount = (int)mApp->mProfile->mNames.size();
	int aSelect = -1;
	for (int i = 0; i < aCount; i++) {
		SexyString aName = mApp->mProfile->GetName(i);
		if (!aName.empty()) {
			mList->AddLine(aName, false);
			if (aName == mApp->mProfile->mName)
				aSelect = i;
		}
	}
	if (aSelect == -1 && mList->GetLineCount() != 0)
		aSelect = 0;
	if (aSelect != -1)
		ListClicked(0, aSelect, 1);
	else
		ShowNewUser();
	mMenu->RefreshPlayer();
	if (mCreateButton != nullptr)
		mCreateButton->mDisabled = mApp->mProfile->mNames.size() >= 8;
}

void ProfilePanel::ShowNewUser()
{
	mMenu->ShowNewUser();
	gApp->mGates->mProfileButton->SetGlowOn(false);
}

NewUserPanel::NewUserPanel(ButtonListener* theButtonListener, EditListener* theEditListener, bool canCancel)
{
	mEditListener = theEditListener;
	mImageId = IMAGE_MAINMENU_NEWUSERDIALOG_ID;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	TwistButton* aButton = new TwistButton(BUTTON_OK, IMAGE_MAINMENU_OKBUTTON, theButtonListener);
	PlaceButton(aButton, mImageId, aScale);
	AddWidget(aButton);
	if (!canCancel) {
		int aWidth = GetImageById(mImageId)->mWidth;
		aButton->mX = aWidth / 2 - aButton->mButtonImage->GetCelWidth() / 2;
	} else {
		aButton = new TwistButton(BUTTON_CANCEL, IMAGE_MAINMENU_CANCELBUTTON, theButtonListener);
		PlaceButton(aButton, mImageId, aScale);
		AddWidget(aButton);
	}
	Image* anImage = GetImageById(mImageId);
	Resize(0, 0, anImage->mWidth, anImage->mHeight);
}

NewUserPanel::~NewUserPanel()
{
	RemoveAllWidgets(true, false);
}

void NewUserPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	mEditWidget = new TwistEditWidget(0, mEditListener);
	mEditWidget->mTextX = Ftol((double)gApp->mScale * 12.0);
	mEditWidget->mCursorOffsetX = Ftol((double)gApp->mScale * -5.0);
	mEditWidget->mSelectionExtra = Ftol((double)gApp->mScale);
	int aHeight = Ftol((double)gApp->mScale * 50.0);
	int aWidth = Ftol((double)mWidth * 0.699999988079071);
	int aY = Ftol((double)gApp->mScale * 74.0);
	int aX = Ftol((double)gApp->mScale * 150.0);
	mEditWidget->Resize(aX, aY, aWidth, aHeight);
	mEditWidget->SetFont(FONT_MENU_TINY, nullptr);
	SetFontLayerColor(mEditWidget->mFont, 0, Color::White);
	mEditWidget->SetColor(EditWidget::COLOR_BKG, Color(0, 0, 0, 0));
	mEditWidget->SetColor(EditWidget::COLOR_TEXT, Color(0xf0f0f0));
	mEditWidget->SetColor(EditWidget::COLOR_HILITE, Color(0xc332d7));
	mEditWidget->SetColor(EditWidget::COLOR_OUTLINE, Color(0, 0, 0, 0));
	mEditWidget->mMaxChars = 11;
	AddWidget(mEditWidget);
	SetFocus(mEditWidget);
	TwistWidget::AddedToManager(theWidgetManager);
	mCapitalize = true;
	mEditWidget->SetText(_S(""), true);
	gApp->mWidgetManager->SetFocus(mEditWidget);
	mSlide.SetCurve(GTS(0x7d9e74));
}

void NewUserPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	// The menu it is in (the original passes its parent as it is)
	gApp->mWidgetManager->SetFocus(dynamic_cast<Widget*>(mParent));
	RemoveWidget(mEditWidget);
	gApp->SafeDeleteWidget(mEditWidget);
	mEditWidget = nullptr;
	TwistWidget::RemovedFromManager(theWidgetManager);
}

void NewUserPanel::Update()
{
	if (!mSlide.IncInVal() && mSlide.mIncRate < 0.0) {
		mParent->RemoveWidget(this);
		gApp->SafeDeleteWidget(this);
	}
	if (mCapitalize && mEditWidget != nullptr && !mEditWidget->mString.empty()) {
		// The original's text is wide (iswlower and towupper); the port's is Latin-1
		SexyChar& aFirst = mEditWidget->mString[0];
		if (iswlower((unsigned char)aFirst))
			aFirst = (SexyChar)towupper((unsigned char)aFirst);
		mCapitalize = false;
	}
}

void NewUserPanel::Draw(Graphics* g)
{
	g->DrawImage(GetImageById(mImageId), 0, 0);
}

void NewUserPanel::DrawAll(ModalFlags* theFlags, Graphics* g)
{
	float aY = (float)(mSlide.GetOutVal() * (double)gApp->mHeight * 0.5);
	g->Translate(0, Ftol((double)aY));
	TwistWidget::DrawAll(theFlags, g);
	g->Translate(0, Ftol(-(double)aY));
}

void NewUserPanel::Hide()
{
	mSlide.mIncRate = -fabs(mSlide.mIncRate);
	mSlide.mTriggered = false;
}

namespace {

// The resolutions' radio buttons: their place in resources.xml and the art they choose (a table at 0x84e754 of
// three pairs, then their count)
const unsigned int kResTable = GTB(0x84e754, 28);

int ResPlaceId(int theIndex)
{
	return GameInt(kResTable + theIndex * 8);
}

int ResArtRes(int theIndex)
{
	return GameInt(kResTable + theIndex * 8 + 4);
}

int ResCount()
{
	return GameInt(kResTable + 0x18);
}

// The text of a dialog line: its property, or the default when the properties have none
SexyString OptionsText(unsigned int theId, unsigned int theDefault)
{
	return gApp->GetString(GTS(theId), GT(theDefault));
}

}  // namespace

OptionsPanel::OptionsPanel(WinTwistApp* theApp)
{
	mApp = theApp;
	mImageId = IMAGE_MAINMENU_OPTIONSDIALOG_ID;
	float aScale = (float)((double)theApp->mArtRes / 1200.0);
	// A button at its place in resources.xml, relative to the dialog's
	auto aPlace = [&](Widget* theWidget, int thePlaceId, int theWidth, int theHeight) {
		int aY = Ftol((double)(GetImageOffsetY(thePlaceId) - GetImageOffsetY(mImageId)) * (double)aScale);
		int aX = Ftol((double)(GetImageOffsetX(thePlaceId) - GetImageOffsetX(mImageId)) * (double)aScale);
		theWidget->Resize(aX, aY, theWidth, theHeight);
	};

	if (theApp->mBoard == nullptr) {
		mRestartButton = nullptr;
		mQuitButton = nullptr;
		mReturnButton = nullptr;
	} else {
		// Over a game: the return, quit (challenge) and restart (help) buttons by the game's mode
		Image* anImage = IMAGE_MAINMENU_OPTIONSRETURNBUTTON;
		mReturnButton = new TwistButton(BUTTON_RETURN, anImage, this);
		mReturnButton->SetFont(FONT_DIALOG_BUTTONS);
		aPlace(mReturnButton, IMAGE_MAINMENU_OPTIONSRETURNBUTTON_ID, anImage->GetCelWidth(), anImage->GetCelHeight());

		bool aChallenge = theApp->mBoard->mGameMode == 2;
		mQuitButton = new TwistButton(BUTTON_QUIT,
			aChallenge ? IMAGE_MAINMENU_OPTIONSCHALLENGEBUTTON : IMAGE_MAINMENU_OPTIONSQUITBUTTON, this);
		mQuitButton->SetFont(FONT_DIALOG_BUTTONS);
		anImage = IMAGE_MAINMENU_OPTIONSQUITBUTTON;
		aPlace(mQuitButton, IMAGE_MAINMENU_OPTIONSQUITBUTTON_ID, anImage->GetCelWidth(), anImage->GetCelHeight());

		int aMode = theApp->mBoard->mGameMode;
		int aRestartPlace;
		if (aMode == 2) {
			anImage = IMAGE_MAINMENU_OPTIONSRESTARTBUTTON;
			mRestartButton = new TwistButton(BUTTON_RESTART, anImage, this);
			aRestartPlace = IMAGE_MAINMENU_OPTIONSRESTARTBUTTON_ID;
		} else if (aMode == 3) {
			anImage = IMAGE_MAINMENU_OPTIONSRESTARTBLITZBUTTON;
			mRestartButton = new TwistButton(BUTTON_RESTART, anImage, this);
			aRestartPlace = IMAGE_MAINMENU_OPTIONSRESTARTBLITZBUTTON_ID;
		} else {
			anImage = IMAGE_MAINMENU_OPTIONSHELPBUTTON;
			mRestartButton = new TwistButton(BUTTON_HELP, anImage, this);
			aRestartPlace = IMAGE_MAINMENU_OPTIONSRESTARTBUTTON_ID;
		}
		aPlace(mRestartButton, aRestartPlace, anImage->GetCelWidth(), anImage->GetCelHeight());

		// it zooms in
		mZooming = true;
		mZoom.SetCurve(GTS(0x7d9bc0));
	}

	TwistSlider* aSlider = new TwistSlider(GetImageById(IMAGE_MAINMENU_SLIDERBARBASEMUSIC_ID),
		IMAGE_MAINMENU_SLIDERTHUMBMUSIC, SLIDER_MUSIC, this);
	mMusicSlider = aSlider;
	aSlider->SetValue(theApp->GetMusicVolume());
	Image* aTrack = GetImageById(IMAGE_MAINMENU_SLIDERBARBASEMUSIC_ID);
	aPlace(aSlider, IMAGE_MAINMENU_SLIDERBARBASEMUSIC_ID, aTrack->mWidth, aTrack->mHeight);
	aSlider = new TwistSlider(GetImageById(IMAGE_MAINMENU_SLIDERBARBASESOUNDS_ID), IMAGE_MAINMENU_SLIDERTHUMBSOUNDS,
		SLIDER_SOUNDS, this);
	mSoundsSlider = aSlider;
	aSlider->SetValue(theApp->GetSfxVolume());
	aTrack = GetImageById(IMAGE_MAINMENU_SLIDERBARBASESOUNDS_ID);
	aPlace(aSlider, IMAGE_MAINMENU_SLIDERBARBASESOUNDS_ID, aTrack->mWidth, aTrack->mHeight);

	// Each box tells the panel at once, which ignores it: the panel is not in the manager yet
	mFullscreenCheckbox = MakeCheckbox(IMAGE_MAINMENU_RADIOBUTTONFULLSCREEN_ID, CHECKBOX_FULLSCREEN);
	mFullscreenCheckbox->SetChecked(!theApp->mIsWindowed, true);
	m3DCheckbox = MakeCheckbox(IMAGE_MAINMENU_RADIOBUTTONHWA_ID, CHECKBOX_3D);
	m3DCheckbox->SetChecked(theApp->Is3DAccelerated(), true);
	mHintsCheckbox = MakeCheckbox(IMAGE_MAINMENU_RADIOBUTTONHINTS_ID, CHECKBOX_HINTS);
	mHintsCheckbox->SetChecked(theApp->mShowHints, true);
	mCustomCursorsCheckbox = MakeCheckbox(IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS_ID, CHECKBOX_CUSTOM_CURSORS);
	mCustomCursorsCheckbox->SetChecked(theApp->mProfile->mCustomCursors, true);
	mVisualizersCheckbox = MakeCheckbox(IMAGE_MAINMENU_RADIOBUTTONVISUALIZERS_ID, CHECKBOX_VISUALIZERS);
	// the visualizers need Direct3D
	mVisualizersCheckbox->SetChecked(theApp->mDoViz && gD3DInterface != nullptr, true);
	int i = 0;
	if (0 < ResCount()) {
		do {
			mResCheckboxes[i] = MakeCheckbox(ResPlaceId(i), CHECKBOX_RES_FIRST + i);
			mResCheckboxes[i]->SetChecked(mApp->mArtRes == ResArtRes(i), true);
			i++;
		} while (i < ResCount());
	}

	Image* aDialogImage = GetImageById(mImageId);
	int aHeight = Ftol((double)aDialogImage->mHeight * (double)1.1f);
	int aWidth = aDialogImage->mWidth;
	int aY = Ftol((double)GetImageOffsetY(mImageId) * (double)aScale);
	int aX = Ftol((double)GetImageOffsetX(mImageId) * (double)aScale);
	Resize(aX, aY, aWidth, aHeight);
	mSettingBoxes = false;
}

OptionsPanel::~OptionsPanel()
{
	delete mMusicSlider;
	delete mSoundsSlider;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete m3DCheckbox;
	delete mVisualizersCheckbox;
	delete mHintsCheckbox;
	delete mReturnButton;
	delete mQuitButton;
	// port: the original deletes neither the resolutions' radio buttons nor the restart button (NOTES.md)
	for (int i = 0; i < RES_COUNT; i++)
		delete mResCheckboxes[i];
	delete mRestartButton;
}

int OptionsPanel::GetPreferredWidth()
{
	return Ftol((double)gApp->mScale * 585.0);
}

void OptionsPanel::AddedToManager(WidgetManager* theWidgetManager)
{
	MenuPanel::AddedToManager(theWidgetManager);
	AddWidget(mMusicSlider);
	AddWidget(mSoundsSlider);
	AddWidget(mFullscreenCheckbox);
	AddWidget(m3DCheckbox);
	AddWidget(mCustomCursorsCheckbox);
	AddWidget(mHintsCheckbox);
	AddWidget(mVisualizersCheckbox);
	int i = 0;
	if (0 < ResCount()) {
		do {
			AddWidget(mResCheckboxes[i]);
			i++;
		} while (i < ResCount());
	}
	if (mReturnButton != nullptr)
		AddWidget(mReturnButton);
	if (mQuitButton != nullptr)
		AddWidget(mQuitButton);
	if (mRestartButton != nullptr)
		AddWidget(mRestartButton);
	if (mApp->mBoard != nullptr && mApp->mBoard->IsTimed())
		mApp->mBoard->Pause(false);
}

void OptionsPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveAllWidgets(false, false);
	if (mApp->mBoard != nullptr && mApp->mBoard->IsTimed())
		mApp->mBoard->Unpause(true);
}

void OptionsPanel::Update()
{
	bool wasZooming = mZooming;
	MenuPanel::Update();
	if (!mZooming && wasZooming && mZoom.GetOutVal() == 0.0) {
		// zoomed out over the game: gone
		mParent->RemoveWidget(this);
		mApp->SafeDeleteWidget(this);
	}
	if (mApp->mBoard == nullptr) {
		// the game is over: its buttons go
		if (mQuitButton != nullptr) {
			RemoveWidget(mQuitButton);
			delete mQuitButton;
			mQuitButton = nullptr;
		}
		if (mReturnButton != nullptr) {
			RemoveWidget(mReturnButton);
			delete mReturnButton;
			mReturnButton = nullptr;
		}
	}
}

void OptionsPanel::Draw(Graphics* g)
{
	mClip = false;
	Rect anOldClip = g->mClipRect;
	if (gD3DInterface != nullptr)
		g->mClipRect = Rect(-1000, -1000, 5000, 5000);  // with Direct3D the clip is opened wide
	g->DrawImage(GetImageById(mImageId), 0, 0);
	if (gApp->mBoard != nullptr) {
		// the bar of the game's buttons, the large one with three
		int aBarId = IMAGE_MAINMENU_OPTIONDIALOGBAR_ID + (mRestartButton != nullptr ? 1 : 0);
		int aY = Ftol((double)(GetImageOffsetY(aBarId) - GetImageOffsetY(mImageId)) * (double)mApp->mScale);
		int aX = Ftol((double)(GetImageOffsetX(aBarId) - GetImageOffsetX(mImageId)) * (double)mApp->mScale);
		g->DrawImage(GetImageById(aBarId), aX, aY);
	}
	g->mClipRect = anOldClip;
}

void OptionsPanel::KeyDown(KeyCode theKey)
{
	if (mZooming)
		return;
	MenuPanel::KeyDown(theKey);
	if (theKey == KEYCODE_ESCAPE || theKey == KEYCODE_RETURN)
		ButtonDepress(BUTTON_RETURN);
}

void OptionsPanel::SliderVal(int theId, double theVal)
{
	if (mZooming)
		return;
	if (theId == SLIDER_MUSIC) {
		mApp->SetMusicVolume(theVal);
	} else if (theId == SLIDER_SOUNDS) {
		mApp->SetSfxVolume(theVal);
		if (!mSoundsSlider->mDragging && mWidgetManager != nullptr)
			mApp->PlaySample(SOUND_TRANSFER);
	}
}

void OptionsPanel::CheckboxChecked(int theId, bool checked)
{
	if (mWidgetManager == nullptr || mSettingBoxes || mZooming)
		return;
	bool aSetRadios = true;
	if (theId == CHECKBOX_FULLSCREEN) {
		bool aWindowed;
		if (!checked) {
			if (gApp->mForceFullscreen) {
				// the desktop cannot take a window: the box is checked again
				SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
				SexyString aLines = OptionsText(GTA(0x7da758), GTA(0x7da628));
				SexyString aHeader = OptionsText(GTA(0x7da614), GTA(0x7da5f0));
				gApp->DoDialog(0x2f, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_YES);
				mFullscreenCheckbox->SetChecked(true, true);
				mApp->PlaySample(SOUND_CLICK_DIALOG);
				return;
			}
			aWindowed = true;
		} else {
			aWindowed = false;
		}
		mApp->SwitchScreenMode(aWindowed);
		// The original releases the mouse capture and forgets the mouse, then makes the app active again
		SDL_CaptureMouse(SDL_FALSE);
		mFullscreenCheckbox->mIsDown = false;
		mFullscreenCheckbox->mIsOver = false;
		mWidgetManager->mDownButtons = 0;
		mWidgetManager->mLastDownWidget = nullptr;
		mWidgetManager->mOverWidget = nullptr;
		mApp->mActive = true;
		mApp->RehupFocus();
	} else if (theId == CHECKBOX_3D) {
		m3DCheckbox->mChecked = checked;
		if (!checked) {
			if (mApp->Is3DAccelerated() && mApp->Is3DAccelerationRecommended()) {
				// it stays on unless the player says yes
				m3DCheckbox->SetChecked(true, true);
				mQuestion = QUESTION_3D_OFF;
				SexyString aFooter = GT(0x80d9a4);
				SexyString aLines = OptionsText(GTA(0x7da940), GTA(0x7da838));
				SexyString aHeader = OptionsText(GTA(0x7da81c), GTA(0x7da7f0));
				mApp->DoDialog(4, true, aHeader, aLines, aFooter,
					TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO);
			}
		} else {
			if (!mApp->Is3DAccelerationSupported()) {
				m3DCheckbox->SetChecked(false, true);
				SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
				SexyString aLines = OptionsText(GTA(0x7daf20), GTA(0x7dae28));
				SexyString aHeader = OptionsText(GTA(0x7db230), GTA(0x7db214));
				gApp->DoDialog(0x30, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_YES);
				return;
			}
			if (!mApp->Is3DAccelerationRecommended()) {
				m3DCheckbox->SetChecked(false, true);
				mQuestion = QUESTION_3D_ON;
				SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
				SexyString aLines = OptionsText(GTA(0x7dacf0), GTA(0x7daa88));
				SexyString aHeader = OptionsText(GTA(0x80d468), GTA(0x7daa78));
				gApp->DoDialog(4, true, aHeader, aLines, aFooter,
					TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO);
			} else {
				if (mApp->Is3DAccelerated()) {
					mApp->PlaySample(SOUND_CLICK_DIALOG);
					return;
				}
				m3DCheckbox->SetChecked(false, true);
				mQuestion = QUESTION_3D_ON;
				SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
				SexyString aLines = OptionsText(GTA(0x7daa3c), GTA(0x7da9c8));
				SexyString aHeader = OptionsText(GTA(0x80d468), GTA(0x7daa78));
				gApp->DoDialog(4, true, aHeader, aLines, aFooter,
					TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO);
			}
		}
	} else if (theId == CHECKBOX_HINTS) {
		mApp->mShowHints = checked;
	} else if (theId == CHECKBOX_CUSTOM_CURSORS) {
		mApp->mProfile->mCustomCursors = checked;
		mApp->mCustomCursorsEnabled = checked;
		mApp->EnableCustomCursors(mApp->mCustomCursorsEnabled);
	} else if (theId == CHECKBOX_VISUALIZERS) {
		if (gD3DInterface == nullptr) {
			// without Direct3D the box goes off again
			mSettingBoxes = true;
			mVisualizersCheckbox->SetChecked(false, true);
			mSettingBoxes = false;
			SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
			SexyString aLines = OptionsText(GTA(0x7db010), GTA(0x7dafa0));
			SexyString aHeader = OptionsText(GTA(0x7db230), GTA(0x7db214));
			gApp->DoDialog(0x28, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_NO);
		} else {
			mApp->mDoViz = checked;
		}
	} else if (CHECKBOX_RES_FIRST - 1 < theId && theId < CHECKBOX_RES_FIRST + RES_COUNT) {
		if (checked) {
			if (!m3DCheckbox->mChecked) {
				// the larger art needs 3D: the radio buttons go back to the art in use
				mQuestion = QUESTION_RES_600;
				SexyString aFooter = GT(0x80d9a4);
				SexyString aLines = OptionsText(GTA(0x7db628), GTA(0x7db5a0));
				SexyString aHeader = OptionsText(GTA(0x7db58c), GTA(0x7db568));
				mApp->DoDialog(4, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_NO);
				aSetRadios = false;
			} else {
				int anArtRes = mApp->mArtRes;
				if ((theId == CHECKBOX_RES_FIRST && 600 < anArtRes) ||
					(theId == CHECKBOX_RES_FIRST + 1 && 768 < anArtRes)) {
					aSetRadios = false;
					mQuestion = (theId != CHECKBOX_RES_FIRST) + QUESTION_RES_600;
					SexyString aFooter = GT(0x80d9a4);
					SexyString aLines = OptionsText(GTA(0x7db4e0), GTA(0x7db3d0));
					SexyString aHeader = OptionsText(GTA(0x7db3b8), GTA(0x7db394));
					mApp->DoDialog(4, true, aHeader, aLines, aFooter,
					TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO);
				}
				if ((theId == CHECKBOX_RES_FIRST + 1 && anArtRes < 768) ||
					(theId == CHECKBOX_RES_FIRST + 2 && anArtRes < 1200)) {
					aSetRadios = false;
					// port: the original first asks the game's host interface (app +0x658, absent here) whether
					// this art is supported ("IsHighResSupported", "IsUltraResSupported"), and says "Not
					// Supported" (dialog 0x30) when the card cannot take it
					mQuestion = (theId != CHECKBOX_RES_FIRST + 1) + QUESTION_RES_768;
					SexyString aFooter = GT(0x80d9a4);
					SexyString aLines = OptionsText(GTA(0x7db190), GTA(0x7db088));
					SexyString aHeader = OptionsText(GTA(0x7db070), GTA(0x7db048));
					mApp->DoDialog(4, true, aHeader, aLines, aFooter,
					TwistDialog::BUTTON_MODE_YES | TwistDialog::BUTTON_MODE_NO);
				}
			}
		}
		if (!checked || aSetRadios) {
			// the one clicked is the one checked, even when it was clicked off
			int aCount = ResCount();
			int i = 0;
			if (0 < aCount) {
				do {
					mResCheckboxes[i]->mChecked = theId == i + CHECKBOX_RES_FIRST;
					i++;
				} while (i < aCount);
			}
		} else {
			// back to the art in use, which only 3D can raise above 600
			int anArtRes = mApp->mArtRes;
			bool a3D = m3DCheckbox->mChecked;
			mResCheckboxes[0]->mChecked = !(anArtRes != 600 && a3D);
			mResCheckboxes[1]->mChecked = anArtRes == 768 && a3D;
			mResCheckboxes[2]->mChecked = anArtRes == 1200 && a3D;
		}
	}
	mApp->PlaySample(SOUND_CLICK_DIALOG);
}

void OptionsPanel::ButtonDepress(int theId)
{
	if (mZooming)
		return;
	mApp->PlaySample(SOUND_CLICK_DIALOG);
	if (theId == BUTTON_RETURN) {
		mApp->FinishOptionsDialog(true);
	} else if (theId == BUTTON_QUIT) {
		// 0x6b38f3: the game's quit question, then FinishOptionsDialog(false)
		mApp->mBoard->AskQuit();
		mApp->FinishOptionsDialog(false);
	} else if (theId == BUTTON_RESTART) {
		// The restart question (dialog 0xb), by the game's mode
		SexyString aHeader;
		SexyString aLines;
		if (mApp->mBoard->mGameMode == 3) {
			aHeader = mApp->GetString(GTS(0x7db6fc));
			aLines = mApp->GetString(GTS(0x7db6d8));
		} else {
			aHeader = mApp->GetString(GTS(0x7db6b0));
			aLines = mApp->GetString(GTS(0x7db688));
		}
		TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 0xb, true, aHeader, aLines,
			GT(0x80d9a4), 3);
		aDialog->mTwistListener = mApp;
		double aMinWidth = (double)gGemWidth * 6.75;
		double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
		aDialog->mWidth = Ftol(aWidth);
		aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
		aDialog->mZOrder = 200;
		mApp->AddDialog(aDialog);
		aDialog->mX = mApp->mWidth / 2 - aDialog->mWidth / 2;
		aDialog->mY = mApp->mHeight / 2 - aDialog->mHeight / 2;
		aDialog->mYesButton->mLabel = mApp->GetString(GTS(0x812e14), GT(0x7db674));
		aDialog->mNoButton->mLabel = mApp->GetString(GTS(0x812e10), GT(0x7db66c));
		mApp->FinishOptionsDialog(false);
	} else if (theId == BUTTON_HELP) {
		// the panel zooms out, and the help comes up over the game
		mZooming = true;
		mZoom.SetCurve(GTS(0x7db720));
		mApp->ShowHelpDialog();
		mApp->ClearUpdateBacklog(false);
	}
}

TwistCheckbox* OptionsPanel::MakeCheckbox(int thePlaceId, int theId)
{
	Image* anImage = IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS;
	TwistCheckbox* aCheckbox = new TwistCheckbox(anImage, anImage, theId, this);
	int aDialogId = mImageId;
	float aScale = mApp->mScale;
	int aY = Ftol((double)(GetImageOffsetY(thePlaceId) - GetImageOffsetY(aDialogId)) * (double)aScale);
	int aX = Ftol((double)(GetImageOffsetX(thePlaceId) - GetImageOffsetX(aDialogId)) * (double)aScale);
	aCheckbox->Resize(aX, aY, anImage->GetCelWidth(), anImage->GetCelHeight());
	return aCheckbox;
}

void OptionsPanel::ApplySettings(bool theResolution)
{
	bool aWindowed = !mFullscreenCheckbox->IsChecked();
	bool a3D = m3DCheckbox->IsChecked();
	mCustomCursorsCheckbox->IsChecked();
	if (a3D == mApp->Is3DAccelerated()) {
		if (aWindowed != gApp->mIsWindowed)
			gApp->SwitchScreenMode(aWindowed, a3D);
	} else {
		if (!a3D) {
			mApp->RegistryWriteBoolean("Is3D", false);
		} else {
			mApp->RegistryEraseValue(_S("Is3D"));
			mApp->mFullscreenBits = 32;
			mApp->RegistryWriteInteger("FullscreenBits", mApp->mFullscreenBits);
			// The original, when its display has no 3D device (0x4a7930: so in 2D mode), writes the art's size,
			// which 2D mode made 600, and asks to confirm 3D at the restart (0xe13ba4, not ported: the port's restart
			// is a new start)
			if (!mApp->Is3DAccelerated())
				mApp->mArtResChanged = true;
		}
		// The original's restart (+0x81b, then Shutdown): natively the exe starts again, on the web the page
		// reloads, so its canvas is new and takes the WebGL2 context 3D mode asks for
		mApp->RequestRestart(true, nullptr);
	}
	mApp->EnableCustomCursors(mCustomCursorsCheckbox->IsChecked());
	int anArtRes;
	if (mResCheckboxes[0]->IsChecked())
		anArtRes = 600;
	else
		anArtRes = mResCheckboxes[1]->IsChecked() ? 768 : 1200;
	if (theResolution && anArtRes != mApp->mArtRes) {
		// 0x6a006a: the new size is kept (written at shutdown, +0xf35), the window goes back to its default place, and
		// the game restarts
		mApp->mArtRes = anArtRes;
		mApp->mPreferredX = -1;
		mApp->mPreferredY = -1;
		mApp->mArtResChanged = true;
		mApp->RequestRestart(true, nullptr);
	}
}

void OptionsPanel::Answer(bool theYes)
{
	if (!theYes)
		return;
	mSettingBoxes = true;
	switch (mQuestion) {
	case QUESTION_3D_ON:
		m3DCheckbox->SetChecked(true, true);
		break;
	case QUESTION_3D_OFF:
		m3DCheckbox->SetChecked(false, true);
		mResCheckboxes[0]->SetChecked(true, true);
		mResCheckboxes[1]->SetChecked(false, true);
		mResCheckboxes[2]->SetChecked(false, true);
		break;
	case QUESTION_RES_600:
		mResCheckboxes[0]->SetChecked(true, true);
		mResCheckboxes[1]->SetChecked(false, true);
		mResCheckboxes[2]->SetChecked(false, true);
		break;
	case QUESTION_RES_768:
		mResCheckboxes[0]->SetChecked(false, true);
		mResCheckboxes[1]->SetChecked(true, true);
		mResCheckboxes[2]->SetChecked(false, true);
		break;
	case QUESTION_RES_1200:
		mResCheckboxes[0]->SetChecked(false, true);
		mResCheckboxes[1]->SetChecked(false, true);
		mResCheckboxes[2]->SetChecked(true, true);
		break;
	default:
		mSettingBoxes = false;
		return;
	}
	ApplySettings(true);
	mSettingBoxes = false;
}

EndStats::EndStats(WinTwistApp* theApp)
	: mApp(theApp)
{
	// The bars' images, their stats and the stats under them (0x84ee40, 0x84ee50 and a table on the stack), and the
	// numbers' colours (0x85d608)
	static const int kBarImages[4] = {IMAGE_FINALSTATSUI_BAR1_ID, IMAGE_FINALSTATSUI_BAR2_ID,
		IMAGE_FINALSTATSUI_BAR3_ID, IMAGE_FINALSTATSUI_BAR4_ID};
	static const int kBarStats[4] = {60, 61, 59, 58};
	static const int kMultStats[4] = {0x4a, 0x45, 0x40, 0x41};
	static const Color kBarColors[4] = {Color(255, 0, 255), Color(255, 96, 0), Color(255, 200, 0), Color(255, 255, 0)};

	mState = 0;
	mReplayFade = 0;
	mDialog = IMAGE_FINALSTATSUI_DIALOGBOX;
	mMinHeight = Ftol((double)mApp->mScale * 100.0);
	mPercent = 0;
	mBiggestValue = 0;
	mBiggest = 0;
	mClip = false;
	for (int i = 0; i < 4; i++) {
		Bar& aBar = mBars[i];
		aBar.mValue = mApp->mStats[kBarStats[i]];
		aBar.mShown = 0;
		aBar.mMult = mApp->mStats[kMultStats[i]];
		aBar.mMultCopy = aBar.mMult;
		aBar.mHeight = mMinHeight;
		aBar.mFullHeight = mMinHeight;
		aBar.mColor = kBarColors[i];
		aBar.mLabel = mApp->GetString(StrFormat(GTS(0x7e207c).c_str(), i));
		aBar.mImageId = kBarImages[i];
		aBar.mIsShown = true;
		if (mBiggestValue < aBar.mValue) {
			mBiggestValue = aBar.mValue;
			mBiggest = i;
		}
		mPercent += aBar.mValue;
	}
	float aMaxHeight = (float)Ftol((double)mApp->mScale * 220.0);
	if (mBiggestValue < 1) {
		mStep = 0;
	} else {
		int i = 4;
		Bar* aBar = mBars;
		do {
			aBar->mFullHeight = Ftol((long double)aBar->mValue / (long double)mBiggestValue * (long double)aMaxHeight);
			aBar++;
			i--;
		} while (i != 0);
		int aStep = Ftol((double)mBiggestValue * 0.009999999776482582);
		mStep = aStep;
		if (aStep < 0x65) {
			aStep -= aStep % 10;
			if (aStep < 10)
				aStep = 10;
		} else {
			aStep -= aStep % 100;
		}
		mStep = aStep;
	}
	unsigned int aScore = (unsigned int)mApp->mStats[73];
	if (aScore == 0)
		mPercent = 0;
	else
		mPercent = Ftol((long double)mPercent / (long double)aScore * 100.0);
	mDrop.SetCurve(GTS(0x7e2050));
	mY = Ftol((double)-mApp->mHeight * 1.399999976158142);

	TwistButton* aButton = new TwistButton(0, IMAGE_FINALSTATSUI_CONTINUEBTN, this);
	AddWidget(aButton);
	aButton->mWidth = aButton->mButtonImage->GetCelWidth();
	aButton->mHeight = aButton->mButtonImage->GetCelHeight();
	aButton->mX = mDialog->mWidth / 2 - aButton->mWidth / 2;
	aButton->mY = Ftol((double)mDialog->mHeight * 0.9179999828338623);
	mContinueButton = aButton;
	int aY = Ftol((double)mDialog->mHeight * 0.36500000953674316);
	for (int i = 0; i < 3; i++) {
		mReplayButtons[i] = nullptr;
	}
	for (int i = 0; i < 3; i++) {
		// a button by each replay the game kept
		if (!mApp->mBestMoves[i].mBuffer.mData.empty()) {
			aButton = new TwistButton(i + 1, IMAGE_FINALSTATSUI_VIEWREPLAYBTN, this);
			mReplayButtons[i] = aButton;
			aButton->mWidth = aButton->mButtonImage->GetCelWidth();
			aButton->mHeight = aButton->mButtonImage->GetCelHeight();
			aButton->mX = Ftol((double)mDialog->mWidth * 0.6470000147819519);
			aButton->mY = aY;
			AddWidget(aButton);
		}
		aY = Ftol((double)IMAGE_FINALSTATSUI_VIEWREPLAYBTN->GetCelHeight() * 1.350000023841858 + (double)aY);
	}
	mColorCycle.mSpeed = 0.25f;
	for (int i = 0; i < 84; i++)
		mStats[i] = mApp->mStats[i];
	for (int i = 0; i < 3; i++)
		mReplayCRCs[i] = mApp->mBestMoves[i].mBuffer.GetCRC32(0);
}

EndStats::~EndStats()
{
	RemoveAllWidgets(true, true);
}

int EndStats::GetPreferredWidth()
{
	return mDialog->mWidth;
}

int EndStats::GetPreferredHeight()
{
	return mDialog->mHeight;
}

void EndStats::Update()
{
	TwistWidget::Update();
	if (mApp->mBoard == nullptr) {
		if (gD3DInterface == nullptr && mState != 0 && mDrop.HasBeenTriggered() && mDrop.GetOutVal() < -1.0) {
			// without Direct3D, once it is out of sight above it takes another curve
			mDrop.SetCurve(GTS(0x7e21d4));
		}
		mReplayFade = (float)((double)mReplayFade - 0.019999999552965164);
	} else {
		// A replay plays on its board under the stats: once its move is over and the delay has run out it fades and
		// goes, and the buttons work again
		Board* aBoard = mApp->mBoard;
		aBoard->mBoardOffsetX = (float)Ftol((double)gApp->mScale * 50.0);
		mApp->mBoard->mBoardOffsetY = (float)Ftol((double)gApp->mScale * 450.0);
		aBoard = mApp->mBoard;
		if (aBoard->mReplayWaiting && 0 < aBoard->mRecordedFrames) {
			float aDelay = (float)((double)mReplayFadeDelay - 0.009999999776482582);
			mReplayFadeDelay = aDelay;
			if (aDelay <= 0.0f) {
				float aFade = (float)((double)mReplayFade - 0.019999999552965164);
				mReplayFade = aFade;
				if (aFade < 0.5f) {
					mApp->mWidgetManager->RemoveWidget(mApp->mBoard);
					delete mApp->mBoard;
					mApp->mBoard = nullptr;
					mApp->mWidgetManager->SetFocus(this);
					for (int i = 0; i < 3; i++) {
						if (mReplayButtons[i] != nullptr)
							mReplayButtons[i]->SetDisabled(false);
					}
					mContinueButton->SetDisabled(false);
				}
			}
		} else {
			float aFade = (float)((double)mReplayFade + 0.019999999552965164);
			if (1.0f < aFade)
				aFade = 1.0f;
			mReplayFade = aFade;
			if (0.5f <= aFade && !aBoard->mVisible)
				aBoard->SetVisible(true);
		}
	}
	mColorCycle.Update();
	mDrop.IncInVal();
	mY = Ftol(mDrop.GetOutVal() * (double)mApp->mHeight);
	if (mState == 0) {
		if (mDrop.HasBeenTriggered()) {
			mArrivedMark = 2.0f;
			mState = 1;
			if (-1 < mApp->mAutoPlayMode)
				ButtonDepress(0);
		}
	} else if (mState == 1) {
		// The bars count up, a step an update, with a tick every third update
		int aThird = mUpdateCnt / 3;
		bool aCounting = false;
		Bar* aBar = mBars;
		int i = 4;
		do {
			int aValue = aBar->mValue;
			if (aValue != aBar->mShown) {
				int aShown = aBar->mShown + mStep;
				if (aValue < aShown)
					aShown = aValue;
				aBar->mShown = aShown;
				float aShare = (float)((long double)aShown / (long double)aValue);
				aBar->mHeight = Ftol((double)aShare * (double)aBar->mFullHeight + (double)mMinHeight);
				aCounting = true;
			}
			aBar++;
			i--;
		} while (i != 0);
		if (aCounting && aThird % 3 == 0) {
			float aPitch = (float)((long double)mBars[mBiggest].mShown / (long double)mBiggestValue * 12.0);
			mApp->PlaySoundEx(SOUND_BLITZ_COUNTUP, 0, aPitch, 0.0f, true, true);
		}
	} else if (mState == 2) {
		if (mDrop.HasBeenTriggered())
			mClicked = true;
	}
}

void EndStats::Draw(Graphics* g)
{
	g->GetColorizeImages();
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
	int aDialogX = Ftol((double)(mWidth / 2) - (double)mDialog->mWidth * 0.5);
	g->DrawImage(mDialog, aDialogX, Ftol(0.0 - (double)Ftol((double)mApp->mScale * -46.0)));

	// The connectors between the replay buttons whose replays are the same
	Image* aConnector = nullptr;
	double aConnectorY = 0;
	if (mReplayCRCs[0] != 0 && mReplayCRCs[0] == mReplayCRCs[1] && mReplayCRCs[0] == mReplayCRCs[2]) {
		aConnector = IMAGE_FINALSTATSUI_CONNECTORTRIPLE;
		aConnectorY = (double)Ftol((double)mApp->mScale * 457.0);
	} else if (mReplayCRCs[0] != 0 && mReplayCRCs[0] == mReplayCRCs[1]) {
		aConnector = IMAGE_FINALSTATSUI_CONNECTORSHORT;
		aConnectorY = (double)Ftol((double)mApp->mScale * 457.0);
	} else if (mReplayCRCs[0] != 0 && mReplayCRCs[0] == mReplayCRCs[2]) {
		aConnector = IMAGE_FINALSTATSUI_CONNECTORLONG;
		aConnectorY = (double)Ftol((double)mApp->mScale * 458.0);
	} else if (mReplayCRCs[1] != 0 && mReplayCRCs[1] == mReplayCRCs[2]) {
		aConnector = IMAGE_FINALSTATSUI_CONNECTORSHORT;
		aConnectorY = (double)Ftol((double)mApp->mScale * 498.0);
	}
	if (aConnector != nullptr) {
		int aY = Ftol(aConnectorY + 0.0);
		g->DrawImage(aConnector, aDialogX - Ftol((double)mApp->mScale * -808.0), aY);
	}

	// The level (or blitz), the score, the time and the other stats
	SexyString aStr;
	if (mApp->mLastGameMode == 3)
		aStr = mApp->GetString(GTS(0x7e22c0));
	else
		aStr = StrFormat(mApp->GetString(GTS(0x7e22d0)).c_str(), mApp->mNewScoreLevel);
	g->SetFont(FONT_DIALOG_BUTTONS);
	g->SetColor(Color(255, 0, 255));
	int aY = Ftol((double)mApp->mScale * 180.0);
	double aMidX = (double)mWidth * 0.5099999904632568;
	g->DrawString(aStr, Ftol(aMidX - (double)(g->StringWidth(aStr) / 2)), aY);

	float aLineY = (float)((double)(float)((double)mApp->mScale * 276.0) + 4.0);
	Font* aFont = FONT_HUGE_OUTLINE;
	SetFontLayerColor(aFont, 0, Color(255, 255, 255, 0x20));
	SetFontLayerColor(aFont, 1, Color(255, 0, 255));
	SetFontLayerColor(aFont, 2, Color(255, 255, 255));
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 255));
	aStr = CommaSeperate(mStats[73]);
	aMidX = (double)mWidth * 0.5099999904632568;
	aY = Ftol((double)aLineY);
	g->DrawString(aStr, Ftol(aMidX - (double)(g->StringWidth(aStr) / 2)), aY);

	g->SetFont(FONT_SCORE);
	aLineY = (float)((double)aLineY + (double)(float)((double)mApp->mScale * 104.0));
	g->SetColor(Color(0, 0x9c, 255));
	int aMinutes = mStats[35] / 60;
	int aSeconds = mStats[35] % 60;
	aStr = StrFormat(mApp->GetString(GTS(0x7e2290)).c_str(), aMinutes, aSeconds);
	aY = Ftol((double)aLineY);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 500.0), aY);
	aStr = CommaSeperate(mStats[43]);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 950.0), aY);

	aLineY = (float)((double)aLineY + (double)(float)((double)mApp->mScale * 37.5));
	g->SetColor(Color(0, 0xea, 255));
	float aShare = 0;
	if (mStats[33] != 0)
		aShare = (float)(1.0L - (long double)mStats[34] / (long double)mStats[33]);
	aStr = StrFormat(mApp->GetString(GTS(0x7e2270)).c_str(), Ftol((double)aShare * 100.0));
	aY = Ftol((double)aLineY);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 500.0), aY);
	float aMult = 0;
	if (mStats[33] != 0)
		aMult = (float)((long double)mStats[57] / (long double)mStats[33]);
	aStr = StrFormat(mApp->GetString(GTS(0x7e2260)).c_str(), (double)aMult);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 950.0), aY);

	// The replays' scores
	aLineY = (float)((double)aLineY + (double)(float)((double)mApp->mScale * 69.0));
	g->SetColor(Color(0, 255, 0xd8));
	aStr = CommaSeperate(mApp->mBestMoves[0].mBest);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 625.0), Ftol((double)aLineY));
	aLineY = (float)((double)aLineY + (double)(float)((double)mApp->mScale * 43.0));
	g->SetColor(Color(0, 255, 0xd8));
	aStr = CommaSeperate(mApp->mBestMoves[1].mBest);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 625.0), Ftol((double)aLineY));
	aLineY = (float)((double)aLineY + (double)(float)((double)mApp->mScale * 43.0));
	g->SetColor(Color(0, 255, 0x4e));
	aStr = CommaSeperate(mApp->mBestMoves[2].mBest);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 625.0), Ftol((double)aLineY));

	aStr = StrFormat(mApp->GetString(GTS(0x7e2230)).c_str(), mPercent);
	g->SetColor(Color(0, 255, 0));
	aY = 4 - Ftol((double)mApp->mScale * -669.0);
	g->DrawString(aStr, Ftol((double)mApp->mScale * 650.0), aY);

	// The bars, each with its number above, its label and its multiplier
	g->SetColorizeImages(true);
	GetIdByImage(mDialog);
	int aBase = Ftol((double)GetImageOffsetY(mBars[0].mImageId) * (double)mApp->mScale) -
				Ftol((double)mApp->mScale * -108.0);
	for (int i = 0; i < 4; i++) {
		Bar& aBar = mBars[i];
		g->SetColor(Color::White);
		int aX = Ftol((double)GetImageOffsetX(aBar.mImageId) * (double)mApp->mScale) -
				 Ftol((double)mApp->mScale * 380.0);
		int aWidth = GetImageById(aBar.mImageId)->mWidth;
		int aHeight = aBar.mHeight;
		g->DrawImageBox(Rect(aX, aBase - aHeight, aWidth, aHeight), GetImageById(IMAGE_FINALSTATSUI_BAR1_ID + i));
		g->SetColor(aBar.mColor);
		aStr = CommaSeperate(aBar.mShown);
		int aHalf = aWidth / 2;
		aY = aBase - Ftol((double)mApp->mScale * 10.0) - aHeight;
		g->DrawString(aStr, aHalf - g->StringWidth(aStr) / 2 + aX, aY);
		g->SetColor(Color::Black);
		aStr = aBar.mLabel;
		aY = aBase - Ftol((double)mApp->mScale * 50.0);
		g->DrawString(aStr, aHalf - g->StringWidth(aStr) / 2 + aX, aY);
		Color aColor = aBar.mColor;
		aColor.mRed = Ftol((double)aColor.mRed * 0.25);
		aColor.mGreen = Ftol((double)aColor.mGreen * 0.25);
		aColor.mBlue = Ftol((double)aColor.mBlue * 0.25);
		g->SetColor(aColor);
		aStr = StrFormat(GTC(0x7e2208), aBar.mMult);
		aY = aBase - Ftol((double)mApp->mScale * 10.0);
		g->DrawString(aStr, aHalf - g->StringWidth(aStr) / 2 + aX, aY);
	}

	if (0.0f < mReplayFade) {
		double anAlpha = ((double)mReplayFade + (double)mReplayFade) * 255.0;
		if (255.0 < anAlpha)
			anAlpha = 255.0;
		g->SetColor(Color(255, 255, 255, Ftol(anAlpha)));
		g->SetColorizeImages(true);
		int aReplayY = Ftol((double)mApp->mScale * 600.0);
		g->DrawImage(IMAGE_FINALSTATSUI_REPLAY, Ftol((double)mApp->mScale * 180.0), aReplayY);
		DeferOverlay(200);
	}
}

void EndStats::DrawOverlay(Graphics* g)
{
	if (0.5f < mReplayFade) {
		double aFade = 1.0 - (double)mReplayFade;
		int anAlpha = Ftol((aFade + aFade) * 255.0);
		if (0 < anAlpha) {
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, anAlpha));
			int aY = Ftol((double)mApp->mScale * 600.0);
			g->DrawImage(IMAGE_FINALSTATSUI_REPLAY, Ftol((double)mApp->mScale * 180.0), aY);
		}
	}
}

void EndStats::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
}

void EndStats::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

void EndStats::ButtonPress(int, int)
{
	mApp->PlaySample(SOUND_CLICK);
}

void EndStats::ButtonDepress(int theId)
{
	if (mApp->mBoard != nullptr || mState != 1)
		return;
	if (theId == 0) {
		// Continue: it drops away
		mDrop.SetCurve(GTS(0x7e2308));
		mState = 2;
		mMouseVisible = false;
	} else if (0 < theId && theId < 4) {
		// One of the game's replays, on a board of its own (+0x37c4) under the stats; it shows once mReplayFade has
		// risen past half (Update)
		if (gD3DInterface == nullptr)
			mDrop.SetCurve(GTS(0x7e2334));  // without Direct3D the stats rise out of the way
		// port: the exe also sets a flag of its sound manager (+0x300a0) that puts every sound started from now on
		// through DirectSound's parametric equaliser, until the board is removed; the port's sound manager has
		// none. It also makes a Buffer it never uses.
		mApp->mScoresLoaded = false;
		TwistButton** aButton = mReplayButtons;
		int i = 3;
		do {
			if (*aButton != nullptr)
				(*aButton)->SetDisabled(true);
			aButton++;
			i--;
		} while (i != 0);
		mContinueButton->SetDisabled(true);
		Board* aBoard = new Board(mApp, gApp->mLastGameMode, 8, 0);
		aBoard->Resize(0, 0, mApp->mWidth, mApp->mHeight);
		mApp->mBoard = aBoard;
		aBoard->mIsReplay = true;
		mApp->mWidgetManager->AddWidget(aBoard);
		aBoard->ReadConfig(true);
		aBoard->mAlertGlow = 1.0f;
		aBoard->Update();
		aBoard->ShowTube(true, true);
		WinTwistApp::BestMove& aGame = mApp->mBestMoves[theId - 1];
		aBoard->mGameLoaded = false;
		aBoard->SetScore(aBoard->mScore, false);
		aBoard->mStartState = 1;
		aBoard->mBoardZoom = 0.4f;
		aGame.mBuffer.SeekFront();
		aBoard->mShakeY = 0;
		aBoard->mShakeX = 0;
		aBoard->StartReplay(aGame.mBuffer, aGame.mReplayStart, aGame.mReplayEnd, false);
		aBoard->mClearedOnHyperspaceStart = 0;
		aBoard->mTubeOffsetX = 0;
		aBoard->mModeFlags = 8;
		aBoard->mBarWidth = aBoard->GetGridRight();
		aBoard->mAlertGlow = 1.0f;
		aBoard->SetVisible(false);
		mReplayFadeDelay = 1.5f;
		mReplayFade = 0;
		if (gD3DInterface == nullptr) {
			aBoard->mReplaySlideWait = 100;
			aBoard->StartTubeSlide();
			aBoard->mShownBeforeStart = true;
		} else {
			// with Direct3D the gems and the effects run 25 updates ahead instead
			for (int anUpdate = 25; anUpdate != 0; anUpdate--) {
				for (int aCell = 0; aCell < aBoard->mCols * aBoard->mRows; aCell++) {
					if (aBoard->mGrid[aCell] != nullptr)
						aBoard->mGrid[aCell]->Update();
				}
				aBoard->mFXManager->Update();
				aBoard->mFXManager2->Update();
			}
		}
		aBoard->LayoutTube();
		Update();
	}
}
