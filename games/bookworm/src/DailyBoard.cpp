#include "DailyBoard.h"
#include "Board.h"
#include "BookWormApp.h"
#include "Compat2004.h"
#include "ModFeatures.h"
#include "MTRand.h"
#include "StatKeeper.h"
#include "UserManager.h"
#include "platform/VFS.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <map>

using namespace Sexy;

// The save trailer's tag and version
static const int DAILY_TAG = 0x44415942;    // "DAYB"
static const int DAILY_VERSION = 1;

static const char* const gDayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* const gMonthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// Today's local date as yyyymmdd
static int Today()
{
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	if (aTime == NULL)
		return 20000101;
	return (aTime->tm_year + 1900) * 10000 + (aTime->tm_mon + 1) * 100 + aTime->tm_mday;
}

// "25 Oct", or with the day of the week "Sat 25 Oct"
static SexyString DateName(int theDate, bool withDay)
{
	int aYear = theDate / 10000;
	int aMonth = (theDate / 100) % 100;
	int aDay = theDate % 100;
	if ((aMonth < 1) || (aMonth > 12))
		return StrFormat("%d", theDate);
	SexyString aName = StrFormat("%d %s", aDay, gMonthNames[aMonth - 1]);
	if (!withDay)
		return aName;
	// Sakamoto's day of the week, 0 = Sunday
	static const int aMonthOffsets[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	int aY = (aMonth < 3) ? aYear - 1 : aYear;
	int aWeekDay = (aY + aY / 4 - aY / 100 + aY / 400 + aMonthOffsets[aMonth - 1] + aDay) % 7;
	return SexyString(gDayNames[aWeekDay]) + " " + aName;
}

// A generator's seed for the day, the mode and theStream (0: the letters, 1: the Board's own)
static unsigned long DaySeed(int theDate, int theMode, int theStream)
{
	uint32_t aSeed = (uint32_t)theDate * 2654435761u ^ ((uint32_t)theMode << 8) ^ ((uint32_t)theStream << 16);
	aSeed ^= aSeed >> 16;
	aSeed *= 0x7feb352du;
	aSeed ^= aSeed >> 15;
	aSeed *= 0x846ca68bu;
	aSeed ^= aSeed >> 16;
	return (unsigned long)aSeed;
}

// The player's results, beside the saved games (BookWormApp::GetSaveGamePath's folder)
static SexyString RecordPath(const SexyString& theUserName)
{
	SexyString aPath = "userdata/players/";
	if (theUserName.length() != 0)
		aPath += theUserName + "/";
	return aPath + "daily-board.txt";
}

DailyBoard::DailyBoard(Board* theBoard)
{
	mBoard = theBoard;
	mDate = 0;
	mRecorded = false;
}

DailyBoard* DailyBoard::Create(Board* theBoard)
{
	return ModFeatureEnabled("dailyBoard") ? new DailyBoard(theBoard) : NULL;
}

void DailyBoard::Deal()
{
	BookWormApp* anApp = mBoard->mApp;
	mDate = (anApp->mDailyDate != 0) ? anApp->mDailyDate : Today();
	SeedMTRand2004(*gGameRand, DaySeed(mDate, anApp->mGameMode, 0));
	SeedMTRand2004(*mBoard->mRand, DaySeed(mDate, anApp->mGameMode, 1));
	Board::SetGlobalState(-1, -1);
	anApp->mBonusWord = "";
}

SexyString DailyBoard::Greeting() const
{
	if (mDate == 0)
		return "";
	return "Daily board\n" + DateName(mDate, true);
}

void DailyBoard::GameOver()
{
	if ((mDate == 0) || (mRecorded))
		return;
	mRecorded = true;

	BookWormApp* anApp = mBoard->mApp;
	const char* aModeName = (anApp->mGameMode == GAMEMODE_ACTION) ? "action" : "classic";
	int aScore = mBoard->mGameStats->mScore;
	SexyString aPath = RecordPath(anApp->mUserManager->GetCurrentUserName());

	// the player's earlier games of this mode: the day's count and best, each other day's best
	int aDayGames = 0;
	int aDayBest = 0;
	std::map<int, int> anOtherDays;
	FILE* aFile = Platform::VFS::Get().Open(aPath, "rb");
	bool isNewFile = aFile == NULL;
	if (aFile != NULL)
	{
		char aLine[256];
		while (fgets(aLine, sizeof(aLine), aFile) != NULL)
		{
			int aDate, aLineScore, aLevel, aWords;
			char aMode[16];
			if ((sscanf(aLine, "%d %15s %d %d %d", &aDate, aMode, &aLineScore, &aLevel, &aWords) != 5) ||
				(strcmp(aMode, aModeName) != 0))
				continue;
			if (aDate == mDate)
			{
				aDayGames++;
				aDayBest = std::max(aDayBest, aLineScore);
			}
			else
			{
				std::map<int, int>::iterator anIt = anOtherDays.find(aDate);
				if ((anIt == anOtherDays.end()) || (aLineScore > anIt->second))
					anOtherDays[aDate] = aLineScore;
			}
		}
		fclose(aFile);
	}

	aFile = Platform::VFS::Get().Open(aPath, "ab");
	if (aFile != NULL)
	{
		if (isNewFile)
			fputs("# daily board results: date, mode, score, level, words\n", aFile);
		fprintf(aFile, "%d %s %d %d %d\n", mDate, aModeName, aScore, mBoard->mLevel + 1, mBoard->mGameStats->mWords);
		fclose(aFile);
		anApp->SyncUserData();  // web persistence of the user dir
	}

	int aBestOther = -1;
	int aBestOtherDate = 0;
	for (std::map<int, int>::const_iterator anIt = anOtherDays.begin(); anIt != anOtherDays.end(); ++anIt)
	{
		if (anIt->second > aBestOther)
		{
			aBestOther = anIt->second;
			aBestOtherDate = anIt->first;
		}
	}
	int aTodayBest = std::max(aDayBest, aScore);

	// short lines: the balloon is as wide as the column left of the page
	mTitle = "Daily board";
	mSummary = StrFormat("%s, game %d\nThis game: %s", DateName(mDate, true).c_str(), aDayGames + 1,
		CommaSeperate(aScore).c_str());
	if (aDayGames != 0)
		mSummary += "\nBest today: " + CommaSeperate(aTodayBest);
	if (aBestOtherDate != 0)
	{
		if (aTodayBest >= aBestOther)
			mSummary += "\nBest day: today";
		else
			mSummary += StrFormat("\nBest day: %s\n(%s)", DateName(aBestOtherDate, false).c_str(), CommaSeperate(aBestOther).c_str());
		mSummary += StrFormat("\nDays played: %d", (int)anOtherDays.size() + 1);
	}

	mPraise = "";
	if ((aBestOtherDate != 0) && (aScore > aBestOther) && ((aDayGames == 0) || (aScore > aDayBest)))
		mPraise = "Your best daily board yet!";
	else if ((aDayGames != 0) && (aScore > aDayBest))
		mPraise = "Your best game today!";
}

static void WriteInt(FILE* theFile, int theValue)
{
	fwrite(&theValue, 4, 1, theFile);
}

static bool ReadInt(FILE* theFile, int* theValue)
{
	return fread(theValue, 4, 1, theFile) == 1;
}

static void WriteRand(FILE* theFile, const MTRand& theRand)
{
	uint32_t aState[MTRAND_N];
	int anIndex = 0;
	theRand.GetState(aState, &anIndex);
	fwrite(aState, 4, MTRAND_N, theFile);
	WriteInt(theFile, anIndex);
}

static bool ReadRand(FILE* theFile, MTRand* theRand)
{
	uint32_t aState[MTRAND_N];
	int anIndex = 0;
	return (fread(aState, 4, MTRAND_N, theFile) == MTRAND_N) && (ReadInt(theFile, &anIndex)) &&
		(theRand->SetState(aState, anIndex));
}

// After the bots' record: the tag, the version, the day, what RandomLetter and Refill keep between games, the
// app's next bonus word, then both generators
void DailyBoard::Write(FILE* theFile) const
{
	int aLastLetterIdx = -1;
	int aLastRefillLevel = -1;
	Board::GetGlobalState(&aLastLetterIdx, &aLastRefillLevel);
	const SexyString& aBonusWord = mBoard->mApp->mBonusWord;

	WriteInt(theFile, DAILY_TAG);
	WriteInt(theFile, DAILY_VERSION);
	WriteInt(theFile, mDate);
	WriteInt(theFile, aLastLetterIdx);
	WriteInt(theFile, aLastRefillLevel);
	WriteInt(theFile, (int)aBonusWord.size());
	if (aBonusWord.size() != 0)
		fwrite(aBonusWord.data(), 1, aBonusWord.size(), theFile);
	WriteRand(theFile, *gGameRand);
	WriteRand(theFile, *mBoard->mRand);
}

// false: no day there (a game saved without the mod), or a damaged record; the game is then not a daily one
bool DailyBoard::Read(FILE* theFile)
{
	long aStart = ftell(theFile);
	int aTag = 0;
	int aVersion = 0;
	int aDate = 0;
	int aLastLetterIdx = -1;
	int aLastRefillLevel = -1;
	int aLength = 0;
	SexyString aBonusWord;
	MTRand aGameRand;
	MTRand aBoardRand;
	bool isRead = (ReadInt(theFile, &aTag)) && (aTag == DAILY_TAG) && (ReadInt(theFile, &aVersion)) &&
		(aVersion == DAILY_VERSION) && (ReadInt(theFile, &aDate)) && (ReadInt(theFile, &aLastLetterIdx)) &&
		(ReadInt(theFile, &aLastRefillLevel)) && (ReadInt(theFile, &aLength)) && (aLength >= 0) && (aLength <= 64);
	if (isRead)
	{
		aBonusWord.assign((size_t)aLength, ' ');
		isRead = ((aLength == 0) || (fread(&aBonusWord[0], 1, (size_t)aLength, theFile) == (size_t)aLength)) &&
			(ReadRand(theFile, &aGameRand)) && (ReadRand(theFile, &aBoardRand));
	}
	if (!isRead)
	{
		fseek(theFile, aStart, SEEK_SET);
		mDate = 0;
		return false;
	}

	mDate = aDate;
	Board::SetGlobalState(aLastLetterIdx, aLastRefillLevel);
	mBoard->mApp->mBonusWord = aBonusWord;
	*gGameRand = aGameRand;
	*mBoard->mRand = aBoardRand;
	return true;
}
