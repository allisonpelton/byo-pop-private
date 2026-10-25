#include "Replay.h"
#include "Board.h"
#include "WinBMApp.h"
#include "Res.h"

#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "platform/WebPersist.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

using namespace Sexy;

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
};

}

int Replay::OpArgCount(int theOp)
{
	switch (theOp)
	{
	case OP_CLICK:
	case OP_MOVE:
		return 2;
	case OP_LEAVE:
	case OP_NEXT:
	case OP_HINT:
		return 0;
	}
	return -1;
}

void Replay::File::Write(std::vector<uint8_t>* theOut) const
{
	theOut->clear();
	theOut->insert(theOut->end(), { 'B', 'M', 'R', 'P' });
	PutU16(theOut, FORMAT);
	PutU16(theOut, 0);

	std::vector<uint8_t> aPayload(mMeta.begin(), mMeta.end());
	PutRecord(theOut, "META", aPayload);

	aPayload.clear();
	PutU8(&aPayload, (uint32_t) mGameType);
	PutU8(&aPayload, (uint32_t) mSkill);
	PutU8(&aPayload, mUserSeeded ? 1 : 0);
	PutU8(&aPayload, mAppFlags);
	PutU32(&aPayload, mRngSeed);
	PutU32(&aPayload, mGameSeed);
	PutU16(&aPayload, (uint32_t) mScoreKey.size());
	aPayload.insert(aPayload.end(), mScoreKey.begin(), mScoreKey.end());
	PutRecord(theOut, "GAME", aPayload);

	aPayload.clear();
	PutU32(&aPayload, (uint32_t) mEvents.size());
	uint32_t aTick = 0;
	for (const Event& anEvent : mEvents)
	{
		PutVarint(&aPayload, anEvent.mTick - aTick);
		aTick = anEvent.mTick;
		PutU8(&aPayload, (uint32_t) anEvent.mOp);
		if (OpArgCount(anEvent.mOp) == 2)
		{
			PutZigzag(&aPayload, anEvent.mX);
			PutZigzag(&aPayload, anEvent.mY);
		}
	}
	PutRecord(theOut, "EVTS", aPayload);

	aPayload.clear();
	PutU32(&aPayload, (uint32_t) mBagColumns.size());
	for (uint16_t aValue : mBagColumns)
		PutU16(&aPayload, aValue);
	PutRecord(theOut, "BAGS", aPayload);

	aPayload.clear();
	PutU32(&aPayload, mEndTick);
	PutU32(&aPayload, (uint32_t) mScore);
	PutU32(&aPayload, (uint32_t) mLevel);
	PutU32(&aPayload, (uint32_t) mCoins);
	PutRecord(theOut, "END ", aPayload);
}

bool Replay::File::Read(const std::vector<uint8_t>& theData, std::string* theError)
{
	*this = File();
	if ((theData.size() < 8) || (memcmp(&theData[0], "BMRP", 4) != 0))
	{
		*theError = "not a Big Money replay";
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
		Reader r(aLen ? &theData[aPos] : NULL, aLen);
		if (aTag == Tag("META"))
			mMeta.assign((const char*) (aLen ? &theData[aPos] : NULL), aLen);
		else if (aTag == Tag("GAME"))
		{
			mGameType = (int) r.U8();
			mSkill = (int) r.U8();
			mUserSeeded = (r.U8() != 0);
			mAppFlags = r.U8();
			mRngSeed = r.U32();
			mGameSeed = r.U32();
			uint32_t aKeyLen = r.U16();
			for (uint32_t i = 0; (i < aKeyLen) && (r.mOk); i++)
				mScoreKey += (char) r.U8();
			if ((!r.mOk) || (mGameType < 0) || (mGameType > 2) || (mSkill < 1) || (mSkill > 3))
			{
				*theError = "the game record is damaged";
				return false;
			}
			hasGame = true;
		}
		else if (aTag == Tag("EVTS"))
		{
			uint32_t aCount = r.U32();
			if (aCount > MAX_ITEMS)
			{
				*theError = "too many events";
				return false;
			}
			mEvents.reserve(aCount);
			uint32_t aTick = 0;
			for (uint32_t i = 0; (i < aCount) && (r.mOk); i++)
			{
				Event anEvent;
				aTick += r.Varint();
				anEvent.mTick = aTick;
				anEvent.mOp = (int) r.U8();
				int anArgs = OpArgCount(anEvent.mOp);
				if (anArgs < 0)
				{
					*theError = "unknown event " + std::to_string(anEvent.mOp);
					return false;
				}
				if (anArgs == 2)
				{
					anEvent.mX = r.Zigzag();
					anEvent.mY = r.Zigzag();
				}
				mEvents.push_back(anEvent);
			}
			if (!r.mOk)
			{
				*theError = "the events are damaged";
				return false;
			}
		}
		else if (aTag == Tag("BAGS"))
		{
			uint32_t aCount = r.U32();
			if ((aCount > MAX_ITEMS) || ((size_t) aCount * 2 + 4 > aLen))
			{
				*theError = "the bag draws are damaged";
				return false;
			}
			for (uint32_t i = 0; i < aCount; i++)
				mBagColumns.push_back((uint16_t) r.U16());
		}
		else if (aTag == Tag("END "))
		{
			mEndTick = r.U32();
			mScore = (int32_t) r.U32();
			mLevel = (int32_t) r.U32();
			mCoins = (int32_t) r.U32();
			if (!r.mOk)
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

static const char* const kModeNames[3] = { "action", "strategy", "puzzle" };
static const char* const kSkillNames[4] = { "", "easy", "normal", "hard" };

ReplayRecorder::ReplayRecorder(WinBMApp* theApp, uint32_t theRngSeed, bool theUserSeeded, uint32_t theGameSeed)
{
	mApp = theApp;
	mFile.mGameType = theApp->mGameType;
	mFile.mSkill = theApp->mSkillLevel;
	mFile.mUserSeeded = theUserSeeded;
	mFile.mRngSeed = theRngSeed;
	mFile.mGameSeed = theUserSeeded ? theGameSeed : 0;
	mFile.mAppFlags = (theApp->mShowInstructor ? Replay::APP_SHOW_INSTRUCTOR : 0) |
		(theApp->mHasClickedGroup ? Replay::APP_HAS_CLICKED_GROUP : 0) |
		(theApp->mDangerHintShown ? Replay::APP_DANGER_HINT_SHOWN : 0) |
		(theApp->mBagCollectedOnce ? Replay::APP_BAG_COLLECTED_ONCE : 0) |
		(theApp->mBombSeenOnce ? Replay::APP_BOMB_SEEN_ONCE : 0);
	mFile.mScoreKey = Platform::ModManager::Get().GetScoreKey();
}

void ReplayRecorder::AddEvent(uint32_t theTick, int theOp, int theX, int theY)
{
	if (mFile.mEvents.size() >= Replay::MAX_ITEMS)
		return;
	std::vector<Replay::Event>& anEvents = mFile.mEvents;
	if ((theOp == Replay::OP_MOVE) && (!anEvents.empty()) && (anEvents.back().mOp == Replay::OP_MOVE) &&
		(anEvents.back().mTick == theTick))
	{
		anEvents.back().mX = theX;
		anEvents.back().mY = theY;
		return;
	}
	Replay::Event anEvent;
	anEvent.mTick = theTick;
	anEvent.mOp = theOp;
	anEvent.mX = theX;
	anEvent.mY = theY;
	anEvents.push_back(anEvent);
}

void ReplayRecorder::AddBagColumn(int theValue)
{
	if (mFile.mBagColumns.size() < Replay::MAX_ITEMS)
		mFile.mBagColumns.push_back((uint16_t) theValue);
}

// theText as a JSON string
static std::string JsonString(const std::string& theText)
{
	std::string anOut = "\"";
	for (unsigned char c : theText)
	{
		if ((c == '"') || (c == '\\'))
		{
			anOut += '\\';
			anOut += (char) c;
		}
		else if (c < 0x20)
		{
			char aBuffer[8];
			snprintf(aBuffer, sizeof(aBuffer), "\\u%04x", c);
			anOut += aBuffer;
		}
		else
			anOut += (char) c;
	}
	return anOut + "\"";
}

// Replays (*.bmr) not pinned ("-keep") beyond the newest theKeep are deleted; names start with the date,
// so the newest sort last
static void PruneReplays(const std::string& theDir, int theKeep)
{
	std::vector<std::string> aNames;
	for (const std::string& aName : Platform::ListDirectory(theDir))
	{
		if ((aName.size() > 4) && (aName.compare(aName.size() - 4, 4, ".bmr") == 0) && (aName.find("-keep") == std::string::npos))
			aNames.push_back(aName);
	}
	std::sort(aNames.begin(), aNames.end());
	for (size_t i = 0; i + (size_t) theKeep < aNames.size(); i++)
		remove((theDir + "/" + aNames[i]).c_str());
}

std::string ReplayRecorder::Finish(Board* theBoard, bool thePinned)
{
	mFile.mHasEnd = true;
	mFile.mEndTick = (uint32_t) theBoard->mUpdateCnt;
	mFile.mScore = theBoard->mScore;
	mFile.mLevel = theBoard->mLevel;
	mFile.mCoins = theBoard->mGameCoinsCleared;

	time_t aNow = time(NULL);
	struct tm aTime = *localtime(&aNow);
	char aDate[32];
	strftime(aDate, sizeof(aDate), "%Y-%m-%d %H:%M:%S", &aTime);
	char aStamp[32];
	strftime(aStamp, sizeof(aStamp), "%Y%m%d-%H%M%S", &aTime);

	int aMode = std::min(std::max(mFile.mGameType, 0), 2);
	int aSkill = std::min(std::max(mFile.mSkill, 1), 3);
	std::string aMods;
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
		aMods += std::string(aMods.empty() ? "" : ", ") + JsonString(aMod->mId + "@" + aMod->mVersion);
	mFile.mMeta = std::string("{\"game\": \"Big Money Deluxe (byo-pop)\", \"date\": ") + JsonString(aDate) +
		", \"mode\": " + JsonString(kModeNames[aMode]) + ", \"skill\": " + JsonString(kSkillNames[aSkill]) +
		", \"score\": " + std::to_string(mFile.mScore) + ", \"level\": " + std::to_string(mFile.mLevel + 1) +
		", \"seed\": " + (mFile.mUserSeeded ? std::to_string(mFile.mGameSeed) : std::string("null")) +
		", \"mods\": [" + aMods + "], \"scoreKey\": " + JsonString(mFile.mScoreKey) + "}";

	std::string aDir = mApp->mReplayDir;
	Platform::MakeDirs(aDir);
	std::string aBase = aDir + "/" + aStamp + "-" + kModeNames[aMode] + "-" + kSkillNames[aSkill] + "-" + std::to_string(mFile.mScore);
	std::string aSuffix = thePinned ? "-keep.bmr" : ".bmr";
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
	SDL_Log("replay: %s (%d events, %d bag draws)", aPath.c_str(), (int) mFile.mEvents.size(), (int) mFile.mBagColumns.size());
	return aPath;
}

// ---------------------------------------------------------------------------------------------------------
// Playback

ReplayPlayer::ReplayPlayer(const Replay::File& theFile)
	: mFile(theFile)
{
}

void ReplayPlayer::Apply(Board* theBoard, uint32_t theTick)
{
	while ((mNextEvent < mFile.mEvents.size()) && (mFile.mEvents[mNextEvent].mTick <= theTick))
	{
		const Replay::Event& anEvent = mFile.mEvents[mNextEvent++];
		if ((anEvent.mTick < theTick) && (!mDesync))
		{
			// An event the board never had a tick for: the board has gone another way
			mDesync = true;
			SDL_Log("replay: desync at tick %u (an event of tick %u)", (unsigned) theTick, (unsigned) anEvent.mTick);
		}
		switch (anEvent.mOp)
		{
		case Replay::OP_CLICK:
			if (theBoard->mState == STATE_PLAYING)
				theBoard->ClickAt(anEvent.mX, anEvent.mY);
			break;
		case Replay::OP_MOVE:
			theBoard->mMouseX = anEvent.mX;
			theBoard->mMouseY = anEvent.mY;
			theBoard->UpdateHover();
			break;
		case Replay::OP_LEAVE:
			theBoard->UpdateHover();
			break;
		case Replay::OP_NEXT:
			if (theBoard->mState == STATE_LEVEL_DONE)
				theBoard->StartNextLevel();
			break;
		case Replay::OP_HINT:
			if (theBoard->mState == STATE_PLAYING)
				theBoard->RequestSolverHint();
			break;
		}
	}
}

int ReplayPlayer::NextBagColumn(uint32_t theTick)
{
	if (mNextBag < mFile.mBagColumns.size())
		return mFile.mBagColumns[mNextBag++];
	if (!mDesync)
	{
		mDesync = true;
		SDL_Log("replay: desync at tick %u (no bag draw left)", (unsigned) theTick);
	}
	return 0;
}

void ReplayPlayer::CheckEnd(Board* theBoard, uint32_t theTick)
{
	if (mEnded)
		return;
	mEnded = true;
	mVerified = (!mDesync) && (theTick == mFile.mEndTick) && (theBoard->mScore == mFile.mScore) &&
		(theBoard->mLevel == mFile.mLevel) && (theBoard->mGameCoinsCleared == mFile.mCoins) &&
		(mNextEvent == mFile.mEvents.size()) && (mNextBag == mFile.mBagColumns.size());
	SDL_Log("replay: %s: end tick %u (recorded %u), score %d (%d), level %d (%d), coins %d (%d)",
		mVerified ? "verified" : "DIFFERENT", (unsigned) theTick, (unsigned) mFile.mEndTick, theBoard->mScore, mFile.mScore,
		theBoard->mLevel + 1, mFile.mLevel + 1, theBoard->mGameCoinsCleared, mFile.mCoins);
}
