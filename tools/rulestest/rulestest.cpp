// Unit fixtures for the code-mod rules (src/game/GameRules.h; docs/CODE_MODS_DESIGN.md 3.5-3.7 and the
// stage 1 row of 9.2): merge, atomic groups, keyed objects, null resets, the addTypes union, validation
// with drop-lowest, the canonical form, and RulesId vectors that the native and the wasm build must
// both reproduce. Stage 2: MTRand draws and GetState/SetState round trips (the RNG a board owns).
// Framework-free: GameRules.cpp, Sha256.cpp, Json.cpp and the framework's MTRand.cpp only.
//
//   rulestest              run every fixture; prints a summary, exit status 0 when all pass
//   rulestest -v           also print each check and the vectors' canonical text
//
// Native: build/native/rulestest.exe. Web: build/web/rulestest.js under Node
// (E:/retools/emsdk/node/<version>/bin/node.exe build/web/rulestest.js).

#include "GameRules.h"
#include "Sha256.h"
#include "MTRand.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

using namespace Sexy;
using Platform::JsonValue;

static int gChecks = 0;
static int gFailures = 0;
static bool gVerbose = false;

static void Check(bool theOk, const std::string& theWhat)
{
	gChecks++;
	if (!theOk)
	{
		gFailures++;
		printf("FAIL  %s\n", theWhat.c_str());
	}
	else if (gVerbose)
		printf("ok    %s\n", theWhat.c_str());
}

// One mod: its id and the part of its mod.json that matters here ("tunables", "rules", "features")
struct TestMod
{
	const char*		mId;
	const char*		mJson;
};

// The data\diff.txt of Bejeweled Deluxe 1.87: 500 150 1 0.5 250 10 2 0.5 1 2 0.01 0.005 (game state, not
// an asset: the same numbers GAME_NOTES and WinDMApp.h document)
static DiffValues OriginalDiff()
{
	DiffValues aDiff;
	aDiff.mNormalPointsBase = 500;
	aDiff.mNormalPointsPerLevel = 150;
	aDiff.mNormalMultBase = 1.0;
	aDiff.mNormalMultPerLevel = 0.5;
	aDiff.mTimedPointsBase = 250;
	aDiff.mTimedPointsPerLevel = 10;
	aDiff.mTimedMultBase = 2.0;
	aDiff.mTimedMultPerLevel = 0.5;
	aDiff.mTimeDrainBase = 1.0;
	aDiff.mTimeDrainPerLevel = 2.0;
	aDiff.mTimeDrainAccelBase = 0.01;
	aDiff.mTimeDrainAccelPerLevel = 0.005;
	return aDiff;
}

struct Result
{
	GameRules		mRules;
	GameFeatures	mFeatures;
	RulesStatus		mStatus;
};

// theMods lowest priority first, as MergeRules takes them
static Result Merge(const std::vector<TestMod>& theMods)
{
	std::vector<RulesSource> aSources;
	for (size_t i = 0; i < theMods.size(); i++)
	{
		JsonValue aManifest;
		std::string anError;
		if (!Platform::ParseJson(theMods[i].mJson, aManifest, &anError))
			Check(false, std::string("fixture JSON of ") + theMods[i].mId + ": " + anError);
		RulesSource aSource;
		aSource.mModId = theMods[i].mId;
		aSource.mTunables = aManifest["tunables"];
		aSource.mRules = aManifest["rules"];
		aSource.mFeatures = aManifest["features"];
		aSources.push_back(aSource);
	}
	Result aResult;
	aResult.mRules.mDiff = OriginalDiff();
	MergeRules(aSources, &aResult.mRules, &aResult.mStatus);
	MergeFeatures(aSources, &aResult.mFeatures, &aResult.mStatus);
	return aResult;
}

static bool HasMessage(const std::vector<RulesMessage>& theMessages, const std::string& theMod, const std::string& theText)
{
	for (size_t i = 0; i < theMessages.size(); i++)
	{
		if ((theMessages[i].mModId == theMod) && (theMessages[i].mText.find(theText) != std::string::npos))
			return true;
	}
	return false;
}

static std::string Messages(const Result& theResult)
{
	std::string aText;
	const std::vector<RulesMessage>* aLists[3] = {&theResult.mStatus.mWarnings, &theResult.mStatus.mErrors, &theResult.mStatus.mDropped};
	const char* aNames[3] = {"warning", "error", "dropped"};
	for (int k = 0; k < 3; k++)
		for (size_t i = 0; i < aLists[k]->size(); i++)
			aText += std::string("\n        ") + aNames[k] + " " + (*aLists[k])[i].mModId + ": " + (*aLists[k])[i].mText;
	return aText;
}

static bool Clean(const Result& theResult)
{
	return theResult.mStatus.mWarnings.empty() && theResult.mStatus.mErrors.empty() && theResult.mStatus.mDropped.empty();
}

static std::string Hex(const std::string& theBytes)
{
	static const char aHex[] = "0123456789abcdef";
	std::string aText;
	for (size_t i = 0; i < theBytes.size(); i++)
	{
		aText += aHex[(unsigned char) theBytes[i] >> 4];
		aText += aHex[(unsigned char) theBytes[i] & 15];
	}
	return aText;
}

// The mods a key came from, joined with ", "
static std::string Prov(const Result& theResult, const std::string& theKey)
{
	std::map<std::string, std::vector<std::string> >::const_iterator it = theResult.mStatus.mProvenance.find(theKey);
	std::string aText;
	if (it != theResult.mStatus.mProvenance.end())
	{
		for (size_t i = 0; i < it->second.size(); i++)
			aText += (i ? ", " : "") + it->second[i];
	}
	return aText;
}

// The ids of the dropped mods, in drop order, joined with ", "
static std::string Dropped(const Result& theResult)
{
	std::string aText;
	for (size_t i = 0; i < theResult.mStatus.mDropped.size(); i++)
		aText += (i ? ", " : "") + theResult.mStatus.mDropped[i].mModId;
	return aText;
}

// The 3.3 / 3.4 examples, as written in the design
static const TestMod gDesignExamples[] =
{
	{"free-swaps", "{\"rules\": {\"moves\": {\"allowInvalid\": true, \"invalidPenalty\": 0}, \"noMoves\": {\"normal\": \"continue\"}}}"},
	{"nine-gems", "{\"rules\": {\"board\": {\"gemColors\": 9}}}"},
	{"wide-board", "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}"},
	{"diamond", "{\"rules\": {\"board\": {\"mask\": [\"...###...\", \"..#####..\", \".#######.\", \"#########\", \"####.####\", "
		"\"#########\", \".#######.\", \"..#####..\", \"...###...\"]}}}"},
	{"chuzzle-slides", "{\"rules\": {\"moves\": {\"type\": \"slide\"}}}"},
	{"twist", "{\"rules\": {\"moves\": {\"type\": \"twist\", \"twist\": {\"direction\": \"both\"}}}}"},
	{"shapes", "{\"rules\": {\"match\": {\"diagonal\": true, \"crossings\": \"merge\", \"shapes\": {\"square\": {\"preset\": \"square\"}, "
		"\"L3\": {\"cells\": [[0,0],[0,1],[1,1]], \"rotations\": true, \"mirror\": true}}, "
		"\"bonus\": {\"L\": 50, \"T\": 80, \"plus\": 120, \"square\": 30, \"L3\": 10, \"diagonal\": 20}}}}"},
	{"power-gems", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run4\"], \"radius\": 1, \"points\": 20}, "
		"\"lightning\": {\"createdBy\": [\"run5\", \"T\", \"L\"], \"points\": 30}}}}"},
	{"gold-quest", "{\"rules\": {\"gold\": {\"enabled\": true, \"timeLimit\": {\"normal\": null, \"timed\": {\"base\": 180, \"perLevel\": -10, \"min\": 60}}}}}"},
	{"seeds", "{\"features\": {\"seeds\": {\"entry\": true, \"perGame\": true}}}"},
	{"replays", "{\"features\": {\"replays\": {\"keep\": 20, \"hover\": true}}}"},
	{"undo", "{\"rules\": {\"undo\": {\"enabled\": true, \"depth\": 3, \"penalty\": 25}}}"},
	{"online", "{\"features\": {\"leaderboard\": {\"ranked\": \"ask\"}}}"}
};

static const char* const gTwistedQuest = "{\"rules\": {"
	"\"board\": {\"gemColors\": 8, \"levels\": [{\"cols\": 8, \"rows\": 8}, {\"mask\": [\"..#####..\", \".#######.\", \"#########\", "
	"\"#########\", \"####.####\", \"#########\", \"#########\", \".#######.\", \"..#####..\"]}]}, "
	"\"moves\": {\"type\": [\"swap\", \"twist\"]}, "
	"\"match\": {\"crossings\": \"merge\", \"bonus\": {\"L\": 40, \"T\": 60}}, "
	"\"specials\": {\"flame\": {\"createdBy\": [\"run4\"]}, \"lightning\": {\"createdBy\": [\"T\", \"run5\"]}}, "
	"\"gold\": {\"enabled\": true}, "
	"\"undo\": {\"enabled\": true, \"depth\": 1, \"penalty\": 50}}, "
	"\"features\": {\"replays\": true, \"seeds\": true}}";

// ---------------------------------------------------------------------------------------------------

static void TestSha256()
{
	Check(Sha256::HashHex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "sha256 of \"\"");
	Check(Sha256::HashHex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 of \"abc\"");
	Check(Sha256::HashHex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
		"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "sha256 of the 448-bit FIPS vector");
	std::string aMillion(1000000, 'a');
	Check(Sha256::HashHex(aMillion) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", "sha256 of a million 'a'");
}

static void TestDefaults()
{
	Result r = Merge({});
	GameRules aDefaults;
	aDefaults.mDiff = OriginalDiff();
	Check(r.mRules.Canonical() == aDefaults.Canonical(), "no mods: the default rules");
	Check(Clean(r), "no mods: no messages" + Messages(r));
	Check(!r.mRules.HasVariantRules(), "defaults are not variant rules");
	Check(r.mRules.FillMinimum() == 3, "orthogonal runs need 3 colours (3.6)");
	std::string aMessage;
	Check(r.mRules.Validate(&aMessage, NULL), "the defaults validate");
	// u16 version, u16 0 (no fields), then 4 + 8*3 + 4 + 8*7 diff bytes
	Check(r.mRules.Canonical().size() == 2 + 2 + 4 + 24 + 4 + 56, "default canonical size");
	Check(r.mRules.Canonical().substr(0, 4) == std::string("\x01\x00\x00\x00", 4), "default canonical header: version 1, no fields");
	Check(r.mFeatures.Text() == "none", "no features");
}

static void TestTunables()
{
	// example-relaxed, as shipped
	Result r = Merge({{"example-relaxed", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10}}}"}});
	Check(r.mRules.mTunables.mHintPenalty == 10.0, "example-relaxed: hintPenalty 10");
	Check(Clean(r), "example-relaxed: no messages" + Messages(r));
	Check(!r.mRules.HasVariantRules(), "tunables are not variant rules");
	Check(Prov(r, "scoring.hintPenalty") == "example-relaxed", "provenance of a tunable");

	// The old lenient reading: wrong types give the default, numbers are truncated, empty arrays are
	// ignored, gemColors is clamped (to 3..12 here; GameConfig plays at most 7 until stage 8), unknown keys
	// are ignored silently
	r = Merge({{"lenient", "{\"tunables\": {\"scoring\": {\"hintPenalty\": \"x\", \"excellentFactor\": true, \"lengthBonus\": 12.7, "
		"\"longLengthBonus\": null, \"setValues\": []}, \"board\": {\"gemColors\": 20}, \"timing\": {\"startDrainDelay\": -5.9}, "
		"\"whatever\": 1}}"}});
	Check(r.mRules.mTunables.mHintPenalty == 50.0, "lenient: a string hintPenalty is the default");
	Check(r.mRules.mTunables.mExcellentFactor == 1.0, "lenient: a bool reads as 1 (ModManager::GetTunable)");
	Check(r.mRules.mTunables.mLengthBonus == 12, "lenient: 12.7 truncates to 12");
	Check(r.mRules.mTunables.mLongLengthBonus == 10, "lenient: null is the default");
	Check(r.mRules.mTunables.mSetValues.size() == 9, "lenient: an empty setValues is the default");
	Check(r.mRules.mBoard.mGemColors == 12, "lenient: gemColors 20 clamps to 12");
	Check(r.mRules.mTunables.mStartDrainDelay == -5, "lenient: -5.9 truncates to -5");
	Check(Clean(r), "lenient tunables: no messages" + Messages(r));

	// Outside the int range a number reads as INT_MIN, what the native game's (int) cast always gave it
	// (x86's "integer indefinite"); values that truncate into the range are ordinary
	r = Merge({{"huge", "{\"tunables\": {\"board\": {\"gemColors\": 3e9}, \"scoring\": {\"setValues\": [3e9, 20], \"lengthBonus\": 3e9, "
		"\"longLengthBonus\": -3e9}, \"timing\": {\"startDrainDelay\": 2147483647.9, \"levelDrainDelay\": -2147483648.9}}}"}});
	Check(r.mRules.mBoard.mGemColors == 3, "lenient: gemColors 3e9 reads as INT_MIN, clamped to 3 (as the stage-0 native game)");
	Check(r.mRules.mTunables.mSetValues == std::vector<int>({INT_MIN, 20}), "lenient: setValues 3e9 reads as INT_MIN");
	Check(r.mRules.mTunables.mLengthBonus == INT_MIN && r.mRules.mTunables.mLongLengthBonus == INT_MIN, "lenient: 3e9 and -3e9 read as INT_MIN");
	Check(r.mRules.mTunables.mStartDrainDelay == INT_MAX && r.mRules.mTunables.mLevelDrainDelay == INT_MIN, "lenient: truncation into the int range");

	// Priority: the higher mod wins a tunable, even with a wrong type (it then means the default)
	r = Merge({{"low", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10, \"lengthBonus\": 20}}}"},
		{"high", "{\"tunables\": {\"scoring\": {\"hintPenalty\": \"oops\"}}}"}});
	Check(r.mRules.mTunables.mHintPenalty == 50.0, "tunables: the higher mod's wrong type resets to the default");
	Check(r.mRules.mTunables.mLengthBonus == 20, "tunables: the lower mod keeps its other keys");

	// Inside one mod "rules" wins over "tunables"; under "rules" the same keys are checked strictly
	r = Merge({{"both", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10}}, \"rules\": {\"scoring\": {\"hintPenalty\": 25}}}"}});
	Check(r.mRules.mTunables.mHintPenalty == 25.0, "rules win over tunables inside one mod");
	r = Merge({{"strict", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10}}, \"rules\": {\"scoring\": {\"hintPenalty\": \"x\"}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "strict", "rules ignored: scoring.hintPenalty must be a number"), "rules: a wrong type rejects the rules" + Messages(r));
	Check(r.mRules.mTunables.mHintPenalty == 10.0, "rejected rules: the mod's tunables still apply");
}

static void TestLeaves()
{
	Result r = Merge({{"a", "{\"rules\": {\"match\": {\"minRun\": 4}, \"moves\": {\"allowInvalid\": true}}}"},
		{"b", "{\"rules\": {\"match\": {\"minRun\": 2}}}"}});
	Check(r.mRules.mMatch.mMinRun == 2, "leaf: the higher mod wins");
	Check(r.mRules.mMoves.mAllowInvalid, "leaf: the lower mod keeps its other leaves");
	Check(HasMessage(r.mStatus.mWarnings, "b", "match.minRun overrides a"), "leaf conflict warning" + Messages(r));
	Check(Prov(r, "match.minRun") == "b" && Prov(r, "moves.allowInvalid") == "a", "leaf provenance");
	Check(r.mRules.mMatch.mDiagonalMinRun == 2, "diagonalMinRun defaults to minRun");

	// The same value set twice is no conflict
	r = Merge({{"a", "{\"rules\": {\"match\": {\"minRun\": 4}}}"}, {"b", "{\"rules\": {\"match\": {\"minRun\": 4}}}"}});
	Check(Clean(r), "equal leaves: no warning" + Messages(r));

	// Arrays are replaced whole
	r = Merge({{"a", "{\"rules\": {\"scoring\": {\"setValues\": [1, 2, 3, 4]}}}"}, {"b", "{\"rules\": {\"scoring\": {\"setValues\": [9]}}}"}});
	Check(r.mRules.mTunables.mSetValues == std::vector<int>({9}), "arrays are replaced whole");
}

static void TestGroups()
{
	// shape: one mod's mask never meets another mod's row count
	const char* aDiamond = "{\"rules\": {\"board\": {\"mask\": [\"...###...\", \"..#####..\", \".#######.\", \"#########\", \"####.####\", "
		"\"#########\", \".#######.\", \"..#####..\", \"...###...\"]}}}";
	Result r = Merge({{"wide-board", "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}"}, {"diamond", aDiamond}});
	Check(r.mRules.mBoard.mCols == 9 && r.mRules.mBoard.mRows == 9 && r.mRules.mBoard.mMask.size() == 9, "shape group: the diamond comes whole");
	Check(HasMessage(r.mStatus.mWarnings, "diamond", "overrides wide-board"), "shape group warning" + Messages(r));
	r = Merge({{"diamond", aDiamond}, {"wide-board", "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}"}});
	Check(r.mRules.mBoard.mCols == 10 && r.mRules.mBoard.mRows == 7 && r.mRules.mBoard.mMask.empty(), "shape group: the 10x7 comes whole, without the mask");
	BoardLayout aLayout = r.mRules.LayoutForLevel(0);
	Check(aLayout.mExtent == 508 && aLayout.mOriginX == 203 && aLayout.mOriginY == 92 && aLayout.mScreenBottom == 596, "10x7 layout (5.1)");

	// moves: the 3.5 example
	r = Merge({{"chuzzle-slides", "{\"rules\": {\"moves\": {\"type\": \"slide\", \"slide\": {\"snapTicks\": 20}}}}"},
		{"twist", "{\"rules\": {\"moves\": {\"type\": \"twist\", \"twist\": {\"direction\": \"both\"}}}}"}});
	Check(r.mRules.mMoves.mTypes == GameRules::MOVE_TWIST && r.mRules.mMoves.mTwistBoth, "moves group: twist supplies the group");
	Check(r.mRules.mMoves.mSlideSnapTicks == 12, "moves group: the lower mod's snapTicks does not leak in");
	Check(HasMessage(r.mStatus.mWarnings, "twist", "moves.type overrides chuzzle-slides"), "the design's warning text" + Messages(r));

	// gold and undo: keys the supplier leaves out take their defaults
	r = Merge({{"a", "{\"rules\": {\"undo\": {\"enabled\": true, \"depth\": 5, \"penalty\": 25}, \"gold\": {\"enabled\": true, \"timeBonus\": 3}}}"},
		{"b", "{\"rules\": {\"undo\": {\"enabled\": true}, \"gold\": {\"gild\": \"matches\"}}}"}});
	Check(r.mRules.mUndo.mEnabled && r.mRules.mUndo.mDepth == 1 && r.mRules.mUndo.mPenalty == 0, "undo group: whole from b");
	Check(!r.mRules.mGold.mEnabled && !r.mRules.mGold.mGildBlasts && r.mRules.mGold.mTimeBonus == 10, "gold group: whole from b");

	// A leaf outside the moves group combines with it (3.5 table: free-swaps + chuzzle-slides)
	r = Merge({{"chuzzle-slides", "{\"rules\": {\"moves\": {\"type\": \"slide\"}}}"},
		{"free-swaps", "{\"rules\": {\"moves\": {\"allowInvalid\": true}, \"noMoves\": {\"normal\": \"continue\"}}}"}});
	Check(r.mRules.mMoves.mTypes == GameRules::MOVE_SLIDE && r.mRules.mMoves.mAllowInvalid && r.mRules.mNoMoves.mNormal == GameRules::NOMOVES_CONTINUE,
		"allowInvalid is a leaf: it combines with the slides group");
	Check(Clean(r), "free-swaps + chuzzle-slides: no messages" + Messages(r));
}

static void TestUnion()
{
	Result r = Merge({{"slides-too", "{\"rules\": {\"moves\": {\"addTypes\": [\"slide\"]}}}"},
		{"twist", "{\"rules\": {\"moves\": {\"type\": \"twist\", \"twist\": {\"direction\": \"both\"}}}}"}});
	Check(r.mRules.mMoves.mTypes == (GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE), "addTypes: slide added to twist");
	Check(r.mRules.mMoves.mTwistBoth, "addTypes does not supply the moves group");
	Check(Clean(r), "addTypes: no warning" + Messages(r));
	Check(Prov(r, "moves.addTypes") == "slides-too", "addTypes provenance");
	Check(GameRules::MoveTypesText(r.mRules.mMoves.mTypes) == "twist+slide", "move types text in fixed order");

	// Added to the default swap when no mod sets the group; a higher reset removes it
	r = Merge({{"slides-too", "{\"rules\": {\"moves\": {\"addTypes\": \"slide\"}}}"}});
	Check(r.mRules.mMoves.mTypes == (GameRules::MOVE_SWAP | GameRules::MOVE_SLIDE), "addTypes on the default swap");
	r = Merge({{"slides-too", "{\"rules\": {\"moves\": {\"addTypes\": [\"slide\", \"twist\"]}}}"}, {"reset", "{\"rules\": {\"moves\": {\"addTypes\": null}}}"}});
	Check(r.mRules.mMoves.mTypes == GameRules::MOVE_SWAP, "addTypes: null removes the lower additions");
	r = Merge({{"slides-too", "{\"rules\": {\"moves\": {\"addTypes\": [\"slide\"]}}}"}, {"plain", "{\"rules\": {\"moves\": null}}"}});
	Check(r.mRules.mMoves.mTypes == GameRules::MOVE_SWAP, "moves: null removes the lower additions too");

	// Several mods add types: each one is in the provenance (so each gets its own status messages)
	r = Merge({{"add-a", "{\"rules\": {\"moves\": {\"addTypes\": [\"slide\"]}}}"}, {"add-b", "{\"rules\": {\"moves\": {\"addTypes\": [\"twist\"]}}}"}});
	Check(r.mRules.mMoves.mTypes == (GameRules::MOVE_SWAP | GameRules::MOVE_TWIST | GameRules::MOVE_SLIDE) && Prov(r, "moves.addTypes") == "add-a, add-b",
		"addTypes provenance lists every adder, lowest priority first");
}

static void TestKeyed()
{
	Result r = Merge({{"a", "{\"rules\": {\"match\": {\"shapes\": {\"square\": {\"preset\": \"square\"}}, \"bonus\": {\"square\": 30}}, \"board\": {\"gemColors\": 12}}}"},
		{"b", "{\"rules\": {\"match\": {\"shapes\": {\"L3\": {\"cells\": [[0,0],[0,1],[1,1]], \"rotations\": true, \"mirror\": true}}, "
			"\"bonus\": {\"square\": 50, \"L3\": 10}}}}"}});
	Check(r.mRules.mMatch.mShapes.size() == 2 && r.mRules.mMatch.mShapes.count("square") && r.mRules.mMatch.mShapes.count("L3"), "keyed: shapes merge per id");
	Check(r.mRules.mMatch.mBonus["square"] == 50 && r.mRules.mMatch.mBonus["L3"] == 10, "keyed: bonus merges per id, the higher mod wins an id");
	Check(HasMessage(r.mStatus.mWarnings, "b", "match.bonus.square overrides a"), "keyed conflict warning" + Messages(r));
	Check(Prov(r, "match.shapes.square") == "a" && Prov(r, "match.shapes.L3") == "b", "keyed provenance");

	// specials.flame and specials.lightning merge per id: each one comes from one mod
	r = Merge({{"a", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run4\"], \"radius\": 2}, \"lightning\": {}}}}"},
		{"b", "{\"rules\": {\"specials\": {\"flame\": {\"points\": 5}}}}"}});
	Check(r.mRules.mSpecials.mFlame.mEnabled && r.mRules.mSpecials.mFlame.mRadius == 1 && r.mRules.mSpecials.mFlame.mPoints == 5,
		"specials.flame comes whole from b (radius back to 1)");
	Check(r.mRules.mSpecials.mLightning.mEnabled && r.mRules.mSpecials.mLightning.mCreatedBy == std::vector<std::string>({"run5"}),
		"specials.lightning from a, with the Bejeweled 2 default trigger run5");
}

static void TestNull()
{
	Result r = Merge({{"power-gems", "{\"rules\": {\"specials\": {\"lightning\": {\"createdBy\": [\"run5\"]}}}}"},
		{"no-lightning", "{\"rules\": {\"specials\": {\"lightning\": null}}}"}});
	Check(!r.mRules.mSpecials.mLightning.mEnabled, "null resets a key");
	Check(HasMessage(r.mStatus.mWarnings, "no-lightning", "specials.lightning overrides power-gems"), "a reset is an override" + Messages(r));

	r = Merge({{"gold-quest", "{\"rules\": {\"gold\": {\"enabled\": true}}}"}, {"no-gold", "{\"rules\": {\"gold\": null}}"}});
	Check(!r.mRules.mGold.mEnabled && r.mRules.mNoMoves.mNormal == GameRules::NOMOVES_GAME_OVER, "null resets a group (and noMoves.normal follows)");
	r = Merge({{"gold-quest", "{\"rules\": {\"gold\": {\"enabled\": true}}}"}});
	Check(r.mRules.mNoMoves.mNormal == GameRules::NOMOVES_FALL_OUT, "with gold, noMoves.normal defaults to fallOut");

	r = Merge({{"a", "{\"rules\": {\"match\": {\"shapes\": {\"square\": {\"preset\": \"square\"}}, \"bonus\": {\"square\": 30}}}}"},
		{"b", "{\"rules\": {\"match\": {\"shapes\": {\"square\": null}}}}"}});
	Check(r.mRules.mMatch.mShapes.empty() && r.mRules.mMatch.mBonus["square"] == 30, "null resets a keyed id");
	Check(HasMessage(r.mStatus.mWarnings, "b", "match.shapes.square overrides a"), "a keyed reset is an override" + Messages(r));
	Check(HasMessage(r.mStatus.mWarnings, "a", "match.bonus.square has no effect"), "a bonus for a pattern that no longer exists");
	r = Merge({{"a", "{\"rules\": {\"match\": {\"shapes\": {\"T\": {\"preset\": \"T4\"}}}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "a", "a shape id is"), "a reserved shape id is an error" + Messages(r));

	r = Merge({{"a", "{\"rules\": {\"match\": {\"shapes\": {\"square\": {\"preset\": \"square\"}, \"sq2\": {\"preset\": \"square\"}}}}}"},
		{"b", "{\"rules\": {\"match\": {\"shapes\": null}}}"}});
	Check(r.mRules.mMatch.mShapes.empty(), "null resets a whole keyed object");

	r = Merge({{"a", "{\"rules\": {\"match\": {\"minRun\": 4, \"diagonal\": true, \"bonus\": {\"diagonal\": 5}}, \"moves\": {\"allowInvalid\": true}}}"},
		{"b", "{\"rules\": {\"match\": null}}"}});
	Check(r.mRules.mMatch.mMinRun == 3 && !r.mRules.mMatch.mDiagonal && r.mRules.mMatch.mBonus.empty() && r.mRules.mMoves.mAllowInvalid,
		"null resets a namespace, nothing else");
}

static void TestDropLowest()
{
	// The 3.5 table: minRun 2 (A, higher) + diagonal (B, lower) with gemColors 4: the colour minimum is 5,
	// so B, the lower contributor, is dropped and the merge runs again
	Result r = Merge({{"B", "{\"rules\": {\"match\": {\"diagonal\": true}}}"},
		{"A", "{\"rules\": {\"match\": {\"minRun\": 2}, \"board\": {\"gemColors\": 4}}}"}});
	Check(r.mStatus.mDropped.size() == 1 && HasMessage(r.mStatus.mDropped, "B", "below 5"), "drop-lowest: B dropped, minimum 5" + Messages(r));
	Check(r.mRules.mMatch.mMinRun == 2 && !r.mRules.mMatch.mDiagonal && r.mRules.mBoard.mGemColors == 4, "drop-lowest: A remains");

	// Each mod alone is fine
	r = Merge({{"B", "{\"rules\": {\"match\": {\"diagonal\": true}}}"}});
	Check(Clean(r) && r.mRules.mMatch.mDiagonal, "B alone");

	// A mod that only fails with another one: noMoves "continue" relies on a lower allowInvalid that a
	// higher mod turns off; the lowest contributor (free-swaps) goes
	r = Merge({{"free-swaps", "{\"rules\": {\"moves\": {\"allowInvalid\": true}, \"noMoves\": {\"normal\": \"continue\"}}}"},
		{"strict", "{\"rules\": {\"moves\": {\"allowInvalid\": false}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "free-swaps", "noMoves \"continue\" needs moves.allowInvalid"), "drop-lowest: noMoves" + Messages(r));
	Check(r.mRules.mNoMoves.mNormal == GameRules::NOMOVES_GAME_OVER, "drop-lowest: noMoves back to the default");

	// A mod that fails on its own is dropped too (3.6 "Reject" for a cross-field check)
	r = Merge({{"bad-continue", "{\"rules\": {\"noMoves\": {\"timed\": \"continue\"}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "bad-continue", "needs moves.allowInvalid"), "a lone failing mod is dropped" + Messages(r));
	Check(r.mRules.Canonical() == Merge({}).mRules.Canonical(), "nothing of it remains");

	// Only the mods that supplied a cause of the failure are candidates (Validate): the values the check reads
	// that differ from their defaults, and the board shape only where the original 8x8 board would pass
	const char* aDiamond = "{\"rules\": {\"board\": {\"mask\": [\"...###...\", \"..#####..\", \".#######.\", \"#########\", \"####.####\", "
		"\"#########\", \".#######.\", \"..#####..\", \"...###...\"]}}}";
	const char* aFlameL = "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"L\"]}}}}";
	const char* aFlameDiagonal = "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"diagonal\"]}}}}";
	r = Merge({{"diamond", aDiamond}, {"B", "{\"rules\": {\"match\": {\"diagonal\": true}}}"},
		{"A", "{\"rules\": {\"match\": {\"minRun\": 2}, \"board\": {\"gemColors\": 4}}}"}});
	Check(Dropped(r) == "B" && r.mRules.mBoard.mCols == 9, "the 3.5 table case under a shape mod: only B is dropped" + Messages(r));
	r = Merge({{"wide-board", "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}"}, {"flame-L", aFlameL}});
	Check(Dropped(r) == "flame-L" && r.mRules.RulesIdHex() == Merge({{"wide-board", "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}"}}).mRules.RulesIdHex(),
		"an L trigger does not depend on the board: only flame-L is dropped" + Messages(r));
	r = Merge({{"bonus", "{\"rules\": {\"match\": {\"bonus\": {\"diagonal\": 20}}}}"},
		{"diag4", "{\"rules\": {\"match\": {\"diagonal\": true}, \"board\": {\"gemColors\": 4}}}"}});
	Check(Dropped(r) == "diag4" && r.mRules.mMatch.mBonus.count("diagonal"), "a bonus does not raise the colour minimum: only diag4 is dropped" + Messages(r));
	r = Merge({{"cross", "{\"rules\": {\"match\": {\"crossings\": \"merge\"}}}"}, {"fd", aFlameDiagonal}});
	Check(Dropped(r) == "fd" && r.mRules.mMatch.mMergeCrossings, "crossings do not make diagonals: only fd is dropped" + Messages(r));
	r = Merge({{"mr4", "{\"rules\": {\"match\": {\"minRun\": 4}}}"}, {"fd", aFlameDiagonal}});
	Check(Dropped(r) == "fd" && r.mRules.mMatch.mMinRun == 4, "minRun does not make diagonals: only fd is dropped" + Messages(r));
	r = Merge({{"gold-quest", "{\"rules\": {\"gold\": {\"enabled\": true}}}"}, {"bad", "{\"rules\": {\"noMoves\": {\"timed\": \"continue\"}}}"}});
	Check(Dropped(r) == "bad" && r.mRules.mGold.mEnabled, "the derived noMoves.normal is no cause: only bad is dropped" + Messages(r));
	r = Merge({{"swap", "{\"rules\": {\"moves\": {\"type\": \"swap\"}}}"},
		{"strips", "{\"rules\": {\"board\": {\"mask\": [\"####\", \"....\", \"####\", \"....\", \"####\"]}}}"},
		{"twist-too", "{\"rules\": {\"moves\": {\"addTypes\": [\"twist\"]}}}"}});
	Check(Dropped(r) == "strips" && r.mRules.mMoves.mTypes == (GameRules::MOVE_SWAP | GameRules::MOVE_TWIST),
		"twist without a 2x2: the shape and the mod that added twist are the causes, not the plain swap" + Messages(r));
	r = Merge({{"small", "{\"rules\": {\"board\": {\"cols\": 3, \"rows\": 3}}}"}, {"flame4", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run4\"]}}}}"}});
	Check(Dropped(r) == "small" && r.mRules.mSpecials.mFlame.mEnabled, "run4 fits the original board: the 3x3 board is a cause" + Messages(r));
	r = Merge({{"small", "{\"rules\": {\"board\": {\"cols\": 3, \"rows\": 3}}}"}, {"flame9", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run9\"]}}}}"}});
	Check(Dropped(r) == "flame9" && r.mRules.mBoard.mCols == 3, "run9 fits no original board: the 3x3 board is no cause" + Messages(r));
	r = Merge({{"wide", "{\"rules\": {\"board\": {\"cols\": 16, \"rows\": 4}}}"},
		{"long", "{\"rules\": {\"board\": {\"gemColors\": 3}, \"match\": {\"shapes\": {\"long\": {\"cells\": [[0,0],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],[9,0]]}}}}}"}});
	Check(Dropped(r) == "wide", "a pattern that only fits the wider board: the board is a cause of the colour minimum" + Messages(r));

	// The 3.4 "separate mods" as written: power-gems' L trigger has no crossings mod, so only power-gems goes
	r = Merge({gDesignExamples[3], gDesignExamples[5], gDesignExamples[7], gDesignExamples[8], gDesignExamples[11]});
	Check(Dropped(r) == "power-gems" && r.mRules.mBoard.mCols == 9 && r.mRules.mMoves.mTypes == GameRules::MOVE_TWIST && r.mRules.mGold.mEnabled &&
		r.mRules.mUndo.mEnabled, "3.4 separate mods without crossings: only power-gems is dropped" + Messages(r));

	// A tunables-only mod can supply a cause too; dropped, its tunables go with it
	r = Merge({{"low", "{\"tunables\": {\"board\": {\"gemColors\": 3}, \"scoring\": {\"hintPenalty\": 5}}}"},
		{"high", "{\"rules\": {\"match\": {\"minRun\": 2, \"diagonal\": true}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "low", "below 5"), "drop-lowest can drop a tunables mod" + Messages(r));
	Check(r.mRules.mTunables.mHintPenalty == 50.0 && r.mRules.mBoard.mGemColors == 7, "the dropped mod's tunables are gone");
}

static void TestValidation()
{
	// Type and range checks reject a mod's rules; its tunables stay
	Result r = Merge({{"big", "{\"tunables\": {\"scoring\": {\"lengthBonus\": 5}}, \"rules\": {\"board\": {\"cols\": 20}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "big", "board.cols must be an integer 3..16"), "cols 20 rejected" + Messages(r));
	Check(r.mRules.mBoard.mCols == 8 && r.mRules.mTunables.mLengthBonus == 5, "rejected rules: defaults, tunables kept");
	r = Merge({{"t", "{\"rules\": {\"match\": {\"minRun\": \"3\"}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "t", "match.minRun must be an integer 2..5"), "a string int is rejected");
	r = Merge({{"t", "{\"rules\": {\"moves\": {\"type\": [\"swap\", \"spin\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "t", "moves.type must be"), "an unknown move type is rejected");
	r = Merge({{"t", "{\"rules\": 5}"}});
	Check(HasMessage(r.mStatus.mErrors, "t", "\"rules\" must be an object"), "rules must be an object");
	{
		// "rules" names a file that ModManager could not read: its reason is the error
		RulesSource aSource;
		aSource.mModId = "file";
		aSource.mRules = JsonValue(std::string("rules/nope.json"));
		aSource.mRulesError = "rules file rules/nope.json: not found";
		aSource.mTunables = Platform::JsonValue::MakeObject();
		aSource.mTunables.Set("scoring", Platform::JsonValue::MakeObject());
		Result aResult;
		MergeRules({aSource}, &aResult.mRules, &aResult.mStatus);
		Check(aResult.mStatus.mErrors.size() == 1 && HasMessage(aResult.mStatus.mErrors, "file", "rules ignored: rules file rules/nope.json: not found"),
			"an unreadable rules file is reported with its reason" + Messages(aResult));
	}

	// The mask legend (3.2, 4.14): # cell, . hole, d dirt, 2..9 dirt layers; dirt counts as a cell
	r = Merge({{"dirt", "{\"rules\": {\"board\": {\"mask\": [\"#d##\", \"#29#\", \"####\", \"#..#\"]}}}"}});
	Check(Clean(r), "dirt mask accepted" + Messages(r));
	BoardLayout aLayout = r.mRules.LayoutForLevel(0);
	Check(aLayout.mCols == 4 && aLayout.mRows == 4 && aLayout.IsDirt(1, 0) && aLayout.mDirt[0][1] == 1 && aLayout.mDirt[1][1] == 2 &&
		aLayout.mDirt[1][2] == 9 && !aLayout.IsPlayable(1, 3) && !aLayout.IsDirt(1, 3) && aLayout.CountCells() == 14 && aLayout.HasDirt(),
		"dirt layout: layers and holes");
	r = Merge({{"dirt9", "{\"rules\": {\"board\": {\"mask\": [\"ddd\", \"d#d\", \"ddd\"]}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "dirt9", "no line of match.minRun"), "nine dirt cells pass the 9-cell check, then no line of 3 gems" + Messages(r));
	r = Merge({{"x", "{\"rules\": {\"board\": {\"mask\": [\"#x#\", \"###\", \"###\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "x", "#.d23456789"), "an unknown mask character is rejected");
	r = Merge({{"tiny", "{\"rules\": {\"board\": {\"mask\": [\"##.\", \"#.#\", \".##\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "tiny", "at least 9"), "fewer than 9 cells is rejected" + Messages(r));
	r = Merge({{"ragged", "{\"rules\": {\"board\": {\"mask\": [\"###\", \"####\", \"###\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "ragged", "same length"), "rows of different length are rejected");
	r = Merge({{"mismatch", "{\"rules\": {\"board\": {\"cols\": 8, \"mask\": [\"#########\", \"#########\", \"#########\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "mismatch", "not board.cols x board.rows"), "mask dims against cols/rows");
	r = Merge({{"both", "{\"rules\": {\"board\": {\"mask\": [\"###\", \"###\", \"###\"], \"levels\": [{\"cols\": 8, \"rows\": 8}]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "both", "board.mask and board.levels"), "mask and levels in one mod");
	r = Merge({{"lv", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 8, \"rows\": 8}, {\"mask\": [\"####\", \"#..#\", \"####\"]}]}}}"}});
	Check(Clean(r) && r.mRules.ShapeCount() == 2 && r.mRules.ShapeForLevel(3).mCols == 4 && r.mRules.ShapeForLevel(2).mCols == 8,
		"levels: level L uses levels[L % n]" + Messages(r));
	r = Merge({{"lv", "{\"rules\": {\"board\": {\"cols\": 10, \"levels\": [{\"cols\": 8, \"rows\": 8}, {\"mask\": [\"####\", \"#..#\", \"####\"]}]}}}"}});
	Check(HasMessage(r.mStatus.mWarnings, "lv", "ignored with board.levels") && r.mRules.mBoard.mCols == 8 && Prov(r, "board.cols") == "" &&
		Prov(r, "board.levels") == "lv", "board.cols ignored next to board.levels: no provenance for it" + Messages(r));
	r = Merge({{"lv", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 8, \"rows\": 8}, {\"mask\": [\"#.#\", \"...\", \"#.#\"]}]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "lv", "board.levels[1] has 4 cells"), "levels are checked one by one" + Messages(r));

	// Cross-field checks (merged set)
	r = Merge({{"small", "{\"rules\": {\"board\": {\"cols\": 3, \"rows\": 3}, \"match\": {\"minRun\": 4}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "small", "no line of match.minRun (4)"), "a line of minRun cells" + Messages(r));
	r = Merge({{"checker", "{\"rules\": {\"board\": {\"mask\": [\"#.#.\", \".#.#\", \"#.#.\", \".#.#\", \"#.#.\"]}}, \"tunables\": {}}"}});
	Check(HasMessage(r.mStatus.mDropped, "checker", "no line of match.minRun (3)"), "a checker mask has 10 cells but no line" + Messages(r));
	r = Merge({{"strips", "{\"rules\": {\"board\": {\"mask\": [\"####\", \"....\", \"####\", \"....\", \"####\"]}, \"moves\": {\"type\": \"twist\"}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "strips", "twist needs a 2x2 block"), "twist needs a 2x2" + Messages(r));
	r = Merge({{"flame-L", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"L\"]}}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "flame-L", "the pattern L cannot occur"), "a trigger that cannot occur" + Messages(r));
	r = Merge({{"crossings", "{\"rules\": {\"match\": {\"crossings\": \"merge\"}}}"}, {"flame-L", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"L\"]}}}}"}});
	Check(Clean(r) && r.mRules.mSpecials.mFlame.mEnabled, "the same trigger with another mod's crossings: merge" + Messages(r));
	r = Merge({{"same", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run5\", \"run4\"]}, \"lightning\": {\"createdBy\": [\"run4\", \"run5\", \"run4\"]}}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "same", "same patterns"), "identical triggers" + Messages(r));
	r = Merge({{"run9", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run9\"]}}}}"}});
	Check(HasMessage(r.mStatus.mDropped, "run9", "run9 cannot occur"), "a run longer than the board" + Messages(r));

	// Clamp and warn (3.6)
	r = Merge({{"undo", "{\"rules\": {\"undo\": {\"enabled\": true, \"depth\": 200}, \"moves\": {\"slide\": {\"snapTicks\": 2}}, "
		"\"gold\": {\"timeLimit\": {\"normal\": {\"base\": 5, \"perLevel\": 0, \"min\": 3}}}}}"}});
	Check(r.mRules.mUndo.mDepth == 100 && HasMessage(r.mStatus.mWarnings, "undo", "undo.depth 200 clamped to 100"), "undo.depth clamped" + Messages(r));
	Check(r.mRules.mMoves.mSlideSnapTicks == 4 && HasMessage(r.mStatus.mWarnings, "undo", "snapTicks 2 clamped to 4"), "snapTicks clamped");
	Check(r.mRules.mGold.mTimeLimitNormal.mEnabled && r.mRules.mGold.mTimeLimitNormal.mBase == 10 && r.mRules.mGold.mTimeLimitNormal.mMin == 10,
		"time limits clamped to 10 s");

	// Unknown keys warn, with a suggestion, and are ignored
	r = Merge({{"typo", "{\"rules\": {\"match\": {\"minrun\": 4}, \"gold\": {\"enabeld\": true}, \"specials\": {\"flame\": {\"radiu\": 2}}, \"bogus\": 1}}"}});
	Check(HasMessage(r.mStatus.mWarnings, "typo", "unknown key match.minrun ignored (did you mean match.minRun?)"), "did you mean match.minRun" + Messages(r));
	Check(HasMessage(r.mStatus.mWarnings, "typo", "(did you mean gold.enabled?)"), "did you mean gold.enabled");
	Check(HasMessage(r.mStatus.mWarnings, "typo", "(did you mean specials.flame.radius?)"), "did you mean specials.flame.radius");
	Check(HasMessage(r.mStatus.mWarnings, "typo", "unknown key bogus ignored"), "unknown key without a suggestion");
	Check(r.mRules.mMatch.mMinRun == 3 && !r.mRules.mGold.mEnabled, "unknown keys are ignored");
}

static void TestFillMinimum()
{
	Check(Merge({}).mRules.FillMinimum() == 3, "fill minimum: orthogonal runs 3");
	Check(Merge({{"m", "{\"rules\": {\"match\": {\"minRun\": 2, \"diagonal\": true}}}"}}).mRules.FillMinimum() == 5, "fill minimum: minRun 2 with diagonals 5");
	Check(Merge({{"m", "{\"rules\": {\"match\": {\"shapes\": {\"sq\": {\"preset\": \"square\"}}}}}"}}).mRules.FillMinimum() == 4, "fill minimum: squares 4");
	Check(Merge({{"m", "{\"rules\": {\"match\": {\"minRun\": 2}}}"}}).mRules.FillMinimum() == 3, "fill minimum: minRun 2 alone 3");
	Check(Merge({{"m", "{\"rules\": {\"match\": {\"diagonal\": true}}}"}}).mRules.FillMinimum() == 5, "fill minimum: diagonals of 3 5");
	// A shape that repeats a run is the same instance as the run, counted once
	Result aLine = Merge({{"a", "{\"rules\": {\"board\": {\"gemColors\": 4}, \"match\": {\"shapes\": {\"line\": {\"cells\": [[0,0],[1,0],[2,0]], \"rotations\": true}}}}}"}});
	Check(Clean(aLine) && aLine.mRules.FillMinimum() == 3, "fill minimum: a shape equal to the runs adds nothing" + Messages(aLine));
	// A cell pattern counts every orientation placed through the cell with its other cells filled
	Check(Merge({{"m", "{\"rules\": {\"board\": {\"gemColors\": 12}, \"match\": {\"shapes\": {\"l\": {\"preset\": \"L3\"}}}}}"}}).mRules.FillMinimum() == 7,
		"fill minimum: L3 in all orientations 7");
}

static void TestDesignExamples()
{
	for (size_t i = 0; i < sizeof(gDesignExamples) / sizeof(gDesignExamples[0]); i++)
	{
		Result r = Merge({gDesignExamples[i]});
		std::string anId = gDesignExamples[i].mId;
		if (anId == "shapes")
		{
			// Design gap: diagonals, squares and L3 in all orientations need 10 colours to fill (3.6), so
			// the example as written fails the colour minimum; with gemColors 10 it is valid
			Check(HasMessage(r.mStatus.mDropped, "shapes", "below 10"), "example shapes: needs 10 colours" + Messages(r));
		}
		else if (anId == "power-gems")
		{
			// Design gap: lightning's T and L triggers need match.crossings "merge", which the example does
			// not set; alone it is dropped, with a crossings mod it is valid
			Check(HasMessage(r.mStatus.mDropped, "power-gems", "cannot occur"), "example power-gems: T and L need crossings" + Messages(r));
		}
		else
			Check(Clean(r), "example " + anId + " is valid" + Messages(r));
	}

	Result r = Merge({{"shapes-10", "{\"rules\": {\"board\": {\"gemColors\": 10}, \"match\": {\"diagonal\": true, \"crossings\": \"merge\", "
		"\"shapes\": {\"square\": {\"preset\": \"square\"}, \"L3\": {\"cells\": [[0,0],[0,1],[1,1]], \"rotations\": true, \"mirror\": true}}}}}"}});
	Check(Clean(r) && r.mRules.FillMinimum() == 10, "the shapes example with gemColors 10" + Messages(r));
	r = Merge({{"crossings", "{\"rules\": {\"match\": {\"crossings\": \"merge\"}}}"}, gDesignExamples[7]});
	Check(Clean(r) && r.mRules.mSpecials.mLightning.mEnabled, "power-gems with a crossings mod" + Messages(r));

	// 3.4: the separate mods land in different groups and combine without overrides
	r = Merge({gDesignExamples[3], gDesignExamples[5], {"crossings", "{\"rules\": {\"match\": {\"crossings\": \"merge\"}}}"},
		gDesignExamples[7], gDesignExamples[8], gDesignExamples[11]});
	Check(Clean(r), "diamond + twist + crossings + power-gems + gold-quest + undo combine" + Messages(r));
	Check(r.mRules.mBoard.mCols == 9 && r.mRules.mMoves.mTypes == GameRules::MOVE_TWIST && r.mRules.mSpecials.mLightning.mEnabled &&
		r.mRules.mGold.mEnabled && r.mRules.mUndo.mDepth == 3, "the combination has every part");

	r = Merge({{"twisted-quest", gTwistedQuest}});
	Check(Clean(r), "twisted-quest (3.4) is valid" + Messages(r));
	Check(r.mFeatures.mReplays && r.mFeatures.mSeeds && r.mFeatures.mSeedEntry && r.mFeatures.mSeedPerGame, "twisted-quest features");
	Check(r.mRules.ShapeCount() == 2 && r.mRules.LayoutForLevel(1).mCols == 9 && r.mRules.LayoutForLevel(1).CountCells() == 68,
		"twisted-quest level shapes");
}

static void TestFeatures()
{
	Result r = Merge({{"seeds", "{\"features\": {\"seeds\": true}}"}});
	Check(r.mFeatures.mSeeds && r.mFeatures.mSeedEntry && r.mFeatures.mSeedPerGame && !r.mFeatures.mReplays, "seeds: true");
	Check(Prov(r, "features.seeds") == "seeds" && !r.mRules.HasVariantRules(), "features: provenance, no rules");
	// The object form turns on only the members it sets: the seed field alone keeps every game on the global
	// stream (perGame gives each game its own RNG, docs/CODE_MODS_DESIGN.md 6.2)
	r = Merge({{"entry", "{\"features\": {\"seeds\": {\"entry\": true}}}"}});
	Check(r.mFeatures.mSeeds && r.mFeatures.mSeedEntry && !r.mFeatures.mSeedPerGame && (r.mFeatures.Text() == "seeds(entry)"),
		"seeds {entry: true}: entry only (" + r.mFeatures.Text() + ")");
	r = Merge({{"pergame", "{\"features\": {\"seeds\": {\"perGame\": true}}}"}});
	Check(r.mFeatures.mSeeds && !r.mFeatures.mSeedEntry && r.mFeatures.mSeedPerGame && (r.mFeatures.Text() == "seeds(perGame)"),
		"seeds {perGame: true}: perGame only (" + r.mFeatures.Text() + ")");
	r = Merge({{"empty", "{\"features\": {\"seeds\": {}}}"}});
	Check(!r.mFeatures.mSeeds && !r.mFeatures.mSeedEntry && !r.mFeatures.mSeedPerGame && (r.mFeatures.Text() == "none"),
		"seeds {}: nothing on (" + r.mFeatures.Text() + ")");
	r = Merge({{"low", "{\"features\": {\"seeds\": true}}"}, {"high", "{\"features\": {\"seeds\": {\"entry\": true}}}"}});
	Check(r.mFeatures.mSeedEntry && !r.mFeatures.mSeedPerGame && HasMessage(r.mStatus.mWarnings, "high", "features.seeds overrides low"),
		"seeds: the higher mod's object supplies the whole feature" + Messages(r));
	r = Merge({{"r", "{\"features\": {\"replays\": {\"keep\": 5, \"hover\": true}}}"}});
	Check(r.mFeatures.mReplays && r.mFeatures.mReplayKeep == 5 && r.mFeatures.mReplayHover, "replays options");
	r = Merge({{"online", "{\"features\": {\"leaderboard\": {\"ranked\": \"always\"}}}"}});
	Check(r.mFeatures.mLeaderboard && r.mFeatures.mRankedAlways && r.mFeatures.mReplays && r.mFeatures.mSeedPerGame,
		"leaderboard implies replays and seeds.perGame");
	r = Merge({{"low", "{\"features\": {\"replays\": {\"keep\": 5}, \"seeds\": true}}"}, {"high", "{\"features\": {\"replays\": false}}"}});
	Check(!r.mFeatures.mReplays && r.mFeatures.mSeeds && HasMessage(r.mStatus.mWarnings, "high", "features.replays overrides low"),
		"features merge per feature" + Messages(r));
	r = Merge({{"bad", "{\"features\": {\"replays\": {\"keep\": \"all\"}, \"seeds\": true}}"}});
	Check(HasMessage(r.mStatus.mErrors, "bad", "features ignored: features.replays.keep") && !r.mFeatures.mSeeds, "a bad feature value ignores the mod's features");
	r = Merge({{"typo", "{\"features\": {\"replay\": true}}"}});
	Check(HasMessage(r.mStatus.mWarnings, "typo", "did you mean features.replays?"), "unknown feature" + Messages(r));
	Check(r.mFeatures.Text() == "none", "unknown feature ignored");
}

// JSON has no NaN or Infinity, though strtod reads them: rejected, so no libc's NaN reaches a RulesId
static void TestJson()
{
	const char* aBad[] = {"-nan", "-inf", "-infinity", "-nan(123)", "1e999", "-1e999", "[1, -nan]", "{\"tunables\": {\"scoring\": {\"hintPenalty\": -nan}}}"};
	for (size_t i = 0; i < sizeof(aBad) / sizeof(aBad[0]); i++)
	{
		JsonValue aValue;
		std::string anError;
		Check(!Platform::ParseJson(aBad[i], aValue, &anError) && (anError.find("bad number") != std::string::npos),
			std::string("JSON rejects ") + aBad[i] + " (" + anError + ")");
	}
	JsonValue aValue;
	Check(Platform::ParseJson("-0", aValue) && aValue.IsNumber() && (aValue.AsNumber() == 0.0), "JSON reads -0");
	Check(Platform::ParseJson("[1e308, -2.5e-308]", aValue) && (aValue[0].AsNumber() == 1e308), "JSON reads large and small finite numbers");
}

// ---------------------------------------------------------------------------------------------------
// Canonical form and RulesId

static void TestCanonical()
{
	// Equivalent rules share a RulesId
	std::string aDefault = Merge({}).mRules.Canonical();
	Check(Merge({{"full", "{\"rules\": {\"board\": {\"mask\": [\"########\", \"########\", \"########\", \"########\", "
		"\"########\", \"########\", \"########\", \"########\"]}}}"}}).mRules.Canonical() == aDefault, "a full 8x8 mask is the default board");
	Check(Merge({{"z", "{\"rules\": {\"match\": {\"bonus\": {\"L\": 0}}, \"moves\": {\"type\": [\"swap\", \"swap\"]}}}"}}).mRules.Canonical() == aDefault,
		"a zero bonus and a repeated swap are the defaults");
	Check(Merge({{"d", "{\"rules\": {\"match\": {\"diagonal\": true, \"diagonalMinRun\": 3}}}"}}).mRules.Canonical() ==
		Merge({{"d", "{\"rules\": {\"match\": {\"diagonal\": true}}}"}}).mRules.Canonical(), "diagonalMinRun equal to minRun is the default");
	Check(Merge({{"g", "{\"rules\": {\"gold\": {\"enabled\": true}, \"noMoves\": {\"normal\": \"fallOut\"}}}"}}).mRules.Canonical() ==
		Merge({{"g", "{\"rules\": {\"gold\": {\"enabled\": true}}}"}}).mRules.Canonical(), "noMoves.normal fallOut under gold is the default");
	Check(Merge({{"g", "{\"rules\": {\"gold\": {\"enabled\": true}, \"noMoves\": {\"normal\": \"gameOver\"}}}"}}).mRules.Canonical() !=
		Merge({{"g", "{\"rules\": {\"gold\": {\"enabled\": true}}}"}}).mRules.Canonical(), "noMoves.normal gameOver under gold is not");
	Check(Merge({{"s", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run5\", \"run4\", \"run4\"]}}}}"}}).mRules.Canonical() ==
		Merge({{"s", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run4\", \"run5\"]}}}}"}}).mRules.Canonical(), "createdBy is a set");
	Check(Merge({{"a", "{\"rules\": {\"undo\": {\"enabled\": true}}}"}, {"b", "{\"rules\": {\"match\": {\"minRun\": 4}}}"}}).mRules.Canonical() ==
		Merge({{"ab", "{\"rules\": {\"match\": {\"minRun\": 4}, \"undo\": {\"enabled\": true}}}"}}).mRules.Canonical(),
		"two mods with the same effective rules share a RulesId");

	// One spelling per rule (3.7): board.levels that repeat a cycle, a single level, cell patterns with a
	// preset's orientations, run names, -0
	const char* aShape10x7 = "{\"rules\": {\"board\": {\"cols\": 10, \"rows\": 7}}}";
	const char* aMask = "{\"mask\": [\"####\", \"#..#\", \"####\"]}";
	Result r = Merge({{"one", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 8, \"rows\": 8}]}}}"}});
	Check(r.mRules.Canonical() == aDefault && !r.mRules.HasVariantRules() && Clean(r), "board.levels [8x8] is the original board" + Messages(r));
	Check(Merge({{"two", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 8, \"rows\": 8}, {\"mask\": [\"########\", \"########\", "
		"\"########\", \"########\", \"########\", \"########\", \"########\", \"########\"]}]}}}"}}).mRules.Canonical() == aDefault,
		"two original levels are the original board");
	Check(Merge({{"l", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 10, \"rows\": 7}, {\"cols\": 10, \"rows\": 7}]}}}"}}).mRules.Canonical() ==
		Merge({{"b", aShape10x7}}).mRules.Canonical(), "identical levels are board.cols/rows");
	std::string aLevels = "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 8, \"rows\": 8}, " + std::string(aMask);
	std::string aCycle2 = aLevels + "]}}}";
	std::string aCycle4 = aLevels + ", {\"cols\": 8, \"rows\": 8}, " + aMask + "]}}}";
	std::string aNoCycle = aLevels + ", {\"cols\": 8, \"rows\": 8}]}}}";
	Check(Merge({{"l", aCycle4.c_str()}}).mRules.Canonical() == Merge({{"l", aCycle2.c_str()}}).mRules.Canonical(), "levels that repeat a cycle are the cycle");
	Check(Merge({{"l", aNoCycle.c_str()}}).mRules.ShapeCount() == 3, "levels that do not repeat stay");
	std::string aSquare = Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 6}, \"match\": {\"shapes\": {\"sq\": {\"preset\": \"square\"}}}}}"}}).mRules.Canonical();
	Check(Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 6}, \"match\": {\"shapes\": {\"sq\": {\"cells\": [[5,5],[6,5],[5,6],[6,6]], "
		"\"rotations\": true, \"mirror\": true}}}}}"}}).mRules.Canonical() == aSquare, "the square's cells, with any flags, are the preset square");
	std::string anL3 = Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 8}, \"match\": {\"shapes\": {\"l\": {\"preset\": \"L3\"}}}}}"}}).mRules.Canonical();
	Check(Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 8}, \"match\": {\"shapes\": {\"l\": {\"cells\": [[1,0],[1,1],[0,1]], \"rotations\": true}}}}}"}}).mRules.Canonical() == anL3,
		"an L of three with rotations (the mirror adds nothing) is the preset L3");
	Check(Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 8}, \"match\": {\"shapes\": {\"t\": {\"cells\": [[1,0],[0,1],[1,1],[1,2]], \"rotations\": true}}}}}"}}).mRules.mMatch.mShapes["t"].mPreset == "T4",
		"a T of four with rotations is the preset T4");
	r = Merge({{"s", "{\"rules\": {\"board\": {\"gemColors\": 8}, \"match\": {\"shapes\": {\"j\": {\"cells\": [[3,1],[4,1],[3,2]], \"mirror\": true}}}}}"}});
	Check(r.mRules.mMatch.mShapes["j"].mPreset.empty() && r.mRules.mMatch.mShapes["j"].mMirror && !r.mRules.mMatch.mShapes["j"].mRotations &&
		r.mRules.mMatch.mShapes["j"].mCells == std::vector<std::pair<int, int> >({{0, 0}, {0, 1}, {1, 0}}), "a pattern no preset covers: its smallest orientation");
	r = Merge({{"zero", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run05\"]}}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "zero", "invalid pattern name"), "run05 is not a pattern name (one spelling: run5)" + Messages(r));
	r = Merge({{"zero", "{\"rules\": {\"specials\": {\"flame\": {\"createdBy\": [\"run0000000000000000000000000000000000000005\"]}}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "zero", "invalid pattern name"), "a run name with many leading zeros is rejected" + Messages(r));
	Check(Merge({{"z", "{\"tunables\": {\"scoring\": {\"hintPenalty\": -0}}}"}}).mRules.Canonical() ==
		Merge({{"z", "{\"rules\": {\"scoring\": {\"hintPenalty\": 0}}}"}}).mRules.Canonical(), "hintPenalty -0 is 0");
	Check(Merge({{"z", "{\"tunables\": {\"scoring\": {\"hintPenalty\": -0}}}"}}).mRules.CanonicalLines()[1] == "scoring.hintPenalty = 0", "-0 is written 0");

	// Round trip
	const char* aSets[] = {"{}", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10.25, \"setValues\": [1, -2, 3]}}}", gTwistedQuest,
		"{\"rules\": {\"board\": {\"mask\": [\"#d##\", \"#29#\", \"####\", \"#..#\"], \"dirtPoints\": 7}, \"gold\": {\"countDirt\": false, "
		"\"timeLimit\": {\"normal\": {\"base\": 200, \"perLevel\": -3, \"min\": 20}, \"timed\": null}}}}",
		"{\"rules\": {\"board\": {\"gemColors\": 12}, \"match\": {\"minRun\": 4, \"diagonal\": true, \"diagonalMinRun\": 2, \"shapes\": {\"zz\": {\"cells\": [[3,1],[4,1],[3,2]], "
		"\"mirror\": true}, \"aa\": {\"preset\": \"T4\"}}, \"bonus\": {\"zz\": -4, \"aa\": 99}}, \"moves\": {\"type\": [\"slide\"], \"slide\": {\"snapTicks\": 30}, "
		"\"invalidPenalty\": 5, \"invalidCountsAsMove\": false, \"allowInvalid\": true}, \"noMoves\": {\"normal\": \"continue\", \"timed\": \"gameOver\"}, "
		"\"specials\": {\"chain\": false, \"createIn\": \"move\"}, \"undo\": {\"enabled\": true, \"max\": 3, \"inTimed\": true, \"restoreTime\": true, \"restoreRng\": false}, "
		"\"timing\": {\"warningFraction\": 0.3, \"startDrainDelay\": 1, \"levelDrainDelay\": 2}, \"scoring\": {\"excellentFactor\": 99.5, \"lengthBonus\": -1, "
		"\"longLengthBonus\": 0}}}"};
	for (size_t i = 0; i < sizeof(aSets) / sizeof(aSets[0]); i++)
	{
		Result r = Merge({{"m", aSets[i]}});
		Check(r.mStatus.mErrors.empty() && r.mStatus.mDropped.empty(), "round-trip set " + std::to_string(i) + " is valid" + Messages(r));
		std::string aBytes = r.mRules.Canonical();
		GameRules aCopy;
		std::string anError;
		Check(aCopy.FromCanonical(aBytes, &anError), "FromCanonical of set " + std::to_string(i) + " " + anError);
		Check(aCopy.Canonical() == aBytes, "canonical round trip of set " + std::to_string(i));
		Check(aCopy.RulesIdHex() == r.mRules.RulesIdHex(), "RulesId round trip of set " + std::to_string(i));
		Check(aCopy.CanonicalLines() == r.mRules.CanonicalLines(), "text round trip of set " + std::to_string(i));
	}

	// Malformed canonical bytes are refused and leave the rules alone
	std::string aBytes = Merge({{"m", gTwistedQuest}}).mRules.Canonical();
	GameRules aRules;
	std::string aBefore = aRules.Canonical();
	Check(!aRules.FromCanonical(aBytes.substr(0, aBytes.size() - 1)), "FromCanonical: truncated");
	Check(!aRules.FromCanonical(aBytes + "x"), "FromCanonical: trailing byte");
	std::string aBad = aBytes;
	aBad[2] = (char) 0xEE;  // first field id
	Check(!aRules.FromCanonical(aBad), "FromCanonical: unknown field id");
	aBad = aBytes;
	aBad[0] = 2;
	Check(!aRules.FromCanonical(aBad), "FromCanonical: another rules version");
	Check(aRules.Canonical() == aBefore, "FromCanonical failures change nothing");

	// The canonical layout of one field: u16 version, (u16 id 48, u8 kind 1, i32 4), u16 0, the diff values
	std::string aMinRun = Merge({{"m", "{\"rules\": {\"match\": {\"minRun\": 4}}}"}}).mRules.Canonical();
	Check(Hex(aMinRun.substr(0, 11)) == "0100300001040000000000", "canonical bytes of match.minRun 4");
}

// RulesId vectors: the expected values are fixed here, so the native and the wasm build check the same
// numbers (doubles hashed as IEEE bits, ints little-endian, no text formatting)
struct Vector
{
	const char*		mName;
	std::vector<TestMod> mMods;
	const char*		mRulesId;
};

static void TestVectors()
{
	const Vector aVectors[] =
	{
		{"default", {}, "0e0943ce3dc5f1735c47f27c4dd38871daec36ba999f20926bb1a8ec3f5032b3"},
		{"example-relaxed", {{"example-relaxed", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10}}}"}}, "3af35ac3966e672b703bb6ef43acfa840ffc5763d987b4c55df9c84a04060b73"},
		{"twisted-quest", {{"twisted-quest", gTwistedQuest}}, "41a2453aa4acda72d75c85864f2b2f104ce2aad78f949e41bc2082e9e8d81ca5"},
		{"diamond+twist+crossings+power-gems+gold-quest+undo", {gDesignExamples[3], gDesignExamples[5],
			{"crossings", "{\"rules\": {\"match\": {\"crossings\": \"merge\"}}}"}, gDesignExamples[7], gDesignExamples[8], gDesignExamples[11]}, "cebae23770a9a5d27d245536fc06ca18e7b2ea8b12ca48e9e11856f2e21d9dd1"},
		{"dirt", {{"dirt", "{\"rules\": {\"board\": {\"mask\": [\"#d##\", \"#29#\", \"####\", \"#..#\"], \"dirtPoints\": 7}}, "
			"\"tunables\": {\"timing\": {\"warningFraction\": 0.1}}}"}}, "29424691e3829211aa3f66703af9dffcdb77dfc776f7767a38069e46bedaafb7"}
	};
	for (size_t i = 0; i < sizeof(aVectors) / sizeof(aVectors[0]); i++)
	{
		Result r = Merge(aVectors[i].mMods);
		std::string anId = r.mRules.RulesIdHex();
		printf("vector %-52s %s\n", aVectors[i].mName, anId.c_str());
		if (gVerbose)
		{
			std::vector<std::string> aLines = r.mRules.CanonicalLines();
			for (size_t j = 0; j < aLines.size(); j++)
				printf("    %s\n", aLines[j].c_str());
		}
		Check(anId == aVectors[i].mRulesId, std::string("RulesId vector ") + aVectors[i].mName);
	}

	// Doubles that no mod.json can carry (JSON rejects NaN) but data\diff.txt or a replay header might: the
	// canonical form writes -0 as 0 and every NaN as the one quiet NaN, whatever bits the platform made
	const double aNaN = std::numeric_limits<double>::quiet_NaN();
	const double aZero = 0.0;
	const double aValues[4] = {aNaN, -aNaN, aZero, -aZero};
	std::string anIds[4];
	for (int i = 0; i < 4; i++)
	{
		GameRules aRules;
		aRules.mDiff = OriginalDiff();
		aRules.mTunables.mHintPenalty = aValues[i];
		aRules.mDiff.mTimeDrainPerLevel = aValues[i];
		anIds[i] = aRules.RulesIdHex();
	}
	printf("vector %-52s %s\n", "hintPenalty and timeDrainPerLevel NaN", anIds[0].c_str());
	printf("vector %-52s %s\n", "hintPenalty and timeDrainPerLevel 0", anIds[2].c_str());
	Check(anIds[0] == "41d2bb7df0e62510eca4bbfd8f628c6ee6018f1566c4c63ff558e40387a52f60", "RulesId vector NaN");
	Check(anIds[1] == anIds[0], "RulesId: a NaN with the sign bit is the same NaN");
	Check(anIds[2] == "e7aa4e2f5e20d39239fae824fdb8f94e00b324762043c4515cfb3d9534547a5e", "RulesId vector 0");
	Check(anIds[3] == anIds[2], "RulesId: -0 is 0");
}

// Set seed (stage 2): the generator a board owns (Sexy::MTRand). Its draws must be the reference MT19937's
// (31 bits) on every platform, and GetState/SetState must carry the whole state (words and index), which
// Serialize() does not (MTRand.h)
static void TestMTRand()
{
	struct Draw { unsigned long mSeed; int mNumber; unsigned long mValue; };
	// MT19937 init_genrand: seed 5489's first draw is 3499211612 & 0x7fffffff; seed 1's second and third are the
	// per-game seeds of the oracle's -seed=1 runs; seed 0 plays as 4357
	const Draw aDraws[] =
	{
		{5489, 1, 1351727964}, {1, 1, 1791095845}, {1, 2, 2135392491}, {1, 3, 946286476}, {0, 1, 2146374468},
		{4357, 1, 2146374468}, {4357, 2, 699692587}, {4242, 1, 1395647406}, {4242, 1000, 1622832918},
		{4294967295ul, 1, 419326371}
	};
	for (size_t i = 0; i < sizeof(aDraws) / sizeof(aDraws[0]); i++)
	{
		MTRand aRand(aDraws[i].mSeed);
		unsigned long aValue = 0;
		for (int n = 0; n < aDraws[i].mNumber; n++)
			aValue = aRand.Next();
		Check(aValue == aDraws[i].mValue, "MTRand(" + std::to_string(aDraws[i].mSeed) + ") draw " + std::to_string(aDraws[i].mNumber) +
			" = " + std::to_string(aDraws[i].mValue) + " (got " + std::to_string(aValue) + ")");
	}

	// SetState(GetState()) goes on with the same draws, wherever in the 624-word block it was taken
	const int aSkips[] = {0, 1, 300, 623, 624, 625, 1247, 1248, 5000};
	for (size_t i = 0; i < sizeof(aSkips) / sizeof(aSkips[0]); i++)
	{
		MTRand anOriginal(4242);
		for (int n = 0; n < aSkips[i]; n++)
			anOriginal.Next();
		uint32_t aState[MTRAND_N];
		int anIndex = -1;
		anOriginal.GetState(aState, &anIndex);
		MTRand aCopy(1);
		bool isSet = aCopy.SetState(aState, anIndex);
		bool isSame = isSet && (anIndex == ((aSkips[i] == 0) ? MTRAND_N : (aSkips[i] - 1) % MTRAND_N + 1));
		for (int n = 0; (n < 2000) && isSame; n++)
			isSame = anOriginal.Next() == aCopy.Next();
		Check(isSame, "MTRand SetState(GetState()) after " + std::to_string(aSkips[i]) + " draws: the same next 2000 draws");
	}

	// An index outside 0..MTRAND_N+1 is refused and changes nothing
	MTRand aRand(7);
	MTRand aReference(7);
	uint32_t aState[MTRAND_N];
	int anIndex;
	MTRand(99).GetState(aState, &anIndex);
	Check(!aRand.SetState(aState, -1) && !aRand.SetState(aState, MTRAND_N + 2) && (aRand.Next() == aReference.Next()),
		"MTRand SetState refuses an index outside 0..625");

	// The all-zero state draws 0 forever (FillBoard would never find a colour): refused at every index. Only
	// the top bit of the first word takes part in the recurrence, so its low bits do not make it valid
	for (int aFirst = 0; aFirst < 2; aFirst++)
	{
		for (int i = 0; i < MTRAND_N; i++)
			aState[i] = 0;
		aState[0] = (aFirst == 0) ? 0 : 0x7fffffffu;
		bool isRefused = true;
		for (int anAt = 0; anAt <= MTRAND_N + 1; anAt++)
			isRefused = isRefused && !aRand.SetState(aState, anAt);
		Check(isRefused && (aRand.Next() == aReference.Next()), std::string("MTRand SetState refuses the zero state") +
			((aFirst == 0) ? "" : " with low bits in the first word"));
	}
	aState[0] = 0x80000000u;
	bool isDrawing = aRand.SetState(aState, MTRAND_N);
	unsigned long anOr = 0;
	for (int n = 0; (n < 2 * MTRAND_N) && isDrawing; n++)
		anOr |= aRand.Next();
	Check(isDrawing && (anOr != 0), "MTRand SetState takes a state whose only set bit is the first word's top bit");

	// The documented Serialize() limit: a copy made from it skips the rest of the current block
	MTRand aSerialized(4242);
	for (int n = 0; n < 100; n++)
		aSerialized.Next();
	MTRand aFromSerial(aSerialized.Serialize());
	for (int n = 100; n < MTRAND_N; n++)
		aSerialized.Next();
	bool isNextBlock = true;
	for (int n = 0; (n < 1000) && isNextBlock; n++)
		isNextBlock = aSerialized.Next() == aFromSerial.Next();
	Check(isNextBlock, "MTRand(Serialize()) goes on from the next block (Serialize keeps no index)");
}

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;
	}

	TestSha256();
	TestDefaults();
	TestTunables();
	TestLeaves();
	TestGroups();
	TestUnion();
	TestKeyed();
	TestNull();
	TestDropLowest();
	TestValidation();
	TestFillMinimum();
	TestDesignExamples();
	TestFeatures();
	TestJson();
	TestCanonical();
	TestVectors();
	TestMTRand();

	printf("rulestest: %d checks, %d failures: %s\n", gChecks, gFailures, gFailures == 0 ? "PASS" : "FAIL");
	return gFailures == 0 ? 0 : 1;
}
