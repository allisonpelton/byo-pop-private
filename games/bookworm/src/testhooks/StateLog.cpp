#include "StateLog.h"
#include "../BookWormApp.h"
#include "../Board.h"
#include "../StatKeeper.h"
#include <cstdint>
#include <cstring>

using namespace Sexy;

// The values in the order of games/bookworm/tools/identical/values.txt, which gives each one's address in the original
static const char* const gStateNames[] = { "score", "state", "level", "gmti", "gmt0", "bmti", "words", "letters", "fires",
	"firetimer", "bonusgiven", "firemeter", "lvlscore", "burned", "gamemode", "praise" };
static const int NUM_STATE_VALUES = sizeof(gStateNames) / sizeof(gStateNames[0]);

// An MTRand's 624 state words and then mti, its index (framework/MTRand.h), as in the original
static uint32_t RandWord(const MTRand* theRand, int theIndex)
{
	return reinterpret_cast<const uint32_t*>(theRand)[theIndex];
}

StateLog::StateLog(const std::string& theFileName)
{
	mFile = fopen(theFileName.c_str(), "w");
	mLast.resize(NUM_STATE_VALUES);
}

StateLog::~StateLog()
{
	if (mFile != NULL)
		fclose(mFile);
}

void StateLog::Update(BookWormApp* theApp)
{
	if (mFile == NULL)
		return;
	std::vector<std::string> aValues(NUM_STATE_VALUES, "-");
	char aBuf[32];
	Board* aBoard = theApp->mBoard;
	uint32_t anInts[NUM_STATE_VALUES];
	bool aKnown[NUM_STATE_VALUES];
	memset(aKnown, 0, sizeof(aKnown));
	if (gGameRand != NULL)
	{
		anInts[3] = RandWord(gGameRand, 624);
		anInts[4] = RandWord(gGameRand, 0);
		aKnown[3] = aKnown[4] = true;
	}
	anInts[14] = (uint32_t)theApp->mGameMode;
	aKnown[14] = true;
	if (aBoard != NULL)
	{
		uint64_t aMeter;
		memcpy(&aMeter, &aBoard->mFireMeter, sizeof(aMeter));
		anInts[0] = (uint32_t)aBoard->mGameStats->mScore;
		anInts[1] = (uint32_t)aBoard->mState;
		anInts[2] = (uint32_t)aBoard->mLevel;
		anInts[5] = RandWord(aBoard->mRand, 624);
		anInts[6] = (uint32_t)aBoard->mGameStats->mWords;
		anInts[7] = (uint32_t)aBoard->mGameStats->mLetters;
		anInts[8] = (uint32_t)aBoard->mFireTileCount;
		anInts[9] = (uint32_t)aBoard->mActionFireTimer;
		anInts[10] = (uint32_t)aBoard->mBonusWordsGiven;
		anInts[11] = (uint32_t)(aMeter >> 32);  // the double's high word, as a 32-bit watch reads it at +4
		anInts[12] = (uint32_t)aBoard->mLevelStats->mScore;
		anInts[13] = (uint32_t)aBoard->mGameStats->mTilesBurned;
		anInts[15] = (uint32_t)aBoard->mLexPraisePending;   // the bool and three bytes of padding, which are 0 there
		aKnown[15] = true;
		aKnown[0] = aKnown[1] = aKnown[2] = aKnown[5] = aKnown[6] = aKnown[7] = aKnown[8] = aKnown[9] = true;
		aKnown[10] = aKnown[11] = aKnown[12] = aKnown[13] = true;
	}
	for (int i = 0; i < NUM_STATE_VALUES; i++)
	{
		if (aKnown[i])
		{
			sprintf(aBuf, "%u", anInts[i]);
			aValues[i] = aBuf;
		}
		if (aValues[i] != mLast[i])
			fprintf(mFile, "watch %s: %s -> %s at update %d\n", gStateNames[i], mLast[i].empty() ? "-" : mLast[i].c_str(),
				aValues[i].c_str(), theApp->mUpdateCount);
		mLast[i] = aValues[i];
	}
	fflush(mFile);
}
