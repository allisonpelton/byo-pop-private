// port(variant): GameRules, the mod rules schema, merge, validation and canonical form (see GameRules.h and
// games/bejeweled/docs/CODE_MODS_DESIGN.md section 3). Not part of the original game.

#include "GameRules.h"
#include "Sha256.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <set>

using namespace Sexy;
using Platform::JsonValue;

namespace
{

// ---------------------------------------------------------------------------------------------------
// The schema table

// Also the type byte of a field in the canonical form
enum FieldKind
{
	KIND_INT = 1,
	KIND_BOOL = 2,
	KIND_DOUBLE = 3,
	KIND_ENUM = 4,          // stored as an index into mNames, encoded as the name
	KIND_INT_LIST = 5,
	KIND_MASK = 6,
	KIND_LEVELS = 7,
	KIND_TIME_LIMIT = 8,
	KIND_SHAPES = 9,        // keyed: merged per id
	KIND_BONUS = 10,        // keyed: merged per id
	KIND_SPECIAL = 11,
	KIND_MOVE_TYPES = 12    // a set of bits, one per name of mNames: moves.type, arrows.directions
};

// Atomic groups (3.5): the highest-priority mod that sets any key of a group supplies all of it
enum FieldGroup
{
	GROUP_NONE = 0,
	GROUP_SHAPE,
	GROUP_MOVES,
	GROUP_GOLD,
	GROUP_UNDO,
	GROUP_GRAVITY,
	GROUP_ARROWS,
	NUM_GROUPS
};

enum FieldFlags
{
	FLAG_TUNABLE = 1,       // a legacy tunable: read leniently under "tunables", not a variant rule
	FLAG_CLAMP = 2,         // out of range: clamped with a warning instead of rejected
	FLAG_BOOL = 4,          // KIND_ENUM with two names, stored as a bool (index 1 = true)
	FLAG_LOOK = 8           // presentation (look.*): in the RulesId, not a variant rule
};

struct FieldDef
{
	uint16_t		mId;            // canonical field id: never reused or renumbered
	const char*		mKey;
	int				mKind;
	int				mGroup;
	int				mFlags;
	int				mMin;           // KIND_INT, KIND_BONUS values
	int				mMax;
	const char* const* mNames;      // KIND_ENUM
	int				mNameCount;
	void*			(*mPtr)(GameRules*);
};

const char* const gTwistNames[] = {"cw", "both"};
const char* const gNoMovesNames[] = {"gameOver", "fallOut", "continue"};
const char* const gCrossingNames[] = {"separate", "merge"};
const char* const gCreateInNames[] = {"any", "move"};
const char* const gGildNames[] = {"matches", "matchesAndBlasts", "blasts"};
const char* const gMoveTypeNames[] = {"swap", "twist", "slide"};
const char* const gGravityNames[] = {"down", "up", "alternate"};
const char* const gBackdropNames[] = {"normal", "twilight"};
const char* const gArrowNames[] = {"up", "right", "down", "left", "upRight", "downRight", "downLeft", "upLeft"}; // GameRules::ArrowDirection
const char* const gArrowEdgeNames[] = {"top", "right", "bottom", "left"}; // GameRules::ArrowEdge

#define RULE_FIELD(theMember) [](GameRules* r) -> void* { return &r->theMember; }
#define RULE_NAMES(theNames) theNames, (int) (sizeof(theNames) / sizeof(theNames[0]))

// In canonical order. A field added later gets a new id and a default, so existing RulesIds stay.
const FieldDef gFields[] =
{
	{1, "board.cols", KIND_INT, GROUP_SHAPE, 0, 3, 16, NULL, 0, RULE_FIELD(mBoard.mCols)},
	{2, "board.rows", KIND_INT, GROUP_SHAPE, 0, 3, 16, NULL, 0, RULE_FIELD(mBoard.mRows)},
	{3, "board.mask", KIND_MASK, GROUP_SHAPE, 0, 0, 0, NULL, 0, RULE_FIELD(mBoard.mMask)},
	{4, "board.levels", KIND_LEVELS, GROUP_SHAPE, 0, 0, 0, NULL, 0, RULE_FIELD(mBoard.mLevels)},
	{5, "board.gemColors", KIND_INT, GROUP_NONE, FLAG_TUNABLE, GameRules::MIN_GEM_COLORS, GameRules::MAX_GEM_COLORS, NULL, 0, RULE_FIELD(mBoard.mGemColors)},
	{6, "board.dirtPoints", KIND_INT, GROUP_NONE, 0, 0, 1000000, NULL, 0, RULE_FIELD(mBoard.mDirtPoints)},
	{7, "board.gravity.direction", KIND_ENUM, GROUP_GRAVITY, 0, 0, 0, RULE_NAMES(gGravityNames), RULE_FIELD(mBoard.mGravity)},
	{8, "board.gravity.fallAccel", KIND_INT, GROUP_GRAVITY, FLAG_CLAMP, 50, 1000, NULL, 0, RULE_FIELD(mBoard.mFallAccel)},
	{16, "moves.type", KIND_MOVE_TYPES, GROUP_MOVES, 0, 0, 0, RULE_NAMES(gMoveTypeNames), RULE_FIELD(mMoves.mTypes)},
	{17, "moves.twist.direction", KIND_ENUM, GROUP_MOVES, FLAG_BOOL, 0, 0, RULE_NAMES(gTwistNames), RULE_FIELD(mMoves.mTwistBoth)},
	{18, "moves.slide.snapTicks", KIND_INT, GROUP_MOVES, FLAG_CLAMP, 4, 60, NULL, 0, RULE_FIELD(mMoves.mSlideSnapTicks)},
	{19, "moves.allowInvalid", KIND_BOOL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mMoves.mAllowInvalid)},
	{20, "moves.invalidPenalty", KIND_INT, GROUP_NONE, 0, 0, 1000000, NULL, 0, RULE_FIELD(mMoves.mInvalidPenalty)},
	{21, "moves.invalidCountsAsMove", KIND_BOOL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mMoves.mInvalidCountsAsMove)},
	{32, "noMoves.normal", KIND_ENUM, GROUP_NONE, 0, 0, 0, RULE_NAMES(gNoMovesNames), RULE_FIELD(mNoMoves.mNormal)},
	{33, "noMoves.timed", KIND_ENUM, GROUP_NONE, 0, 0, 0, RULE_NAMES(gNoMovesNames), RULE_FIELD(mNoMoves.mTimed)},
	{48, "match.minRun", KIND_INT, GROUP_NONE, 0, 2, 5, NULL, 0, RULE_FIELD(mMatch.mMinRun)},
	{49, "match.diagonal", KIND_BOOL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mMatch.mDiagonal)},
	{50, "match.diagonalMinRun", KIND_INT, GROUP_NONE, 0, 2, 5, NULL, 0, RULE_FIELD(mMatch.mDiagonalMinRun)},
	{51, "match.crossings", KIND_ENUM, GROUP_NONE, FLAG_BOOL, 0, 0, RULE_NAMES(gCrossingNames), RULE_FIELD(mMatch.mMergeCrossings)},
	{52, "match.shapes", KIND_SHAPES, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mMatch.mShapes)},
	{53, "match.bonus", KIND_BONUS, GROUP_NONE, 0, -100000, 100000, NULL, 0, RULE_FIELD(mMatch.mBonus)},
	{64, "specials.flame", KIND_SPECIAL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mSpecials.mFlame)},
	{65, "specials.lightning", KIND_SPECIAL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mSpecials.mLightning)},
	{66, "specials.chain", KIND_BOOL, GROUP_NONE, 0, 0, 0, NULL, 0, RULE_FIELD(mSpecials.mChain)},
	{67, "specials.createIn", KIND_ENUM, GROUP_NONE, FLAG_BOOL, 0, 0, RULE_NAMES(gCreateInNames), RULE_FIELD(mSpecials.mCreateInMoveOnly)},
	{80, "gold.enabled", KIND_BOOL, GROUP_GOLD, 0, 0, 0, NULL, 0, RULE_FIELD(mGold.mEnabled)},
	{81, "gold.gild", KIND_ENUM, GROUP_GOLD, 0, 0, 0, RULE_NAMES(gGildNames), RULE_FIELD(mGold.mGild)},
	{82, "gold.timeLimit.normal", KIND_TIME_LIMIT, GROUP_GOLD, 0, 0, 0, NULL, 0, RULE_FIELD(mGold.mTimeLimitNormal)},
	{83, "gold.timeLimit.timed", KIND_TIME_LIMIT, GROUP_GOLD, 0, 0, 0, NULL, 0, RULE_FIELD(mGold.mTimeLimitTimed)},
	{84, "gold.timeBonus", KIND_INT, GROUP_GOLD, 0, 0, 1000000, NULL, 0, RULE_FIELD(mGold.mTimeBonus)},
	{85, "gold.countDirt", KIND_BOOL, GROUP_GOLD, 0, 0, 0, NULL, 0, RULE_FIELD(mGold.mCountDirt)},
	{96, "undo.enabled", KIND_BOOL, GROUP_UNDO, 0, 0, 0, NULL, 0, RULE_FIELD(mUndo.mEnabled)},
	{97, "undo.depth", KIND_INT, GROUP_UNDO, FLAG_CLAMP, 1, 100, NULL, 0, RULE_FIELD(mUndo.mDepth)},
	{98, "undo.max", KIND_INT, GROUP_UNDO, 0, -1, 1000000, NULL, 0, RULE_FIELD(mUndo.mMax)},
	{99, "undo.penalty", KIND_INT, GROUP_UNDO, 0, 0, 1000000, NULL, 0, RULE_FIELD(mUndo.mPenalty)},
	{100, "undo.inTimed", KIND_BOOL, GROUP_UNDO, 0, 0, 0, NULL, 0, RULE_FIELD(mUndo.mInTimed)},
	{101, "undo.restoreTime", KIND_BOOL, GROUP_UNDO, 0, 0, 0, NULL, 0, RULE_FIELD(mUndo.mRestoreTime)},
	{102, "undo.restoreRng", KIND_BOOL, GROUP_UNDO, 0, 0, 0, NULL, 0, RULE_FIELD(mUndo.mRestoreRng)},
	{112, "scoring.setValues", KIND_INT_LIST, GROUP_NONE, FLAG_TUNABLE, INT_MIN, INT_MAX, NULL, 0, RULE_FIELD(mTunables.mSetValues)},
	{113, "scoring.lengthBonus", KIND_INT, GROUP_NONE, FLAG_TUNABLE, INT_MIN, INT_MAX, NULL, 0, RULE_FIELD(mTunables.mLengthBonus)},
	{114, "scoring.longLengthBonus", KIND_INT, GROUP_NONE, FLAG_TUNABLE, INT_MIN, INT_MAX, NULL, 0, RULE_FIELD(mTunables.mLongLengthBonus)},
	{115, "scoring.excellentFactor", KIND_DOUBLE, GROUP_NONE, FLAG_TUNABLE, 0, 0, NULL, 0, RULE_FIELD(mTunables.mExcellentFactor)},
	{116, "scoring.hintPenalty", KIND_DOUBLE, GROUP_NONE, FLAG_TUNABLE, 0, 0, NULL, 0, RULE_FIELD(mTunables.mHintPenalty)},
	{128, "timing.startDrainDelay", KIND_INT, GROUP_NONE, FLAG_TUNABLE, INT_MIN, INT_MAX, NULL, 0, RULE_FIELD(mTunables.mStartDrainDelay)},
	{129, "timing.levelDrainDelay", KIND_INT, GROUP_NONE, FLAG_TUNABLE, INT_MIN, INT_MAX, NULL, 0, RULE_FIELD(mTunables.mLevelDrainDelay)},
	{130, "timing.warningFraction", KIND_DOUBLE, GROUP_NONE, FLAG_TUNABLE, 0, 0, NULL, 0, RULE_FIELD(mTunables.mWarningFraction)},
	{144, "look.backdrop", KIND_ENUM, GROUP_NONE, FLAG_LOOK, 0, 0, RULE_NAMES(gBackdropNames), RULE_FIELD(mLook.mBackdrop)},
	{145, "look.soundPitch", KIND_INT, GROUP_NONE, FLAG_LOOK | FLAG_CLAMP, -12, 12, NULL, 0, RULE_FIELD(mLook.mSoundPitch)},
	{160, "arrows.enabled", KIND_BOOL, GROUP_ARROWS, 0, 0, 0, NULL, 0, RULE_FIELD(mArrows.mEnabled)},
	{161, "arrows.directions", KIND_MOVE_TYPES, GROUP_ARROWS, 0, 0, 0, RULE_NAMES(gArrowNames), RULE_FIELD(mArrows.mDirections)},
	{162, "arrows.every", KIND_INT, GROUP_ARROWS, FLAG_CLAMP, 1, 100, NULL, 0, RULE_FIELD(mArrows.mEvery)},
	{163, "arrows.everyMin", KIND_INT, GROUP_ARROWS, FLAG_CLAMP, 1, 100, NULL, 0, RULE_FIELD(mArrows.mEveryMin)},
	{164, "arrows.bonus", KIND_INT, GROUP_ARROWS, 0, 0, 1000000, NULL, 0, RULE_FIELD(mArrows.mBonus)},
	{165, "arrows.turn", KIND_BOOL, GROUP_ARROWS, 0, 0, 0, NULL, 0, RULE_FIELD(mArrows.mTurn)},
	{166, "arrows.shots", KIND_MOVE_TYPES, GROUP_ARROWS, 0, 0, 0, RULE_NAMES(gArrowEdgeNames), RULE_FIELD(mArrows.mShots)}
};

const int NUM_FIELDS = (int) (sizeof(gFields) / sizeof(gFields[0]));

// Objects in the key tree that are not fields themselves
const char* const gNamespaces[] =
{
	"board", "board.gravity", "moves", "moves.twist", "moves.slide", "noMoves", "match", "specials", "gold",
	"gold.timeLimit", "undo", "scoring", "timing", "look", "arrows"
};

const char* const ADD_TYPES_KEY = "moves.addTypes";

// The derived defaults: noMoves.normal is "fallOut" with gold, else "gameOver"; match.diagonalMinRun is
// match.minRun. While merging, DERIVED marks a field that takes the derived default.
const int DERIVED = -1;

// Ints in BoardLayout's MAX_* range for shape patterns
const int MAX_SHAPE_CELLS = 16;
const int MAX_ID_LENGTH = 32;

int FindField(const std::string& theKey)
{
	for (int i = 0; i < NUM_FIELDS; i++)
	{
		if (theKey == gFields[i].mKey)
			return i;
	}
	return -1;
}

int FieldIndex(const char* theKey)
{
	return FindField(theKey);
}

bool IsNamespace(const std::string& theKey)
{
	for (size_t i = 0; i < sizeof(gNamespaces) / sizeof(gNamespaces[0]); i++)
	{
		if (theKey == gNamespaces[i])
			return true;
	}
	return false;
}

bool IsKeyed(const FieldDef& theField)
{
	return (theField.mKind == KIND_SHAPES) || (theField.mKind == KIND_BONUS);
}

// theKey names theField itself or a namespace above it ("match" covers match.minRun)
bool KeyCovers(const std::string& theKey, const char* theField)
{
	size_t n = theKey.size();
	return (strncmp(theKey.c_str(), theField, n) == 0) && ((theField[n] == '\0') || (theField[n] == '.'));
}

template<typename T> T& FieldRef(const FieldDef& theField, GameRules& theRules)
{
	return *(T*) theField.mPtr(&theRules);
}

template<typename T> const T& FieldRef(const FieldDef& theField, const GameRules& theRules)
{
	return *(const T*) theField.mPtr(const_cast<GameRules*>(&theRules));
}

// ---------------------------------------------------------------------------------------------------
// Small helpers

std::string IntText(int theValue)
{
	char aBuf[16];
	snprintf(aBuf, sizeof(aBuf), "%d", theValue);
	return aBuf;
}

bool ReadInt(const JsonValue& theValue, int* theOut)
{
	if (!theValue.IsNumber())
		return false;
	double d = theValue.AsNumber();
	if ((d != d) || (d != std::floor(d)) || (d < (double) INT_MIN) || (d > (double) INT_MAX))
		return false;
	*theOut = (int) d;
	return true;
}

// The legacy (int) conversion of a tunable (ModManager::GetTunableInt and GetTunableArray), made defined
// (6.1 rule 3): it truncates, and a value outside the int range gives INT_MIN, the "integer indefinite"
// that x86's cvttsd2si returned to the native game for it. wasm's saturating conversion gave INT_MAX
// instead, so now both platforms read such a tunable as the native game always did.
int LenientInt(double theValue)
{
	if ((theValue != theValue) || (theValue <= -2147483649.0) || (theValue >= 2147483648.0))
		return INT_MIN;
	return (int) theValue;
}

// A double as the canonical form and its text carry it: -0 is 0 (it plays the same) and every NaN is
// the one quiet NaN, so equivalent values hash alike on every platform
double CanonicalDouble(double theValue)
{
	if (theValue == 0.0)
		return 0.0;
	if (theValue != theValue)
		return std::numeric_limits<double>::quiet_NaN();
	return theValue;
}

std::string RangeText(int theMin, int theMax)
{
	if ((theMin == INT_MIN) && (theMax == INT_MAX))
		return "an integer";
	return "an integer " + IntText(theMin) + ".." + IntText(theMax);
}

int EditDistance(const std::string& a, const std::string& b)
{
	std::vector<int> aPrev(b.size() + 1), aCur(b.size() + 1);
	for (size_t j = 0; j <= b.size(); j++)
		aPrev[j] = (int) j;
	for (size_t i = 1; i <= a.size(); i++)
	{
		aCur[0] = (int) i;
		for (size_t j = 1; j <= b.size(); j++)
		{
			int aCost = (tolower((unsigned char) a[i - 1]) == tolower((unsigned char) b[j - 1])) ? 0 : 1;
			aCur[j] = std::min(std::min(aPrev[j] + 1, aCur[j - 1] + 1), aPrev[j - 1] + aCost);
		}
		aPrev.swap(aCur);
	}
	return aPrev[b.size()];
}

// " (did you mean x?)" for the closest candidate, or ""
std::string DidYouMean(const std::string& theKey, const std::vector<std::string>& theCandidates)
{
	std::string aBest;
	int aBestDistance = INT_MAX;
	for (size_t i = 0; i < theCandidates.size(); i++)
	{
		int aDistance = EditDistance(theKey, theCandidates[i]);
		if (aDistance < aBestDistance)
		{
			aBestDistance = aDistance;
			aBest = theCandidates[i];
		}
	}
	int aLimit = std::max(2, (int) theKey.size() / 3);
	if (aBest.empty() || (aBestDistance > aLimit))
		return "";
	return " (did you mean " + aBest + "?)";
}

std::vector<std::string> KnownKeys()
{
	std::vector<std::string> aKeys;
	for (int i = 0; i < NUM_FIELDS; i++)
		aKeys.push_back(gFields[i].mKey);
	for (size_t i = 0; i < sizeof(gNamespaces) / sizeof(gNamespaces[0]); i++)
		aKeys.push_back(gNamespaces[i]);
	aKeys.push_back(ADD_TYPES_KEY);
	return aKeys;
}

std::string UnknownKey(const std::string& theKey, const std::vector<std::string>& theCandidates)
{
	return "unknown key " + theKey + " ignored" + DidYouMean(theKey, theCandidates);
}

// Pattern names (3.2): runN, diagonal, L, T, plus, or a shape id
bool IsReservedPattern(const std::string& theName)
{
	if ((theName == "diagonal") || (theName == "L") || (theName == "T") || (theName == "plus"))
		return true;
	if ((theName.size() > 3) && (theName.compare(0, 3, "run") == 0))
	{
		for (size_t i = 3; i < theName.size(); i++)
		{
			if (!isdigit((unsigned char) theName[i]))
				return false;
		}
		return true;
	}
	return false;
}

bool IsShapeId(const std::string& theName)
{
	if (theName.empty() || (theName.size() > (size_t) MAX_ID_LENGTH) || !isalpha((unsigned char) theName[0]))
		return false;
	for (size_t i = 0; i < theName.size(); i++)
	{
		if (!isalnum((unsigned char) theName[i]) && (theName[i] != '_'))
			return false;
	}
	return !IsReservedPattern(theName);
}

// runN with N 2..16, written without leading zeros ("run05" is not "run5": one spelling per pattern)
int RunLength(const std::string& theName)
{
	if ((theName.size() < 4) || (theName.size() > 5) || (theName.compare(0, 3, "run") != 0) || (theName[3] == '0'))
		return 0;
	for (size_t i = 3; i < theName.size(); i++)
	{
		if (!isdigit((unsigned char) theName[i]))
			return 0;
	}
	int n = atoi(theName.c_str() + 3);
	return ((n >= 2) && (n <= BoardLayout::MAX_COLS)) ? n : 0;
}

bool IsTriggerName(const std::string& theName)
{
	return (RunLength(theName) > 0) || (theName == "diagonal") || (theName == "L") || (theName == "T") ||
		(theName == "plus") || IsShapeId(theName);
}

bool IsBonusName(const std::string& theName)
{
	return (theName == "diagonal") || (theName == "L") || (theName == "T") || (theName == "plus") || IsShapeId(theName);
}

typedef std::vector<std::pair<int, int> > CellList;

CellList NormalizeCells(CellList theCells)
{
	if (theCells.empty())
		return theCells;
	int aMinX = INT_MAX;
	int aMinY = INT_MAX;
	for (size_t i = 0; i < theCells.size(); i++)
	{
		aMinX = std::min(aMinX, theCells[i].first);
		aMinY = std::min(aMinY, theCells[i].second);
	}
	for (size_t i = 0; i < theCells.size(); i++)
	{
		theCells[i].first -= aMinX;
		theCells[i].second -= aMinY;
	}
	std::sort(theCells.begin(), theCells.end());
	return theCells;
}

// The cells of a preset shape and its orientations
bool PresetCells(const std::string& thePreset, CellList* theCells, bool* theRotations, bool* theMirror)
{
	theCells->clear();
	if (thePreset == "square")
	{
		*theCells = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
		*theRotations = false;
		*theMirror = false;
	}
	else if (thePreset == "L3")
	{
		*theCells = {{0, 0}, {0, 1}, {1, 1}};
		*theRotations = true;
		*theMirror = true;
	}
	else if (thePreset == "T4")
	{
		*theCells = {{0, 0}, {1, 0}, {2, 0}, {1, 1}};
		*theRotations = true;
		*theMirror = true;
	}
	else
		return false;
	return true;
}

// Distinct orientations of a cell pattern (rotations by 90 degrees, mirror in x)
std::vector<CellList> Orientations(const CellList& theCells, bool theRotations, bool theMirror)
{
	std::vector<CellList> anOut;
	std::set<CellList> aSeen;
	for (int aMirror = 0; aMirror < (theMirror ? 2 : 1); aMirror++)
	{
		CellList aCells = theCells;
		if (aMirror != 0)
		{
			for (size_t i = 0; i < aCells.size(); i++)
				aCells[i].first = -aCells[i].first;
		}
		for (int aRot = 0; aRot < (theRotations ? 4 : 1); aRot++)
		{
			CellList aNorm = NormalizeCells(aCells);
			if (aSeen.insert(aNorm).second)
				anOut.push_back(aNorm);
			for (size_t i = 0; i < aCells.size(); i++)
				aCells[i] = std::make_pair(-aCells[i].second, aCells[i].first);
		}
	}
	return anOut;
}

CellList ShapeCells(const ShapePattern& theShape, bool* theRotations, bool* theMirror)
{
	CellList aCells;
	if (!theShape.mPreset.empty())
		PresetCells(theShape.mPreset, &aCells, theRotations, theMirror);
	else
	{
		aCells = theShape.mCells;
		*theRotations = theShape.mRotations;
		*theMirror = theShape.mMirror;
	}
	return aCells;
}

std::set<CellList> OrientationSet(const CellList& theCells, bool theRotations, bool theMirror)
{
	std::vector<CellList> anOrientations = Orientations(theCells, theRotations, theMirror);
	return std::set<CellList>(anOrientations.begin(), anOrientations.end());
}

// The one spelling of a cell pattern (3.7: equivalent rules share a RulesId). Patterns with the same
// orientations match the same cells, so a pattern whose orientations are a preset's becomes that preset,
// and any other becomes its smallest orientation with the fewest flags that give back all of them: "square"
// and the four square cells with any flags are one pattern, and so are the L3 cells with rotations, with or
// without mirror.
void NormalizeShape(ShapePattern* theShape)
{
	bool aRotations;
	bool aMirror;
	CellList aCells = ShapeCells(*theShape, &aRotations, &aMirror);
	std::set<CellList> anOrientations = OrientationSet(aCells, aRotations, aMirror);
	static const char* const aPresets[] = {"square", "L3", "T4"};
	*theShape = ShapePattern();
	for (int i = 0; i < 3; i++)
	{
		PresetCells(aPresets[i], &aCells, &aRotations, &aMirror);
		if (OrientationSet(aCells, aRotations, aMirror) == anOrientations)
		{
			theShape->mPreset = aPresets[i];
			return;
		}
	}
	theShape->mCells = *anOrientations.begin();
	for (int aFlags = 0; aFlags < 4; aFlags++)
	{
		theShape->mRotations = (aFlags & 1) != 0;
		theShape->mMirror = (aFlags & 2) != 0;
		if (OrientationSet(theShape->mCells, theShape->mRotations, theShape->mMirror) == anOrientations)
			return;
	}
}

// ---------------------------------------------------------------------------------------------------
// Canonical bytes

class ByteWriter
{
public:
	std::string		mBytes;

	void U8(int theValue)
	{
		mBytes += (char) (uint8_t) theValue;
	}

	void U16(int theValue)
	{
		U8(theValue & 0xFF);
		U8((theValue >> 8) & 0xFF);
	}

	void I32(int theValue)
	{
		uint32_t aValue = (uint32_t) theValue;
		for (int i = 0; i < 4; i++)
			U8((int) ((aValue >> (i * 8)) & 0xFF));
	}

	void F64(double theValue)
	{
		uint64_t aBits;
		theValue = CanonicalDouble(theValue);
		memcpy(&aBits, &theValue, sizeof(aBits));
		for (int i = 0; i < 8; i++)
			U8((int) ((aBits >> (i * 8)) & 0xFF));
	}

	void Str(const std::string& theText)
	{
		U16((int) theText.size());
		mBytes += theText;
	}

	void Strings(const std::vector<std::string>& theTexts)
	{
		U16((int) theTexts.size());
		for (size_t i = 0; i < theTexts.size(); i++)
			Str(theTexts[i]);
	}
};

class ByteReader
{
public:
	const std::string& mBytes;
	size_t			mPos;
	bool			mFailed;

	explicit ByteReader(const std::string& theBytes) : mBytes(theBytes), mPos(0), mFailed(false) {}

	bool AtEnd() const { return mPos >= mBytes.size(); }

	int U8()
	{
		if (mPos >= mBytes.size())
		{
			mFailed = true;
			return 0;
		}
		return (uint8_t) mBytes[mPos++];
	}

	int U16()
	{
		int aLow = U8();
		return aLow | (U8() << 8);
	}

	int I32()
	{
		uint32_t aValue = 0;
		for (int i = 0; i < 4; i++)
			aValue |= (uint32_t) U8() << (i * 8);
		return (int) aValue;
	}

	double F64()
	{
		uint64_t aBits = 0;
		for (int i = 0; i < 8; i++)
			aBits |= (uint64_t) U8() << (i * 8);
		double aValue;
		memcpy(&aValue, &aBits, sizeof(aValue));
		return aValue;
	}

	std::string Str()
	{
		int aLength = U16();
		if (mFailed || (mPos + aLength > mBytes.size()))
		{
			mFailed = true;
			return "";
		}
		std::string aText = mBytes.substr(mPos, aLength);
		mPos += aLength;
		return aText;
	}

	std::vector<std::string> Strings()
	{
		int aCount = U16();
		std::vector<std::string> aTexts;
		for (int i = 0; (i < aCount) && !mFailed; i++)
			aTexts.push_back(Str());
		return aTexts;
	}
};

// FLAG_BOOL enums are stored as bools, the others as ints
bool EnumIsBool(const FieldDef& theField)
{
	return (theField.mFlags & FLAG_BOOL) != 0;
}

void SetEnum(const FieldDef& theField, GameRules& theRules, int theIndex)
{
	if (EnumIsBool(theField))
		FieldRef<bool>(theField, theRules) = (theIndex != 0);
	else
		FieldRef<int>(theField, theRules) = theIndex;
}

int GetEnum(const FieldDef& theField, const GameRules& theRules)
{
	if (EnumIsBool(theField))
		return FieldRef<bool>(theField, theRules) ? 1 : 0;
	return FieldRef<int>(theField, theRules);
}

int EnumIndex(const FieldDef& theField, const std::string& theName)
{
	for (int i = 0; i < theField.mNameCount; i++)
	{
		if (theName == theField.mNames[i])
			return i;
	}
	return -1;
}

void EncodeValue(const FieldDef& theField, const GameRules& theRules, ByteWriter& w)
{
	switch (theField.mKind)
	{
	case KIND_INT:
		w.I32(FieldRef<int>(theField, theRules));
		break;
	case KIND_BOOL:
		w.U8(FieldRef<bool>(theField, theRules) ? 1 : 0);
		break;
	case KIND_DOUBLE:
		w.F64(FieldRef<double>(theField, theRules));
		break;
	case KIND_ENUM:
	{
		int anIndex = GetEnum(theField, theRules);
		w.Str(((anIndex >= 0) && (anIndex < theField.mNameCount)) ? theField.mNames[anIndex] : "");
		break;
	}
	case KIND_INT_LIST:
	{
		const std::vector<int>& aList = FieldRef<std::vector<int> >(theField, theRules);
		w.U16((int) aList.size());
		for (size_t i = 0; i < aList.size(); i++)
			w.I32(aList[i]);
		break;
	}
	case KIND_MASK:
		w.Strings(FieldRef<std::vector<std::string> >(theField, theRules));
		break;
	case KIND_LEVELS:
	{
		const std::vector<LevelShape>& aLevels = FieldRef<std::vector<LevelShape> >(theField, theRules);
		w.U16((int) aLevels.size());
		for (size_t i = 0; i < aLevels.size(); i++)
		{
			w.I32(aLevels[i].mCols);
			w.I32(aLevels[i].mRows);
			w.Strings(aLevels[i].mMask);
		}
		break;
	}
	case KIND_TIME_LIMIT:
	{
		const TimeLimit& aLimit = FieldRef<TimeLimit>(theField, theRules);
		w.U8(aLimit.mEnabled ? 1 : 0);
		if (aLimit.mEnabled)
		{
			w.I32(aLimit.mBase);
			w.I32(aLimit.mPerLevel);
			w.I32(aLimit.mMin);
		}
		break;
	}
	case KIND_SHAPES:
	{
		const std::map<std::string, ShapePattern>& aShapes = FieldRef<std::map<std::string, ShapePattern> >(theField, theRules);
		w.U16((int) aShapes.size());
		for (std::map<std::string, ShapePattern>::const_iterator it = aShapes.begin(); it != aShapes.end(); ++it)
		{
			w.Str(it->first);
			w.Str(it->second.mPreset);
			w.U16((int) it->second.mCells.size());
			for (size_t i = 0; i < it->second.mCells.size(); i++)
			{
				w.I32(it->second.mCells[i].first);
				w.I32(it->second.mCells[i].second);
			}
			w.U8(it->second.mRotations ? 1 : 0);
			w.U8(it->second.mMirror ? 1 : 0);
		}
		break;
	}
	case KIND_BONUS:
	{
		const std::map<std::string, int>& aBonus = FieldRef<std::map<std::string, int> >(theField, theRules);
		w.U16((int) aBonus.size());
		for (std::map<std::string, int>::const_iterator it = aBonus.begin(); it != aBonus.end(); ++it)
		{
			w.Str(it->first);
			w.I32(it->second);
		}
		break;
	}
	case KIND_SPECIAL:
	{
		const SpecialRule& aSpecial = FieldRef<SpecialRule>(theField, theRules);
		w.U8(aSpecial.mEnabled ? 1 : 0);
		if (aSpecial.mEnabled)
		{
			w.Strings(aSpecial.mCreatedBy);
			w.I32(aSpecial.mRadius);
			w.I32(aSpecial.mPoints);
		}
		break;
	}
	case KIND_MOVE_TYPES:
		w.U8(FieldRef<int>(theField, theRules));
		break;
	}
}

bool DecodeValue(const FieldDef& theField, GameRules& theRules, ByteReader& r)
{
	switch (theField.mKind)
	{
	case KIND_INT:
		FieldRef<int>(theField, theRules) = r.I32();
		break;
	case KIND_BOOL:
		FieldRef<bool>(theField, theRules) = r.U8() != 0;
		break;
	case KIND_DOUBLE:
		FieldRef<double>(theField, theRules) = r.F64();
		break;
	case KIND_ENUM:
	{
		int anIndex = EnumIndex(theField, r.Str());
		if (anIndex < 0)
			return false;
		SetEnum(theField, theRules, anIndex);
		break;
	}
	case KIND_INT_LIST:
	{
		std::vector<int>& aList = FieldRef<std::vector<int> >(theField, theRules);
		aList.clear();
		int aCount = r.U16();
		for (int i = 0; (i < aCount) && !r.mFailed; i++)
			aList.push_back(r.I32());
		break;
	}
	case KIND_MASK:
		FieldRef<std::vector<std::string> >(theField, theRules) = r.Strings();
		break;
	case KIND_LEVELS:
	{
		std::vector<LevelShape>& aLevels = FieldRef<std::vector<LevelShape> >(theField, theRules);
		aLevels.clear();
		int aCount = r.U16();
		for (int i = 0; (i < aCount) && !r.mFailed; i++)
		{
			LevelShape aLevel;
			aLevel.mCols = r.I32();
			aLevel.mRows = r.I32();
			aLevel.mMask = r.Strings();
			aLevels.push_back(aLevel);
		}
		break;
	}
	case KIND_TIME_LIMIT:
	{
		TimeLimit& aLimit = FieldRef<TimeLimit>(theField, theRules);
		aLimit = TimeLimit();
		aLimit.mEnabled = r.U8() != 0;
		if (aLimit.mEnabled)
		{
			aLimit.mBase = r.I32();
			aLimit.mPerLevel = r.I32();
			aLimit.mMin = r.I32();
		}
		break;
	}
	case KIND_SHAPES:
	{
		std::map<std::string, ShapePattern>& aShapes = FieldRef<std::map<std::string, ShapePattern> >(theField, theRules);
		aShapes.clear();
		int aCount = r.U16();
		for (int i = 0; (i < aCount) && !r.mFailed; i++)
		{
			std::string anId = r.Str();
			ShapePattern aShape;
			aShape.mPreset = r.Str();
			int aCells = r.U16();
			for (int j = 0; (j < aCells) && !r.mFailed; j++)
			{
				int x = r.I32();
				aShape.mCells.push_back(std::make_pair(x, r.I32()));
			}
			aShape.mRotations = r.U8() != 0;
			aShape.mMirror = r.U8() != 0;
			aShapes[anId] = aShape;
		}
		break;
	}
	case KIND_BONUS:
	{
		std::map<std::string, int>& aBonus = FieldRef<std::map<std::string, int> >(theField, theRules);
		aBonus.clear();
		int aCount = r.U16();
		for (int i = 0; (i < aCount) && !r.mFailed; i++)
		{
			std::string anId = r.Str();
			aBonus[anId] = r.I32();
		}
		break;
	}
	case KIND_SPECIAL:
	{
		SpecialRule& aSpecial = FieldRef<SpecialRule>(theField, theRules);
		aSpecial = SpecialRule();
		aSpecial.mEnabled = r.U8() != 0;
		if (aSpecial.mEnabled)
		{
			aSpecial.mCreatedBy = r.Strings();
			aSpecial.mRadius = r.I32();
			aSpecial.mPoints = r.I32();
		}
		break;
	}
	case KIND_MOVE_TYPES:
		FieldRef<int>(theField, theRules) = r.U8();
		break;
	default:
		return false;
	}
	return !r.mFailed;
}

std::string EncodeField(const FieldDef& theField, const GameRules& theRules)
{
	ByteWriter w;
	EncodeValue(theField, theRules, w);
	return w.mBytes;
}

void CopyField(const FieldDef& theField, GameRules& theDest, const GameRules& theSource)
{
	switch (theField.mKind)
	{
	case KIND_INT:
	case KIND_MOVE_TYPES:
		FieldRef<int>(theField, theDest) = FieldRef<int>(theField, theSource);
		break;
	case KIND_BOOL:
		FieldRef<bool>(theField, theDest) = FieldRef<bool>(theField, theSource);
		break;
	case KIND_DOUBLE:
		FieldRef<double>(theField, theDest) = FieldRef<double>(theField, theSource);
		break;
	case KIND_ENUM:
		SetEnum(theField, theDest, GetEnum(theField, theSource));
		break;
	case KIND_INT_LIST:
		FieldRef<std::vector<int> >(theField, theDest) = FieldRef<std::vector<int> >(theField, theSource);
		break;
	case KIND_MASK:
		FieldRef<std::vector<std::string> >(theField, theDest) = FieldRef<std::vector<std::string> >(theField, theSource);
		break;
	case KIND_LEVELS:
		FieldRef<std::vector<LevelShape> >(theField, theDest) = FieldRef<std::vector<LevelShape> >(theField, theSource);
		break;
	case KIND_TIME_LIMIT:
		FieldRef<TimeLimit>(theField, theDest) = FieldRef<TimeLimit>(theField, theSource);
		break;
	case KIND_SHAPES:
		FieldRef<std::map<std::string, ShapePattern> >(theField, theDest) = FieldRef<std::map<std::string, ShapePattern> >(theField, theSource);
		break;
	case KIND_BONUS:
		FieldRef<std::map<std::string, int> >(theField, theDest) = FieldRef<std::map<std::string, int> >(theField, theSource);
		break;
	case KIND_SPECIAL:
		FieldRef<SpecialRule>(theField, theDest) = FieldRef<SpecialRule>(theField, theSource);
		break;
	}
}

// The defaults to compare theRules with, derived defaults resolved against theRules
GameRules DefaultsFor(const GameRules& theRules)
{
	GameRules aDefaults;
	aDefaults.mNoMoves.mNormal = theRules.mGold.mEnabled ? GameRules::NOMOVES_FALL_OUT : GameRules::NOMOVES_GAME_OVER;
	aDefaults.mMatch.mDiagonalMinRun = theRules.mMatch.mMinRun;
	return aDefaults;
}

void ResolveDerived(GameRules& theRules)
{
	if (theRules.mNoMoves.mNormal == DERIVED)
		theRules.mNoMoves.mNormal = theRules.mGold.mEnabled ? GameRules::NOMOVES_FALL_OUT : GameRules::NOMOVES_GAME_OVER;
	if (theRules.mMatch.mDiagonalMinRun == DERIVED)
		theRules.mMatch.mDiagonalMinRun = theRules.mMatch.mMinRun;
}

// Defaults with the derived fields left open (the start of a mod's view and of a merge)
GameRules OpenDefaults()
{
	GameRules aRules;
	aRules.mNoMoves.mNormal = DERIVED;
	aRules.mMatch.mDiagonalMinRun = DERIVED;
	return aRules;
}

// ---------------------------------------------------------------------------------------------------
// Values as JSON (the text form, and messages)

JsonValue NumberJson(double theValue)
{
	return JsonValue(CanonicalDouble(theValue));
}

JsonValue StringsJson(const std::vector<std::string>& theTexts)
{
	JsonValue anArray = JsonValue::MakeArray();
	for (size_t i = 0; i < theTexts.size(); i++)
		anArray.Push(JsonValue(theTexts[i]));
	return anArray;
}

JsonValue ValueJson(const FieldDef& theField, const GameRules& theRules)
{
	switch (theField.mKind)
	{
	case KIND_INT:
		return NumberJson(FieldRef<int>(theField, theRules));
	case KIND_BOOL:
		return JsonValue(FieldRef<bool>(theField, theRules));
	case KIND_DOUBLE:
		return NumberJson(FieldRef<double>(theField, theRules));
	case KIND_ENUM:
	{
		int anIndex = GetEnum(theField, theRules);
		return JsonValue(std::string(((anIndex >= 0) && (anIndex < theField.mNameCount)) ? theField.mNames[anIndex] : "?"));
	}
	case KIND_INT_LIST:
	{
		JsonValue anArray = JsonValue::MakeArray();
		const std::vector<int>& aList = FieldRef<std::vector<int> >(theField, theRules);
		for (size_t i = 0; i < aList.size(); i++)
			anArray.Push(NumberJson(aList[i]));
		return anArray;
	}
	case KIND_MASK:
		return StringsJson(FieldRef<std::vector<std::string> >(theField, theRules));
	case KIND_LEVELS:
	{
		JsonValue anArray = JsonValue::MakeArray();
		const std::vector<LevelShape>& aLevels = FieldRef<std::vector<LevelShape> >(theField, theRules);
		for (size_t i = 0; i < aLevels.size(); i++)
		{
			JsonValue aLevel = JsonValue::MakeObject();
			aLevel.Set("cols", NumberJson(aLevels[i].mCols));
			aLevel.Set("rows", NumberJson(aLevels[i].mRows));
			if (!aLevels[i].mMask.empty())
				aLevel.Set("mask", StringsJson(aLevels[i].mMask));
			anArray.Push(aLevel);
		}
		return anArray;
	}
	case KIND_TIME_LIMIT:
	{
		const TimeLimit& aLimit = FieldRef<TimeLimit>(theField, theRules);
		if (!aLimit.mEnabled)
			return JsonValue();
		JsonValue anObject = JsonValue::MakeObject();
		anObject.Set("base", NumberJson(aLimit.mBase));
		anObject.Set("perLevel", NumberJson(aLimit.mPerLevel));
		anObject.Set("min", NumberJson(aLimit.mMin));
		return anObject;
	}
	case KIND_SHAPES:
	{
		JsonValue anObject = JsonValue::MakeObject();
		const std::map<std::string, ShapePattern>& aShapes = FieldRef<std::map<std::string, ShapePattern> >(theField, theRules);
		for (std::map<std::string, ShapePattern>::const_iterator it = aShapes.begin(); it != aShapes.end(); ++it)
		{
			JsonValue aShape = JsonValue::MakeObject();
			if (!it->second.mPreset.empty())
				aShape.Set("preset", JsonValue(it->second.mPreset));
			else
			{
				JsonValue aCells = JsonValue::MakeArray();
				for (size_t i = 0; i < it->second.mCells.size(); i++)
				{
					JsonValue aCell = JsonValue::MakeArray();
					aCell.Push(NumberJson(it->second.mCells[i].first));
					aCell.Push(NumberJson(it->second.mCells[i].second));
					aCells.Push(aCell);
				}
				aShape.Set("cells", aCells);
				aShape.Set("rotations", JsonValue(it->second.mRotations));
				aShape.Set("mirror", JsonValue(it->second.mMirror));
			}
			anObject.Set(it->first, aShape);
		}
		return anObject;
	}
	case KIND_BONUS:
	{
		JsonValue anObject = JsonValue::MakeObject();
		const std::map<std::string, int>& aBonus = FieldRef<std::map<std::string, int> >(theField, theRules);
		for (std::map<std::string, int>::const_iterator it = aBonus.begin(); it != aBonus.end(); ++it)
			anObject.Set(it->first, NumberJson(it->second));
		return anObject;
	}
	case KIND_SPECIAL:
	{
		const SpecialRule& aSpecial = FieldRef<SpecialRule>(theField, theRules);
		if (!aSpecial.mEnabled)
			return JsonValue();
		JsonValue anObject = JsonValue::MakeObject();
		anObject.Set("createdBy", StringsJson(aSpecial.mCreatedBy));
		if (strcmp(theField.mKey, "specials.flame") == 0)
			anObject.Set("radius", NumberJson(aSpecial.mRadius));
		anObject.Set("points", NumberJson(aSpecial.mPoints));
		return anObject;
	}
	case KIND_MOVE_TYPES:
	{
		JsonValue anArray = JsonValue::MakeArray();
		int aTypes = FieldRef<int>(theField, theRules);
		for (int i = 0; i < theField.mNameCount; i++)
		{
			if (aTypes & (1 << i))
				anArray.Push(JsonValue(std::string(theField.mNames[i])));
		}
		return anArray;
	}
	}
	return JsonValue();
}

// ---------------------------------------------------------------------------------------------------
// Parsing one mod

class ParseLog
{
public:
	std::vector<std::string> mErrors;
	std::vector<std::string> mWarnings;

	bool Error(const std::string& theText)
	{
		mErrors.push_back(theText);
		return false;
	}

	void Warning(const std::string& theText)
	{
		mWarnings.push_back(theText);
	}
};

bool ParseMask(const std::string& theKey, const JsonValue& theValue, std::vector<std::string>* theMask, ParseLog& theLog)
{
	theMask->clear();
	if (theValue.IsArray())
	{
		for (size_t i = 0; i < theValue.Size(); i++)
		{
			if (!theValue[i].IsString())
			{
				theMask->clear();
				break;
			}
			theMask->push_back(theValue[i].AsString());
		}
	}
	if (theMask->empty() || !BoardLayout::MaskIsWellFormed(*theMask))
		return theLog.Error(theKey + " must be 1..16 strings of the same length (1..16), each character one of #.d23456789");
	return true;
}

// A mask that is all '#' is the same board as no mask
void NormalizeMask(std::vector<std::string>* theMask)
{
	for (size_t i = 0; i < theMask->size(); i++)
	{
		if ((*theMask)[i].find_first_not_of('#') != std::string::npos)
			return;
	}
	theMask->clear();
}

bool ParseLevelShape(const std::string& theKey, const JsonValue& theValue, LevelShape* theShape, ParseLog& theLog)
{
	*theShape = LevelShape();
	if (!theValue.IsObject())
		return theLog.Error(theKey + " must be an object {cols, rows, mask}");
	bool hasCols = false;
	bool hasRows = false;
	bool hasMask = false;
	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& aName = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string aKey = theKey + "." + aName;
		if ((aName == "cols") || (aName == "rows"))
		{
			int n;
			if (!ReadInt(aValue, &n) || (n < 3) || (n > BoardLayout::MAX_COLS))
				return theLog.Error(aKey + " must be an integer 3..16");
			if (aName == "cols")
			{
				theShape->mCols = n;
				hasCols = true;
			}
			else
			{
				theShape->mRows = n;
				hasRows = true;
			}
		}
		else if (aName == "mask")
		{
			if (!ParseMask(aKey, aValue, &theShape->mMask, theLog))
				return false;
			hasMask = true;
		}
		else
			theLog.Warning(UnknownKey(aKey, {theKey + ".cols", theKey + ".rows", theKey + ".mask"}));
	}
	if (hasMask)
	{
		int aCols = (int) theShape->mMask[0].size();
		int aRows = (int) theShape->mMask.size();
		if ((hasCols && (theShape->mCols != aCols)) || (hasRows && (theShape->mRows != aRows)))
			return theLog.Error(theKey + ".mask is " + IntText(aCols) + "x" + IntText(aRows) + ", not " +
				IntText(theShape->mCols) + "x" + IntText(theShape->mRows));
		theShape->mCols = aCols;
		theShape->mRows = aRows;
		NormalizeMask(&theShape->mMask);
	}
	return true;
}

bool ParseCreatedBy(const std::string& theKey, const JsonValue& theValue, std::vector<std::string>* theOut, ParseLog& theLog)
{
	theOut->clear();
	if (theValue.IsArray() && (theValue.Size() > 0) && (theValue.Size() <= 16))
	{
		for (size_t i = 0; i < theValue.Size(); i++)
		{
			if (!theValue[i].IsString() || !IsTriggerName(theValue[i].AsString()))
				return theLog.Error(theKey + " has an invalid pattern name (run2..run16, diagonal, L, T, plus or a shape id)");
			theOut->push_back(theValue[i].AsString());
		}
		std::sort(theOut->begin(), theOut->end());
		theOut->erase(std::unique(theOut->begin(), theOut->end()), theOut->end());
		return true;
	}
	return theLog.Error(theKey + " must be a list of 1..16 pattern names");
}

bool ParseSpecial(const std::string& theKey, const JsonValue& theValue, bool isFlame, SpecialRule* theSpecial, ParseLog& theLog)
{
	*theSpecial = SpecialRule();
	if (theValue.IsNull())
		return true;
	if (!theValue.IsObject())
		return theLog.Error(theKey + " must be null or an object {createdBy, " + (isFlame ? "radius, " : "") + "points}");
	// Bejeweled 2: a run of 4 makes a flame gem, a run of 5 a lightning gem
	theSpecial->mEnabled = true;
	theSpecial->mCreatedBy.push_back(isFlame ? "run4" : "run5");
	theSpecial->mRadius = 1;
	theSpecial->mPoints = isFlame ? 20 : 30;
	std::vector<std::string> aKnown = {theKey + ".createdBy", theKey + ".points"};
	if (isFlame)
		aKnown.push_back(theKey + ".radius");
	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& aName = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string aKey = theKey + "." + aName;
		if (aName == "createdBy")
		{
			if (!ParseCreatedBy(aKey, aValue, &theSpecial->mCreatedBy, theLog))
				return false;
		}
		else if ((aName == "radius") && isFlame)
		{
			if (!ReadInt(aValue, &theSpecial->mRadius) || (theSpecial->mRadius < 1) || (theSpecial->mRadius > 3))
				return theLog.Error(aKey + " must be an integer 1..3");
		}
		else if (aName == "points")
		{
			if (!ReadInt(aValue, &theSpecial->mPoints) || (theSpecial->mPoints < 0) || (theSpecial->mPoints > 1000000))
				return theLog.Error(aKey + " must be an integer 0..1000000");
		}
		else
			theLog.Warning(UnknownKey(aKey, aKnown));
	}
	return true;
}

bool ParseTimeLimit(const std::string& theKey, const JsonValue& theValue, TimeLimit* theLimit, ParseLog& theLog)
{
	*theLimit = TimeLimit();
	if (theValue.IsNull())
		return true;
	if (!theValue.IsObject())
		return theLog.Error(theKey + " must be null (untimed) or an object {base, perLevel, min} in seconds");
	theLimit->mEnabled = true;
	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& aName = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string aKey = theKey + "." + aName;
		int* aTarget = (aName == "base") ? &theLimit->mBase : (aName == "perLevel") ? &theLimit->mPerLevel :
			(aName == "min") ? &theLimit->mMin : NULL;
		if (aTarget == NULL)
		{
			theLog.Warning(UnknownKey(aKey, {theKey + ".base", theKey + ".perLevel", theKey + ".min"}));
			continue;
		}
		if (!ReadInt(aValue, aTarget) || (*aTarget < -100000) || (*aTarget > 100000))
			return theLog.Error(aKey + " must be an integer number of seconds");
	}
	// 3.6: time limits of at least 10 s
	if (theLimit->mBase < 10)
	{
		theLog.Warning(theKey + ".base " + IntText(theLimit->mBase) + " clamped to 10 seconds");
		theLimit->mBase = 10;
	}
	if (theLimit->mMin < 10)
	{
		theLog.Warning(theKey + ".min " + IntText(theLimit->mMin) + " clamped to 10 seconds");
		theLimit->mMin = 10;
	}
	return true;
}

bool ParseShape(const std::string& theKey, const JsonValue& theValue, ShapePattern* theShape, ParseLog& theLog)
{
	*theShape = ShapePattern();
	if (!theValue.IsObject())
		return theLog.Error(theKey + " must be an object {\"preset\": ...} or {\"cells\": [[x,y],...], \"rotations\", \"mirror\"}");
	bool hasCells = false;
	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& aName = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string aKey = theKey + "." + aName;
		if (aName == "preset")
		{
			CellList aCells;
			bool aRotations;
			bool aMirror;
			if (!aValue.IsString() || !PresetCells(aValue.AsString(), &aCells, &aRotations, &aMirror))
				return theLog.Error(aKey + " must be \"square\", \"L3\" or \"T4\"");
			theShape->mPreset = aValue.AsString();
		}
		else if (aName == "cells")
		{
			hasCells = true;
			if (!aValue.IsArray() || (aValue.Size() < 2) || (aValue.Size() > (size_t) MAX_SHAPE_CELLS))
				return theLog.Error(aKey + " must be a list of 2..16 cells [x, y]");
			for (size_t j = 0; j < aValue.Size(); j++)
			{
				int x;
				int y;
				const JsonValue& aCell = aValue[j];
				if (!aCell.IsArray() || (aCell.Size() != 2) || !ReadInt(aCell[0], &x) || !ReadInt(aCell[1], &y) ||
					(x < 0) || (y < 0) || (x >= BoardLayout::MAX_COLS) || (y >= BoardLayout::MAX_ROWS))
					return theLog.Error(aKey + " must be a list of cells [x, y] with x and y 0..15");
				theShape->mCells.push_back(std::make_pair(x, y));
			}
		}
		else if ((aName == "rotations") || (aName == "mirror"))
		{
			if (!aValue.IsBool())
				return theLog.Error(aKey + " must be true or false");
			(aName == "rotations" ? theShape->mRotations : theShape->mMirror) = aValue.AsBool();
		}
		else
			theLog.Warning(UnknownKey(aKey, {theKey + ".preset", theKey + ".cells", theKey + ".rotations", theKey + ".mirror"}));
	}
	if (theShape->mPreset.empty() == !hasCells)
	{
		if (hasCells)
			return theLog.Error(theKey + " has both \"preset\" and \"cells\"");
		return theLog.Error(theKey + " needs \"preset\" or \"cells\"");
	}
	if (!theShape->mPreset.empty())
	{
		// A preset brings its own orientations
		theShape->mRotations = false;
		theShape->mMirror = false;
	}
	else
	{
		CellList aCells = NormalizeCells(theShape->mCells);
		if (std::unique(aCells.begin(), aCells.end()) != aCells.end())
			return theLog.Error(theKey + ".cells lists a cell twice");
		theShape->mCells = aCells;
	}
	NormalizeShape(theShape);
	return true;
}

// A name or a list of names (moves.type, moves.addTypes, arrows.directions): a bit for each, at its index
bool ParseNameSet(const std::string& theKey, const JsonValue& theValue, const char* const* theNames, int theNameCount,
	int* theTypes, ParseLog& theLog)
{
	*theTypes = 0;
	std::vector<JsonValue> aNames;
	if (theValue.IsString())
		aNames.push_back(theValue);
	else if (theValue.IsArray())
		aNames = theValue.Items();
	for (size_t i = 0; i < aNames.size(); i++)
	{
		int aType = -1;
		for (int j = 0; j < theNameCount; j++)
		{
			if (aNames[i].IsString() && (aNames[i].AsString() == theNames[j]))
				aType = j;
		}
		if (aType < 0)
		{
			*theTypes = 0;
			break;
		}
		*theTypes |= 1 << aType;
	}
	if (*theTypes == 0)
	{
		std::string aList;
		for (int j = 0; j < theNameCount; j++)
			aList += std::string(j == 0 ? "" : ", ") + "\"" + theNames[j] + "\"";
		return theLog.Error(theKey + " must be " + aList + " or a list of them");
	}
	return true;
}

// A value under "rules": type and range checked
bool ParseStrict(const FieldDef& theField, const JsonValue& theValue, GameRules& theRules, ParseLog& theLog)
{
	std::string aKey = theField.mKey;
	switch (theField.mKind)
	{
	case KIND_INT:
	{
		int n;
		if (!ReadInt(theValue, &n))
			return theLog.Error(aKey + " must be " + RangeText(theField.mMin, theField.mMax));
		if ((n < theField.mMin) || (n > theField.mMax))
		{
			if (!(theField.mFlags & FLAG_CLAMP))
				return theLog.Error(aKey + " must be " + RangeText(theField.mMin, theField.mMax));
			int aClamped = std::max(theField.mMin, std::min(theField.mMax, n));
			theLog.Warning(aKey + " " + IntText(n) + " clamped to " + IntText(aClamped));
			n = aClamped;
		}
		FieldRef<int>(theField, theRules) = n;
		return true;
	}
	case KIND_BOOL:
		if (!theValue.IsBool())
			return theLog.Error(aKey + " must be true or false");
		FieldRef<bool>(theField, theRules) = theValue.AsBool();
		return true;
	case KIND_DOUBLE:
	{
		double d = theValue.AsNumber();
		if (!theValue.IsNumber() || (d != d) || (d > 1e300) || (d < -1e300))
			return theLog.Error(aKey + " must be a number");
		FieldRef<double>(theField, theRules) = d;
		return true;
	}
	case KIND_ENUM:
	{
		int anIndex = theValue.IsString() ? EnumIndex(theField, theValue.AsString()) : -1;
		if (anIndex < 0)
		{
			std::string aNames;
			for (int i = 0; i < theField.mNameCount; i++)
				aNames += std::string(i ? ", " : "") + "\"" + theField.mNames[i] + "\"";
			return theLog.Error(aKey + " must be one of " + aNames);
		}
		SetEnum(theField, theRules, anIndex);
		return true;
	}
	case KIND_INT_LIST:
	{
		std::vector<int> aList;
		if (theValue.IsArray() && (theValue.Size() > 0) && (theValue.Size() <= 64))
		{
			for (size_t i = 0; i < theValue.Size(); i++)
			{
				int n;
				if (!ReadInt(theValue[i], &n))
				{
					aList.clear();
					break;
				}
				aList.push_back(n);
			}
		}
		if (aList.empty())
			return theLog.Error(aKey + " must be a list of 1..64 integers");
		FieldRef<std::vector<int> >(theField, theRules) = aList;
		return true;
	}
	case KIND_MASK:
	{
		std::vector<std::string>& aMask = FieldRef<std::vector<std::string> >(theField, theRules);
		return ParseMask(aKey, theValue, &aMask, theLog);
	}
	case KIND_LEVELS:
	{
		std::vector<LevelShape> aLevels;
		if (!theValue.IsArray() || (theValue.Size() < 1) || (theValue.Size() > (size_t) GameRules::MAX_LEVELS))
			return theLog.Error(aKey + " must be a list of 1.." + IntText(GameRules::MAX_LEVELS) + " shapes {cols, rows, mask}");
		for (size_t i = 0; i < theValue.Size(); i++)
		{
			LevelShape aLevel;
			if (!ParseLevelShape(aKey + "[" + IntText((int) i) + "]", theValue[i], &aLevel, theLog))
				return false;
			aLevels.push_back(aLevel);
		}
		FieldRef<std::vector<LevelShape> >(theField, theRules) = aLevels;
		return true;
	}
	case KIND_TIME_LIMIT:
		return ParseTimeLimit(aKey, theValue, &FieldRef<TimeLimit>(theField, theRules), theLog);
	case KIND_SPECIAL:
		return ParseSpecial(aKey, theValue, aKey == "specials.flame", &FieldRef<SpecialRule>(theField, theRules), theLog);
	case KIND_MOVE_TYPES:
		return ParseNameSet(aKey, theValue, theField.mNames, theField.mNameCount, &FieldRef<int>(theField, theRules), theLog);
	}
	return theLog.Error(aKey + " cannot be set this way");
}

// A legacy tunable under "tunables": read as GameConfig::Load always did (ModManager::GetTunable*): a
// wrong type gives the default, numbers are truncated to ints, gemColors is clamped
void ParseLenient(const FieldDef& theField, const JsonValue& theValue, GameRules& theRules)
{
	GameRules aDefaults;
	switch (theField.mKind)
	{
	case KIND_INT:
	{
		int n = theValue.IsNumber() ? LenientInt(theValue.AsNumber()) : FieldRef<int>(theField, aDefaults);
		FieldRef<int>(theField, theRules) = std::max(theField.mMin, std::min(theField.mMax, n));
		break;
	}
	case KIND_DOUBLE:
		FieldRef<double>(theField, theRules) = theValue.AsNumber(FieldRef<double>(theField, aDefaults));
		break;
	case KIND_INT_LIST:
	{
		std::vector<int> aList;
		if (theValue.IsArray())
		{
			for (size_t i = 0; i < theValue.Size(); i++)
				aList.push_back(LenientInt(theValue[i].AsNumber()));
		}
		FieldRef<std::vector<int> >(theField, theRules) = aList.empty() ? FieldRef<std::vector<int> >(theField, aDefaults) : aList;
		break;
	}
	}
}

// One enabled mod's own view of the rules: the defaults plus what its "tunables" and "rules" set
struct ModView
{
	std::string				mId;
	GameRules				mRules;
	std::vector<char>		mSet;               // per field: set or reset by this mod
	std::map<std::string, bool> mShapeEntries;  // match.shapes.<id>: set (true) or reset (false)
	std::map<std::string, bool> mBonusEntries;  // match.bonus.<id>
	bool					mShapesReset;       // match.shapes: null
	bool					mBonusReset;
	int						mAddTypes;          // moves.addTypes
	bool					mAddTypesReset;
	bool					mHasRules;          // "rules" read and accepted

	ModView() : mRules(OpenDefaults()), mSet(NUM_FIELDS, 0), mShapesReset(false), mBonusReset(false), mAddTypes(0),
		mAddTypesReset(false), mHasRules(false) {}

	std::map<std::string, bool>& Entries(const FieldDef& theField)
	{
		return (theField.mKind == KIND_SHAPES) ? mShapeEntries : mBonusEntries;
	}

	bool& ContainerReset(const FieldDef& theField)
	{
		return (theField.mKind == KIND_SHAPES) ? mShapesReset : mBonusReset;
	}
};

void ResetField(ModView& theView, int theField)
{
	const FieldDef& aField = gFields[theField];
	GameRules aDefaults = OpenDefaults();
	CopyField(aField, theView.mRules, aDefaults);
	theView.mSet[theField] = 1;
	if (IsKeyed(aField))
	{
		theView.ContainerReset(aField) = true;
		theView.Entries(aField).clear();
	}
}

void ResetNamespace(ModView& theView, const std::string& theNamespace)
{
	for (int i = 0; i < NUM_FIELDS; i++)
	{
		if (KeyCovers(theNamespace, gFields[i].mKey))
			ResetField(theView, i);
	}
	if (KeyCovers(theNamespace, ADD_TYPES_KEY))
	{
		theView.mAddTypes = 0;
		theView.mAddTypesReset = true;
	}
}

void ParseKeyed(ModView& theView, int theField, const JsonValue& theValue, ParseLog& theLog)
{
	const FieldDef& aField = gFields[theField];
	std::string aKey = aField.mKey;
	theView.mSet[theField] = 1;
	if (theValue.IsNull())
	{
		ResetField(theView, theField);
		return;
	}
	if (!theValue.IsObject())
	{
		theLog.Error(aKey + " must be an object of " + (aField.mKind == KIND_SHAPES ? "shape id: pattern" : "pattern: points"));
		return;
	}
	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& anId = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string anEntryKey = aKey + "." + anId;
		if (aField.mKind == KIND_SHAPES)
		{
			std::map<std::string, ShapePattern>& aShapes = theView.mRules.mMatch.mShapes;
			if (!IsShapeId(anId))
			{
				theLog.Error(anEntryKey + ": a shape id is a letter and up to 31 letters, digits or _, and not runN, diagonal, L, T or plus");
				continue;
			}
			if (aValue.IsNull())
			{
				aShapes.erase(anId);
				theView.mShapeEntries[anId] = false;
				continue;
			}
			ShapePattern aShape;
			if (ParseShape(anEntryKey, aValue, &aShape, theLog))
			{
				aShapes[anId] = aShape;
				theView.mShapeEntries[anId] = true;
			}
			// Validate refuses more (a bound on FillMinimum's work); a mod.json of thousands stops here
			if (aShapes.size() > (size_t) GameRules::MAX_SHAPES)
			{
				theLog.Error(aKey + " has more than the " + IntText(GameRules::MAX_SHAPES) + " shapes a rule set may have");
				return;
			}
		}
		else
		{
			std::map<std::string, int>& aBonus = theView.mRules.mMatch.mBonus;
			if (!IsBonusName(anId))
			{
				theLog.Error(anEntryKey + ": bonus patterns are diagonal, L, T, plus or a shape id");
				continue;
			}
			int aPoints = 0;
			if (!aValue.IsNull() && (!ReadInt(aValue, &aPoints) || (aPoints < aField.mMin) || (aPoints > aField.mMax)))
			{
				theLog.Error(anEntryKey + " must be " + RangeText(aField.mMin, aField.mMax));
				continue;
			}
			// A bonus of 0 is the default: the same as a reset
			if (aPoints == 0)
				aBonus.erase(anId);
			else
				aBonus[anId] = aPoints;
			theView.mBonusEntries[anId] = aPoints != 0;
		}
	}
}

void WalkRules(ModView& theView, const JsonValue& theObject, const std::string& thePrefix, ParseLog& theLog,
	const std::vector<std::string>& theKnownKeys)
{
	for (size_t i = 0; i < theObject.Members().size(); i++)
	{
		const std::string& aName = theObject.Members()[i].first;
		const JsonValue& aValue = theObject.Members()[i].second;
		std::string aPath = thePrefix.empty() ? aName : thePrefix + "." + aName;
		if (aPath == ADD_TYPES_KEY)
		{
			if (aValue.IsNull())
			{
				theView.mAddTypes = 0;
				theView.mAddTypesReset = true;
			}
			else
				ParseNameSet(aPath, aValue, gMoveTypeNames, 3, &theView.mAddTypes, theLog);
			continue;
		}
		int aField = FindField(aPath);
		if (aField >= 0)
		{
			const FieldDef& aDef = gFields[aField];
			if (IsKeyed(aDef))
				ParseKeyed(theView, aField, aValue, theLog);
			else if (aValue.IsNull() && (aDef.mKind != KIND_TIME_LIMIT))
				ResetField(theView, aField);
			else
			{
				ParseStrict(aDef, aValue, theView.mRules, theLog);
				theView.mSet[aField] = 1;
			}
			continue;
		}
		if (IsNamespace(aPath))
		{
			if (aValue.IsNull())
				ResetNamespace(theView, aPath);
			else if (aValue.IsObject())
				WalkRules(theView, aValue, aPath, theLog, theKnownKeys);
			else
				theLog.Error(aPath + " must be an object (or null to reset it)");
			continue;
		}
		theLog.Warning(UnknownKey(aPath, theKnownKeys));
	}
}

// "tunables": the old generic flattening (ModManager::FlattenInto); only the legacy keys count, the rest
// is ignored without a word, as before
void WalkTunables(ModView& theView, const JsonValue& theValue, const std::string& thePrefix)
{
	if (theValue.IsObject())
	{
		for (size_t i = 0; i < theValue.Members().size(); i++)
		{
			const std::string& aName = theValue.Members()[i].first;
			WalkTunables(theView, theValue.Members()[i].second, thePrefix.empty() ? aName : thePrefix + "." + aName);
		}
		return;
	}
	int aField = thePrefix.empty() ? -1 : FindField(thePrefix);
	if ((aField < 0) || !(gFields[aField].mFlags & FLAG_TUNABLE))
		return;
	ParseLenient(gFields[aField], theValue, theView.mRules);
	theView.mSet[aField] = 1;
}

// The shape keys one mod set: board.mask gives cols/rows, board.levels excludes board.mask
void ResolveShape(ModView& theView, ParseLog& theLog)
{
	GameRules::BoardRules& aBoard = theView.mRules.mBoard;
	bool hasCols = theView.mSet[FieldIndex("board.cols")] != 0;
	bool hasRows = theView.mSet[FieldIndex("board.rows")] != 0;
	bool hasMask = (theView.mSet[FieldIndex("board.mask")] != 0) && !aBoard.mMask.empty();
	bool hasLevels = (theView.mSet[FieldIndex("board.levels")] != 0) && !aBoard.mLevels.empty();
	if (hasMask && hasLevels)
	{
		theLog.Error("board.mask and board.levels cannot both be set by one mod");
		return;
	}
	if (hasMask)
	{
		int aCols = (int) aBoard.mMask[0].size();
		int aRows = (int) aBoard.mMask.size();
		if ((aCols < 3) || (aRows < 3))
		{
			theLog.Error("board.mask must be at least 3x3");
			return;
		}
		if ((hasCols && (aBoard.mCols != aCols)) || (hasRows && (aBoard.mRows != aRows)))
		{
			theLog.Error("board.mask is " + IntText(aCols) + "x" + IntText(aRows) + ", not board.cols x board.rows " +
				IntText(aBoard.mCols) + "x" + IntText(aBoard.mRows));
			return;
		}
		// The size comes from the mask, so the mod sets it too (provenance)
		aBoard.mCols = aCols;
		aBoard.mRows = aRows;
		theView.mSet[FieldIndex("board.cols")] = 1;
		theView.mSet[FieldIndex("board.rows")] = 1;
		NormalizeMask(&aBoard.mMask);
	}
	if (hasLevels)
	{
		if (hasCols || hasRows)
		{
			// Ignored, so not this mod's values either (provenance)
			theLog.Warning("board.cols and board.rows are ignored with board.levels");
			aBoard.mCols = 8;
			aBoard.mRows = 8;
			theView.mSet[FieldIndex("board.cols")] = 0;
			theView.mSet[FieldIndex("board.rows")] = 0;
		}

		// One spelling per board (3.7): level L uses levels[L % n], so a list that repeats a shorter cycle
		// is that cycle, and a single shape is board.cols/rows/mask ([{8x8}] is the original board)
		std::vector<LevelShape>& aLevels = aBoard.mLevels;
		int n = (int) aLevels.size();
		int aPeriod = n;
		for (int p = 1; p < n; p++)
		{
			bool isPeriod = (n % p) == 0;
			for (int i = p; (i < n) && isPeriod; i++)
				isPeriod = (aLevels[i].mCols == aLevels[i % p].mCols) && (aLevels[i].mRows == aLevels[i % p].mRows) &&
					(aLevels[i].mMask == aLevels[i % p].mMask);
			if (isPeriod)
			{
				aPeriod = p;
				break;
			}
		}
		aLevels.resize(aPeriod);
		if (aPeriod == 1)
		{
			aBoard.mCols = aLevels[0].mCols;
			aBoard.mRows = aLevels[0].mRows;
			aBoard.mMask = aLevels[0].mMask;
			aLevels.clear();
			theView.mSet[FieldIndex("board.cols")] = 1;
			theView.mSet[FieldIndex("board.rows")] = 1;
			theView.mSet[FieldIndex("board.mask")] = !aBoard.mMask.empty();
			theView.mSet[FieldIndex("board.levels")] = 0;
		}
	}
}

// ---------------------------------------------------------------------------------------------------
// Merging

struct MergeResult
{
	GameRules							mRules;
	std::vector<std::set<int> >			mContributors;  // per field: the views its merged value came from
	std::set<int>						mTypeContributors[3]; // per moves.type bit: the views that put it there
	std::vector<RulesMessage>			mWarnings;
	std::map<std::string, std::vector<std::string> > mProvenance;

	MergeResult() : mContributors(NUM_FIELDS) {}
};

void MergeViews(const std::vector<ModView>& theViews, const std::vector<bool>& theActive, MergeResult& theResult)
{
	GameRules& aMerged = theResult.mRules;
	aMerged = OpenDefaults();
	int aCount = (int) theViews.size();

	// Atomic groups
	for (int aGroup = GROUP_NONE + 1; aGroup < NUM_GROUPS; aGroup++)
	{
		int aSupplier = -1;
		for (int i = aCount - 1; (i >= 0) && (aSupplier < 0); i--)
		{
			if (!theActive[i])
				continue;
			for (int f = 0; f < NUM_FIELDS; f++)
			{
				if ((gFields[f].mGroup == aGroup) && theViews[i].mSet[f])
				{
					aSupplier = i;
					break;
				}
			}
		}
		if (aSupplier < 0)
			continue;

		const ModView& aSource = theViews[aSupplier];
		for (int f = 0; f < NUM_FIELDS; f++)
		{
			if (gFields[f].mGroup != aGroup)
				continue;
			CopyField(gFields[f], aMerged, aSource.mRules);
			theResult.mContributors[f].insert(aSupplier);
			if (aSource.mSet[f])
				theResult.mProvenance[gFields[f].mKey] = {aSource.mId};
		}
		if (aGroup == GROUP_MOVES)
		{
			for (int b = 0; b < 3; b++)
			{
				if (aSource.mRules.mMoves.mTypes & (1 << b))
					theResult.mTypeContributors[b].insert(aSupplier);
			}
		}

		// A lower mod's group loses as a whole: name the first key where it differs
		for (int i = aSupplier - 1; i >= 0; i--)
		{
			if (!theActive[i])
				continue;
			bool isContributor = false;
			for (int f = 0; f < NUM_FIELDS; f++)
				isContributor = isContributor || ((gFields[f].mGroup == aGroup) && theViews[i].mSet[f]);
			if (!isContributor)
				continue;
			for (int f = 0; f < NUM_FIELDS; f++)
			{
				if ((gFields[f].mGroup == aGroup) && (EncodeField(gFields[f], theViews[i].mRules) != EncodeField(gFields[f], aSource.mRules)))
				{
					theResult.mWarnings.push_back({aSource.mId, std::string(gFields[f].mKey) + " overrides " + theViews[i].mId});
					break;
				}
			}
		}
	}

	// Leaves, and the keyed objects per id
	for (int f = 0; f < NUM_FIELDS; f++)
	{
		const FieldDef& aField = gFields[f];
		if (aField.mGroup != GROUP_NONE)
			continue;

		if (IsKeyed(aField))
		{
			// id -> the view that set it last; values come from that view
			std::map<std::string, int> anOwners;
			for (int i = 0; i < aCount; i++)
			{
				if (!theActive[i])
					continue;
				const ModView& aView = theViews[i];
				bool aReset = (aField.mKind == KIND_SHAPES) ? aView.mShapesReset : aView.mBonusReset;
				if (aReset && !anOwners.empty())
				{
					for (std::map<std::string, int>::iterator it = anOwners.begin(); it != anOwners.end(); ++it)
						theResult.mWarnings.push_back({aView.mId, std::string(aField.mKey) + " (reset) overrides " + theViews[it->second].mId});
					anOwners.clear();
				}
				const std::map<std::string, bool>& anEntries = (aField.mKind == KIND_SHAPES) ? aView.mShapeEntries : aView.mBonusEntries;
				for (std::map<std::string, bool>::const_iterator it = anEntries.begin(); it != anEntries.end(); ++it)
				{
					std::map<std::string, int>::iterator anOwner = anOwners.find(it->first);
					if (anOwner != anOwners.end())
					{
						bool aSame;
						if (aField.mKind == KIND_SHAPES)
						{
							GameRules a;
							GameRules b;
							std::map<std::string, ShapePattern>::const_iterator x = aView.mRules.mMatch.mShapes.find(it->first);
							std::map<std::string, ShapePattern>::const_iterator y = theViews[anOwner->second].mRules.mMatch.mShapes.find(it->first);
							if (x != aView.mRules.mMatch.mShapes.end())
								a.mMatch.mShapes[it->first] = x->second;
							if (y != theViews[anOwner->second].mRules.mMatch.mShapes.end())
								b.mMatch.mShapes[it->first] = y->second;
							aSame = EncodeField(aField, a) == EncodeField(aField, b);
						}
						else
						{
							std::map<std::string, int>::const_iterator x = aView.mRules.mMatch.mBonus.find(it->first);
							std::map<std::string, int>::const_iterator y = theViews[anOwner->second].mRules.mMatch.mBonus.find(it->first);
							int aValue = (x != aView.mRules.mMatch.mBonus.end()) ? x->second : 0;
							int anOther = (y != theViews[anOwner->second].mRules.mMatch.mBonus.end()) ? y->second : 0;
							aSame = aValue == anOther;
						}
						if (!aSame)
							theResult.mWarnings.push_back({aView.mId, std::string(aField.mKey) + "." + it->first + " overrides " + theViews[anOwner->second].mId});
					}
					if (it->second)
						anOwners[it->first] = i;
					else
						anOwners.erase(it->first);
				}
			}
			for (std::map<std::string, int>::iterator it = anOwners.begin(); it != anOwners.end(); ++it)
			{
				const GameRules& aSource = theViews[it->second].mRules;
				if (aField.mKind == KIND_SHAPES)
					aMerged.mMatch.mShapes[it->first] = aSource.mMatch.mShapes.find(it->first)->second;
				else
					aMerged.mMatch.mBonus[it->first] = aSource.mMatch.mBonus.find(it->first)->second;
				theResult.mContributors[f].insert(it->second);
				theResult.mProvenance[std::string(aField.mKey) + "." + it->first] = {theViews[it->second].mId};
			}
			continue;
		}

		int aWinner = -1;
		for (int i = aCount - 1; (i >= 0) && (aWinner < 0); i--)
		{
			if (theActive[i] && theViews[i].mSet[f])
				aWinner = i;
		}
		if (aWinner < 0)
			continue;
		CopyField(aField, aMerged, theViews[aWinner].mRules);
		theResult.mContributors[f].insert(aWinner);
		theResult.mProvenance[aField.mKey] = {theViews[aWinner].mId};
		for (int i = aWinner - 1; i >= 0; i--)
		{
			if (theActive[i] && theViews[i].mSet[f] && (EncodeField(aField, theViews[i].mRules) != EncodeField(aField, theViews[aWinner].mRules)))
				theResult.mWarnings.push_back({theViews[aWinner].mId, std::string(aField.mKey) + " overrides " + theViews[i].mId});
		}
	}

	// moves.addTypes: a union on top of the type the moves group resolved to; a reset (moves.addTypes:
	// null, moves: null) removes the lower mods' additions
	int anAdded = 0;
	std::vector<int> anAdders;
	for (int i = 0; i < aCount; i++)
	{
		if (!theActive[i])
			continue;
		if (theViews[i].mAddTypesReset)
		{
			anAdded = 0;
			anAdders.clear();
		}
		if (theViews[i].mAddTypes != 0)
		{
			anAdded |= theViews[i].mAddTypes;
			anAdders.push_back(i);
		}
	}
	if (anAdded != 0)
	{
		int aTypeField = FieldIndex("moves.type");
		aMerged.mMoves.mTypes |= anAdded;
		std::vector<std::string>& anIds = theResult.mProvenance[ADD_TYPES_KEY];
		for (size_t i = 0; i < anAdders.size(); i++)
		{
			theResult.mContributors[aTypeField].insert(anAdders[i]);
			for (int b = 0; b < 3; b++)
			{
				if (theViews[anAdders[i]].mAddTypes & (1 << b))
					theResult.mTypeContributors[b].insert(anAdders[i]);
			}
			anIds.push_back(theViews[anAdders[i]].mId);
		}
	}

	// The derived defaults (noMoves.normal from gold.enabled, match.diagonalMinRun from match.minRun) have no
	// contributors of their own: a derived value is the default, which never fails a check (Validate)
	ResolveDerived(aMerged);
}

// A merged bonus for a pattern the merged match rules can never form
void WarnUnusedBonus(const GameRules& theRules, MergeResult& theResult)
{
	for (std::map<std::string, int>::const_iterator it = theRules.mMatch.mBonus.begin(); it != theRules.mMatch.mBonus.end(); ++it)
	{
		bool canOccur = (it->first == "diagonal") ? theRules.mMatch.mDiagonal :
			((it->first == "L") || (it->first == "T") || (it->first == "plus")) ? theRules.mMatch.mMergeCrossings :
			(theRules.mMatch.mShapes.count(it->first) != 0);
		if (!canOccur)
			theResult.mWarnings.push_back({theResult.mProvenance["match.bonus." + it->first].front(),
				"match.bonus." + it->first + " has no effect: the merged match rules never form that pattern"});
	}
}

// What a failing check in GameRules::Validate reports, for MergeRules to drop the lowest-priority mod that
// supplied one of them: the values that make it fail. Those are the fields the check reads whose values
// differ from their defaults (the original rules pass every check, so a default value, a derived one
// included, is never the cause), the shape group only where the check would pass on the original 8x8 board,
// and for move types "moves.type=<type>": the mods that put that type into moves.type.
class CauseKeys
{
public:
	const GameRules&			mRules;
	GameRules					mDefaults;
	std::vector<std::string>	mKeys;

	explicit CauseKeys(const GameRules& theRules) : mRules(theRules), mDefaults(DefaultsFor(theRules)) {}

	void Field(const char* theKey)
	{
		const FieldDef& aField = gFields[FieldIndex(theKey)];
		if (EncodeField(aField, mRules) != EncodeField(aField, mDefaults))
			mKeys.push_back(theKey);
	}

	void Shape()
	{
		Field("board.cols");
		Field("board.rows");
		Field("board.mask");
		Field("board.levels");
	}

	void MoveType(int theType)
	{
		for (int b = 0; b < 3; b++)
		{
			if (theType == (1 << b))
				mKeys.push_back(std::string("moves.type=") + gMoveTypeNames[b]);
		}
	}
};

// theRules on the original full 8x8 board
GameRules OnOriginalBoard(const GameRules& theRules)
{
	GameRules aRules = theRules;
	aRules.mBoard.mCols = 8;
	aRules.mBoard.mRows = 8;
	aRules.mBoard.mMask.clear();
	aRules.mBoard.mLevels.clear();
	return aRules;
}

} // namespace

// ---------------------------------------------------------------------------------------------------
// GameRules

std::string GameRules::Canonical() const
{
	GameRules aDefaults = DefaultsFor(*this);
	ByteWriter w;
	w.U16(RULES_VERSION);
	for (int f = 0; f < NUM_FIELDS; f++)
	{
		std::string aValue = EncodeField(gFields[f], *this);
		if (aValue == EncodeField(gFields[f], aDefaults))
			continue;
		w.U16(gFields[f].mId);
		w.U8(gFields[f].mKind);
		w.mBytes += aValue;
	}
	w.U16(0);
	w.I32(mDiff.mNormalPointsBase);
	w.F64(mDiff.mNormalPointsPerLevel);
	w.F64(mDiff.mNormalMultBase);
	w.F64(mDiff.mNormalMultPerLevel);
	w.I32(mDiff.mTimedPointsBase);
	w.F64(mDiff.mTimedPointsPerLevel);
	w.F64(mDiff.mTimedMultBase);
	w.F64(mDiff.mTimedMultPerLevel);
	w.F64(mDiff.mTimeDrainBase);
	w.F64(mDiff.mTimeDrainPerLevel);
	w.F64(mDiff.mTimeDrainAccelBase);
	w.F64(mDiff.mTimeDrainAccelPerLevel);
	return w.mBytes;
}

bool GameRules::FromCanonical(const std::string& theBytes, std::string* theError)
{
	GameRules aRules = OpenDefaults();
	ByteReader r(theBytes);
	std::string anError;
	int aVersion = r.U16();
	if (aVersion != RULES_VERSION)
		anError = "rules version " + IntText(aVersion) + " is not " + IntText(RULES_VERSION);
	int aLastIndex = -1;
	while (anError.empty())
	{
		int anId = r.U16();
		if (r.mFailed)
		{
			anError = "truncated";
			break;
		}
		if (anId == 0)
			break;
		int aField = -1;
		for (int f = 0; f < NUM_FIELDS; f++)
		{
			if (gFields[f].mId == anId)
				aField = f;
		}
		if (aField < 0)
			anError = "unknown field id " + IntText(anId);
		else if (aField <= aLastIndex)
			anError = "field " + std::string(gFields[aField].mKey) + " out of order";
		else if (r.U8() != gFields[aField].mKind)
			anError = "field " + std::string(gFields[aField].mKey) + " has the wrong type";
		else if (!DecodeValue(gFields[aField], aRules, r))
			anError = "field " + std::string(gFields[aField].mKey) + " is malformed";
		aLastIndex = aField;
	}
	if (anError.empty())
	{
		aRules.mDiff.mNormalPointsBase = r.I32();
		aRules.mDiff.mNormalPointsPerLevel = r.F64();
		aRules.mDiff.mNormalMultBase = r.F64();
		aRules.mDiff.mNormalMultPerLevel = r.F64();
		aRules.mDiff.mTimedPointsBase = r.I32();
		aRules.mDiff.mTimedPointsPerLevel = r.F64();
		aRules.mDiff.mTimedMultBase = r.F64();
		aRules.mDiff.mTimedMultPerLevel = r.F64();
		aRules.mDiff.mTimeDrainBase = r.F64();
		aRules.mDiff.mTimeDrainPerLevel = r.F64();
		aRules.mDiff.mTimeDrainAccelBase = r.F64();
		aRules.mDiff.mTimeDrainAccelPerLevel = r.F64();
		if (r.mFailed)
			anError = "truncated diff.txt values";
		else if (!r.AtEnd())
			anError = "trailing bytes";
	}
	if (!anError.empty())
	{
		if (theError != NULL)
			*theError = anError;
		return false;
	}
	ResolveDerived(aRules);
	*this = aRules;
	return true;
}

void GameRules::RulesId(uint8_t theId[32]) const
{
	std::string aBytes = Canonical();
	Sha256::Hash(aBytes.data(), aBytes.size(), theId);
}

std::string GameRules::RulesIdHex() const
{
	return Sha256::HashHex(Canonical());
}

std::vector<std::string> GameRules::CanonicalLines() const
{
	GameRules aDefaults = DefaultsFor(*this);
	std::vector<std::string> aLines;
	aLines.push_back("rulesVersion = " + IntText(RULES_VERSION));
	for (int f = 0; f < NUM_FIELDS; f++)
	{
		if (EncodeField(gFields[f], *this) != EncodeField(gFields[f], aDefaults))
			aLines.push_back(std::string(gFields[f].mKey) + " = " + ValueJson(gFields[f], *this).Dump(0));
	}
	JsonValue aDiff = JsonValue::MakeArray();
	const double aValues[12] =
	{
		(double) mDiff.mNormalPointsBase, mDiff.mNormalPointsPerLevel, mDiff.mNormalMultBase, mDiff.mNormalMultPerLevel,
		(double) mDiff.mTimedPointsBase, mDiff.mTimedPointsPerLevel, mDiff.mTimedMultBase, mDiff.mTimedMultPerLevel,
		mDiff.mTimeDrainBase, mDiff.mTimeDrainPerLevel, mDiff.mTimeDrainAccelBase, mDiff.mTimeDrainAccelPerLevel
	};
	for (int i = 0; i < 12; i++)
		aDiff.Push(NumberJson(aValues[i]));
	aLines.push_back("diff.txt = " + aDiff.Dump(0));
	return aLines;
}

std::vector<std::string> GameRules::VariantKeys() const
{
	GameRules aDefaults = DefaultsFor(*this);
	std::vector<std::string> aKeys;
	for (int f = 0; f < NUM_FIELDS; f++)
	{
		if (!(gFields[f].mFlags & (FLAG_TUNABLE | FLAG_LOOK)) && (EncodeField(gFields[f], *this) != EncodeField(gFields[f], aDefaults)))
			aKeys.push_back(gFields[f].mKey);
	}
	return aKeys;
}

std::vector<std::string> GameRules::LookKeys() const
{
	GameRules aDefaults = DefaultsFor(*this);
	std::vector<std::string> aKeys;
	for (int f = 0; f < NUM_FIELDS; f++)
	{
		if ((gFields[f].mFlags & FLAG_LOOK) && (EncodeField(gFields[f], *this) != EncodeField(gFields[f], aDefaults)))
			aKeys.push_back(gFields[f].mKey);
	}
	return aKeys;
}

bool GameRules::HasVariantRules() const
{
	return !VariantKeys().empty();
}

bool GameRules::ScoringTunablesChanged() const
{
	TunableRules anOriginal;
	return (mTunables.mSetValues != anOriginal.mSetValues) || (mTunables.mLengthBonus != anOriginal.mLengthBonus) ||
		(mTunables.mLongLengthBonus != anOriginal.mLongLengthBonus) || (mTunables.mExcellentFactor != anOriginal.mExcellentFactor) ||
		(mTunables.mHintPenalty != anOriginal.mHintPenalty);
}

bool GameRules::PlaysAlikeInEveryEngine() const
{
	return !HasVariantRules() && !ScoringTunablesChanged() && (mBoard.mGemColors <= ORIGINAL_GEM_COLORS);
}

// arrows.every at level 1 (theLevel 0), a move fewer each level, never below arrows.everyMin (nor every, when
// everyMin is the larger: then every level takes every)
int GameRules::ArrowInterval(int theLevel) const
{
	int aMin = std::min(mArrows.mEveryMin, mArrows.mEvery);
	return std::max(aMin, mArrows.mEvery - theLevel);
}

bool GameRules::HasDirtRule() const
{
	for (int aLevel = 0; aLevel < ShapeCount(); aLevel++)
	{
		if (LayoutForLevel(aLevel).HasDirt())
			return true;
	}
	return false;
}

int GameRules::ShapeCount() const
{
	return mBoard.mLevels.empty() ? 1 : (int) mBoard.mLevels.size();
}

LevelShape GameRules::ShapeForLevel(int theLevel) const
{
	if (mBoard.mLevels.empty())
	{
		LevelShape aShape;
		aShape.mCols = mBoard.mCols;
		aShape.mRows = mBoard.mRows;
		aShape.mMask = mBoard.mMask;
		return aShape;
	}
	int n = (int) mBoard.mLevels.size();
	return mBoard.mLevels[((theLevel % n) + n) % n];
}

BoardLayout GameRules::LayoutForLevel(int theLevel) const
{
	LevelShape aShape = ShapeForLevel(theLevel);
	BoardLayout aLayout;
	aLayout.SetMask(aShape.mCols, aShape.mRows, aShape.mMask);
	return aLayout;
}

std::vector<std::vector<std::pair<int, int> > > GameRules::ShapeOrientations(const ShapePattern& theShape)
{
	bool aRotations;
	bool aMirror;
	CellList aCells = ShapeCells(theShape, &aRotations, &aMirror);
	std::set<CellList> anOrientations = OrientationSet(aCells, aRotations, aMirror);
	return std::vector<CellList>(anOrientations.begin(), anOrientations.end());
}

int GameRules::FillMinimum() const
{
	// Every pattern the reroll-until-no-match fill must avoid, as normalised cell offsets. A set: two patterns
	// with the same cells (a shape that repeats a run) are one instance at each placement, not two.
	std::set<CellList> aPatternSet;
	CellList aRow;
	CellList aCol;
	for (int i = 0; i < mMatch.mMinRun; i++)
	{
		aRow.push_back(std::make_pair(i, 0));
		aCol.push_back(std::make_pair(0, i));
	}
	aPatternSet.insert(NormalizeCells(aRow));
	aPatternSet.insert(NormalizeCells(aCol));
	if (mMatch.mDiagonal)
	{
		CellList aDown;
		CellList anUp;
		for (int i = 0; i < mMatch.mDiagonalMinRun; i++)
		{
			aDown.push_back(std::make_pair(i, i));
			anUp.push_back(std::make_pair(mMatch.mDiagonalMinRun - 1 - i, i));
		}
		aPatternSet.insert(NormalizeCells(aDown));
		aPatternSet.insert(NormalizeCells(anUp));
	}
	for (std::map<std::string, ShapePattern>::const_iterator it = mMatch.mShapes.begin(); it != mMatch.mShapes.end(); ++it)
	{
		bool aRotations;
		bool aMirror;
		CellList aCells = ShapeCells(it->second, &aRotations, &aMirror);
		std::set<CellList> anOrientations = OrientationSet(aCells, aRotations, aMirror);
		aPatternSet.insert(anOrientations.begin(), anOrientations.end());
	}
	std::vector<CellList> aPatterns(aPatternSet.begin(), aPatternSet.end());

	// The fill orders in use (NewBoardVariant): down fills bottom-up, up top-down, alternate both ways
	bool anOrders[2] = {mBoard.mGravity != GRAVITY_UP, mBoard.mGravity != GRAVITY_DOWN};
	int aMost = 0;
	for (int aPass = 0; aPass < ShapeCount() * 2; aPass++)
	{
		bool isTopDown = (aPass % 2) != 0;
		if (!anOrders[aPass % 2])
			continue;
		BoardLayout aLayout = LayoutForLevel(aPass / 2);
		// Fill order: rows from the entry side's far end, columns left to right, gem cells only
		int anOrder[BoardLayout::MAX_ROWS][BoardLayout::MAX_COLS];
		int aNext = 0;
		for (int r = BoardLayout::MAX_ROWS - 1; r >= 0; r--)
		{
			for (int c = 0; c < BoardLayout::MAX_COLS; c++)
				anOrder[r][c] = aLayout.IsPlayable(c, r) ? 0 : -1;
		}
		for (int i = 0; i < aLayout.mRows; i++)
		{
			int r = isTopDown ? i : aLayout.mRows - 1 - i;
			for (int c = 0; c < aLayout.mCols; c++)
			{
				if (anOrder[r][c] >= 0)
					anOrder[r][c] = aNext++;
			}
		}

		// A cell's count is the pattern instances whose other cells are all filled before it: the instances on gem
		// cells only, each counted at its cell that comes last in fill order. One pass over the placements (the
		// cells of a normalised pattern start at 0, so a placement starts on the board), not one per cell.
		int aCounts[BoardLayout::MAX_ROWS][BoardLayout::MAX_COLS] = {};
		for (size_t p = 0; p < aPatterns.size(); p++)
		{
			const CellList& aPattern = aPatterns[p];
			for (int aY = 0; aY < aLayout.mRows; aY++)
			{
				for (int anX = 0; anX < aLayout.mCols; anX++)
				{
					int aLastX = -1;
					int aLastY = -1;
					for (size_t q = 0; q < aPattern.size(); q++)
					{
						int x = anX + aPattern[q].first;
						int y = aY + aPattern[q].second;
						if (!aLayout.IsPlayable(x, y))
						{
							aLastX = -1;
							break;
						}
						if ((aLastX < 0) || (anOrder[y][x] > anOrder[aLastY][aLastX]))
						{
							aLastX = x;
							aLastY = y;
						}
					}
					if (aLastX >= 0)
						aCounts[aLastY][aLastX]++;
				}
			}
		}
		for (int r = 0; r < aLayout.mRows; r++)
		{
			for (int c = 0; c < aLayout.mCols; c++)
			{
				if (anOrder[r][c] >= 0)
					aMost = std::max(aMost, aCounts[r][c]);
			}
		}
	}
	return aMost + 1;
}

bool GameRules::ValidateShape(std::string* theMessage) const
{
	std::string aMessage;
	// The mod path takes at most MAX_LEVELS (ParseStrict); rules decoded from a file can say more
	if ((int) mBoard.mLevels.size() > MAX_LEVELS)
		aMessage = "board.levels has " + IntText((int) mBoard.mLevels.size()) + " shapes, more than " + IntText(MAX_LEVELS);
	for (int aLevel = 0; (aLevel < ShapeCount()) && aMessage.empty(); aLevel++)
	{
		LevelShape aShape = ShapeForLevel(aLevel);
		std::string aWhich = mBoard.mLevels.empty() ? "the board" : "board.levels[" + IntText(aLevel) + "]";
		if ((aShape.mCols < 3) || (aShape.mCols > BoardLayout::MAX_COLS) || (aShape.mRows < 3) || (aShape.mRows > BoardLayout::MAX_ROWS))
			aMessage = aWhich + " must be 3..16 cells wide and high";
		else if (!aShape.mMask.empty() && (!BoardLayout::MaskIsWellFormed(aShape.mMask) ||
			((int) aShape.mMask.size() != aShape.mRows) || ((int) aShape.mMask[0].size() != aShape.mCols)))
			aMessage = aWhich + ": the mask does not match the board size";
		else if (LayoutForLevel(aLevel).CountCells() < 9)
			aMessage = aWhich + " has " + IntText(LayoutForLevel(aLevel).CountCells()) + " cells; at least 9 are needed";
	}
	if (aMessage.empty())
		return true;
	if (theMessage != NULL)
		*theMessage = aMessage;
	return false;
}

bool GameRules::Validate(std::string* theMessage, std::vector<std::string>* theKeys) const
{
	CauseKeys aCause(*this);
	std::string aMessage;
	if (!ValidateShape(&aMessage))
		aCause.Shape();

	int aLongest = 0;
	for (int aLevel = 0; (aLevel < ShapeCount()) && aMessage.empty(); aLevel++)
	{
		BoardLayout aLayout = LayoutForLevel(aLevel);
		std::string aWhich = mBoard.mLevels.empty() ? "the board" : "board.levels[" + IntText(aLevel) + "]";
		// The original board has lines of 8 and 2x2 blocks, so in these three the shape is always a cause
		if (aLayout.LongestLine() < mMatch.mMinRun)
		{
			aMessage = aWhich + " has no line of match.minRun (" + IntText(mMatch.mMinRun) + ") cells";
			aCause.Shape();
			aCause.Field("match.minRun");
		}
		else if ((mMoves.mTypes & MOVE_TWIST) && !aLayout.HasFullSquare())
		{
			aMessage = "moves.type twist needs a 2x2 block of cells, which " + aWhich + " lacks";
			aCause.Shape();
			aCause.MoveType(MOVE_TWIST);
		}
		else if ((mMoves.mTypes & MOVE_SLIDE) && (aLayout.LongestLine() < 2))
		{
			aMessage = "moves.type slide needs two cells in a row, which " + aWhich + " lacks";
			aCause.Shape();
			aCause.MoveType(MOVE_SLIDE);
		}
		aLongest = std::max(aLongest, aLayout.LongestLine());
	}

	// FillMinimum's work grows with the shapes (8 orientations each) times the levels: a bound for any input
	if (aMessage.empty() && ((int) mMatch.mShapes.size() > MAX_SHAPES))
	{
		aMessage = "match.shapes has " + IntText((int) mMatch.mShapes.size()) + " shapes, more than the " + IntText(MAX_SHAPES) +
			" a rule set may have";
		aCause.Field("match.shapes");
	}

	if (aMessage.empty())
	{
		int aMinimum = FillMinimum();
		if (mBoard.mGemColors < aMinimum)
		{
			aMessage = "board.gemColors " + IntText(mBoard.mGemColors) + " is below " + IntText(aMinimum) +
				", the fewest colours a board can be filled with under these match rules";
			// What FillMinimum reads
			aCause.Field("board.gemColors");
			aCause.Field("match.minRun");
			if (mMatch.mDiagonal)
			{
				aCause.Field("match.diagonal");
				aCause.Field("match.diagonalMinRun");
			}
			aCause.Field("match.shapes");
			if (OnOriginalBoard(*this).FillMinimum() <= mBoard.mGemColors)
				aCause.Shape();
			// The upward fill order, where the downward one alone would pass
			GameRules aFallingDown = *this;
			aFallingDown.mBoard.mGravity = GRAVITY_DOWN;
			if (aFallingDown.FillMinimum() <= mBoard.mGemColors)
				aCause.Field("board.gravity.direction");
		}
	}

	const SpecialRule* aSpecials[2] = {&mSpecials.mFlame, &mSpecials.mLightning};
	const char* aSpecialKeys[2] = {"specials.flame", "specials.lightning"};
	for (int i = 0; (i < 2) && aMessage.empty(); i++)
	{
		if (!aSpecials[i]->mEnabled)
			continue;
		for (size_t j = 0; (j < aSpecials[i]->mCreatedBy.size()) && aMessage.empty(); j++)
		{
			const std::string& aName = aSpecials[i]->mCreatedBy[j];
			int aRun = RunLength(aName);
			bool canOccur = (aRun > 0) ? (std::max(aRun, mMatch.mMinRun) <= aLongest) :
				(aName == "diagonal") ? mMatch.mDiagonal :
				((aName == "L") || (aName == "T") || (aName == "plus")) ? mMatch.mMergeCrossings :
				(mMatch.mShapes.count(aName) != 0);
			if (!canOccur)
			{
				aMessage = std::string(aSpecialKeys[i]) + ".createdBy: the pattern " + aName + " cannot occur under the match rules";
				aCause.Field(aSpecialKeys[i]);
				// A run longer than every line: the shape is a cause when the original board's lines of 8 fit
				// it (match.minRun never is: the line check above makes every level hold a line of minRun).
				// The other names fail on match rules at their defaults (no diagonals, crossings "separate",
				// no such shape id), so only the trigger is the cause.
				if ((aRun > 0) && (aRun <= 8))
					aCause.Shape();
			}
		}
	}
	if (aMessage.empty() && mSpecials.mFlame.mEnabled && mSpecials.mLightning.mEnabled &&
		(mSpecials.mFlame.mCreatedBy == mSpecials.mLightning.mCreatedBy))
	{
		aMessage = "specials.flame and specials.lightning are created by the same patterns";
		aCause.Field("specials.flame");
		aCause.Field("specials.lightning");
	}

	if (aMessage.empty() && ((mNoMoves.mNormal == NOMOVES_CONTINUE) || (mNoMoves.mTimed == NOMOVES_CONTINUE)) && !mMoves.mAllowInvalid)
	{
		// moves.allowInvalid is false here, its default: only the modes that hold "continue"
		aMessage = "noMoves \"continue\" needs moves.allowInvalid";
		if (mNoMoves.mNormal == NOMOVES_CONTINUE)
			aCause.Field("noMoves.normal");
		if (mNoMoves.mTimed == NOMOVES_CONTINUE)
			aCause.Field("noMoves.timed");
	}

	if (aMessage.empty())
		return true;
	if (theMessage != NULL)
		*theMessage = aMessage;
	if (theKeys != NULL)
		*theKeys = aCause.mKeys;
	return false;
}

// theValue at a dotted key path of theObject ("gold.timeLimit.normal"), making the objects on the way
static void SetJsonPath(JsonValue& theObject, const std::string& thePath, const JsonValue& theValue)
{
	size_t aDot = thePath.find('.');
	if (aDot == std::string::npos)
	{
		theObject.Set(thePath, theValue);
		return;
	}
	std::string aHead = thePath.substr(0, aDot);
	JsonValue aChild = (theObject.Has(aHead) && theObject[aHead].IsObject()) ? theObject[aHead] : JsonValue::MakeObject();
	SetJsonPath(aChild, thePath.substr(aDot + 1), theValue);
	theObject.Set(aHead, aChild);
}

bool GameRules::CheckRecorded(std::string* theError) const
{
	std::string anError;
	// The diff.txt values. SetupLevelData: mPointMultiplier = multPerLevel * L + multBase, mLevelPoints =
	// (int)((pointsPerLevel * L + pointsBase) * mPointMultiplier), and Update divides by mLevelPoints. With both
	// bases above 0, their product at least 1 and both steps at least 0, every level L >= 0 gets at least 1 (the
	// same doubles at L = 0, larger ones after; past the int range the conversion gives INT_MIN, never 0)
	const DiffValues& d = mDiff;
	const double aValues[12] =
	{
		(double) d.mNormalPointsBase, d.mNormalPointsPerLevel, d.mNormalMultBase, d.mNormalMultPerLevel,
		(double) d.mTimedPointsBase, d.mTimedPointsPerLevel, d.mTimedMultBase, d.mTimedMultPerLevel,
		d.mTimeDrainBase, d.mTimeDrainPerLevel, d.mTimeDrainAccelBase, d.mTimeDrainAccelPerLevel
	};
	for (int i = 0; (i < 12) && anError.empty(); i++)
	{
		if (!std::isfinite(aValues[i]))
			anError = "diff.txt value " + IntText(i + 1) + " is not a finite number";
	}
	for (int aMode = 0; (aMode < 2) && anError.empty(); aMode++)
	{
		int aBase = (aMode == 0) ? d.mNormalPointsBase : d.mTimedPointsBase;
		double aPerLevel = (aMode == 0) ? d.mNormalPointsPerLevel : d.mTimedPointsPerLevel;
		double aMultBase = (aMode == 0) ? d.mNormalMultBase : d.mTimedMultBase;
		double aMultPerLevel = (aMode == 0) ? d.mNormalMultPerLevel : d.mTimedMultPerLevel;
		double aLevelZero = (aPerLevel * 0.0 + aBase) * (aMultPerLevel * 0.0 + aMultBase); // SetupLevelData's doubles
		if ((aBase < 1) || !(aPerLevel >= 0.0) || !(aMultBase > 0.0) || !(aMultPerLevel >= 0.0) || !(aLevelZero >= 1.0))
			anError = std::string("diff.txt's ") + ((aMode == 0) ? "normal" : "timed") + " points and multiplier make a level of no points " +
				"(the bases must be above 0 with a product of at least 1, the steps per level at least 0)";
	}

	// What the mod path refuses anyway, before the work grows with it (a RULE can hold 65,535 of each)
	if (anError.empty() && ((int) mMatch.mShapes.size() > MAX_SHAPES))
		anError = "match.shapes has " + IntText((int) mMatch.mShapes.size()) + " shapes, more than " + IntText(MAX_SHAPES);
	if (anError.empty() && ((int) mBoard.mLevels.size() > MAX_LEVELS))
		anError = "board.levels has " + IntText((int) mBoard.mLevels.size()) + " shapes, more than " + IntText(MAX_LEVELS);

	// The rules: a mod.json that says every field this set holds off its default (the tunables under "tunables",
	// read as leniently as the game reads them; the rest under "rules"), through the mod path
	GameRules aDefaults = DefaultsFor(*this);
	JsonValue aTunables = JsonValue::MakeObject();
	JsonValue aRules = JsonValue::MakeObject();
	for (int f = 0; (f < NUM_FIELDS) && anError.empty(); f++)
	{
		const FieldDef& aField = gFields[f];
		if (EncodeField(aField, *this) == EncodeField(aField, aDefaults))
			continue;
		JsonValue aValue;
		if (aField.mKind == KIND_DOUBLE)
		{
			double aDouble = FieldRef<double>(aField, *this);
			if (!std::isfinite(aDouble))
			{
				anError = std::string(aField.mKey) + " is not a finite number";
				break;
			}
			aValue = JsonValue(aDouble); // its bits as they are
		}
		else
			aValue = ValueJson(aField, *this);
		SetJsonPath((aField.mFlags & FLAG_TUNABLE) ? aTunables : aRules, aField.mKey, aValue);
	}
	if (anError.empty())
	{
		RulesSource aSource;
		aSource.mModId = "RULE";
		aSource.mTunables = aTunables;
		if (aRules.Size() > 0)
			aSource.mRules = aRules;
		GameRules aMerged;
		RulesStatus aStatus;
		MergeRules(std::vector<RulesSource>(1, aSource), &aMerged, &aStatus);
		aMerged.mDiff = mDiff;
		if (!aStatus.mErrors.empty())
			anError = aStatus.mErrors[0].mText;
		else if (!aStatus.mDropped.empty())
			anError = "rules a game cannot play: " + aStatus.mDropped[0].mText;
		else if (aMerged.Canonical() != Canonical())
			anError = "rules no mod makes (a value out of its range or not in its one spelling)";
	}

	if (anError.empty())
		return true;
	if (theError != NULL)
		*theError = anError;
	return false;
}

std::string GameRules::MoveTypesText(int theTypes)
{
	std::string aText;
	for (int i = 0; i < 3; i++)
	{
		if (theTypes & (1 << i))
			aText += std::string(aText.empty() ? "" : "+") + gMoveTypeNames[i];
	}
	return aText;
}

// ---------------------------------------------------------------------------------------------------
// MergeRules

void Sexy::MergeRules(const std::vector<RulesSource>& theSources, GameRules* theRules, RulesStatus* theStatus)
{
	std::vector<std::string> aKnownKeys = KnownKeys();
	std::vector<ModView> aViews(theSources.size());
	for (size_t i = 0; i < theSources.size(); i++)
	{
		const RulesSource& aSource = theSources[i];
		ModView& aView = aViews[i];
		aView.mId = aSource.mModId;
		WalkTunables(aView, aSource.mTunables, "");
		if (aSource.mRules.IsNull() && aSource.mRulesError.empty())
			continue;

		ModView aTunablesOnly = aView;
		ParseLog aLog;
		if (!aSource.mRulesError.empty())
			aLog.Error(aSource.mRulesError);
		else if (!aSource.mRules.IsObject())
			aLog.Error("\"rules\" must be an object");
		else
		{
			WalkRules(aView, aSource.mRules, "", aLog, aKnownKeys);
			if (aLog.mErrors.empty())
				ResolveShape(aView, aLog);
		}
		// Per mod (3.5 rule 7): type and range checks (above) and the checks of the shape group, which one
		// mod supplies whole. The cross-field checks run on the merged set below, so a mod may rely on
		// another one (power-gems' T and L triggers on a mod's "crossings": "merge").
		std::string aMessage;
		if (aLog.mErrors.empty() && !aView.mRules.ValidateShape(&aMessage))
			aLog.Error(aMessage);
		for (size_t j = 0; j < aLog.mWarnings.size(); j++)
			theStatus->mWarnings.push_back({aView.mId, aLog.mWarnings[j]});
		if (aLog.mErrors.empty())
			aView.mHasRules = true;
		else
		{
			std::string aText = "rules ignored: " + aLog.mErrors[0];
			if (aLog.mErrors.size() > 1)
				aText += " (and " + IntText((int) aLog.mErrors.size() - 1) + " more)";
			theStatus->mErrors.push_back({aView.mId, aText});
			aView = aTunablesOnly;
		}
	}

	// The merged set: drop the lowest-priority mod contributing to a failing check, then merge again
	std::vector<bool> anActive(aViews.size(), true);
	MergeResult aResult;
	for (;;)
	{
		aResult = MergeResult();
		MergeViews(aViews, anActive, aResult);
		std::string aMessage;
		std::vector<std::string> aKeys;
		if (aResult.mRules.Validate(&aMessage, &aKeys))
			break;

		// The mods that supplied a value the check names as a cause (CauseKeys)
		std::set<int> aCauses;
		for (size_t k = 0; k < aKeys.size(); k++)
		{
			size_t anEquals = aKeys[k].find('=');
			if (anEquals != std::string::npos)
			{
				for (int b = 0; b < 3; b++)
				{
					if (aKeys[k].compare(anEquals + 1, std::string::npos, gMoveTypeNames[b]) == 0)
						aCauses.insert(aResult.mTypeContributors[b].begin(), aResult.mTypeContributors[b].end());
				}
				continue;
			}
			for (int f = 0; f < NUM_FIELDS; f++)
			{
				if (KeyCovers(aKeys[k], gFields[f].mKey))
					aCauses.insert(aResult.mContributors[f].begin(), aResult.mContributors[f].end());
			}
		}
		int aLowest = -1;
		for (std::set<int>::const_iterator it = aCauses.begin(); (it != aCauses.end()) && (aLowest < 0); ++it)
		{
			if (anActive[*it])
				aLowest = *it;
		}
		if (aLowest < 0)
		{
			// Cannot happen: the defaults pass every check
			theStatus->mErrors.push_back({"", "merged rules fail (" + aMessage + "); using the original rules"});
			aResult = MergeResult();
			aResult.mRules = GameRules();
			break;
		}
		anActive[aLowest] = false;
		theStatus->mDropped.push_back({aViews[aLowest].mId, aMessage});
	}

	WarnUnusedBonus(aResult.mRules, aResult);
	theStatus->mWarnings.insert(theStatus->mWarnings.end(), aResult.mWarnings.begin(), aResult.mWarnings.end());
	theStatus->mProvenance.insert(aResult.mProvenance.begin(), aResult.mProvenance.end());
	DiffValues aDiff = theRules->mDiff;
	*theRules = aResult.mRules;
	theRules->mDiff = aDiff;
}

// ---------------------------------------------------------------------------------------------------
// Features

std::string GameFeatures::Text() const
{
	std::vector<std::string> aParts;
	if (mSeeds)
	{
		std::string aText = "seeds(";
		aText += mSeedEntry ? "entry" : "";
		aText += (mSeedEntry && mSeedPerGame) ? ", " : "";
		aText += mSeedPerGame ? "perGame" : "";
		aParts.push_back(aText + ")");
	}
	if (mReplays)
		aParts.push_back("replays(keep " + IntText(mReplayKeep) + (mReplayHover ? ", hover)" : ")"));
	if (mLeaderboard)
		aParts.push_back(std::string("leaderboard(") + (mRankedAlways ? "always" : "ask") + ")");
	if (mLiveSplit)
		aParts.push_back("livesplit");
	if (mCodeArt)
		aParts.push_back("codeArt(gems " + mCodeArtGems + ", palette " + mCodeArtPalette + ")");
	if (mGameStats)
		aParts.push_back("gameStats");
	if (mLean)
		aParts.push_back(std::string("lean(") + (mLeanImages ? "images, " : "") + "frameRate " + IntText(mLeanFrameRate) + ")");
	if (mFixes)
		aParts.push_back(std::string("fixes(") + (mFixSelection ? "selection" : "") + ((mFixSelection && mFixResumeLook) ? ", " : "") +
			(mFixResumeLook ? "resumeLook" : "") + ")");
	std::string aText;
	for (size_t i = 0; i < aParts.size(); i++)
		aText += (i ? ", " : "") + aParts[i];
	return aText.empty() ? "none" : aText;
}

namespace
{

enum { FEATURE_SEEDS, FEATURE_REPLAYS, FEATURE_LEADERBOARD, FEATURE_LIVESPLIT, FEATURE_CODEART, FEATURE_GAMESTATS, FEATURE_LEAN,
	FEATURE_FIXES, NUM_FEATURES };
const char* const gFeatureNames[NUM_FEATURES] = {"seeds", "replays", "leaderboard", "livesplit", "codeArt", "gameStats", "lean", "fixes"};
const char* const gCodeArtGems[] = {"cel", "glossy", "su"};
const char* const gCodeArtPalettes[] = {"blue", "violet", "emerald", "ember", "night"};

// One feature's value: false / null (off), true (on with its defaults) or its options object
bool ParseFeature(int theFeature, const JsonValue& theValue, GameFeatures* theOut, ParseLog& theLog)
{
	std::string aKey = std::string("features.") + gFeatureNames[theFeature];
	bool anOn = theValue.IsObject() || (theValue.IsBool() && theValue.AsBool());
	if (!theValue.IsNull() && !theValue.IsBool() && !theValue.IsObject())
		return theLog.Error(aKey + " must be true, false, null or an object");
	switch (theFeature)
	{
	case FEATURE_SEEDS:
		// true turns on both; the object form only the members it sets to true, so {"entry": true} is the
		// seed field alone: perGame changes the RNG of every game
		// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2)
		theOut->mSeeds = anOn;
		theOut->mSeedEntry = anOn && theValue.IsBool();
		theOut->mSeedPerGame = anOn && theValue.IsBool();
		break;
	case FEATURE_REPLAYS:
		theOut->mReplays = anOn;
		theOut->mReplayKeep = 20;
		theOut->mReplayHover = false;
		break;
	case FEATURE_LEADERBOARD:
		theOut->mLeaderboard = anOn;
		theOut->mRankedAlways = false;
		break;
	case FEATURE_LIVESPLIT:
		// no options yet: an object turns it on like true, and each of its members is an unknown key
		theOut->mLiveSplit = anOn;
		break;
	case FEATURE_CODEART:
		theOut->mCodeArt = anOn;
		theOut->mCodeArtGems = "cel";
		theOut->mCodeArtPalette = "blue";
		break;
	case FEATURE_GAMESTATS:
		// no options: an object turns it on like true, and each of its members is an unknown key
		theOut->mGameStats = anOn;
		break;
	case FEATURE_LEAN:
		theOut->mLean = anOn;
		theOut->mLeanImages = true;
		theOut->mLeanFrameRate = 0;
		break;
	case FEATURE_FIXES:
		// every fix on unless the object turns it off
		theOut->mFixes = anOn;
		theOut->mFixSelection = true;
		theOut->mFixResumeLook = true;
		break;
	}
	if (!theValue.IsObject())
		return true;

	for (size_t i = 0; i < theValue.Members().size(); i++)
	{
		const std::string& aName = theValue.Members()[i].first;
		const JsonValue& aValue = theValue.Members()[i].second;
		std::string aSubKey = aKey + "." + aName;
		bool* aFlag = NULL;
		if ((theFeature == FEATURE_SEEDS) && (aName == "entry"))
			aFlag = &theOut->mSeedEntry;
		else if ((theFeature == FEATURE_SEEDS) && (aName == "perGame"))
			aFlag = &theOut->mSeedPerGame;
		else if ((theFeature == FEATURE_REPLAYS) && (aName == "hover"))
			aFlag = &theOut->mReplayHover;
		else if ((theFeature == FEATURE_LEAN) && (aName == "images"))
			aFlag = &theOut->mLeanImages;
		else if ((theFeature == FEATURE_FIXES) && (aName == "selection"))
			aFlag = &theOut->mFixSelection;
		else if ((theFeature == FEATURE_FIXES) && (aName == "resumeLook"))
			aFlag = &theOut->mFixResumeLook;
		if (aFlag != NULL)
		{
			if (!aValue.IsBool())
				return theLog.Error(aSubKey + " must be true or false");
			*aFlag = aValue.AsBool();
		}
		else if ((theFeature == FEATURE_REPLAYS) && (aName == "keep"))
		{
			if (!ReadInt(aValue, &theOut->mReplayKeep) || (theOut->mReplayKeep < 0) || (theOut->mReplayKeep > 1000))
				return theLog.Error(aSubKey + " must be an integer 0..1000");
		}
		else if ((theFeature == FEATURE_LEAN) && (aName == "frameRate"))
		{
			int aRate = 0;
			if (!ReadInt(aValue, &aRate) || ((aRate != 0) && ((aRate < 10) || (aRate > 100))))
				return theLog.Error(aSubKey + " must be 0 (every frame) or an integer 10..100");
			theOut->mLeanFrameRate = aRate;
		}
		else if ((theFeature == FEATURE_LEADERBOARD) && (aName == "ranked"))
		{
			if (!aValue.IsString() || ((aValue.AsString() != "ask") && (aValue.AsString() != "always")))
				return theLog.Error(aSubKey + " must be \"ask\" or \"always\"");
			theOut->mRankedAlways = aValue.AsString() == "always";
		}
		else if ((theFeature == FEATURE_CODEART) && ((aName == "gems") || (aName == "palette")))
		{
			bool isGems = aName == "gems";
			const char* const* aNames = isGems ? gCodeArtGems : gCodeArtPalettes;
			int aCount = isGems ? 3 : 5;
			std::string aList;
			bool isKnown = false;
			for (int k = 0; k < aCount; k++)
			{
				aList += std::string(k ? ", " : "") + "\"" + aNames[k] + "\"";
				isKnown |= aValue.IsString() && (aValue.AsString() == aNames[k]);
			}
			if (!isKnown)
				return theLog.Error(aSubKey + " must be one of " + aList);
			(isGems ? theOut->mCodeArtGems : theOut->mCodeArtPalette) = aValue.AsString();
		}
		else
		{
			std::vector<std::string> aKnown;
			if (theFeature == FEATURE_SEEDS)
				aKnown = {aKey + ".entry", aKey + ".perGame"};
			else if (theFeature == FEATURE_REPLAYS)
				aKnown = {aKey + ".keep", aKey + ".hover"};
			else if (theFeature == FEATURE_LEADERBOARD)
				aKnown = {aKey + ".ranked"};
			else if (theFeature == FEATURE_CODEART)
				aKnown = {aKey + ".gems", aKey + ".palette"};
			else if (theFeature == FEATURE_LEAN)
				aKnown = {aKey + ".images", aKey + ".frameRate"};
			else if (theFeature == FEATURE_FIXES)
				aKnown = {aKey + ".selection", aKey + ".resumeLook"};
			theLog.Warning(UnknownKey(aSubKey, aKnown));
		}
	}
	if ((theFeature == FEATURE_SEEDS) && !theOut->mSeedEntry && !theOut->mSeedPerGame)
		theOut->mSeeds = false;
	return true;
}

} // namespace

void Sexy::MergeFeatures(const std::vector<RulesSource>& theSources, GameFeatures* theFeatures, RulesStatus* theStatus)
{
	std::vector<std::string> aKnown;
	for (int i = 0; i < NUM_FEATURES; i++)
		aKnown.push_back(std::string("features.") + gFeatureNames[i]);

	// Per feature, the highest-priority mod that sets it supplies all of it
	GameFeatures aMerged;
	std::string aSetter[NUM_FEATURES];
	std::string aSetterText[NUM_FEATURES];
	for (size_t i = 0; i < theSources.size(); i++)
	{
		const RulesSource& aSource = theSources[i];
		if (aSource.mFeatures.IsNull())
			continue;
		ParseLog aLog;
		GameFeatures aParsed;
		bool aSet[NUM_FEATURES] = {};
		if (!aSource.mFeatures.IsObject())
			aLog.Error("\"features\" must be an object");
		else
		{
			for (size_t j = 0; j < aSource.mFeatures.Members().size(); j++)
			{
				const std::string& aName = aSource.mFeatures.Members()[j].first;
				int aFeature = -1;
				for (int k = 0; k < NUM_FEATURES; k++)
				{
					if (aName == gFeatureNames[k])
						aFeature = k;
				}
				if (aFeature < 0)
				{
					aLog.Warning(UnknownKey("features." + aName, aKnown));
					continue;
				}
				if (ParseFeature(aFeature, aSource.mFeatures.Members()[j].second, &aParsed, aLog))
					aSet[aFeature] = true;
			}
		}
		for (size_t j = 0; j < aLog.mWarnings.size(); j++)
			theStatus->mWarnings.push_back({aSource.mModId, aLog.mWarnings[j]});
		if (!aLog.mErrors.empty())
		{
			theStatus->mErrors.push_back({aSource.mModId, "features ignored: " + aLog.mErrors[0]});
			continue;
		}
		for (int k = 0; k < NUM_FEATURES; k++)
		{
			if (!aSet[k])
				continue;
			if (k == FEATURE_SEEDS)
			{
				aMerged.mSeeds = aParsed.mSeeds;
				aMerged.mSeedEntry = aParsed.mSeedEntry;
				aMerged.mSeedPerGame = aParsed.mSeedPerGame;
			}
			else if (k == FEATURE_REPLAYS)
			{
				aMerged.mReplays = aParsed.mReplays;
				aMerged.mReplayKeep = aParsed.mReplayKeep;
				aMerged.mReplayHover = aParsed.mReplayHover;
			}
			else if (k == FEATURE_LEADERBOARD)
			{
				aMerged.mLeaderboard = aParsed.mLeaderboard;
				aMerged.mRankedAlways = aParsed.mRankedAlways;
			}
			else if (k == FEATURE_LIVESPLIT)
			{
				aMerged.mLiveSplit = aParsed.mLiveSplit;
				aMerged.mLiveSplitMod = aParsed.mLiveSplit ? aSource.mModId : std::string();
			}
			else if (k == FEATURE_CODEART)
			{
				aMerged.mCodeArt = aParsed.mCodeArt;
				aMerged.mCodeArtGems = aParsed.mCodeArtGems;
				aMerged.mCodeArtPalette = aParsed.mCodeArtPalette;
			}
			else if (k == FEATURE_GAMESTATS)
				aMerged.mGameStats = aParsed.mGameStats;
			else if (k == FEATURE_FIXES)
			{
				aMerged.mFixes = aParsed.mFixes;
				aMerged.mFixSelection = aParsed.mFixSelection;
				aMerged.mFixResumeLook = aParsed.mFixResumeLook;
			}
			else
			{
				aMerged.mLean = aParsed.mLean;
				aMerged.mLeanImages = aParsed.mLeanImages;
				aMerged.mLeanFrameRate = aParsed.mLeanFrameRate;
			}
			GameFeatures aText;
			if (k == FEATURE_SEEDS)
			{
				aText.mSeeds = aParsed.mSeeds;
				aText.mSeedEntry = aParsed.mSeedEntry;
				aText.mSeedPerGame = aParsed.mSeedPerGame;
			}
			else if (k == FEATURE_REPLAYS)
			{
				aText.mReplays = aParsed.mReplays;
				aText.mReplayKeep = aParsed.mReplayKeep;
				aText.mReplayHover = aParsed.mReplayHover;
			}
			else if (k == FEATURE_LEADERBOARD)
			{
				aText.mLeaderboard = aParsed.mLeaderboard;
				aText.mRankedAlways = aParsed.mRankedAlways;
			}
			else if (k == FEATURE_LIVESPLIT)
				aText.mLiveSplit = aParsed.mLiveSplit;
			else if (k == FEATURE_CODEART)
			{
				aText.mCodeArt = aParsed.mCodeArt;
				aText.mCodeArtGems = aParsed.mCodeArtGems;
				aText.mCodeArtPalette = aParsed.mCodeArtPalette;
			}
			else if (k == FEATURE_GAMESTATS)
				aText.mGameStats = aParsed.mGameStats;
			else if (k == FEATURE_FIXES)
			{
				aText.mFixes = aParsed.mFixes;
				aText.mFixSelection = aParsed.mFixSelection;
				aText.mFixResumeLook = aParsed.mFixResumeLook;
			}
			else
			{
				aText.mLean = aParsed.mLean;
				aText.mLeanImages = aParsed.mLeanImages;
				aText.mLeanFrameRate = aParsed.mLeanFrameRate;
			}
			if (!aSetter[k].empty() && (aSetterText[k] != aText.Text()))
				theStatus->mWarnings.push_back({aSource.mModId, std::string("features.") + gFeatureNames[k] + " overrides " + aSetter[k]});
			aSetter[k] = aSource.mModId;
			aSetterText[k] = aText.Text();
		}
	}
	for (int k = 0; k < NUM_FEATURES; k++)
	{
		if (!aSetter[k].empty())
			theStatus->mProvenance[std::string("features.") + gFeatureNames[k]] = {aSetter[k]};
	}

	// The leaderboard implies replays and a seed for every game
	if (aMerged.mLeaderboard)
	{
		aMerged.mReplays = true;
		aMerged.mSeeds = true;
		aMerged.mSeedPerGame = true;
	}
	*theFeatures = aMerged;
}
