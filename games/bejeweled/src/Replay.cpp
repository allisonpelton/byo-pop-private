// port(variant): the .bjr replay format (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.4, 7.3); see Replay.h.
// Not part of the original game.

#include "Replay.h"
#include "Sha256.h"
#include "Piece.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Sexy;

namespace
{

const int gOpArgs[Replay::NUM_OPS] = {-1, 2, 0, 2, 3, 5, 0, 1, 1, 0, 0, 1, 2, 2, 0, 1};
const char* const gOpNames[Replay::NUM_OPS] = {"?", "SELECT", "DESELECT", "TARGET", "TWIST", "SLIDE", "HINT", "PAUSE",
	"UNPAUSE", "UNDO", "NEW_GAME", "XRAND", "HOVER", "SAVE", "LOAD", "CHECK"};

const uint32_t TAG_RULE = Replay::Tag("RULE");
const uint32_t TAG_RLID = Replay::Tag("RLID");
const uint32_t TAG_META = Replay::Tag("META");
const uint32_t TAG_SESS = Replay::Tag("SESS");
const uint32_t TAG_LDSV = Replay::Tag("LDSV");
const uint32_t TAG_RNGS = Replay::Tag("RNGS");
const uint32_t TAG_SEGM = Replay::Tag("SEGM");
const uint32_t TAG_SVHA = Replay::Tag("SVHA");
const uint32_t TAG_END = Replay::Tag("END ");

const size_t RNGS_SIZE = 4 + 1 + 1 + Replay::MT_WORDS * 4 + 4;
const size_t END_SIZE = 1 + 4 + 4 + 4 + 4 + 8 + 4;

// The v2 save's chunks (Replay.h); RNGS is the replay's tag
const uint32_t TAG_MOVE = Replay::Tag("MOVE");
const size_t MOVE_SIZE = 3 * 4;
const size_t SAVE_HEADER_FIXED = 4 + Replay::RULES_ID_SIZE + 2 + 2 + 4; // the version up to the mask

void PutU8(std::string* theOut, int theValue)
{
	theOut->push_back((char) (uint8_t) theValue);
}

void PutU16(std::string* theOut, uint32_t theValue)
{
	for (int i = 0; i < 2; i++)
		theOut->push_back((char) (uint8_t) (theValue >> (i * 8)));
}

void PutU32(std::string* theOut, uint32_t theValue)
{
	for (int i = 0; i < 4; i++)
		theOut->push_back((char) (uint8_t) (theValue >> (i * 8)));
}

void PutU64(std::string* theOut, uint64_t theValue)
{
	for (int i = 0; i < 8; i++)
		theOut->push_back((char) (uint8_t) (theValue >> (i * 8)));
}

void PutRecord(std::string* theOut, uint32_t theTag, const std::string& thePayload)
{
	PutU32(theOut, theTag);
	PutU32(theOut, (uint32_t) thePayload.size());
	theOut->append(thePayload);
}

std::string RngPayload(const Replay::RngState& theRng)
{
	std::string aPayload;
	PutU32(&aPayload, theRng.mSeed);
	PutU8(&aPayload, theRng.mUserSeeded ? 1 : 0);
	PutU8(&aPayload, theRng.mRanked ? 1 : 0);
	for (int i = 0; i < Replay::MT_WORDS; i++)
		PutU32(&aPayload, theRng.mMt[i]);
	PutU32(&aPayload, (uint32_t) theRng.mMti);
	return aPayload;
}

std::string SegmentPayload(const Replay::Segment& theSegment)
{
	std::string aPayload;
	PutU8(&aPayload, theSegment.mMode);
	PutU8(&aPayload, theSegment.mRng.mKind);
	if (theSegment.mRng.mKind == Replay::RNG_SEED)
		PutU32(&aPayload, theSegment.mRng.mSeed);
	else
	{
		for (int i = 0; i < Replay::MT_WORDS; i++)
			PutU32(&aPayload, theSegment.mRng.mMt[i]);
		PutU32(&aPayload, (uint32_t) theSegment.mRng.mMti);
	}
	PutU8(&aPayload, theSegment.mUserSeeded ? 1 : 0);
	PutU8(&aPayload, theSegment.mShowInstructions ? 1 : 0);
	PutU32(&aPayload, (uint32_t) theSegment.mTimesPlayed);
	PutU32(&aPayload, (uint32_t) theSegment.mPointsAscent);
	PutU32(&aPayload, (uint32_t) theSegment.mEvents.size());
	uint32_t aTick = 0;
	for (size_t i = 0; i < theSegment.mEvents.size(); i++)
	{
		const Replay::Event& anEvent = theSegment.mEvents[i];
		// Ticks never go back within a segment (one board's mUpdateCnt)
		Replay::PutVarint(&aPayload, (anEvent.mTick >= aTick) ? (anEvent.mTick - aTick) : 0);
		aTick = (anEvent.mTick >= aTick) ? anEvent.mTick : aTick;
		PutU8(&aPayload, anEvent.mOp);
		int aCount = Replay::OpArgCount(anEvent.mOp);
		for (int a = 0; a < aCount; a++)
			Replay::PutVarint(&aPayload, Replay::ZigZag(anEvent.mArgs[a]));
	}
	return aPayload;
}

// Bounded little-endian reads over one record's payload
class Reader
{
public:
	const std::string&	mBytes;
	size_t				mPos;
	size_t				mEnd;
	bool				mOk;

	Reader(const std::string& theBytes, size_t thePos, size_t theEnd) : mBytes(theBytes), mPos(thePos), mEnd(theEnd), mOk(true) {}

	bool Has(size_t theCount)
	{
		if (mOk && (theCount <= mEnd - mPos))
			return true;
		mOk = false;
		return false;
	}

	uint32_t Bytes(int theCount)
	{
		if (!Has(theCount))
			return 0;
		uint32_t aValue = 0;
		for (int i = 0; i < theCount; i++)
			aValue |= (uint32_t) (uint8_t) mBytes[mPos + i] << (i * 8);
		mPos += theCount;
		return aValue;
	}

	int U8() { return (int) Bytes(1); }
	uint32_t U16() { return Bytes(2); }
	uint32_t U32() { return Bytes(4); }

	uint64_t U64()
	{
		uint64_t aLow = U32();
		uint64_t aHigh = U32();
		return aLow | (aHigh << 32);
	}

	uint64_t Varint()
	{
		uint64_t aValue = 0;
		if (mOk && !Replay::GetVarint(mBytes, &mPos, mEnd, &aValue))
			mOk = false;
		return aValue;
	}

	bool AtEnd() const { return mOk && (mPos == mEnd); }
};

bool ReadRng(Reader& theReader, Replay::RngState* theRng)
{
	theRng->mSeed = theReader.U32();
	theRng->mUserSeeded = theReader.U8() != 0;
	theRng->mRanked = theReader.U8() != 0;
	for (int i = 0; i < Replay::MT_WORDS; i++)
		theRng->mMt[i] = theReader.U32();
	theRng->mMti = (int32_t) theReader.U32();
	return theReader.AtEnd();
}

// theEventBudget: how many more events the file may hold (Replay::MAX_EVENTS in all)
bool ReadSegment(Reader& theReader, Replay::Segment* theSegment, size_t* theEventBudget, std::string* theError)
{
	theSegment->mMode = theReader.U8();
	theSegment->mRng.mKind = theReader.U8();
	if (theSegment->mRng.mKind == Replay::RNG_SEED)
		theSegment->mRng.mSeed = theReader.U32();
	else if (theSegment->mRng.mKind == Replay::RNG_STATE)
	{
		for (int i = 0; i < Replay::MT_WORDS; i++)
			theSegment->mRng.mMt[i] = theReader.U32();
		theSegment->mRng.mMti = (int32_t) theReader.U32();
	}
	else
	{
		*theError = "SEGM: unknown RNG start kind " + std::to_string(theSegment->mRng.mKind);
		return false;
	}
	theSegment->mUserSeeded = theReader.U8() != 0;
	theSegment->mShowInstructions = theReader.U8() != 0;
	theSegment->mTimesPlayed = (int32_t) theReader.U32();
	theSegment->mPointsAscent = (int32_t) theReader.U32();
	uint32_t aCount = theReader.U32();
	if (!theReader.mOk)
	{
		*theError = "SEGM: truncated header";
		return false;
	}
	// Every event takes at least 2 bytes (a tick delta and an op): a count the payload cannot hold, or more
	// events than a file may have, is refused before anything is allocated for it.
	if (aCount > (theReader.mEnd - theReader.mPos) / 2)
	{
		*theError = "SEGM: " + std::to_string(aCount) + " events cannot fit in " + std::to_string(theReader.mEnd - theReader.mPos) + " bytes";
		return false;
	}
	if (aCount > *theEventBudget)
	{
		*theError = "SEGM: " + std::to_string(aCount) + " events, more than the " + std::to_string((int) Replay::MAX_EVENTS) +
			" a replay may have";
		return false;
	}
	*theEventBudget -= aCount;
	// Two passes over the events: the first checks them all (ticks, CheckPlayable's bound on one tick, ops, the
	// end of the payload) and keeps none, so a file that fails allocates nothing for them; the second keeps them,
	// in room made for exactly that many (sizeof(Event) per 2 bytes of the file at most; no vector doubling,
	// whose peak is three times as much and stays in a wasm instance's memory)
	size_t anEventsAt = theReader.mPos;
	for (int aPass = 0; aPass < 2; aPass++)
	{
		if (aPass == 1)
		{
			theReader.mPos = anEventsAt;
			theSegment->mEvents.reserve(aCount);
		}
		uint64_t aTick = 0;
		uint32_t aTickStart = 0; // the first event at the tick of event i
		for (uint32_t i = 0; i < aCount; i++)
		{
			Replay::Event anEvent;
			uint64_t aDelta = theReader.Varint();
			aTick += (aDelta <= 0xffffffffull) ? aDelta : 0x100000000ull;
			if (aTick > 0xffffffffull)
			{
				*theError = "SEGM: event " + std::to_string(i) + ": tick past 2^32";
				return false;
			}
			if (aDelta != 0)
				aTickStart = i;
			if (i - aTickStart >= (uint32_t) Replay::MAX_EVENTS_PER_TICK)
			{
				*theError = "SEGM: event " + std::to_string(i) + ": more than " + std::to_string((int) Replay::MAX_EVENTS_PER_TICK) +
					" events at one tick";
				return false;
			}
			anEvent.mTick = (uint32_t) aTick;
			anEvent.mOp = theReader.U8();
			int anArgs = Replay::OpArgCount(anEvent.mOp);
			if (!theReader.mOk)
				break;
			if (anArgs < 0)
			{
				*theError = "SEGM: event " + std::to_string(i) + ": unknown op " + std::to_string(anEvent.mOp);
				return false;
			}
			for (int a = 0; a < anArgs; a++)
				anEvent.mArgs[a] = Replay::UnZigZag(theReader.Varint());
			if (aPass == 1)
				theSegment->mEvents.push_back(anEvent);
		}
		if (!theReader.AtEnd())
		{
			*theError = theReader.mOk ? "SEGM: data after the last event" : "SEGM: truncated event";
			return false;
		}
	}
	return true;
}

}

int Replay::OpArgCount(int theOp)
{
	return ((theOp > 0) && (theOp < NUM_OPS)) ? gOpArgs[theOp] : -1;
}

const char* Replay::OpName(int theOp)
{
	return ((theOp > 0) && (theOp < NUM_OPS)) ? gOpNames[theOp] : gOpNames[0];
}

uint32_t Replay::Tag(const char* theName)
{
	uint32_t aTag = 0;
	for (int i = 0; i < 4; i++)
		aTag |= (uint32_t) (uint8_t) theName[i] << (i * 8);
	return aTag;
}

std::string Replay::TagName(uint32_t theTag)
{
	std::string aName;
	for (int i = 0; i < 4; i++)
	{
		char c = (char) (theTag >> (i * 8));
		aName += ((c >= 32) && (c < 127)) ? c : '?';
	}
	return aName;
}

void Replay::PutVarint(std::string* theOut, uint64_t theValue)
{
	while (theValue >= 0x80)
	{
		theOut->push_back((char) (uint8_t) ((theValue & 0x7f) | 0x80));
		theValue >>= 7;
	}
	theOut->push_back((char) (uint8_t) theValue);
}

bool Replay::GetVarint(const std::string& theBytes, size_t* thePos, size_t theEnd, uint64_t* theValue)
{
	uint64_t aValue = 0;
	size_t aPos = *thePos;
	for (int i = 0; i < 10; i++)
	{
		if ((aPos >= theEnd) || (aPos >= theBytes.size()))
			return false;
		uint8_t aByte = (uint8_t) theBytes[aPos++];
		// The tenth byte holds bit 63 only
		if ((i == 9) && (aByte > 1))
			return false;
		aValue |= (uint64_t) (aByte & 0x7f) << (7 * i);
		if ((aByte & 0x80) == 0)
		{
			*thePos = aPos;
			*theValue = aValue;
			return true;
		}
	}
	return false;
}

uint64_t Replay::ZigZag(int64_t theValue)
{
	return ((uint64_t) theValue << 1) ^ (uint64_t) (theValue >> 63);
}

int64_t Replay::UnZigZag(uint64_t theValue)
{
	return (int64_t) ((theValue >> 1) ^ (~(theValue & 1) + 1));
}

uint32_t Replay::SaveHash(const void* theData, size_t theSize)
{
	uint8_t aDigest[Sha256::DIGEST_SIZE];
	Sha256::Hash(theData, theSize, aDigest);
	return (uint32_t) aDigest[0] | ((uint32_t) aDigest[1] << 8) | ((uint32_t) aDigest[2] << 16) | ((uint32_t) aDigest[3] << 24);
}

int Replay::CountMoves(const Segment& theSegment)
{
	int aMoves = 0;
	for (size_t i = 0; i < theSegment.mEvents.size(); i++)
	{
		const Event& anEvent = theSegment.mEvents[i];
		if ((anEvent.mOp == OP_TARGET) && (anEvent.mArgs[0] >= 0))
			aMoves++;
	}
	return aMoves;
}

std::string Replay::VariantSaveHeader(const VariantSave& theSave)
{
	std::string aHeader((const char*) theSave.mRulesId, RULES_ID_SIZE);
	PutU16(&aHeader, (uint32_t) theSave.mCols);
	PutU16(&aHeader, (uint32_t) theSave.mRows);
	PutU32(&aHeader, (uint32_t) theSave.mLevelShape);
	std::string aMask(((size_t) theSave.mCols * theSave.mRows + 7) / 8, '\0');
	for (size_t i = 0; (i < theSave.mPlayable.size()) && (i < aMask.size() * 8); i++)
	{
		if (theSave.mPlayable[i])
			aMask[i / 8] = (char) (aMask[i / 8] | (1 << (i % 8)));
	}
	return aHeader + aMask;
}

std::string Replay::VariantSaveChunks(const VariantSave& theSave)
{
	std::string aChunks;
	PutRecord(&aChunks, TAG_RNGS, RngPayload(theSave.mRng));
	std::string aMoves;
	PutU32(&aMoves, (uint32_t) theSave.mMoves);
	PutU32(&aMoves, (uint32_t) theSave.mInvalidMoves);
	PutU32(&aMoves, (uint32_t) theSave.mUndosUsed);
	PutRecord(&aChunks, TAG_MOVE, aMoves);
	PutRecord(&aChunks, TAG_END, "");
	return aChunks;
}

bool Replay::ReadVariantSave(const std::string& theSave, VariantSave* theOut, std::string* theError)
{
	std::string aDummy;
	std::string& anErr = (theError != NULL) ? *theError : aDummy;
	*theOut = VariantSave();
	VariantSave& s = *theOut;
	if (theSave.size() < SAVE_HEADER_FIXED)
	{
		anErr = "shorter than the v2 header";
		return false;
	}
	Reader aHeader(theSave, 0, theSave.size());
	uint32_t aVersion = aHeader.U32();
	if (aVersion != (uint32_t) SAVE_VERSION_VARIANT)
	{
		anErr = "version " + std::to_string(aVersion) + ", not the v2 save's " + std::to_string((int) SAVE_VERSION_VARIANT);
		return false;
	}
	memcpy(s.mRulesId, theSave.data() + 4, RULES_ID_SIZE);
	aHeader.mPos += RULES_ID_SIZE;
	s.mCols = (int) aHeader.U16();
	s.mRows = (int) aHeader.U16();
	s.mLevelShape = (int32_t) aHeader.U32();
	if ((s.mCols < 3) || (s.mCols > 16) || (s.mRows < 3) || (s.mRows > 16))
	{
		anErr = "a " + std::to_string(s.mCols) + "x" + std::to_string(s.mRows) + " board (3..16 each)";
		return false;
	}
	size_t aCells = (size_t) s.mCols * s.mRows;
	size_t aMaskSize = (aCells + 7) / 8;
	if (theSave.size() - aHeader.mPos < aMaskSize)
	{
		anErr = "it ends in the mask";
		return false;
	}
	for (size_t i = 0; i < aCells; i++)
		s.mPlayable.push_back((((uint8_t) theSave[aHeader.mPos + i / 8] >> (i % 8)) & 1) != 0);
	size_t aPos = aHeader.mPos + aMaskSize;
	s.mBodyStart = aPos;

	// The v1 field sequence: its size follows from the present bytes
	if (theSave.size() - aPos < (size_t) SAVE_HEAD_SIZE)
	{
		anErr = "it ends before the cells";
		return false;
	}
	aPos += SAVE_HEAD_SIZE;
	for (size_t i = 0; i < aCells; i++)
	{
		if (aPos >= theSave.size())
		{
			anErr = "it ends in the cells";
			return false;
		}
		bool isPresent = (theSave[aPos++] != 0);
		if (!isPresent)
			continue;
		if (!s.mPlayable[i])
		{
			anErr = "a gem in the hole at cell (" + std::to_string(i % s.mCols) + "," + std::to_string(i / s.mCols) + ")";
			return false;
		}
		if (theSave.size() - aPos < Piece::SAVE_SIZE)
		{
			anErr = "it ends in the cells";
			return false;
		}
		aPos += Piece::SAVE_SIZE;
	}
	if (theSave.size() - aPos < (size_t) SAVE_TAIL_SIZE)
	{
		anErr = "it ends before the chunks";
		return false;
	}
	aPos += SAVE_TAIL_SIZE;
	s.mBodyEnd = aPos;

	// The chunks, up to END
	bool hasRng = false;
	bool hasMoves = false;
	for (;;)
	{
		if (theSave.size() - aPos < 8)
		{
			anErr = "no END chunk (truncated?)";
			return false;
		}
		Reader aChunkHeader(theSave, aPos, aPos + 8);
		uint32_t aTag = aChunkHeader.U32();
		uint32_t aLen = aChunkHeader.U32();
		aPos += 8;
		if (aLen > theSave.size() - aPos)
		{
			anErr = TagName(aTag) + ": " + std::to_string(aLen) + " bytes, " + std::to_string(theSave.size() - aPos) + " left";
			return false;
		}
		Reader aReader(theSave, aPos, aPos + aLen);
		aPos += aLen;
		if (aTag == TAG_END)
		{
			if (aLen != 0)
			{
				anErr = "END: " + std::to_string(aLen) + " bytes, not 0";
				return false;
			}
			break;
		}
		if (aTag == TAG_RNGS)
		{
			if (hasRng || (aLen != RNGS_SIZE) || !ReadRng(aReader, &s.mRng))
			{
				anErr = hasRng ? std::string("RNGS twice") : "RNGS: " + std::to_string(aLen) + " bytes, not " + std::to_string(RNGS_SIZE);
				return false;
			}
			hasRng = true;
		}
		else if (aTag == TAG_MOVE)
		{
			if (hasMoves || (aLen != MOVE_SIZE))
			{
				anErr = hasMoves ? std::string("MOVE twice") : "MOVE: " + std::to_string(aLen) + " bytes, not " + std::to_string(MOVE_SIZE);
				return false;
			}
			s.mMoves = (int32_t) aReader.U32();
			s.mInvalidMoves = (int32_t) aReader.U32();
			s.mUndosUsed = (int32_t) aReader.U32();
			hasMoves = true;
		}
		// any other chunk: a later stage's, skipped
	}
	if (aPos != theSave.size())
	{
		anErr = std::to_string(theSave.size() - aPos) + " bytes after END";
		return false;
	}
	if (!hasRng || !hasMoves)
	{
		anErr = !hasRng ? "no RNGS chunk" : "no MOVE chunk";
		return false;
	}
	return true;
}

bool Replay::SaveIsTimed(const std::string& theSave, bool* isTimed)
{
	if (theSave.size() < 4)
		return false;
	Reader aReader(theSave, 0, 4);
	uint32_t aVersion = aReader.U32();
	size_t aPos = 0;
	if (aVersion == (uint32_t) SAVE_VERSION)
		aPos = 4;
	else if ((aVersion == (uint32_t) SAVE_VERSION_VARIANT) && (theSave.size() >= SAVE_HEADER_FIXED))
	{
		Reader aSize(theSave, 4 + RULES_ID_SIZE, SAVE_HEADER_FIXED);
		size_t aCols = aSize.U16();
		size_t aRows = aSize.U16();
		aPos = SAVE_HEADER_FIXED + (aCols * aRows + 7) / 8;
	}
	if ((aPos == 0) || (aPos >= theSave.size()))
		return false;
	*isTimed = (theSave[aPos] != 0);
	return true;
}

namespace
{

// Bounded reads of a v1 save (Board::WriteSaveGame's field order), each value checked against a range as it
// is read; the first value out of range (or past the end) stops the check with its name in mError
class SaveCheck
{
public:
	const std::string&	mBytes;
	size_t				mPos;
	std::string			mError;

	SaveCheck(const std::string& theBytes) : mBytes(theBytes), mPos(0) {}

	bool Ok() const { return mError.empty(); }

	bool Take(size_t theCount, void* theOut, const std::string& theName)
	{
		if (!Ok())
			return false;
		if (theCount > mBytes.size() - mPos)
		{
			mError = "it ends before " + theName;
			return false;
		}
		memcpy(theOut, mBytes.data() + mPos, theCount); // the save is the host's raw little-endian bytes
		mPos += theCount;
		return true;
	}

	int32_t Int(const std::string& theName, int64_t theMin, int64_t theMax)
	{
		int32_t aValue = 0;
		if (Take(4, &aValue, theName) && ((aValue < theMin) || (aValue > theMax)))
			mError = theName + " is " + std::to_string(aValue) + ", not " + std::to_string((long long) theMin) + ".." +
				std::to_string((long long) theMax);
		return aValue;
	}

	int Bool(const std::string& theName)
	{
		uint8_t aValue = 0;
		if (Take(1, &aValue, theName) && (aValue > 1))
			mError = theName + " is " + std::to_string(aValue) + ", not a bool";
		return aValue;
	}

	void Real(const std::string& theName, double theMin, double theMax)
	{
		double aValue = 0;
		if (Take(8, &aValue, theName) && (!std::isfinite(aValue) || (aValue < theMin) || (aValue > theMax)))
			mError = theName + " is " + std::to_string(aValue) + ", not " + std::to_string(theMin) + ".." + std::to_string(theMax);
	}

	void Skip(size_t theCount, const std::string& theName)
	{
		char aByte;
		for (size_t i = 0; (i < theCount) && Take(1, &aByte, theName); i++)
			;
	}
};

// One present cell's 0x78 Piece bytes (Piece.h): what Draw and the Update functions index or loop with must be
// in range; the position (mX, mY) is not checked: LoadGame puts the piece at its cell
void CheckPiece(SaveCheck& theCheck, int theCol, int theRow)
{
	std::string aCell = "cell (" + std::to_string(theCol) + "," + std::to_string(theRow) + ")'s ";
	size_t aBase = theCheck.mPos;
	SaveCheck& c = theCheck;
	auto At = [&](size_t theOffset, const char* theName) -> std::string
	{
		// the fields are read in their order; padding and the unchecked ones are skipped up to theOffset
		if (c.Ok() && (c.mPos < aBase + theOffset))
			c.Skip(aBase + theOffset - c.mPos, aCell + theName);
		return aCell + theName;
	};
	c.Int(At(offsetof(Piece, mColor), "mColor"), 0, 6);              // a gem sheet row, mGemShrinkImages[mColor]
	c.Int(At(offsetof(Piece, mCol), "mCol"), theCol, theCol);        // the cell it is in: the grid writes go there
	c.Int(At(offsetof(Piece, mRow), "mRow"), theRow, theRow);
	c.Real(At(offsetof(Piece, mVelY), "mVelY"), -1000.0, 1000.0);
	c.Bool(At(offsetof(Piece, mGlowing), "mGlowing"));
	c.Real(At(offsetof(Piece, mGlowAlpha), "mGlowAlpha"), -1.0, 1.0); // a crossfade alpha (just under 0 once it fades)
	c.Int(At(offsetof(Piece, mGlowFrame), "mGlowFrame"), 0, 14);     // gem sheet columns
	c.Bool(At(offsetof(Piece, mDying), "mDying"));
	c.Int(At(offsetof(Piece, mDyingFrame), "mDyingFrame"), 0, 5);    // mGemShrinkImages[][mDyingFrame]
	c.Int(At(offsetof(Piece, mSpinFrame), "mSpinFrame"), 0, 14);
	c.Int(At(offsetof(Piece, mSparkleCount), "mSparkleCount"), 0, 58); // DoHint's 58, down by 1
	c.Int(At(offsetof(Piece, mSparkleFrame), "mSparkleFrame"), 0, 58); // sparkle cels from it (% 14, also below 0)
	c.Real(At(offsetof(Piece, mHoleAngle), "mHoleAngle"), -1e9, 1e9);
	c.Real(At(offsetof(Piece, mHoleAngVel), "mHoleAngVel"), -1e6, 1e6);
	c.Real(At(offsetof(Piece, mHoleRadius), "mHoleRadius"), 0.0, 10000.0); // the vortex's shrink frame comes from it
	At(Piece::SAVE_SIZE, "record");
}

}

bool Replay::CheckSave(const std::string& theSave, int theCols, int theRows, std::string* theError, const uint8_t* theRulesId)
{
	std::string aDummy;
	std::string& anErr = (theError != NULL) ? *theError : aDummy;
	SaveCheck c(theSave);
	const int64_t SCORE_MAX = 1000000000;   // far over any game's, and its sums and *340 bar products stay in an int
	const int64_t LEVEL_MAX = 100000;
	// A v2 save (stage 5): of the replay's rules and the board's shape, its chunks well-formed; the same fields
	// follow its header as a v1 save's version
	VariantSave aVariant;
	bool isVariant = (theSave.size() >= 4) && (theSave.compare(0, 4, std::string("\x01\x01\0\0", 4)) == 0);
	if (isVariant)
	{
		std::string aWhy;
		if (theRulesId == NULL)
			c.mError = "a v2 save, and the replay names no rules for it";
		else if (!ReadVariantSave(theSave, &aVariant, &aWhy))
			c.mError = "a v2 save that does not read: " + aWhy;
		else if (memcmp(aVariant.mRulesId, theRulesId, RULES_ID_SIZE) != 0)
			c.mError = "a v2 save of other rules than the replay's";
		else if ((aVariant.mCols != theCols) || (aVariant.mRows != theRows) || (aVariant.mLevelShape != 0))
			c.mError = "a v2 save of a " + std::to_string(aVariant.mCols) + "x" + std::to_string(aVariant.mRows) + " board, level shape " +
				std::to_string(aVariant.mLevelShape);
		else if (std::find(aVariant.mPlayable.begin(), aVariant.mPlayable.end(), false) != aVariant.mPlayable.end())
			c.mError = "a v2 save with holes in its mask";
		else if ((aVariant.mMoves < 0) || (aVariant.mInvalidMoves < 0) || (aVariant.mUndosUsed < 0))
			c.mError = "a v2 save whose MOVE counts go below 0";
		c.mPos = aVariant.mBodyStart;
	}
	else
		c.Int("the version", 1, 1);
	int isTimed = c.Bool("mTimedMode");
	c.Int("mLevel", 0, LEVEL_MAX);        // level % 4 and % 5 index SetupLevel's tables
	c.Int("mDisplayLevel", 1, LEVEL_MAX + 1);
	for (int aRow = 0; aRow < theRows; aRow++)
	{
		for (int aCol = 0; aCol < theCols; aCol++)
		{
			if (c.Bool("cell (" + std::to_string(aCol) + "," + std::to_string(aRow) + ")'s present byte"))
				CheckPiece(c, aCol, aRow);
		}
	}
	int32_t aState = c.Int("mState", 0, 9);
	if (c.Ok() && ((aState == 5) || (aState == 6)))         // STATE_GAME_OVER, _DONE: a save is a game in progress
		c.mError = "mState is " + std::to_string(aState) + ", a game over";
	if (c.Ok() && (c.Bool("mTimedMode (again)") != isTimed))
		c.mError = "the two mTimedMode bytes differ";
	c.Bool("mIsNewGame");
	c.Int("mBarWidth", 0, 340);           // a source width in mTimerBarImages
	c.Int("mPlaySeconds", 0, 0x7fffffff);
	c.Int("mIdleTicks", 0, 0x7fffffff);
	c.Int("mScore", 0, SCORE_MAX);
	c.Real("mPointMultiplier", 1e-6, 1e6); // a divisor of the move score
	c.Int("mDisplayScore", 0, SCORE_MAX);
	c.Int("mGemsCleared", 0, 0x7fffffff);
	c.Int("mMoveMatchCount", 0, 0x7fffffff);
	c.Int("mCascadeCount", 0, 0x7fffffff);
	c.Int("mLongestCascade", 0, 0x7fffffff);
	c.Int("mBestMoveScore", 0, SCORE_MAX);
	c.Bool("mStatsDisabled");
	c.Skip(4, "mUnused264");
	int32_t aStart = c.Int("mLevelStartScore", 0, SCORE_MAX);
	int32_t aTarget = c.Int("mLevelTargetScore", 0, 2 * SCORE_MAX);
	if (c.Ok() && (aTarget <= aStart))             // Update divides by their difference
		c.mError = "mLevelTargetScore " + std::to_string(aTarget) + " is not over mLevelStartScore " + std::to_string(aStart);
	c.Int("mLevelPoints", 0, SCORE_MAX);
	c.Real("mTimeDrain", -1e8, 1e8);
	c.Real("mTimeDrainRate", -1e8, 1e8);
	c.Real("mTimeDrainAccel", -1e8, 1e8);
	c.Int("mBarColor", 0, 24);            // mTimerBarImages[mBarColor]
	c.Int("mWipeY", 0, 1000);
	c.Int("mBlackHoleFrame", 0, 8);       // a source column of the black hole image
	c.Real("mBlackHoleLevel", 0.0, 1.0);
	c.Bool("mLevelTextShown");
	if (c.Ok() && (c.mPos != (isVariant ? aVariant.mBodyEnd : theSave.size())))
		c.mError = std::to_string(theSave.size() - c.mPos) + " bytes after the last field";
	if (!c.Ok())
	{
		anErr = "not a save the game could have made: " + c.mError;
		return false;
	}
	return true;
}

// The checks a replay passes before a board is made for it (Replay.h). Each message names where it fails.
bool Replay::CheckPlayable(const File& theFile, int theCols, int theRows, std::string* theError, bool isOriginalRules)
{
	std::string aDummy;
	std::string& anErr = (theError != NULL) ? *theError : aDummy;
	bool isAbiPlayable = (theFile.mEngineAbi == (uint32_t) ENGINE_ABI) ||
		(isOriginalRules && (theFile.mEngineAbi >= (uint32_t) FIRST_ENGINE_ABI) && (theFile.mEngineAbi < (uint32_t) ENGINE_ABI));
	if (!isAbiPlayable)
	{
		anErr = "recorded by engine " + std::to_string(theFile.mEngineAbi) + ", this build plays engine " + std::to_string((int) ENGINE_ABI) +
			(isOriginalRules ? " (and the original rules' replays from engine " + std::to_string((int) FIRST_ENGINE_ABI) + " on)" : "");
		return false;
	}
	if (!theFile.mHasRules)
	{
		anErr = "no RULE record";
		return false;
	}
	if (theFile.mSegments.empty())
	{
		anErr = "no segment";
		return false;
	}
	if (theFile.mEnd.mEndTick > (uint32_t) MAX_TICKS)
	{
		anErr = "END at tick " + std::to_string(theFile.mEnd.mEndTick) + ", past the " + std::to_string((int) MAX_TICKS) + " a replay may last";
		return false;
	}
	int64_t anXrand = 0;
	uint64_t aPlayed = 0;
	for (size_t s = 0; s < theFile.mSegments.size(); s++)
	{
		const Segment& aSegment = theFile.mSegments[s];
		std::string aWhere = "segment " + std::to_string(s) + ": ";
		bool isLast = (s + 1 == theFile.mSegments.size());
		if ((aSegment.mMode != 0) && (aSegment.mMode != 1))
		{
			anErr = aWhere + "mode " + std::to_string(aSegment.mMode);
			return false;
		}
		if ((aSegment.mRng.mKind == RNG_SEED) && (aSegment.mRng.mSeed == 0))
		{
			anErr = aWhere + "RNG start seed 0";
			return false;
		}
		if (aSegment.mUserSeeded && (aSegment.mRng.mKind != RNG_SEED))
		{
			anErr = aWhere + "user-seeded without a seed";
			return false;
		}
		// The bytes an LDSV gives the original LoadGame, which trusts a save (Replay.h, CheckSave)
		if (aSegment.mHasLoadSave && !CheckSave(aSegment.mLoadSave, theCols, theRows, &anErr, theFile.mHasRulesId ? theFile.mRulesId : NULL))
		{
			anErr = aWhere + "LDSV: " + anErr;
			return false;
		}
		// What one replay can make the player do: the segments' board ticks (each up to its last event, the
		// last one up to END) at most MAX_TICKS in all, so the CLI player and the verifier end; no more than
		// MAX_EVENTS_PER_TICK events in one frame (below)
		aPlayed += isLast ? theFile.mEnd.mEndTick : (aSegment.mEvents.empty() ? 0 : aSegment.mEvents.back().mTick);
		if (aPlayed > (uint64_t) MAX_TICKS)
		{
			anErr = aWhere + "its board ticks and the segments' before it come to " + std::to_string((unsigned long long) aPlayed) +
				", more than the " + std::to_string((int) MAX_TICKS) + " a replay may last";
			return false;
		}
		int aLoads = 0;
		int aSaves = 0;
		size_t aTickStart = 0; // the first event at the tick of event i
		for (size_t i = 0; i < aSegment.mEvents.size(); i++)
		{
			const Event& anEvent = aSegment.mEvents[i];
			const int64_t* a = anEvent.mArgs;
			std::string anAt = aWhere + "event " + std::to_string(i) + " (" + OpName(anEvent.mOp) + " at tick " + std::to_string(anEvent.mTick) + "): ";
			bool isCell = (a[0] >= 0) && (a[0] < theCols) && (a[1] >= 0) && (a[1] < theRows);
			if ((i > 0) && (anEvent.mTick != aSegment.mEvents[i - 1].mTick))
				aTickStart = i;
			if (i - aTickStart >= (size_t) MAX_EVENTS_PER_TICK)
			{
				anErr = anAt + "more than " + std::to_string((int) MAX_EVENTS_PER_TICK) + " events at one tick";
				return false;
			}
			switch (anEvent.mOp)
			{
			case OP_SELECT:
				if (!isCell)
				{
					anErr = anAt + "not a cell of the board";
					return false;
				}
				break;
			case OP_TARGET:
				if (!isCell && !((a[0] == -1) && (a[1] == -1)))
				{
					anErr = anAt + "not a cell of the board";
					return false;
				}
				break;
			case OP_TWIST:
			case OP_SLIDE:
				anErr = anAt + "a move this build does not play";
				return false;
			case OP_UNDO:
				// stage 6: no arguments; whether the board may undo then is the player's gate (WatchController)
				break;
			case OP_PAUSE:
			case OP_UNPAUSE:
				if ((a[0] != 0) && (a[0] != 1))
				{
					anErr = anAt + "not 0 or 1";
					return false;
				}
				break;
			case OP_XRAND:
				if ((a[0] < 1) || (a[0] > MAX_XRAND))
				{
					anErr = anAt + "not 1.." + std::to_string((int) MAX_XRAND) + " draws";
					return false;
				}
				anXrand += a[0];
				if (anXrand > MAX_XRAND)
				{
					anErr = anAt + "more than " + std::to_string((int) MAX_XRAND) + " outside draws in the replay";
					return false;
				}
				break;
			case OP_HOVER:
				if ((a[0] < -0x7fffffffll - 1) || (a[0] > 0x7fffffff) || (a[1] < -0x7fffffffll - 1) || (a[1] > 0x7fffffff))
				{
					anErr = anAt + "not a position";
					return false;
				}
				break;
			case OP_SAVE:
				if (((a[0] != SAVE_GAME) && (a[0] != SAVE_SNAPSHOT)) || (a[1] < 0) || (a[1] > 0xffffffffll))
				{
					anErr = anAt + "not a save kind and hash";
					return false;
				}
				// The game saves as its board goes away (Shutdown, ~Board; a snapshot sidecar adds its SAVE after
				// them): never before the segment's last tick, and never more than a few times. Each SAVE makes a
				// save and hashes it, so this also bounds the work
				if (anEvent.mTick != aSegment.mEvents.back().mTick)
				{
					anErr = anAt + "a SAVE before the segment's last tick (the game saves only as its board goes away)";
					return false;
				}
				if (++aSaves > MAX_SAVES_PER_SEGMENT)
				{
					anErr = anAt + "more than " + std::to_string((int) MAX_SAVES_PER_SEGMENT) + " SAVEs in one segment";
					return false;
				}
				break;
			case OP_LOAD:
				if ((anEvent.mTick != 0) || (++aLoads > 1))
				{
					anErr = anAt + "a LOAD only once, at the segment's tick 0";
					return false;
				}
				if (!aSegment.mHasLoadRng)
				{
					anErr = anAt + "no RNGS for the board to go on with";
					return false;
				}
				if (!aSegment.mHasLoadSave && ((s == 0) || theFile.mSegments[s - 1].mEvents.empty() ||
					(theFile.mSegments[s - 1].mEvents.back().mOp != OP_SAVE)))
				{
					anErr = anAt + "no save to load (no LDSV, and no SAVE ends the segment before)";
					return false;
				}
				break;
			case OP_CHECK:
				if (anEvent.mTick == 0)
				{
					anErr = anAt + "a CHECK before the first update";
					return false;
				}
				break;
			default:
				break;
			}
		}
		if ((aLoads == 0) && (aSegment.mHasLoadRng || aSegment.mHasLoadSave))
		{
			anErr = aWhere + "an RNGS or LDSV without a LOAD";
			return false;
		}
		if ((s > 0) && (aLoads == 0))
		{
			anErr = aWhere + "a later segment that does not LOAD the game";
			return false;
		}
		if (!isLast && (aSegment.mEvents.empty() || (aSegment.mEvents.back().mOp != OP_SAVE)))
		{
			anErr = aWhere + "a segment another follows that does not end in a SAVE";
			return false;
		}
		if (isLast && !aSegment.mEvents.empty() && (aSegment.mEvents.back().mTick > theFile.mEnd.mEndTick))
		{
			anErr = aWhere + "events after the END tick " + std::to_string(theFile.mEnd.mEndTick);
			return false;
		}
	}
	return true;
}

std::string Replay::Write(const File& theFile)
{
	std::string anOut = "BJRP";
	PutU16(&anOut, (uint32_t) theFile.mFormat);
	PutU16(&anOut, (uint32_t) theFile.mFlags);
	PutU32(&anOut, theFile.mEngineAbi);
	if (theFile.mHasRules)
		PutRecord(&anOut, TAG_RULE, theFile.mRules);
	if (theFile.mHasRulesId)
		PutRecord(&anOut, TAG_RLID, std::string((const char*) theFile.mRulesId, RULES_ID_SIZE));
	if (theFile.mHasMeta)
		PutRecord(&anOut, TAG_META, theFile.mMeta);
	if (theFile.mHasSession)
		PutRecord(&anOut, TAG_SESS, theFile.mSession);
	for (size_t i = 0; i < theFile.mSegments.size(); i++)
	{
		const Segment& aSegment = theFile.mSegments[i];
		if (aSegment.mHasLoadSave)
			PutRecord(&anOut, TAG_LDSV, aSegment.mLoadSave);
		if (aSegment.mHasLoadRng)
			PutRecord(&anOut, TAG_RNGS, RngPayload(aSegment.mLoadRng));
		PutRecord(&anOut, TAG_SEGM, SegmentPayload(aSegment));
	}
	if (theFile.mHasTrailingRng)
		PutRecord(&anOut, TAG_RNGS, RngPayload(theFile.mTrailingRng));
	if (theFile.mHasSaveHash)
	{
		std::string aPayload;
		PutU32(&aPayload, theFile.mSaveHash);
		PutRecord(&anOut, TAG_SVHA, aPayload);
	}
	std::string anEnd;
	PutU8(&anEnd, theFile.mEnd.mReason);
	PutU32(&anEnd, theFile.mEnd.mEndTick);
	PutU32(&anEnd, (uint32_t) theFile.mEnd.mScore);
	PutU32(&anEnd, (uint32_t) theFile.mEnd.mLevel);
	PutU32(&anEnd, (uint32_t) theFile.mEnd.mGems);
	PutU64(&anEnd, theFile.mEnd.mLogicHash);
	PutU32(&anEnd, theFile.mEnd.mBoardDraws);
	PutRecord(&anOut, TAG_END, anEnd);
	return anOut;
}

bool Replay::Read(const std::string& theBytes, File* theFile, std::string* theError)
{
	std::string anError;
	std::string& anErr = (theError != NULL) ? *theError : anError;
	*theFile = File();
	if (theBytes.size() > (size_t) MAX_FILE_SIZE)
	{
		anErr = std::to_string(theBytes.size()) + " bytes, more than the " + std::to_string((int) MAX_FILE_SIZE) + " a replay may have";
		return false;
	}
	if ((theBytes.size() < 12) || (theBytes.compare(0, 4, "BJRP") != 0))
	{
		anErr = (theBytes.size() < 12) ? "shorter than the header" : "not a replay (no BJRP magic)";
		return false;
	}
	Reader aHeader(theBytes, 4, 12);
	theFile->mFormat = (int) aHeader.U16();
	theFile->mFlags = (int) aHeader.U16();
	theFile->mEngineAbi = aHeader.U32();
	if (theFile->mFormat != FORMAT)
	{
		anErr = "format " + std::to_string(theFile->mFormat) + " (this build reads " + std::to_string((int) FORMAT) + ")";
		return false;
	}

	// Where the record order stands (Replay.h): RULE, RLID, META, SESS first (aHeaderAt: the last of them
	// read, 1..4), then per segment [LDSV] [RNGS] SEGM, then [RNGS] [SVHA] END. A record out of place or
	// repeated is refused, so no two readers can take different copies of it.
	int aHeaderAt = 0;
	bool isPastHeader = false;
	bool hasEnd = false;
	bool hasPendingRng = false;
	bool hasPendingSave = false;
	RngState aPendingRng;
	std::string aPendingSave;
	size_t anEventBudget = MAX_EVENTS;
	size_t aPos = 12;
	while (aPos < theBytes.size())
	{
		if (hasEnd)
		{
			anErr = "data after END";
			return false;
		}
		if (theBytes.size() - aPos < 8)
		{
			anErr = "truncated record header at byte " + std::to_string(aPos);
			return false;
		}
		Reader aRecordHeader(theBytes, aPos, aPos + 8);
		uint32_t aTag = aRecordHeader.U32();
		uint32_t aLen = aRecordHeader.U32();
		aPos += 8;
		if (aLen > theBytes.size() - aPos)
		{
			anErr = TagName(aTag) + ": " + std::to_string(aLen) + " bytes, " + std::to_string(theBytes.size() - aPos) + " left";
			return false;
		}
		Reader aReader(theBytes, aPos, aPos + aLen);
		size_t aStart = aPos;
		aPos += aLen;

		int aHeaderIndex = (aTag == TAG_RULE) ? 1 : (aTag == TAG_RLID) ? 2 : (aTag == TAG_META) ? 3 : (aTag == TAG_SESS) ? 4 : 0;
		bool isKnown = (aHeaderIndex > 0) || (aTag == TAG_LDSV) || (aTag == TAG_RNGS) || (aTag == TAG_SEGM) || (aTag == TAG_SVHA) ||
			(aTag == TAG_END);
		if (!isKnown)
			continue; // a later format addition: skipped wherever it is
		const char* aMisplaced = NULL;
		if (aHeaderIndex > 0)
		{
			if (isPastHeader || (aHeaderIndex <= aHeaderAt))
				aMisplaced = "RULE, RLID, META and SESS come first, each once, in that order";
			aHeaderAt = aHeaderIndex;
		}
		else
		{
			isPastHeader = true;
			if ((aTag == TAG_LDSV) && (hasPendingSave || hasPendingRng || theFile->mHasSaveHash))
				aMisplaced = "an LDSV comes once before the RNGS and the SEGM it goes with";
			else if ((aTag == TAG_RNGS) && (hasPendingRng || theFile->mHasSaveHash))
				aMisplaced = "an RNGS comes once before a SEGM, or after the last one before SVHA";
			else if ((aTag == TAG_SEGM) && theFile->mHasSaveHash)
				aMisplaced = "a SEGM after SVHA";
			else if ((aTag == TAG_SVHA) && (theFile->mHasSaveHash || hasPendingSave))
				aMisplaced = "SVHA comes once, after the last SEGM (an LDSV needs a SEGM after it)";
			else if ((aTag == TAG_END) && hasPendingSave)
				aMisplaced = "an LDSV with no SEGM after it";
		}
		if (aMisplaced != NULL)
		{
			anErr = TagName(aTag) + " at byte " + std::to_string(aStart - 8) + ": out of place (" + aMisplaced + ")";
			return false;
		}

		if (aTag == TAG_RULE)
		{
			theFile->mHasRules = true;
			theFile->mRules = theBytes.substr(aStart, aLen);
		}
		else if (aTag == TAG_RLID)
		{
			if (aLen != RULES_ID_SIZE)
			{
				anErr = "RLID: " + std::to_string(aLen) + " bytes, not 32";
				return false;
			}
			theFile->mHasRulesId = true;
			memcpy(theFile->mRulesId, theBytes.data() + aStart, RULES_ID_SIZE);
		}
		else if (aTag == TAG_META)
		{
			theFile->mHasMeta = true;
			theFile->mMeta = theBytes.substr(aStart, aLen);
		}
		else if (aTag == TAG_SESS)
		{
			theFile->mHasSession = true;
			theFile->mSession = theBytes.substr(aStart, aLen);
		}
		else if (aTag == TAG_LDSV)
		{
			hasPendingSave = true;
			aPendingSave = theBytes.substr(aStart, aLen);
		}
		else if (aTag == TAG_RNGS)
		{
			if ((aLen != RNGS_SIZE) || !ReadRng(aReader, &aPendingRng))
			{
				anErr = "RNGS: " + std::to_string(aLen) + " bytes, not " + std::to_string(RNGS_SIZE);
				return false;
			}
			hasPendingRng = true;
		}
		else if (aTag == TAG_SEGM)
		{
			if (theFile->mSegments.size() >= (size_t) MAX_SEGMENTS)
			{
				anErr = "SEGM at byte " + std::to_string(aStart - 8) + ": more than the " + std::to_string((int) MAX_SEGMENTS) +
					" segments a replay may have";
				return false;
			}
			theFile->mSegments.push_back(Segment());
			Segment& aSegment = theFile->mSegments.back();
			if (!ReadSegment(aReader, &aSegment, &anEventBudget, &anErr))
			{
				anErr = "segment " + std::to_string(theFile->mSegments.size() - 1) + ": " + anErr;
				return false;
			}
			aSegment.mHasLoadRng = hasPendingRng;
			aSegment.mLoadRng = aPendingRng;
			aSegment.mHasLoadSave = hasPendingSave;
			aSegment.mLoadSave.swap(aPendingSave);
			hasPendingRng = false;
			hasPendingSave = false;
			aPendingSave.clear();
		}
		else if (aTag == TAG_SVHA)
		{
			theFile->mSaveHash = aReader.U32();
			if (!aReader.AtEnd())
			{
				anErr = "SVHA: " + std::to_string(aLen) + " bytes, not 4";
				return false;
			}
			theFile->mHasSaveHash = true;
		}
		else if (aTag == TAG_END)
		{
			EndInfo& anEnd = theFile->mEnd;
			anEnd.mReason = aReader.U8();
			anEnd.mEndTick = aReader.U32();
			anEnd.mScore = (int32_t) aReader.U32();
			anEnd.mLevel = (int32_t) aReader.U32();
			anEnd.mGems = (int32_t) aReader.U32();
			anEnd.mLogicHash = aReader.U64();
			anEnd.mBoardDraws = aReader.U32();
			if ((aLen != END_SIZE) || !aReader.AtEnd())
			{
				anErr = "END: " + std::to_string(aLen) + " bytes, not " + std::to_string(END_SIZE);
				return false;
			}
			if (anEnd.mReason > END_SAVED)
			{
				anErr = "END: reason " + std::to_string(anEnd.mReason) + " (0..5)";
				return false;
			}
			hasEnd = true;
		}
	}
	if (!hasEnd)
	{
		anErr = "no END record (truncated?)";
		return false;
	}
	theFile->mHasTrailingRng = hasPendingRng;
	theFile->mTrailingRng = aPendingRng;
	return true;
}
