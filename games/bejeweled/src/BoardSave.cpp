// port(variant): the v2 save of a variant board (games/bejeweled/docs/CODE_MODS_DESIGN.md 7.2), Board's part. Not
// part of the original game. The format is Replay.h's (Replay::VariantSave): the version 0x101 and a header with
// the board's RulesId, its size, its level shape and its mask, then the v1 field sequence that Board::WriteSaveGame
// and Board::LoadGame (Board.cpp) write and read as for a v1 save, then the chunks: RNGS (the RNG the game goes
// on with) and MOVE (the move counts), END.
//
// Writing: WriteSaveGame writes the header after the version and the chunks after the v1 fields, only for a
// variant board (VariantFlags::mAny); every other board writes the v1 save byte for byte as before.
// Reading: a variant board takes only a v2 save, and only one of its own rules and shape. LoadGame hands the
// version to ReadVariantHeader, which checks the whole file before the board changes (the header, where the v1
// fields end, the chunks, the RNG state MTRand must take) and leaves the buffer at the v1 fields; a v1 save, a
// save of other rules (a mod edited without a new version: another RulesId under the same file name) or of
// another shape, or a file cut short is refused, and LoadGame returns false as for a v1 save of another
// version. The v1 check refuses a v2 save on any other board, so neither kind loads where it does not belong.
// GameLoadedVariant then gives the board the chunks' RNG and counts (ReadVariantChunks). As with v1, a resume
// does not restore mUpdateCnt, mTimeDrainDelay, the timers or the popups (6.5): it is a new segment.
//
// Stage 7: the save's level shape is one of the rules' (board.levels, or the one board.cols/rows/mask give), the
// one of its level (mLevel % their count), and its size and mask are that shape's; ReadVariantHeader then gives the
// board that layout (SetLoadedLayout, BoardShape.cpp) before LoadGame reads the cells.

#include "Board.h"
#include "MTRand.h"

#include <SDL.h>
#include <cstring>
#include <vector>

using namespace Sexy;

void Board::GetVariantSave(Replay::VariantSave* theSave)
{
	Replay::VariantSave& s = *theSave;
	s = Replay::VariantSave();
	mRules.RulesId(s.mRulesId);
	s.mCols = mCols;
	s.mRows = mRows;
	s.mLevelShape = mLevelShape;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
			s.mPlayable.push_back(mLayout.IsPlayable(aCol, aRow));
	}
	GetRngState(&s.mRng);
	s.mMoves = mMoveCount;
	s.mInvalidMoves = mInvalidMoves;
	s.mUndosUsed = mUndosUsed;
	// GRAV (stage 7c) when the rules set a gravity rule, as in StateCodec
	s.mHasGravity = mRules.HasGravityRule();
	s.mGravityFlips = mGravityFlips;
	s.mGravityNoFlip = mGravityNoFlip;
	// DIRT (stage 7b) when the rules have dirt, as in StateCodec
	s.mHasDirt = mDirtRules;
	s.mDiagonalLeftFirst = mDiagonalLeftFirst;
	if (s.mHasDirt)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
				s.mDirtLayers.push_back(mLayout.IsDirt(aCol, aRow) ? mLayout.mDirt[aRow][aCol] : 0);
		}
	}
	// KIND (stage 11) when the rules have specials: each gem's kind
	s.mHasKinds = mVariant.mSpecials;
	if (s.mHasKinds)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
				s.mKinds.push_back((mBoard[aRow][aCol] != NULL) ? (uint8_t) mBoard[aRow][aCol]->mKind : 0);
		}
	}
	// GOLD and GTIM (stage 12) when the rules have gold: the gold cells, the level's time
	s.mHasGold = mVariant.mGold;
	s.mHasGoldTime = mVariant.mGold;
	if (s.mHasGold)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
				s.mGold.push_back(mGold[aRow][aCol]);
		}
		s.mGoldTicksLeft = mGoldTicksLeft;
		s.mGoldTickLimit = mGoldTickLimit;
	}
	// ARRW (stage 18) when the rules have arrows. A save in STATE_ARROW_STEPPING holds the steps done (WriteSaveGame's
	// STATE_FALLING); an escape traded nothing, so it holds the settle before it, the step due again
	s.mHasArrows = mVariant.mArrows;
	if (s.mHasArrows)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
				s.mArrows.push_back((mBoard[aRow][aCol] != NULL) ? (uint8_t) mBoard[aRow][aCol]->mArrow : 0);
		}
		bool isEscaping = (mState == STATE_ARROW_STEPPING) && IsArrowEscaping();
		s.mArrowMoves = mArrowMoves - (isEscaping ? 1 : 0);
		s.mArrowsCleared = mArrowsCleared;
		s.mArrowStepDue = mArrowStepDue || isEscaping;
	}
}

void Board::WriteVariantHeader(Buffer* theBuffer)
{
	Replay::VariantSave aSave;
	GetVariantSave(&aSave);
	std::string aHeader = Replay::VariantSaveHeader(aSave);
	theBuffer->WriteBytes((const uchar*) aHeader.data(), (int) aHeader.size());
}

void Board::WriteVariantChunks(Buffer* theBuffer)
{
	Replay::VariantSave aSave;
	GetVariantSave(&aSave);
	std::string aChunks = Replay::VariantSaveChunks(aSave);
	theBuffer->WriteBytes((const uchar*) aChunks.data(), (int) aChunks.size());
}

static std::string BufferText(const Buffer& theBuffer)
{
	if (theBuffer.GetDataLen() <= 0)
		return "";
	return std::string((const char*) theBuffer.GetDataPtr(), (size_t) theBuffer.GetDataLen());
}

// LoadGame (0x4cca98), after it read the version. True leaves the buffer at the v1 fields (mTimedMode).
bool Board::ReadVariantHeader(const Buffer& theBuffer, int theVersion)
{
	std::string aBytes = BufferText(theBuffer);
	Replay::VariantSave aSave;
	BoardLayout aLayout;
	std::string aWhy;
	if (theVersion != SAVEGAME_VERSION_VARIANT)
		aWhy = "a v" + std::to_string(theVersion) + " save, and a variant board takes only the v2 save";
	else if (Replay::ReadVariantSave(aBytes, &aSave, &aWhy))
	{
		Replay::VariantSave aBoard;
		GetVariantSave(&aBoard);
		MTRand aCheck(4357);
		int aShapes = mRules.ShapeCount();
		if (memcmp(aSave.mRulesId, aBoard.mRulesId, sizeof(aSave.mRulesId)) != 0)
			aWhy = "a save of other rules (a mod changed without a new version?)";
		else if ((aSave.mLevelShape < 0) || (aSave.mLevelShape >= aShapes))
			aWhy = "a save of level shape " + std::to_string(aSave.mLevelShape) + ", and the rules have " + std::to_string(aShapes);
		else
		{
			aLayout = mRules.LayoutForLevel(aSave.mLevelShape);
			// The v1 fields begin with mTimedMode (1 byte), then mLevel
			int32_t aLevel = 0;
			if (aSave.mBodyStart + 5 <= aBytes.size())
				memcpy(&aLevel, aBytes.data() + aSave.mBodyStart + 1, 4);
			// a dirt cell may have lost its last layer (stage 7b)
			if (!aLayout.FitsSaveMask(aSave.mCols, aSave.mRows, aSave.mPlayable, aSave.mHasDirt ? &aSave.mDirtLayers : NULL))
				aWhy = "a save of another board shape (" + std::to_string(aSave.mCols) + "x" + std::to_string(aSave.mRows) + ", level shape " +
					std::to_string(aSave.mLevelShape) + ")";
			else if ((aLevel < 0) || (aLevel % aShapes != aSave.mLevelShape))
				aWhy = "a save of level " + std::to_string(aLevel) + " in level shape " + std::to_string(aSave.mLevelShape) + ", which is not that level's";
			else if ((aSave.mRng.mSeed != 0) && !aCheck.SetState(aSave.mRng.mMt, aSave.mRng.mMti))
				aWhy = "an RNG state MTRand refuses";
		}
	}
	if (!aWhy.empty())
	{
		SDL_Log("savegame: %s not loaded: %s", mPlaybackOnly ? "the replay's save" : GetSaveGameFileName().c_str(), aWhy.c_str());
		return false;
	}

	// The board takes the shape of the save's level (stage 7); LoadGame then reads the cells within it
	SetLoadedLayout(aLayout, aSave.mLevelShape);
	if (aLayout.HasDirt())
	{
		// and the save's dirt layers (stage 7b); without a DIRT chunk, as its mask says: all of a cell's or none
		std::vector<uint8_t> aLayers = aSave.mDirtLayers;
		if (!aSave.mHasDirt)
		{
			for (int aRow = 0; aRow < aLayout.mRows; aRow++)
			{
				for (int aCol = 0; aCol < aLayout.mCols; aCol++)
					aLayers.push_back((aLayout.IsDirt(aCol, aRow) && !aSave.mPlayable[aRow * aLayout.mCols + aCol]) ? aLayout.mDirt[aRow][aCol] : 0);
			}
		}
		std::string aDirtWhy;
		ApplyDirtLayers(aLayers, &aDirtWhy); // FitsSaveMask let them through
	}

	// The buffer reads on at the v1 fields
	theBuffer.SeekFront();
	std::vector<uchar> aSkipped(aSave.mBodyStart);
	theBuffer.ReadBytes(aSkipped.data(), (int) aSkipped.size());
	return true;
}

// GameLoadedVariant, at the end of a LoadGame that ReadVariantHeader let through: the RNG the game goes on with
// (seed 0: the global stream, as a v1 save without a sidecar), the MOVE counts, the GRAV state, DIRT's toggle, the
// KIND chunk's kinds and the gold (GOLD, GTIM)
void Board::ReadVariantChunks(const Buffer& theBuffer)
{
	Replay::VariantSave aSave;
	if (!Replay::ReadVariantSave(BufferText(theBuffer), &aSave))
		return; // ReadVariantHeader read the same bytes

	const Replay::RngState& aState = aSave.mRng;
	MTRand* aRng = NULL;
	if (aState.mSeed != 0)
	{
		aRng = new MTRand(aState.mSeed);
		aRng->SetState(aState.mMt, aState.mMti);
	}
	delete mOwnRng;
	mOwnRng = aRng;
	mGameSeed = aState.mSeed;
	mUserSeeded = (aRng != NULL) && aState.mUserSeeded;
	mRanked = (aRng != NULL) && aState.mRanked;
#ifndef BEJ_NO_VARIANTS
	mVariant.mOwnRng = (mOwnRng != NULL);
#endif

	mMoveCount = aSave.mMoves;
	mInvalidMoves = aSave.mInvalidMoves;
	mUndosUsed = aSave.mUndosUsed;
	mGravityFlips = aSave.mGravityFlips;
	mGravityNoFlip = aSave.mGravityNoFlip;
	mDiagonalLeftFirst = aSave.mDiagonalLeftFirst;
	// the gems' kinds (stage 11); ReadVariantSave let a kind through only where the save has a gem
	if (aSave.mHasKinds && (aSave.mKinds.size() == (size_t)(mCols * mRows)))
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				if (mBoard[aRow][aCol] != NULL)
					mBoard[aRow][aCol]->mKind = aSave.mKinds[aRow * mCols + aCol];
			}
		}
	}
	// the gold (stage 12): LoadGame calls no SetupLevel (and RebuildLoadedBackground keeps it through its own), so it
	// comes only from here; without GTIM the level's whole time
	if (mVariant.mGold)
	{
		memset(mGold, 0, sizeof(mGold));
		mGoldShines.clear();
		if (aSave.mHasGold && (aSave.mGold.size() == (size_t)(mCols * mRows)))
		{
			for (int aRow = 0; aRow < mRows; aRow++)
			{
				for (int aCol = 0; aCol < mCols; aCol++)
					mGold[aRow][aCol] = aSave.mGold[aRow * mCols + aCol];
			}
		}
		mGoldTickLimit = aSave.mHasGoldTime ? aSave.mGoldTickLimit : GoldLimitTicks();
		mGoldTicksLeft = aSave.mHasGoldTime ? aSave.mGoldTicksLeft : mGoldTickLimit;
	}
	// the arrows (stage 18); ReadVariantSave let an arrow through only where the save has a gem
	if (mVariant.mArrows)
	{
		bool hasCells = aSave.mHasArrows && (aSave.mArrows.size() == (size_t)(mCols * mRows));
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				if (mBoard[aRow][aCol] != NULL)
					mBoard[aRow][aCol]->mArrow = hasCells ? aSave.mArrows[aRow * mCols + aCol] : 0;
			}
		}
		mArrowMoves = aSave.mArrowMoves;
		mArrowsCleared = aSave.mArrowsCleared;
		mArrowStepDue = aSave.mArrowStepDue;
		mArrowSteps.clear();
		mArrowTick = 0;
	}
}
