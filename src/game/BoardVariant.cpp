// port(variant): Board members for code mods (docs/CODE_MODS_DESIGN.md 2.3). Not part of the original game.
// Stage 1 (9.2): the rules snapshot, the layout and the variant flags. The layout is the original 8x8
// whatever the rules say, and no translated function reads the flags yet, so every board plays as the
// original; later stages switch the generalised paths on.

#include "Board.h"
#include "GameConfig.h"

using namespace Sexy;

// Called first in the constructor (0x4cb5bc), before SetupLevel and FillBoard use the layout
void Board::InitVariant()
{
	mRules = gGameConfig.mRules;
	// The board shape rules (board.cols/rows/mask/levels) take effect in stage 7 (FillBoardVariant,
	// RefillMasked, DrawVariant); until then the layout keeps the original constants
	mLayout = BoardLayout();
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;

#ifndef BEJ_NO_VARIANTS
	mVariant = ComputeVariantFlags(mRules, gGameConfig.mFeatures);
	if (gGameConfig.mForceVariant)
	{
		// -forcevariant: every generalised path at default rules; the RNG source, the save format and the
		// tutorial stay original (mAny, mOwnRng and mNoTutorial are left alone)
		mVariant.mShape = true;
		mVariant.mMatch = true;
		mVariant.mSearch = true;
		mVariant.mDraw = true;
	}
#endif
}

VariantFlags Board::ComputeVariantFlags(const GameRules& theRules, const GameFeatures& theFeatures)
{
	VariantFlags aFlags;
	const GameRules::MatchRules& aMatch = theRules.mMatch;
	bool hasPatterns = !aMatch.mShapes.empty() || aMatch.mDiagonal || aMatch.mMergeCrossings;
	bool hasSpecials = theRules.mSpecials.mFlame.mEnabled || theRules.mSpecials.mLightning.mEnabled;
	bool hasNonSwapMoves = theRules.mMoves.mTypes != GameRules::MOVE_SWAP;

	aFlags.mAny = theRules.HasVariantRules();
	// An explicit seed (stage 2) also gives the board its own RNG
	aFlags.mOwnRng = aFlags.mAny || theFeatures.mSeedPerGame || theFeatures.mLeaderboard;

	BoardLayout aFirst = theRules.LayoutForLevel(0);
	aFlags.mShape = (theRules.ShapeCount() > 1) || !aFirst.IsOriginal();
	for (int aLevel = 0; aLevel < theRules.ShapeCount(); aLevel++)
		aFlags.mScaled = aFlags.mScaled || (theRules.LayoutForLevel(aLevel).mExtent > BoardLayout::VIEW_SIZE);

	// The tutorial looks for a horizontal 3-run swap to the right in rows 4..6, columns 1..6
	bool canTutorial = ((theRules.mMoves.mTypes & GameRules::MOVE_SWAP) != 0) && (aMatch.mMinRun == 3) && !aMatch.mDiagonal &&
		(aFirst.mCols >= 8) && (aFirst.mRows >= 7);
	for (int aRow = 4; (aRow <= 6) && canTutorial; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
			canTutorial = canTutorial && aFirst.IsPlayable(aCol, aRow);
	}
	aFlags.mNoTutorial = !canTutorial;

	aFlags.mSpecials = hasSpecials;
	aFlags.mGold = theRules.mGold.mEnabled;
	aFlags.mUndo = theRules.mUndo.mEnabled;
	aFlags.mMatch = hasPatterns || (aMatch.mMinRun != 3) || hasSpecials || aFlags.mGold;
	aFlags.mSearch = hasPatterns || (aMatch.mMinRun != 3) || hasNonSwapMoves;
	aFlags.mMoves = (theRules.mMoves.mTypes & (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE)) != 0;
	aFlags.mInvalid = theRules.mMoves.mAllowInvalid;
	aFlags.mNoMoves = (theRules.mNoMoves.mNormal != GameRules::NOMOVES_GAME_OVER) || (theRules.mNoMoves.mTimed != GameRules::NOMOVES_FALL_OUT);
	aFlags.mDraw = aFlags.mShape || hasSpecials || aFlags.mGold || hasNonSwapMoves || aFlags.mNoTutorial;
	return aFlags;
}
