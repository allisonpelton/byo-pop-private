#include "ChallengeScreen.h"

#include <cmath>
#include <cstdio>

#include "Application.h"
#include "Avatar.h"
#include "ChallengeMeter.h"
#include "Common.h"
#include "Creature.h"
#include "Cubby.h"
#include "Dialog.h"
#include "DrawUtil.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Image.h"
#include "Insets.h"
#include "NcButton.h"
#include "Res.h"
#include "Screens.h"
#include "Stats.h"
#include "ValueList.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

const double PI_HALF = 1.5707963267948966;

// The seconds-ish on the countdown at which each part of it starts: the level's number is shown from 4 down to 2.6667,
// "Ready?" from there to 0.5714, "Go!" below that, and the round itself runs when the count is under 0.5
const double COUNTDOWN_START = 5.0;
const double COUNTDOWN_LEVEL_TEXT_END = 2.6666666666666665;
const double COUNTDOWN_READY_END = 0.5714285714285714;
const double COUNTDOWN_PLAY = 0.5;

// 0x592048: the order in which the sixteen cubbies send up their gnome, one row for each of the eleven shuffled numbers of
// the game's screen (the cubbies are numbered along the rows of the grid, left to right, top to bottom)
const int CUBBY_ORDERS[GameScreen::NUM_SHUFFLED_BOARDS][ChallengeScreen::NUM_COLUMNS * ChallengeScreen::NUM_ROWS] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0 },
	{ 0, 1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12 },
	{ 15, 11, 7, 3, 2, 6, 10, 14, 13, 9, 5, 1, 0, 4, 8, 12 },
	{ 0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9 },
	{ 5, 6, 10, 9, 8, 4, 0, 1, 2, 3, 7, 11, 15, 14, 13, 12 },
	{ 12, 8, 13, 14, 9, 4, 0, 5, 10, 15, 11, 6, 1, 2, 7, 3 },
	{ 0, 4, 5, 1, 2, 6, 3, 7, 11, 15, 14, 10, 9, 13, 12, 8 },
	{ 0, 5, 4, 1, 6, 3, 2, 7, 10, 15, 11, 14, 9, 12, 13, 8 },
	{ 0, 4, 8, 12, 13, 14, 15, 11, 10, 9, 5, 1, 2, 6, 7, 3 },
	{ 15, 10, 7, 2, 3, 6, 11, 14, 13, 8, 5, 0, 1, 4, 9, 12 }
};

// 0x44cb20: none of the boxes that stop the round is up (the bonus round's rules, the game menu, the leave box, the "Ready?"
// box and the box that closes the round)
bool IsNoDialogOpen()
{
	if (gApp->GetDialog(DIALOG_BONUS_ROUND) != nullptr)
		return false;
	if (gApp->GetDialog(DIALOG_GAME_MENU) != nullptr)
		return false;
	if (gApp->GetDialog(DIALOG_LEAVE_GAME) != nullptr)
		return false;
	if (gApp->GetDialog(DIALOG_READY_GO) != nullptr)
		return false;
	if (gApp->GetDialog(DIALOG_CHALLENGE_FINISHED) != nullptr)
		return false;
	return true;
}

// 0x41d210: the look and size of a box that the application did not make through NewDialog: the fonts and colours of the other
// boxes, the buttons' cels, and the width the text needs (at least theMinWidth). The original also puts the box at the top of
// the screen, below another box that is up; the caller moves it at once, so that part is left out.
void LayoutDialog(Dialog* theDialog, int theMinWidth)
{
	theDialog->SetHeaderFont(FONT_PTBANANASPLIT27);
	theDialog->SetLinesFont(FONT_EUROFURENCE14);
	theDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	theDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0, 255));

	int aWidth = theMinWidth;
	int aNeededWidth = FONT_ARIAL10B->StringWidth(theDialog->mDialogHeader) + theDialog->mContentInsets.mRight +
		theDialog->mContentInsets.mLeft + theDialog->mBackgroundInsets.mRight + 0x14 + theDialog->mBackgroundInsets.mLeft;
	if (aWidth < aNeededWidth)
		aWidth = aNeededWidth;

	theDialog->Resize(0, 0, aWidth, theDialog->GetPreferredHeight(aWidth));
	if (theDialog->mYesButton != nullptr)
		SetupButtonCels(theDialog->mYesButton, 3);
	if (theDialog->mNoButton != nullptr)
		SetupButtonCels(theDialog->mNoButton, 3);
}

// What the closing box reads of the round: the gnomes dealt with, the goal, and the clicks that missed
struct RoundResult
{
	int				mCount;
	int				mGoal;
	int				mMisses;
	bool			mPerfect;		// every gnome, and no miss
	int				mCoins;			// three for each gnome and fifty-two more for a perfect round
};

RoundResult GetRoundResult()
{
	RoundResult aResult;
	aResult.mCount = GetStats()->GetInt(STAT_CHALLENGE_COUNT);
	aResult.mGoal = GetStats()->GetInt(STAT_CHALLENGE_GOAL);
	aResult.mMisses = GetStats()->GetInt(STAT_CHALLENGE_MISSES);
	aResult.mPerfect = aResult.mCount == aResult.mGoal && aResult.mMisses == 0;
	aResult.mCoins = aResult.mCount * 3;
	if (aResult.mPerfect)
		aResult.mCoins = aResult.mCoins + 0x34;
	return aResult;
}

}

//////////////////////////////////////////////////////////////////////////
// The box that closes the round

// 0x44c170 (the destructor), 0x44c310 (the constructor): class Sexy::ChallengeFinishedDialog : public NcMovingDialog (0x168 bytes)
class ChallengeFinishedDialog : public NcMovingDialog
{
public:
	ChallengeFinishedDialog();

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x44c190
	virtual void	Update() override;												// 0x44c1f0
	virtual void	Draw(Graphics* g) override;										// 0x44c500
	virtual int		GetPreferredHeight(int theWidth) override;						// 0x44c2e0
	virtual void	ButtonDepress(int theId) override;								// 0x44c290
	virtual void	Closed() override;												// 0x44c2c0
};

// 0x44c310
ChallengeFinishedDialog::ChallengeFinishedDialog() :
	NcMovingDialog(0x15e, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_CHALLENGE_FINISHED, true, "", "", "Return To Game",
		Dialog::BUTTONS_FOOTER)
{
	SetButtonFont(FONT_PTBANANASPLIT27BUTTONS);
	mContentInsets = Insets(0x3c, 0x32, 0x3c, 0x32);

	RoundResult aResult = GetRoundResult();
	gApp->PlaySample(aResult.mPerfect ? SOUND_VOICE_BONUS_PERFECT : SOUND_VOICE_BONUS_WELLDONE);
}

// 0x44c190: the button is as wide as the box's picture of it, in the middle
void ChallengeFinishedDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);
	mYesButton->Resize(theWidth / 2 - 0xaf + theX, mYesButton->mY, 0x15e, mYesButton->mHeight);
}

// 0x44c1f0: every 25 updates the coins counted so far (one for each 5 updates) make the sound of a coin, until all are counted
void ChallengeFinishedDialog::Update()
{
	Widget::Update();
	if (mUpdateCnt % 25 == 0)
	{
		RoundResult aResult = GetRoundResult();
		if (mUpdateCnt / 5 < aResult.mCoins)
		{
			if (gApp->mTrophyRoom == nullptr)
				gApp->PlaySample(SOUND_COIN_THREE_PICKED);
		}
	}
}

// 0x44c2e0
int ChallengeFinishedDialog::GetPreferredHeight(int theWidth)
{
	return 0x15e;
}

// 0x44c500
void ChallengeFinishedDialog::Draw(Graphics* g)
{
	RoundResult aResult = GetRoundResult();
	int aCoins = aResult.mCoins;

	Dialog::Draw(g);
	Graphics aG(*g);

	aG.SetColor(Color(255, 255, 255, 255));
	aG.SetFont(FONT_PTBANANASPLIT27);
	int aCenterX = mWidth / 2;
	std::string aTitle = "Bonus Round Completed";
	aG.DrawString(aTitle, aCenterX - aG.StringWidth(aTitle) / 2, 0x41);

	aG.SetColor(Color(0, 0, 0, 255));
	aG.SetFont(FONT_EUROFURENCE14BOLD);
	int aLineY = aG.GetFont()->GetHeight() + 0x5a;
	int aLineHeights = 2;				// how many lines of the font the coin row stays below the verdict
	if (aResult.mCount < aResult.mGoal || aResult.mMisses > 0)
		aLineHeights = 3;

	std::string aVerdict;
	int aVerdictY = aLineY;
	if (aCoins == 0)
	{
		aVerdict = "Better luck next time!";
		aVerdictY = aLineY + 0x32;
	}
	else if (aCoins < 0x16)
		aVerdict = "Not bad! Practice makes perfect...";
	else
		aVerdict = "Well played!";
	aG.DrawString(aVerdict, aCenterX - aG.StringWidth(aVerdict) / 2, aVerdictY);

	// the perfect round's line pulses (its alpha is a sine of the age, a period of about 99 updates)
	int aPulse = ToInt((sin((double)(mUpdateCnt % 100) * (double)0.06346651911735535f) + 1.0) * 127.5);
	int aCoinRowOffset;
	if (aResult.mPerfect)
	{
		aG.SetColor(Color(255, 255, 255, aPulse));
		aG.SetFont(FONT_EUROFURENCE14BOLD);
		std::string aPerfect = "PERFECT! Coin bonanza!";
		int aPerfectY = aLineY + aG.GetFont()->GetHeight() * aLineHeights;
		aG.DrawString(aPerfect, aCenterX - aG.StringWidth(aPerfect) / 2, aPerfectY);
		aCoinRowOffset = 0;
	}
	else
		aCoinRowOffset = -25;

	// the coins won so far, in a row that overlaps by all but six pixels, and the number of them
	Rect aCoinCel = IMAGE_COIN->GetCelRect(0, 0);
	int aRowStartX = (mWidth - (IMAGE_COIN->GetCelWidth() + aCoins * 6 - 6)) / 2;
	int aShown = 0;
	int aCoinX = aRowStartX;
	if (aCoins > 0)
	{
		while (aShown < aCoins && aShown < mUpdateCnt / 5)
		{
			aG.DrawImage(IMAGE_COIN, aCoinX, aCoinRowOffset + 0xaa, aCoinCel);
			aShown++;
			aCoinX += 6;
		}

		aG.SetFont(FONT_KIDPRINT27BOLD);
		aG.SetColor(Color(0x96, 0x5a, 0));
		char aBuffer[32];
		snprintf(aBuffer, sizeof(aBuffer), "%d", aShown);
		std::string aNumber = aBuffer;
		aG.DrawString(aNumber, ((IMAGE_COIN->GetCelWidth() / 2 + aShown * 6) - aG.StringWidth(aNumber) / 2) - 5 + aRowStartX,
			aCoinRowOffset + 0xda);
	}
}

// 0x44c290: the only button takes the player back to the game
void ChallengeFinishedDialog::ButtonDepress(int theId)
{
	if (theId == Dialog::ID_YES)
	{
		CloseWith(Dialog::ID_YES);
		gApp->LeaveChallenge();
	}
}

// 0x44c2c0: the box has gone up and out of sight
void ChallengeFinishedDialog::Closed()
{
	if (mCloseResult == Dialog::ID_YES)
		gApp->KillDialog(DIALOG_CHALLENGE_FINISHED);
}

// 0x44c9e0 (0x41f0f0 forwards to it)
Dialog* Sexy::MakeChallengeFinishedDialog()
{
	ChallengeFinishedDialog* aDialog = new ChallengeFinishedDialog();
	LayoutDialog(aDialog, 0x2ee);
	aDialog->Resize((gApp->mWidth - aDialog->mWidth) / 2, -aDialog->mHeight, aDialog->mWidth, aDialog->mHeight);
	gApp->AddDialog(DIALOG_CHALLENGE_FINISHED, aDialog);

	RoundResult aResult = GetRoundResult();
	GetStats()->AddIntQuiet(STAT_COIN_BALANCE, aResult.mCoins);
	if (aResult.mPerfect)
		GetStats()->AddInt(STAT_COINS_COLLECTED, 0x34);
	return aDialog;
}

//////////////////////////////////////////////////////////////////////////
// The bonus round

// 0x44d800
ChallengeScreen::ChallengeScreen() :
	mFade(1.0),
	mFinished(false),
	mReadySaid(false),
	mGoSaid(false),
	mRunning(true),
	mCountdown(COUNTDOWN_START),
	mClicksCleared(false)
{
	mMenuButton = MakeNcButton(1, this, "", IMAGE_INGAMEUI_MENUBUTTON, 3);
	mMenuButton->mDarkensWithFade = true;
	GetAvatar()->mOverTarget = false;
}

// 0x44d290
ChallengeScreen::~ChallengeScreen()
{
	delete mMenuButton;
}

// 0x44cd40
int ChallengeScreen::GetCreatureKind()
{
	return KIND_BIG_COIN;
}

// 0x44cd30
Image* ChallengeScreen::GetBackdrop()
{
	return IMAGE_CHALLENGE_BACKDROP;
}

// 0x44d360 (vfd8): the level's objects are put aside and the round's own are made: the meter, and a grid of sixteen cubbies
void ChallengeScreen::Opened(bool theLoaded)
{
	ObjectManager::Get()->DeactivateAll();
	if (!gApp->IsPaused())
		gApp->PauseGame(true);

	MakeChallengeMeter();
	GetStats()->SetInt(STAT_CHALLENGE_COUNT, 0);
	GetStats()->SetInt(STAT_CHALLENGE_MISSES, 0);
	if (!theLoaded)
	{
		if (gApp->mGameScreen->mBonusRoundsDone == 0)
			gApp->DoBonusRoundDialog();
	}
	GetAvatar()->mActive = true;

	// each row of the grid is shifted to the right of the one above it; the cubbies are named from 90001 on
	int aNameNumber = 90001;
	for (int aRow = 0, aY = 0xcc; aY < 0x21c; aRow++, aY += 0x54)
	{
		for (int aColumn = 0; aColumn < NUM_COLUMNS; aColumn++)
		{
			std::string aName = MakeCubbyName(aNameNumber);
			AddCubby(ToInt(aColumn * 159.5 + aRow * 27.5 + 56.0), aY, aName);
			aNameNumber++;
		}
	}

	// the gnomes come up in an order that depends on how many rounds have been played, faster at each round (down to the
	// eighth)
	int aRoundsDone = gApp->mGameScreen->mBonusRoundsDone;
	int aSpeedUp = aRoundsDone < 8 ? aRoundsDone : 8;
	double aGap = 0.6 - aSpeedUp * 0.04;
	GameScreen* aGameScreen = gApp->mGameScreen;
	const int* anOrder = CUBBY_ORDERS[aGameScreen->mShuffledBoards[aGameScreen->mBonusRoundsDone % GameScreen::NUM_SHUFFLED_BOARDS]];
	for (int i = 0; i < (int)mCubbies.size(); i++)
		GetCubby(anOrder[i])->mTimer = i * aGap;
}

// 0x44ccf0 (vfdc)
void ChallengeScreen::Closing()
{
	GetAvatar()->mActive = false;
	ObjectManager::Get()->RemoveByActive(true);
	ObjectManager::Get()->ActivateAll();
	gApp->PauseGame(false);
}

// 0x44cd80
void ChallengeScreen::StartChallenge()
{
	mRunning = true;
	mMenuButton->SetDisabled(false);
	gApp->mGameScreen->ShowCrosshair();
	gApp->mFocusedUpdate = mUpdateCnt;
}

// 0x44cc40
void ChallengeScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
}

// 0x44cc70
void ChallengeScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
}

// 0x44cca0
void ChallengeScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMenuButton->Move(gApp->mWidth - mMenuButton->mWidth - 6, theY + 6);
}

// 0x44cd50: the menu button is the only button; the other id leaves the round
void ChallengeScreen::ButtonDepress(int theId)
{
	if (theId == 0)
		gApp->LeaveChallenge();
	else if (theId == 1)
		gApp->ShowGameMenu();
}

// 0x44cdc0: the focus is lost with no box up: the round stops until the next click
void ChallengeScreen::LostFocus()
{
	if (IsNoDialogOpen())
	{
		mRunning = false;
		mMenuButton->SetDisabled(true);
		if (gApp->mGameScreen != nullptr)
			gApp->mGameScreen->UpdateHover();
	}
}

// 0x44cbb0
void ChallengeScreen::MouseEnter()
{
	Widget::MouseEnter();
	if (!mRunning)
	{
		if (IsNoDialogOpen())
		{
			gApp->mGameScreen->UpdateHover();
			return;
		}
	}
	gApp->mGameScreen->ShowCrosshair();
}

// 0x44cc00
void ChallengeScreen::MouseLeave()
{
	Widget::MouseLeave();
	gApp->mGameScreen->UpdateHover();
}

// 0x44ce10: a click ends the pause, or is the hammer's
void ChallengeScreen::MouseDown(int x, int y, int theClickCount)
{
	if (gApp->mFocusedUpdate == mUpdateCnt)
		return;			// the click that gave the screen the focus again is eaten

	Widget::MouseDown(x, y, theClickCount);
	if (!mRunning)
	{
		if (IsNoDialogOpen())
		{
			StartChallenge();
			return;
		}
	}
	Avatar* anAvatar = GetAvatar();
	if (anAvatar != nullptr)
		anAvatar->AddClick(x, y);
}

// 0x44ce90
void ChallengeScreen::UpdateF(float theFrac)
{
	Widget::UpdateF(theFrac);
	MarkDirty();

	// the screen goes dark behind a box, while the zoom wipe runs after the round, and while it waits to be clicked
	bool aDarken = false;
	if (gApp->IsMenuOpen() || gApp->IsReadyGoOpen())
		aDarken = true;
	else if (mCountdown <= 0.0 && gApp->mZooming)
		aDarken = true;
	else if (gApp->IsBonusRoundOpen())
		aDarken = true;
	else if (!mRunning && IsNoDialogOpen())
		aDarken = true;

	if (!aDarken)
	{
		if (mFade > 0.0)
		{
			mFade = mFade - theFrac * 0.02;
			if (mFade < 0.0)
				mFade = 0.0;
		}

		if (mCountdown > 0.0)
		{
			mCountdown = mCountdown - theFrac * 0.01;
			if (mCountdown <= 0.0)
				mCountdown = 0.0;

			if (mCountdown <= COUNTDOWN_LEVEL_TEXT_END && mCountdown > COUNTDOWN_READY_END && !mReadySaid)
			{
				gApp->PlaySample(SOUND_VOICE_BONUS_READY);
				mReadySaid = true;
			}
			else if (mCountdown <= COUNTDOWN_READY_END && !mGoSaid)
			{
				gApp->PlaySample(SOUND_VOICE_BONUS_GO);
				mGoSaid = true;
			}
		}

		if (mCountdown < COUNTDOWN_PLAY)
		{
			if (!mClicksCleared)
			{
				GetAvatar()->ClearClicks();
				mClicksCleared = true;
			}
			ObjectManager::Get()->Update(theFrac);

			// each cubby that has not sent a gnome up waits out its time and then does; the ones that have had theirs and are
			// empty again are counted
			int aDoneCount = 0;
			for (int i = 0; i < (int)mCubbies.size(); i++)
			{
				Cubby* aCubby = GetCubby(i);
				if (aCubby->mState == Cubby::STATE_EMPTY)
				{
					double aStep = theFrac * 0.01;
					double aTimer = aCubby->mTimer - aStep;
					if (aTimer <= 0.0)
						aTimer = 0.0;
					aCubby->mTimer = aTimer;
					if (aTimer == 0.0 && aCubby->GetCreatureCount() == 0)
						aCubby->SetState(Cubby::STATE_RISING, -1);
					if (aCubby->GetCreatureCount() > 0)
						aDoneCount++;
				}
			}

			if (!mFinished && aDoneCount == GetStats()->GetInt(STAT_CHALLENGE_GOAL))
			{
				mFinished = true;
				MakeChallengeFinishedDialog();
				gApp->mGameScreen->AddBonusRoundDone();
				RoundResult aResult = GetRoundResult();
				if (aResult.mPerfect)
					GetStats()->AddInt(STAT_PERFECT_BONUS_SCREENS, 1);
				GetStats()->AddIntQuiet(STAT_BONUS_HITS, aResult.mCount);
				GetStats()->AddIntQuiet(STAT_BONUS_MISSES, aResult.mMisses);
			}
		}
	}
	else
	{
		if (mFade < 1.0)
		{
			mFade = theFrac * 0.02 + mFade;
			if (mFade > 1.0)
				mFade = 1.0;
		}
	}
	gApp->mFade = mFade;
}

// 0x44d950
void ChallengeScreen::Draw(Graphics* g)
{
	Widget::Draw(g);
	Graphics aG(*g);
	aG.DrawImage(IMAGE_CHALLENGE_BACKDROP, 0, 0);
	ObjectManager::Get()->Draw(&aG);

	if (mFade > 0.0)
	{
		int anAlpha = ToInt(sin(mFade * PI_HALF) * 128.0);
		aG.SetColor(Color(0, 0, 0, anAlpha));
		aG.FillRect(0, 0, mWidth, mHeight);
	}

	// the count before the round: the number of the round, "Ready?" and "Go!", each growing in and shrinking out
	if (mCountdown > 0.0 && mCountdown <= 4.0)
	{
		aG.SetColor(Color(255, 255, 255, 255));
		aG.SetFont(FONT_PTBANANASPLIT27);
		std::string aText;
		double aProgress = 0.0;
		bool aHaveText = true;
		if (mCountdown > COUNTDOWN_LEVEL_TEXT_END)
		{
			char aBuffer[32];
			snprintf(aBuffer, sizeof(aBuffer), "%d", gApp->mGameScreen->mBonusRoundsDone + 1);
			aText = std::string(aBuffer) + " Bonus Round!";
			aProgress = (mCountdown - COUNTDOWN_LEVEL_TEXT_END) * 0.375;
		}
		else if (mCountdown > COUNTDOWN_READY_END)
		{
			aText = "Ready?";
			aProgress = (mCountdown - COUNTDOWN_READY_END) * 0.47727272727272735;
		}
		else
		{
			aText = "Go!";
			aProgress = mCountdown * 1.75;
		}

		double aT = 1.0 - aProgress;
		double aScale;
		if (aT >= 0.2)
		{
			if (aT <= 0.8)
				aScale = 1.0;
			else
				aScale = sin((1.0 - (aT - 0.8) * 5.0) * 0.7853981852531433);
		}
		else
			aScale = sin(aT * 3.9269909262657166);

		int aTextWidth = aG.GetFont()->StringWidth(aText);
		aG.SetColorizeImages(true);
		aG.SetColor(Color(255, 255, 255, ToInt(aScale * 255.0)));
		int aTextHeight = aG.GetFont()->GetHeight();
		aG.DrawString(aText, mWidth / 2 - aTextWidth / 2, mHeight / 2 - aTextHeight / 2);
	}

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		aG.SetColorizeImages(false);
		aG.DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
	}

	// the pause: a band across the screen
	if (!mRunning && IsNoDialogOpen() && !gApp->mZooming)
	{
		aG.SetColor(Color(0, 0, 0, 255));
		aG.FillRect(0, 200, mWidth, 0x78);
		aG.SetColorizeImages(false);
		aG.SetFont(FONT_PTBANANASPLIT27);
		aG.SetColor(Color(255, 255, 255, 255));

		std::string aPaused = "Paused";
		aG.DrawString(aPaused, (mWidth - aG.StringWidth(aPaused)) / 2, (mHeight - aG.GetFont()->GetHeight() * 2) / 2);
		std::string aResume = "Click Here To Resume";
		aG.DrawString(aResume, (mWidth - aG.StringWidth(aResume)) / 2, mHeight / 2);
	}
}

// 0x44d5d0 (vfe4): the countdown, the gnomes dealt with, the goal, whether the round is over, whether the closing box is up, and
// the two voices that have played
std::string ChallengeScreen::SaveExtra()
{
	ValueList aList;
	aList.Add(mCountdown);
	aList.AddInt(GetStats()->GetInt(STAT_CHALLENGE_COUNT));
	aList.AddInt(GetStats()->GetInt(STAT_CHALLENGE_GOAL));
	aList.AddBool(mFinished);
	aList.AddBool(gApp->GetDialog(DIALOG_CHALLENGE_FINISHED) != nullptr);
	aList.AddBool(mReadySaid);
	aList.AddBool(mGoSaid);
	return aList.ToString();
}

// 0x44d6e0 (vfe8): a round that had begun starts its count again from 5; a box that was up is made again (and pays again)
void ChallengeScreen::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCountdown = aList.NextDouble();
	if (mCountdown > 0.0)
		mCountdown = COUNTDOWN_START;
	GetStats()->SetInt(STAT_CHALLENGE_COUNT, aList.NextInt());
	GetStats()->SetInt(STAT_CHALLENGE_GOAL, aList.NextInt());
	mFinished = aList.NextBool();
	if (aList.NextBool())
		MakeChallengeFinishedDialog();
	mReadySaid = aList.NextBool();
	mGoSaid = aList.NextBool();
}
