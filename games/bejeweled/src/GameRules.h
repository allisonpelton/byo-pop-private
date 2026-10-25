#ifndef __GAMERULES_H__
#define __GAMERULES_H__

// port(variant): the gameplay rules a set of mods asks for (games/bejeweled/docs/CODE_MODS_DESIGN.md
// section 3). Not part of the original game. Framework-free (Json and the header-only BoardLayout only), so
// the game, the rules tests (games/bejeweled/tools/rulestest) and later the headless verifier share it.
//
// mod.json gains "rules" (gameplay; a superset of "tunables", 3.2), "features" (replays, seeds,
// leaderboard: no gameplay effect) and "imageOps" (generated art; ModManager keeps those). MergeRules()
// combines the enabled mods' "tunables" and "rules" (3.5):
//   - leaves: the highest-priority mod wins; arrays are replaced whole;
//   - atomic groups (shape: board.cols/rows/mask/levels; moves: moves.type/twist.*/slide.*; gold.*;
//     undo.*): the highest-priority mod that sets any key of a group supplies the whole group, keys it
//     leaves out take their defaults;
//   - keyed objects (match.shapes.<id>, match.bonus.<id>; specials.flame / specials.lightning are
//     leaves holding objects): merged per id;
//   - null resets a key, an id, a keyed object or a whole namespace ("gold": null) to its default;
//   - moves.addTypes is a union operator added to the type the moves group resolved to;
//   - provenance: key -> the mods that set it (several only for the moves.addTypes union), and a warning
//     for each overridden value ("a: moves.type overrides b");
//   - validation (3.6): a mod failing the type and range checks or the shape group's checks has its
//     "rules" ignored (an error; its "tunables" still apply); a merged set failing a cross-field check
//     (a line of minRun cells, twist/slide room, the colour minimum, createdBy, noMoves "continue") drops
//     the lowest-priority mod that supplied a cause of the failure (Validate: the values the check reads
//     that differ from their defaults; the board shape only where the original 8x8 board would pass) and
//     merges again, so a lone mod that fails one is dropped, and a mod may rely on another (T and L
//     triggers on a "crossings": "merge").
// Inside "tunables" the legacy keys (scoring.*, timing.*, board.gemColors) keep their old lenient
// reading (a wrong type means the default, numbers are truncated, outside the int range they read as
// INT_MIN like the native game's (int) cast, gemColors is clamped), so tunables-only mods behave exactly as
// before in the native game; inside "rules" every key is type and range checked.
//
// Each rule has one spelling, so equivalent rules share a RulesId (3.7): an all-'#' mask is no mask,
// board.levels that repeat a shorter cycle are that cycle and a single level is board.cols/rows/mask, a
// cell pattern is a preset when one has its orientations (else its smallest orientation with the fewest
// flags), createdBy is a sorted set, run names have no leading zeros, a bonus of 0 is no bonus.
//
// Canonical() is the binary canonical form (3.7): u16 rulesVersion; then, in schema order, every field
// whose value differs from its default as (u16 fieldId, u8 kind, value) with ints as little-endian i32,
// doubles as their 8 IEEE-754 bytes (-0 as 0, any NaN as the quiet NaN), bools as u8, strings and masks
// length-prefixed (u16), keyed objects in sorted key order; a u16 0 ends the fields; finally the 12
// data\diff.txt values as loaded, always. RulesId = SHA-256 of Canonical(). No text formatting of numbers
// takes part, so native and wasm agree.

#include "BoardLayout.h"
#include "platform/Json.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

// One board shape: board.cols/rows/mask, or one entry of board.levels. An empty mask is a full board.
struct LevelShape
{
	int								mCols = 8;
	int								mRows = 8;
	std::vector<std::string>		mMask;
};

// gold.timeLimit.normal / .timed: null (untimed) or {base, perLevel, min} in seconds
struct TimeLimit
{
	bool							mEnabled = false;
	int								mBase = 180;
	int								mPerLevel = -10;
	int								mMin = 60;
};

// match.shapes.<id>: {"preset": "square" | "L3" | "T4"} or {"cells": [[x,y],...], "rotations", "mirror"}, in
// the one spelling of its orientations (a preset when one matches them; see the top of this file)
struct ShapePattern
{
	std::string						mPreset;        // empty for a cell pattern
	std::vector<std::pair<int, int> > mCells;        // translated to (0,0), sorted
	bool							mRotations = false;
	bool							mMirror = false;
};

// specials.flame / specials.lightning: null or {"createdBy": [patterns], "radius" (flame), "points"}
struct SpecialRule
{
	bool							mEnabled = false;
	std::vector<std::string>		mCreatedBy;     // sorted, without duplicates
	int								mRadius = 1;
	int								mPoints = 0;
};

// data\diff.txt in the order WinDMApp::LoadingThreadProc reads it (WinDMApp.h)
struct DiffValues
{
	int								mNormalPointsBase = 0;
	double							mNormalPointsPerLevel = 0.0;
	double							mNormalMultBase = 0.0;
	double							mNormalMultPerLevel = 0.0;
	int								mTimedPointsBase = 0;
	double							mTimedPointsPerLevel = 0.0;
	double							mTimedMultBase = 0.0;
	double							mTimedMultPerLevel = 0.0;
	double							mTimeDrainBase = 0.0;
	double							mTimeDrainPerLevel = 0.0;
	double							mTimeDrainAccelBase = 0.0;
	double							mTimeDrainAccelPerLevel = 0.0;
};

class GameRules
{
public:
	enum
	{
		RULES_VERSION = 1,
		MIN_GEM_COLORS = 3,
		MAX_GEM_COLORS = 12,
		MAX_LEVELS = 64,        // board.levels
		MAX_SHAPES = 32         // match.shapes: Validate's fill minimum grows with shapes x levels
	};

	enum MoveType
	{
		MOVE_SWAP = 1,
		MOVE_TWIST = 2,
		MOVE_SLIDE = 4
	};

	enum NoMovesRule
	{
		NOMOVES_GAME_OVER = 0,
		NOMOVES_FALL_OUT = 1,
		NOMOVES_CONTINUE = 2
	};

	enum GravityDirection
	{
		GRAVITY_DOWN = 0,
		GRAVITY_UP = 1,
		GRAVITY_ALTERNATE = 2   // Bejeweled 2's Twilight (4.16): starts up, flips at each settle that ends a move
	};

	enum Backdrop
	{
		BACKDROP_NORMAL = 0,
		BACKDROP_TWILIGHT = 1   // the checker tiles mirrored left to right, each colour channel halved
	};

	enum
	{
		DEFAULT_FALL_ACCEL = 180    // board.gravity.fallAccel: 180 / 1000.0 is the original's 0.18
	};

	struct BoardRules
	{
		int							mCols = 8;              // board.cols 3..16 (shape)
		int							mRows = 8;              // board.rows 3..16 (shape)
		std::vector<std::string>	mMask;                  // board.mask (shape); empty = full
		std::vector<LevelShape>		mLevels;                // board.levels (shape); level L uses [L % n]
		int							mGemColors = 7;         // board.gemColors 3..12 (also a tunable)
		int							mDirtPoints = 0;        // board.dirtPoints: points per dirt layer removed
		int							mGravity = GRAVITY_DOWN; // board.gravity.direction (gravity)
		int							mFallAccel = DEFAULT_FALL_ACCEL; // board.gravity.fallAccel 50..1000 (gravity)
	};

	struct MoveRules
	{
		int							mTypes = MOVE_SWAP;     // moves.type (+ moves.addTypes) (moves)
		bool						mTwistBoth = false;     // moves.twist.direction "cw" / "both" (moves)
		int							mSlideSnapTicks = 12;   // moves.slide.snapTicks 4..60 (moves)
		bool						mAllowInvalid = false;  // moves.allowInvalid
		int							mInvalidPenalty = 0;    // moves.invalidPenalty
		bool						mInvalidCountsAsMove = true; // moves.invalidCountsAsMove
	};

	struct NoMovesRules
	{
		int							mNormal = NOMOVES_GAME_OVER; // noMoves.normal (default fallOut with gold)
		int							mTimed = NOMOVES_FALL_OUT;   // noMoves.timed
	};

	struct MatchRules
	{
		int							mMinRun = 3;            // match.minRun 2..5
		bool						mDiagonal = false;      // match.diagonal
		int							mDiagonalMinRun = 3;    // match.diagonalMinRun (default = minRun)
		bool						mMergeCrossings = false; // match.crossings "separate" / "merge"
		std::map<std::string, ShapePattern> mShapes;        // match.shapes.<id>
		std::map<std::string, int>	mBonus;                 // match.bonus.<pattern> (0 entries dropped)
	};

	struct SpecialRules
	{
		SpecialRule					mFlame;                 // specials.flame
		SpecialRule					mLightning;             // specials.lightning
		bool						mChain = true;          // specials.chain
		bool						mCreateInMoveOnly = false; // specials.createIn "any" / "move"
	};

	struct GoldRules
	{
		bool						mEnabled = false;       // gold.enabled (gold)
		bool						mGildBlasts = true;     // gold.gild "matches" / "matchesAndBlasts"
		TimeLimit					mTimeLimitNormal;       // gold.timeLimit.normal: null
		TimeLimit					mTimeLimitTimed = TimeLimit{true, 180, -10, 60}; // gold.timeLimit.timed
		int							mTimeBonus = 10;        // gold.timeBonus
		bool						mCountDirt = true;      // gold.countDirt (4.14)
	};

	struct UndoRules
	{
		bool						mEnabled = false;       // undo.enabled (undo)
		int							mDepth = 1;             // undo.depth 1..100
		int							mMax = -1;              // undo.max per game, -1 unlimited
		int							mPenalty = 0;           // undo.penalty
		bool						mInTimed = false;       // undo.inTimed
		bool						mRestoreTime = false;   // undo.restoreTime
		bool						mRestoreRng = true;     // undo.restoreRng
	};

	// Presentation (4.16): in the RulesId and the score key, never a variant board
	struct LookRules
	{
		int							mBackdrop = BACKDROP_NORMAL; // look.backdrop
		int							mSoundPitch = 0;        // look.soundPitch -12..12 semitones
	};

	// The legacy tunables (GameConfig.h)
	struct TunableRules
	{
		std::vector<int>			mSetValues = {10, 20, 30, 40, 60, 80, 110, 160, 210}; // scoring.setValues
		int							mLengthBonus = 10;      // scoring.lengthBonus
		int							mLongLengthBonus = 10;  // scoring.longLengthBonus
		double						mExcellentFactor = 120.0; // scoring.excellentFactor
		double						mHintPenalty = 50.0;    // scoring.hintPenalty
		int							mStartDrainDelay = 300; // timing.startDrainDelay
		int							mLevelDrainDelay = 200; // timing.levelDrainDelay
		double						mWarningFraction = 0.15; // timing.warningFraction
	};

	BoardRules						mBoard;
	MoveRules						mMoves;
	NoMovesRules					mNoMoves;
	MatchRules						mMatch;
	SpecialRules					mSpecials;
	GoldRules						mGold;
	UndoRules						mUndo;
	LookRules						mLook;
	TunableRules					mTunables;
	DiffValues						mDiff;          // always part of the canonical form

public:
	// Binary canonical form and its SHA-256 (see the top of this file)
	std::string						Canonical() const;
	bool							FromCanonical(const std::string& theBytes, std::string* theError = NULL);
	void							RulesId(uint8_t theId[32]) const;
	std::string						RulesIdHex() const;

	// The canonical form as text: "key = value" per non-default field, then the diff.txt values
	std::vector<std::string>		CanonicalLines() const;

	// Any rule beyond the legacy tunables differs from its default (a variant board, 1.2 principle 4)
	bool							HasVariantRules() const;
	// A scoring tunable (setValues, lengthBonus, longLengthBonus, excellentFactor, hintPenalty) differs from the
	// original's: the board's displayed score then closes large gaps fast (Board::RollScoreFast, stage 7)
	bool							ScoringTunablesChanged() const;
	// Replays of these rules play alike in every engine from Replay::FIRST_ENGINE_ABI on: no variant rule and no
	// scoring tunable changed (engine 3 changed how their displayed score rolls)
	bool							PlaysAlikeInEveryEngine() const;
	std::vector<std::string>		VariantKeys() const;   // those keys, in schema order
	std::vector<std::string>		LookKeys() const;      // the look.* keys that differ from their defaults
	// A board.gravity rule differs from its default: the board then keeps its gravity state in saves, hashes and
	// traces (4.16; default rules, -forcevariant included, keep the frozen forms)
	bool							HasGravityRule() const { return (mBoard.mGravity != GRAVITY_DOWN) || (mBoard.mFallAccel != DEFAULT_FALL_ACCEL); }

	// The checks of 3.6 (all of them, for the merged set). False with a message and the causes of the
	// failure: the fields the failing check reads whose values differ from their defaults (the board shape
	// only where the check would pass on the original 8x8 board), and "moves.type=<type>" for the mods that
	// put that move type into moves.type
	bool							Validate(std::string* theMessage, std::vector<std::string>* theKeys) const;
	// Only the shape group's checks (sizes, masks, at least 9 cells): what one mod is checked for alone
	bool							ValidateShape(std::string* theMessage) const;

	// Rules decoded from someone else's file (a replay's RULE, FromCanonical): whether they are rules the game
	// makes, exactly what MergeRules makes of a mod.json that says them (every field in the range the mod path
	// takes and normalised as it normalises, every check of Validate passed, so the canonical bytes come out
	// alike), with a finite tunable in every double, and 12 diff.txt values every level of the original
	// SetupLevel plays with: finite, points and multiplier bases above 0 whose product is at least 1, per-level
	// steps not below 0, so mLevelPoints, Board::Update's divisor, is at least 1 on every level (or past the int
	// range). False with a message. (The game's own diff.txt is read as the original reads it.)
	bool							CheckRecorded(std::string* theError) const;

	// The fewest gem colours the fill can always complete with (3.6): 1 + the most pattern instances a
	// cell can complete in fill order, over every level's layout and every fill order gravity uses (bottom-up
	// for down, top-down for up)
	int								FillMinimum() const;

	// Board shapes: level 0 is board.cols/rows/mask, or board.levels[L % n]
	int								ShapeCount() const;
	LevelShape						ShapeForLevel(int theLevel) const;
	BoardLayout						LayoutForLevel(int theLevel) const;

	int								NoMovesFor(bool isTimed) const { return isTimed ? mNoMoves.mTimed : mNoMoves.mNormal; }
	static std::string				MoveTypesText(int theTypes);   // "swap+twist"
};

// Port features (3.1): no gameplay effect, not part of the RulesId or the score key
struct GameFeatures
{
	bool							mSeeds = false;         // true: both below; an object: the members it sets to true
	bool							mSeedEntry = false;     // seed field in Extras
	bool							mSeedPerGame = false;   // every game gets a shown, replayable seed
	bool							mReplays = false;
	int								mReplayKeep = 20;
	bool							mReplayHover = false;
	bool							mLeaderboard = false;   // implies replays and seeds.perGame
	bool							mRankedAlways = false;  // leaderboard.ranked "always" (else "ask")
	bool							mLiveSplit = false;     // fill the LiveSplit block (desktop builds, 4.15)
	std::string						mLiveSplitMod;          // the mod that turned it on (its livesplit/ templates)

	bool							Any() const { return mSeeds || mReplays || mLeaderboard || mLiveSplit; }
	std::string						Text() const;           // "seeds(entry, perGame), replays(keep 20), livesplit" or "none"
};

// One enabled mod's gameplay keys, as ModManager read them from its mod.json
struct RulesSource
{
	std::string						mModId;
	Platform::JsonValue				mTunables;      // "tunables" (an object, or null)
	Platform::JsonValue				mRules;         // "rules" (an object, or null)
	std::string						mRulesError;    // "rules" named a file that could not be read: why
	Platform::JsonValue				mFeatures;      // "features" (an object, or null)
};

struct RulesMessage
{
	std::string						mModId;
	std::string						mText;
};

struct RulesStatus
{
	std::vector<RulesMessage>		mWarnings;      // overridden values, unknown keys, clamped values
	std::vector<RulesMessage>		mErrors;        // a mod's "rules" (or "features") ignored
	std::vector<RulesMessage>		mDropped;       // a mod dropped from the merged set, with the failed check
	// key -> the mods that set it ("features.<name>" too); several only for moves.addTypes, lowest priority first
	std::map<std::string, std::vector<std::string> > mProvenance;
};

// theSources lowest priority first (the order ModManager::Apply merges data in)
void MergeRules(const std::vector<RulesSource>& theSources, GameRules* theRules, RulesStatus* theStatus);
void MergeFeatures(const std::vector<RulesSource>& theSources, GameFeatures* theFeatures, RulesStatus* theStatus);

}

#endif
