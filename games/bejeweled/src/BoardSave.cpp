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
			std::vector<bool> aPlayable;
			for (int aRow = 0; aRow < aLayout.mRows; aRow++)
			{
				for (int aCol = 0; aCol < aLayout.mCols; aCol++)
					aPlayable.push_back(aLayout.IsPlayable(aCol, aRow));
			}
			// The v1 fields begin with mTimedMode (1 byte), then mLevel
			int32_t aLevel = 0;
			if (aSave.mBodyStart + 5 <= aBytes.size())
				memcpy(&aLevel, aBytes.data() + aSave.mBodyStart + 1, 4);
			if ((aSave.mCols != aLayout.mCols) || (aSave.mRows != aLayout.mRows) || (aSave.mPlayable != aPlayable))
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

	// The buffer reads on at the v1 fields
	theBuffer.SeekFront();
	std::vector<uchar> aSkipped(aSave.mBodyStart);
	theBuffer.ReadBytes(aSkipped.data(), (int) aSkipped.size());
	return true;
}

// GameLoadedVariant, at the end of a LoadGame that ReadVariantHeader let through: the RNG the game goes on with
// (seed 0: the global stream, as a v1 save without a sidecar) and the MOVE counts
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
}
