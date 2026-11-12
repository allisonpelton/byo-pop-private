// port(mod): mod "greedy-bots", the greedy bots (GreedyBots.h). Not part of the original game.

#include "GreedyBots.h"
#include "BookWormApp.h"
#include "ModWords.h"
#include "Piece.h"
#include "Solver.h"
#include "StatKeeper.h"
#include "WordScore.h"
#include <algorithm>
#include <chrono>
#include <cstdint>

using namespace Sexy;

// A bot still alive after this many moves stops (shown as still going): a game can last a long time
static const int GREEDY_MAX_MOVES = 2500;

// Ticks of the tile pass at most after a move (burning takes 77 at rest)
static const int GREEDY_MAX_SETTLE_TICKS = 5000;

// GreedyBots::Write's record
static const int GREEDY_RECORD_TAG = 0x53544f42;
static const int GREEDY_RECORD_VERSION = 1;

// The top row of a column: even columns hold rows 1..7, odd columns rows 0..7 (Board.cpp's TopRow)
static inline int TopRow(int theCol)
{
	return (theCol % 2 == 0) ? 1 : 0;
}

// Board::RowY, Board::ColX
static inline int RowY(int theCol, int theRow)
{
	return (theCol % 2 == 0) ? theRow * TILE_SIZE - 17 : theRow * TILE_SIZE + 11;
}

static inline int ColX(int theCol)
{
	return theCol * TILE_SIZE + 233;
}

// --------------------------------------------------------------------------------------------------------
// BotGame

BotGame::BotGame()
{
	mApp = NULL;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			mGrid[aRow][aCol] = NULL;
	mLastLetterIdx = -1;
	mLastRefillLevel = -1;
	mState = BOARD_PLAYING;
	mLevel = 0;
	mFireMeter = 0.0;
	mSkipRandomFire = false;
	mRewardStreak = 0;
	mFireTileCount = 0;
	mBonusWordsGiven = 0;
	mScore = 0;
	mWords = 0;
	mBonusWords = 0;
	mScrambles = 0;
	mLevelScore = 0;
	mLevelWords = 0;
	mModFireDue = 0;
	mMoves = 0;
}

BotGame::~BotGame()
{
	Clear();
}

void BotGame::Clear()
{
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = NULL;
		}
	}
}

void BotGame::Capture(Board* theBoard)
{
	Clear();
	mApp = theBoard->mApp;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = TopRow(aCol); aRow < BOARD_ROWS; aRow++)
		{
			Piece* aPiece = theBoard->mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			Piece* aCopy = new Piece();
			aCopy->mRow = aRow;
			aCopy->mCol = aCol;
			aCopy->mX = (double)ColX(aCol);
			aCopy->mY = (double)RowY(aCol, aRow);
			aCopy->mLetter = aPiece->mLetter;
			aCopy->mNextLetter = aPiece->mNextLetter;
			aCopy->mType = aPiece->mType;
			aCopy->mBurnCountdown = aPiece->mBurnCountdown;
			aCopy->mBurning = aPiece->mBurning;
			aCopy->mBurnProgress = aPiece->mBurnProgress;
			aCopy->mBurnDelay = aPiece->mBurnDelay;
			mGrid[aRow][aCol] = aCopy;
		}
	}

	mGameRand = *gGameRand;
	Board::GetGlobalState(&mLastLetterIdx, &mLastRefillLevel);
	mState = theBoard->mState;
	mLevel = theBoard->mLevel;
	mFireMeter = theBoard->mFireMeter;
	mSkipRandomFire = theBoard->mSkipRandomFire;
	mRewardStreak = theBoard->mRewardStreak;
	mFireTileCount = theBoard->mFireTileCount;
	mBonusWordsGiven = theBoard->mBonusWordsGiven;
	mBonusWord = theBoard->mBonusWord;
	mNextBonusWord = mApp->mBonusWord;
	mScore = theBoard->mGameStats->mScore;
	mWords = theBoard->mGameStats->mWords;
	mBonusWords = theBoard->mGameStats->mBonusWords;
	mScrambles = theBoard->mGameStats->mScrambles;
	mLevelScore = theBoard->mLevelStats->mScore;
	mLevelWords = theBoard->mLevelStats->mWords;
	mModFireDue = 0;
	mMoves = 0;
}

std::vector<Piece*> BotGame::Tiles() const
{
	std::vector<Piece*> aTiles;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mLetter >= 'A') && (aPiece->mLetter <= 'Z'))
				aTiles.push_back(aPiece);
		}
	}
	return aTiles;
}

std::string BotGame::State() const
{
	std::string aState;
	for (int aCell = 0; aCell < BOARD_ROWS * BOARD_COLS; aCell++)
	{
		Piece* aPiece = mGrid[aCell / BOARD_COLS][aCell % BOARD_COLS];
		aState += (aPiece == NULL) ? '.' : aPiece->mLetter;
		aState += (aPiece == NULL) ? '.' : (char)('0' + aPiece->mType);
	}
	return aState + StrFormat(" score %d fire %.4f tiles %d bonus %s", mScore, mFireMeter, mFireTileCount,
		mBonusWord.c_str());
}

// Board::RandomLetter
int BotGame::RandomLetter()
{
	for (;;)
	{
		int aRoll = (int)mGameRand.Next() % 95500;
		int anIndex = 0;
		while (anIndex < 25)
		{
			aRoll -= gLetterWeights[anIndex];
			if (aRoll < 0)
				break;
			anIndex++;
		}

		if (anIndex != mLastLetterIdx)
		{
			mLastLetterIdx = anIndex;
			return 'A' + anIndex;
		}
	}
}

// Board::PieceBelow
Piece* BotGame::Below(Piece* thePiece) const
{
	if (thePiece->mRow < 7)
		return mGrid[thePiece->mRow + 1][thePiece->mCol];
	return NULL;
}

// Board::CheckForFalling in Classic (the new tiles start above the board: the game draws their heights from mRand)
void BotGame::Refill()
{
	bool isLevelUp = mLevel > mLastRefillLevel;
	int aFireDue = 0;
	mLastRefillLevel = mLevel;
	std::vector<Piece*> aNewTiles;

	int aChance = gFireChanceByLevel[((unsigned int)mLevel > 11) ? 12 : mLevel];
	if ((aChance > 0) && ((int)(mGameRand.Next() % 100) < aChance))
	{
		if (!mSkipRandomFire)
			aFireDue = 1;
		else
			mSkipRandomFire = false;
	}

	if ((!isLevelUp) && (mFireMeter <= 0.0))
	{
		aFireDue++;
		mFireMeter = gFireMeterRefill[((unsigned int)mLevel > 10) ? 11 : mLevel] + mFireMeter;
	}

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		int aTop = 7;
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != NULL)
			{
				aPiece->mRow = aTop;
				mGrid[aTop][aPiece->mCol] = aPiece;
				aTop--;
			}
		}

		int aTopRow = TopRow(aCol);
		for (int aRow = aTop; aRow >= aTopRow; aRow--)
		{
			Piece* aPiece = new Piece();
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = (double)ColX(aCol);
			aPiece->mY = (double)(RowY(aCol, aRow) - 8 * TILE_SIZE);
			aPiece->mLetter = (char)RandomLetter();
			aPiece->mVY = 0.0;

			if ((aFireDue > 0) && (aTop == aTopRow))
			{
				aPiece->SetType(TILE_FIRE, true);
				mFireTileCount++;
				aPiece->mLetter = (char)RandomLetter();
				aFireDue--;
			}

			if ((aPiece->mType == TILE_NORMAL) && (mGameRand.Next() % 20 == 0))
				aPiece->SetType(TILE_GREEN, true);

			mGrid[aRow][aCol] = aPiece;
			if (mModFireDue > 0)
				aNewTiles.push_back(aPiece);
		}

		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType != TILE_FIRE) || (aRow >= 7))
				continue;
			Piece* aBelow = Below(aPiece);
			if ((aBelow != NULL) && (aBelow->mBurnCountdown < 1))
				aBelow->ResetBurnCountdown();
		}
	}

	if (mModFireDue > 0)
	{
		SetModFire(aNewTiles, mModFireDue);
		mModFireDue = 0;
	}
}

// Board::SetModFire
void BotGame::SetModFire(const std::vector<Piece*>& theNewTiles, int theCount)
{
	std::vector<Piece*> aCandidates;
	for (Piece* aPiece : theNewTiles)
	{
		if ((aPiece->mType == TILE_NORMAL) && (aPiece->mRow < 7))
			aCandidates.push_back(aPiece);
	}
	for (int i = 0; (i < theCount) && (!aCandidates.empty()); i++)
	{
		unsigned int anIndex = mGameRand.Next() % (unsigned int)aCandidates.size();
		Piece* aPiece = aCandidates[anIndex];
		aCandidates.erase(aCandidates.begin() + anIndex);
		aPiece->SetType(TILE_FIRE, true);
		mFireTileCount++;
		Piece* aBelow = Below(aPiece);
		if ((aBelow != NULL) && (aBelow->mBurnCountdown < 1))
			aBelow->ResetBurnCountdown();
	}
}

// Board::BurnTiles(true) in Classic: the word's tiles (mSelected) neither burn nor advance their fire
void BotGame::BurnTiles()
{
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType != TILE_FIRE))
				continue;

			if (aRow < 7)
			{
				Piece* aBelow = Below(aPiece);
				if ((aBelow != NULL) && (aBelow->mBurnCountdown > 0) && (!aPiece->mSelected))
				{
					if ((--aBelow->mBurnCountdown == 0) && (!aBelow->mSelected))
						aBelow->StartBurn();
				}
			}
			else if (!aPiece->mSelected)
				mState = BOARD_GAMEOVER_PENDING;
		}
	}
}

// Board::GenerateSuperBonusTiles (its 6-letter draw is dead: gLen6Gate is never written)
void BotGame::AwardSpecialTile(const WordScore& theWord)
{
	int aLength = (int)theWord.mWord.size();
	unsigned int aType = TILE_NORMAL;

	if (aLength >= 11)
		aType = TILE_DIAMOND;
	else if (aLength == 10)
		aType = (mGameRand.Next() & 1) | TILE_SAPPHIRE;
	else if (aLength == 9)
		aType = TILE_SAPPHIRE;
	else if (aLength == 8)
		aType = ((mGameRand.Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
	else if (aLength == 7)
		aType = ((mGameRand.Next() & 3) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
	else if (aLength == 6)
		aType = TILE_GOLD;
	else if (aLength == 5)
	{
		if (mGameRand.Next() % 5 == 0)
			aType = TILE_GOLD;
	}
	else if (aLength == 4)
	{
		if (mGameRand.Next() % 100 == 0)
			aType = TILE_GOLD;
	}

	bool isRewarded;
	if (theWord.mTypeCount[TILE_SAPPHIRE] >= 2)
	{
		aType = TILE_DIAMOND;
		isRewarded = true;
	}
	else if (theWord.mTypeCount[TILE_GOLD] > 2)
	{
		aType = TILE_DIAMOND;
		isRewarded = true;
	}
	else if (theWord.mTypeCount[TILE_GOLD] == 2)
	{
		if (aType < TILE_DIAMOND)
			aType = TILE_SAPPHIRE;
		isRewarded = true;
	}
	else
	{
		if (theWord.mTypeCount[TILE_GREEN] > 2)
		{
			unsigned int aLimit = ((mGameRand.Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
			if (aType <= aLimit)
				aType = ((mGameRand.Next() & 1) != 0) ? TILE_SAPPHIRE : TILE_GOLD;
		}
		isRewarded = aType != TILE_NORMAL;
	}

	if (isRewarded)
		mRewardStreak++;
	else
		mRewardStreak = 0;

	if (mRewardStreak >= 5)
	{
		if (aType < 6)
			aType = TILE_DIAMOND;
	}
	else if ((mRewardStreak > 2) && (aType < TILE_DIAMOND))
		aType = TILE_SAPPHIRE;
	else if (aType == TILE_NORMAL)
		return;

	// Board::GetNormalPieces
	std::vector<Piece*> aCandidates;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mType == TILE_NORMAL) && (!aPiece->mBurning) && (!aPiece->mPendingFire))
				aCandidates.push_back(aPiece);
		}
	}
	if (aCandidates.size() == 0)
		return;

	Piece* aPiece = aCandidates[mGameRand.Next() % (unsigned int)aCandidates.size()];
	aPiece->SetType((int)aType, true);
}

// Board::RemoveBonusTiles (rude-words)
void BotGame::RemoveBonusTiles()
{
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = TopRow(aCol); aRow < BOARD_ROWS; aRow++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType == TILE_NORMAL) || (aPiece->mType == TILE_FIRE))
				continue;
			aPiece->SetType(TILE_NORMAL, false);
			if (aPiece->mBurnCountdown > 1)
				aPiece->mBurnCountdown = 1;
		}
	}
}

// Board::GetNextBonusWord: into the app's next bonus word
void BotGame::PickBonusWord()
{
	unsigned int aGivenIndex = (unsigned int)mBonusWordsGiven;
	int aWantedLength = gBonusWordLength[(aGivenIndex > 8) ? 9 : aGivenIndex];
	int aTries = (int)mApp->mBonusWords.size();

	while (aTries >= 1)
	{
		unsigned int aCount = (unsigned int)mApp->mBonusWords.size();
		const SexyString& aWord = mApp->mBonusWords[mGameRand.Next() % aCount];
		if ((_stricmp(aWord.c_str(), mNextBonusWord.c_str()) != 0) && ((int)aWord.size() == aWantedLength))
		{
			mNextBonusWord = aWord;
			mBonusWordsGiven++;
			return;
		}
		aTries--;
	}
}

// Board::Update step 3: the banner takes up the next bonus word (the board at rest, the banner too)
void BotGame::ShowBonusWord()
{
	if ((mState == BOARD_PLAYING) && (mBonusWord.size() == 0) && ((mLevel > 1) || ((mLevel == 1) && (mLevelWords >= 1))))
		mBonusWord = mNextBonusWord;
}

// Board::Submit for a valid word
bool BotGame::Submit(const std::vector<Piece*>& thePath)
{
	if ((mState != BOARD_PLAYING) || (thePath.empty()))
		return false;

	PieceList aList(thePath.begin(), thePath.end());
	WordScore aWord;
	aWord.mApp = mApp;
	aWord.mLevel = mLevel;
	aWord.CalcTileStats(&aList);

	ModWordEffects aModEffects;
	bool isModWord = ModWordGetEffects(aWord.mWord, &aModEffects);
	if ((isModWord) && (aWord.mScore < 0) && (aWord.mScore < -mLevelScore))
		aWord.mScore = -mLevelScore;

	mLevelScore += aWord.mScore;
	mLevelWords++;
	mScore += aWord.mScore;
	mWords++;

	int aLength = (int)aWord.mWord.size();
	unsigned int aLengthIndex = (unsigned int)(aLength - 3);
	unsigned int aMeterIndex = (aLengthIndex > 4) ? 5 : aLengthIndex;
	if ((aWord.mTypeCount[TILE_FIRE] == 0) || (gFireMeterByLength[aMeterIndex] > 0.0))
		mFireMeter = gFireMeterByLength[aMeterIndex] + mFireMeter;

	if (_stricmp(aWord.mWord.c_str(), mBonusWord.c_str()) == 0)
	{
		unsigned int aBonusIndex = (unsigned int)mBonusWords;
		int aPoints = gBonusWordPoints[(aBonusIndex > 9) ? 10 : aBonusIndex];
		mLevelScore += aPoints;
		mScore += aPoints;
		mBonusWords++;
		if (mBonusWord != "")
		{
			PickBonusWord();
			mBonusWord = "";
		}
		mSkipRandomFire = true;
		mFireMeter = (double)aLength + mFireMeter;
	}

	for (Piece* aPiece : thePath)
		aPiece->mSelected = true;
	BurnTiles();
	mFireMeter = mFireMeter - 1.0;

	// Board::RemovePieces
	for (Piece* aPiece : thePath)
	{
		mGrid[aPiece->mRow][aPiece->mCol] = NULL;
		if (aPiece->mType == TILE_FIRE)
		{
			mFireTileCount--;
			Piece* aBelow = Below(aPiece);
			if (aBelow != NULL)
				aBelow->ResetBurnCountdown();
		}
	}

	if ((mScore >= Board::GetPointsAtLevel((unsigned int)mLevel)) && (mState == BOARD_PLAYING))
		mState = BOARD_LEVEL_COMPLETE;

	mModFireDue = 0;
	if ((isModWord) && (mState == BOARD_PLAYING))
		mModFireDue = aModEffects.mEveryTile ? BOARD_ROWS * BOARD_COLS : aModEffects.mFire;
	Refill();
	if ((!isModWord) || (aModEffects.mBonus == MODWORD_BONUS_KEEP))
		AwardSpecialTile(aWord);
	else if (aModEffects.mBonus == MODWORD_BONUS_REMOVE)
		RemoveBonusTiles();

	for (Piece* aPiece : thePath)
		delete aPiece;
	mMoves++;
	Settle();
	return true;
}

// Board::Scramble's refusals in Classic (the dialogs' and the silent ones)
bool BotGame::CanScramble() const
{
	if ((mState != BOARD_PLAYING) || (mFireTileCount > 9))
		return false;

	bool hasCandidate = false;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		Piece* aPiece = mGrid[TopRow(aCol)][aCol];
		if ((aPiece != NULL) && (aPiece->mType == TILE_NORMAL))
			hasCandidate = true;
	}
	if (!hasCandidate)
		return false;

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;
			if (((aPiece->mType == TILE_FIRE) && (aRow == 7)) || (aPiece->mBurning))
				return false;
		}
	}
	return true;
}

// Board::Scramble in Classic (the letters change at once: the game shows them when its flip ends)
bool BotGame::Scramble()
{
	if (!CanScramble())
		return false;

	std::vector<Piece*> aCandidates;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		Piece* aPiece = mGrid[TopRow(aCol)][aCol];
		if ((aPiece != NULL) && (aPiece->mType == TILE_NORMAL))
			aCandidates.push_back(aPiece);
	}

	mScrambles++;
	int aCount = (int)aCandidates.size();
	if (mScrambles < aCount)
		aCount = mScrambles;
	for (int i = 0; i < aCount; i++)
	{
		unsigned int anIndex = mGameRand.Next() % (unsigned int)aCandidates.size();
		Piece* aPiece = aCandidates[anIndex];
		aPiece->SetType(TILE_FIRE, true);
		mFireTileCount++;
		aCandidates.erase(aCandidates.begin() + anIndex);
	}

	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		for (int aRow = 7; aRow >= TopRow(aCol); aRow--)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mType == TILE_FIRE))
				continue;
			aPiece->SetType(TILE_NORMAL, false);
			aPiece->mLetter = (char)RandomLetter();
		}
	}

	BurnTiles();
	Refill();
	mMoves++;
	Settle();
	return true;
}

// Board::Update's tile pass until no tile burns (every other effect of the pass is left out: none draws from
// gGameRand); then the tiles rest where they belong, a complete level goes on to the next (Board::LevelUp)
// and the banner takes up the bonus word
void BotGame::Settle()
{
	for (int aTick = 0; aTick < GREEDY_MAX_SETTLE_TICKS; aTick++)
	{
		bool isBurning = false;
		for (int aRow = 0; (aRow < BOARD_ROWS) && (!isBurning); aRow++)
		{
			for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			{
				if ((mGrid[aRow][aCol] != NULL) && (mGrid[aRow][aCol]->mBurning))
					isBurning = true;
			}
		}
		if (!isBurning)
			break;

		bool aRefill = false;
		std::vector<Piece*> aDeadPieces;
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			int aTargetY = RowY(aCol, 7);
			for (int aRow = 7; aRow >= 0; aRow--)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					continue;

				double aTarget = (double)aTargetY;
				if (aPiece->mY < aTarget)
				{
					aPiece->mVY += 0.2;
					aPiece->mY += aPiece->mVY;
				}
				else if (aPiece->mBurning)
				{
					if (aPiece->mBurnDelay > 0)
						aPiece->mBurnDelay--;
					else
						aPiece->mBurnProgress++;
					if (aPiece->mBurnProgress >= 57)
					{
						mGrid[aPiece->mRow][aPiece->mCol] = NULL;
						aDeadPieces.push_back(aPiece);
						aRefill = true;
					}
				}
				if (aPiece->mY > aTarget)
				{
					aPiece->mVY = 0.0;
					aPiece->mY = aTarget;
				}
				if (aPiece->mY == aTarget)
					aPiece->mVY = 0.0;
				aTargetY = aTargetY + aPiece->mBurnProgress - 57;
			}
		}

		if (aRefill)
			Refill();
		for (Piece* aPiece : aDeadPieces)
			delete aPiece;
	}

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;
			aPiece->mY = (double)RowY(aCol, aRow);
			aPiece->mVY = 0.0;
		}
	}

	if (mState == BOARD_LEVEL_COMPLETE)
	{
		mLevelScore = mScore - Board::GetPointsAtLevel((unsigned int)mLevel);
		mLevelWords = 0;
		mLevel++;
		mState = BOARD_PLAYING;
	}
	ShowBonusWord();
}

// --------------------------------------------------------------------------------------------------------
// GreedyBots

GreedyBots::GreedyBots()
{
	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
	{
		mGames[i] = NULL;
		mResults[i].mEnd = GREEDY_PLAYING;
		mResults[i].mScore = 0;
		mResults[i].mLevel = 0;
		mResults[i].mWords = 0;
	}
	mHasResults = false;
	mElapsedMs = 0.0;
}

GreedyBots::~GreedyBots()
{
	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
		delete mGames[i];
}

void GreedyBots::Start(Board* theBoard)
{
	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
	{
		delete mGames[i];
		mGames[i] = new BotGame();
		mGames[i]->Capture(theBoard);
		mResults[i].mEnd = GREEDY_PLAYING;
		mResults[i].mScore = mGames[i]->mScore;
		mResults[i].mLevel = mGames[i]->mLevel;
		mResults[i].mWords = mGames[i]->mWords;
	}
	mHasResults = true;
}

bool GreedyBots::Finished() const
{
	if (!mHasResults)
		return false;
	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
	{
		if (mResults[i].mEnd == GREEDY_PLAYING)
			return false;
	}
	return true;
}

// The bonus word's points on top of the word's, as Submit adds them
static int BonusPoints(const BotGame* theGame, const SolverWord& theWord)
{
	if ((theGame->mBonusWord.size() == 0) || (_stricmp(theWord.mWord.c_str(), theGame->mBonusWord.c_str()) != 0))
		return 0;
	unsigned int anIndex = (unsigned int)theGame->mBonusWords;
	return gBonusWordPoints[(anIndex > 9) ? 10 : anIndex];
}

// GREEDY_BEST_WORD: a word that ends the game at once last, then the points, then the length
static bool BestWordFirst(const SolverWord& a, const SolverWord& b)
{
	bool aFatal = a.mSafety == SOLVER_FATAL;
	bool bFatal = b.mSafety == SOLVER_FATAL;
	if (aFatal != bFatal)
		return bFatal;
	if (a.mScore != b.mScore)
		return a.mScore > b.mScore;
	return a.mWord.size() > b.mWord.size();
}

// GREEDY_FIRE_FIRST: safer first; then a word with fire tiles, the solver's fire hint order among them;
// else by score
static bool FireFirst(const SolverWord& a, const SolverWord& b)
{
	if (a.mSafety != b.mSafety)
		return a.mSafety < b.mSafety;
	bool aFire = a.mFireTiles > 0;
	bool bFire = b.mFireTiles > 0;
	if (aFire != bFire)
		return aFire;
	return aFire ? SolverBetterForFire(a, b) : SolverBetterByScore(a, b);
}

// GREEDY_LEAST_FIRE, the solver's first hint with "fewerFire" on: safer first; then a word with fire tiles, the
// lowest fire tile taken first; then the fewest new fire tiles, the fire meter's drain counted (mFireCost); then the
// solver's fire hint order or the points. The fewest new fire alone leaves the fire on the board to burn down: on
// Classic seed 1 that bot burned at level 19, where the fire-first bot reached level 44.
static bool LeastFireFirst(const SolverWord& theFirst, const SolverWord& theSecond)
{
	if (theFirst.mSafety != theSecond.mSafety)
		return theFirst.mSafety < theSecond.mSafety;
	bool aFire = theFirst.mFireTiles > 0;
	bool bFire = theSecond.mFireTiles > 0;
	if (aFire != bFire)
		return aFire;
	if ((aFire) && (theFirst.mFireRow != theSecond.mFireRow))
		return theFirst.mFireRow > theSecond.mFireRow;
	if (theFirst.mFireCost != theSecond.mFireCost)
		return theFirst.mFireCost < theSecond.mFireCost;
	return aFire ? SolverBetterForFire(theFirst, theSecond) : SolverBetterByScore(theFirst, theSecond);
}

bool GreedyBots::PlayMove(BotGame* theGame, int theKind)
{
	std::vector<Piece*> aTiles = theGame->Tiles();
	std::vector<SolverWord> aWords;
	FindSolverWords(aTiles, theGame->mApp, &aWords, theGame->mLevel);
	if (theKind == GREEDY_LEAST_FIRE)
	{
		SolverFireRater aRater(aTiles, true, theGame->mLevel, theGame->mLevel > theGame->mLastRefillLevel,
			theGame->mFireMeter, theGame->mSkipRandomFire, theGame->mBonusWord);
		for (SolverWord& aWord : aWords)
			aRater.Rate(&aWord);
	}

	// Every bot: a word that ends the game at once last, then the bonus word (the user's choice), then its own order
	const SolverWord* aBest = NULL;
	bool isBestBonus = false;
	for (size_t i = 0; i < aWords.size(); i++)
	{
		int aBonus = BonusPoints(theGame, aWords[i]);
		aWords[i].mScore += aBonus;
		const SolverWord& aRival = *(aBest ? aBest : &aWords[i]);
		bool isFatal = (aWords[i].mSafety == SOLVER_FATAL);
		bool isBetter;
		if (isFatal != (aRival.mSafety == SOLVER_FATAL))
			isBetter = !isFatal;
		else if ((aBonus > 0) != isBestBonus)
			isBetter = (aBonus > 0);
		else
			isBetter = (theKind == GREEDY_BEST_WORD) ? BestWordFirst(aWords[i], aRival) :
				(theKind == GREEDY_FIRE_FIRST) ? FireFirst(aWords[i], aRival) : LeastFireFirst(aWords[i], aRival);
		if ((aBest == NULL) || (isBetter))
		{
			aBest = &aWords[i];
			isBestBonus = (aBonus > 0);
		}
	}

	if (aBest != NULL)
		return theGame->Submit(aBest->mPath);
	return theGame->Scramble();
}

bool GreedyBots::Update(double theBudgetMs)
{
	if ((!mHasResults) || (Finished()))
		return false;

	std::chrono::steady_clock::time_point aStart = std::chrono::steady_clock::now();
	bool hasEnded = false;
	for (;;)
	{
		// the bot with the fewest moves plays next
		int aKind = -1;
		for (int i = 0; i < NUM_GREEDY_BOTS; i++)
		{
			if ((mResults[i].mEnd == GREEDY_PLAYING) && (mGames[i] != NULL) &&
				((aKind < 0) || (mGames[i]->mMoves < mGames[aKind]->mMoves)))
				aKind = i;
		}
		if (aKind < 0)
			break;

		BotGame* aGame = mGames[aKind];
		bool hasMoved = PlayMove(aGame, aKind);
		GreedyBotResult& aResult = mResults[aKind];
		aResult.mScore = aGame->mScore;
		aResult.mLevel = aGame->mLevel;
		aResult.mWords = aGame->mWords;
		if (aGame->IsOver())
			aResult.mEnd = GREEDY_BURNED;
		else if (!hasMoved)
			aResult.mEnd = GREEDY_STUCK;
		else if (aGame->mMoves >= GREEDY_MAX_MOVES)
			aResult.mEnd = GREEDY_CAPPED;
		if (aResult.mEnd != GREEDY_PLAYING)
		{
			hasEnded = true;
			delete mGames[aKind];
			mGames[aKind] = NULL;
		}

		double anElapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aStart).count();
		if (anElapsed >= theBudgetMs)
			break;
	}
	mElapsedMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - aStart).count();
	return hasEnded;
}

// --------------------------------------------------------------------------------------------------------
// The saved game's record of the bots (after Board's): "BOTS", its version and the number of bots (0: none; 2
// before GREEDY_LEAST_FIRE); per bot its result and whether its game goes on, then that game: its counters, the
// fire meter, the bonus words, gGameRand's whole state and every cell's letter, type and countdown

static void WriteInt(FILE* theFile, int theValue)
{
	fwrite(&theValue, 4, 1, theFile);
}

static bool ReadInt(FILE* theFile, int* theValue)
{
	return fread(theValue, 4, 1, theFile) == 1;
}

static void WriteString(FILE* theFile, const SexyString& theString)
{
	WriteInt(theFile, (int)theString.size());
	if (theString.size() != 0)
		fwrite(theString.data(), 1, theString.size(), theFile);
}

static bool ReadString(FILE* theFile, SexyString* theString)
{
	int aLength = 0;
	if ((!ReadInt(theFile, &aLength)) || (aLength < 0) || (aLength > 64))
		return false;
	theString->assign((size_t)aLength, ' ');
	return (aLength == 0) || (fread(&(*theString)[0], 1, (size_t)aLength, theFile) == (size_t)aLength);
}

void BotGame::Write(FILE* theFile) const
{
	const int aValues[] = { mLastLetterIdx, mLastRefillLevel, mState, mLevel, mSkipRandomFire ? 1 : 0, mRewardStreak,
		mFireTileCount, mBonusWordsGiven, mScore, mWords, mBonusWords, mScrambles, mLevelScore, mLevelWords, mMoves };
	for (int aValue : aValues)
		WriteInt(theFile, aValue);
	fwrite(&mFireMeter, 8, 1, theFile);
	WriteString(theFile, mBonusWord);
	WriteString(theFile, mNextBonusWord);

	uint32_t aRandState[MTRAND_N];
	int aRandIndex = 0;
	mGameRand.GetState(aRandState, &aRandIndex);
	fwrite(aRandState, 4, MTRAND_N, theFile);
	WriteInt(theFile, aRandIndex);

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			WriteInt(theFile, (aPiece != NULL) ? (unsigned char)aPiece->mLetter : 0);
			WriteInt(theFile, (aPiece != NULL) ? aPiece->mType : 0);
			WriteInt(theFile, (aPiece != NULL) ? aPiece->mBurnCountdown : 0);
		}
	}
}

bool BotGame::Read(FILE* theFile, BookWormApp* theApp)
{
	int aValues[15];
	for (int& aValue : aValues)
	{
		if (!ReadInt(theFile, &aValue))
			return false;
	}
	double aFireMeter = 0.0;
	SexyString aBonusWord, aNextBonusWord;
	uint32_t aRandState[MTRAND_N];
	int aRandIndex = 0;
	if ((fread(&aFireMeter, 8, 1, theFile) != 1) || (!ReadString(theFile, &aBonusWord)) ||
		(!ReadString(theFile, &aNextBonusWord)) || (fread(aRandState, 4, MTRAND_N, theFile) != MTRAND_N) ||
		(!ReadInt(theFile, &aRandIndex)) || (!mGameRand.SetState(aRandState, aRandIndex)))
		return false;

	int aCells[BOARD_ROWS][BOARD_COLS][3];
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			for (int i = 0; i < 3; i++)
			{
				if (!ReadInt(theFile, &aCells[aRow][aCol][i]))
					return false;
			}
			int aLetter = aCells[aRow][aCol][0];
			if ((aLetter != 0) && (((aLetter < 'A') || (aLetter > 'Z')) || (aCells[aRow][aCol][1] < TILE_NORMAL) ||
				(aCells[aRow][aCol][1] >= NUM_TILE_TYPES)))
				return false;
		}
	}

	Clear();
	mApp = theApp;
	mLastLetterIdx = aValues[0];
	mLastRefillLevel = aValues[1];
	mState = aValues[2];
	mLevel = aValues[3];
	mSkipRandomFire = aValues[4] != 0;
	mRewardStreak = aValues[5];
	mFireTileCount = aValues[6];
	mBonusWordsGiven = aValues[7];
	mScore = aValues[8];
	mWords = aValues[9];
	mBonusWords = aValues[10];
	mScrambles = aValues[11];
	mLevelScore = aValues[12];
	mLevelWords = aValues[13];
	mMoves = aValues[14];
	mFireMeter = aFireMeter;
	mBonusWord = aBonusWord;
	mNextBonusWord = aNextBonusWord;
	mModFireDue = 0;
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			if (aCells[aRow][aCol][0] == 0)
				continue;
			Piece* aPiece = new Piece();
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = (double)ColX(aCol);
			aPiece->mY = (double)RowY(aCol, aRow);
			aPiece->mLetter = (char)aCells[aRow][aCol][0];
			aPiece->mType = aCells[aRow][aCol][1];
			aPiece->mBurnCountdown = aCells[aRow][aCol][2];
			mGrid[aRow][aCol] = aPiece;
		}
	}
	return true;
}

void GreedyBots::Write(FILE* theFile, const GreedyBots* theBots)
{
	bool hasBots = (theBots != NULL) && (theBots->mHasResults);
	WriteInt(theFile, GREEDY_RECORD_TAG);
	WriteInt(theFile, GREEDY_RECORD_VERSION);
	WriteInt(theFile, hasBots ? (int)NUM_GREEDY_BOTS : 0);
	if (!hasBots)
		return;
	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
	{
		const GreedyBotResult& aResult = theBots->mResults[i];
		const BotGame* aGame = (aResult.mEnd == GREEDY_PLAYING) ? theBots->mGames[i] : NULL;
		WriteInt(theFile, aResult.mEnd);
		WriteInt(theFile, aResult.mScore);
		WriteInt(theFile, aResult.mLevel);
		WriteInt(theFile, aResult.mWords);
		WriteInt(theFile, (aGame != NULL) ? 1 : 0);
		if (aGame != NULL)
			aGame->Write(theFile);
	}
}

bool GreedyBots::Read(FILE* theFile, BookWormApp* theApp)
{
	int aHeader[3];
	for (int& aValue : aHeader)
	{
		if (!ReadInt(theFile, &aValue))
			return false;
	}
	if ((aHeader[0] != GREEDY_RECORD_TAG) || (aHeader[1] != GREEDY_RECORD_VERSION) || (aHeader[2] < GREEDY_LEAST_FIRE) ||
		(aHeader[2] > NUM_GREEDY_BOTS))
		return false;

	// the bots the record has not: absent
	GreedyBotResult aResults[NUM_GREEDY_BOTS] = {};
	BotGame* aGames[NUM_GREEDY_BOTS] = {};
	for (int i = aHeader[2]; i < NUM_GREEDY_BOTS; i++)
		aResults[i].mEnd = GREEDY_ABSENT;
	bool isGood = true;
	for (int i = 0; (i < aHeader[2]) && (isGood); i++)
	{
		int aValues[5];
		for (int& aValue : aValues)
		{
			if (!ReadInt(theFile, &aValue))
				isGood = false;
		}
		if ((!isGood) || (aValues[0] < GREEDY_PLAYING) || (aValues[0] > GREEDY_ABSENT) ||
			((aValues[0] == GREEDY_PLAYING) != (aValues[4] == 1)))
		{
			isGood = false;
			break;
		}
		aResults[i].mEnd = aValues[0];
		aResults[i].mScore = aValues[1];
		aResults[i].mLevel = aValues[2];
		aResults[i].mWords = aValues[3];
		if (aValues[4] == 1)
		{
			aGames[i] = new BotGame();
			isGood = aGames[i]->Read(theFile, theApp);
		}
	}
	if (!isGood)
	{
		for (int i = 0; i < NUM_GREEDY_BOTS; i++)
			delete aGames[i];
		return false;
	}

	for (int i = 0; i < NUM_GREEDY_BOTS; i++)
	{
		delete mGames[i];
		mGames[i] = aGames[i];
		mResults[i] = aResults[i];
	}
	mHasResults = true;
	return true;
}
