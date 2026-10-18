// port(variant): Board members for code mods (docs/CODE_MODS_DESIGN.md 2.3). Not part of the original game.
// Stage 1 (9.2): the rules snapshot, the layout and the variant flags. The layout is the original 8x8
// whatever the rules say, and no translated function reads the flags yet, so every board plays as the
// original; later stages switch the generalised paths on.
// Stage 2: the board's RNG (6.2). A seed the player chose, any rule, or the seeds.perGame feature gives
// the board its own MTRand, which Board::Rand draws from; every other board draws from the global stream
// as the original does.

#include "Board.h"
#include "GameConfig.h"

using namespace Sexy;

// Called first in the constructor (0x4cb5bc), before SetupLevel and FillBoard use the layout and the RNG
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

	// The RNG source WinDMApp::NewGame chose (WinDMApp::ChooseBoardRng), taken once: a board made any other
	// way draws from the global stream
	mOwnRng = NULL;
	mGameSeed = gGameConfig.mNextBoardSeed;
	mUserSeeded = (mGameSeed != 0) && gGameConfig.mNextBoardUserSeeded;
	mRanked = false;
	mBoardDraws = 0;
	mTakesPendingSeed = mUserSeeded && (mGameSeed == gGameConfig.mPendingSeed);
	gGameConfig.mNextBoardSeed = 0;
	gGameConfig.mNextBoardUserSeeded = false;
	if (mGameSeed != 0)
		mOwnRng = new MTRand(mGameSeed);
#ifndef BEJ_NO_VARIANTS
	// ComputeVariantFlags says whether the rules need an own RNG; NewGame drew its seed (or took the
	// player's), and -forcevariant leaves the source alone
	mVariant.mOwnRng = (mOwnRng != NULL);
#endif
}

// A seed the player chose stays pending until a game on it is played (docs/CODE_MODS_DESIGN.md 4.10, the
// user's decision of 2026-10-18): the board that got it uses it up with its first swap that makes a match
// (UpdateSwap, 0x4cd84f, where mIsNewGame is cleared) or its game over (GameOver, 0x4c9b47; not the one
// RequestNewGame starts, which clears mTakesPendingSeed first). Until then every board that replaces it
// gets the seed again: the second of the double NewGame in TitleScreenIsFinished, a mode switch, New Game.
// A board that resumed a save (GameLoadedVariant) never uses it up.
void Board::UsePendingSeed()
{
	if (!mTakesPendingSeed)
		return;
	mTakesPendingSeed = false;
	if (gGameConfig.mPendingSeed == mGameSeed)
		gGameConfig.mPendingSeed = 0;
}

// The end of a LoadGame (0x4cca98) that succeeded. The resumed game is not the one the board's seed would
// make, and until saves keep the RNG state (the default-rules sidecar of stage 3, the v2 RNGS chunk of
// stage 5; docs/CODE_MODS_DESIGN.md 7.2, 7.3) the v1 save has nothing to restore. So for now the game
// goes on unseeded: it draws from the global stream, its scores go to the normal table, and the game-over
// screen shows no SEED line. The v1 save is written as before. The pending seed stays for the next game.
void Board::GameLoadedVariant()
{
	mTakesPendingSeed = false;
	delete mOwnRng;
	mOwnRng = NULL;
	mGameSeed = 0;
	mUserSeeded = false;
#ifndef BEJ_NO_VARIANTS
	mVariant.mOwnRng = false;
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
	// The rules and features that give a board its own RNG even without a seed the player chose
	// (WinDMApp::ChooseBoardRng)
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
