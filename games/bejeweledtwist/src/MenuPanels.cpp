// The main menu's panels (MenuPanels.h). port: the high scores, profile and new player panels are ported; of the
// others only what the menu needs of them.
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
#include "TwistDialog.h"
#include "TwistEditWidget.h"
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

HelpPanel::HelpPanel(WinTwistApp*)
{
	NotPorted("HelpPanel");
}

void HelpPanel::SetPage(int thePage)
{
	mPage = thePage;
	NotPorted("HelpPanel::SetPage");
}

void HelpPanel::NextPage()
{
	NotPorted("HelpPanel::NextPage");
}

void HelpPanel::PrevPage()
{
	NotPorted("HelpPanel::PrevPage");
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

void OptionsPanel::Unknown6A006A(bool)
{
	NotPorted("OptionsPanel 0x6a006a");
}

OptionsPanel::OptionsPanel(WinTwistApp*)
{
	NotPorted("OptionsPanel");
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
