// port(mod): mod "word-journal", the word journal (WordJournal.h). Not part of the original game.

#include "WordJournal.h"
#include "Board.h"
#include "BookWormApp.h"
#include "ModFeatures.h"
#include "PlayerFiles.h"
#include "StatKeeper.h"
#include "WordScore.h"
#include "platform/VFS.h"
#include <cstring>
#include <vector>

using namespace Sexy;

static const char* const JOURNAL_FILE = "word-journal.txt";
static const char* const JOURNAL_HEADER =
	"# word journal: date, mode, level, word, points; \"game\" starts a game, \"undo\" takes back the word before\n";

JournalStats::JournalStats()
{
	mWords = 0;
	mGames = 0;
	mPoints = 0;
	mTiles = 0;
	mFirstDate = 0;
	mBest.mScore = -1;
	mBest.mLevel = 0;
	mBest.mDate = 0;
	mLongest.mScore = -1;
	mLongest.mLevel = 0;
	mLongest.mDate = 0;
	memset(mTileCounts, 0, sizeof(mTileCounts));
}

bool WordJournal::Enabled()
{
	return ModFeatureEnabled("wordJournal");
}

void WordJournal::WordPlayed(Board* theBoard, const WordScore& theWord)
{
	if (!Enabled())
		return;
	const char* aMode = (theBoard->mApp->mGameMode == GAMEMODE_ACTION) ? "action" : "classic";
	int aDate = LocalDate();
	SexyString aLines;
	if (theBoard->mGameStats->mWords == 0)
		aLines = StrFormat("game %d %s\n", aDate, aMode);
	aLines += StrFormat("%d %s %d %s %d\n", aDate, aMode, theBoard->mLevel + 1, theWord.mWord.c_str(), theWord.mScore);
	AppendPlayerFile(theBoard->mApp, JOURNAL_FILE, JOURNAL_HEADER, aLines);
}

void WordJournal::WordUndone(Board* theBoard)
{
	if (Enabled())
		AppendPlayerFile(theBoard->mApp, JOURNAL_FILE, JOURNAL_HEADER, "undo\n");
}

bool WordJournal::Load(BookWormApp* theApp, JournalStats* theStats)
{
	*theStats = JournalStats();
	FILE* aFile = Platform::VFS::Get().Open(PlayerFilePath(theApp, JOURNAL_FILE), "rb");
	if (aFile == NULL)
		return false;

	// the words, with the undone ones taken out; a game counts while its first word stands
	struct Entry
	{
		JournalWord		mWord;
		bool			mFirst;
	};
	std::vector<Entry> anEntries;
	bool isGameStart = false;
	char aLine[256];
	while (fgets(aLine, sizeof(aLine), aFile) != NULL)
	{
		if (aLine[0] == '#')
			continue;
		if (strncmp(aLine, "game", 4) == 0)
		{
			isGameStart = true;
			continue;
		}
		if (strncmp(aLine, "undo", 4) == 0)
		{
			if (!anEntries.empty())
			{
				isGameStart = anEntries.back().mFirst;
				anEntries.pop_back();
			}
			continue;
		}
		int aDate, aLevel, aScore;
		char aMode[16];
		char aWord[64];
		if ((sscanf(aLine, "%d %15s %d %63s %d", &aDate, aMode, &aLevel, aWord, &aScore) != 5) || (aLevel < 1))
			continue;
		Entry anEntry;
		anEntry.mWord.mWord = aWord;
		anEntry.mWord.mScore = aScore;
		anEntry.mWord.mLevel = aLevel;
		anEntry.mWord.mDate = aDate;
		anEntry.mFirst = isGameStart;
		isGameStart = false;
		anEntries.push_back(anEntry);
	}
	fclose(aFile);

	for (size_t i = 0; i < anEntries.size(); i++)
	{
		const JournalWord& aWord = anEntries[i].mWord;
		if (anEntries[i].mFirst)
			theStats->mGames++;
		theStats->mWords++;
		theStats->mPoints += aWord.mScore;
		if ((theStats->mFirstDate == 0) || (aWord.mDate < theStats->mFirstDate))
			theStats->mFirstDate = aWord.mDate;
		if (aWord.mScore > theStats->mBest.mScore)
			theStats->mBest = aWord;
		if ((aWord.mWord.size() > theStats->mLongest.mWord.size()) ||
			((aWord.mWord.size() == theStats->mLongest.mWord.size()) && (aWord.mScore > theStats->mLongest.mScore)))
			theStats->mLongest = aWord;
		std::map<int, JournalWord>::iterator anIt = theStats->mLevelBests.find(aWord.mLevel);
		if ((anIt == theStats->mLevelBests.end()) || (aWord.mScore > anIt->second.mScore))
			theStats->mLevelBests[aWord.mLevel] = aWord;

		// the tiles: a Q is the Qu tile, its U is not a tile of its own
		const SexyString& aLetters = aWord.mWord;
		for (size_t j = 0; j < aLetters.size(); j++)
		{
			char aLetter = aLetters[j];
			if ((aLetter < 'A') || (aLetter > 'Z'))
				continue;
			theStats->mTileCounts[aLetter - 'A']++;
			theStats->mTiles++;
			if ((aLetter == 'Q') && (j + 1 < aLetters.size()) && (aLetters[j + 1] == 'U'))
				j++;
		}
	}
	return true;
}
