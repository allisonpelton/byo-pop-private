// The main menu's panels (MenuPanels.h). port: the high scores, help, profile and new player panels are ported; of
// the others only what the menu needs of them.
#include "MenuPanels.h"

#include "ColorCycle.h"
#include "Dialog.h"
#include "EffectsManager.h"
#include "EditListener.h"
#include "Font.h"
#include "GameText.h"
#include "Gates.h"
#include "Graphics.h"
#include "Image.h"
#include "ListWidget.h"
#include "Menu.h"
#include "MenuButtonWidget.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistCheckbox.h"
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

void NotPorted(const char* theWhat)
{
	static std::set<std::string> sSaid;
	if (sSaid.insert(theWhat).second)
		SDL_Log("%s is not ported yet", theWhat);
}

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

MenuPanel::~MenuPanel() = default;

HighScoresPanel::HighScoresPanel(Menu* theMenu)
{
	mMenu = theMenu;
	mClip = false;
	mApp = theMenu->mApp;
	mUnknownB0 = IMAGE_MAINMENU_SCORESDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_MAINMENU_SCORESDIALOG_ID);
	mRevealTimer = 0;
	mVoiceDelay = 0;
	mState = 0;
	mMouseDown = false;
	Resize(0, GetImageOffsetY(IMAGE_MAINMENU_SCORESDIALOG_ID), mDialogImage->mWidth, mDialogImage->mHeight);
	mUnknown164 = 5.0f;
	mUnknown178 = false;
	mCategory = -99;
	mShown = -99;
	SetCategory(1, true);
	ResetNewRow();
	mFXManager = new EffectsManager();
	mStateTimer = 0;
	mUnknown214 = 0;
	mUnknown218 = 0;
	mFade.SetConstant(1.0);
	mFading = false;
	// The two lists from the app's twenty high scores, ten each
	int anIndex = 0;
	std::vector<Row>* aList = mRows;
	do {
		int aCount = 10;
		do {
			Row aRow;
			aRow.mUnknown0 = true;
			aRow.mUnknown4 = 1.0f;
			aRow.mUnknown8 = 0;
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
		// The new score goes into its list: the rows below it make room one by one, each with a gem's sound, and the
		// tenth row's name falls off the list (comes with the end of a game, which alone sets mState)
		NotPorted("HighScoresPanel::Update's new high score (0x695d08)");
	}

	// The board behind the scores after a game (comes with the board)
	if (mApp->mBoard != nullptr)
		NotPorted("HighScoresPanel::Update's board (0x696583)");
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
		// A best move's replay, on a board of its own (comes with the board)
		int aReplay = GameInt(GTB(0x84e550, 12) + (theId - BUTTON_VIEW_REPLAY) * 4);
		const Profile::SavedGame& aGame = mApp->mProfile->mSavedGames[aReplay];
		if (!aGame.mData.mData.empty())
			NotPorted("HighScoresPanel's replays (0x695124)");
	}
}

void HighScoresPanel::SetCategory(int theCategory, bool theNow)
{
	if (mShown != theCategory || theNow || (mCategory != -1 && mCategory != theCategory)) {
		mUnknown218 = 0;
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
	mUnknown1F8 = -1;
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
	mUnknownB0 = IMAGE_HOWTOPLAY_HELPDIALOG_ID;
	mDialogImage = GetImageById(IMAGE_HOWTOPLAY_HELPDIALOG_ID);
	mUnknown158 = 0;
	mMouseDown = false;
	int aSize = Ftol((double)gApp->mScale * 1000.0);
	Resize(0, 0, aSize, aSize);
	mUnknown160 = 5.0f;
	mUnknown170 = false;
	mFXManager = new EffectsManager();
	mUnknown15C = 0;
	mClip = false;
	mUnknown17C = false;
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
			mApp->mUnknownF44 = nullptr;
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
	int aY = Ftol((double)gApp->mScale * (double)GetImageOffsetY(mUnknownB0));
	int aX = Ftol((double)gApp->mScale * (double)GetImageOffsetX(mUnknownB0));
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
	mUnknownB0 = IMAGE_MAINMENU_PROFILEDIALOG_ID;
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
	PlaceButton(mCreateButton, mUnknownB0, aScale);
	AddWidget(mCreateButton);
	PlaceButton(mDeleteButton, mUnknownB0, aScale);
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
	mUnknownB0 = IMAGE_MAINMENU_NEWUSERDIALOG_ID;
	float aScale = (float)gApp->mArtRes / 1200.0f;
	TwistButton* aButton = new TwistButton(BUTTON_OK, IMAGE_MAINMENU_OKBUTTON, theButtonListener);
	PlaceButton(aButton, mUnknownB0, aScale);
	AddWidget(aButton);
	if (!canCancel) {
		int aWidth = GetImageById(mUnknownB0)->mWidth;
		aButton->mX = aWidth / 2 - aButton->mButtonImage->GetCelWidth() / 2;
	} else {
		aButton = new TwistButton(BUTTON_CANCEL, IMAGE_MAINMENU_CANCELBUTTON, theButtonListener);
		PlaceButton(aButton, mUnknownB0, aScale);
		AddWidget(aButton);
	}
	Image* anImage = GetImageById(mUnknownB0);
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
	g->DrawImage(GetImageById(mUnknownB0), 0, 0);
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
	mUnknownB0 = IMAGE_MAINMENU_OPTIONSDIALOG_ID;
	float aScale = (float)((double)theApp->mArtRes / 1200.0);
	// A button at its place in resources.xml, relative to the dialog's
	auto aPlace = [&](Widget* theWidget, int thePlaceId, int theWidth, int theHeight) {
		int aY = Ftol((double)(GetImageOffsetY(thePlaceId) - GetImageOffsetY(mUnknownB0)) * (double)aScale);
		int aX = Ftol((double)(GetImageOffsetX(thePlaceId) - GetImageOffsetX(mUnknownB0)) * (double)aScale);
		theWidget->Resize(aX, aY, theWidth, theHeight);
	};

	if (theApp->mBoard == nullptr) {
		mRestartButton = nullptr;
		mQuitButton = nullptr;
		mReturnButton = nullptr;
	} else {
		// port: over a game the panel has the return, quit (challenge) and restart (help) buttons by the game's mode,
		// and it zooms in (0x7d9bc0); they come with the board
		NotPorted("OptionsPanel over a game (0x69e4c5)");
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
	// port: the visualizers need Direct3D (gD3DInterface), which the port does not have, as in 2D mode
	mVisualizersCheckbox->SetChecked(false, true);
	int i = 0;
	if (0 < ResCount()) {
		do {
			mResCheckboxes[i] = MakeCheckbox(ResPlaceId(i), CHECKBOX_RES_FIRST + i);
			mResCheckboxes[i]->SetChecked(mApp->mArtRes == ResArtRes(i), true);
			i++;
		} while (i < ResCount());
	}

	Image* aDialogImage = GetImageById(mUnknownB0);
	int aHeight = Ftol((double)aDialogImage->mHeight * (double)1.1f);
	int aWidth = aDialogImage->mWidth;
	int aY = Ftol((double)GetImageOffsetY(mUnknownB0) * (double)aScale);
	int aX = Ftol((double)GetImageOffsetX(mUnknownB0) * (double)aScale);
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
	// port: a game that can be paused is paused (0x6a6ad0, 0x6af1d0(false)); it comes with the board
	if (mApp->mBoard != nullptr)
		NotPorted("OptionsPanel pausing the game (0x6af1d0)");
}

void OptionsPanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	MenuPanel::RemovedFromManager(theWidgetManager);
	RemoveAllWidgets(false, false);
	// port: and resumed (0x6af2be(true)); it comes with the board
	if (mApp->mBoard != nullptr)
		NotPorted("OptionsPanel resuming the game (0x6af2be)");
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
	// The original opens the clip wide with Direct3D, which the port does not have
	g->DrawImage(GetImageById(mUnknownB0), 0, 0);
	if (gApp->mBoard != nullptr) {
		// the bar of the game's buttons, the large one with three
		int aBarId = IMAGE_MAINMENU_OPTIONDIALOGBAR_ID + (mRestartButton != nullptr ? 1 : 0);
		int aY = Ftol((double)(GetImageOffsetY(aBarId) - GetImageOffsetY(mUnknownB0)) * (double)mApp->mScale);
		int aX = Ftol((double)(GetImageOffsetX(aBarId) - GetImageOffsetX(mUnknownB0)) * (double)mApp->mScale);
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
		// port: without Direct3D (gD3DInterface, which the port does not have) the box goes off again; with it the
		// original sets mDoViz
		mSettingBoxes = true;
		mVisualizersCheckbox->SetChecked(false, true);
		mSettingBoxes = false;
		SexyString aFooter = OptionsText(GTA(0x80d3c8), GTA(0x80d9a4));
		SexyString aLines = OptionsText(GTA(0x7db010), GTA(0x7dafa0));
		SexyString aHeader = OptionsText(GTA(0x7db230), GTA(0x7db214));
		gApp->DoDialog(0x28, true, aHeader, aLines, aFooter, TwistDialog::BUTTON_MODE_NO);
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
		// port: the game's quit question (0x6b38f3), then FinishOptionsDialog(false); they come with the board
		NotPorted("OptionsPanel's quit (0x6b38f3)");
	} else if (theId == BUTTON_RESTART) {
		// port: the restart question (dialog 0xb, by the game's mode); it comes with the board
		NotPorted("OptionsPanel's restart (0x69fc10)");
	} else if (theId == BUTTON_HELP) {
		// port: the panel zooms out (0x7db720) and the help comes up over the game (0x75f82d); they come with the
		// board
		NotPorted("OptionsPanel's help (0x69fc10)");
	}
}

TwistCheckbox* OptionsPanel::MakeCheckbox(int thePlaceId, int theId)
{
	Image* anImage = IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS;
	TwistCheckbox* aCheckbox = new TwistCheckbox(anImage, anImage, theId, this);
	int aDialogId = mUnknownB0;
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
		// port: 3D acceleration on or off is written to the registry ("Is3D", "FullscreenBits") and the game
		// restarts; the port cannot turn 3D on, so the box never differs
		NotPorted("OptionsPanel restarting for 3D (0x6a006a)");
	}
	mApp->EnableCustomCursors(mCustomCursorsCheckbox->IsChecked());
	int anArtRes;
	if (mResCheckboxes[0]->IsChecked())
		anArtRes = 600;
	else
		anArtRes = mResCheckboxes[1]->IsChecked() ? 768 : 1200;
	if (theResolution && anArtRes != mApp->mArtRes) {
		// port: the original keeps the art's size and restarts the game; the port's art is always 600, which only
		// 3D could change
		NotPorted("OptionsPanel restarting for the resolution (0x6a006a)");
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

ChallengeMenu::ChallengeMenu()
{
	NotPorted("ChallengeMenu");
}

void ChallengeMenu::Show(int, bool)
{
	NotPorted("ChallengeMenu::Show");
}

int ChallengeMenu::Unknown6E0DDB(bool, bool)
{
	NotPorted("ChallengeMenu 0x6e0ddb");
	return 0;
}

EndStats::EndStats(WinTwistApp*)
{
	NotPorted("EndStats");
}

int EndStats::GetPreferredWidth()
{
	return 0;
}

int EndStats::GetPreferredHeight()
{
	return 0;
}
