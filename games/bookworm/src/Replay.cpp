// port(mod): mod "replays", replays of games (Replay.h). Not part of the original game.

#include "Replay.h"
#include "Board.h"
#include "BookWormApp.h"
#include "DailyBoard.h"
#include "LevelUpWidget.h"
#include "Lex.h"
#include "OverlayWidget.h"
#include "Effect.h"
#include "ModFeatures.h"
#include "Piece.h"
#include "StatKeeper.h"
#include "Undo.h"
#include "Compat2004.h"

#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

using namespace Sexy;

unsigned int Sexy::gLexCrtUpdates = 0;

// ---------------------------------------------------------------------------------------------------------
// The file

namespace
{

uint32_t Tag(const char* theName)
{
	return (uint32_t)(uint8_t) theName[0] | ((uint32_t)(uint8_t) theName[1] << 8) | ((uint32_t)(uint8_t) theName[2] << 16) |
		((uint32_t)(uint8_t) theName[3] << 24);
}

void PutU8(std::vector<uint8_t>* theOut, uint32_t theValue)
{
	theOut->push_back((uint8_t) theValue);
}

void PutU16(std::vector<uint8_t>* theOut, uint32_t theValue)
{
	PutU8(theOut, theValue & 0xff);
	PutU8(theOut, (theValue >> 8) & 0xff);
}

void PutU32(std::vector<uint8_t>* theOut, uint32_t theValue)
{
	PutU16(theOut, theValue & 0xffff);
	PutU16(theOut, theValue >> 16);
}

void PutVarint(std::vector<uint8_t>* theOut, uint32_t theValue)
{
	while (theValue >= 0x80)
	{
		PutU8(theOut, (theValue & 0x7f) | 0x80);
		theValue >>= 7;
	}
	PutU8(theOut, theValue);
}

void PutZigzag(std::vector<uint8_t>* theOut, int32_t theValue)
{
	PutVarint(theOut, ((uint32_t) theValue << 1) ^ (uint32_t)(theValue >> 31));
}

void PutString(std::vector<uint8_t>* theOut, const std::string& theText)
{
	size_t aLen = std::min(theText.size(), (size_t) 0xffff);
	PutU16(theOut, (uint32_t) aLen);
	theOut->insert(theOut->end(), theText.begin(), theText.begin() + aLen);
}

void PutRand(std::vector<uint8_t>* theOut, const Replay::RandState& theState)
{
	for (int i = 0; i < MTRAND_N; i++)
		PutU32(theOut, theState.mWords[i]);
	PutU32(theOut, (uint32_t) theState.mIndex);
}

void PutRecord(std::vector<uint8_t>* theOut, const char* theTag, const std::vector<uint8_t>& thePayload)
{
	PutU32(theOut, Tag(theTag));
	PutU32(theOut, (uint32_t) thePayload.size());
	theOut->insert(theOut->end(), thePayload.begin(), thePayload.end());
}

// A bounds-checked reader over one record
struct Reader
{
	const uint8_t* mData;
	size_t mSize;
	size_t mPos = 0;
	bool mOk = true;

	Reader(const uint8_t* theData, size_t theSize) : mData(theData), mSize(theSize) {}

	uint32_t U8()
	{
		if (mPos + 1 > mSize)
		{
			mOk = false;
			return 0;
		}
		return mData[mPos++];
	}
	uint32_t U16() { uint32_t a = U8(); return a | (U8() << 8); }
	uint32_t U32() { uint32_t a = U16(); return a | (U16() << 16); }
	uint32_t Varint()
	{
		uint32_t aValue = 0;
		for (int aShift = 0; aShift < 35; aShift += 7)
		{
			uint32_t aByte = U8();
			if (!mOk)
				return 0;
			aValue |= (aByte & 0x7f) << aShift;
			if ((aByte & 0x80) == 0)
				return aValue;
		}
		mOk = false;
		return 0;
	}
	int32_t Zigzag()
	{
		uint32_t aValue = Varint();
		return (int32_t)((aValue >> 1) ^ (0u - (aValue & 1)));
	}
	std::string String()
	{
		uint32_t aLen = U16();
		std::string aText;
		for (uint32_t i = 0; (i < aLen) && (mOk); i++)
			aText += (char) U8();
		return aText;
	}
	void Rand(Replay::RandState* theState)
	{
		for (int i = 0; i < MTRAND_N; i++)
			theState->mWords[i] = U32();
		theState->mIndex = (int) U32();
	}
};

}

void Replay::RandState::Get(const MTRand& theRand)
{
	theRand.GetState(mWords, &mIndex);
}

bool Replay::RandState::Set(MTRand* theRand) const
{
	return theRand->SetState(mWords, mIndex);
}

void Replay::File::Write(std::vector<uint8_t>* theOut) const
{
	theOut->clear();
	theOut->insert(theOut->end(), { 'B', 'W', 'R', 'P' });
	PutU16(theOut, FORMAT);
	PutU16(theOut, 0);

	std::vector<uint8_t> aPayload(mMeta.begin(), mMeta.end());
	PutRecord(theOut, "META", aPayload);

	aPayload.clear();
	PutU8(&aPayload, (uint32_t) mMode);
	PutU8(&aPayload, mAppFlags);
	PutU32(&aPayload, mCrtSeed);
	PutU32(&aPayload, mDailyDate);
	PutU32(&aPayload, (uint32_t) mLastLetterIdx);
	PutU32(&aPayload, (uint32_t) mLastRefillLevel);
	PutString(&aPayload, mNextBonusWord);
	PutRand(&aPayload, mGameRand);
	PutRand(&aPayload, mBoardRand);
	PutRand(&aPayload, mEffectRand);
	PutString(&aPayload, mScoreKey);
	PutRecord(theOut, "GAME", aPayload);

	aPayload.clear();
	PutU32(&aPayload, (uint32_t) mEvents.size());
	uint32_t aTime = 0;
	for (const Event& anEvent : mEvents)
	{
		PutVarint(&aPayload, anEvent.mTime - aTime);
		aTime = anEvent.mTime;
		PutU8(&aPayload, (uint32_t) anEvent.mOp);
		PutVarint(&aPayload, (uint32_t) anEvent.mArgs.size());
		for (int32_t anArg : anEvent.mArgs)
			PutZigzag(&aPayload, anArg);
	}
	PutRecord(theOut, "EVTS", aPayload);

	aPayload.clear();
	PutU32(&aPayload, mEndTick);
	PutU32(&aPayload, (uint32_t) mScore);
	PutU32(&aPayload, (uint32_t) mLevel);
	PutU32(&aPayload, (uint32_t) mWords);
	PutRecord(theOut, "END ", aPayload);
}

bool Replay::File::Read(const std::vector<uint8_t>& theData, std::string* theError)
{
	*this = File();
	if ((theData.size() < 8) || (memcmp(&theData[0], "BWRP", 4) != 0))
	{
		*theError = "not a BookWorm replay";
		return false;
	}
	if (theData.size() > MAX_FILE_SIZE)
	{
		*theError = "the file is too large";
		return false;
	}
	Reader aHeader(&theData[4], 4);
	uint32_t aFormat = aHeader.U16();
	if (aFormat != FORMAT)
	{
		*theError = "replay format " + std::to_string(aFormat) + " is not supported";
		return false;
	}

	bool hasGame = false;
	size_t aPos = 8;
	while (aPos < theData.size())
	{
		if (aPos + 8 > theData.size())
		{
			*theError = "the file is cut short";
			return false;
		}
		Reader aRecordHeader(&theData[aPos], 8);
		uint32_t aTag = aRecordHeader.U32();
		uint32_t aLen = aRecordHeader.U32();
		aPos += 8;
		if (aLen > theData.size() - aPos)
		{
			*theError = "the file is cut short";
			return false;
		}
		Reader aReader(aLen ? &theData[aPos] : NULL, aLen);
		if (aTag == Tag("META"))
			mMeta.assign((const char*) (aLen ? &theData[aPos] : NULL), aLen);
		else if (aTag == Tag("GAME"))
		{
			mMode = (int) aReader.U8();
			mAppFlags = aReader.U8();
			mCrtSeed = aReader.U32();
			mDailyDate = aReader.U32();
			mLastLetterIdx = (int32_t) aReader.U32();
			mLastRefillLevel = (int32_t) aReader.U32();
			mNextBonusWord = aReader.String();
			aReader.Rand(&mGameRand);
			aReader.Rand(&mBoardRand);
			aReader.Rand(&mEffectRand);
			mScoreKey = aReader.String();
			MTRand aCheck;
			if ((!aReader.mOk) || (mMode < GAMEMODE_CLASSIC) || (mMode > GAMEMODE_ACTION) || (!mGameRand.Set(&aCheck)) ||
				(!mBoardRand.Set(&aCheck)) || (!mEffectRand.Set(&aCheck)))
			{
				*theError = "the game record is damaged";
				return false;
			}
			hasGame = true;
		}
		else if (aTag == Tag("EVTS"))
		{
			uint32_t aCount = aReader.U32();
			if (aCount > MAX_ITEMS)
			{
				*theError = "too many events";
				return false;
			}
			mEvents.reserve(aCount);
			uint32_t aTime = 0;
			for (uint32_t i = 0; (i < aCount) && (aReader.mOk); i++)
			{
				Event anEvent;
				aTime += aReader.Varint();
				anEvent.mTime = aTime;
				anEvent.mOp = (int) aReader.U8();
				uint32_t anArgs = aReader.Varint();
				if ((anEvent.mOp < 1) || (anEvent.mOp >= NUM_OPS) || (anArgs > MAX_ARGS))
				{
					*theError = "unknown event " + std::to_string(anEvent.mOp);
					return false;
				}
				for (uint32_t j = 0; (j < anArgs) && (aReader.mOk); j++)
					anEvent.mArgs.push_back(aReader.Zigzag());
				mEvents.push_back(anEvent);
			}
			if (!aReader.mOk)
			{
				*theError = "the events are damaged";
				return false;
			}
		}
		else if (aTag == Tag("END "))
		{
			mEndTick = aReader.U32();
			mScore = (int32_t) aReader.U32();
			mLevel = (int32_t) aReader.U32();
			mWords = (int32_t) aReader.U32();
			if (!aReader.mOk)
			{
				*theError = "the end record is damaged";
				return false;
			}
			mHasEnd = true;
		}
		aPos += aLen;
	}
	if ((!hasGame) || (!mHasEnd))
	{
		*theError = "the replay is incomplete";
		return false;
	}
	return true;
}

bool Replay::LoadFile(const std::string& thePath, File* theFile, std::string* theError)
{
	FILE* aFile = fopen(thePath.c_str(), "rb");
	if (aFile == NULL)
	{
		*theError = "cannot open " + thePath;
		return false;
	}
	std::vector<uint8_t> aData;
	uint8_t aBuffer[65536];
	size_t aRead;
	while ((aRead = fread(aBuffer, 1, sizeof(aBuffer), aFile)) > 0)
	{
		aData.insert(aData.end(), aBuffer, aBuffer + aRead);
		if (aData.size() > MAX_FILE_SIZE)
			break;
	}
	fclose(aFile);
	return theFile->Read(aData, theError);
}

bool Replay::SaveFile(const std::string& thePath, const File& theFile)
{
	std::vector<uint8_t> aData;
	theFile.Write(&aData);
	FILE* aFile = fopen(thePath.c_str(), "wb");
	if (aFile == NULL)
		return false;
	bool isOk = (fwrite(aData.data(), 1, aData.size(), aFile) == aData.size());
	isOk = (fclose(aFile) == 0) && isOk;
	return isOk;
}

// ---------------------------------------------------------------------------------------------------------
// Recording

static const char* const kModeNames[2] = { "classic", "action" };

// The selection as cells, row * BOARD_COLS + col (-1 for a tile that is not on the board)
static std::vector<int32_t> SelectionCells(Board* theBoard)
{
	std::vector<int32_t> aCells;
	for (Piece* aPiece : theBoard->mSelection)
		aCells.push_back(aPiece != NULL ? aPiece->mRow * BOARD_COLS + aPiece->mCol : -1);
	return aCells;
}

// A seed for rand() from the clock: never from the game's generators, whose draws a watch would not make
static uint32_t NewSeed()
{
	uint64_t aCount = SDL_GetPerformanceCounter();
	return (uint32_t) time(NULL) ^ (uint32_t) aCount ^ (uint32_t) (aCount >> 32) ^ (SDL_GetTicks() * 2654435761u);
}

ReplayRecorder::ReplayRecorder(Board* theBoard)
{
	mBoard = theBoard;
	mLexUpdates = gLexCrtUpdates;
	mTicked = false;
	mInTick = false;
	mLexOver = false;
}

void ReplayRecorder::Start()
{
	BookWormApp* anApp = mBoard->mApp;
	// rand() from here on is the game's: Lex's idle animations come at the same ticks in a watch
	mFile.mCrtSeed = NewSeed();
	CrtSRand(mFile.mCrtSeed);
	mFile.mMode = anApp->mGameMode;
	mFile.mAppFlags = (anApp->mShowDefs ? Replay::APP_SHOW_DEFS : 0) | (anApp->mShowHints ? Replay::APP_SHOW_HINTS : 0) |
		(anApp->mWatchOutHintShown ? Replay::APP_WATCH_OUT_HINT : 0) |
		(anApp->mFireAlertHintShown ? Replay::APP_FIRE_ALERT_HINT : 0) |
		(anApp->mGreenTileHintShown ? Replay::APP_GREEN_TILE_HINT : 0) |
		(anApp->mScrambleHintShown ? Replay::APP_SCRAMBLE_HINT : 0);
	mFile.mDailyDate = (mBoard->mDaily != NULL) ? (uint32_t) mBoard->mDaily->mDate : 0;
	int aLastLetterIdx, aLastRefillLevel;
	Board::GetGlobalState(&aLastLetterIdx, &aLastRefillLevel);
	mFile.mLastLetterIdx = aLastLetterIdx;
	mFile.mLastRefillLevel = aLastRefillLevel;
	mFile.mNextBonusWord = anApp->mBonusWord;
	mFile.mGameRand.Get(*gGameRand);
	mFile.mBoardRand.Get(*mBoard->mRand);
	mFile.mScoreKey = Platform::ModManager::Get().GetScoreKey();
	mLexUpdates = gLexCrtUpdates;
}

void ReplayRecorder::Flush()
{
	std::vector<int32_t> aCells = SelectionCells(mBoard);
	if (aCells != mSelection)
	{
		mSelection = aCells;
		AddOp(Replay::OP_SELECT, aCells);
	}
	bool isLexOver = (mBoard->mLex != NULL) && (mBoard->mLex->mIsOver);
	if (isLexOver != mLexOver)
	{
		mLexOver = isLexOver;
		AddOp(Replay::OP_LEX, { isLexOver ? 1 : 0 });
	}
}

void ReplayRecorder::BeforeTick()
{
	mInTick = false;
	Flush();

	// Lex updates once between two ticks; at the first tick and where he did not (a dialog was up, say), rand() is
	// seeded again, at the same time in a watch
	unsigned int aLexUpdates = gLexCrtUpdates - mLexUpdates;
	mLexUpdates = gLexCrtUpdates;
	if (!mTicked)
		mFile.mEffectRand.Get(*mBoard->mOverlay->mEffectMgr->mRand);
	if ((!mTicked) || (aLexUpdates != 1))
	{
		int32_t aSeed = (int32_t) NewSeed();
		CrtSRand((unsigned int) aSeed);
		AddOp(Replay::OP_CRT, { aSeed });
	}
	mTicked = true;
}

void ReplayRecorder::InTick()
{
	mInTick = true;
}

void ReplayRecorder::AfterTick()
{
	Flush();
	mInTick = false;
}

void ReplayRecorder::AddOp(int theOp, const std::vector<int32_t>& theArgs)
{
	if ((theOp != Replay::OP_SELECT) && (theOp != Replay::OP_LEX))
		Flush();
	if (mFile.mEvents.size() >= Replay::MAX_ITEMS)
		return;
	Replay::Event anEvent;
	anEvent.mTime = (uint32_t) mBoard->mUpdateCnt * 2 + (mInTick ? 0 : 1);
	anEvent.mOp = theOp;
	anEvent.mArgs = theArgs;
	mFile.mEvents.push_back(anEvent);
}

// theText as a JSON string
static std::string JsonString(const std::string& theText)
{
	std::string anOut = "\"";
	for (unsigned char aChar : theText)
	{
		if ((aChar == '"') || (aChar == '\\'))
		{
			anOut += '\\';
			anOut += (char) aChar;
		}
		else if (aChar < 0x20)
		{
			char aBuffer[8];
			snprintf(aBuffer, sizeof(aBuffer), "\\u%04x", aChar);
			anOut += aBuffer;
		}
		else
			anOut += (char) aChar;
	}
	return anOut + "\"";
}

// Replays (*.bwr) not pinned ("-keep") beyond the newest theKeep are deleted; names start with the date, so the
// newest sort last
static void PruneReplays(const std::string& theDir, int theKeep)
{
	std::vector<std::string> aNames;
	for (const std::string& aName : Platform::ListDirectory(theDir))
	{
		if ((aName.size() > 4) && (aName.compare(aName.size() - 4, 4, ".bwr") == 0) && (aName.find("-keep") == std::string::npos))
			aNames.push_back(aName);
	}
	std::sort(aNames.begin(), aNames.end());
	for (size_t i = 0; i + (size_t) theKeep < aNames.size(); i++)
		remove((theDir + "/" + aNames[i]).c_str());
}

std::string ReplayRecorder::Finish(bool thePinned)
{
	Flush();
	mFile.mHasEnd = true;
	mFile.mEndTick = (uint32_t) mBoard->mUpdateCnt;
	mFile.mScore = mBoard->mGameStats->mScore;
	mFile.mLevel = mBoard->mLevel;
	mFile.mWords = mBoard->mGameStats->mWords;

	time_t aNow = time(NULL);
	struct tm aTime = *localtime(&aNow);
	char aDate[32];
	strftime(aDate, sizeof(aDate), "%Y-%m-%d %H:%M:%S", &aTime);
	char aStamp[32];
	strftime(aStamp, sizeof(aStamp), "%Y%m%d-%H%M%S", &aTime);

	const char* aMode = kModeNames[(mFile.mMode == GAMEMODE_ACTION) ? 1 : 0];
	std::string aMods;
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
		aMods += std::string(aMods.empty() ? "" : ", ") + JsonString(aMod->mId + "@" + aMod->mVersion);
	mFile.mMeta = std::string("{\"game\": \"BookWorm Deluxe (byo-pop)\", \"date\": ") + JsonString(aDate) +
		", \"mode\": " + JsonString(aMode) + ", \"score\": " + std::to_string(mFile.mScore) + ", \"level\": " +
		std::to_string(mFile.mLevel + 1) + ", \"words\": " + std::to_string(mFile.mWords) + ", \"daily\": " +
		(mFile.mDailyDate ? std::to_string(mFile.mDailyDate) : std::string("null")) + ", \"mods\": [" + aMods +
		"], \"scoreKey\": " + JsonString(mFile.mScoreKey) + "}";

	std::string aDir = mBoard->mApp->mReplayDir;
	Platform::MakeDirs(aDir);
	std::string aBase = aDir + "/" + aStamp + "-" + aMode + "-" + std::to_string(mFile.mScore);
	std::string aSuffix = thePinned ? "-keep.bwr" : ".bwr";
	std::string aPath = aBase + aSuffix;
	for (int n = 2; Platform::IsFile(aPath) && (n < 100); n++)
		aPath = aBase + "-" + std::to_string(n) + aSuffix;
	if (!Replay::SaveFile(aPath, mFile))
	{
		SDL_Log("replay: cannot write %s", aPath.c_str());
		return "";
	}
	PruneReplays(aDir, ModFeatureInt("replays", "keep", 50, 1, 10000));
	Platform::SyncUserData();
	SDL_Log("replay: %s (%d events)", aPath.c_str(), (int) mFile.mEvents.size());
	return aPath;
}

// ---------------------------------------------------------------------------------------------------------
// Playback

ReplayPlayer::ReplayPlayer(Board* theBoard, const Replay::File& theFile)
	: mFile(theFile)
{
	mBoard = theBoard;
}

void ReplayPlayer::Start()
{
	BookWormApp* anApp = mBoard->mApp;
	mFile.mGameRand.Set(gGameRand);
	mFile.mBoardRand.Set(mBoard->mRand);
	Board::SetGlobalState(mFile.mLastLetterIdx, mFile.mLastRefillLevel);
	anApp->mBonusWord = mFile.mNextBonusWord;
	CrtSRand(mFile.mCrtSeed);
	if (mBoard->mDaily != NULL)
		mBoard->mDaily->mDate = (int) mFile.mDailyDate;   // the greeting names the recording's day
}

void ReplayPlayer::Apply(bool isInTick)
{
	uint32_t aTick = (uint32_t) mBoard->mUpdateCnt;
	uint32_t aTime = aTick * 2 + (isInTick ? 0 : 1);
	if ((!isInTick) && (!mStarted))
	{
		mStarted = true;
		mFile.mEffectRand.Set(mBoard->mOverlay->mEffectMgr->mRand);
	}
	mApplying = true;
	while ((mNextEvent < mFile.mEvents.size()) && (mFile.mEvents[mNextEvent].mTime <= aTime))
	{
		const Replay::Event& anEvent = mFile.mEvents[mNextEvent++];
		if ((anEvent.mTime < aTime) && (!mDesync))
		{
			// A move whose time the board never had: it has gone another way
			mDesync = true;
			SDL_Log("replay: desync at time %u (a move of time %u)", (unsigned) aTime, (unsigned) anEvent.mTime);
		}
		const std::vector<int32_t>& anArgs = anEvent.mArgs;
		switch (anEvent.mOp)
		{
		case Replay::OP_SELECT:
			if (anArgs != SelectionCells(mBoard))
			{
				std::vector<Piece*> aPath;
				for (int32_t aCell : anArgs)
				{
					Piece* aPiece = ((aCell >= 0) && (aCell < BOARD_ROWS * BOARD_COLS)) ?
						mBoard->mBoard[aCell / BOARD_COLS][aCell % BOARD_COLS] : NULL;
					if (aPiece == NULL)
					{
						if (!mDesync)
							SDL_Log("replay: desync at tick %u (no tile at cell %d)", (unsigned) aTick, (int) aCell);
						mDesync = true;
						aPath.clear();
						break;
					}
					aPath.push_back(aPiece);
				}
				if (aPath.empty())
				{
					std::for_each(mBoard->mSelection.begin(), mBoard->mSelection.end(), Piece::Deselect);
					mBoard->ClearWord();
				}
				else
					mBoard->SelectTiles(aPath);
			}
			break;
		case Replay::OP_SUBMIT:
			if (anArgs.size() >= 3)
			{
				mBoard->mSelectMode = anArgs[2];
				mBoard->Submit(anArgs[0], anArgs[1]);
			}
			break;
		case Replay::OP_SCRAMBLE:
			mBoard->Scramble();
			break;
		case Replay::OP_UNDO:
			if (mBoard->mUndo != NULL)
				mBoard->mUndo->Restore();
			else
				mDesync = true;
			break;
		case Replay::OP_NEXT:
			if ((!anArgs.empty()) && (anArgs[0] == 1) && (mBoard->mLevelUpWidget != NULL))
				mBoard->mLevelUpWidget->ButtonDepress(LevelUpWidget::LEVELUP_ID_CONTINUE);
			else if ((mBoard->mSummaryShown) && (mBoard->mState != BOARD_GAMEOVER))
				mBoard->ButtonDepress(BOARD_BUTTON_SUBMIT);     // any button leaves a level's summary
			else
				mDesync = true;
			break;
		case Replay::OP_CRT:
			if (!anArgs.empty())
				CrtSRand((unsigned int) anArgs[0]);
			break;
		case Replay::OP_LEX:
			if ((!anArgs.empty()) && (anArgs[0] == 2) && (mBoard->mLex != NULL))
				mBoard->mLex->mGlow = 0.0;
			else if (!anArgs.empty())
				mLexOver = (anArgs[0] == 1);
			break;
		}
	}
	mApplying = false;
}

void ReplayPlayer::CheckEnd()
{
	if (mEnded)
		return;
	mEnded = true;
	uint32_t aTick = (uint32_t) mBoard->mUpdateCnt;
	StatKeeper* aStats = mBoard->mGameStats;
	mVerified = (!mDesync) && (aTick == mFile.mEndTick) && (aStats->mScore == mFile.mScore) &&
		(mBoard->mLevel == mFile.mLevel) && (aStats->mWords == mFile.mWords) && (mNextEvent == mFile.mEvents.size());
	SDL_Log("replay: %s: end tick %u (recorded %u), score %d (%d), level %d (%d), words %d (%d)",
		mVerified ? "verified" : "DIFFERENT", (unsigned) aTick, (unsigned) mFile.mEndTick, (int) aStats->mScore,
		(int) mFile.mScore, mBoard->mLevel + 1, mFile.mLevel + 1, (int) aStats->mWords, (int) mFile.mWords);
}
