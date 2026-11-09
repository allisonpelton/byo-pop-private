#include "HighScoreWidget.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "ReplayRecorder.h"
#include "Res.h"
#include "platform/VFS.h"

#include "Graphics.h"
#include "Font.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "HyperlinkWidget.h"
#include "WidgetManager.h"
#include "GameStatsPage.h"

#include <cstdio>
#include <ctime>

using namespace Sexy;

static const char* HIGHSCORE_FILE_NAME = "userdata\\hiscores.dat";

// hiscores.dat record checksum: score ^ 0x42BEEF ^ (each name byte, sign-extended, * 17)
static int GetHighScoreChecksum(const char* theName, int theLength, int theScore)
{
	int aChecksum = theScore ^ HighScoreWidget::HIGHSCORE_CHECKSUM_KEY;
	for (int i = 0; i < theLength; i++)
		aChecksum ^= (signed char) theName[i] * 17;
	return aChecksum;
}

// part of ReadFromFile (0x4ceec2): reads one record (u16 length, name, int32 score, int32 checksum). Names longer than 255 bytes are
// cut to 255 and the rest stays in the stream, as in the original. Returns false at end of file.
static bool ReadHighScoreRecord(FILE* theFile, SexyString* theName, int* theScore, bool* isValid)
{
	unsigned short aLength;
	if (fread(&aLength, 2, 1, theFile) != 1)
		return false;
	if (aLength > 255)
		aLength = 255;

	char aName[256];
	if ((aLength > 0) && (fread(aName, aLength, 1, theFile) != 1))
		return false;
	aName[aLength] = 0;

	int aChecksum;
	if ((fread(theScore, 4, 1, theFile) != 1) || (fread(&aChecksum, 4, 1, theFile) != 1))
		return false;

	// The checksum covers all aLength bytes, the name only the part before an embedded NUL
	*theName = aName;
	*isValid = aChecksum == GetHighScoreChecksum(aName, aLength, *theScore);
	return true;
}

// part of WriteToFile (0x4cf1b4): writes one record in the format ReadHighScoreRecord reads
static void WriteHighScoreRecord(FILE* theFile, const SexyString& theName, int theScore)
{
	unsigned short aLength = (unsigned short) theName.length();
	int aChecksum = GetHighScoreChecksum(theName.c_str(), aLength, theScore);
	fwrite(&aLength, 2, 1, theFile);
	fwrite(theName.c_str(), aLength, 1, theFile);
	fwrite(&theScore, 4, 1, theFile);
	fwrite(&aChecksum, 4, 1, theFile);
}

// port(variant): the table a game's scores go to: hiscores.dat (scoped for gameplay mods), or for a seed the
// player chose, practice on a known board, its own "-seeded" table
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 3.7, 4.10). Per-game seeds (rules, seeds.perGame) keep the
// normal table. A replay's board (stage 3b, Options -> High Scores while watching) is not the player's game:
// the player's table.
static std::string HighScoreFileName(Board* theBoard)
{
	if ((theBoard != NULL) && (theBoard->mUserSeeded) && (!theBoard->mPlaybackOnly))
		return GameConfig::SeededFileName(HIGHSCORE_FILE_NAME);
	return GameConfig::ScopedFileName(HIGHSCORE_FILE_NAME);
}

// port(variant): "SEED n" between the stats and the tip, for a board with its own RNG. Original games have
// none, so their screen is unchanged. Mods can rename it with the string HSWIDGET_SEED.
static void DrawSeedLine(Graphics* g, WinDMApp* theApp, Board* theBoard)
{
	g->SetFont(theApp->mMainFont);
	g->SetColor(Color(255, 100, 220));
	HighScoreWidget::DrawCenteredString(g, theApp->GetString("HSWIDGET_SEED", _S("SEED")) + StrFormat(_S(" %u"), (unsigned) theBoard->mGameSeed), 408, 120);
}

// port: inlined in the original
static void DrawRightAlignedString(Graphics* g, const SexyString& theString, int theRight, int theY)
{
	g->DrawString(theString, theRight - g->GetFont()->StringWidth(theString), theY);
}

// port: inlined in the original
void HighScoreWidget::DrawCenteredString(Graphics* g, const SexyString& theString, int theCenterX, int theY)
{
	g->DrawString(theString, theCenterX - g->GetFont()->StringWidth(theString) / 2, theY);
}

// port: inlined in the original
void HighScoreWidget::DrawHeading(Graphics* g, const SexyString& theString, int theCenterX, int theY)
{
	int aWidth = g->GetFont()->StringWidth(theString);
	int aX = theCenterX - aWidth / 2;
	g->DrawString(theString, aX, theY);
	g->FillRect(aX, theY + 3, aWidth, 1);
}

// port: inlined in the original
void HighScoreWidget::DrawScoreRow(Graphics* g, const SexyString& theName, int theScore, int theCenterX, int theY)
{
	g->DrawString(theName, theCenterX - 74, theY);
	DrawRightAlignedString(g, StrFormat("%d", theScore), theCenterX + 74, theY);
}

// 0x4ced59
int HighScoreWidget::GetDayNum()
{
	time_t aTime = time(NULL);
	tm* aLocalTime = localtime(&aTime);
	return aLocalTime->tm_year * 366 + aLocalTime->tm_yday;
}

// 0x4ced82
int HighScoreWidget::GetScorePlace(int theMode, int theScore)
{
	// Ties do not place
	int aRank = 0;
	for (int i = NUM_HIGH_SCORES - 1; i >= 0; i--)
	{
		if (theScore > mScores[theMode][i])
			aRank = i + 1;
	}
	return aRank;
}

// 0x4cedaf
void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayAgainLink);
	if (mWatchReplayLink != NULL) // port(variant)
		theWidgetManager->AddWidget(mWatchReplayLink);
	if (mStatsLink != NULL) // port(variant)
		theWidgetManager->AddWidget(mStatsLink);
}

// 0x4cedd2
void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayAgainLink);
	if (mWatchReplayLink != NULL) // port(variant)
		theWidgetManager->RemoveWidget(mWatchReplayLink);
	if (mStatsLink != NULL) // port(variant)
		theWidgetManager->RemoveWidget(mStatsLink);
	if (mStatsPage != NULL) // port(variant): a new game from the left panel while the page is open
		theWidgetManager->RemoveWidget(mStatsPage);
}

// 0x4cedf5
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mSlideY = -theHeight;

	if (mBoardImage == NULL)
	{
		mBoardImage = new DDImage(mApp->mDDInterface);
		mBoardImage->Create(mWidth, mHeight);
		Graphics aBoardG(mBoardImage);
		mApp->mBoard->Draw(&aBoardG);
		mBoardImage->GenerateDDSurface();
	}
}

// 0x4ceeb7
void HighScoreWidget::ButtonDepress(int theId)
{
	if (theId == STATS_LINK_ID) // port(variant): the Stats page over the board window; a click on it closes it
	{
		if ((mStatsPage == NULL) && (mWidgetManager != NULL))
		{
			mStatsPage = new GameStatsPage(mApp, [this]() { CloseStats(); });
			mStatsPage->Resize(GameStatsPage::PAGE_X, GameStatsPage::PAGE_Y, GameStatsPage::PAGE_WIDTH, GameStatsPage::PAGE_HEIGHT);
			mWidgetManager->AddWidget(mStatsPage);
			mWidgetManager->BringToFront(mStatsPage);
			mPlayAgainLink->SetVisible(false);
			mStatsLink->SetVisible(false);
			if (mWatchReplayLink != NULL)
				mWatchReplayLink->SetVisible(false);
		}
		return;
	}
	if (theId == WATCH_REPLAY_LINK_ID) // port(variant): this game's replay, if it is still there
	{
		std::string aPath = ReplayRecorder::FinishedReplay(mApp->mBoard);
		if (!aPath.empty())
			mApp->WatchReplay(aPath, false);
		return;
	}
	mApp->NewGame();
}

// 0x4ceec2
void HighScoreWidget::ReadFromFile()
{
	FILE* aFile = Platform::VFS::Get().Open(mFileName, "rb"); // port(variant): was GameConfig::ScopedFileName(HIGHSCORE_FILE_NAME)
	if (aFile == NULL)
		return;

	// port: the original never checks fread and reuses stale locals on a truncated file; here loading
	// stops at the end of the file and the records not read keep their defaults
	SexyString aName;
	int aScore;
	bool isValid;
	for (int aMode = 0; aMode < NUM_HIGH_SCORE_MODES; aMode++)
	{
		int aDay;
		if ((fread(&aDay, 4, 1, aFile) != 1) || (!ReadHighScoreRecord(aFile, &aName, &aScore, &isValid)))
			break;

		// Today's best only counts on the day it was saved
		if ((aDay == GetDayNum()) && (isValid))
		{
			mTodayNames[aMode] = aName;
			mTodayScores[aMode] = aScore;
		}

		int i;
		for (i = 0; i < NUM_HIGH_SCORES; i++)
		{
			if (!ReadHighScoreRecord(aFile, &aName, &aScore, &isValid))
				break;

			// A tampered record keeps its name, but its score drops to score % 256
			mNames[aMode][i] = aName;
			mScores[aMode][i] = isValid ? aScore : aScore % 256;
		}
		if (i < NUM_HIGH_SCORES)
			break;
	}

	fclose(aFile);
}

// 0x4cf1b4
void HighScoreWidget::WriteToFile()
{
	std::string aFileName = mFileName; // port(variant): was GameConfig::ScopedFileName(HIGHSCORE_FILE_NAME)
	mApp->EraseFile(aFileName);
	// port: the VFS creates the user dir (the original calls MkDir(GetAppDataFolder() + "userdata"))
	FILE* aFile = Platform::VFS::Get().Open(aFileName, "wb");
	if (aFile == NULL)
	{
		mApp->Popup("Failed to create high score file!\nPlease report to support@popcap.com.");
		return;
	}

	for (int aMode = 0; aMode < NUM_HIGH_SCORE_MODES; aMode++)
	{
		// Always stamped with today, so an older "today" record becomes today's on the next load
		int aDay = GetDayNum();
		fwrite(&aDay, 4, 1, aFile);
		WriteHighScoreRecord(aFile, mTodayNames[aMode], mTodayScores[aMode]);
		for (int i = 0; i < NUM_HIGH_SCORES; i++)
			WriteHighScoreRecord(aFile, mNames[aMode][i], mScores[aMode][i]);
	}

	fclose(aFile);
	mApp->SyncUserData();
}

// 0x4cf4a7
SexyString HighScoreWidget::GetOrderSuffix(int theNum)
{
	if ((theNum % 100) / 10 == 1)
		return _S("th");

	switch (theNum % 10)
	{
	case 1:
		return _S("st");
	case 2:
		return _S("nd");
	case 3:
		return _S("rd");
	default:
		return _S("th");
	}
}

// 0x4cf50a
void HighScoreWidget::Draw(Graphics* g)
{
	Board* aBoard = mApp->mBoard;

	g->SetColor(Color::Black);
	g->FillRect(0, 0, mWidth, mHeight);

	// The panel slides down from above; below it the board snapshot stays visible
	int aClipHeight = mSlideY + mHeight;
	if (aClipHeight > PANEL_CLIP_BOTTOM)
		aClipHeight = PANEL_CLIP_BOTTOM;

	Graphics aPanelG(*g);
	aPanelG.ClipRect(0, 0, mWidth, aClipHeight);
	aPanelG.Translate(0, mSlideY);

	aPanelG.SetColor(Color(21, 0, 56));
	aPanelG.FillRect(0, 0, mWidth, mHeight);

	aPanelG.SetFont(mApp->mHeaderFont);
	aPanelG.SetColor(Color(255, 0, 177));
	DrawCenteredString(&aPanelG, mApp->GetString("HSWIDGET_GAME_OVER"), 408, 45);

	aPanelG.SetFont(mApp->mMainFont);
	aPanelG.SetColor(Color(252, 0, 195));
	DrawRightAlignedString(&aPanelG, mApp->GetString("HSWIDGET_GAME_DURATION"), 341, 88);
	DrawRightAlignedString(&aPanelG, mApp->GetString("HSWIDGET_GEMS_CLEARED"), 341, 105);
	DrawRightAlignedString(&aPanelG, mApp->GetString("HSWIDGET_LONGEST_CASCADE"), 546, 88);
	DrawRightAlignedString(&aPanelG, mApp->GetString("HSWIDGET_BEST_SINGLE_MOVE"), 546, 105);

	aPanelG.SetColor(Color(255, 100, 220));
	DrawRightAlignedString(&aPanelG, StrFormat("%2d.%02d", aBoard->mGameDuration / 60, aBoard->mGameDuration % 60), 382, 88);
	DrawRightAlignedString(&aPanelG, StrFormat("%d", aBoard->mGemsCleared), 382, 105);
	DrawRightAlignedString(&aPanelG, StrFormat("%d", aBoard->mLongestCascade), 587, 88);
	DrawRightAlignedString(&aPanelG, StrFormat("%d", aBoard->mBestMoveScore), 587, 105);

	if (aBoard->mOwnRng != NULL) // port(variant): the game's seed
		DrawSeedLine(&aPanelG, mApp, aBoard);

	aPanelG.SetFont(mApp->mMainFont);
	aPanelG.SetColor(Color(250, 0, 0));
	DrawCenteredString(&aPanelG, mTip, 408, 136);

	aPanelG.SetColor(Color(0, 218, 0));
	aPanelG.FillRect(191, 151, 434, 23);
	aPanelG.SetColor(Color(0, 0, 0));
	aPanelG.SetFont(mApp->mHeaderFont);
	SexyString aString = mApp->GetString("HSWIDGET_HIGH_SCORES");
	int aTitleY = (aPanelG.GetFont()->GetAscent() + 23) / 2 + 150;
	aPanelG.DrawString(aString, (434 - aPanelG.GetFont()->StringWidth(aString)) / 2 + 191, aTitleY);

	// The "CLICK HERE TO PLAY AGAIN" bar (the link itself is a separate top-level widget)
	aPanelG.SetColor(Color(0, 218, 0));
	aPanelG.FillRect(191, 389, 434, 23);

	for (int aMode = 0; aMode < NUM_HIGH_SCORE_MODES; aMode++)
	{
		int aCenterX = aMode * 238 + 287;
		aPanelG.SetFont(mApp->mMainFont);

		aPanelG.SetColor(Color(255, 193, 0));
		DrawHeading(&aPanelG, mApp->GetString((aMode == 0) ? "HSWIDGET_NORMAL" : "HSWIDGET_TIMED"), aCenterX, 197);

		aPanelG.SetColor(Color(46, 216, 0));
		DrawHeading(&aPanelG, mApp->GetString("HSWIDGET_TODAYS_HIGH_SCORE"), aCenterX, 223);

		// Rows cycle through the hues, each row 2 steps (40) ahead of the one above
		aPanelG.SetColor(Color((int) mApp->HSLToRGB((mColorCycle * 20) % 256, 255, 180)));
		if (mTodayScores[aMode] < 1)
			DrawCenteredString(&aPanelG, mApp->GetString("HSWIDGET_EMPTY"), aCenterX, 249);
		else
			DrawScoreRow(&aPanelG, mTodayNames[aMode], mTodayScores[aMode], aCenterX, 249);

		aPanelG.SetColor(Color(46, 216, 0));
		DrawHeading(&aPanelG, mApp->GetString("HSWIDGET_ALL_TIME_HIGH_SCORES"), aCenterX, 279);

		for (int i = 0; i < NUM_HIGH_SCORES; i++)
		{
			aPanelG.SetColor(Color((int) mApp->HSLToRGB(((mColorCycle + 2 + i) * 20) % 256, 255, 180)));
			DrawScoreRow(&aPanelG, mNames[aMode][i], mScores[aMode][i], aCenterX, 303 + i * 17);
		}
	}

	// The HUD frame and score are drawn unslid, still clipped to the panel
	aPanelG.Translate(0, -mSlideY);
	aPanelG.DrawImage(mApp->mImages[IMAGE_ALPHABACK], 0, 0);
	aPanelG.SetFont(mApp->mScoreFont);
	aPanelG.SetColor(Color(173, 231, 255));
	DrawCenteredString(&aPanelG, StrFormat("%d", aBoard->mScore), 93, 106);

	Graphics aBoardG(*g);
	aBoardG.ClipRect(0, aClipHeight, mWidth, mHeight - aClipHeight);
	aBoardG.DrawImage(mBoardImage, 0, 0);
}

// 0x4d06d8
void HighScoreWidget::AddHighScore(const SexyString& theName)
{
	if (theName.empty())
		return;

	// The board's mode here, the app's mode in ProcessScoreStuff (always equal)
	int aMode = mApp->mBoard->mTimedMode ? 1 : 0;
	int aScore = mApp->mBoard->mScore;
	int aRank = GetScorePlace(aMode, aScore);

	if (aScore > mTodayScores[aMode])
	{
		mTodayScores[aMode] = aScore;
		mTodayNames[aMode] = theName;
	}

	if (aRank != 0)
	{
		for (int i = NUM_HIGH_SCORES - 1; i >= aRank; i--)
		{
			mNames[aMode][i] = mNames[aMode][i - 1];
			mScores[aMode][i] = mScores[aMode][i - 1];
		}
		mNames[aMode][aRank - 1] = theName;
		mScores[aMode][aRank - 1] = aScore;
	}

	WriteToFile();
}

// 0x4d07cd
HighScoreWidget::HighScoreWidget(WinDMApp* theApp)
{
	mSlideY = 0;
	mApp = theApp;
	mColorCycle = 0;

	mPlayAgainLink = new HyperlinkWidget(PLAY_AGAIN_LINK_ID, this);
	mPlayAgainLink->mLabel = mApp->GetString("HSWIDGET_CLICK_HERE_TO_PLAY_AGAIN");
	mPlayAgainLink->SetFont(mApp->mHeaderFont);
	mPlayAgainLink->mColor = Color(0, 0, 0);
	mPlayAgainLink->mOverColor = Color(255, 255, 255);
	mPlayAgainLink->mUnderlineSize = 0;

	// port(variant): hidden until Update finds this game's replay written (ReplayRecorder::AfterUpdate writes it an update
	// after this widget is made: it asks the widget whether the score makes a table)
	// port(variant): the Stats page's link, for a board that counted (features.gameStats)
	mStatsLink = NULL;
	mStatsPage = NULL;
	if ((mApp->mBoard != NULL) && (mApp->mBoard->mStats != NULL))
	{
		mStatsLink = new HyperlinkWidget(STATS_LINK_ID, this);
		mStatsLink->mLabel = mApp->GetString("HSWIDGET_STATS", _S("STATS"));
		mStatsLink->SetFont(mApp->mMainFont);
		mStatsLink->mColor = Color(255, 100, 220);
		mStatsLink->mOverColor = Color(255, 255, 255);
	}
	mWatchReplayLink = NULL;
	if (gGameConfig.mFeatures.mReplays)
	{
		mWatchReplayLink = new HyperlinkWidget(WATCH_REPLAY_LINK_ID, this);
		mWatchReplayLink->mLabel = mApp->GetString("HSWIDGET_WATCH_REPLAY", _S("WATCH REPLAY"));
		mWatchReplayLink->SetFont(mApp->mMainFont);
		mWatchReplayLink->mColor = Color(255, 100, 220);
		mWatchReplayLink->mOverColor = Color(255, 255, 255);
		mWatchReplayLink->mVisible = false;
	}

	mBoardImage = NULL;

	int aTipNum = Rand() % 9;
	if (aTipNum <= 7)
		mTip = mApp->GetString(StrFormat("HSWIDGET_TIP%d", aTipNum));
	else
		mTip = mApp->GetString("HSWIDGET_TIP_DEFAULT");

	mTodayScores[0] = 0;
	mNames[0][0] = _S("Ace");
	mScores[0][0] = 1000;
	mNames[0][1] = _S("John");
	mScores[0][1] = 900;
	mNames[0][2] = _S("Chris");
	mScores[0][2] = 800;
	mNames[0][3] = _S("Brian");
	mScores[0][3] = 700;
	mNames[0][4] = _S("Ben");
	mScores[0][4] = 600;

	mTodayScores[1] = 0;
	mNames[1][0] = _S("Bradley");
	mScores[1][0] = 1000;
	mNames[1][1] = _S("Mike");
	mScores[1][1] = 900;
	mNames[1][2] = _S("Billy");
	mScores[1][2] = 800;
	mNames[1][3] = _S("Jason");
	mScores[1][3] = 700;
	mNames[1][4] = _S("Wim");
	mScores[1][4] = 600;

	mFileName = HighScoreFileName(mApp->mBoard); // port(variant): the table of the game on the board
	ReadFromFile();
}

// 0x4d0dad
HighScoreWidget::~HighScoreWidget()
{
	delete mBoardImage;
	delete mPlayAgainLink;
	delete mWatchReplayLink; // port(variant)
	delete mStatsLink; // port(variant)
	delete mStatsPage; // port(variant)
	WriteToFile();
}

// 0x4d0e4b
void HighScoreWidget::ProcessScoreStuff()
{
	int aScore = mApp->mBoard->mScore;
	int aMode = mApp->mTimedMode ? 1 : 0;
	bool isBestToday = aScore > mTodayScores[aMode];
	int aRank = GetScorePlace(aMode, aScore);

	// port: always registered; the unregistered branch appended a "register to record your score"
	// question to the same messages and showed ShowRegisterToRecordScoreDialog instead
	SexyString aMessage;
	if (isBestToday)
	{
		if (aRank > 0)
			aMessage = _S("You got the highest score of\nthe day and ") + StrFormat("%d", aRank) + GetOrderSuffix(aRank) + _S(" place\nof all time!");
		else
			aMessage = _S("You got the highest score\nof the day!");
	}
	else if (aRank > 0)
		aMessage = _S("You got ") + StrFormat("%d", aRank) + GetOrderSuffix(aRank) + _S(" place of all time!");
	else
		return;

	mApp->DoRecordScoreDialog(aMessage);
}

// 0x4d1187
void HighScoreWidget::Update()
{
	Widget::Update();
	if ((mWatchReplayLink != NULL) && !mWatchReplayLink->mVisible && (mStatsPage == NULL) && !ReplayRecorder::FinishedReplay(mApp->mBoard).empty()) // port(variant)
		mWatchReplayLink->SetVisible(true);

	if (mSlideY < 0)
	{
		mSlideY += SLIDE_SPEED;
		if (mSlideY >= 0)
		{
			mSlideY = 0;
			ProcessScoreStuff();
		}
		MarkDirty();
		mPlayAgainLink->Resize(265, mSlideY + 389, 286, 23);
		if (mWatchReplayLink != NULL) // port(variant): right-aligned with the stats at 587, its baseline at 120
		{
			int aLinkWidth = mApp->mMainFont->StringWidth(mWatchReplayLink->mLabel) + 8;
			int aLinkHeight = mApp->mMainFont->GetHeight() + 4;
			mWatchReplayLink->Resize(591 - aLinkWidth, mSlideY + 121 - (aLinkHeight + mApp->mMainFont->GetAscent()) / 2, aLinkWidth, aLinkHeight);
		}
		if (mStatsLink != NULL) // port(variant): left-aligned at 225, on the same row
		{
			int aLinkWidth = mApp->mMainFont->StringWidth(mStatsLink->mLabel) + 8;
			int aLinkHeight = mApp->mMainFont->GetHeight() + 4;
			mStatsLink->Resize(225, mSlideY + 121 - (aLinkHeight + mApp->mMainFont->GetAscent()) / 2, aLinkWidth, aLinkHeight);
		}
	}

	if (mUpdateCnt % 10 == 0)
	{
		mColorCycle++;
		MarkDirty();
	}
}

// port(variant): the Stats page closes (its own click: deleted after the mouse event); the links come back
void HighScoreWidget::CloseStats()
{
	if (mStatsPage == NULL)
		return;
	if (mStatsPage->mWidgetManager != NULL)
		mStatsPage->mWidgetManager->RemoveWidget(mStatsPage);
	mApp->SafeDeleteWidget(mStatsPage);
	mStatsPage = NULL;
	mPlayAgainLink->SetVisible(true);
	mStatsLink->SetVisible(true);
	if ((mWatchReplayLink != NULL) && !ReplayRecorder::FinishedReplay(mApp->mBoard).empty())
		mWatchReplayLink->SetVisible(true);
	MarkDirty();
}

// 0x4d11fd
void HighScoreWidget::CheckForHighScore()
{
	ProcessScoreStuff();
}
