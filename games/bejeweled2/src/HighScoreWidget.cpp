// HighScoreWidget.cpp: HighScoreWidget (HighScoreWidget.h).
#include "HighScoreWidget.h"

#include <math.h>

#include "Board.h"
#include "Buffer.h"
#include "Common.h"
#include "Font.h"
#include "Graphics.h"
#include "MainMenu.h"
#include "MusicInterface.h"
#include "PodDialog.h"
#include "Profile.h"
#include "Res.h"
#include "WidgetManager.h"
#include "WinDMApp.h"

using namespace Sexy;

// the first long of users/hiscores.dat
static const unsigned int HIGH_SCORE_MAGIC = 0xb9e7e3f8;

// the board's state when the game is over (the panel then comes after the game, not from the pause)
static const int BOARD_STATE_GAME_OVER = 7;

// the gems collected that give the next word of the rank (the exe's data at 0x62bb70), and the words
static const int sRankGemCounts[10] = {0, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000};
static const char* sRankAdjectives[10] = {"New", "Junior", "Apprentice", "Journeyman", "Senior", "Expert", "Pro", "Master",
	"Superior", "Legendary"};
static const char* sRankNouns[16] = {"Gemwasher", "Appraiser", "Digger", "Searcher", "Collector", "Geologist", "Gemologist",
	"Jeweler", "Polisher", "Bezelier", "Etcher", "Engraver", "Gemcarver", "Lapidary", "Glyptographer", "Jewelcrafter"};

// the names of the tables as they start, and after a reset: even modes first, then odd modes (the scores are the same)
static const char* sStartNamesEven[10] = {"Ace", "John", "Eric", "Brian", "Ben", "Hans", "Jeff", "Walter", "Dave", "Nick"};
static const char* sStartNamesOdd[10] = {"Josh", "Shawn", "Jason", "Tysen", "Chad", "Don", "Brenna", "Kathy", "Sukhbir", "J.R."};
static const int sStartScores[10] = {2000, 1900, 1800, 1700, 1600, 1000, 900, 800, 700, 600};

// 0x5c530a
HighScoreWidget::HighScoreWidget(WinDMApp* theApp)
{
	mApp = theApp;
	mHueOffset = 0;
	mHasAlpha = true;
	mPulse = 0;
	mClosing = 0;
	ResetHighScores();

	// the button back to where the player came from
	mBackButton = new PodButton(BUTTON_BACK, this);
	mBackButton->mGlowColor = Color(255, 255, 0);
	mBackButton->mGlowImage = IMAGE_HS_BUTTON_ROLLOVER;
	if (mApp->mBoard == nullptr || mApp->mBoard->mState == BOARD_STATE_GAME_OVER)
		mBackButton->mButtonImage = IMAGE_HS_BACK_TO_MENU_TEXT;
	else
		mBackButton->mButtonImage = IMAGE_HS_BACK_TO_GAME_TEXT;

	// the other one: play again, or reset the tables when the panel comes from the menu
	if (mApp->mBoard != nullptr)
	{
		mActionButton = new PodButton(BUTTON_PLAY_AGAIN, this);
		mActionButton->mGlowColor = Color(255, 255, 0);
		mActionButton->mGlowImage = IMAGE_HS_BUTTON_ROLLOVER;
		mActionButton->mButtonImage = IMAGE_HS_PLAY_AGAIN_TEXT;
	}
	else
	{
		mActionButton = new PodButton(BUTTON_RESET, this);
		mActionButton->mGlowColor = Color(255, 255, 0);
		mActionButton->mGlowImage = IMAGE_HS_BUTTON_ROLLOVER;
		mActionButton->mButtonImage = IMAGE_HS_RESET_SCORES_TEXT;
	}

	mSlideDelay = 13;
	mPanelSlide.SetMode(0);
	mPanelSlide.SetRamp(2);
	mPanelSlide.SetOutRange(900.0, 0);
	mButtonSlide.SetMode(0);
	mButtonSlide.SetRamp(2);
	mButtonSlide.SetOutRange(1500.0, 0);
	mTitleSlide.SetMode(0);
	mTitleSlide.SetRamp(2);
	mTitleSlide.SetOutRange(400.0, 0);
	mBottomSlide.SetMode(0);
	mBottomSlide.SetRamp(2);
	mBottomSlide.SetOutRange(600.0, 0);

	if (mApp->mMainMenu != nullptr)
		mApp->mMainMenu->Hide();
	if (mApp->mBoard != nullptr)
		mApp->mBoard->Hide();
	ReadFromFile();

	// the score of a finished game that is not a puzzle goes into the tables, unless that was done already
	Board* aBoard = mApp->mBoard;
	if (aBoard != nullptr && !aBoard->mIsPuzzle && aBoard->mState == BOARD_STATE_GAME_OVER)
		mGameEnded = true;
	else
		mGameEnded = false;
	mShowGame = mGameEnded;
	if (mGameEnded && !mApp->mBoard->mCheated)
		ProcessScoreStuff();

	// the rank: a word for the gems collected, then a title for the levels and the other counters of the profile
	Profile* aProfile = mApp->mProfile;
	int anAdjective = 0;
	for (int i = 0; i < 10; i++)
	{
		if (!(aProfile->mStat[3] < sRankGemCounts[i]))
			anAdjective = i;
	}
	int aNoun = 0;
	if (aProfile->mTotal7C >= 80)
		aNoun = 5;
	else if (aProfile->mTotal7C >= 70)
		aNoun = 4;
	else if (aProfile->mTotal7C >= 40)
		aNoun = 3;
	else if (aProfile->mTotal7C >= 20)
		aNoun = 2;
	else if (aProfile->mTotal7C >= 1)
		aNoun = 1;
	if (aProfile->mModeLevel[3] >= 100)
		aNoun += 3;
	else if (aProfile->mModeLevel[3] >= 50)
		aNoun += 2;
	else if (aProfile->mModeLevel[3] >= 10)
		aNoun += 1;
	if (aProfile->mModeLevel[0] >= 20)
		aNoun += 3;
	else if (aProfile->mModeLevel[0] >= 10)
		aNoun += 2;
	else if (aProfile->mModeLevel[0] >= 5)
		aNoun += 1;
	if (aProfile->mModeLevel[1] >= 12)
		aNoun += 4;
	else if (aProfile->mModeLevel[1] >= 9)
		aNoun += 3;
	else if (aProfile->mModeLevel[1] >= 6)
		aNoun += 2;
	else if (aProfile->mModeLevel[1] >= 3)
		aNoun += 1;
	mRank = sRankAdjectives[anAdjective];
	mRank += " ";
	mRank += sRankNouns[aNoun];
}

// 0x5c5798: the tables are saved on the way out
HighScoreWidget::~HighScoreWidget()
{
	delete mBackButton;
	delete mActionButton;
	WriteToFile();
}

// 0x5c352e
void HighScoreWidget::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);
	theManager->AddWidget(mBackButton);
	theManager->AddWidget(mActionButton);
}

// 0x5c355e
void HighScoreWidget::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);
	theManager->RemoveWidget(mBackButton);
	theManager->RemoveWidget(mActionButton);
}

// 0x5c37d7
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
}

// 0x5c37fe
void HighScoreWidget::ButtonPress(int theId)
{
	if (mClosing != 1)
		mApp->PlaySample(SOUND_CLICK);
}

// 0x5c358e: the string in black, two pixels down and to the right, then in the color it was set to
void HighScoreWidget::DrawShadowString(Graphics* g, const std::string& theString, int theX, int theY)
{
	Color aColor = g->GetColor();
	g->SetColor(Color(0, 0, 0, 200));
	g->DrawString(theString, theX + 2, theY + 2);
	g->SetColor(aColor);
	g->DrawString(theString, theX, theY);
}

// 0x5c35fa
void HighScoreWidget::Update()
{
	if (!mApp->mHasFocus)
		return;

	Widget::Update();
	bool aNewScore = false;
	for (int i = 0; i < NUM_TABLES; i++)
	{
		if (mNewPlace[i] != 0)
			aNewScore = true;
	}
	if (aNewScore)
		mPulse++;
	if (mNewPlace[0] != -1 || mNewPlace[1] != -1)
		mPulse++;

	if (mSlideDelay > 0)
	{
		mSlideDelay--;
		return;
	}

	bool aSlideDone = !mPanelSlide.IncInVal(0.01);
	mButtonSlide.IncInVal(0.01);
	mTitleSlide.IncInVal(0.01);
	mBottomSlide.IncInVal(0.01);
	mBackButton->Resize(mApp->ScaleNum(0x12 - (int)mButtonSlide.GetOutVal(), 0), mApp->ScaleNum(0x1f, 0),
		mApp->ScaleNumUp(0xd8, 0), mApp->ScaleNumUp(0x2d, 0));
	mActionButton->Resize(mApp->ScaleNum((int)mButtonSlide.GetOutVal() + 0x317, 0), mApp->ScaleNum(0x1f, 0),
		mApp->ScaleNumUp(0xd8, 0), mApp->ScaleNumUp(0x2d, 0));
	if (mUpdateCnt % 10 == 0)
	{
		mHueOffset++;
		MarkDirty();
	}
	if (aSlideDone && mClosing == 1)
		mApp->KillHighScoreWidget();
}

// 0x5c3efe
void HighScoreWidget::Draw(Graphics* g)
{
	int aSlide = (int)mPanelSlide.GetOutVal();
	g->DrawImage(IMAGE_HS_TITLE, mApp->ScaleNum(0x18b, 0), mApp->ScaleNum(0x14 - (int)mTitleSlide.GetOutVal(), 0));

	// a secret mode shows its own table; everything else shows the tables of the first two modes
	int aTable = -1;
	if (mApp->mBoard != nullptr && mApp->mGameMode > 3)
		aTable = mApp->mGameMode;

	g->DrawImage(IMAGE_HS_TOP_BUTTON, mApp->ScaleNum(0x10 - (int)mButtonSlide.GetOutVal(), 0), mApp->ScaleNum(0xe, 0));
	g->DrawImage(IMAGE_HS_TOP_BUTTON, mApp->ScaleNum((int)mButtonSlide.GetOutVal() + 0x315, 0), mApp->ScaleNum(0xe, 0));

	if (aTable == -1)
	{
		g->DrawImage(IMAGE_HS_LEFT, mApp->ScaleNum(7 - aSlide, 0), mApp->ScaleNum(0x11, 0));
		g->DrawImage(IMAGE_HS_RIGHT, mApp->ScaleNum(aSlide + 0x21f, 0), mApp->ScaleNum(0x11, 0));
		for (int aPanel = 0; aPanel < 2; aPanel++)
		{
			int aNameX;
			int aScoreX;
			if (aPanel == 0)
			{
				aNameX = 0x26 - aSlide;
				aScoreX = 0x1c1 - aSlide;
			}
			else
			{
				aNameX = aSlide + 0x23e;
				aScoreX = aSlide + 0x3d9;
			}
			int* aScore = mScores[aPanel];
			int aY = 0xd2;
			int i = 0;
			std::string* aName = mNames[aPanel];
			while (true)
			{
				// the place of the new score has a pulsing color, the others the color of their place
				if (mNewPlace[aPanel] == i)
				{
					int aPulse = (int)(cos((double)mPulse * 0.12) * -65.0);
					g->SetColor(Color(mApp->HSLToRGB(((mHueOffset + i + 2) * 20) % 256, 0xec, 0xbe - aPulse)));
				}
				else
					g->SetColor(Color(mApp->HSLToRGB(((mHueOffset + i + 2) * 20) % 256, 0xdc, 0xee)));

				std::string aScoreString = CommaSeperate(*aScore);
				int aScoreWidth = FONT_CONT_MED->StringWidth(aScoreString);
				std::string aNameString = *aName;
				int aMaxWidth = mApp->ScaleNum(0x113, 0);
				if (FONT_CONT_MED->StringWidth(aNameString) > aMaxWidth)
				{
					// a name that is too wide loses its last letters to three dots
					while (FONT_CONT_MED->StringWidth(aNameString + "...") > aMaxWidth)
						aNameString.erase(aNameString.end() - 1);
					aNameString += "...";
				}
				g->SetFont(FONT_CONT_MED);
				DrawShadowString(g, aNameString, mApp->ScaleNum(aNameX, 0), mApp->ScaleNum(aY, 0));
				g->SetFont(FONT_CONT_MED);
				DrawShadowString(g, aScoreString, mApp->ScaleNum(aScoreX, 0) - aScoreWidth, mApp->ScaleNum(aY, 0));

				aY += 0x28;
				i++;
				aScore++;
				aName++;
				if (aY >= 0x262)
					break;
			}
		}
	}
	else
	{
		// one table in a box
		aSlide *= 2;
		g->DrawImageBox(Rect(mApp->ScaleNum(200 - aSlide, 0), mApp->ScaleNum(0x68, 0), mApp->ScaleNum(0x270, 0),
			mApp->ScaleNum(0x1e0, 0)), IMAGE_DIALOG);
		int aNameX = 0x10e - aSlide;
		int aScoreX = 0x2f2 - aSlide;
		int* aScore = mScores[aTable];
		std::string* aName = mNames[aTable];
		int i = 0;
		int aY = 0xc0;
		while (true)
		{
			if (mNewPlace[aTable] == i)
			{
				int aPulse = (int)(cos((double)mPulse * 0.12) * -65.0);
				g->SetColor(Color(mApp->HSLToRGB(((mHueOffset + i + 2) * 20) % 256, 0xec, 0xbe - aPulse)));
			}
			else
				g->SetColor(Color(mApp->HSLToRGB(((mHueOffset + i + 2) * 20) % 256, 0xdc, 0xee)));

			std::string aScoreString = CommaSeperate(*aScore);
			int aScoreWidth = FONT_CONT_MED->StringWidth(aScoreString);
			std::string aNameString = *aName;
			int aMaxWidth = mApp->ScaleNum(0x17c, 0);
			if (FONT_CONT_MED->StringWidth(aNameString) > aMaxWidth)
			{
				while (FONT_CONT_MED->StringWidth(aNameString + "...") > aMaxWidth)
					aNameString.erase(aNameString.end() - 1);
				aNameString += "...";
			}
			g->SetFont(FONT_CONT_MED);
			DrawShadowString(g, aNameString, mApp->ScaleNum(aNameX, 0), mApp->ScaleNum(aY, 0));
			g->SetFont(FONT_CONT_MED);
			DrawShadowString(g, aScoreString, mApp->ScaleNum(aScoreX, 0) - aScoreWidth, mApp->ScaleNum(aY, 0));

			aY += 0x28;
			i++;
			aScore++;
			aName++;
			if (aY >= 0x250)
				break;
		}
	}

	// the plate at the bottom: the player's name and rank, and the statistics
	int aBottomSlide = (int)mBottomSlide.GetOutVal();
	g->Translate(0, aBottomSlide);
	g->DrawImage(IMAGE_HS_BOTTOM, mApp->ScaleNum(4, 0), mApp->ScaleNum(0x25f, 0));
	g->SetFont(FONT_CONT_MED);
	g->SetColor(Color(255, 255, 255));
	DrawShadowString(g, mApp->mProfile->mName, mApp->ScaleNum(0x2e, 0), mApp->ScaleNum(0x29b, 0));
	g->SetFont(FONT_CONT_SM);
	g->SetColor(Color(0xf1, 0xf4, 0xff));
	DrawShadowString(g, mRank, mApp->ScaleNum(0x2e, 0), mApp->ScaleNum(0x2bc, 0));

	// (a name that is too wide moves the two columns of statistics to the right; it is measured with the large font while
	// the statistics are drawn with the small one)
	g->SetFont(FONT_CONTINUUM_MED_14);
	int aNameWidth = FONT_CONT_MED->StringWidth(mApp->mProfile->mName);
	int aStatsX1;
	int aStatsX2;
	if (mApp->ScaleNum(0x12c, 0) < aNameWidth)
	{
		aStatsX1 = mApp->ScaleNum(0x1a4, 0xc);
		aStatsX2 = mApp->ScaleNum(0x2bc, 0);
	}
	else
	{
		aStatsX1 = mApp->ScaleNum(0x164, 0xc);
		aStatsX2 = mApp->ScaleNum(0x2a0, 0);
	}

	if (!mShowGame)
	{
		// the statistics of the profile
		Profile* aProfile = mApp->mProfile;
		DrawShadowString(g, StrFormat("Best Score: %s", CommaSeperate(aProfile->mStat[0]).c_str()), aStatsX1, mApp->ScaleNum(0x294, 0));
		int aSeconds = aProfile->mStat[1];
		DrawShadowString(g, StrFormat("Total Time Played: %dh %dm", aSeconds / 3600, (aSeconds / 60) % 60), aStatsX1, mApp->ScaleNum(0x2aa, 0));
		DrawShadowString(g, StrFormat("Puzzles Solved: %d", aProfile->mStat[2]), aStatsX1, mApp->ScaleNum(0x2c0, 0));
		DrawShadowString(g, StrFormat("Gems Collected: %s", CommaSeperate(aProfile->mStat[3]).c_str()), aStatsX1, mApp->ScaleNum(0x2d6, 0));
		DrawShadowString(g, StrFormat("Biggest Cascade: x%d", aProfile->mStat[4]), aStatsX2, mApp->ScaleNum(0x294, 0));
		DrawShadowString(g, StrFormat("Biggest Combo: %s", CommaSeperate(aProfile->mStat[5]).c_str()), aStatsX2, mApp->ScaleNum(0x2aa, 0));
		DrawShadowString(g, StrFormat("Power Gems Created: %s", CommaSeperate(aProfile->mStat[6]).c_str()), aStatsX2, mApp->ScaleNum(0x2c0, 0));
		DrawShadowString(g, StrFormat("Hyper Cubes Created: %s", CommaSeperate(aProfile->mStat[7]).c_str()), aStatsX2, mApp->ScaleNum(0x2d6, 0));
	}
	else
	{
		// the statistics of the game that just ended
		Board* aBoard = mApp->mBoard;
		g->SetColor(Color(0xf1, 0xf4, 0xff));
		DrawShadowString(g, StrFormat("Score: %s", CommaSeperate(aBoard->mScore).c_str()), aStatsX1, mApp->ScaleNum(0x294, 0));
		DrawShadowString(g, StrFormat("Levels Completed: %d", aBoard->mLevel), aStatsX1, mApp->ScaleNum(0x2aa, 0));
		int aSeconds = aBoard->m15e14;
		DrawShadowString(g, StrFormat("Time Played: %dh %dm", aSeconds / 3600, (aSeconds / 60) % 60), aStatsX1, mApp->ScaleNum(0x2c0, 0));
		DrawShadowString(g, StrFormat("Gems Collected: %s", CommaSeperate(aBoard->m15e44).c_str()), aStatsX1, mApp->ScaleNum(0x2d6, 0));
		DrawShadowString(g, StrFormat("Biggest Cascade: x%d", aBoard->m15e6c), aStatsX2, mApp->ScaleNum(0x294, 0));
		DrawShadowString(g, StrFormat("Biggest Combo: %s", CommaSeperate(aBoard->m15e70).c_str()), aStatsX2, mApp->ScaleNum(0x2aa, 0));
		DrawShadowString(g, StrFormat("Power Gems Created: %s", CommaSeperate(aBoard->m15e7c).c_str()), aStatsX2, mApp->ScaleNum(0x2c0, 0));
		DrawShadowString(g, StrFormat("Hyper Cubes Created: %s", CommaSeperate(aBoard->m15e84).c_str()), aStatsX2, mApp->ScaleNum(0x2d6, 0));
	}
	g->Translate(0, -aBottomSlide);
}

// 0x5c50f7: the first press of a button slides the panel out again (the back button) or acts at once
void HighScoreWidget::ButtonDepress(int theId)
{
	if (mClosing == 1)
		return;

	if (theId == BUTTON_BACK)
	{
		// every slide goes back the way it came
		mPanelSlide.SetOutRange(mPanelSlide.mOutMax, mPanelSlide.mOutMin);
		mPanelSlide.SetInVal(0);
		mPanelSlide.SetRamp(1);
		mButtonSlide.SetOutRange(mButtonSlide.mOutMax, mButtonSlide.mOutMin);
		mButtonSlide.SetInVal(0);
		mButtonSlide.SetRamp(1);
		mTitleSlide.SetOutRange(mTitleSlide.mOutMax, mTitleSlide.mOutMin);
		mTitleSlide.SetInVal(0);
		mTitleSlide.SetRamp(1);
		mBottomSlide.SetOutRange(mBottomSlide.mOutMax, mBottomSlide.mOutMin);
		mBottomSlide.SetInVal(0);
		mBottomSlide.SetRamp(1);
		mApp->PlaySample(SOUND_PANEL_SLIDE);
		mClosing = 1;
		if (mApp->mBoard != nullptr)
		{
			if (mApp->mBoard->mState == BOARD_STATE_GAME_OVER)
			{
				mApp->DoMainMenu();
				mApp->mMainMenu->mBackdropClear = true;
			}
			else
				mApp->mBoard->Show();
			mApp->mMusicInterface->FadeOut(mApp->mCurrentSong, true, 0.006);  // slot 0x30 of the music interface
			mApp->mPendingMusicOffset = mApp->GetInteger("MUSICOFFSET_MAIN_MENU");
		}
		if (mApp->mMainMenu != nullptr)
			mApp->mMainMenu->Show();
	}
	else if (theId == BUTTON_PLAY_AGAIN)
	{
		if (mApp->mBoard->m165ae)
		{
			// the display is reloaded first
			mApp->DoReloading(mApp->mIsWindowed, mApp->Is3DAccelerated());
			mApp->mWasInGame = true;
		}
		else
		{
			mApp->DoNewGame();
			mApp->mBoard->m1770 = false;
		}
	}
	else
		mApp->ConfirmResetHighScores();
}

// 0x5c3501: the place a score takes in the table of a mode, 1 to 10, or 0 when it takes none
int HighScoreWidget::GetScorePlace(int theMode, int theScore)
{
	int aPlace = 0;
	for (int i = 9; i >= 0; i--)
	{
		if (mScores[theMode][i] < theScore)
			aPlace = i + 1;
	}
	return aPlace;
}

// 0x5c385d: the score of the game that ended goes into the table under the profile's name
void HighScoreWidget::ProcessScoreStuff()
{
	int aMode = mApp->mGameMode;
	int aScore = mApp->mBoard->mScore;
	int aPlace = GetScorePlace(aMode, aScore);
	if (aPlace > 0)
		mApp->PlaySample(SOUND_HIGH_SCORE);
	NameEntered(mApp->mProfile->mName);
}

// 0x5c5029: the score of the game takes its place under the name, the lower places move down, the tables are saved
void HighScoreWidget::NameEntered(const std::string& theName)
{
	if (theName.empty())
		return;

	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mGameMode;
	int aPlace = GetScorePlace(aMode, aScore);
	mNewPlace[aMode] = aPlace - 1;
	if (aPlace != 0)
	{
		if (aPlace < NUM_PLACES)
		{
			int aCount = NUM_PLACES - aPlace;
			int anIndex = NUM_PLACES - 1;
			do
			{
				mNames[aMode][anIndex] = mNames[aMode][anIndex - 1];
				aCount--;
				mScores[aMode][anIndex] = mScores[aMode][anIndex - 1];
				anIndex--;
			} while (aCount != 0);
		}
		mNames[aMode][aPlace - 1] = theName;
		mScores[aMode][aPlace - 1] = aScore;
	}
	WriteToFile();
}

// 0x5c38f7: every table gets the starting names and scores, and no place is marked as new
void HighScoreWidget::ResetHighScores()
{
	for (int aMode = 0; aMode < NUM_TABLES; aMode++)
	{
		mNewPlace[aMode] = -1;
		for (int i = 0; i < NUM_PLACES; i++)
		{
			mNames[aMode][i] = (aMode % 2 == 0) ? sStartNamesEven[i] : sStartNamesOdd[i];
			mScores[aMode][i] = sStartScores[i];
		}
	}
}

// 0x5c3add: users/hiscores.dat, the 9 tables of 10 places one after the other: the length of the name (2 bytes), the name,
// the score and a check number of the score and name (4 bytes each). A score with a wrong check number is cut down to
// its last 8 bits.
void HighScoreWidget::ReadFromFile()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(GetAppDataFolder() + "users/hiscores.dat", &aBuffer, false))
		return;

	if ((unsigned int)aBuffer.ReadLong() != HIGH_SCORE_MAGIC)
		return;

	for (int aMode = 0; aMode < NUM_TABLES; aMode++)
	{
		for (int i = 0; i < NUM_PLACES; i++)
		{
			// (a file that ends early leaves the places after it as they were: the original would go on with what it had read)
			if (aBuffer.AtEnd() || aBuffer.PastEnd())
				return;

			unsigned short aLength = (unsigned short)aBuffer.ReadShort();
			if (aLength > 0xff)
				aLength = 0xff;
			char aName[0x100];
			for (int j = 0; j < aLength; j++)
				aName[j] = (char)aBuffer.ReadByte();
			aName[aLength] = '\0';
			int aScore = (int)aBuffer.ReadLong();
			unsigned int aCheck = (unsigned int)aBuffer.ReadLong();

			unsigned int aSum = (unsigned int)aScore ^ 0x42beef;
			for (int j = 0; j < aLength; j++)
				aSum = aSum ^ (unsigned int)((signed char)aName[j] * 0x11);
			mNames[aMode][i] = aName;
			if (aCheck != aSum)
				aScore = aScore % 256;
			mScores[aMode][i] = aScore;
		}
	}
}

// 0x5c3c76
void HighScoreWidget::WriteToFile()
{
	MkDir(GetAppDataFolder() + "users");
	std::string aFileName = GetAppDataFolder() + "users/hiscores.dat";
	mApp->EraseFile(aFileName);

	Buffer aBuffer;
	aBuffer.WriteLong((long)(int)HIGH_SCORE_MAGIC);
	for (int aMode = 0; aMode < NUM_TABLES; aMode++)
	{
		for (int i = 0; i < NUM_PLACES; i++)
		{
			short aLength = (short)mNames[aMode][i].size();
			aBuffer.WriteShort(aLength);
			aBuffer.WriteBytes((const uchar*)mNames[aMode][i].c_str(), aLength);
			aBuffer.WriteLong(mScores[aMode][i]);
			unsigned int aSum = (unsigned int)mScores[aMode][i] ^ 0x42beef;
			for (int j = 0; j < aLength; j++)
				aSum = aSum ^ (unsigned int)((signed char)mNames[aMode][i][j] * 0x11);
			aBuffer.WriteLong((long)(int)aSum);
		}
	}
	if (!mApp->WriteBufferToFile(aFileName, &aBuffer))
		mApp->Popup("Failed to create high score file!\nPlease report to support@popcap.com.");
}
