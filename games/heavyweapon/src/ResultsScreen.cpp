#include "ResultsScreen.h"
#include "Board.h"
#include "Compat2004.h"
#include "CutsceneScreen.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameText.h"
#include "DDImage.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "Res.h"
#include "Sprite.h"
#include "WidgetManager.h"

#include <stdio.h>

using namespace Sexy;

namespace
{
	const int FADE_MAX = 128;
	const int FADE_STEP = 4;
	const double PANEL_ALPHA_SCALE = 1.75;		// the right panel darkens 1.75 times as fast
	const int MAX_TIME = 10000;
	const int LINE_TIME = 50;					// a line appears every 50 updates
	const int BONUS_STEPS = 150;				// a bonus counts up in 150 steps of an update
	const int REINFORCEMENT_TIME_1 = 400;		// the lost tanks appear at these two times
	const int REINFORCEMENT_TIME_2 = 450;
	const int LIST_WIDTH = 480;					// the list's side; the right panel is the rest
	const int VALUE_RIGHT = 460;				// the values' right edge
	const int LABEL_X = 20;
	const int RANK_COUNT = 20;
	const int PROMOTION_BLINK_PERIOD = 80;
	const int PROMOTION_BLINK_OFF = 31;			// "promotion!" is hidden for the first 31 updates of every 80
	const int TOTAL_DESTRUCTION = 100;			// the kill percentages of the destruction bonuses
	const int MASSIVE_DESTRUCTION = 95;
	const int MAJOR_DESTRUCTION = 90;
	const int ADVANCE_X = 495;
	const int ADVANCE_Y = 420;
	const int ADVANCE_WIDTH = 135;
	const int ADVANCE_HEIGHT = 45;
	const double ADVANCE_FADE_SPEED = 0.05;
	const int CUTSCENE_LEVEL = 8;				// after the 9th level comes the middle cutscene

	// The rank names, one a level (0x52e181, 25 bytes apiece)
	const unsigned int RANK_NAMES[RANK_COUNT] =
	{
		GTA(0x52e181), GTA(0x52e19a), GTA(0x52e1b3), GTA(0x52e1cc), GTA(0x52e1e5),
		GTA(0x52e1fe), GTA(0x52e217), GTA(0x52e230), GTA(0x52e249), GTA(0x52e262),
		GTA(0x52e27b), GTA(0x52e294), GTA(0x52e2ad), GTA(0x52e2c6), GTA(0x52e2df),
		GTA(0x52e2f8), GTA(0x52e311), GTA(0x52e32a), GTA(0x52e343), GTA(0x52e35c)
	};

	// The destruction bonus for a kill percentage, (level + 1) thousands
	int DestructionBonus(int theKillPercentage, int theLevel)
	{
		if (theKillPercentage == TOTAL_DESTRUCTION)
			return (theLevel + 1) * 10000;
		else if (theKillPercentage < MASSIVE_DESTRUCTION)
		{
			if (theKillPercentage < MAJOR_DESTRUCTION)
				return 0;
			return (theLevel + 1) * 1000;
		}
		return (theLevel + 1) * 5000;
	}

	// One line of the list: its label at the left, its value right-aligned
	void DrawLine(Graphics* g, const SexyString& theLabel, const std::string& theValue, int theY)
	{
		g->DrawString(theLabel, LABEL_X, theY);
		g->DrawString(theValue, VALUE_RIGHT - g->GetFont()->StringWidth(theValue), theY);
	}

	std::string IntText(int theValue)
	{
		char aText[32];
		sprintf(aText, "%i", theValue);
		return aText;
	}

	// A bonus line's value: NONE when there was no bonus
	std::string BonusText(int theShown, int theBonus)
	{
		if (theBonus < 1)
			return GT(0x52e5a8);
		return IntText(theShown);
	}
}

// 0x426e60: the bonuses go into the score now; the campaign is saved as at the next level's start
ResultsScreen::ResultsScreen(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mBoard = theApp->mBoard;
	mFade = 0;
	mTime = 0;
	mKillPercentage = KillPercentage(mBoard->mEnemiesDestroyed, mBoard->mEnemiesSpawned);
	mDestructionBonusShown = 0;
	mDestructionBonus = DestructionBonus(mKillPercentage, theApp->mLevel);
	mFriendlyFireBonusShown = 0;
	mFriendlyFireBonus = mBoard->mFriendliesDestroyed == 0 ? (theApp->mLevel + 1) * 5000 : 0;
	mSurvivalBonusShown = 0;
	mSurvivalBonus = theApp->mLives == HeavyWeaponApp::CAMPAIGN_LIVES - 1 ? (theApp->mLevel + 1) * 5000 : 0;
	mReinforcementsShown = 0;
	mReinforcements = HeavyWeaponApp::CAMPAIGN_LIVES - 1 - theApp->mLives;
	mScoreShown = theApp->mScore;
	theApp->mScore += mSurvivalBonus + mFriendlyFireBonus + mDestructionBonus;

	mAdvanceButton = new DialogButton(NULL, 0, this);
	mAdvanceButton->mButtonImage = mApp->mSprites[IMAGE_ADVANCEBTN]->mImage;
	mAdvanceButton->mOverRect = Rect(135, 0, 130, 40);
	mAdvanceButton->mDownRect = Rect(270, 0, 135, 45);
	mAdvanceButton->mOverAlphaSpeed = ADVANCE_FADE_SPEED;
	mAdvanceButton->mOverAlphaFadeInSpeed = ADVANCE_FADE_SPEED;
	mApp->mMusicInterface->PlayMusic(SONG_ATOMIC_TANK_73, 0x49, false);
	mApp->SetCursorImage(0, NULL);

	// the save: the bonuses are added a second time, as is the next level and a full set of tanks, and taken back
	// after (so a campaign resumed from the save has the bonuses twice)
	Board* aBoard = mBoard;
	int aBonus = DestructionBonus(KillPercentage(aBoard->mEnemiesDestroyed, aBoard->mEnemiesSpawned), mApp->mLevel);
	int aFriendlyFireBonus = aBoard->mFriendliesDestroyed == 0 ? (mApp->mLevel + 1) * 5000 : 0;
	int aSurvivalBonus = mApp->mLives == HeavyWeaponApp::CAMPAIGN_LIVES - 1 ? (mApp->mLevel + 1) * 5000 : 0;
	aBonus = aSurvivalBonus + aFriendlyFireBonus + aBonus;
	mApp->mScore += aBonus;
	int aLevel = mApp->mLevel;
	if (aLevel < HeavyWeaponApp::FINAL_LEVEL)
		mApp->mLevel = aLevel + 1;
	int aLives = mApp->mLives;
	mApp->mLives = HeavyWeaponApp::CAMPAIGN_LIVES;
	mApp->SaveUserProgress();
	mApp->CurrentUser().mMegaLaserParts = (unsigned short) aBoard->mMegaLaserPartsCaught;
	mApp->SaveUsers();
	mApp->mLives = aLives;
	mApp->mScore -= aBonus;
	if (aLevel < HeavyWeaponApp::FINAL_LEVEL)
		mApp->mLevel--;

	Widget::Resize(0, 0, 640, 480);
	mAdvanceButton->Resize(ADVANCE_X, ADVANCE_Y, ADVANCE_WIDTH, ADVANCE_HEIGHT);
}

// 0x426af0 (deleting 0x426c50)
ResultsScreen::~ResultsScreen()
{
	delete mAdvanceButton;
}

// 0x426b50 slot 1
void ResultsScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mAdvanceButton);
	mApp->PlaySample(SOUND_V_LEVELCOMPLETE, 0);
}

// 0x426b90 slot 2
void ResultsScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mAdvanceButton);
}

// 0x426bc0 slot 13
void ResultsScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mAdvanceButton->Resize(ADVANCE_X, ADVANCE_Y, ADVANCE_WIDTH, ADVANCE_HEIGHT);
}

// 0x426c00 slot 31: a click skips to the end of the current line's 50 updates
void ResultsScreen::MouseUp(int x, int y)
{
	mTime = mTime / LINE_TIME * LINE_TIME + (LINE_TIME - 1);
}

// 0x426c30 slot 47: the board under it is redrawn too
void ResultsScreen::MarkDirty()
{
	Widget::MarkDirty();
	mBoard->MarkDirty();
}

// 0x426c80 slot 17
void ResultsScreen::Update()
{
	Widget::Update();
	if (mFade < FADE_MAX)
		mFade += FADE_STEP;
	if (mTime < MAX_TIME)
		mTime++;
	if (mApp->mScore < mScoreShown)
		mScoreShown = mApp->mScore;

	// a tick for each line as it appears (one more when there is a destruction bonus)
	int aTime = mTime;
	if ((double)aTime * 0.02 == (double)(aTime / LINE_TIME) && LINE_TIME < aTime &&
		aTime < ((mDestructionBonus > 0) + 8) * LINE_TIME)
		mApp->PlaySample(SOUND_STATS, 0);

	if (mTime > 250)
	{
		int aBonus = mFriendlyFireBonus;
		if (mFriendlyFireBonusShown < aBonus)
		{
			mFriendlyFireBonusShown += aBonus / BONUS_STEPS;
			if (aBonus < mFriendlyFireBonusShown)
				mFriendlyFireBonusShown = aBonus;
			mScoreShown += aBonus / BONUS_STEPS;
			mApp->PlaySample(SOUND_STATBEEP, 0);
		}
	}
	if (mTime > 300)
	{
		int aBonus = mSurvivalBonus;
		if (mSurvivalBonusShown < aBonus)
		{
			mSurvivalBonusShown += aBonus / BONUS_STEPS;
			if (aBonus < mSurvivalBonusShown)
				mSurvivalBonusShown = aBonus;
			mScoreShown += aBonus / BONUS_STEPS;
			mApp->PlaySample(SOUND_STATBEEP, 0);
		}
	}
	if ((mTime == REINFORCEMENT_TIME_1 || mTime == REINFORCEMENT_TIME_2) && mReinforcementsShown < mReinforcements)
	{
		mReinforcementsShown++;
		mApp->PlaySample(SOUND_REINFORCEMENTS, 0);
	}
	if (mTime > 400)
	{
		int aBonus = mDestructionBonus;
		if (mDestructionBonusShown < aBonus && aBonus > 0)
		{
			mDestructionBonusShown += aBonus / BONUS_STEPS;
			if (aBonus < mDestructionBonusShown)
				mDestructionBonusShown = aBonus;
			mScoreShown += aBonus / BONUS_STEPS;
			mApp->PlaySample(SOUND_STATBEEP, 0);
		}
	}
	MarkDirty();
}

// 0x427460 slot 16
void ResultsScreen::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, mFade));
	g->FillRect(0, 0, LIST_WIDTH, 480);
	g->SetColor(Color(0x80, 0x80, 0x80, (int)((double)mFade * PANEL_ALPHA_SCALE)));
	g->FillRect(LIST_WIDTH, 0, 640 - LIST_WIDTH, 480);
	g->SetColor(Color(0, 0, 0));
	g->FillRect(LIST_WIDTH - 1, 0, 3, 480);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawLine(LIST_WIDTH, 0, LIST_WIDTH, 479);
	g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
	g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET42]);
	g->Translate(-80, 0);
	WriteCenteredLine(g, 100, GT(0x52e620));
	g->Translate(80, 0);

	// the rank: the furthest level reached (as just saved), one less while the level played is behind it
	UserRecord& aUser = mApp->CurrentUser();
	int aRank = aUser.mMaxLevel;
	if (aRank > RANK_COUNT - 1)
		aRank = RANK_COUNT - 1;
	if (mApp->mLevel < aRank)
		aRank--;
	mApp->mSprites[IMAGE_LARGEINSIGNIA]->Draw(g, 560, 200, true, aRank, 0, false);
	g->Translate(240, 0);
	if ((unsigned int) mApp->mLevel == (unsigned int) aUser.mMaxLevel && aUser.mMaxLevel < RANK_COUNT)
	{
		if (mApp->mUpdateCount % PROMOTION_BLINK_PERIOD >= PROMOTION_BLINK_OFF)
		{
			g->SetFont(mApp->mFonts[FONT_OUTLINED]);
			g->SetColor(Color(0xff, 0xff, 0xff));
			WriteCenteredLine(g, 100, GT(0x52e614));
		}
	}
	else
	{
		g->SetFont(mApp->mFonts[FONT_OUTLINED]);
		g->SetColor(Color(0xff, 0xff, 0xff));
		WriteCenteredLine(g, 100, GT(0x52e608));
	}
	g->Translate(-240, 0);
	g->SetFont(mApp->mFonts[FONT_OUTLINED]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	WriteWordWrapped2004(g, Rect(500, 300, 120, 100), GT(RANK_NAMES[aRank]),
		mApp->mFonts[FONT_OUTLINED]->GetLineSpacing(), 0);

	// the list, a line every 50 updates
	g->SetFont(mApp->mFonts[FONT_RUBBERSTAMPLET20]);
	if (mTime > 100)
		DrawLine(g, GT(0x52e5f8), IntText(mScoreShown), 150);
	if (mTime > 150)
		DrawLine(g, GT(0x52e5e4), IntText(mBoard->mEnemiesDestroyed), 195);
	if (mTime > 200)
	{
		char aText[32];
		sprintf(aText, "%i%%", mKillPercentage);
		DrawLine(g, GT(0x52e5d0), aText, 225);
	}
	if (mTime > 250)
		DrawLine(g, GT(0x52e5b0), BonusText(mFriendlyFireBonusShown, mFriendlyFireBonus), 255);
	if (mTime > 300)
		DrawLine(g, GT(0x52e598), BonusText(mSurvivalBonusShown, mSurvivalBonus), 285);
	if (mTime > 350)
	{
		if (mReinforcements < 1)
			DrawLine(g, GT(0x52e588), GT(0x52e5a8), 315);
		else
		{
			g->DrawString(GT(0x52e588), LABEL_X, 315);
			int x = 430;
			for (int i = 0; i < mReinforcementsShown; i++)
			{
				mApp->mSprites[IMAGE_REINFORCEMENT]->Draw(g, x, 295, false, 0, 0, false);
				x -= 35;
			}
		}
	}
	if (mDestructionBonus > 0 && mTime > 400)
	{
		if (mKillPercentage == TOTAL_DESTRUCTION)
			g->DrawString(GT(0x52e574), LABEL_X, 345);
		else if (mKillPercentage < MASSIVE_DESTRUCTION)
		{
			if (mKillPercentage >= MAJOR_DESTRUCTION)
				g->DrawString(GT(0x52e548), LABEL_X, 345);
		}
		else
			g->DrawString(GT(0x52e55c), LABEL_X, 345);
		std::string aValue = IntText(mDestructionBonusShown);
		g->DrawString(aValue, VALUE_RIGHT - g->GetFont()->StringWidth(aValue), 345);
	}
}

// 0x410670 (listener slot 4; the board's code, which reads the same mApp)
void ResultsScreen::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONDOWN, 0);
}

// 0x427260 (listener slot 5): ADVANCE; the board goes, and with it this screen
void ResultsScreen::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	int aLevel = mApp->mLevel;
	if (aLevel == CUTSCENE_LEVEL)
	{
		CutsceneScreen* aCutscene = new CutsceneScreen(mApp, false);
		mApp->mWidgetManager->AddWidget(aCutscene);
		mApp->mWidgetManager->SetFocus(aCutscene);
		mApp->mWidgetManager->RemoveWidget(mApp->mBoard);
		mApp->SafeDeleteWidget(mApp->mBoard);
		mApp->mBoard = NULL;
		mApp->mLevel++;
		mApp->mLives = HeavyWeaponApp::CAMPAIGN_LIVES;
	}
	else if (aLevel == HeavyWeaponApp::FINAL_LEVEL)
	{
		// the campaign is over: the player's record no longer has one to continue
		CutsceneScreen* aCutscene = new CutsceneScreen(mApp, true);
		mApp->mWidgetManager->AddWidget(aCutscene);
		mApp->mWidgetManager->SetFocus(aCutscene);
		mApp->mWidgetManager->RemoveWidget(mApp->mBoard);
		mApp->SafeDeleteWidget(mApp->mBoard);
		mApp->mBoard = NULL;
		mApp->CurrentUser().mLevel = 0;
		mApp->CurrentUser().mScore = 0;
		mApp->SaveUsers();
	}
	else
	{
		mApp->mLevel = aLevel + 1;
		mApp->mLives = HeavyWeaponApp::CAMPAIGN_LIVES;
		mApp->ShowArmory();
	}
	mApp->mWidgetManager->RemoveWidget(this);
	mApp->SafeDeleteWidget(this);
}
