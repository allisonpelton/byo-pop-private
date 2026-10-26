// port(variant): the board state's field table and its encodings (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.6); see StateCodec.h. Not part of the original game.
//
// The table is VisitBoard below: one call per field, in Board.h's declaration order, with its flags. The
// same list drives the writer (FULL, LOGIC, SNAPSHOT, UNDO) and the SNAPSHOT and UNDO reader, so the two
// cannot drift apart. A field added to Board needs a row here (and in the stage-0 list of testhooks/StateHash.cpp's
// LOGIC column if play reads it); its FULL bytes go after the existing ones only if the -tracehash
// baselines are recorded again.

#include "StateCodec.h"
#include "Board.h"
#include "Common.h"
#include "MTRand.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

namespace
{

const int FULL = StateCodec::F_FULL;
const int LOGIC = StateCodec::F_LOGIC;
const int SNAP = StateCodec::F_SNAP;
const int V2 = StateCodec::F_V2;
const int UNDO = StateCodec::F_UNDO;
const int DERIVED = StateCodec::F_DERIVED;

// The RNG Board::Rand draws from: the board's own, else the global stream
void BoardRngState(Board* theBoard, const MTRand* theOverride, uint32_t theState[MTRAND_N], int* theIndex)
{
	if (theOverride != NULL)
		theOverride->GetState(theState, theIndex);
	else if (theBoard->mOwnRng != NULL)
		theBoard->mOwnRng->GetState(theState, theIndex);
	else
		GetRandMTState(theState, theIndex);
}

class Writer
{
public:
	Board*			mBoard;
	int				mProfileFlag;
	bool			mSkipDerived;
	const MTRand*	mRng;
	std::string		mOut;             // mLen bytes written; Bytes() gives them
	size_t			mLen;
	std::vector<std::pair<size_t, const char*> >* mRows;
	std::vector<Piece*> mOrphans;     // SNAPSHOT: pieces outside the grid, in the order first referenced

	Writer(Board* theBoard, int theProfile, const MTRand* theRng, bool skipDerived, std::vector<std::pair<size_t, const char*> >* theRows) :
		mBoard(theBoard), mSkipDerived(skipDerived), mRng(theRng), mLen(0), mRows(theRows)
	{
		mProfileFlag = (theProfile == StateCodec::PROFILE_FULL) ? FULL : (theProfile == StateCodec::PROFILE_LOGIC) ? LOGIC :
			(theProfile == StateCodec::PROFILE_UNDO) ? UNDO : SNAP;
		// An 8x8 board's encodings are 3..7 KB: written into place without a reallocation (the difftest and
		// -codeccheck encode after every update)
		mOut.resize(8192);
	}

	bool Want(int theFlags, const char* theName)
	{
		if (((theFlags & mProfileFlag) == 0) || (mSkipDerived && ((theFlags & DERIVED) != 0)))
			return false;
		if (mRows != NULL)
			mRows->push_back(std::make_pair(mLen, theName));
		return true;
	}

	// Room for theSize more bytes; where they go
	char* Grow(size_t theSize)
	{
		if (mLen + theSize > mOut.size())
			mOut.resize(std::max(mOut.size() * 2, mLen + theSize));
		char* aPlace = &mOut[mLen];
		mLen += theSize;
		return aPlace;
	}

	std::string Bytes()
	{
		mOut.resize(mLen);
		return std::move(mOut);
	}

	static void LittleEndian(char* theOut, uint64_t theValue, int theSize)
	{
		for (int i = 0; i < theSize; i++)
			theOut[i] = (char) (uint8_t) (theValue >> (i * 8));
	}

	void PutByte(uint8_t theByte) { *Grow(1) = (char) theByte; }

	void PutInt(int theValue) { LittleEndian(Grow(4), (uint32_t) theValue, 4); }

	void PutBool(bool theValue) { PutByte(theValue ? 1 : 0); }

	void PutDouble(double theValue)
	{
		uint64_t aBits;
		memcpy(&aBits, &theValue, sizeof(aBits));
		LittleEndian(Grow(8), aBits, 8);
	}

	void PutRaw(const void* theData, size_t theSize)
	{
		if (theSize > 0)
			memcpy(Grow(theSize), theData, theSize);
	}

	void PutMt(const uint32_t theState[MTRAND_N], int theIndex)
	{
		char* aPlace = Grow((MTRAND_N + 1) * 4);
		for (int i = 0; i < MTRAND_N; i++)
			LittleEndian(aPlace + i * 4, theState[i], 4);
		LittleEndian(aPlace + MTRAND_N * 4, (uint32_t) theIndex, 4);
	}

	void Int(const char* theName, int& theValue, int theFlags) { if (Want(theFlags, theName)) PutInt(theValue); }
	void Bool(const char* theName, bool& theValue, int theFlags) { if (Want(theFlags, theName)) PutBool(theValue); }
	void Double(const char* theName, double& theValue, int theFlags) { if (Want(theFlags, theName)) PutDouble(theValue); }
	void DerivedBool(const char* theName, bool theValue, int theFlags) { if (Want(theFlags, theName)) PutBool(theValue); }
	void DerivedInt(const char* theName, int theValue, int theFlags) { if (Want(theFlags, theName)) PutInt(theValue); }
	// A variant board's own field: in its encodings only (a board of the original rules encodes as before)
	void VariantInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mAny) Int(theName, theValue, theFlags); }
	// A gravity rule set's field (stage 7c): read from the rules, not from the flags -forcevariant sets, so a board of
	// default rules encodes as before
	void GravityInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mRules.HasGravityRule()) Int(theName, theValue, theFlags); }
	void GravityBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mRules.HasGravityRule()) Bool(theName, theValue, theFlags); }
	// A twist or slide rule set's field (stage 10), read from the rules as the gravity rows
	void MovesInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mRules.mMoves.mTypes & (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE)) Int(theName, theValue, theFlags); }
	// A dirt rule set's rows (stage 7b), read from the rules as the gravity rows: the layers left on each cell (0 off dirt)
	// A gold rule set's rows (stage 12): the level's time, and the gold cells (1 gold, 0 not)
	void GoldInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mGold) Int(theName, theValue, theFlags); }
	void GoldCells(int theFlags)
	{
		if (!mBoard->mVariant.mGold || !Want(theFlags, "mGold"))
			return;
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
				PutByte(mBoard->mGold[aRow][aCol]);
		}
	}
	// An arrow rule set's rows (stage 18): each gem's arrow (0 none, else its direction + 1; 0 where no gem), the counts,
	// and the steps of STATE_ARROW_STEPPING (their count, then each one's cells)
	void ArrowInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mArrows) Int(theName, theValue, theFlags); }
	void ArrowBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mVariant.mArrows) Bool(theName, theValue, theFlags); }
	void ArrowCells(int theFlags)
	{
		if (!mBoard->mVariant.mArrows || !Want(theFlags, "mArrow"))
			return;
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
			{
				Piece* aPiece = mBoard->mBoard[aRow][aCol];
				PutByte((aPiece != NULL) ? (uint8_t) aPiece->mArrow : 0);
			}
		}
	}
	void ArrowSteps(int theFlags)
	{
		if (!mBoard->mVariant.mArrows || !Want(theFlags, "mArrowSteps"))
			return;
		PutInt((int) mBoard->mArrowSteps.size());
		for (const Board::ArrowStep& s : mBoard->mArrowSteps)
		{
			PutInt(s.mFromCol);
			PutInt(s.mFromRow);
			PutInt(s.mToCol);
			PutInt(s.mToRow);
		}
	}
	void DirtBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mDirtRules) Bool(theName, theValue, theFlags); }
	void DirtLayers(int theFlags)
	{
		if (!mBoard->mDirtRules || !Want(theFlags, "mDirt"))
			return;
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
				PutByte(mBoard->mLayout.IsDirt(aCol, aRow) ? mBoard->mLayout.mDirt[aRow][aCol] : 0);
		}
	}

	void Layout(int theFlags)
	{
		if (!Want(theFlags, "layout"))
			return;
		PutInt(mBoard->mCols);
		PutInt(mBoard->mRows);
	}

	// One piece: FULL the raw savegame record; LOGIC the fields play reads; SNAPSHOT the record and the
	// variant tail; UNDO the colour and the kind (the piece is at rest in its cell)
	void PutPiece(Piece* thePiece)
	{
		if (mProfileFlag == FULL)
			PutRaw(thePiece, Piece::SAVE_SIZE);
		else if (mProfileFlag == SNAP)
		{
			PutRaw(thePiece, Piece::SAVE_SIZE);
			PutInt(thePiece->mKind);
			PutInt(thePiece->mKindFrame);
		}
		else if (mProfileFlag == UNDO)
		{
			PutInt(thePiece->mColor);
			PutInt(thePiece->mKind);
		}
		else
		{
			PutInt(thePiece->mColor);
			PutInt(thePiece->mKind);
			PutInt(thePiece->mCol);
			PutInt(thePiece->mRow);
			int aState = mBoard->mState;
			// libm's cos/sin set the positions in the swap and vortex states; nothing play decides reads them there
			if ((aState != STATE_SWAPPING) && (aState != STATE_SWAPPING_BACK) && (aState != STATE_BLACK_HOLE))
			{
				PutDouble(thePiece->mX);
				PutDouble(thePiece->mY);
			}
			PutInt(thePiece->mVelX);
			PutDouble(thePiece->mVelY);
			PutInt(thePiece->mSwapOffX);
			PutInt(thePiece->mSwapOffY);
			PutBool(thePiece->mDying);
			PutInt(thePiece->mDyingFrame);
		}
	}

	void Cells(int theFlags)
	{
		if (!Want(theFlags, "mBoard"))
			return;
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
			{
				Piece* aPiece = mBoard->mBoard[aRow][aCol];
				PutBool(aPiece != NULL);
				if (aPiece != NULL)
					PutPiece(aPiece);
			}
		}
	}

	void PieceRef(const char* theName, Piece*& thePiece, int theFlags)
	{
		if (!Want(theFlags, theName))
			return;
		if (thePiece == NULL)
		{
			PutInt(-1);
			return;
		}
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
			{
				if (mBoard->mBoard[aRow][aCol] == thePiece)
				{
					PutInt(aRow * mBoard->mCols + aCol);
					return;
				}
			}
		}
		int aCells = mBoard->mRows * mBoard->mCols;
		if (mProfileFlag != SNAP)
		{
			PutInt(aCells);
			PutPiece(thePiece);
			return;
		}
		// SNAPSHOT: the same piece outside the grid referenced twice stays one piece
		size_t anIndex = 0;
		while ((anIndex < mOrphans.size()) && (mOrphans[anIndex] != thePiece))
			anIndex++;
		PutInt(aCells + (int) anIndex);
		if (anIndex == mOrphans.size())
		{
			mOrphans.push_back(thePiece);
			PutPiece(thePiece);
		}
	}

	void Popups(int theFlags)
	{
		if (!Want(theFlags, "mScorePopups"))
			return;
		PutInt((int) mBoard->mScorePopups.size());
		for (std::list<ScorePopup>::iterator anItr = mBoard->mScorePopups.begin(); anItr != mBoard->mScorePopups.end(); ++anItr)
		{
			PutInt(anItr->mX);
			PutInt(anItr->mY);
			PutInt(anItr->mLife);
			PutInt(anItr->mUnused);
			PutInt(anItr->mPoints);
		}
	}

	// FULL: the stage-0 -tracehash list's own-RNG fields, only for a board with its own RNG. LOGIC: the RNG
	// Board::Rand draws from, whatever it is. SNAPSHOT and UNDO: everything, with its presence.
	void Rng(int theFlags)
	{
		if (!Want(theFlags, "rng"))
			return;
		Board* b = mBoard;
		uint32_t aState[MTRAND_N];
		int anIndex = 0;
		if (mProfileFlag == FULL)
		{
			if (b->mOwnRng == NULL)
				return;
			PutInt((int) b->mGameSeed);
			PutBool(b->mUserSeeded);
			PutBool(b->mTakesPendingSeed);
			PutInt((int) b->mBoardDraws);
			b->mOwnRng->GetState(aState, &anIndex);
			PutMt(aState, anIndex);
		}
		else if (mProfileFlag == LOGIC)
		{
			PutInt((int) b->mGameSeed);
			PutBool(b->mUserSeeded);
			PutBool(b->mRanked);
			PutInt((int) b->mBoardDraws);
			BoardRngState(b, mRng, aState, &anIndex);
			PutMt(aState, anIndex);
		}
		else
		{
			PutBool(b->mOwnRng != NULL);
			PutInt((int) b->mGameSeed);
			PutBool(b->mUserSeeded);
			PutBool(b->mRanked);
			PutBool(b->mTakesPendingSeed);
			PutInt((int) b->mBoardDraws);
			if (b->mOwnRng != NULL)
			{
				b->mOwnRng->GetState(aState, &anIndex);
				PutMt(aState, anIndex);
			}
		}
	}
};

class Reader
{
public:
	Board*			mBoard;
	const std::string& mBytes;
	size_t			mPos;
	bool			mOk;
	std::string		mError;
	std::vector<Piece*> mOrphans;
	int				mFlag;            // the rows it reads: SNAP, or UNDO for an undo snapshot

	Reader(Board* theBoard, const std::string& theBytes, int theFlag) : mBoard(theBoard), mBytes(theBytes), mPos(0), mOk(true), mFlag(theFlag) {}

	void Fail(const std::string& theError)
	{
		if (mOk)
			mError = theError + " at byte " + std::to_string(mPos);
		mOk = false;
	}

	bool Has(size_t theCount, const char* theName)
	{
		if (mOk && (theCount <= mBytes.size() - mPos))
			return true;
		Fail(std::string("truncated in ") + theName);
		return false;
	}

	int GetInt(const char* theName)
	{
		if (!Has(4, theName))
			return 0;
		uint32_t aValue = 0;
		for (int i = 0; i < 4; i++)
			aValue |= (uint32_t) (uint8_t) mBytes[mPos + i] << (i * 8);
		mPos += 4;
		return (int) aValue;
	}

	bool GetBool(const char* theName)
	{
		if (!Has(1, theName))
			return false;
		uint8_t aByte = (uint8_t) mBytes[mPos++];
		if (aByte > 1)
			Fail(std::string("not a bool in ") + theName);
		return aByte != 0;
	}

	double GetDouble(const char* theName)
	{
		if (!Has(8, theName))
			return 0.0;
		uint64_t aBits = 0;
		for (int i = 0; i < 8; i++)
			aBits |= (uint64_t) (uint8_t) mBytes[mPos + i] << (i * 8);
		mPos += 8;
		double aValue;
		memcpy(&aValue, &aBits, sizeof(aValue));
		return aValue;
	}

	void GetMt(uint32_t theState[MTRAND_N], int* theIndex)
	{
		for (int i = 0; i < MTRAND_N; i++)
			theState[i] = (uint32_t) GetInt("rng");
		*theIndex = GetInt("rng");
	}

	void Int(const char* theName, int& theValue, int theFlags) { if (theFlags & mFlag) theValue = GetInt(theName); }
	void Bool(const char* theName, bool& theValue, int theFlags) { if (theFlags & mFlag) theValue = GetBool(theName); }
	void Double(const char* theName, double& theValue, int theFlags) { if (theFlags & mFlag) theValue = GetDouble(theName); }
	void DerivedBool(const char*, bool, int) {}
	void DerivedInt(const char*, int, int) {}
	// The target board's flags say whether the snapshot has the row (RoundTrip gives it the source's)
	void VariantInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mAny) Int(theName, theValue, theFlags); }
	void GravityInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mRules.HasGravityRule()) Int(theName, theValue, theFlags); }
	void GravityBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mRules.HasGravityRule()) Bool(theName, theValue, theFlags); }
	void MovesInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mRules.mMoves.mTypes & (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE)) Int(theName, theValue, theFlags); }
	void DirtBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mDirtRules) Bool(theName, theValue, theFlags); }
	void GoldInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mGold) Int(theName, theValue, theFlags); }
	void GoldCells(int theFlags)
	{
		if (!mBoard->mVariant.mGold || ((theFlags & mFlag) == 0))
			return;
		memset(mBoard->mGold, 0, sizeof(mBoard->mGold));
		for (int i = 0; i < mBoard->mRows * mBoard->mCols; i++)
		{
			if (!Has(1, "mGold"))
				return;
			uint8_t aGold = (uint8_t) mBytes[mPos++];
			if (aGold > 1)
				Fail("gold " + std::to_string(aGold) + " at cell (" + std::to_string(i % mBoard->mCols) + "," + std::to_string(i / mBoard->mCols) + ")");
			mBoard->mGold[i / mBoard->mCols][i % mBoard->mCols] = aGold;
		}
	}
	// After the cells: an arrow only on a gem; a step from a cell to the next one in a line, or a cell off the board
	void ArrowInt(const char* theName, int& theValue, int theFlags) { if (mBoard->mVariant.mArrows) Int(theName, theValue, theFlags); }
	void ArrowBool(const char* theName, bool& theValue, int theFlags) { if (mBoard->mVariant.mArrows) Bool(theName, theValue, theFlags); }
	void ArrowCells(int theFlags)
	{
		if (!mBoard->mVariant.mArrows || ((theFlags & mFlag) == 0))
			return;
		for (int i = 0; i < mBoard->mRows * mBoard->mCols; i++)
		{
			if (!Has(1, "mArrow"))
				return;
			uint8_t anArrow = (uint8_t) mBytes[mPos++];
			Piece* aPiece = mBoard->mBoard[i / mBoard->mCols][i % mBoard->mCols];
			if ((anArrow > GameRules::NUM_ARROW_DIRECTIONS) || ((anArrow > 0) && (aPiece == NULL)))
				Fail("arrow " + std::to_string(anArrow) + " at cell (" + std::to_string(i % mBoard->mCols) + "," + std::to_string(i / mBoard->mCols) + ")");
			else if (aPiece != NULL)
				aPiece->mArrow = anArrow;
		}
	}
	void ArrowSteps(int theFlags)
	{
		if (!mBoard->mVariant.mArrows || ((theFlags & mFlag) == 0))
			return;
		mBoard->mArrowSteps.clear();
		int aCount = GetInt("mArrowSteps");
		if (mOk && ((aCount < 0) || (aCount > mBoard->mRows * mBoard->mCols)))
			Fail(std::to_string(aCount) + " arrow steps");
		for (int i = 0; mOk && (i < aCount); i++)
		{
			Board::ArrowStep s;
			s.mFromCol = GetInt("mArrowSteps");
			s.mFromRow = GetInt("mArrowSteps");
			s.mToCol = GetInt("mArrowSteps");
			s.mToRow = GetInt("mArrowSteps");
			// A step goes along a row or a column, or along an exact diagonal (the diagonal arrows)
			bool isFromCell = (s.mFromCol >= 0) && (s.mFromCol < mBoard->mCols) && (s.mFromRow >= 0) && (s.mFromRow < mBoard->mRows);
			int aColStep = std::abs(s.mToCol - s.mFromCol), aRowStep = std::abs(s.mToRow - s.mFromRow);
			bool isToNear = (s.mToCol >= -1) && (s.mToCol <= mBoard->mCols) && (s.mToRow >= -1) && (s.mToRow <= mBoard->mRows) &&
				(((aColStep == 0) != (aRowStep == 0)) || ((aColStep == aRowStep) && (aColStep > 0)));
			if (mOk && (!isFromCell || !isToNear))
				Fail("an arrow step from (" + std::to_string(s.mFromCol) + "," + std::to_string(s.mFromRow) + ") to (" +
					std::to_string(s.mToCol) + "," + std::to_string(s.mToRow) + ")");
			mBoard->mArrowSteps.push_back(s);
		}
	}
	// After the cells: a cell with layers holds no gem
	void DirtLayers(int theFlags)
	{
		if (!mBoard->mDirtRules || ((theFlags & mFlag) == 0))
			return;
		std::vector<uint8_t> aLayers;
		for (int i = 0; i < mBoard->mRows * mBoard->mCols; i++)
		{
			if (!Has(1, "mDirt"))
				return;
			aLayers.push_back((uint8_t) mBytes[mPos++]);
			if ((aLayers.back() > 0) && (mBoard->mBoard[i / mBoard->mCols][i % mBoard->mCols] != NULL))
				Fail("a gem on the dirt at cell (" + std::to_string(i % mBoard->mCols) + "," + std::to_string(i / mBoard->mCols) + ")");
		}
		std::string aWhy;
		if (mOk && !mBoard->ApplyDirtLayers(aLayers, &aWhy))
			Fail(aWhy);
	}

	void Layout(int theFlags)
	{
		if ((theFlags & mFlag) == 0)
			return;
		int aCols = GetInt("layout");
		int aRows = GetInt("layout");
		if (mOk && ((aCols != mBoard->mCols) || (aRows != mBoard->mRows)))
			Fail("a " + std::to_string(aCols) + "x" + std::to_string(aRows) + " snapshot for a " + std::to_string(mBoard->mCols) + "x" +
				std::to_string(mBoard->mRows) + " board");
	}

	Piece* NewPiece()
	{
		if (!Has(Piece::SAVE_SIZE + 8, "piece"))
			return NULL;
		Piece* aPiece = new Piece();
		memcpy((void*) aPiece, mBytes.data() + mPos, Piece::SAVE_SIZE);
		mPos += Piece::SAVE_SIZE;
		aPiece->mKind = GetInt("piece");
		aPiece->mKindFrame = GetInt("piece");
		return aPiece;
	}

	// UNDO: a fresh piece at rest in its cell, its cosmetic fields zero (Piece())
	Piece* NewRestingPiece(int theCol, int theRow)
	{
		if (!Has(8, "piece"))
			return NULL;
		Piece* aPiece = new Piece();
		aPiece->mColor = GetInt("piece");
		aPiece->mKind = GetInt("piece");
		aPiece->mCol = theCol;
		aPiece->mRow = theRow;
		aPiece->mX = mBoard->GetColX(theCol);
		aPiece->mY = mBoard->GetRowY(theRow);
		return aPiece;
	}

	void Cells(int theFlags)
	{
		if ((theFlags & mFlag) == 0)
			return;
		for (int aRow = 0; aRow < mBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < mBoard->mCols; aCol++)
			{
				delete mBoard->mBoard[aRow][aCol];
				mBoard->mBoard[aRow][aCol] = NULL;
				if (GetBool("mBoard"))
					mBoard->mBoard[aRow][aCol] = (mFlag == UNDO) ? NewRestingPiece(aCol, aRow) : NewPiece();
			}
		}
	}

	void PieceRef(const char* theName, Piece*& thePiece, int theFlags)
	{
		if ((theFlags & mFlag) == 0)
			return;
		int aRef = GetInt(theName);
		int aCells = mBoard->mRows * mBoard->mCols;
		thePiece = NULL;
		if (!mOk || (aRef == -1))
			return;
		if ((aRef >= 0) && (aRef < aCells))
			thePiece = mBoard->mBoard[aRef / mBoard->mCols][aRef % mBoard->mCols];
		else if ((aRef >= aCells) && ((size_t) (aRef - aCells) < mOrphans.size()))
			thePiece = mOrphans[aRef - aCells];
		else if ((aRef >= aCells) && ((size_t) (aRef - aCells) == mOrphans.size()))
		{
			thePiece = NewPiece();
			if (thePiece != NULL)
			{
				mOrphans.push_back(thePiece);
				// Owned like the pieces FillBoard parks, so the board frees it
				mBoard->mOrphanedPieces.push_back(thePiece);
			}
		}
		else
			Fail(std::string("bad piece reference in ") + theName);
	}

	void Popups(int theFlags)
	{
		if ((theFlags & mFlag) == 0)
			return;
		mBoard->mScorePopups.clear();
		int aCount = GetInt("mScorePopups");
		if ((aCount < 0) || ((size_t) aCount > (mBytes.size() - mPos) / 20))
		{
			Fail("bad popup count");
			return;
		}
		for (int i = 0; i < aCount; i++)
		{
			ScorePopup aPopup;
			aPopup.mX = GetInt("mScorePopups");
			aPopup.mY = GetInt("mScorePopups");
			aPopup.mLife = GetInt("mScorePopups");
			aPopup.mUnused = GetInt("mScorePopups");
			aPopup.mPoints = GetInt("mScorePopups");
			mBoard->mScorePopups.push_back(aPopup);
		}
	}

	void Rng(int theFlags)
	{
		if ((theFlags & mFlag) == 0)
			return;
		Board* b = mBoard;
		bool hasOwn = GetBool("rng");
		b->mGameSeed = (uint32_t) GetInt("rng");
		b->mUserSeeded = GetBool("rng");
		b->mRanked = GetBool("rng");
		b->mTakesPendingSeed = GetBool("rng");
		b->mBoardDraws = (uint32_t) GetInt("rng");
		delete b->mOwnRng;
		b->mOwnRng = NULL;
		if (hasOwn)
		{
			uint32_t aState[MTRAND_N];
			int anIndex = 0;
			GetMt(aState, &anIndex);
			if (mOk)
			{
				b->mOwnRng = new MTRand(1);
				if (!b->mOwnRng->SetState(aState, anIndex))
					Fail("an RNG state MTRand refuses");
			}
		}
#ifndef BEJ_NO_VARIANTS
		b->mVariant.mOwnRng = (b->mOwnRng != NULL);
#endif
	}
};

// The table: every Board field in declaration order (Board.h), with the encodings that take it
template<class V> void VisitBoard(V& v, Board* b)
{
	v.Layout(LOGIC | SNAP | V2 | UNDO);
	v.Int("mUpdateCnt", b->mUpdateCnt, FULL | LOGIC | SNAP);   // the board tick (WidgetContainer)
	v.Int("mLevel", b->mLevel, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mDisplayLevel", b->mDisplayLevel, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Cells(FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mState", b->mState, FULL | LOGIC | SNAP | V2);
	v.Bool("mTimedMode", b->mTimedMode, FULL | LOGIC | SNAP | V2);
	v.Bool("mIsNewGame", b->mIsNewGame, FULL | LOGIC | SNAP | V2 | UNDO);
	v.PieceRef("mSelectedPiece", b->mSelectedPiece, FULL | LOGIC | SNAP);
	v.Int("mCursorFrame", b->mCursorFrame, FULL | SNAP);
	v.Bool("mShowTutorial", b->mShowTutorial, FULL | LOGIC | SNAP);
	v.Int("mTutorialBob", b->mTutorialBob, FULL | SNAP);
	v.Int("mTutorialBobDir", b->mTutorialBobDir, FULL | SNAP);
	v.Int("mTutorialCol", b->mTutorialCol, FULL | LOGIC | SNAP);
	v.Int("mTutorialRow", b->mTutorialRow, FULL | LOGIC | SNAP);
	v.Int("mTutorialCol2", b->mTutorialCol2, FULL | LOGIC | SNAP);
	v.PieceRef("mSwapPiece1", b->mSwapPiece1, FULL | LOGIC | SNAP);
	v.PieceRef("mSwapPiece2", b->mSwapPiece2, FULL | LOGIC | SNAP);
	v.Int("mSwapAngle", b->mSwapAngle, FULL | LOGIC | SNAP);
	v.Int("mSwapCenterX", b->mSwapCenterX, FULL | LOGIC | SNAP);
	v.Int("mSwapCenterY", b->mSwapCenterY, FULL | LOGIC | SNAP);
	v.Int("mBarWidth", b->mBarWidth, FULL | LOGIC | SNAP | V2);
	v.Int("mPlayStartTick", b->mPlayStartTick, FULL | LOGIC | SNAP);
	v.Int("mPlaySeconds", b->mPlaySeconds, FULL | LOGIC | SNAP | V2);
	v.Int("mGameDuration", b->mGameDuration, FULL | LOGIC | SNAP);
	v.Int("mIdleTicks", b->mIdleTicks, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mScore", b->mScore, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Double("mPointMultiplier", b->mPointMultiplier, FULL | LOGIC | SNAP | V2);
	v.Int("mDisplayScore", b->mDisplayScore, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mGemsCleared", b->mGemsCleared, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mMoveMatchCount", b->mMoveMatchCount, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mCascadeCount", b->mCascadeCount, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mLastLandSoundTick", b->mLastLandSoundTick, FULL | SNAP);
	v.Int("mGameOverCountdown", b->mGameOverCountdown, FULL | LOGIC | SNAP);
	v.Int("mLongestCascade", b->mLongestCascade, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mBestMoveScore", b->mBestMoveScore, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Int("mUnused218", b->mUnused218, FULL | LOGIC | SNAP);
	v.Int("mMoveScore", b->mMoveScore, FULL | LOGIC | SNAP | UNDO);
	v.Int("mMoveScoreTimer", b->mMoveScoreTimer, FULL | LOGIC | SNAP);
	v.Double("mMoveScoreAlpha", b->mMoveScoreAlpha, FULL | SNAP);
	v.Double("mHintGlowAlpha", b->mHintGlowAlpha, FULL | SNAP);
	v.Int("mHintGlowTimer", b->mHintGlowTimer, FULL | SNAP);
	v.Int("mHintGlowDuration", b->mHintGlowDuration, FULL | SNAP);
	v.Bool("mNewGamePending", b->mNewGamePending, FULL | LOGIC | SNAP);
	v.Int("mPauseCount", b->mPauseCount, FULL | LOGIC | SNAP);
	v.Int("mUserPauseCount", b->mUserPauseCount, FULL | LOGIC | SNAP);
	v.Popups(FULL | SNAP);
	v.Bool("mStatsDisabled", b->mStatsDisabled, FULL | LOGIC | SNAP | V2);
	v.Int("mUnused264", b->mUnused264, FULL | LOGIC | SNAP | V2);
	v.Int("mLevelStartScore", b->mLevelStartScore, FULL | LOGIC | SNAP | V2);
	v.Int("mLevelTargetScore", b->mLevelTargetScore, FULL | LOGIC | SNAP | V2);
	v.Int("mLevelPoints", b->mLevelPoints, FULL | LOGIC | SNAP | V2);
	v.Double("mTimeDrain", b->mTimeDrain, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Double("mTimeDrainRate", b->mTimeDrainRate, FULL | LOGIC | SNAP | V2 | UNDO);
	v.Double("mTimeDrainAccel", b->mTimeDrainAccel, FULL | LOGIC | SNAP | V2);
	v.Int("mBarColor", b->mBarColor, FULL | LOGIC | SNAP | V2);
	v.Int("mLastWarningTick", b->mLastWarningTick, FULL | SNAP);
	v.Int("mMouseX", b->mMouseX, FULL | SNAP);
	v.Int("mMouseY", b->mMouseY, FULL | SNAP);
	v.Int("mMouseDownX", b->mMouseDownX, FULL | SNAP);
	v.Int("mMouseDownY", b->mMouseDownY, FULL | SNAP);
	v.Bool("mMouseDown", b->mMouseDown, FULL | SNAP);
	v.Int("mInputDelay", b->mInputDelay, FULL | LOGIC | SNAP);
	v.Bool("mBackgroundChanged", b->mBackgroundChanged, FULL | SNAP);
	v.DerivedBool("mTextImage", b->mTextImage != NULL, FULL | DERIVED);
	v.DerivedBool("mTextColorImage", b->mTextColorImage != NULL, FULL | DERIVED);
	v.Bool("mShowingText", b->mShowingText, FULL | SNAP);
	v.Double("mTextPhase", b->mTextPhase, FULL | SNAP);
	v.Int("mTextDuration", b->mTextDuration, FULL | SNAP);
	v.Int("mTextTimeLeft", b->mTextTimeLeft, FULL | SNAP);
	v.Int("mWipeY", b->mWipeY, FULL | LOGIC | SNAP | V2);
	v.Int("mElectOffset", b->mElectOffset, FULL | LOGIC | SNAP);
	v.Int("mBlackHoleFrame", b->mBlackHoleFrame, FULL | LOGIC | SNAP | V2);
	v.Int("mTimeDrainDelay", b->mTimeDrainDelay, FULL | LOGIC | SNAP | UNDO);
	v.Double("mBlackHoleLevel", b->mBlackHoleLevel, FULL | LOGIC | SNAP | V2);
	v.Bool("mLevelTextShown", b->mLevelTextShown, FULL | LOGIC | SNAP | V2);
	v.DerivedInt("mOrphanedPieces", (int) b->mOrphanedPieces.size(), FULL | DERIVED);
	// port(variant) members (stage 2: the board's RNG; mOwnRng, mGameSeed, mUserSeeded, mRanked, mBoardDraws,
	// mTakesPendingSeed). Not in the table: mRules, mVariant, mLayout (the rules snapshot and what follows
	// from it), the recorder and playback members, mRngStart (replay bookkeeping), mBoardSerial and mResumed
	// (the LiveSplit observer's bookkeeping, stage 2b: design 4.15 gives them no row)
	v.Rng(FULL | LOGIC | SNAP | V2 | UNDO);
	// Stage 5, the moves (the v2 save's MOVE chunk), on a variant board only. Not FULL: that stays the stage-0
	// list and the board's own RNG, so a rule set that plays as the original (the oracle's inert cases) hashes
	// as its reference; LOGIC (the replays' CHECK and END) and SNAPSHOT take them
	v.VariantInt("mMoveCount", b->mMoveCount, LOGIC | SNAP | V2 | UNDO);
	v.VariantInt("mInvalidMoves", b->mInvalidMoves, LOGIC | SNAP | V2 | UNDO);
	v.VariantInt("mUndosUsed", b->mUndosUsed, LOGIC | SNAP | V2);
	// Stage 7, the level's shape (index into board.levels; the v2 save's header holds it). The layout itself follows
	// from the rules, this index and the dirt layers below (not in the table, like mLayout and mPendingLayout, which equals mLayout between
	// ticks); not LOGIC, which has mLevel and the layout's size (so the replays' hashes of stage 6 still hold), nor
	// UNDO, since an undo never crosses a level
	v.VariantInt("mLevelShape", b->mLevelShape, SNAP | V2);
	// Stage 7c, gravity (the v2 save's GRAV chunk), only when the rules set a gravity rule: FULL too, since no such
	// rule set plays as the original; UNDO, since an undo restores the direction "alternate" points in
	v.GravityInt("mGravityFlips", b->mGravityFlips, FULL | LOGIC | SNAP | V2 | UNDO);
	v.GravityBool("mGravityNoFlip", b->mGravityNoFlip, FULL | LOGIC | SNAP | V2 | UNDO);
	// Stage 7b, dirt (the v2 save's DIRT chunk), only when the rules have dirt: the layers left, which make mLayout
	// differ from the level's mask, and the side the next diagonal slide tries first. FULL too, since no such rule set
	// plays as the original; UNDO, since an undo brings the dirt back
	v.DirtLayers(FULL | LOGIC | SNAP | V2 | UNDO);
	v.DirtBool("mDiagonalLeftFirst", b->mDiagonalLeftFirst, FULL | LOGIC | SNAP | V2 | UNDO);
	// Stage 12, the gold board (the v2 save's GOLD and GTIM chunks), only when the rules have gold: the gold cells and the
	// level's time. FULL too, since no such rule set plays as the original; UNDO, since an undo brings the gold back (and
	// the time left with undo.restoreTime: Board::Undo)
	v.GoldCells(FULL | LOGIC | SNAP | V2 | UNDO);
	v.GoldInt("mGoldTicksLeft", b->mGoldTicksLeft, FULL | LOGIC | SNAP | V2 | UNDO);
	v.GoldInt("mGoldTickLimit", b->mGoldTickLimit, FULL | LOGIC | SNAP | V2);
	// Stage 10, twists and slides, only when the rules have them: the move of states 10..13 (the grid holds the board
	// before it, so a save needs none of it: not V2) and its animation. FULL too, since no such rule set plays as the
	// original; not UNDO, since an undo is taken in STATE_PLAYING
	v.MovesInt("mMoveInPlay.mType", b->mMoveInPlay.mType, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveInPlay.mA", b->mMoveInPlay.mA, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveInPlay.mB", b->mMoveInPlay.mB, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveInPlay.mC", b->mMoveInPlay.mC, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveInPlay.mD", b->mMoveInPlay.mD, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveInPlay.mE", b->mMoveInPlay.mE, FULL | LOGIC | SNAP);
	v.MovesInt("mMoveTick", b->mMoveTick, FULL | LOGIC | SNAP);
	v.MovesInt("mSlideFrom", b->mSlideFrom, FULL | LOGIC | SNAP);
	v.MovesInt("mSlideTo", b->mSlideTo, FULL | LOGIC | SNAP);
	// Stage 18, arrow gems (the v2 save's ARRW chunk), only when the rules have them: each gem's arrow, the moves toward
	// the next and the arrows cleared (the bonus's factor). FULL too, since no such rule set plays as the original; UNDO,
	// since an undo brings the arrows back. The step a move made due, and STATE_ARROW_STEPPING's steps (the grid holds
	// them done) with their animation: not UNDO, which is taken in STATE_PLAYING; a save holds the steps done
	// (Board::GetVariantSave)
	v.ArrowCells(FULL | LOGIC | SNAP | V2 | UNDO);
	v.ArrowInt("mArrowMoves", b->mArrowMoves, FULL | LOGIC | SNAP | V2 | UNDO);
	v.ArrowInt("mArrowsCleared", b->mArrowsCleared, FULL | LOGIC | SNAP | V2 | UNDO);
	v.ArrowBool("mArrowStepDue", b->mArrowStepDue, FULL | LOGIC | SNAP | V2);
	v.ArrowSteps(FULL | LOGIC | SNAP);
	v.ArrowInt("mArrowTick", b->mArrowTick, FULL | LOGIC | SNAP);
}

}

std::string StateCodec::Encode(Board* theBoard, int theProfile, const MTRand* theRng, bool skipDerived,
	std::vector<std::pair<size_t, const char*> >* theRows)
{
	Writer aWriter(theBoard, theProfile, theRng, skipDerived, theRows);
	VisitBoard(aWriter, theBoard);
	return aWriter.Bytes();
}

bool StateCodec::Decode(Board* theBoard, const std::string& theBytes, std::string* theError, int theProfile)
{
	Reader aReader(theBoard, theBytes, (theProfile == PROFILE_UNDO) ? UNDO : SNAP);
	VisitBoard(aReader, theBoard);
	if (aReader.mOk && (aReader.mPos != theBytes.size()))
		aReader.Fail(std::to_string(theBytes.size() - aReader.mPos) + " bytes after the snapshot");
	if (!aReader.mOk && (theError != NULL))
		*theError = aReader.mError;
	return aReader.mOk;
}

uint64_t StateCodec::Fnv1a(const std::string& theBytes)
{
	uint64_t aHash = 14695981039346656037ULL;
	for (size_t i = 0; i < theBytes.size(); i++)
	{
		aHash ^= (uint8_t) theBytes[i];
		aHash *= 1099511628211ULL;
	}
	return aHash;
}

uint64_t StateCodec::LogicHash(Board* theBoard, const MTRand* theRng)
{
	return Fnv1a(Encode(theBoard, PROFILE_LOGIC, theRng));
}

// The first row at which two encodings differ, for the message
static std::string FirstDifference(const std::string& theA, const std::string& theB, const std::vector<std::pair<size_t, const char*> >& theRows)
{
	size_t aPos = 0;
	while ((aPos < theA.size()) && (aPos < theB.size()) && (theA[aPos] == theB[aPos]))
		aPos++;
	const char* aName = "?";
	for (size_t i = 0; i < theRows.size(); i++)
	{
		if (theRows[i].first <= aPos)
			aName = theRows[i].second;
	}
	return "byte " + std::to_string(aPos) + " (" + aName + "), " + std::to_string(theA.size()) + " vs " + std::to_string(theB.size()) + " bytes";
}

std::string StateCodec::RoundTrip(Board* theBoard, Board* theScratch)
{
	theScratch->mRules = theBoard->mRules;
#ifndef BEJ_NO_VARIANTS
	theScratch->mVariant = theBoard->mVariant;
#endif
	theScratch->mLayout = theBoard->mLayout;
	theScratch->mPendingLayout = theBoard->mPendingLayout;
	theScratch->mDirtRules = theBoard->mDirtRules;
	theScratch->mCols = theBoard->mCols;
	theScratch->mRows = theBoard->mRows;
	// The pieces outside the grid the last restore made (Decode replaces the grid's itself)
	for (size_t i = 0; i < theScratch->mOrphanedPieces.size(); i++)
		delete theScratch->mOrphanedPieces[i];
	theScratch->mOrphanedPieces.clear();

	std::vector<std::pair<size_t, const char*> > aRows;
	std::string aSnapshot = Encode(theBoard, PROFILE_SNAPSHOT, NULL, false, &aRows);
	std::string anError;
	if (!Decode(theScratch, aSnapshot, &anError))
		return "snapshot decode: " + anError;
	std::string anAgain = Encode(theScratch, PROFILE_SNAPSHOT);
	if (anAgain != aSnapshot)
		return "snapshot written again differs at " + FirstDifference(aSnapshot, anAgain, aRows);

	aRows.clear();
	std::string aFull = Encode(theBoard, PROFILE_FULL, NULL, true, &aRows);
	std::string aScratchFull = Encode(theScratch, PROFILE_FULL, NULL, true);
	if (aFull != aScratchFull)
		return "FULL of the restored board differs at " + FirstDifference(aFull, aScratchFull, aRows);

	aRows.clear();
	std::string aLogic = Encode(theBoard, PROFILE_LOGIC, NULL, false, &aRows);
	std::string aScratchLogic = Encode(theScratch, PROFILE_LOGIC);
	if (aLogic != aScratchLogic)
		return "LOGIC of the restored board differs at " + FirstDifference(aLogic, aScratchLogic, aRows);
	return "";
}

std::string StateCodec::UndoRoundTrip(Board* theBoard, Board* theScratch)
{
	std::vector<std::pair<size_t, const char*> > aRows;
	std::string anUndo = Encode(theBoard, PROFILE_UNDO, NULL, false, &aRows);
	std::string anError;
	// The restore replaces the scratch's pieces: nothing may point at the old ones
	theScratch->mSelectedPiece = NULL;
	theScratch->mSwapPiece1 = NULL;
	theScratch->mSwapPiece2 = NULL;
	if (!Decode(theScratch, anUndo, &anError, PROFILE_UNDO))
		return "undo snapshot decode: " + anError;
	std::string anAgain = Encode(theScratch, PROFILE_UNDO);
	if (anAgain != anUndo)
		return "undo snapshot written again differs at " + FirstDifference(anUndo, anAgain, aRows);
	return "";
}
