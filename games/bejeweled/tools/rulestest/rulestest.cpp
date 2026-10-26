// Unit fixtures for the code-mod rules (games/bejeweled/src/GameRules.h;
// games/bejeweled/docs/CODE_MODS_DESIGN.md 3.5-3.7 and the stage 1 row of 9.2): merge, atomic groups, keyed
// objects, null resets, the addTypes union, validation with drop-lowest, the canonical form, and RulesId
// vectors that the native and the wasm build must both reproduce. Stage 2: MTRand draws and
// GetState/SetState round trips (the RNG a board owns). Stage 3: the .bjr replay format (Replay.cpp):
// varints, a write/read round trip of every record and op, a golden byte vector, and the reader on every
// truncation and on corrupt input. Stage 3b: what the replay player accepts (Replay::CheckPlayable), the
// record order and the reader's limits, and the check of a replay's save bytes (Replay::CheckSave). After the
// stage 4 review: the check of a replay's RULE (GameRules::CheckRecorded), the match.shapes cap, the reader's
// segment and per-tick limits, and where a replay may hold SAVEs. Stage 5: the v2 save's format (header,
// chunks, what a reader refuses), a v2 LDSV, the mode of a save, engine ABI 2.
// Framework-free: GameRules.cpp, Sha256.cpp, Replay.cpp, Json.cpp and the framework's MTRand.cpp only.
//
//   rulestest              run every fixture; prints a summary, exit status 0 when all pass
//   rulestest -v           also print each check and the vectors' canonical text
//
// Native: build/native/rulestest.exe. Web: build/web/rulestest.js under Node
// (E:/retools/emsdk/node/<version>/bin/node.exe build/web/rulestest.js).

#include "GameRules.h"
#include "MatchFinder.h"
#include "MoveSet.h"
#include "Sha256.h"
#include "Specials.h"
#include "MTRand.h"
#include "Replay.h"
#include "Piece.h"

#include <climits>
#include <cmath>
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
	Check(!r.mRules.ScoringTunablesChanged() && r.mRules.PlaysAlikeInEveryEngine(), "defaults: original scoring, alike in every engine");
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
	// Stage 7: a scoring tunable gets the fast score roll, so its replays from engines before 3 play otherwise; a
	// timing tunable does not
	Check(r.mRules.ScoringTunablesChanged() && !r.mRules.PlaysAlikeInEveryEngine(), "example-relaxed: scoring changed, not alike in every engine");
	for (const char* aScoring : {"{\"setValues\": [5000]}", "{\"lengthBonus\": 11}", "{\"longLengthBonus\": 9}", "{\"excellentFactor\": 100}"})
	{
		std::string aJson = std::string("{\"tunables\": {\"scoring\": ") + aScoring + "}}";
		Result aChanged = Merge({{"scoring", aJson.c_str()}});
		Check(aChanged.mRules.ScoringTunablesChanged(), std::string("scoring changed: ") + aScoring);
	}
	Result aTiming = Merge({{"timing", "{\"tunables\": {\"timing\": {\"startDrainDelay\": 100}}}"}});
	Check(!aTiming.mRules.ScoringTunablesChanged() && aTiming.mRules.PlaysAlikeInEveryEngine(), "a timing tunable: alike in every engine");
	Result aVariant = Merge({{"undo", "{\"rules\": {\"undo\": {\"enabled\": true}}}"}});
	Check(!aVariant.mRules.ScoringTunablesChanged() && !aVariant.mRules.PlaysAlikeInEveryEngine(), "a variant rule: not alike in every engine");
	// Stage 8: engines before 5 played board.gemColors above 7 as 7
	Check(Merge({{"six", "{\"tunables\": {\"board\": {\"gemColors\": 6}}}"}}).mRules.PlaysAlikeInEveryEngine(), "gemColors 6: alike in every engine");
	Check(Merge({{"seven", "{\"rules\": {\"board\": {\"gemColors\": 7}}}"}}).mRules.PlaysAlikeInEveryEngine(), "gemColors 7: alike in every engine");
	Check(!Merge({{"eight", "{\"rules\": {\"board\": {\"gemColors\": 8}}}"}}).mRules.PlaysAlikeInEveryEngine(), "gemColors 8: not alike in every engine");

	// The old lenient reading: wrong types give the default, numbers are truncated, empty arrays are
	// ignored, gemColors is clamped (to 3..12, as GameConfig plays it), unknown keys
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

// Stage 7b (4.14): which rules have dirt (GameRules::HasDirtRule), a layer cleared (BoardLayout::ClearDirtLayer) and
// the v2 save's mask and DIRT layers a level's mask takes (BoardLayout::FitsSaveMask)
static void TestDirt()
{
	const char* aDirtMask = "{\"rules\": {\"board\": {\"mask\": [\"#d##\", \"#29#\", \"####\", \"#..#\"], \"dirtPoints\": 7}}}";
	Result r = Merge({{"dirt", aDirtMask}});
	Check(Clean(r) && r.mRules.HasDirtRule() && (r.mRules.mBoard.mDirtPoints == 7), "a mask with dirt: a dirt rule" + Messages(r));
	Check(!Merge({{"holes", "{\"rules\": {\"board\": {\"mask\": [\"####\", \"#..#\", \"####\", \"####\"]}}}"}}).mRules.HasDirtRule() &&
		!Merge({}).mRules.HasDirtRule(), "holes alone, or the original board: no dirt rule");
	Check(Merge({{"levels", "{\"rules\": {\"board\": {\"levels\": [{\"cols\": 7, \"rows\": 7}, {\"cols\": 4, \"rows\": 4, \"mask\": "
		"[\"####\", \"#3##\", \"####\", \"####\"]}]}}}"}}).mRules.HasDirtRule(), "dirt on the second level shape only: a dirt rule");

	BoardLayout aLayout = r.mRules.LayoutForLevel(0);
	Check(aLayout.IsDirt(1, 0) && (aLayout.mDirt[0][1] == 1) && (aLayout.mDirt[1][1] == 2) && (aLayout.mDirt[1][2] == 9) && !aLayout.IsPlayable(1, 0),
		"the mask's dirt cells and layers");
	BoardLayout aCleared = aLayout;
	Check(aCleared.ClearDirtLayer(1, 1) && aCleared.IsDirt(1, 1) && (aCleared.mDirt[1][1] == 1), "a layer off: 1 left, still dirt");
	Check(aCleared.ClearDirtLayer(1, 1) && !aCleared.IsDirt(1, 1) && aCleared.IsPlayable(1, 1), "the last layer off: a gem cell");
	Check(!aCleared.ClearDirtLayer(1, 1) && !aCleared.ClearDirtLayer(1, 3), "no layer off a gem cell or a hole");

	// The save's mask from the cleared layout, its layers row-major
	std::vector<bool> aPlayable;
	std::vector<uint8_t> aLayers;
	for (int aRow = 0; aRow < 4; aRow++)
	{
		for (int aCol = 0; aCol < 4; aCol++)
		{
			aPlayable.push_back(aCleared.IsPlayable(aCol, aRow));
			aLayers.push_back(aCleared.IsDirt(aCol, aRow) ? aCleared.mDirt[aRow][aCol] : 0);
		}
	}
	Check(aLayout.FitsSaveMask(4, 4, aPlayable, &aLayers) && aLayout.FitsSaveMask(4, 4, aPlayable), "a cleared dirt cell fits the level's mask");
	Check(!aLayout.FitsSaveMask(4, 5, aPlayable) && !aLayout.FitsSaveMask(5, 4, aPlayable), "another size does not fit");
	std::vector<bool> aHoleFilled = aPlayable;
	aHoleFilled[13] = true;
	Check(!aLayout.FitsSaveMask(4, 4, aHoleFilled), "a gem cell where the mask has a hole does not fit");
	struct BadLayers { const char* mWhat; int mCell; int mLayers; };
	const BadLayers aBads[] =
	{
		{"layers on the cleared cell, a gem cell in the mask", 5, 1},
		{"more layers than the mask's", 1, 2},
		{"layers on a gem cell of the level's mask", 0, 1},
		{"no layers on a dirt cell the save's mask says is no gem cell", 6, 0},
	};
	for (size_t i = 0; i < sizeof(aBads) / sizeof(aBads[0]); i++)
	{
		std::vector<uint8_t> aBad = aLayers;
		aBad[aBads[i].mCell] = (uint8_t) aBads[i].mLayers;
		Check(!aLayout.FitsSaveMask(4, 4, aPlayable, &aBad), std::string("does not fit: ") + aBads[i].mWhat);
	}
}

// Stage 7c (4.16): board.gravity is an atomic group, look.* are leaves that make no variant board, the canonical
// bytes of fields 7, 8, 144 and 145, and the fill minimum taken over every fill order the direction uses
// Stage 18: the arrows group, its direction names and the moves between new arrows
static void TestArrows()
{
	Result r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true}}}"}});
	Check(Clean(r) && r.mRules.mArrows.mEnabled && (r.mRules.mArrows.mDirections == GameRules::ALL_ARROW_DIRECTIONS) &&
		(r.mRules.mArrows.mEvery == 6) && (r.mRules.mArrows.mEveryMin == 3) && (r.mRules.mArrows.mBonus == 100), "arrows: the defaults" + Messages(r));
	Check(r.mRules.HasVariantRules(), "arrows: a variant rule");
	Check((r.mRules.ArrowInterval(0) == 6) && (r.mRules.ArrowInterval(2) == 4) && (r.mRules.ArrowInterval(3) == 3) &&
		(r.mRules.ArrowInterval(40) == 3), "arrows: one move fewer each level, down to everyMin");
	r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"directions\": \"up\", \"every\": 2, \"everyMin\": 5}}}"}});
	Check(Clean(r) && (r.mRules.mArrows.mDirections == (1 << GameRules::ARROW_UP)) && (r.mRules.ArrowInterval(0) == 2) &&
		(r.mRules.ArrowInterval(9) == 2), "arrows: one direction; everyMin above every gives every" + Messages(r));
	r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"directions\": [\"left\", \"right\"]}}}"}});
	Check(Clean(r) && (r.mRules.mArrows.mDirections == ((1 << GameRules::ARROW_LEFT) | (1 << GameRules::ARROW_RIGHT))), "arrows: a list of directions" + Messages(r));
	r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"directions\": [\"upRight\", \"downRight\", \"downLeft\", \"upLeft\"]}}}"}});
	Check(Clean(r) && (r.mRules.mArrows.mDirections == 0xf0) && (GameRules::NUM_ARROW_DIRECTIONS == 8) &&
		(GameRules::ARROW_UP_LEFT == 7), "arrows: the diagonals, bits 4..7" + Messages(r));
	r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"directions\": [\"up\", \"north\"]}}}"}});
	Check(HasMessage(r.mStatus.mErrors, "a", "arrows.directions must be \"up\", \"right\", \"down\", \"left\", \"upRight\", \"downRight\", "
		"\"downLeft\", \"upLeft\" or a list of them") &&
		!r.mRules.mArrows.mEnabled, "arrows: an unknown direction rejects the group" + Messages(r));
	r = Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"every\": 500}}}"}});
	Check(r.mRules.mArrows.mEvery == 100, "arrows: every clamps to 100" + Messages(r));
	Check(Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true}}}"}}).mRules.Canonical() !=
		Merge({{"a", "{\"rules\": {\"arrows\": {\"enabled\": true, \"directions\": \"up\"}}}"}}).mRules.Canonical(), "arrows: the directions are in the canonical form");
}

static void TestGravity()
{
	const char* aTwilight = "{\"rules\": {\"board\": {\"gravity\": {\"direction\": \"alternate\", \"fallAccel\": 75}}, "
		"\"look\": {\"backdrop\": \"twilight\", \"soundPitch\": -2}}}";
	Result r = Merge({{"twilight", aTwilight}});
	Check(Clean(r) && r.mRules.mBoard.mGravity == GameRules::GRAVITY_ALTERNATE && r.mRules.mBoard.mFallAccel == 75 &&
		r.mRules.mLook.mBackdrop == GameRules::BACKDROP_TWILIGHT && r.mRules.mLook.mSoundPitch == -2, "the twilight preset" + Messages(r));
	Check(r.mRules.HasGravityRule() && r.mRules.HasVariantRules(), "twilight: a gravity rule, a variant board");
	Check(r.mRules.VariantKeys() == std::vector<std::string>({"board.gravity.direction", "board.gravity.fallAccel"}), "twilight: the variant keys");
	Check(r.mRules.LookKeys() == std::vector<std::string>({"look.backdrop", "look.soundPitch"}), "twilight: the look keys");
	Check(Hex(r.mRules.Canonical().substr(0, 2 + 3 + 2 + 9 + 3 + 4 + 3 + 2 + 8 + 3 + 4 + 2)) ==
		"0100" "070004" "0900" "616c7465726e617465" "080001" "4b000000" "900004" "0800" "7477696c69676874" "910001" "feffffff" "0000",
		"canonical bytes of fields 7, 8, 144 and 145");

	// look alone: in the RulesId (its own tables), not a variant board
	r = Merge({{"dusk", "{\"rules\": {\"look\": {\"backdrop\": \"twilight\"}}}"}});
	Check(Clean(r) && !r.mRules.HasVariantRules() && !r.mRules.HasGravityRule() && r.mRules.PlaysAlikeInEveryEngine(), "look alone: no variant board" + Messages(r));
	Check(r.mRules.RulesIdHex() != Merge({}).mRules.RulesIdHex(), "look alone: its own RulesId");
	r = Merge({{"low", "{\"rules\": {\"look\": {\"soundPitch\": -3}}}"}, {"dusk", "{\"rules\": {\"look\": {\"backdrop\": \"twilight\"}}}"}});
	Check(Clean(r) && r.mRules.mLook.mSoundPitch == -3 && r.mRules.mLook.mBackdrop == GameRules::BACKDROP_TWILIGHT, "look.* are leaves: they combine" + Messages(r));
	// fallAccel alone: a gravity rule (falls only), the direction stays down
	r = Merge({{"slow", "{\"rules\": {\"board\": {\"gravity\": {\"fallAccel\": 90}}}}"}});
	Check(Clean(r) && r.mRules.HasGravityRule() && r.mRules.HasVariantRules() && r.mRules.mBoard.mGravity == GameRules::GRAVITY_DOWN, "fallAccel alone" + Messages(r));
	Check(Merge({{"d", "{\"rules\": {\"board\": {\"gravity\": {\"direction\": \"down\", \"fallAccel\": 180}}, \"look\": {\"backdrop\": \"normal\", "
		"\"soundPitch\": 0}}}"}}).mRules.Canonical() == Merge({}).mRules.Canonical(), "the defaults said out loud are the original rules");

	// The group: the higher mod supplies all of it; null resets it
	r = Merge({{"twilight", aTwilight}, {"rising", "{\"rules\": {\"board\": {\"gravity\": {\"direction\": \"up\"}}}}"}});
	Check(r.mRules.mBoard.mGravity == GameRules::GRAVITY_UP && r.mRules.mBoard.mFallAccel == GameRules::DEFAULT_FALL_ACCEL &&
		r.mRules.mLook.mSoundPitch == -2, "gravity group: up comes whole, without the lower fallAccel; look stays");
	Check(HasMessage(r.mStatus.mWarnings, "rising", "board.gravity.direction overrides twilight"), "gravity group warning" + Messages(r));
	r = Merge({{"twilight", aTwilight}, {"flat", "{\"rules\": {\"board\": {\"gravity\": null}}}"}});
	Check(!r.mRules.HasGravityRule() && r.mRules.mLook.mBackdrop == GameRules::BACKDROP_TWILIGHT, "\"board\": {\"gravity\": null} resets the group only");
	r = Merge({{"twilight", aTwilight}, {"plain", "{\"rules\": {\"look\": null}}"}});
	Check(r.mRules.LookKeys().empty() && r.mRules.HasGravityRule(), "\"look\": null resets look.*");

	// Validation: names rejected, ranges clamped
	r = Merge({{"side", "{\"rules\": {\"board\": {\"gravity\": {\"direction\": \"sideways\"}}}}"}});
	Check(!r.mStatus.mErrors.empty() && !r.mRules.HasGravityRule(), "an unknown direction is an error" + Messages(r));
	r = Merge({{"c", "{\"rules\": {\"board\": {\"gravity\": {\"fallAccel\": 20}}, \"look\": {\"soundPitch\": 30}}}"}});
	Check(r.mRules.mBoard.mFallAccel == 50 && HasMessage(r.mStatus.mWarnings, "c", "fallAccel 20 clamped to 50"), "fallAccel clamps to 50..1000" + Messages(r));
	Check(r.mRules.mLook.mSoundPitch == 12 && HasMessage(r.mStatus.mWarnings, "c", "soundPitch 30 clamped to 12"), "soundPitch clamps to -12..12" + Messages(r));
	r = Merge({{"c", "{\"rules\": {\"board\": {\"gravity\": {\"fallAccel\": 2000}}, \"look\": {\"soundPitch\": -13}}}"}});
	Check(r.mRules.mBoard.mFallAccel == 1000 && r.mRules.mLook.mSoundPitch == -12 && r.mStatus.mErrors.empty(), "... from above and below" + Messages(r));
	r = Merge({{"c", "{\"rules\": {\"look\": {\"soundPitch\": -1, \"backdrop\": \"dark\"}}}"}});
	Check(r.mRules.LookKeys().empty() && HasMessage(r.mStatus.mErrors, "c", "look.backdrop"), "an unknown backdrop is an error, the mod's rules rejected" + Messages(r));

	// The fill minimum per fill order (3.6): a corner pattern of one orientation on a 3x3 board. Filled bottom-up
	// its three instances (row, column, corner) can end on one cell, top-down at most two; the vertical mirror the
	// other way round. Alternate fills both ways and takes the larger.
	const char* aBoard3 = "\"board\": {\"cols\": 3, \"rows\": 3";
	std::string aCorner = std::string("{\"rules\": {") + aBoard3 + "}, \"match\": {\"shapes\": {\"corner\": {\"cells\": [[0,0],[1,0],[0,1]]}}}}}";
	std::string aMirrored = std::string("{\"rules\": {") + aBoard3 + "}, \"match\": {\"shapes\": {\"corner\": {\"cells\": [[0,0],[0,1],[1,1]]}}}}}";
	const char* aDirections[3] = {"down", "up", "alternate"};
	int aCornerMinimum[3] = {4, 3, 4};
	int aMirroredMinimum[3] = {3, 4, 4};
	for (int i = 0; i < 3; i++)
	{
		std::string aGravity = std::string("{\"rules\": {\"board\": {\"gravity\": {\"direction\": \"") + aDirections[i] + "\"}}}}";
		Check(Merge({{"g", aGravity.c_str()}, {"corner", aCorner.c_str()}}).mRules.FillMinimum() == aCornerMinimum[i],
			std::string("fill minimum of the corner, ") + aDirections[i]);
		Check(Merge({{"g", aGravity.c_str()}, {"corner", aMirrored.c_str()}}).mRules.FillMinimum() == aMirroredMinimum[i],
			std::string("fill minimum of the mirrored corner, ") + aDirections[i]);
	}
	// gemColors 3 with the mirrored corner passes falling down; rising, gravity is a cause and its mod goes
	std::string aMirrored3 = std::string("{\"rules\": {") + aBoard3 + ", \"gemColors\": 3}, \"match\": {\"shapes\": {\"corner\": {\"cells\": [[0,0],[0,1],[1,1]]}}}}}";
	std::string aCorner3 = std::string("{\"rules\": {") + aBoard3 + ", \"gemColors\": 3}, \"match\": {\"shapes\": {\"corner\": {\"cells\": [[0,0],[1,0],[0,1]]}}}}}";
	r = Merge({{"corner", aMirrored3.c_str()}});
	Check(Clean(r), "the mirrored corner in 3 colours falls down" + Messages(r));
	r = Merge({{"twilight", aTwilight}, {"corner", aMirrored3.c_str()}});
	Check(Dropped(r) == "twilight" && HasMessage(r.mStatus.mDropped, "twilight", "below 4"), "the rising fill needs 4: twilight, the lower cause, goes" + Messages(r));
	// the corner in 3 colours fails falling down already: gravity is no cause, so the corner mod goes
	r = Merge({{"twilight", aTwilight}, {"corner", aCorner3.c_str()}});
	Check(Dropped(r) == "corner" && r.mRules.HasGravityRule(), "a fill that fails falling down: gravity is no cause" + Messages(r));
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
	// stream (perGame gives each game its own RNG, games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2)
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

	// Stage 2b: features.livesplit (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15): no gameplay, no options;
	// the mod that turns it on supplies the split templates
	r = Merge({{"livesplit", "{\"features\": {\"livesplit\": true}}"}});
	Check(r.mFeatures.mLiveSplit && (r.mFeatures.mLiveSplitMod == "livesplit") && (r.mFeatures.Text() == "livesplit") &&
		Prov(r, "features.livesplit") == "livesplit" && !r.mRules.HasVariantRules() && Clean(r), "livesplit: true" + Messages(r));
	Check(r.mRules.RulesIdHex() == Merge({}).mRules.RulesIdHex(), "livesplit is not part of the RulesId");
	r = Merge({{"low", "{\"features\": {\"livesplit\": true, \"seeds\": true}}"}, {"high", "{\"features\": {\"livesplit\": false}}"}});
	Check(!r.mFeatures.mLiveSplit && r.mFeatures.mLiveSplitMod.empty() && r.mFeatures.mSeeds &&
		HasMessage(r.mStatus.mWarnings, "high", "features.livesplit overrides low"), "livesplit: false overrides" + Messages(r));
	r = Merge({{"opts", "{\"features\": {\"livesplit\": {\"port\": 1}}}"}});
	Check(r.mFeatures.mLiveSplit && HasMessage(r.mStatus.mWarnings, "opts", "unknown key features.livesplit.port ignored"),
		"livesplit {}: on, with no options" + Messages(r));
	r = Merge({{"bad", "{\"features\": {\"livesplit\": 1}}"}});
	Check(!r.mFeatures.mLiveSplit && HasMessage(r.mStatus.mErrors, "bad", "features ignored: features.livesplit must be"),
		"livesplit: a number is refused" + Messages(r));

	// features.lean (4.21): compact images and a frame rate cap, not part of the RulesId
	r = Merge({{"lean", "{\"features\": {\"lean\": true}}"}});
	Check(r.mFeatures.mLean && r.mFeatures.mLeanImages && (r.mFeatures.mLeanFrameRate == 0) &&
		(r.mFeatures.Text() == "lean(images, frameRate 0)") && Clean(r), "lean: true" + Messages(r));
	Check(r.mRules.RulesIdHex() == Merge({}).mRules.RulesIdHex(), "lean is not part of the RulesId");
	r = Merge({{"lean", "{\"features\": {\"lean\": {\"images\": false, \"frameRate\": 30}}}"}});
	Check(r.mFeatures.mLean && !r.mFeatures.mLeanImages && (r.mFeatures.mLeanFrameRate == 30) &&
		(r.mFeatures.Text() == "lean(frameRate 30)"), "lean options (" + r.mFeatures.Text() + ")");
	r = Merge({{"bad", "{\"features\": {\"lean\": {\"frameRate\": 5}}}"}});
	Check(!r.mFeatures.mLean && HasMessage(r.mStatus.mErrors, "bad", "features ignored: features.lean.frameRate must be"),
		"lean: a frame rate below 10 is refused" + Messages(r));
	r = Merge({{"typo", "{\"features\": {\"lean\": {\"image\": true}}}"}});
	Check(r.mFeatures.mLean && HasMessage(r.mStatus.mWarnings, "typo", "did you mean features.lean.images?"),
		"lean: unknown key" + Messages(r));
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

// GameRules::CheckRecorded, which a replay's RULE passes before it is played (stage 4 review): every rule set the
// mod path makes passes, with the original diff.txt values; a value no mod gives, one out of the mod path's
// ranges or spelling, and diff.txt values that make a level of no points fail
static void TestRecorded()
{
	std::vector<std::string> aSets = {"{}", "{\"tunables\": {\"scoring\": {\"hintPenalty\": 10.25, \"setValues\": [1, -2, 3]}}}",
		"{\"tunables\": {\"scoring\": {\"setValues\": [2000000000], \"hintPenalty\": -1e300}, \"timing\": {\"startDrainDelay\": -2147483648}}}",
		"{\"tunables\": {\"board\": {\"gemColors\": 3e9}}}", gTwistedQuest,
		"{\"rules\": {\"board\": {\"mask\": [\"#d##\", \"#29#\", \"####\", \"#..#\"], \"dirtPoints\": 7}, \"gold\": {\"countDirt\": false, "
		"\"timeLimit\": {\"normal\": {\"base\": 200, \"perLevel\": -3, \"min\": 20}, \"timed\": null}}}}",
		"{\"rules\": {\"board\": {\"gemColors\": 12}, \"match\": {\"minRun\": 4, \"diagonal\": true, \"diagonalMinRun\": 2, \"shapes\": {\"zz\": {\"cells\": [[3,1],[4,1],[3,2]], "
		"\"mirror\": true}, \"aa\": {\"preset\": \"T4\"}}, \"bonus\": {\"zz\": -4, \"aa\": 99}}, \"moves\": {\"type\": [\"slide\"], \"slide\": {\"snapTicks\": 30}, "
		"\"invalidPenalty\": 5, \"invalidCountsAsMove\": false, \"allowInvalid\": true}, \"noMoves\": {\"normal\": \"continue\", \"timed\": \"gameOver\"}, "
		"\"specials\": {\"chain\": false, \"createIn\": \"move\"}, \"undo\": {\"enabled\": true, \"max\": 3, \"inTimed\": true, \"restoreTime\": true, \"restoreRng\": false}, "
		"\"timing\": {\"warningFraction\": 0.3, \"startDrainDelay\": 1, \"levelDrainDelay\": 2}, \"scoring\": {\"excellentFactor\": 99.5, \"lengthBonus\": -1, "
		"\"longLengthBonus\": 0}}}",
		"{\"rules\": {\"board\": {\"levels\": [{\"cols\": 5, \"rows\": 7}, {\"mask\": [\"####\", \"#..#\", \"####\", \"####\"]}, {\"cols\": 16, \"rows\": 3}]}, "
		"\"gold\": {\"enabled\": true, \"timeLimit\": {\"normal\": {\"base\": 100, \"perLevel\": 0, \"min\": 10}}}, \"specials\": {\"flame\": {\"createdBy\": [\"run4\", \"sq\"], "
		"\"radius\": 3}}, \"match\": {\"shapes\": {\"sq\": {\"preset\": \"square\"}}}}}"};
	for (size_t i = 0; i < sizeof(gDesignExamples) / sizeof(gDesignExamples[0]); i++)
		aSets.push_back(gDesignExamples[i].mJson);
	for (size_t i = 0; i < aSets.size(); i++)
	{
		Result r = Merge({{"m", aSets[i].c_str()}});
		std::string anError;
		Check(r.mRules.CheckRecorded(&anError), "recorded: set " + std::to_string(i) + " passes (" + anError + ")");
		// as a replay has it: canonical bytes, decoded
		GameRules aDecoded;
		anError.clear();
		Check(aDecoded.FromCanonical(r.mRules.Canonical(), &anError) && aDecoded.CheckRecorded(&anError),
			"recorded: set " + std::to_string(i) + " decoded passes (" + anError + ")");
	}

	struct Case
	{
		const char*		mWhat;
		void			(*mBreak)(GameRules* r);
	};
	static const Case aCases[] =
	{
		{"normal points base 0", [](GameRules* r) { r->mDiff.mNormalPointsBase = 0; }},
		{"normal points base and per level 0", [](GameRules* r) { r->mDiff.mNormalPointsBase = 0; r->mDiff.mNormalPointsPerLevel = 0; }},
		{"timed points base -250", [](GameRules* r) { r->mDiff.mTimedPointsBase = -250; }},
		{"normal points per level -1", [](GameRules* r) { r->mDiff.mNormalPointsPerLevel = -1; }},
		{"normal multiplier base NaN", [](GameRules* r) { r->mDiff.mNormalMultBase = std::numeric_limits<double>::quiet_NaN(); }},
		{"timed multiplier base 0", [](GameRules* r) { r->mDiff.mTimedMultBase = 0.0; }},
		{"timed multiplier per level -0.5", [](GameRules* r) { r->mDiff.mTimedMultPerLevel = -0.5; }},
		{"normal points x multiplier below 1", [](GameRules* r) { r->mDiff.mNormalMultBase = 1e-9; }},
		{"time drain base infinite", [](GameRules* r) { r->mDiff.mTimeDrainBase = std::numeric_limits<double>::infinity(); }},
		{"time drain accel per level NaN", [](GameRules* r) { r->mDiff.mTimeDrainAccelPerLevel = std::numeric_limits<double>::quiet_NaN(); }},
		{"match.minRun 9", [](GameRules* r) { r->mMatch.mMinRun = 9; }},
		{"board.gemColors 0", [](GameRules* r) { r->mBoard.mGemColors = 0; }},
		{"board.cols 40", [](GameRules* r) { r->mBoard.mCols = 40; }},
		{"undo.depth 200 (the mod path clamps it)", [](GameRules* r) { r->mUndo.mEnabled = true; r->mUndo.mDepth = 200; }},
		{"scoring.hintPenalty NaN", [](GameRules* r) { r->mTunables.mHintPenalty = std::numeric_limits<double>::quiet_NaN(); }},
		{"scoring.excellentFactor infinite", [](GameRules* r) { r->mTunables.mExcellentFactor = std::numeric_limits<double>::infinity(); }},
		{"scoring.setValues empty", [](GameRules* r) { r->mTunables.mSetValues.clear(); }},
		{"an 8x8 mask of cells only (the mod path drops it)", [](GameRules* r) { r->mBoard.mMask.assign(8, "########"); }},
		{"a mask of 2 rows on 8 rows", [](GameRules* r) { r->mBoard.mMask.assign(2, "########"); }},
		{"createdBy out of order", [](GameRules* r) {
			r->mSpecials.mFlame.mEnabled = true; r->mSpecials.mFlame.mCreatedBy = {"run5", "run4"}; r->mSpecials.mFlame.mRadius = 1; }},
		{"a shape cell outside 0..15", [](GameRules* r) {
			ShapePattern aShape; aShape.mCells = {{0, 0}, {0, 99}}; r->mMatch.mShapes["far"] = aShape; r->mBoard.mGemColors = 12; }},
		{"33 shapes", [](GameRules* r) {
			r->mBoard.mGemColors = 12;
			for (int i = 0; i < 33; i++)
			{
				ShapePattern aShape;
				aShape.mCells = {{0, 0}, {i % 16, 1 + i / 16}};
				r->mMatch.mShapes[std::string("s") + std::to_string(i)] = aShape;
			}
		}},
		{"moves.type none", [](GameRules* r) { r->mMoves.mTypes = 0; }},
		{"noMoves.timed 7", [](GameRules* r) { r->mNoMoves.mTimed = 7; }},
		{"65 levels", [](GameRules* r) { r->mBoard.mLevels.assign(65, LevelShape{8, 8, {}}); }},
	};
	for (size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); i++)
	{
		GameRules aRules = Merge({}).mRules;
		aCases[i].mBreak(&aRules);
		std::string anError;
		Check(!aRules.CheckRecorded(&anError) && !anError.empty(), std::string("recorded: refused: ") + aCases[i].mWhat + " (" + anError + ")");
	}

	// The shape cap in the mod path too (a bound on Validate's FillMinimum, whose work grows with shapes x levels).
	// So many shapes never fill with 12 colours anyway: at MAX_SHAPES the colour minimum drops the mod; past it the
	// mod's rules are refused as they are parsed, and a merged set past it drops a contributor before FillMinimum
	auto Shapes = [](int theFirst, int theCount)
	{
		std::string aJson = "{\"rules\": {\"board\": {\"gemColors\": 12}, \"match\": {\"shapes\": {";
		for (int i = theFirst; i < theFirst + theCount; i++)
			aJson += std::string((i > theFirst) ? ", " : "") + "\"s" + std::to_string(i) + "\": {\"cells\": [[0,0],[" + std::to_string(i % 16) + "," +
				std::to_string(1 + i / 16) + "]]}";
		return aJson + "}}}}";
	};
	std::string aMost = Shapes(0, GameRules::MAX_SHAPES);
	Result r = Merge({{"most", aMost.c_str()}});
	Check(HasMessage(r.mStatus.mDropped, "most", "fewest colours"), "MAX_SHAPES shapes: the colour minimum" + Messages(r));
	std::string aMany = Shapes(0, GameRules::MAX_SHAPES + 1);
	r = Merge({{"many", aMany.c_str()}});
	Check(HasMessage(r.mStatus.mErrors, "many", "more than the 32"), "MAX_SHAPES + 1 shapes in one mod: refused" + Messages(r));
	std::string aLow = Shapes(0, 20);
	std::string aHigh = Shapes(20, 20);
	r = Merge({{"low", aLow.c_str()}, {"high", aHigh.c_str()}});
	Check(HasMessage(r.mStatus.mDropped, "low", "more than the 32"), "40 shapes from two mods: the lower one dropped for the count" + Messages(r));
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

static Replay::Event MakeEvent(uint32_t theTick, int theOp, int64_t a0 = 0, int64_t a1 = 0, int64_t a2 = 0, int64_t a3 = 0, int64_t a4 = 0)
{
	Replay::Event anEvent;
	anEvent.mTick = theTick;
	anEvent.mOp = theOp;
	int64_t anArgs[5] = {a0, a1, a2, a3, a4};
	for (int i = 0; i < 5; i++)
		anEvent.mArgs[i] = (i < Replay::OpArgCount(theOp)) ? anArgs[i] : 0;
	return anEvent;
}

static bool SameRng(const Replay::RngState& a, const Replay::RngState& b)
{
	return (a.mSeed == b.mSeed) && (a.mUserSeeded == b.mUserSeeded) && (a.mRanked == b.mRanked) && (a.mMti == b.mMti) &&
		(memcmp(a.mMt, b.mMt, sizeof(a.mMt)) == 0);
}

static bool SameFile(const Replay::File& a, const Replay::File& b, std::string* theWhat)
{
	if ((a.mFormat != b.mFormat) || (a.mFlags != b.mFlags) || (a.mEngineAbi != b.mEngineAbi))
		*theWhat = "header";
	else if ((a.mHasRules != b.mHasRules) || (a.mRules != b.mRules) || (a.mHasRulesId != b.mHasRulesId) ||
		(memcmp(a.mRulesId, b.mRulesId, sizeof(a.mRulesId)) != 0))
		*theWhat = "RULE / RLID";
	else if ((a.mHasMeta != b.mHasMeta) || (a.mMeta != b.mMeta) || (a.mHasSession != b.mHasSession) || (a.mSession != b.mSession))
		*theWhat = "META / SESS";
	else if (a.mSegments.size() != b.mSegments.size())
		*theWhat = "segment count";
	else if ((a.mHasTrailingRng != b.mHasTrailingRng) || (a.mHasTrailingRng && !SameRng(a.mTrailingRng, b.mTrailingRng)))
		*theWhat = "trailing RNGS";
	else if ((a.mHasSaveHash != b.mHasSaveHash) || (a.mSaveHash != b.mSaveHash))
		*theWhat = "SVHA";
	else if ((a.mEnd.mReason != b.mEnd.mReason) || (a.mEnd.mEndTick != b.mEnd.mEndTick) || (a.mEnd.mScore != b.mEnd.mScore) ||
		(a.mEnd.mLevel != b.mEnd.mLevel) || (a.mEnd.mGems != b.mEnd.mGems) || (a.mEnd.mLogicHash != b.mEnd.mLogicHash) ||
		(a.mEnd.mBoardDraws != b.mEnd.mBoardDraws))
		*theWhat = "END";
	for (size_t s = 0; theWhat->empty() && (s < a.mSegments.size()); s++)
	{
		const Replay::Segment& x = a.mSegments[s];
		const Replay::Segment& y = b.mSegments[s];
		bool isSame = (x.mMode == y.mMode) && (x.mRng.mKind == y.mRng.mKind) && (x.mUserSeeded == y.mUserSeeded) &&
			(x.mShowInstructions == y.mShowInstructions) && (x.mTimesPlayed == y.mTimesPlayed) && (x.mPointsAscent == y.mPointsAscent) &&
			(x.mHasLoadRng == y.mHasLoadRng) && (!x.mHasLoadRng || SameRng(x.mLoadRng, y.mLoadRng)) &&
			(x.mHasLoadSave == y.mHasLoadSave) && (x.mLoadSave == y.mLoadSave) && (x.mEvents.size() == y.mEvents.size());
		if (isSame && (x.mRng.mKind == Replay::RNG_SEED))
			isSame = (x.mRng.mSeed == y.mRng.mSeed);
		else if (isSame)
			isSame = (x.mRng.mMti == y.mRng.mMti) && (memcmp(x.mRng.mMt, y.mRng.mMt, sizeof(x.mRng.mMt)) == 0);
		for (size_t e = 0; isSame && (e < x.mEvents.size()); e++)
		{
			isSame = (x.mEvents[e].mTick == y.mEvents[e].mTick) && (x.mEvents[e].mOp == y.mEvents[e].mOp);
			for (int i = 0; isSame && (i < Replay::MAX_ARGS); i++)
				isSame = (x.mEvents[e].mArgs[i] == y.mEvents[e].mArgs[i]);
		}
		if (!isSame)
			*theWhat = "segment " + std::to_string(s);
	}
	return theWhat->empty();
}

// A file with every record and every op, the arguments at their extremes
static Replay::File FullReplay()
{
	Replay::File aFile;
	aFile.mFlags = Replay::FLAG_HOVER | Replay::FLAG_MULTI_SEGMENT;
	aFile.mHasRules = true;
	aFile.mRules = std::string("\x01\x00\x00\x00rules", 9);
	aFile.mHasRulesId = true;
	Sha256::Hash(aFile.mRules.data(), aFile.mRules.size(), aFile.mRulesId);
	aFile.mHasMeta = true;
	aFile.mMeta = "{\"name\":\"T\\u00e9st\",\"date\":\"2026-10-18T12:00:00\"}";
	aFile.mHasSession = true;
	aFile.mSession = "token";

	Replay::Segment aFirst;
	aFirst.mMode = 0;
	aFirst.mRng.mKind = Replay::RNG_SEED;
	aFirst.mRng.mSeed = 4294967295u;
	aFirst.mUserSeeded = true;
	aFirst.mShowInstructions = true;
	aFirst.mTimesPlayed = -7;
	aFirst.mPointsAscent = 32;
	aFirst.mEvents.push_back(MakeEvent(0, Replay::OP_PAUSE, 0));
	aFirst.mEvents.push_back(MakeEvent(0, Replay::OP_UNPAUSE, 1));
	aFirst.mEvents.push_back(MakeEvent(139, Replay::OP_CHECK, (int64_t) 0xfedcba9876543210ull));
	aFirst.mEvents.push_back(MakeEvent(160, Replay::OP_SELECT, 5, 5));
	aFirst.mEvents.push_back(MakeEvent(161, Replay::OP_TARGET, 6, 5));
	aFirst.mEvents.push_back(MakeEvent(161, Replay::OP_TARGET, -1, -1));
	aFirst.mEvents.push_back(MakeEvent(200, Replay::OP_DESELECT));
	aFirst.mEvents.push_back(MakeEvent(201, Replay::OP_TWIST, 3, 4, -1));
	aFirst.mEvents.push_back(MakeEvent(202, Replay::OP_SLIDE, 1, 7, 0, 3, -51));
	aFirst.mEvents.push_back(MakeEvent(300, Replay::OP_HINT));
	aFirst.mEvents.push_back(MakeEvent(301, Replay::OP_UNDO));
	aFirst.mEvents.push_back(MakeEvent(302, Replay::OP_XRAND, 1));
	aFirst.mEvents.push_back(MakeEvent(303, Replay::OP_HOVER, -1, -1));
	aFirst.mEvents.push_back(MakeEvent(70000, Replay::OP_NEW_GAME));
	aFirst.mEvents.push_back(MakeEvent(4000000000u, Replay::OP_SAVE, Replay::SAVE_GAME, 4294967295ll));
	aFirst.mEvents.push_back(MakeEvent(4000000000u, Replay::OP_CHECK, INT64_MIN));
	aFirst.mEvents.push_back(MakeEvent(4294967295u, Replay::OP_CHECK, INT64_MAX));
	aFile.mSegments.push_back(aFirst);

	Replay::Segment aSecond;
	aSecond.mMode = 1;
	aSecond.mRng.mKind = Replay::RNG_STATE;
	MTRand(5489).GetState(aSecond.mRng.mMt, &aSecond.mRng.mMti);
	aSecond.mHasLoadSave = true;
	aSecond.mLoadSave = std::string("\x01\x00\x00\x00save", 8) + std::string(1, '\0') + "bytes";
	aSecond.mHasLoadRng = true;
	aSecond.mLoadRng.mSeed = 4242;
	aSecond.mLoadRng.mUserSeeded = true;
	MTRand(4242).GetState(aSecond.mLoadRng.mMt, &aSecond.mLoadRng.mMti);
	aSecond.mEvents.push_back(MakeEvent(0, Replay::OP_LOAD));
	aSecond.mEvents.push_back(MakeEvent(1, Replay::OP_CHECK, 1));
	aFile.mSegments.push_back(aSecond);

	aFile.mHasTrailingRng = true;
	aFile.mTrailingRng.mSeed = 1;
	aFile.mTrailingRng.mRanked = true;
	MTRand(1).GetState(aFile.mTrailingRng.mMt, &aFile.mTrailingRng.mMti);
	aFile.mHasSaveHash = true;
	aFile.mSaveHash = 0x6a004117;
	aFile.mEnd.mReason = Replay::END_SAVED;
	aFile.mEnd.mEndTick = 260;
	aFile.mEnd.mScore = -5;
	aFile.mEnd.mLevel = 2;
	aFile.mEnd.mGems = 25;
	aFile.mEnd.mLogicHash = 0x8000000000000001ull;
	aFile.mEnd.mBoardDraws = 4294967295u;
	return aFile;
}

static void TestReplayVarints()
{
	const int64_t aValues[] = {0, 1, -1, 63, -64, 64, -65, 127, 128, 300, -300, 2147483647, -2147483647 - 1, 4294967295ll,
		INT64_MAX, INT64_MIN, INT64_MIN + 1};
	bool isOk = true;
	for (size_t i = 0; i < sizeof(aValues) / sizeof(aValues[0]); i++)
	{
		std::string aBytes;
		Replay::PutVarint(&aBytes, Replay::ZigZag(aValues[i]));
		size_t aPos = 0;
		uint64_t aRead = 0;
		isOk = isOk && Replay::GetVarint(aBytes, &aPos, aBytes.size(), &aRead) && (aPos == aBytes.size()) &&
			(Replay::UnZigZag(aRead) == aValues[i]) && (aBytes.size() <= 10);
	}
	Check(isOk, "replay: zigzag varints round trip 0, +-1, the 7-bit edges, the i32 and i64 extremes");
	Check((Replay::ZigZag(0) == 0) && (Replay::ZigZag(-1) == 1) && (Replay::ZigZag(1) == 2) && (Replay::ZigZag(-2) == 3) &&
		(Replay::ZigZag(INT64_MIN) == UINT64_MAX), "replay: zigzag maps 0, -1, 1, -2, INT64_MIN to 0, 1, 2, 3, 2^64-1");

	std::string aMax;
	Replay::PutVarint(&aMax, UINT64_MAX);
	Check(Hex(aMax) == "ffffffffffffffffff01", "replay: 2^64-1 is the 10 bytes ff x9 01 (got " + Hex(aMax) + ")");
	size_t aPos = 0;
	uint64_t aValue = 0;
	std::string anOver = std::string(9, '\xff') + "\x02";
	Check(!Replay::GetVarint(anOver, &aPos, anOver.size(), &aValue) && (aPos == 0), "replay: a tenth varint byte over 1 is refused");
	std::string aLong = std::string(10, '\x80') + std::string(1, '\0');
	Check(!Replay::GetVarint(aLong, &aPos, aLong.size(), &aValue), "replay: an 11-byte varint is refused");
	std::string aCut = "\x80\x80";
	Check(!Replay::GetVarint(aCut, &aPos, aCut.size(), &aValue) && !Replay::GetVarint(aMax, &aPos, 5, &aValue),
		"replay: a varint cut off by the end (of the data, or of the record) is refused");
}

static void TestReplayFormat()
{
	// SaveHash: SHA-256("abc") = ba7816bf..., little-endian
	Check(Replay::SaveHash("abc", 3) == 0xbf1678bau, "replay: SaveHash(\"abc\") is SHA-256's first 4 bytes, little-endian");

	// Round trip: every record, every op
	Replay::File aFile = FullReplay();
	std::string aBytes = Replay::Write(aFile);
	Replay::File aRead;
	std::string anError;
	std::string aWhat;
	bool isRead = Replay::Read(aBytes, &aRead, &anError);
	Check(isRead && SameFile(aFile, aRead, &aWhat), "replay: write, read: every record and op as written (" + anError + aWhat + ")");
	Check(isRead && (Replay::Write(aRead) == aBytes), "replay: read, write: the same bytes");
	Check((Replay::CountMoves(aRead.mSegments[0]) == 3) && (Replay::CountMoves(aRead.mSegments[1]) == 0),
		"replay: CountMoves counts the TARGETs that name a cell, the TWISTs and the SLIDEs");
	bool areCounted = true;
	for (int anOp = 1; anOp < Replay::NUM_OPS; anOp++)
		areCounted = areCounted && (Replay::OpArgCount(anOp) >= 0) && (strcmp(Replay::OpName(anOp), "?") != 0);
	Check(areCounted && (Replay::OpArgCount(0) < 0) && (Replay::OpArgCount(Replay::NUM_OPS) < 0),
		"replay: every op 1..15 has a name and an argument count, 0 and 16 none");

	// A golden vector: the smallest file (header and END) and a one-segment file, byte for byte, the same on
	// native and wasm (engine ABI 10 since stage 18)
	Replay::File anEmpty;
	Check(Hex(Replay::Write(anEmpty)) == "424a5250010000000a00000045" "4e4420" "1d000000" "00" "00000000" "00000000" "00000000"
		"00000000" "0000000000000000" "00000000", "replay: golden bytes of a file with only END");
	Replay::File aSmall;
	Replay::Segment aSegment;
	aSegment.mRng.mSeed = 4242;
	aSegment.mEvents.push_back(MakeEvent(160, Replay::OP_SELECT, 5, 5));
	aSegment.mEvents.push_back(MakeEvent(161, Replay::OP_TARGET, -1, -1));
	aSmall.mSegments.push_back(aSegment);
	aSmall.mEnd.mReason = Replay::END_NO_MOVES;
	aSmall.mEnd.mScore = 90;
	std::string aSmallHex = Hex(Replay::Write(aSmall));
	Check(aSmallHex == "424a5250010000000a000000" "5345474d" "1d000000" "00" "00" "92100000" "00" "00" "00000000" "00000000"
		"02000000" "a001" "01" "0a" "0a" "01" "03" "01" "01"
		"454e4420" "1d000000" "01" "00000000" "5a000000" "00000000" "00000000" "0000000000000000" "00000000",
		"replay: golden bytes of a one-segment file (got " + aSmallHex + ")");

	// Every truncation fails: END is last, so even a cut at a record boundary is caught
	int aParsed = 0;
	for (size_t aLen = 0; aLen < aBytes.size(); aLen++)
	{
		Replay::File aCut;
		if (Replay::Read(aBytes.substr(0, aLen), &aCut))
			aParsed++;
	}
	Check(aParsed == 0, "replay: each of the " + std::to_string(aBytes.size()) + " truncations is refused (" + std::to_string(aParsed) + " read)");

	// Corrupt input: single-byte changes never crash the reader; what it still reads writes back readable
	int aRefused = 0;
	bool isStable = true;
	for (size_t i = 0; i < aBytes.size(); i += (i < 200) ? 1 : 7)
	{
		for (int aMask = 1; aMask < 256; aMask <<= 3)
		{
			std::string aBad = aBytes;
			aBad[i] = (char) (aBad[i] ^ aMask);
			Replay::File aMaybe;
			if (!Replay::Read(aBad, &aMaybe))
				aRefused++;
			else
			{
				Replay::File anAgain;
				isStable = isStable && Replay::Read(Replay::Write(aMaybe), &anAgain);
			}
		}
	}
	Check(isStable && (aRefused > 0), "replay: flipped bits never crash the reader, " + std::to_string(aRefused) +
		" corruptions refused, the rest read back after a write");

	// Specific corruptions
	struct Bad { const char* mWhat; size_t mAt; std::string mBytes; };
	std::string aSmallBytes = Replay::Write(aSmall);
	const size_t aSegmPayload = 12 + 8;                    // the SEGM payload in aSmall
	const size_t aCountAt = aSegmPayload + 1 + 1 + 4 + 1 + 1 + 4 + 4;
	const size_t aFirstOpAt = aCountAt + 4 + 2;
	const Bad aBads[] =
	{
		{"a wrong magic", 0, "X"},
		{"format 2", 4, std::string("\x02", 1)},
		{"a record longer than the file", 16, std::string("\xff\xff\x00\x00", 4)},
		{"an event count of 2^32-1", aCountAt, std::string("\xff\xff\xff\xff", 4)},
		{"one event more than the payload holds", aCountAt, std::string("\x03", 1)},
		{"an unknown op", aFirstOpAt, std::string("\x10", 1)},
		{"op 0", aFirstOpAt, std::string("\x00", 1)},
		{"an RNG start kind of 2", aSegmPayload + 1, std::string("\x02", 1)},
	};
	for (size_t i = 0; i < sizeof(aBads) / sizeof(aBads[0]); i++)
	{
		std::string aBad = aSmallBytes;
		aBad.replace(aBads[i].mAt, aBads[i].mBytes.size(), aBads[i].mBytes);
		Replay::File aNot;
		std::string aWhy;
		bool isRefused = !Replay::Read(aBad, &aNot, &aWhy);
		Check(isRefused && !aWhy.empty(), std::string("replay: ") + aBads[i].mWhat + " is refused (" + aWhy + ")");
	}
	Replay::File aNot;
	std::string aWhy;
	Check(!Replay::Read(aSmallBytes + "x", &aNot, &aWhy), "replay: data after END is refused (" + aWhy + ")");
	std::string aTwoEnds = aSmallBytes + aSmallBytes.substr(aSmallBytes.size() - 37);
	Check(!Replay::Read(aTwoEnds, &aNot, &aWhy), "replay: a second END is refused (" + aWhy + ")");
	// An unknown record is skipped (a later format addition)
	std::string anExtra = aSmallBytes.substr(0, aSmallBytes.size() - 37) + "XTRA" + std::string("\x03\x00\x00\x00", 4) + "abc" +
		aSmallBytes.substr(aSmallBytes.size() - 37);
	aWhy.clear();
	Check(Replay::Read(anExtra, &aNot, &aWhy) && (Replay::Write(aNot) == aSmallBytes), "replay: an unknown record is skipped (" + aWhy + ")");
	// A RNGS or an RLID of the wrong size
	std::string aShortRng = aSmallBytes.substr(0, 12) + "RNGS" + std::string("\x04\x00\x00\x00", 4) + "abcd" + aSmallBytes.substr(12);
	Check(!Replay::Read(aShortRng, &aNot, &aWhy), "replay: an RNGS of 4 bytes is refused (" + aWhy + ")");
	std::string aShortId = aSmallBytes.substr(0, 12) + "RLID" + std::string("\x04\x00\x00\x00", 4) + "abcd" + aSmallBytes.substr(12);
	Check(!Replay::Read(aShortId, &aNot, &aWhy), "replay: an RLID of 4 bytes is refused (" + aWhy + ")");
}

// A v1 save as Board::WriteSaveGame writes one: an 8x8 game in play, every cell present (7873 bytes). Its
// layout for the tests below: the header (version, timed, level, display level) is 13 bytes, each cell a
// present byte and the 0x78 Piece bytes, then the 116 bytes of the board's other fields.
enum { SAVE_CELLS_AT = 13, SAVE_CELL_SIZE = 1 + Piece::SAVE_SIZE, SAVE_TAIL_AT = SAVE_CELLS_AT + 64 * SAVE_CELL_SIZE };

static size_t SavePieceAt(int theCol, int theRow, size_t theOffset)
{
	return SAVE_CELLS_AT + (theRow * 8 + theCol) * SAVE_CELL_SIZE + 1 + theOffset;
}

static std::string V1Save(int theCols = 8, int theRows = 8)
{
	std::string s;
	auto I32 = [&s](int32_t v) { s.append((const char*) &v, 4); };
	auto U8 = [&s](int v) { s.push_back((char) v); };
	auto F64 = [&s](double v) { s.append((const char*) &v, 8); };
	I32(1);
	U8(0);
	I32(0);
	I32(1);
	for (int r = 0; r < theRows; r++)
	{
		for (int c = 0; c < theCols; c++)
		{
			U8(1);
			unsigned char aPiece[Piece::SAVE_SIZE] = {};
			int32_t aColor = (r * 3 + c) % 7;
			int32_t aCol = c;
			int32_t aRow = r;
			double aX = 203 + 51 * c;
			double aY = 16 + 51 * r;
			double anAlpha = -0.011;
			memcpy(aPiece + offsetof(Piece, mColor), &aColor, 4);
			memcpy(aPiece + offsetof(Piece, mX), &aX, 8);
			memcpy(aPiece + offsetof(Piece, mY), &aY, 8);
			memcpy(aPiece + offsetof(Piece, mCol), &aCol, 4);
			memcpy(aPiece + offsetof(Piece, mRow), &aRow, 4);
			memcpy(aPiece + offsetof(Piece, mGlowAlpha), &anAlpha, 8);
			memset(aPiece + offsetof(Piece, mPad39), 0xcd, 7); // padding: anything
			s.append((const char*) aPiece, sizeof(aPiece));
		}
	}
	I32(1);          // STATE_PLAYING
	U8(0);
	U8(0);
	I32(170);        // mBarWidth
	I32(12);
	I32(30);
	I32(150);        // mScore
	F64(1.0);
	I32(150);
	I32(9);
	I32(3);
	I32(1);
	I32(1);
	I32(150);
	U8(0);
	I32(-559038737); // mUnused264: anything
	I32(0);
	I32(1000);       // mLevelTargetScore
	I32(1000);
	F64(0.0);
	F64(0.0);
	F64(0.0);
	I32(23);         // mBarColor
	I32(0);
	I32(0);
	F64(0.0);
	U8(0);
	return s;
}

// Replay::CheckSave: a save the game makes passes; each value out of range that the original LoadGame would
// take on trust, and Draw or an update would index, loop or divide with, is refused
static void TestReplaySave()
{
	std::string aWhy;
	std::string aGood = V1Save();
	Check(aGood.size() == 7873, "save: the test save is the 7873 bytes of a full 8x8 board (" + std::to_string(aGood.size()) + ")");
	Check(Replay::CheckSave(aGood, 8, 8, &aWhy), "save: a game in play passes (" + aWhy + ")");
	std::string aSwapping = aGood;
	int32_t aState = 3;
	memcpy(&aSwapping[SAVE_TAIL_AT], &aState, 4);
	Check(Replay::CheckSave(aSwapping, 8, 8, &aWhy), "save: a swap going back passes (LoadGame makes it STATE_PLAYING) (" + aWhy + ")");
	// A cell without a gem: its present byte 0 and no Piece bytes
	std::string aHole = aGood.substr(0, SAVE_CELLS_AT) + std::string(1, '\0') + aGood.substr(SAVE_CELLS_AT + SAVE_CELL_SIZE);
	Check(Replay::CheckSave(aHole, 8, 8, &aWhy), "save: an empty cell passes (" + aWhy + ")");

	struct Bad { const char* mWhat; size_t mAt; std::string mBytes; };
	auto I32 = [](int32_t v) { return std::string((const char*) &v, 4); };
	auto F64 = [](double v) { return std::string((const char*) &v, 8); };
	const size_t T = SAVE_TAIL_AT;
	const Bad aBads[] =
	{
		{"version 2", 0, I32(2)},
		{"timed 2", 4, std::string(1, '\2')},
		{"level -1", 5, I32(-1)},
		{"display level 0", 9, I32(0)},
		{"a present byte of 2", SAVE_CELLS_AT, std::string(1, '\2')},
		{"colour 7", SavePieceAt(0, 0, offsetof(Piece, mColor)), I32(7)},
		{"colour 0x01000000", SavePieceAt(5, 6, offsetof(Piece, mColor)), I32(0x01000000)},
		{"a piece that says it is in another column", SavePieceAt(3, 2, offsetof(Piece, mCol)), I32(4)},
		{"a piece that says it is in row 8", SavePieceAt(3, 7, offsetof(Piece, mRow)), I32(8)},
		{"mVelY NaN", SavePieceAt(1, 1, offsetof(Piece, mVelY)), F64(std::nan(""))},
		{"mGlowing 2", SavePieceAt(1, 1, offsetof(Piece, mGlowing)), std::string(1, '\2')},
		{"mGlowAlpha 1.5", SavePieceAt(1, 1, offsetof(Piece, mGlowAlpha)), F64(1.5)},
		{"mGlowFrame 15", SavePieceAt(1, 1, offsetof(Piece, mGlowFrame)), I32(15)},
		{"mDying 2", SavePieceAt(1, 1, offsetof(Piece, mDying)), std::string(1, '\2')},
		{"mDyingFrame 6", SavePieceAt(7, 7, offsetof(Piece, mDyingFrame)), I32(6)},
		{"mSpinFrame 15", SavePieceAt(1, 1, offsetof(Piece, mSpinFrame)), I32(15)},
		{"mSparkleCount 59", SavePieceAt(1, 1, offsetof(Piece, mSparkleCount)), I32(59)},
		{"mSparkleFrame -1", SavePieceAt(1, 1, offsetof(Piece, mSparkleFrame)), I32(-1)},
		{"mHoleAngle infinite", SavePieceAt(1, 1, offsetof(Piece, mHoleAngle)), F64(INFINITY)},
		{"mHoleRadius -1", SavePieceAt(1, 1, offsetof(Piece, mHoleRadius)), F64(-1.0)},
		{"state 5 (game over)", T, I32(5)},
		{"state 10", T, I32(10)},
		{"the two timed bytes differ", T + 4, std::string(1, '\1')},
		{"bar width 341", T + 6, I32(341)},
		{"score -1", T + 18, I32(-1)},
		{"multiplier 0", T + 22, F64(0.0)},
		{"level start score = target", T + 59, I32(1000)},
		{"time drain NaN", T + 71, F64(std::nan(""))},
		{"bar colour 25", T + 95, I32(25)},
		{"wipe -3", T + 99, I32(-3)},
		{"black hole frame 9", T + 103, I32(9)},
		{"black hole level 1.5", T + 107, F64(1.5)},
		{"level text shown 2", T + 115, std::string(1, '\2')},
	};
	for (size_t i = 0; i < sizeof(aBads) / sizeof(aBads[0]); i++)
	{
		std::string aBad = aGood;
		aBad.replace(aBads[i].mAt, aBads[i].mBytes.size(), aBads[i].mBytes);
		aWhy.clear();
		Check(!Replay::CheckSave(aBad, 8, 8, &aWhy) && !aWhy.empty(), std::string("save: refused: ") + aBads[i].mWhat + " (" + aWhy + ")");
	}
	aWhy.clear();
	Check(!Replay::CheckSave(aGood + "x", 8, 8, &aWhy), "save: refused: a byte after the last field (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(aGood.substr(0, aGood.size() - 1), 8, 8, &aWhy), "save: refused: one byte short (" + aWhy + ")");
	int aRead = 0;
	for (size_t aLen = 0; aLen < aGood.size(); aLen += 13)
		aRead += Replay::CheckSave(aGood.substr(0, aLen), 8, 8) ? 1 : 0;
	Check(aRead == 0, "save: every cut is refused (" + std::to_string(aRead) + " passed)");
}

static Replay::File PlayableReplay();

// The record order (Replay.h), the END reason, and what the reader and the player take on at most
static void TestReplayLimits()
{
	Replay::File aSmall;
	Replay::Segment aSegment;
	aSegment.mRng.mSeed = 4242;
	aSegment.mEvents.push_back(MakeEvent(160, Replay::OP_SELECT, 5, 5));
	aSmall.mSegments.push_back(aSegment);
	aSmall.mEnd.mReason = Replay::END_NO_MOVES;
	std::string aSmallBytes = Replay::Write(aSmall);
	std::string aHeader = aSmallBytes.substr(0, 12);
	size_t anEndAt = aSmallBytes.size() - 37;
	std::string aSegm = aSmallBytes.substr(12, anEndAt - 12);
	std::string anEnd = aSmallBytes.substr(anEndAt);
	auto Rec = [](const char* theTag, const std::string& thePayload)
	{
		uint32_t aLen = (uint32_t) thePayload.size();
		return std::string(theTag, 4) + std::string((const char*) &aLen, 4) + thePayload;
	};
	Replay::File aWithRng;
	aWithRng.mHasTrailingRng = true;
	MTRand(9).GetState(aWithRng.mTrailingRng.mMt, &aWithRng.mTrailingRng.mMti);
	std::string anRngs = Replay::Write(aWithRng);
	anRngs = anRngs.substr(12, anRngs.size() - 12 - 37);
	std::string aRule = Rec("RULE", "rules");
	std::string anId = Rec("RLID", std::string(32, 'i'));
	std::string aMeta = Rec("META", "{}");
	std::string aSvha = Rec("SVHA", std::string(4, 's'));
	std::string aLdsv = Rec("LDSV", "save");

	Replay::File aNot;
	std::string aWhy;
	Check(Replay::Read(aHeader + aRule + anId + aMeta + aLdsv + anRngs + aSegm + anRngs + aSvha + anEnd, &aNot, &aWhy),
		"limits: every record once, in order, reads (" + aWhy + ")");
	struct Bad { const char* mWhat; std::string mBytes; };
	const Bad aBads[] =
	{
		{"RULE twice", aHeader + aRule + aRule + aSegm + anEnd},
		{"RLID before RULE", aHeader + anId + aRule + aSegm + anEnd},
		{"META after a SEGM", aHeader + aRule + aSegm + aMeta + anEnd},
		{"RULE after an RNGS", aHeader + anRngs + aRule + aSegm + anEnd},
		{"two RNGS before a SEGM", aHeader + anRngs + anRngs + aSegm + anEnd},
		{"an RNGS before an LDSV", aHeader + anRngs + aLdsv + aSegm + anEnd},
		{"two LDSV before a SEGM", aHeader + aLdsv + aLdsv + aSegm + anEnd},
		{"an LDSV after the last SEGM", aHeader + aSegm + aLdsv + anEnd},
		{"an LDSV before SVHA", aHeader + aSegm + aLdsv + aSvha + anEnd},
		{"two SVHA", aHeader + aSegm + aSvha + aSvha + anEnd},
		{"an RNGS after SVHA", aHeader + aSegm + aSvha + anRngs + anEnd},
		{"a SEGM after SVHA", aHeader + aSegm + aSvha + aSegm + anEnd},
		{"END reason 7", aHeader + aSegm + anEnd.substr(0, 8) + std::string(1, '\7') + anEnd.substr(9)},
		{"END reason 255", aHeader + aSegm + anEnd.substr(0, 8) + std::string(1, '\xff') + anEnd.substr(9)},
	};
	for (size_t i = 0; i < sizeof(aBads) / sizeof(aBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::Read(aBads[i].mBytes, &aNot, &aWhy) && !aWhy.empty(), std::string("limits: refused: ") + aBads[i].mWhat + " (" + aWhy + ")");
	}
	aWhy.clear();
	Check(Replay::Read(aHeader + aRule + Rec("XTRA", "x") + anId + aSegm + Rec("XTRA", "") + anEnd, &aNot, &aWhy),
		"limits: unknown records are skipped anywhere (" + aWhy + ")");

	// Sizes: a file over MAX_FILE_SIZE, and a SEGM with more events than a replay may have (refused before it
	// allocates for them: the payload holds 2 bytes for each)
	std::string aBig = aSmallBytes.substr(0, anEndAt) + Rec("XTRA", std::string(Replay::MAX_FILE_SIZE - aSmallBytes.size() - 7, 'x')) + anEnd;
	aWhy.clear();
	Check((aBig.size() == (size_t) Replay::MAX_FILE_SIZE + 1) && !Replay::Read(aBig, &aNot, &aWhy),
		"limits: refused: a file of MAX_FILE_SIZE + 1 bytes (" + aWhy + ")");
	aBig = aSmallBytes.substr(0, anEndAt) + Rec("XTRA", std::string(Replay::MAX_FILE_SIZE - aSmallBytes.size() - 8, 'x')) + anEnd;
	aWhy.clear();
	Check((aBig.size() == (size_t) Replay::MAX_FILE_SIZE) && Replay::Read(aBig, &aNot, &aWhy),
		"limits: a file of MAX_FILE_SIZE bytes reads (" + aWhy + ")");
	aBig.clear();
	aBig.shrink_to_fit();
	std::string aMany = aSegm.substr(8, 1 + 1 + 4 + 1 + 1 + 4 + 4);
	uint32_t aCount = Replay::MAX_EVENTS + 1;
	aMany += std::string((const char*) &aCount, 4) + std::string(2 * (size_t) aCount, '\1');
	aWhy.clear();
	Check(!Replay::Read(aHeader + Rec("SEGM", aMany) + anEnd, &aNot, &aWhy), "limits: refused: a SEGM of MAX_EVENTS + 1 events (" + aWhy + ")");

	// Segments: each one holds two MT states (5 KB) however small its record, so the reader takes at most
	// MAX_SEGMENTS (a minimal SEGM is 28 bytes: a 256 KB file could ask for 9,000)
	std::string aSegments;
	for (int i = 0; i < Replay::MAX_SEGMENTS; i++)
		aSegments += aSegm;
	aWhy.clear();
	Check(Replay::Read(aHeader + aSegments + anEnd, &aNot, &aWhy) && (aNot.mSegments.size() == (size_t) Replay::MAX_SEGMENTS),
		"limits: MAX_SEGMENTS segments read (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::Read(aHeader + aSegments + aSegm + anEnd, &aNot, &aWhy) && (aWhy.find("segments a replay may have") != std::string::npos),
		"limits: refused: MAX_SEGMENTS + 1 segments (" + aWhy + ")");
	// Events at one tick: refused while they are parsed, as CheckPlayable refuses them (a file of millions of
	// events at one tick stops after MAX_EVENTS_PER_TICK + 1)
	auto Busy = [&](uint32_t theCount)
	{
		std::string aPayload = aSegm.substr(8, 1 + 1 + 4 + 1 + 1 + 4 + 4);
		aPayload += std::string((const char*) &theCount, 4);
		aPayload += std::string("\x05\x02", 2);                     // tick 5, DESELECT
		for (uint32_t i = 1; i < theCount; i++)
			aPayload += std::string("\x00\x02", 2);                 // the same tick
		return aHeader + Rec("SEGM", aPayload) + anEnd;
	};
	aWhy.clear();
	Check(Replay::Read(Busy(Replay::MAX_EVENTS_PER_TICK), &aNot, &aWhy), "limits: MAX_EVENTS_PER_TICK events at one tick read (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::Read(Busy(Replay::MAX_EVENTS_PER_TICK + 1), &aNot, &aWhy) && (aWhy.find("events at one tick") != std::string::npos) &&
		(aNot.mSegments.empty() || (aNot.mSegments.back().mEvents.size() <= (size_t) Replay::MAX_EVENTS_PER_TICK + 1)),
		"limits: refused while reading: one event more at that tick (" + aWhy + ")");

	// The player: the END tick, events at one tick, outside draws in all
	Replay::File aGood = PlayableReplay();
	aWhy.clear();
	Check(Replay::CheckPlayable(aGood, 8, 8, &aWhy), "limits: the playable replay (" + aWhy + ")");
	Replay::File aLong = aGood;
	aLong.mEnd.mEndTick = Replay::MAX_TICKS + 1;
	aWhy.clear();
	Check(!Replay::CheckPlayable(aLong, 8, 8, &aWhy), "limits: refused: END past MAX_TICKS (" + aWhy + ")");
	// All segments' ticks count: segment 0 plays to its SAVE at tick 171
	aLong.mEnd.mEndTick = Replay::MAX_TICKS - 171;
	aLong.mSegments[1].mEvents.back().mTick = Replay::MAX_TICKS - 171;
	aWhy.clear();
	Check(Replay::CheckPlayable(aLong, 8, 8, &aWhy), "limits: 171 ticks, then an END at MAX_TICKS - 171 (" + aWhy + ")");
	aLong.mEnd.mEndTick++;
	aLong.mSegments[1].mEvents.back().mTick++;
	aWhy.clear();
	Check(!Replay::CheckPlayable(aLong, 8, 8, &aWhy), "limits: refused: one tick more in all (" + aWhy + ")");
	Replay::File aBusy = aGood;
	std::vector<Replay::Event>& aBusyEvents = aBusy.mSegments[0].mEvents;
	aBusyEvents.insert(aBusyEvents.begin() + 5, Replay::MAX_EVENTS_PER_TICK - 2, MakeEvent(170, Replay::OP_HOVER, 1, 2));
	aWhy.clear();
	Check(Replay::CheckPlayable(aBusy, 8, 8, &aWhy), "limits: MAX_EVENTS_PER_TICK events at one tick (" + aWhy + ")");
	aBusyEvents.insert(aBusyEvents.begin() + 5, MakeEvent(170, Replay::OP_HOVER, 1, 2));
	aWhy.clear();
	Check(!Replay::CheckPlayable(aBusy, 8, 8, &aWhy), "limits: refused: one event more at that tick (" + aWhy + ")");
	Replay::File aDraws = aGood;
	aDraws.mSegments[1].mEvents[4].mArgs[0] = Replay::MAX_XRAND;
	aWhy.clear();
	Check(Replay::CheckPlayable(aDraws, 8, 8, &aWhy), "limits: MAX_XRAND outside draws in all (" + aWhy + ")");
	aDraws.mSegments[1].mEvents.insert(aDraws.mSegments[1].mEvents.begin() + 4, MakeEvent(9, Replay::OP_XRAND, 1));
	aWhy.clear();
	Check(!Replay::CheckPlayable(aDraws, 8, 8, &aWhy), "limits: refused: one outside draw more in the replay (" + aWhy + ")");
}

// A two-segment replay the player of this build can start: a seeded game saved, then resumed
static Replay::File PlayableReplay()
{
	Replay::File aFile;
	aFile.mHasRules = true;
	aFile.mRules = std::string("\x01\x00rules", 7);
	Replay::Segment aFirst;
	aFirst.mRng.mKind = Replay::RNG_SEED;
	aFirst.mRng.mSeed = 4242;
	aFirst.mUserSeeded = true;
	aFirst.mEvents.push_back(MakeEvent(140, Replay::OP_CHECK, 7));
	aFirst.mEvents.push_back(MakeEvent(160, Replay::OP_SELECT, 0, 7));
	aFirst.mEvents.push_back(MakeEvent(161, Replay::OP_TARGET, 7, 7));
	aFirst.mEvents.push_back(MakeEvent(161, Replay::OP_TARGET, -1, -1));
	aFirst.mEvents.push_back(MakeEvent(170, Replay::OP_PAUSE, 1));
	aFirst.mEvents.push_back(MakeEvent(170, Replay::OP_HOVER, 639, -1));
	aFirst.mEvents.push_back(MakeEvent(171, Replay::OP_SAVE, Replay::SAVE_GAME, 4294967295ll));
	aFile.mSegments.push_back(aFirst);
	Replay::Segment aSecond;
	aSecond.mRng.mKind = Replay::RNG_STATE;
	MTRand(5489).GetState(aSecond.mRng.mMt, &aSecond.mRng.mMti);
	aSecond.mHasLoadRng = true;
	aSecond.mLoadRng.mSeed = 4242;
	aSecond.mEvents.push_back(MakeEvent(0, Replay::OP_PAUSE, 0));
	aSecond.mEvents.push_back(MakeEvent(0, Replay::OP_UNPAUSE, 0));
	aSecond.mEvents.push_back(MakeEvent(0, Replay::OP_LOAD));
	aSecond.mEvents.push_back(MakeEvent(1, Replay::OP_CHECK, 9));
	aSecond.mEvents.push_back(MakeEvent(9, Replay::OP_XRAND, 1));
	aSecond.mEvents.push_back(MakeEvent(9, Replay::OP_SAVE, Replay::SAVE_SNAPSHOT, 0));
	aFile.mSegments.push_back(aSecond);
	aFile.mEnd.mReason = Replay::END_SAVED;
	aFile.mEnd.mEndTick = 9;
	return aFile;
}

// Replay::CheckPlayable: the replay above passes, written and read back too; each change below makes it
// one the player refuses before it makes a board
static void TestReplayPlayable()
{
	std::string aWhy;
	Replay::File aGood = PlayableReplay();
	Check(Replay::CheckPlayable(aGood, 8, 8, &aWhy), "playable: a seeded game saved and resumed (" + aWhy + ")");
	Replay::File aRead;
	Check(Replay::Read(Replay::Write(aGood), &aRead, &aWhy) && Replay::CheckPlayable(aRead, 8, 8, &aWhy),
		"playable: also after a write and read (" + aWhy + ")");
	Check(!Replay::CheckPlayable(Replay::File(), 8, 8, &aWhy), "playable: an empty file is refused (" + aWhy + ")");
	Check(!Replay::CheckPlayable(FullReplay(), 8, 8, &aWhy), "playable: TWIST and SLIDE are refused (" + aWhy + ")");

	struct Case
	{
		const char*		mWhat;
		void			(*mBreak)(Replay::File* f);
	};
	static const Case aCases[] =
	{
		{"another engine ABI", [](Replay::File* f) { f->mEngineAbi = Replay::ENGINE_ABI + 1; }},
		{"no RULE", [](Replay::File* f) { f->mHasRules = false; }},
		{"no segment", [](Replay::File* f) { f->mSegments.clear(); }},
		{"mode 2", [](Replay::File* f) { f->mSegments[0].mMode = 2; }},
		{"a seed of 0", [](Replay::File* f) { f->mSegments[0].mRng.mSeed = 0; }},
		{"user-seeded on a state", [](Replay::File* f) { f->mSegments[1].mUserSeeded = true; }},
		{"SELECT off the board", [](Replay::File* f) { f->mSegments[0].mEvents[1].mArgs[0] = 8; }},
		{"SELECT -1 -1", [](Replay::File* f) { f->mSegments[0].mEvents[1].mArgs[0] = -1; f->mSegments[0].mEvents[1].mArgs[1] = -1; }},
		{"TARGET -1 0", [](Replay::File* f) { f->mSegments[0].mEvents[3].mArgs[1] = 0; }},
		{"PAUSE 2", [](Replay::File* f) { f->mSegments[0].mEvents[4].mArgs[0] = 2; }},
		{"HOVER past the int range", [](Replay::File* f) { f->mSegments[0].mEvents[5].mArgs[0] = 0x80000000ll; }},
		{"SAVE kind 3", [](Replay::File* f) { f->mSegments[0].mEvents[6].mArgs[0] = 3; }},
		{"SAVE hash 2^32", [](Replay::File* f) { f->mSegments[0].mEvents[6].mArgs[1] = 0x100000000ll; }},
		{"XRAND 0", [](Replay::File* f) { f->mSegments[1].mEvents[4].mArgs[0] = 0; }},
		{"XRAND past MAX_XRAND", [](Replay::File* f) { f->mSegments[1].mEvents[4].mArgs[0] = Replay::MAX_XRAND + 1; }},
		{"a TWIST off the board", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_TWIST, 7, 3, 1); }},
		{"a TWIST of direction 2", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_TWIST, 3, 3, 2); }},
		{"a SLIDE of k 0", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_SLIDE, 0, 3, 0, 0, 0); }},
		{"a SLIDE of axis 2", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_SLIDE, 2, 3, 0, 1, 51); }},
		{"a SLIDE past the line", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_SLIDE, 1, 3, 0, 8, 51); }},
		{"a SLIDE offset past the line", [](Replay::File* f) { f->mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_SLIDE, 0, 3, 0, 1, 409); }},
		{"a CHECK at tick 0", [](Replay::File* f) { f->mSegments[1].mEvents[0] = MakeEvent(0, Replay::OP_CHECK, 1); }},
		{"a LOAD after tick 0", [](Replay::File* f) { f->mSegments[1].mEvents[2].mTick = 1; }},
		{"two LOADs", [](Replay::File* f) { f->mSegments[1].mEvents[1] = MakeEvent(0, Replay::OP_LOAD); }},
		{"a LOAD without RNGS", [](Replay::File* f) { f->mSegments[1].mHasLoadRng = false; }},
		{"RNGS without a LOAD", [](Replay::File* f) { f->mSegments[0].mHasLoadRng = true; }},
		{"LDSV without a LOAD", [](Replay::File* f) { f->mSegments[0].mHasLoadSave = true; }},
		{"a later segment without a LOAD", [](Replay::File* f) { f->mSegments[1].mEvents.erase(f->mSegments[1].mEvents.begin() + 2); }},
		{"a segment before another not ending in a SAVE", [](Replay::File* f) { f->mSegments[0].mEvents.pop_back(); }},
		{"a first segment that LOADs without LDSV", [](Replay::File* f) { f->mSegments.erase(f->mSegments.begin()); }},
		{"an event after the END tick", [](Replay::File* f) { f->mEnd.mEndTick = 8; }},
		{"a SAVE before the segment's last tick", [](Replay::File* f) {
			f->mSegments[0].mEvents.insert(f->mSegments[0].mEvents.begin() + 4, MakeEvent(170, Replay::OP_SAVE, Replay::SAVE_GAME, 1)); }},
		{"five SAVEs in a segment", [](Replay::File* f) {
			for (int i = 0; i < 4; i++)
				f->mSegments[0].mEvents.push_back(MakeEvent(171, Replay::OP_SAVE, Replay::SAVE_SNAPSHOT, i)); }},
	};
	for (size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); i++)
	{
		Replay::File aBad = PlayableReplay();
		aCases[i].mBreak(&aBad);
		aWhy.clear();
		Check(!Replay::CheckPlayable(aBad, 8, 8, &aWhy) && !aWhy.empty(), std::string("playable: refused: ") + aCases[i].mWhat + " (" + aWhy + ")");
	}

	// Stage 6 plays UNDO (WatchController checks CanUndo as it applies one)
	Replay::File anUndo = PlayableReplay();
	anUndo.mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_UNDO);
	aWhy.clear();
	Check(Replay::CheckPlayable(anUndo, 8, 8, &aWhy), "playable: an UNDO (" + aWhy + ")");
	// Stage 10 plays TWIST and SLIDE, with variant rules (the rules' move types and the board's cells are
	// WatchController's to check: Board::CanTwist, CanSlide)
	Replay::File aMoves = PlayableReplay();
	aMoves.mSegments[0].mEvents[2] = MakeEvent(161, Replay::OP_TWIST, 6, 6, -1);
	aMoves.mSegments[0].mEvents.insert(aMoves.mSegments[0].mEvents.begin() + 3, MakeEvent(161, Replay::OP_SLIDE, 1, 7, 0, 7, -408));
	aWhy.clear();
	Check(Replay::CheckPlayable(aMoves, 8, 8, &aWhy), "playable: a TWIST and a SLIDE (" + aWhy + ")");
	Check(!Replay::CheckPlayable(aMoves, 8, 8, &aWhy, true) && !aWhy.empty(), "playable: refused: a TWIST under the original rules (" + aWhy + ")");

	// What makes it playable again: the first segment's save comes with it (LDSV), then it may LOAD
	Replay::File aFresh = PlayableReplay();
	aFresh.mSegments.erase(aFresh.mSegments.begin());
	aFresh.mSegments[0].mHasLoadSave = true;
	aFresh.mSegments[0].mLoadSave = V1Save();
	aWhy.clear();
	Check(Replay::CheckPlayable(aFresh, 8, 8, &aWhy), "playable: a fresh replay that brings the save it LOADs (LDSV) (" + aWhy + ")");
	aFresh.mSegments[0].mLoadSave = "save";
	aWhy.clear();
	Check(!Replay::CheckPlayable(aFresh, 8, 8, &aWhy) && !aWhy.empty(), "playable: refused: an LDSV that is no save (" + aWhy + ")");
	Check(!Replay::CheckPlayable(aGood, 7, 8, &aWhy), "playable: a 7-column board has no column 7 (" + aWhy + ")");

	// The SAVEs the game makes as a board goes away, at its last tick: Shutdown's, ~Board's, a snapshot's, and one
	// more is still taken (MAX_SAVES_PER_SEGMENT)
	Replay::File aSaves = PlayableReplay();
	for (int i = 0; i < Replay::MAX_SAVES_PER_SEGMENT - 1; i++)
		aSaves.mSegments[0].mEvents.push_back(MakeEvent(171, Replay::OP_SAVE, Replay::SAVE_GAME, i));
	aWhy.clear();
	Check(Replay::CheckPlayable(aSaves, 8, 8, &aWhy), "playable: MAX_SAVES_PER_SEGMENT SAVEs at the segment's last tick (" + aWhy + ")");

	// Stage 5: engine ABI 2; a replay of the original rules from engine 1 still plays (their simulation never
	// changes), one of variant rules only on its own engine
	Replay::File anOld = PlayableReplay();
	anOld.mEngineAbi = 1;
	aWhy.clear();
	Check(Replay::CheckPlayable(anOld, 8, 8, &aWhy, true), "playable: the original rules recorded by engine 1 (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckPlayable(anOld, 8, 8, &aWhy, false) && !aWhy.empty(), "playable: refused: variant rules recorded by engine 1 (" + aWhy + ")");
	anOld.mEngineAbi = Replay::ENGINE_ABI + 1;
	Check(!Replay::CheckPlayable(anOld, 8, 8, &aWhy, true), "playable: refused: a later engine, even for the original rules");
	anOld.mEngineAbi = 0;
	Check(!Replay::CheckPlayable(anOld, 8, 8, &aWhy, true), "playable: refused: engine 0");
	// Stage 7: engine 3 (the board shape rules); stage 7b: engine 4 (dirt); stage 8: engine 5 (more gem colours); stage 9:
	// engine 6 (the match rules); stage 10: engine 7 (twists and slides); stage 11: engine 8 (flame and lightning gems).
	// An earlier engine's replays of variant rules no longer play, its replays of the original rules do
	for (uint32_t anAbi = 2; anAbi < (uint32_t) Replay::ENGINE_ABI; anAbi++)
	{
		anOld.mEngineAbi = anAbi;
		aWhy.clear();
		Check(Replay::CheckPlayable(anOld, 8, 8, &aWhy, true), "playable: the original rules recorded by engine " + std::to_string(anAbi) + " (" + aWhy + ")");
		Check(!Replay::CheckPlayable(anOld, 8, 8, &aWhy, false), "playable: refused: variant rules recorded by engine " + std::to_string(anAbi));
	}
	Check(Replay::ENGINE_ABI == 10, "playable: this is engine 10 (stage 18)");
}

// A v2 save (Replay.h, games/bejeweled/docs/CODE_MODS_DESIGN.md 7.2) around the v1 test save's fields: the rules
// of the design's example 1, an 8x8 board, MTRand(4242) after 1700 draws, 2 moves of which 1 kept invalid move
static Replay::VariantSave TestVariantSave()
{
	GameRules aRules;
	aRules.mMoves.mAllowInvalid = true;
	aRules.mNoMoves.mNormal = GameRules::NOMOVES_CONTINUE;
	Replay::VariantSave aSave;
	aRules.RulesId(aSave.mRulesId);
	aSave.mCols = 8;
	aSave.mRows = 8;
	aSave.mPlayable.assign(64, true);
	MTRand aRng(4242);
	for (int i = 0; i < 1700; i++)
		aRng.Next();
	int anIndex = 0;
	aRng.GetState(aSave.mRng.mMt, &anIndex);
	aSave.mRng.mMti = anIndex;
	aSave.mRng.mSeed = 4242;
	aSave.mRng.mUserSeeded = true;
	aSave.mMoves = 2;
	aSave.mInvalidMoves = 1;
	return aSave;
}

static std::string V2Save(const Replay::VariantSave& theSave, const std::string& theV1 = V1Save())
{
	std::string aVersion("\x01\x01\0\0", 4);
	return aVersion + Replay::VariantSaveHeader(theSave) + theV1.substr(4) + Replay::VariantSaveChunks(theSave);
}

// Stage 5: the v2 save's format (Replay::VariantSaveHeader, VariantSaveChunks, ReadVariantSave), what a replay's
// LDSV may hold of it (Replay::CheckSave with the replay's RulesId), and the mode of either save (SaveIsTimed)
static void TestReplayVariantSave()
{
	std::string aWhy;
	Replay::VariantSave aSave = TestVariantSave();
	std::string aBytes = V2Save(aSave);
	const size_t aHeader = 4 + 32 + 2 + 2 + 4 + 8;
	const size_t aChunks = (8 + 2506) + (8 + 12) + 8;
	Check(aBytes.size() == aHeader + (7873 - 4) + aChunks, "v2: the full 8x8 save is " + std::to_string(aHeader + 7869 + aChunks) + " bytes (" +
		std::to_string(aBytes.size()) + ")");
	Check(Hex(Replay::VariantSaveHeader(aSave).substr(32)) == "08000800" "00000000" "ffffffffffffffff",
		"v2: golden header bytes after the RulesId (8x8, level shape 0, a full mask)");
	Check(Hex(Replay::VariantSaveChunks(aSave).substr(2514)) == "4d4f56450c000000" "02000000" "01000000" "00000000" "454e442000000000",
		"v2: golden MOVE and END chunks");

	Replay::VariantSave aRead;
	Check(Replay::ReadVariantSave(aBytes, &aRead, &aWhy), "v2: reads back (" + aWhy + ")");
	Check((memcmp(aRead.mRulesId, aSave.mRulesId, 32) == 0) && (aRead.mCols == 8) && (aRead.mRows == 8) && (aRead.mLevelShape == 0) &&
		(aRead.mPlayable == aSave.mPlayable) && SameRng(aRead.mRng, aSave.mRng) && (aRead.mMoves == 2) && (aRead.mInvalidMoves == 1) &&
		(aRead.mUndosUsed == 0), "v2: every header and chunk value read back");
	Check((aRead.mBodyStart == aHeader) && (aRead.mBodyEnd == aBytes.size() - aChunks), "v2: where the v1 fields start and end");

	// A hole: absent in the cells, a 0 bit in the mask; a shape another stage writes
	Replay::VariantSave aHoled = aSave;
	aHoled.mPlayable[9] = false;
	std::string aV1Hole = V1Save();
	aV1Hole = aV1Hole.substr(0, SAVE_CELLS_AT + 9 * SAVE_CELL_SIZE) + std::string(1, '\0') + aV1Hole.substr(SAVE_CELLS_AT + 10 * SAVE_CELL_SIZE);
	Check(Replay::ReadVariantSave(V2Save(aHoled, aV1Hole), &aRead, &aWhy) && !aRead.mPlayable[9] && aRead.mPlayable[8],
		"v2: a hole, absent in the cells, reads (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::ReadVariantSave(V2Save(aHoled), &aRead, &aWhy) && !aWhy.empty(), "v2: refused: a gem in a hole (" + aWhy + ")");

	struct Bad { const char* mWhat; std::string mBytes; };
	// GRAV (stage 7c): written when the rules set a gravity rule, absent otherwise (read as 0)
	Check((aRead.mGravityFlips == 0) && !aRead.mGravityNoFlip && !aRead.mHasGravity, "v2: no GRAV chunk reads as 0");
	Replay::VariantSave aGravity = aSave;
	aGravity.mHasGravity = true;
	aGravity.mGravityFlips = 3;
	aGravity.mGravityNoFlip = true;
	std::string aGravityBytes = V2Save(aGravity);
	Check(Hex(Replay::VariantSaveChunks(aGravity).substr(2514 + 20)) == "47524156" "05000000" "03000000" "01" "454e442000000000",
		"v2: golden GRAV chunk after MOVE");
	Check(Replay::ReadVariantSave(aGravityBytes, &aRead, &aWhy) && aRead.mHasGravity && (aRead.mGravityFlips == 3) && aRead.mGravityNoFlip &&
		(aRead.mMoves == 2), "v2: GRAV reads back (" + aWhy + ")");
	std::string aGrav = aGravityBytes.substr(aGravityBytes.size() - 8 - 13, 13);
	std::string aGravNoEnd = aGravityBytes.substr(0, aGravityBytes.size() - 8);
	std::string anEnd = aBytes.substr(aBytes.size() - 8);
	const Bad aGravityBads[] =
	{
		{"GRAV twice", aGravNoEnd + aGrav + anEnd},
		{"GRAV of 4 bytes", aBytes.substr(0, aBytes.size() - 8) + "GRAV" + std::string("\x04\0\0\0", 4) + aGrav.substr(8, 4) + anEnd},
		{"GRAV noFlip 2", aGravNoEnd.substr(0, aGravNoEnd.size() - 1) + "\x02" + anEnd},
		{"GRAV flips below 0", aGravNoEnd.substr(0, aGravNoEnd.size() - 5) + std::string("\xff\xff\xff\xff\0", 5) + anEnd},
	};
	for (size_t i = 0; i < sizeof(aGravityBads) / sizeof(aGravityBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(aGravityBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + aGravityBads[i].mWhat + " (" + aWhy + ")");
	}

	// DIRT (stage 7b): the toggle, then a cell's layers left, written when the rules have dirt
	Check(!aRead.mHasDirt && aRead.mDirtLayers.empty() && aRead.mDiagonalLeftFirst, "v2: no DIRT chunk");
	Replay::VariantSave aDirt = aHoled;
	aDirt.mHasDirt = true;
	aDirt.mDiagonalLeftFirst = false;
	aDirt.mDirtLayers.assign(64, 0);
	aDirt.mDirtLayers[9] = 3;
	std::string aDirtBytes = V2Save(aDirt, aV1Hole);
	std::string aDirtChunk = Replay::VariantSaveChunks(aDirt).substr(2514 + 20);
	Check(Hex(aDirtChunk.substr(0, 8 + 1 + 10)) == "44495254" "41000000" "00" "00000000000000000003", "v2: golden DIRT chunk after MOVE");
	Check(Replay::ReadVariantSave(aDirtBytes, &aRead, &aWhy) && aRead.mHasDirt && !aRead.mDiagonalLeftFirst && (aRead.mDirtLayers == aDirt.mDirtLayers),
		"v2: DIRT reads back (" + aWhy + ")");
	std::string aDirtNoEnd = aDirtBytes.substr(0, aDirtBytes.size() - 8);
	std::string aDirtAlone = aDirtNoEnd.substr(aDirtNoEnd.size() - (8 + 65));
	size_t aDirtCells = aDirtNoEnd.size() - 64;
	auto aWithLayer = [&](size_t theCell, char theLayers) { std::string aCopy = aDirtNoEnd; aCopy[aDirtCells + theCell] = theLayers; return aCopy + anEnd; };
	const Bad aDirtBads[] =
	{
		{"DIRT twice", aDirtNoEnd + aDirtAlone + anEnd},
		{"DIRT of 64 bytes", aDirtNoEnd.substr(0, aDirtNoEnd.size() - (8 + 65)) + "DIRT" + std::string("\x40\0\0\0", 4) + std::string(64, '\0') + anEnd},
		{"DIRT leftFirst 2", aDirtNoEnd.substr(0, aDirtCells - 1) + "\x02" + aDirtNoEnd.substr(aDirtCells) + anEnd},
		{"DIRT 10 layers", aWithLayer(9, 10)},
		{"DIRT layers on a gem cell", aWithLayer(8, 1)},
	};
	for (size_t i = 0; i < sizeof(aDirtBads) / sizeof(aDirtBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(aDirtBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + aDirtBads[i].mWhat + " (" + aWhy + ")");
	}

	// KIND (stage 11): a byte a cell, a gem's kind, written when the rules have specials
	Check(!aRead.mHasKinds && aRead.mKinds.empty(), "v2: no KIND chunk");
	Replay::VariantSave aKinds = aHoled;
	aKinds.mHasKinds = true;
	aKinds.mKinds.assign(64, 0);
	aKinds.mKinds[8] = 1;
	aKinds.mKinds[63] = 2;
	std::string aKindBytes = V2Save(aKinds, aV1Hole);
	std::string aKindChunk = Replay::VariantSaveChunks(aKinds).substr(2514 + 20);
	Check(Hex(aKindChunk.substr(0, 8 + 10)) == "4b494e44" "40000000" "00000000000000000100", "v2: golden KIND chunk after MOVE");
	Check(Replay::ReadVariantSave(aKindBytes, &aRead, &aWhy) && aRead.mHasKinds && (aRead.mKinds == aKinds.mKinds),
		"v2: KIND reads back (" + aWhy + ")");
	std::string aKindNoEnd = aKindBytes.substr(0, aKindBytes.size() - 8);
	std::string aKindAlone = aKindNoEnd.substr(aKindNoEnd.size() - (8 + 64));
	size_t aKindCells = aKindNoEnd.size() - 64;
	auto aWithKind = [&](size_t theCell, char theKind) { std::string aCopy = aKindNoEnd; aCopy[aKindCells + theCell] = theKind; return aCopy + anEnd; };
	const Bad aKindBads[] =
	{
		{"KIND twice", aKindNoEnd + aKindAlone + anEnd},
		{"KIND of 63 bytes", aKindNoEnd.substr(0, aKindNoEnd.size() - (8 + 64)) + "KIND" + std::string("\x3f\0\0\0", 4) + std::string(63, '\0') + anEnd},
		{"KIND 3", aWithKind(10, 3)},
		{"KIND on a hole", aWithKind(9, 1)},
	};
	for (size_t i = 0; i < sizeof(aKindBads) / sizeof(aKindBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(aKindBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + aKindBads[i].mWhat + " (" + aWhy + ")");
	}

	// GOLD and GTIM (stage 12): a byte a cell (1 gold) and the level's time, written when the rules have gold
	Check(Replay::ReadVariantSave(aKindBytes, &aRead, &aWhy) && !aRead.mHasGold && !aRead.mHasGoldTime, "v2: no GOLD or GTIM chunk (" + aWhy + ")");
	Replay::VariantSave aGold = aHoled;
	aGold.mHasGold = true;
	aGold.mGold.assign(64, 0);
	aGold.mGold[8] = 1;
	aGold.mGold[63] = 1;
	aGold.mHasGoldTime = true;
	aGold.mGoldTicksLeft = 4321;
	aGold.mGoldTickLimit = 6000;
	std::string aGoldBytes = V2Save(aGold, aV1Hole);
	std::string aGoldChunk = Replay::VariantSaveChunks(aGold).substr(2514 + 20);
	Check(Hex(aGoldChunk.substr(0, 8 + 10)) == "474f4c44" "40000000" "00000000000000000100", "v2: golden GOLD chunk after MOVE");
	Check(Hex(aGoldChunk.substr(8 + 64, 16)) == "4754494d" "08000000" "e1100000" "70170000", "v2: golden GTIM chunk after GOLD");
	Check(Replay::ReadVariantSave(aGoldBytes, &aRead, &aWhy) && aRead.mHasGold && (aRead.mGold == aGold.mGold) && aRead.mHasGoldTime &&
		(aRead.mGoldTicksLeft == 4321) && (aRead.mGoldTickLimit == 6000), "v2: GOLD and GTIM read back (" + aWhy + ")");
	std::string aGoldNoEnd = aGoldBytes.substr(0, aGoldBytes.size() - 8);
	std::string aGtim = aGoldNoEnd.substr(aGoldNoEnd.size() - 16);
	std::string aGoldAlone = aGoldNoEnd.substr(aGoldNoEnd.size() - 16 - (8 + 64), 8 + 64);
	std::string aBeforeGold = aGoldNoEnd.substr(0, aGoldNoEnd.size() - 16 - (8 + 64));
	size_t aGoldCells = aBeforeGold.size() + 8;
	auto aWithGold = [&](size_t theCell, char theGold) { std::string aCopy = aGoldNoEnd; aCopy[aGoldCells + theCell] = theGold; return aCopy + anEnd; };
	auto aLe32 = [](uint32_t theValue) { std::string anOut; for (int i = 0; i < 4; i++) anOut += (char)((theValue >> (8 * i)) & 0xff); return anOut; };
	auto aWithTime = [&](uint32_t theLeft, uint32_t theLimit) { return aBeforeGold + aGoldAlone + "GTIM" + aLe32(8) + aLe32(theLeft) + aLe32(theLimit) + anEnd; };
	Check(Replay::ReadVariantSave(aWithTime(0, 0), &aRead, &aWhy) && aRead.mHasGoldTime && (aRead.mGoldTickLimit == 0), "v2: GTIM 0 0, no limit (" + aWhy + ")");
	const Bad aGoldBads[] =
	{
		{"GOLD twice", aBeforeGold + aGoldAlone + aGoldAlone + aGtim + anEnd},
		{"GOLD of 63 bytes", aBeforeGold + "GOLD" + std::string("\x3f\0\0\0", 4) + std::string(63, '\0') + aGtim + anEnd},
		{"GOLD 2", aWithGold(10, 2)},
		{"GOLD on a hole", aWithGold(9, 1)},
		{"GTIM twice", aGoldNoEnd + aGtim + anEnd},
		{"GTIM of 4 bytes", aBeforeGold + aGoldAlone + "GTIM" + aLe32(4) + aLe32(0) + anEnd},
		{"GTIM more left than the limit", aWithTime(6001, 6000)},
		{"GTIM a negative limit", aWithTime(0, 0xffffffffu)},
	};
	for (size_t i = 0; i < sizeof(aGoldBads) / sizeof(aGoldBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(aGoldBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + aGoldBads[i].mWhat + " (" + aWhy + ")");
	}

	// ARRW (stage 18): the counts, a step due, then a byte a cell (0 none, else the direction + 1), written when the rules
	// have arrows
	Check(Replay::ReadVariantSave(aGoldBytes, &aRead, &aWhy) && !aRead.mHasArrows, "v2: no ARRW chunk (" + aWhy + ")");
	Replay::VariantSave anArrowSave = aHoled;
	anArrowSave.mHasArrows = true;
	anArrowSave.mArrows.assign(64, 0);
	anArrowSave.mArrows[8] = 1;
	anArrowSave.mArrows[63] = 4;
	anArrowSave.mArrowMoves = 5;
	anArrowSave.mArrowsCleared = 2;
	anArrowSave.mArrowStepDue = true;
	std::string anArrowBytes = V2Save(anArrowSave, aV1Hole);
	std::string anArrowChunk = Replay::VariantSaveChunks(anArrowSave).substr(2514 + 20);
	Check(Hex(anArrowChunk.substr(0, 8 + 9 + 10)) == "41525257" "49000000" "05000000" "02000000" "01" "00000000000000000100",
		"v2: golden ARRW chunk after MOVE");
	Check(Replay::ReadVariantSave(anArrowBytes, &aRead, &aWhy) && aRead.mHasArrows && (aRead.mArrows == anArrowSave.mArrows) &&
		(aRead.mArrowMoves == 5) && (aRead.mArrowsCleared == 2) && aRead.mArrowStepDue, "v2: ARRW reads back (" + aWhy + ")");
	std::string anArrowNoEnd = anArrowBytes.substr(0, anArrowBytes.size() - 8);
	std::string anArrowAlone = anArrowNoEnd.substr(anArrowNoEnd.size() - (8 + 9 + 64));
	std::string aBeforeArrows = anArrowNoEnd.substr(0, anArrowNoEnd.size() - (8 + 9 + 64));
	auto aWithArrowByte = [&](size_t theAt, char theByte) { std::string aCopy = anArrowNoEnd; aCopy[aBeforeArrows.size() + 8 + theAt] = theByte; return aCopy + anEnd; };
	const Bad anArrowBads[] =
	{
		{"ARRW twice", anArrowNoEnd + anArrowAlone + anEnd},
		{"ARRW of 72 bytes", aBeforeArrows + "ARRW" + aLe32(72) + std::string(72, '\0') + anEnd},
		{"ARRW arrow 9", aWithArrowByte(9 + 10, 9)},
		{"ARRW on a hole", aWithArrowByte(9 + 9, 1)},
		{"ARRW negative moves", aWithArrowByte(3, '\x80')},
		{"ARRW step due 2", aWithArrowByte(8, 2)},
	};
	for (size_t i = 0; i < sizeof(anArrowBads) / sizeof(anArrowBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(anArrowBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + anArrowBads[i].mWhat + " (" + aWhy + ")");
	}

	// An unknown chunk before END is skipped
	std::string anExtra = aBytes.substr(0, aBytes.size() - 8) + "XTRA" + std::string("\x02\0\0\0", 4) + "ab" + aBytes.substr(aBytes.size() - 8);
	Check(Replay::ReadVariantSave(anExtra, &aRead, &aWhy) && (aRead.mMoves == 2), "v2: an unknown chunk is skipped (" + aWhy + ")");

	std::string aNoEnd = aBytes.substr(0, aBytes.size() - 8);
	std::string aMove = aBytes.substr(aBytes.size() - 28, 20);
	std::string aRngs = aBytes.substr(aBytes.size() - aChunks, 8 + 2506);
	const size_t aChunkAt = aBytes.size() - aChunks;
	const Bad aBads[] =
	{
		{"version 1", std::string("\x01\0\0\0", 4) + aBytes.substr(4)},
		{"version 0x102", std::string("\x02\x01\0\0", 4) + aBytes.substr(4)},
		{"2 columns", aBytes.substr(0, 36) + std::string("\x02\0", 2) + aBytes.substr(38)},
		{"17 rows", aBytes.substr(0, 38) + std::string("\x11\0", 2) + aBytes.substr(40)},
		{"no END", aNoEnd},
		{"a byte after END", aBytes + "x"},
		{"END with a payload", aNoEnd + "END " + std::string("\x01\0\0\0", 4) + "x"},
		{"MOVE twice", aNoEnd + aMove + aBytes.substr(aBytes.size() - 8)},
		{"RNGS twice", aNoEnd + aRngs + aBytes.substr(aBytes.size() - 8)},
		{"no RNGS", aBytes.substr(0, aChunkAt) + aBytes.substr(aChunkAt + 8 + 2506)},
		{"no MOVE", aBytes.substr(0, aBytes.size() - 28) + aBytes.substr(aBytes.size() - 8)},
		{"MOVE of 8 bytes", aBytes.substr(0, aBytes.size() - 28) + "MOVE" + std::string("\x08\0\0\0", 4) + aMove.substr(8, 8) + aBytes.substr(aBytes.size() - 8)},
		{"a chunk longer than the file", aNoEnd + "GOLD" + std::string("\xff\0\0\0", 4)},
	};
	for (size_t i = 0; i < sizeof(aBads) / sizeof(aBads[0]); i++)
	{
		aWhy.clear();
		Check(!Replay::ReadVariantSave(aBads[i].mBytes, &aRead, &aWhy) && !aWhy.empty(), std::string("v2: refused: ") + aBads[i].mWhat + " (" + aWhy + ")");
	}
	int aCuts = 0;
	for (size_t aLen = 0; aLen < aBytes.size(); aLen += 7)
		aCuts += Replay::ReadVariantSave(aBytes.substr(0, aLen), &aRead) ? 1 : 0;
	Check(aCuts == 0, "v2: every cut is refused (" + std::to_string(aCuts) + " read)");

	// CheckSave: a replay's LDSV may be a v2 save of the replay's rules, of the board's shape
	uint8_t anId[32];
	memcpy(anId, aSave.mRulesId, 32);
	aWhy.clear();
	Check(Replay::CheckSave(aBytes, 8, 8, &aWhy, anId), "v2 check: a v2 save of the replay's rules passes (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(aBytes, 8, 8, &aWhy) && !aWhy.empty(), "v2 check: refused: a v2 save where the replay names no rules (" + aWhy + ")");
	uint8_t anOther[32];
	memcpy(anOther, anId, 32);
	anOther[31] ^= 1;
	aWhy.clear();
	Check(!Replay::CheckSave(aBytes, 8, 8, &aWhy, anOther) && !aWhy.empty(), "v2 check: refused: a v2 save of other rules (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(aBytes, 7, 8, &aWhy, anId) && !aWhy.empty(), "v2 check: refused: a save of another board size (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(V2Save(aHoled, aV1Hole), 8, 8, &aWhy, anId) && !aWhy.empty(), "v2 check: refused: holes on the 8x8 board (" + aWhy + ")");
	Replay::VariantSave aNegative = aSave;
	aNegative.mInvalidMoves = -1;
	aWhy.clear();
	Check(!Replay::CheckSave(V2Save(aNegative), 8, 8, &aWhy, anId) && !aWhy.empty(), "v2 check: refused: a MOVE count below 0 (" + aWhy + ")");
	// The v1 fields are checked as a v1 save's, after the header
	std::string aBadColour = aBytes;
	int32_t aSeven = 7;
	memcpy(&aBadColour[aHeader - 4 + SavePieceAt(2, 3, offsetof(Piece, mColor))], &aSeven, 4);
	aWhy.clear();
	Check(!Replay::CheckSave(aBadColour, 8, 8, &aWhy, anId) && !aWhy.empty(), "v2 check: refused: colour 7 in a cell (" + aWhy + ")");
	aWhy.clear();
	// Stage 8: the rules' colours bound it
	Check(Replay::CheckSave(aBadColour, 8, 8, &aWhy, anId, NULL, 8), "v2 check: colour 7 of 8 colours passes (" + aWhy + ")");
	int32_t aTwelve = 12;
	memcpy(&aBadColour[aHeader - 4 + SavePieceAt(2, 3, offsetof(Piece, mColor))], &aTwelve, 4);
	aWhy.clear();
	Check(!Replay::CheckSave(aBadColour, 8, 8, &aWhy, anId, NULL, 12) && !aWhy.empty(), "v2 check: refused: colour 12 of 12 colours (" + aWhy + ")");
	aWhy.clear();
	Check(Replay::CheckSave(V1Save(), 8, 8, &aWhy, anId), "v2 check: a v1 save still passes (LoadGame says which a board takes) (" + aWhy + ")");

	// Stage 7: with the rules' level shapes a v2 save may be of any of them, in its size and mask and of one of its
	// levels; here the holed 8x8 as the second of two shapes (level 1's), and a 6x6 board
	std::vector<BoardLayout> aTwo(2);
	aTwo[1].mCell[1][1] = BoardLayout::CELL_HOLE;
	Replay::VariantSave aSecond = aHoled;
	aSecond.mLevelShape = 1;
	std::string aLevel1 = aV1Hole;
	aLevel1[5] = 1;
	aWhy.clear();
	Check(Replay::CheckSave(V2Save(aSecond, aLevel1), 8, 8, &aWhy, anId, &aTwo), "v2 shapes: level 1's holed shape passes (" + aWhy + ")");
	aWhy.clear();
	Check(Replay::CheckSave(aBytes, 8, 8, &aWhy, anId, &aTwo), "v2 shapes: level 0's full shape passes (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(V2Save(aHoled, aLevel1), 8, 8, &aWhy, anId, &aTwo) && !aWhy.empty(),
		"v2 shapes: refused: the holed mask as shape 0 (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(V2Save(aSecond, aV1Hole), 8, 8, &aWhy, anId, &aTwo) && !aWhy.empty(),
		"v2 shapes: refused: shape 1 on level 0, whose shape is 0 (" + aWhy + ")");
	Replay::VariantSave aThird = aSave;
	aThird.mLevelShape = 2;
	aWhy.clear();
	Check(!Replay::CheckSave(V2Save(aThird), 8, 8, &aWhy, anId, &aTwo) && !aWhy.empty(), "v2 shapes: refused: shape 2 of two (" + aWhy + ")");
	Replay::VariantSave aSmall = aSave;
	aSmall.mCols = 6;
	aSmall.mRows = 6;
	aSmall.mPlayable.assign(36, true);
	std::string aSmallBytes = V2Save(aSmall, V1Save(6, 6));
	std::vector<BoardLayout> aSix(1);
	aSix[0].SetFull(6, 6);
	aWhy.clear();
	Check(Replay::CheckSave(aSmallBytes, 6, 6, &aWhy, anId, &aSix), "v2 shapes: a 6x6 save of a 6x6 board (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(aSmallBytes, 8, 8, &aWhy, anId, &aTwo) && !aWhy.empty(), "v2 shapes: refused: a 6x6 save of 8x8 shapes (" + aWhy + ")");
	aWhy.clear();
	Check(!Replay::CheckSave(aSmallBytes, 8, 8, &aWhy, anId) && !aWhy.empty(), "v2 shapes: refused: a 6x6 save without shapes, of an 8x8 board (" + aWhy + ")");

	// The replay player: an LDSV that is a v2 save of the replay's rules
	Replay::File aFresh = PlayableReplay();
	aFresh.mSegments.erase(aFresh.mSegments.begin());
	aFresh.mSegments[0].mHasLoadSave = true;
	aFresh.mSegments[0].mLoadSave = aBytes;
	aFresh.mHasRulesId = true;
	memcpy(aFresh.mRulesId, anId, 32);
	aWhy.clear();
	Check(Replay::CheckPlayable(aFresh, 8, 8, &aWhy), "v2 check: a fresh replay whose LDSV is a v2 save of its rules (" + aWhy + ")");
	aFresh.mRulesId[0] ^= 1;
	aWhy.clear();
	Check(!Replay::CheckPlayable(aFresh, 8, 8, &aWhy) && !aWhy.empty(), "v2 check: refused: an LDSV of other rules than the RLID (" + aWhy + ")");
	aFresh.mRulesId[0] ^= 1;
	aFresh.mSegments[0].mLoadSave = aSmallBytes;
	aWhy.clear();
	Check(Replay::CheckPlayable(aFresh, 6, 6, &aWhy, false, &aSix), "v2 shapes: a fresh replay of a 6x6 board whose LDSV is its save (" + aWhy + ")");

	// The mode of a save, v1 or v2
	bool isTimed = true;
	Check(Replay::SaveIsTimed(V1Save(), &isTimed) && !isTimed, "save mode: a v1 normal game");
	std::string aTimed = V1Save();
	aTimed[4] = 1;
	Check(Replay::SaveIsTimed(V2Save(aSave, aTimed), &isTimed) && isTimed, "save mode: a v2 timed game");
	Check(Replay::SaveIsTimed(aBytes, &isTimed) && !isTimed, "save mode: a v2 normal game");
	Check(!Replay::SaveIsTimed("BJRP", &isTimed) && !Replay::SaveIsTimed(std::string("\x01\x01\0\0", 4), &isTimed), "save mode: no save, no mode");
}

// Stage 9 (4.6, 4.7): MatchFinder's groups, their order and kinds, the subset skip, and its fill and search tests
// (AnyGroup, InGroup) against the groups on random grids; at default rules InGroup is the original FindMove's test
static MatchFinder::ColorGrid Grid(const std::vector<std::string>& theRows)
{
	// A letter is a colour (a = 0), "." no gem
	MatchFinder::ColorGrid aGrid;
	aGrid.mRows = (int) theRows.size();
	aGrid.mCols = (int) theRows[0].size();
	for (int r = 0; r < aGrid.mRows; r++)
	{
		for (int c = 0; c < aGrid.mCols; c++)
			aGrid.mColor[r][c] = (theRows[r][c] == '.') ? (int) MatchFinder::NO_GEM : (theRows[r][c] - 'a');
	}
	return aGrid;
}

static MatchFinder::Spec MatchSpec(const std::string& theMatch)
{
	std::string aJson = "{\"rules\": {\"board\": {\"gemColors\": 12}, \"match\": " + theMatch + "}}";
	Result aResult = Merge({{"m", aJson.c_str()}});
	Check(Clean(aResult), "match finder: the rules " + theMatch + Messages(aResult));
	return MatchFinder::Spec::FromRules(aResult.mRules);
}

static std::string GroupsText(const std::vector<MatchFinder::Group>& theGroups)
{
	// "kind:cells" per group, kind V H \ / L T + S, cells as col,row
	static const char* const kKinds = "VH\\/LT+S";
	std::string aText;
	for (const MatchFinder::Group& g : theGroups)
	{
		if (!aText.empty())
			aText += " ";
		aText += kKinds[g.mKind];
		aText += ":";
		for (size_t i = 0; i < g.mCells.size(); i++)
			aText += (i ? ";" : "") + std::to_string(g.mCells[i].first) + "," + std::to_string(g.mCells[i].second);
	}
	return aText;
}

static void CheckGroups(const std::vector<std::string>& theRows, const MatchFinder::Spec& theSpec, const std::string& theExpected, const std::string& theWhat)
{
	std::vector<MatchFinder::Group> aGroups;
	MatchFinder::ColorGrid aGrid = Grid(theRows);
	MatchFinder::FindGroups(aGrid, theSpec, &aGroups);
	std::string aText = GroupsText(aGroups);
	Check(aText == theExpected, "match finder: " + theWhat + " (got " + aText + ")");
	Check(MatchFinder::AnyGroup(aGrid, theSpec) == !aGroups.empty(), "match finder: " + theWhat + ": AnyGroup");
}

static void TestMatchFinder()
{
	MatchFinder::Spec aDefault = MatchFinder::Spec::FromRules(GameRules());
	Check(aDefault.IsDefault(), "match finder: the default rules are the original's");
	// The original order: vertical runs first, then horizontal; a crossing scores in both
	CheckGroups({"bab.", "baaa", "bab."}, aDefault, "V:0,0;0,1;0,2 V:1,0;1,1;1,2 H:1,1;2,1;3,1", "default: columns, then rows");
	CheckGroups({"aab", "bba", "aab"}, aDefault, "", "default: no run of 3");
	CheckGroups({"a.aa", "aaa."}, aDefault, "H:0,1;1,1;2,1", "default: a hole breaks a run");

	MatchFinder::Spec aMerge = MatchSpec("{\"crossings\": \"merge\"}");
	CheckGroups({"a..", "a..", "aaa"}, aMerge, "L:0,0;0,1;0,2;1,2;2,2", "merge: an L");
	CheckGroups({"aaa", ".a.", ".a."}, aMerge, "T:1,0;1,1;1,2;0,0;2,0", "merge: a T at the vertical run's place");
	CheckGroups({".a.", "aaa", ".a."}, aMerge, "+:1,0;1,1;1,2;0,1;2,1", "merge: a plus");
	CheckGroups({"a.b", "a.b", "aab"}, aMerge, "V:0,0;0,1;0,2 V:2,0;2,1;2,2", "merge: runs that do not cross stay runs");
	Check(MatchFinder::BonusKey(MatchFinder::Group(), aMerge) == NULL, "match finder: a run has no bonus key");

	MatchFinder::Spec aTwo = MatchSpec("{\"minRun\": 2}");
	CheckGroups({"ab", "cb"}, aTwo, "V:1,0;1,1", "minRun 2: a run of two");
	MatchFinder::Spec aFour = MatchSpec("{\"minRun\": 4}");
	CheckGroups({"aaab", "bbbb"}, aFour, "H:0,1;1,1;2,1;3,1", "minRun 4: a run of 3 is none");

	MatchFinder::Spec aDiagonal = MatchSpec("{\"diagonal\": true}");
	CheckGroups({"abcb", "cabd", "dbad"}, aDiagonal, "\\:0,0;1,1;2,2 /:3,0;2,1;1,2", "diagonal: \\ then /");
	MatchFinder::Spec aLongDiagonal = MatchSpec("{\"diagonal\": true, \"diagonalMinRun\": 4}");
	CheckGroups({"abcb", "cabd", "dbad"}, aLongDiagonal, "", "diagonalMinRun 4: diagonals of 3 are none");

	MatchFinder::Spec aSquare = MatchSpec("{\"shapes\": {\"sq\": {\"preset\": \"square\"}}}");
	CheckGroups({"aab", "aac"}, aSquare, "S:0,0;0,1;1,0;1,1", "square: one instance");
	CheckGroups({"aaa", "aaa"}, aSquare, "H:0,0;1,0;2,0 H:0,1;1,1;2,1 S:0,0;0,1;1,0;1,1 S:1,0;1,1;2,0;2,1", "square: two overlapping instances after the runs");
	// A shape whose instance repeats a run is skipped
	MatchFinder::Spec aLine = MatchSpec("{\"shapes\": {\"line\": {\"cells\": [[0,0],[1,0],[2,0]]}}}");
	CheckGroups({"aaa", "bcb"}, aLine, "H:0,0;1,0;2,0", "subset skip: an instance inside a run");
	MatchFinder::Spec anL3 = MatchSpec("{\"shapes\": {\"l\": {\"preset\": \"L3\"}}}");
	std::vector<MatchFinder::Group> aGroups;
	MatchFinder::FindGroups(Grid({".a", "aa"}), anL3, &aGroups);
	Check((aGroups.size() == 1) && (aGroups[0].mKind == MatchFinder::GROUP_SHAPE) && (std::string(MatchFinder::BonusKey(aGroups[0], anL3)) == "l"),
		"match finder: an L3 whose box corner is a hole (got " + GroupsText(aGroups) + ")");
	Check(MatchFinder::AnyGroup(Grid({".a", "aa"}), anL3), "match finder: AnyGroup finds an L3 anchored on a hole");
	MatchFinder::FindGroups(Grid({"abc", "bab", "cba"}), aDiagonal, &aGroups);
	Check((aGroups.size() == 1) && (std::string(MatchFinder::BonusKey(aGroups[0], aDiagonal)) == "diagonal"), "match finder: a diagonal's bonus key");

	// Random grids, each spec: AnyGroup is "some group", InGroup(both) "some group holds the cell", InGroup(one
	// direction) "a run of that direction holds it"; at default rules InGroup after a swap is the original FindMove's test
	std::vector<MatchFinder::Spec> aSpecs = {aDefault, aMerge, aTwo, aFour, aDiagonal, aSquare, anL3,
		MatchSpec("{\"minRun\": 2, \"diagonal\": true, \"crossings\": \"merge\", \"shapes\": {\"t\": {\"preset\": \"T4\"}, \"sq\": {\"preset\": \"square\"}}}")};
	MTRand aRand(9009);
	int aMismatches = 0;
	int aMoveMismatches = 0;
	int aGroupCount = 0;
	for (int aTrial = 0; aTrial < 400; aTrial++)
	{
		MatchFinder::ColorGrid aGrid;
		aGrid.mCols = 3 + (int) aRand.Next(6UL);
		aGrid.mRows = 3 + (int) aRand.Next(6UL);
		int aColors = 3 + (int) aRand.Next(4UL);
		for (int r = 0; r < aGrid.mRows; r++)
		{
			for (int c = 0; c < aGrid.mCols; c++)
				aGrid.mColor[r][c] = (aRand.Next(10UL) == 0) ? (int) MatchFinder::NO_GEM : (int) aRand.Next((unsigned long) aColors);
		}
		for (const MatchFinder::Spec& aSpec : aSpecs)
		{
			MatchFinder::FindGroups(aGrid, aSpec, &aGroups);
			aGroupCount += (int) aGroups.size();
			if (MatchFinder::AnyGroup(aGrid, aSpec) != !aGroups.empty())
				aMismatches++;
			MatchFinder::Spec aRunsOnly = aSpec;
			aRunsOnly.mMerge = false;
			std::vector<MatchFinder::Group> aRuns;
			MatchFinder::FindGroups(aGrid, aRunsOnly, &aRuns);
			for (int r = 0; r < aGrid.mRows; r++)
			{
				for (int c = 0; c < aGrid.mCols; c++)
				{
					bool isInAny = false;
					for (const MatchFinder::Group& g : aGroups)
						isInAny = isInAny || g.Contains(c, r);
					bool isInH = false;
					bool isInV = false;
					for (const MatchFinder::Group& g : aRuns)
					{
						isInH = isInH || ((g.mKind == MatchFinder::GROUP_HORIZONTAL) && g.Contains(c, r));
						isInV = isInV || ((g.mKind == MatchFinder::GROUP_VERTICAL) && g.Contains(c, r));
					}
					if ((MatchFinder::InGroup(aGrid, aSpec, c, r, true, true) != isInAny) ||
						(MatchFinder::InGroup(aGrid, aSpec, c, r, true, false) != isInH) ||
						(MatchFinder::InGroup(aGrid, aSpec, c, r, false, true) != isInV))
						aMismatches++;
				}
			}
		}
		// The original's test (BoardRules.cpp FindMove) for each swap, from each of its cells, each flag pair
		for (int r = 0; r < aGrid.mRows; r++)
		{
			for (int c = 0; c < aGrid.mCols; c++)
			{
				for (int d = 0; d < 2; d++)
				{
					int c2 = c + (d == 0);
					int r2 = r + (d == 1);
					if ((c2 >= aGrid.mCols) || (r2 >= aGrid.mRows) || (aGrid.mColor[r][c] == MatchFinder::NO_GEM) || (aGrid.mColor[r2][c2] == MatchFinder::NO_GEM))
						continue;
					std::swap(aGrid.mColor[r][c], aGrid.mColor[r2][c2]);
					for (int anEnd = 0; anEnd < 2; anEnd++)
					{
						int x = anEnd ? c2 : c;
						int y = anEnd ? r2 : r;
						int aColor = aGrid.mColor[y][x];
						int aLeft = x, aRight = x, anUp = y, aDown = y;
						while (aGrid.At(aLeft - 1, y) == aColor) aLeft--;
						while (aGrid.At(aRight + 1, y) == aColor) aRight++;
						while (aGrid.At(x, anUp - 1) == aColor) anUp--;
						while (aGrid.At(x, aDown + 1) == aColor) aDown++;
						for (int f = 1; f < 4; f++)
						{
							bool h = (f & 1) != 0;
							bool v = (f & 2) != 0;
							bool isOriginal = ((aRight - aLeft >= 2) && h) || ((aDown - anUp >= 2) && v);
							if (MatchFinder::InGroup(aGrid, aDefault, x, y, h, v) != isOriginal)
								aMoveMismatches++;
						}
					}
					std::swap(aGrid.mColor[r][c], aGrid.mColor[r2][c2]);
				}
			}
		}
	}
	Check(aGroupCount > 1000, "match finder: the random grids have groups (" + std::to_string(aGroupCount) + ")");
	Check(aMismatches == 0, "match finder: AnyGroup and InGroup agree with FindGroups on random grids (" + std::to_string(aMismatches) + " differ)");
	Check(aMoveMismatches == 0, "match finder: InGroup at default rules is the original FindMove's test (" + std::to_string(aMoveMismatches) + " differ)");
}

// Stage 10 (4.4, 4.5, 4.7): MoveSet's moves, and its search against a brute force that makes every candidate on a
// copy of the grid and asks FindGroups, in the order MoveSet.h gives
static std::string FoundText(const MoveSet::Found& f)
{
	const MoveSet::Move& m = f.mMove;
	return std::to_string(m.mType) + ":" + std::to_string(m.mA) + "," + std::to_string(m.mB) + "," + std::to_string(m.mC) + "," +
		std::to_string(m.mD) + "," + std::to_string(m.mE) + " to " + std::to_string(f.mTo.first) + "," + std::to_string(f.mTo.second) +
		" from " + std::to_string(f.mFrom.first) + "," + std::to_string(f.mFrom.second);
}

static std::vector<MoveSet::Found> BruteMoves(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout,
	const MatchFinder::Spec& theSpec, int theTypes, bool theTwistBoth)
{
	std::vector<MoveSet::Found> aMoves;
	std::vector<MatchFinder::Group> aGroups;
	// The first of the moved gems (from theFrom[i] to theTo[i]) in a group of theMoved
	auto aLanding = [&](const MatchFinder::ColorGrid& theMoved, const std::vector<MatchFinder::Cell>& theFrom,
		const std::vector<MatchFinder::Cell>& theTo, MoveSet::Found* theFound) -> bool
	{
		MatchFinder::FindGroups(theMoved, theSpec, &aGroups);
		for (size_t i = 0; i < theTo.size(); i++)
		{
			for (const MatchFinder::Group& g : aGroups)
			{
				if (g.Contains(theTo[i].first, theTo[i].second))
				{
					theFound->mTo = theTo[i];
					theFound->mFrom = theFrom[i];
					return true;
				}
			}
		}
		return false;
	};
	auto aGem = [&](int c, int r) { return theLayout.IsPlayable(c, r) && (theGrid.At(c, r) != MatchFinder::NO_GEM); };
	if (theTypes & GameRules::MOVE_SWAP)
	{
		const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (int r = 0; r < theGrid.mRows; r++)
			for (int c = 0; c < theGrid.mCols; c++)
				for (int d = 0; d < 4; d++)
				{
					int c2 = c + aDirs[d][0];
					int r2 = r + aDirs[d][1];
					if ((theGrid.At(c, r) == MatchFinder::NO_GEM) || (theGrid.At(c2, r2) == MatchFinder::NO_GEM))
						continue;
					MatchFinder::ColorGrid aMoved = theGrid;
					std::swap(aMoved.mColor[r][c], aMoved.mColor[r2][c2]);
					MoveSet::Found f;
					f.mMove.mType = GameRules::MOVE_SWAP;
					f.mMove.mA = c;
					f.mMove.mB = r;
					f.mMove.mC = c2;
					f.mMove.mD = r2;
					// a swap counts at its first cell only, the gem it brings there (the original's test)
					if (aLanding(aMoved, {MatchFinder::Cell(c2, r2)}, {MatchFinder::Cell(c, r)}, &f))
						aMoves.push_back(f);
				}
	}
	if (theTypes & GameRules::MOVE_TWIST)
	{
		for (int r = 0; r + 1 < theGrid.mRows; r++)
			for (int c = 0; c + 1 < theGrid.mCols; c++)
			{
				if (!aGem(c, r) || !aGem(c + 1, r) || !aGem(c, r + 1) || !aGem(c + 1, r + 1))
					continue;
				std::vector<MatchFinder::Cell> aCorners = {{c, r}, {c + 1, r}, {c + 1, r + 1}, {c, r + 1}};
				for (int aDir = 1; aDir >= (theTwistBoth ? -1 : 1); aDir -= 2)
				{
					MatchFinder::ColorGrid aMoved = theGrid;
					std::vector<MatchFinder::Cell> aTo;
					for (int i = 0; i < 4; i++)
					{
						MatchFinder::Cell aDest = aCorners[(i + 4 + aDir) % 4];
						aMoved.mColor[aDest.second][aDest.first] = theGrid.mColor[aCorners[i].second][aCorners[i].first];
						aTo.push_back(aDest);
					}
					MoveSet::Found f;
					f.mMove.mType = GameRules::MOVE_TWIST;
					f.mMove.mA = c;
					f.mMove.mB = r;
					f.mMove.mC = aDir;
					if (aLanding(aMoved, aCorners, aTo, &f))
						aMoves.push_back(f);
				}
			}
	}
	if (theTypes & GameRules::MOVE_SLIDE)
	{
		for (int anAxis = 0; anAxis < 2; anAxis++)
		{
			int aLines = anAxis ? theGrid.mCols : theGrid.mRows;
			int aLength = anAxis ? theGrid.mRows : theGrid.mCols;
			for (int aLine = 0; aLine < aLines; aLine++)
			{
				auto aCell = [&](int i) { return anAxis ? MatchFinder::Cell(aLine, i) : MatchFinder::Cell(i, aLine); };
				for (int aStart = 0; aStart < aLength; )
				{
					if (!theLayout.IsPlayable(aCell(aStart).first, aCell(aStart).second))
					{
						aStart++;
						continue;
					}
					int anEnd = aStart;
					bool isFull = true;
					while ((anEnd < aLength) && theLayout.IsPlayable(aCell(anEnd).first, aCell(anEnd).second))
					{
						isFull = isFull && aGem(aCell(anEnd).first, aCell(anEnd).second);
						anEnd++;
					}
					int n = anEnd - aStart;
					for (int k = 1; isFull && (k < n); k++)
					{
						MatchFinder::ColorGrid aMoved = theGrid;
						std::vector<MatchFinder::Cell> aFrom;
						std::vector<MatchFinder::Cell> aTo;
						for (int i = 0; i < n; i++)
						{
							MatchFinder::Cell s = aCell(aStart + i);
							MatchFinder::Cell t = aCell(aStart + (i + k) % n);
							aMoved.mColor[t.second][t.first] = theGrid.mColor[s.second][s.first];
							aFrom.push_back(s);
							aTo.push_back(t);
						}
						MoveSet::Found f;
						f.mMove.mType = GameRules::MOVE_SLIDE;
						f.mMove.mA = anAxis;
						f.mMove.mB = aLine;
						f.mMove.mC = aStart;
						f.mMove.mD = n;
						f.mMove.mE = k;
						if (aLanding(aMoved, aFrom, aTo, &f))
							aMoves.push_back(f);
					}
					aStart = anEnd;
				}
			}
		}
	}
	return aMoves;
}

static void TestMoveSet()
{
	// The moves' cells: a clockwise twist takes the top left gem to the top right, a slide wraps in its segment
	MoveSet::Move aTwist;
	aTwist.mType = GameRules::MOVE_TWIST;
	aTwist.mA = 1;
	aTwist.mB = 2;
	aTwist.mC = 1;
	MatchFinder::ColorGrid aGrid = Grid({"abcd", "efgh", "ijkl", "mnop"});
	MoveSet::Apply(aGrid.mColor, aTwist);
	Check((aGrid.mColor[2][1] == 'n' - 'a') && (aGrid.mColor[2][2] == 'j' - 'a') && (aGrid.mColor[3][2] == 'k' - 'a') &&
		(aGrid.mColor[3][1] == 'o' - 'a'), "move set: a clockwise twist turns the block on screen");
	aTwist.mC = -1;
	MoveSet::Apply(aGrid.mColor, aTwist);
	Check((aGrid.mColor[2][1] == 'j' - 'a') && (aGrid.mColor[3][2] == 'o' - 'a'), "move set: an anticlockwise twist turns it back");
	MoveSet::Move aSlide;
	aSlide.mType = GameRules::MOVE_SLIDE;
	aSlide.mA = MoveSet::AXIS_COL;
	aSlide.mB = 3;
	aSlide.mC = 1;
	aSlide.mD = 3;
	aSlide.mE = 2;
	MoveSet::Apply(aGrid.mColor, aSlide);
	Check((aGrid.mColor[0][3] == 'd' - 'a') && (aGrid.mColor[3][3] == 'h' - 'a') && (aGrid.mColor[1][3] == 'l' - 'a') &&
		(aGrid.mColor[2][3] == 'p' - 'a'), "move set: a slide of 2 down a 3-cell segment wraps inside it");
	int aCos = 0;
	int aSin = 0;
	MoveSet::TwistCosSin(9, &aCos, &aSin);
	Check((aCos == 724) && (aSin == 724), "move set: cos and sin of 45 degrees");
	MoveSet::TwistCosSin(MoveSet::TWIST_STEPS, &aCos, &aSin);
	Check((aCos == 0) && (aSin == 1024), "move set: cos and sin of 90 degrees");
	BoardLayout aHoled;
	aHoled.SetMask(5, 1, {"##.##"});
	int aStart = 0;
	int aLength = 0;
	Check(MoveSet::SegmentAt(aHoled, MoveSet::AXIS_ROW, 4, 0, &aStart, &aLength) && (aStart == 3) && (aLength == 2) &&
		!MoveSet::SegmentAt(aHoled, MoveSet::AXIS_ROW, 2, 0, &aStart, &aLength), "move set: a hole splits a row into segments");

	// Random grids on random masks, every type set and twist direction, a few match specs: the search is the brute
	// force's, the n-th move FindMove's, and with swaps alone stage 9's (the brute force's first part)
	std::vector<MatchFinder::Spec> aSpecs = {MatchFinder::Spec::FromRules(GameRules()), MatchSpec("{\"minRun\": 4}"),
		MatchSpec("{\"diagonal\": true, \"crossings\": \"merge\", \"shapes\": {\"sq\": {\"preset\": \"square\"}}}")};
	MTRand aRand(10010);
	int aMismatches = 0;
	int aFound[8] = {};
	for (int aTrial = 0; aTrial < 300; aTrial++)
	{
		int aCols = 2 + (int) aRand.Next(8UL);
		int aRows = 2 + (int) aRand.Next(8UL);
		std::vector<std::string> aMask(aRows, std::string(aCols, '#'));
		for (int r = 0; r < aRows; r++)
			for (int c = 0; c < aCols; c++)
				aMask[r][c] = (aRand.Next(8UL) == 0) ? '.' : '#';
		BoardLayout aLayout;
		aLayout.SetMask(aCols, aRows, aMask);
		MatchFinder::ColorGrid aRandom;
		aRandom.mCols = aCols;
		aRandom.mRows = aRows;
		int aColors = 3 + (int) aRand.Next(4UL);
		for (int r = 0; r < aRows; r++)
			for (int c = 0; c < aCols; c++)
				aRandom.mColor[r][c] = (aMask[r][c] == '.') ? (int) MatchFinder::NO_GEM : (int) aRand.Next((unsigned long) aColors);
		for (const MatchFinder::Spec& aSpec : aSpecs)
		{
			for (int aTypes = 1; aTypes < 8; aTypes++)
			{
				for (int aBoth = 0; aBoth < 2; aBoth++)
				{
					MoveSet::Options anOptions;
					anOptions.mTypes = aTypes;
					anOptions.mTwistBoth = (aBoth != 0);
					std::vector<MoveSet::Found> aMoves;
					MoveSet::FindMoves(aRandom, aLayout, aSpec, anOptions, &aMoves);
					std::vector<MoveSet::Found> aBrute = BruteMoves(aRandom, aLayout, aSpec, aTypes, aBoth != 0);
					bool isSame = (aMoves.size() == aBrute.size());
					for (size_t i = 0; isSame && (i < aMoves.size()); i++)
						isSame = (FoundText(aMoves[i]) == FoundText(aBrute[i]));
					MoveSet::Found aNth;
					for (size_t i = 0; isSame && (i < aMoves.size()); i += 1 + i / 3)
						isSame = MoveSet::FindMove(aRandom, aLayout, aSpec, anOptions, (int) i, &aNth) && (FoundText(aNth) == FoundText(aMoves[i]));
					isSame = isSame && !MoveSet::FindMove(aRandom, aLayout, aSpec, anOptions, (int) aMoves.size(), NULL);
					if (!isSame)
					{
						if (aMismatches++ == 0)
							printf("move set: trial %d, types %d, both %d: %d moves, the brute force %d; first %s vs %s\n", aTrial, aTypes, aBoth,
								(int) aMoves.size(), (int) aBrute.size(), aMoves.empty() ? "-" : FoundText(aMoves[0]).c_str(),
								aBrute.empty() ? "-" : FoundText(aBrute[0]).c_str());
					}
					for (const MoveSet::Found& f : aMoves)
						aFound[f.mMove.mType]++;
				}
			}
		}
	}
	Check(aMismatches == 0, "move set: the search is the brute force's on random grids (" + std::to_string(aMismatches) + " differ)");
	Check((aFound[GameRules::MOVE_SWAP] > 1000) && (aFound[GameRules::MOVE_TWIST] > 1000) && (aFound[GameRules::MOVE_SLIDE] > 1000),
		"move set: the random grids have moves of each type (" + std::to_string(aFound[1]) + " swaps, " + std::to_string(aFound[2]) +
		" twists, " + std::to_string(aFound[4]) + " slides)");
}

// Stage 11 (4.8): the gem a group keeps as a special, and the blasts in turn (Specials.h). theRows a letter a colour, "."
// no gem; theKinds "f" a flame, "l" lightning; theMask the layout ("#" a cell, "." a hole, "d" dirt; all "#" when
// empty). The result: "make f@c,r ...; blast f@c,r:destroyed ..." in order
static std::string SpecialsRun(const std::vector<std::string>& theRows, const std::vector<std::string>& theKinds, const std::string& theRules,
	const std::vector<MatchFinder::Cell>& theMoved = {}, std::vector<std::string> theMask = {})
{
	std::string aJson = "{\"rules\": {\"board\": {\"gemColors\": 12}, " + theRules + "}}";
	Result aResult = Merge({{"s", aJson.c_str()}});
	Check(Clean(aResult), "specials: the rules " + theRules + Messages(aResult));
	MatchFinder::Spec aSpec = MatchFinder::Spec::FromRules(aResult.mRules);
	MatchFinder::ColorGrid aColors = Grid(theRows);
	if (theMask.empty())
		theMask.assign(theRows.size(), std::string(theRows[0].size(), '#'));
	BoardLayout aLayout;
	aLayout.SetMask(aColors.mCols, aColors.mRows, theMask);
	std::vector<MatchFinder::Group> aGroups;
	MatchFinder::FindGroups(aColors, aSpec, &aGroups);
	Specials::Grid aGrid;
	aGrid.mCols = aColors.mCols;
	aGrid.mRows = aColors.mRows;
	for (int r = 0; r < aGrid.mRows; r++)
	{
		for (int c = 0; c < aGrid.mCols; c++)
		{
			aGrid.mGem[r][c] = (aColors.mColor[r][c] != MatchFinder::NO_GEM);
			aGrid.mKind[r][c] = (theKinds.empty() || (theKinds[r][c] == '.')) ? 0 : (theKinds[r][c] == 'f') ? 1 : 2;
		}
	}
	for (const MatchFinder::Group& g : aGroups)
	{
		for (const MatchFinder::Cell& c : g.mCells)
			aGrid.mDying[c.second][c.first] = true;
	}
	std::vector<Specials::Creation> aCreations;
	Specials::Creations(aGroups, aSpec, aResult.mRules.mSpecials, theMoved, aGrid, &aCreations);
	std::string aText = "make";
	for (const Specials::Creation& c : aCreations)
	{
		aGrid.mDying[c.mCell.second][c.mCell.first] = false;
		aText += std::string(" ") + ((c.mKind == 1) ? "f" : "l") + "@" + std::to_string(c.mCell.first) + "," + std::to_string(c.mCell.second);
	}
	std::vector<Specials::Blast> aBlasts;
	Specials::Blasts(aGroups, aLayout, aResult.mRules.mSpecials, aCreations, &aGrid, &aBlasts);
	aText += "; blast";
	for (const Specials::Blast& b : aBlasts)
		aText += std::string(" ") + ((b.mKind == 1) ? "f" : "l") + "@" + std::to_string(b.mCell.first) + "," + std::to_string(b.mCell.second) + ":" +
			std::to_string(b.mDestroyed.size());
	return aText;
}

static void CheckSpecials(const std::string& theGot, const std::string& theExpected, const std::string& theWhat)
{
	Check(theGot == theExpected, "specials: " + theWhat + " (got \"" + theGot + "\")");
}

static void TestSpecials()
{
	// Bejeweled 2's defaults: a run of 4 makes a flame gem (radius 1, 20 points a gem), a run of 5 lightning (30)
	const std::string aB2 = "\"specials\": {\"flame\": {}, \"lightning\": {}}";
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {}, aB2), "make f@2,0; blast", "a run of 4 keeps its middle gem (scan order) as a flame");
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {}, aB2, {{3, 0}, {3, 1}}), "make f@3,0; blast", "... the gem the move put into it");
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {}, aB2, {{2, 1}, {1, 0}}), "make f@1,0; blast", "... the first moved gem in the group");
	CheckSpecials(SpecialsRun({"aaaaa", "bcbcb"}, {}, aB2), "make l@2,0; blast", "a run of 5 makes lightning, not a flame");
	CheckSpecials(SpecialsRun({"aaaaaa", "bcbcbc"}, {}, aB2), "make l@3,0; blast", "a run of 6 makes lightning");
	CheckSpecials(SpecialsRun({"aaa", "bcb"}, {}, aB2), "make; blast", "a run of 3 makes nothing");
	CheckSpecials(SpecialsRun({"ab", "ab", "ab", "ac"}, {}, aB2), "make f@0,2; blast", "a vertical run of 4 (and one of 3)");
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {"..f.", "...."}, aB2), "make f@3,0; blast f@2,0:3",
		"the middle gem a special: the next on round; the flame goes off, sparing the new one");

	// A flame takes the gems not yet dying within its radius; a created special is spared
	CheckSpecials(SpecialsRun({"bcbd", "aaac", "cdcd"}, {"....", ".f..", "...."}, aB2), "make; blast f@1,1:6", "a flame in a run of 3: its 3x3");
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {"f...", "...."}, "\"specials\": {\"flame\": {\"radius\": 2}}"), "make f@2,0; blast f@0,0:3",
		"radius 2; the flame made in the same resolve is spared");
	// Lightning takes its row and column, holes left out, dirt reached but no gem
	CheckSpecials(SpecialsRun({"bcbcb", "aaacd", "cdcdc", "dcdcd"}, {".....", ".l...", ".....", "....."}, aB2), "make; blast l@1,1:5",
		"lightning: its row and its column");
	CheckSpecials(SpecialsRun({"bcbcb", "aaa.d", "cdcdc", "d.dcd"}, {".....", ".l...", ".....", "....."}, aB2,
		{}, {"#####", "###.#", "#####", "#d###"}), "make; blast l@1,1:3", "lightning: a hole and a dirt cell hold no gem");

	// Chains: a special a blast takes goes off in turn with specials.chain, not without
	CheckSpecials(SpecialsRun({"bcbdc", "aaacd", "cdcdc"}, {".....", ".f...", "..l.."}, aB2), "make; blast f@1,1:6 l@2,2:2",
		"chain: the lightning the flame took goes off");
	CheckSpecials(SpecialsRun({"bcbdc", "aaacd", "cdcdc"}, {".....", ".f...", "..l.."}, "\"specials\": {\"flame\": {}, \"lightning\": {}, \"chain\": false}"),
		"make; blast f@1,1:6", "no chain: it only dies");
	CheckSpecials(SpecialsRun({"aaab", "cdcb", "dcdb"}, {"..f.", "....", "...l"}, aB2), "make; blast l@3,2:3 f@2,0:2",
		"two specials in the groups go off in the groups' order: the column's run first");

	// Patterns: lightning checked first; merged crossings, diagonals and shapes name their groups
	CheckSpecials(SpecialsRun({"aaaa", "bcbc"}, {}, "\"specials\": {\"flame\": {\"createdBy\": [\"run4\"]}, \"lightning\": {\"createdBy\": [\"run4\", \"run5\"]}}"),
		"make l@2,0; blast", "a run both name makes lightning");
	CheckSpecials(SpecialsRun({"aaa", "bab", "cad"}, {}, "\"match\": {\"crossings\": \"merge\"}, \"specials\": {\"lightning\": {\"createdBy\": [\"T\"]}}"),
		"make l@1,2; blast", "a T makes lightning at the middle of its cells (its runs' in order)");
	CheckSpecials(SpecialsRun({"abc", "cad", "dca"}, {}, "\"match\": {\"diagonal\": true}, \"specials\": {\"flame\": {\"createdBy\": [\"diagonal\"]}}"),
		"make f@1,1; blast", "a diagonal");
	CheckSpecials(SpecialsRun({"aab", "aac"}, {}, "\"match\": {\"shapes\": {\"sq\": {\"preset\": \"square\"}}}, \"specials\": {\"flame\": {\"createdBy\": [\"sq\"]}}"),
		"make f@1,0; blast", "a shape by its id, at the middle of its orientation's cells");
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
	TestMatchFinder();
	TestMoveSet();
	TestSpecials();
	TestArrows();
	TestGravity();
	TestDirt();
	TestDesignExamples();
	TestFeatures();
	TestJson();
	TestCanonical();
	TestRecorded();
	TestVectors();
	TestMTRand();
	TestReplayVarints();
	TestReplayFormat();
	TestReplayPlayable();
	TestReplaySave();
	TestReplayLimits();
	TestReplayVariantSave();

	printf("rulestest: %d checks, %d failures: %s\n", gChecks, gFailures, gFailures == 0 ? "PASS" : "FAIL");
	return gFailures == 0 ? 0 : 1;
}
