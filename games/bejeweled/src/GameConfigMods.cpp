// port(variant): the half of GameConfig that reads the enabled mods (Platform::ModManager), data\diff.txt
// and the user dir; GameConfig.cpp is the framework-free half. See GameConfig.h.

#include "GameConfig.h"
#include "LiveSplitObserver.h"
#include "GemSheet.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "PakInterface.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sstream>

using namespace Sexy;
using Platform::JsonValue;

// data\diff.txt read exactly as WinDMApp::LoadingThreadProc reads it (the same file through the VFS, the
// same operator>> sequence, no error checks), so the rules carry the values the game plays with
static void ReadDiffValues(DiffValues* theDiff)
{
	std::string aDiffText;
	PFILE* aDiffFile = p_fopen("data\\diff.txt", "r");
	if (aDiffFile != NULL)
	{
		char aBuffer[1024];
		size_t aCount;
		while ((aCount = p_fread(aBuffer, 1, sizeof(aBuffer), aDiffFile)) > 0)
			aDiffText.append(aBuffer, aCount);
		p_fclose(aDiffFile);
	}
	*theDiff = DiffValues();
	std::istringstream aDiffStream(aDiffText);
	aDiffStream >> theDiff->mNormalPointsBase >> theDiff->mNormalPointsPerLevel >> theDiff->mNormalMultBase >> theDiff->mNormalMultPerLevel
		>> theDiff->mTimedPointsBase >> theDiff->mTimedPointsPerLevel >> theDiff->mTimedMultBase >> theDiff->mTimedMultPerLevel
		>> theDiff->mTimeDrainBase >> theDiff->mTimeDrainPerLevel >> theDiff->mTimeDrainAccelBase >> theDiff->mTimeDrainAccelPerLevel;
}

void GameConfig::ResolveRules(GameRules* theRules, GameFeatures* theFeatures, RulesStatus* theStatus)
{
	// Lowest priority first, as ModManager::Apply merges data
	std::vector<RulesSource> aSources;
	const std::vector<Platform::ModInfo>& aMods = Platform::ModManager::Get().GetMods();
	for (std::vector<Platform::ModInfo>::const_reverse_iterator it = aMods.rbegin(); it != aMods.rend(); ++it)
	{
		if (!it->mEnabled)
			continue;
		RulesSource aSource;
		aSource.mModId = it->mId;
		aSource.mTunables = it->mTunablesJson;
		aSource.mRules = it->mRulesJson;
		aSource.mRulesError = it->mRulesError;
		aSource.mFeatures = it->mFeaturesJson;
		aSources.push_back(aSource);
	}

	*theRules = GameRules();
	ReadDiffValues(&theRules->mDiff);
	MergeRules(aSources, theRules, theStatus);
	MergeFeatures(aSources, theFeatures, theStatus);
}

static std::string HexText(const std::string& theBytes)
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

// The mods a merged key came from: the key itself and the keys under it (match.shapes -> each
// match.shapes.<id>), and for moves.type also the moves.addTypes union. Each mod once, in that order.
static std::vector<std::string> ProvenanceFor(const RulesStatus& theStatus, const std::string& theKey)
{
	std::vector<std::string> aMods;
	for (std::map<std::string, std::vector<std::string> >::const_iterator it = theStatus.mProvenance.begin(); it != theStatus.mProvenance.end(); ++it)
	{
		bool isUnder = (it->first.size() > theKey.size()) && (it->first.compare(0, theKey.size(), theKey) == 0) && (it->first[theKey.size()] == '.');
		bool isAdded = (theKey == "moves.type") && (it->first == "moves.addTypes");
		if ((it->first != theKey) && !isUnder && !isAdded)
			continue;
		for (size_t i = 0; i < it->second.size(); i++)
		{
			if (std::find(aMods.begin(), aMods.end(), it->second[i]) == aMods.end())
				aMods.push_back(it->second[i]);
		}
	}
	return aMods;
}

// What this build does not do, as warnings for mods-status.json (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.2): one
// per mod, naming every key of its own that is not played; for rule keys it says what they do change, the RNG (any
// rule gives the board its own). Of the features only the leaderboard is not in this build.
static void AddBuildWarnings(const GameRules& theRules, const GameFeatures& theFeatures, RulesStatus* theStatus)
{
	// per mod: its rule keys, its feature keys
	struct ModKeys
	{
		std::string mModId;
		std::string mRules;
		std::string mFeatures;
	};
	std::vector<ModKeys> aByMod;
	std::vector<std::string> aKeys = theRules.VariantKeys();
#ifndef BEJ_NO_VARIANTS
	// The rules this build plays: kept invalid moves and the noMoves rule (4.1), undo (4.12), the board's shape (4.3),
	// gravity (4.16), dirt (4.14), the match rules (4.6), the move types (4.4, 4.5), the specials (4.8), the gold board
	// (4.9) and arrow gems (4.20), with the arrows that twists turn and their shots.
	// A BEJ_NO_VARIANTS build plays no rule, nor shows look.*.
	static const char* const aPlayed[] = {"moves.allowInvalid", "moves.invalidPenalty", "moves.invalidCountsAsMove", "noMoves.normal",
		"noMoves.timed", "undo.enabled", "undo.depth", "undo.max", "undo.penalty", "undo.inTimed", "undo.restoreTime", "undo.restoreRng",
		"board.cols", "board.rows", "board.mask", "board.levels", "board.gravity.direction", "board.gravity.fallAccel",
		"board.dirtPoints", "match.minRun", "match.diagonal", "match.diagonalMinRun", "match.crossings", "match.shapes",
		"match.bonus", "moves.type", "moves.addTypes", "moves.twist.direction", "moves.slide.snapTicks", "specials.flame",
		"specials.lightning", "specials.chain", "specials.createIn", "gold.enabled", "gold.gild", "gold.timeLimit.normal",
		"gold.timeLimit.timed", "gold.timeBonus", "gold.countDirt", "arrows.enabled", "arrows.directions", "arrows.every",
		"arrows.everyMin", "arrows.bonus", "arrows.turn", "arrows.shots"};
	for (size_t i = 0; i < sizeof(aPlayed) / sizeof(aPlayed[0]); i++)
		aKeys.erase(std::remove(aKeys.begin(), aKeys.end(), std::string(aPlayed[i])), aKeys.end());
#else
	std::vector<std::string> aLookKeys = theRules.LookKeys();
	aKeys.insert(aKeys.end(), aLookKeys.begin(), aLookKeys.end());
#endif
	const size_t aNumRuleKeys = aKeys.size();
	// The feature that is on but not in this build (stage 14's online scores)
	if (theFeatures.mLeaderboard)
		aKeys.push_back("features.leaderboard");
	for (size_t i = 0; i < aKeys.size(); i++)
	{
		std::vector<std::string> aMods = ProvenanceFor(*theStatus, aKeys[i]);
		if (aMods.empty())
			aMods.push_back("");
		for (size_t m = 0; m < aMods.size(); m++)
		{
			size_t j = 0;
			while ((j < aByMod.size()) && (aByMod[j].mModId != aMods[m]))
				j++;
			if (j == aByMod.size())
			{
				aByMod.push_back(ModKeys());
				aByMod[j].mModId = aMods[m];
			}
			std::string& aList = (i < aNumRuleKeys) ? aByMod[j].mRules : aByMod[j].mFeatures;
			aList += (aList.empty() ? "" : ", ") + aKeys[i];
		}
	}
	for (size_t i = 0; i < aByMod.size(); i++)
	{
		const ModKeys& aMod = aByMod[i];
		std::string aText;
		if (!aMod.mRules.empty())
			aText = "not played in this build yet (for now a rule only gives each game its own seed, shown at game over): " + aMod.mRules;
		if (!aMod.mFeatures.empty())
			aText += (aText.empty() ? "" : "; ") + std::string("not in this build yet, no effect: ") + aMod.mFeatures;
		theStatus->mWarnings.push_back({aMod.mModId, aText});
	}
#ifndef BEJ_HAS_LIVESPLIT
	// The LiveSplit block exists in desktop builds only (LiveSplitObserver.h)
	if (theFeatures.mLiveSplit)
		theStatus->mWarnings.push_back({theFeatures.mLiveSplitMod, "features.livesplit: desktop only, ignored"});
#endif

	// The gem sheet's generated rows (stage 8): what the imageOps get wrong
	GemSheet::ModLooks(Platform::ModManager::Get().GetImageOps(), &theStatus->mWarnings);
}

static JsonValue MessagesJson(const std::vector<RulesMessage>& theMessages)
{
	JsonValue anArray = JsonValue::MakeArray();
	for (size_t i = 0; i < theMessages.size(); i++)
	{
		JsonValue aMessage = JsonValue::MakeObject();
		aMessage.Set("mod", JsonValue(theMessages[i].mModId));
		aMessage.Set("text", JsonValue(theMessages[i].mText));
		anArray.Push(aMessage);
	}
	return anArray;
}

// <user dir>/mods-status.json (games/bejeweled/docs/CODE_MODS_DESIGN.md 3.5): what the enabled mods' rules
// resolved to at the last start, for web/shell.html's mods panel
static std::string StatusText(const GameRules& theRules, const GameFeatures& theFeatures, const RulesStatus& theStatus)
{
	JsonValue aStatus = JsonValue::MakeObject();
	aStatus.Set("rulesId", JsonValue(theRules.RulesIdHex()));
	aStatus.Set("canonical", JsonValue(HexText(theRules.Canonical())));
	JsonValue aLines = JsonValue::MakeArray();
	std::vector<std::string> aRuleLines = theRules.CanonicalLines();
	for (size_t i = 0; i < aRuleLines.size(); i++)
		aLines.Push(JsonValue(aRuleLines[i]));
	aStatus.Set("rules", aLines);
	aStatus.Set("features", JsonValue(theFeatures.Text()));
	aStatus.Set("warnings", MessagesJson(theStatus.mWarnings));
	aStatus.Set("errors", MessagesJson(theStatus.mErrors));
	aStatus.Set("dropped", MessagesJson(theStatus.mDropped));
	// key -> [mod, ...]: one mod, several only for the moves.addTypes union
	JsonValue aProvenance = JsonValue::MakeObject();
	for (std::map<std::string, std::vector<std::string> >::const_iterator it = theStatus.mProvenance.begin(); it != theStatus.mProvenance.end(); ++it)
	{
		JsonValue anIds = JsonValue::MakeArray();
		for (size_t i = 0; i < it->second.size(); i++)
			anIds.Push(JsonValue(it->second[i]));
		aProvenance.Set(it->first, anIds);
	}
	aStatus.Set("provenance", aProvenance);

	// The enabled mods this status is for, top priority first
	JsonValue aMods = JsonValue::MakeArray();
	std::vector<const Platform::ModInfo*> anEnabled = Platform::ModManager::Get().GetEnabledMods();
	for (size_t i = 0; i < anEnabled.size(); i++)
	{
		const Platform::ModInfo* aMod = anEnabled[i];
		JsonValue anEntry = JsonValue::MakeObject();
		anEntry.Set("id", JsonValue(aMod->mId));
		anEntry.Set("version", JsonValue(aMod->mVersion));
		anEntry.Set("affectsScores", JsonValue(aMod->AffectsScores()));
		anEntry.Set("rules", JsonValue(aMod->mHasRules));
		anEntry.Set("features", JsonValue(aMod->mHasFeatures));
		anEntry.Set("imageOps", JsonValue(aMod->mHasImageOps));
		anEntry.Set("diff", JsonValue(aMod->mOverridesDiff));
		// port(mods): a mod with settings: the values this start plays with (docs/MOD_MENU_DESIGN.md 8.2)
		if (aMod->HasSettings())
		{
			JsonValue aValues = JsonValue::MakeObject();
			for (const auto& aValue : aMod->mValues)
				aValues.Set(aValue.first, aValue.second);
			anEntry.Set("values", aValues);
		}
		aMods.Push(anEntry);
	}
	aStatus.Set("mods", aMods);
	aStatus.Set("scoreKey", JsonValue(Platform::ModManager::Get().GetScoreKey()));
	return aStatus.Dump(2) + "\n";
}

// A folder named theName (any case) directly in theDir (VFS::FindCaseInsensitive finds files only)
static bool HasFolder(const std::string& theDir, const char* theName)
{
	for (const std::string& anEntry : Platform::ListDirectory(theDir))
	{
		if ((anEntry.size() == strlen(theName)) && (SDL_strcasecmp(anEntry.c_str(), theName) == 0) &&
			Platform::IsDirectory(theDir + "/" + anEntry))
			return true;
	}
	return false;
}

// port(variant): a mod whose only effect is features.livesplit (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 4.15, Q18): its mod.json has only descriptive keys and a "features" object whose one key is "livesplit",
// and its folder has none of the game's data folders. It changes nothing on screen or in play, so it shows
// no mod signs, which would otherwise be in every run video.
// port(mods): read in the effective manifest (the settings' values put in, so a key a value leaves out does not
// count), and "settings" is descriptive: a livesplit mod that gains options stays sign-free
// (docs/MOD_MENU_DESIGN.md 6.2 item 5)
static bool IsLiveSplitOnly(const Platform::ModInfo& theMod)
{
	static const char* const aInfoKeys[] = {"id", "name", "version", "author", "description", "priority", "settings"};
	static const char* const aDataFolders[] = {"images", "sounds", "music", "data", "properties"};
	const JsonValue& aManifest = theMod.mEffective.IsObject() ? theMod.mEffective : theMod.mManifest;
	if (!theMod.mHasManifest || !aManifest.IsObject())
		return false;
	bool hasLiveSplit = false;
	for (size_t i = 0; i < aManifest.Members().size(); i++)
	{
		const std::string& aKey = aManifest.Members()[i].first;
		const JsonValue& aValue = aManifest.Members()[i].second;
		if (aKey == "features")
		{
			if (!aValue.IsObject() || (aValue.Members().size() != 1) || (aValue.Members()[0].first != "livesplit"))
				return false;
			hasLiveSplit = true;
		}
		else if (std::find(std::begin(aInfoKeys), std::end(aInfoKeys), aKey) == std::end(aInfoKeys))
			return false;
	}
	for (const char* aFolder : aDataFolders)
	{
		if (HasFolder(theMod.mPath, aFolder))
			return false;
	}
	return hasLiveSplit;
}

void GameConfig::Load(bool theCanCommit)
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	aMods.CommitNextGameValues(theCanCommit); // port(mods): returns at once when nothing is held
	mStatus = RulesStatus();
	RulesStatus aStatus;
	ResolveRules(&mRules, &mFeatures, &aStatus);
	LoadFromRules(mRules);
	// port(variant): a mod that only turns on LiveSplit does not count (IsLiveSplitOnly)
	mAnyModEnabled = false;
	for (const Platform::ModInfo* aMod : aMods.GetEnabledMods())
		mAnyModEnabled = mAnyModEnabled || !IsLiveSplitOnly(*aMod);

	// Mods without rules or features leave the user dir as it was (no status file, no log lines)
	if (!aMods.HasRulesOrFeatures())
	{
		aMods.WriteStatus("");
		return;
	}

	AddBuildWarnings(mRules, mFeatures, &aStatus);
	mStatus = aStatus;
	std::string aText = StatusText(mRules, mFeatures, aStatus);
	static std::string sLogged;
	if (aText != sLogged)
	{
		sLogged = aText;
		SDL_Log("mods: rules %s, features %s: %d warnings, %d errors, %d dropped", mRules.RulesIdHex().substr(0, 16).c_str(),
			mFeatures.Text().c_str(), (int) aStatus.mWarnings.size(), (int) aStatus.mErrors.size(), (int) aStatus.mDropped.size());
		for (size_t i = 0; i < aStatus.mErrors.size(); i++)
			SDL_Log("mods: error: %s: %s", aStatus.mErrors[i].mModId.c_str(), aStatus.mErrors[i].mText.c_str());
		for (size_t i = 0; i < aStatus.mDropped.size(); i++)
			SDL_Log("mods: dropped: %s: %s", aStatus.mDropped[i].mModId.c_str(), aStatus.mDropped[i].mText.c_str());
		for (size_t i = 0; i < aStatus.mWarnings.size(); i++)
			SDL_Log("mods: warning: %s: %s", aStatus.mWarnings[i].mModId.c_str(), aStatus.mWarnings[i].mText.c_str());
	}
	aMods.WriteStatus(aText);
}

// port(mods): NewGame's Load reads "rules", "tunables" and "features" again (ResolveRules; the legacy tunables through
// ModManager::GetTunable*), so a value used only there applies at the next fresh game. Not features.livesplit: the
// LiveSplit observer's run state and block outlive a board, so turning it on or off waits for a restart. Nor
// features.codeArt: its images are drawn once, at start-up.
bool GameConfig::IsNextGamePath(const std::string& thePath)
{
	auto isUnder = [&thePath](const char* theKey) {
		size_t aLength = strlen(theKey);
		return (thePath.compare(0, aLength, theKey) == 0) && ((thePath.size() == aLength) || (thePath[aLength] == '.'));
	};
	if (isUnder("features.livesplit") || isUnder("features.codeArt") || (thePath == "features"))
		return false;
	return isUnder("rules") || isUnder("tunables") || isUnder("features");
}

std::string GameConfig::DumpRules()
{
	GameRules aRules;
	GameFeatures aFeatures;
	RulesStatus aStatus;
	ResolveRules(&aRules, &aFeatures, &aStatus);
	AddBuildWarnings(aRules, aFeatures, &aStatus);

	std::string aText;
	std::vector<std::string> aLines = aRules.CanonicalLines();
	for (size_t i = 0; i < aLines.size(); i++)
		aText += aLines[i] + "\n";
	aText += "rulesId = " + aRules.RulesIdHex() + "\n";
	aText += "canonical = " + HexText(aRules.Canonical()) + "\n";
	aText += "features = " + aFeatures.Text() + "\n";
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	aText += "scoreKey = " + (aKey.empty() ? std::string("(none)") : aKey) + "\n";
	// Whether this build plays rules at all (the oracle reads this text in the exe: a BEJ_NO_VARIANTS candidate
	// skips the cases that play rules)
#ifdef BEJ_NO_VARIANTS
	aText += "variants = off (a BEJ_NO_VARIANTS build: no rule is played)\n";
#else
	aText += "variants = on\n";
#endif
	for (std::map<std::string, std::vector<std::string> >::const_iterator it = aStatus.mProvenance.begin(); it != aStatus.mProvenance.end(); ++it)
	{
		aText += "provenance " + it->first + " <-";
		for (size_t i = 0; i < it->second.size(); i++)
			aText += std::string(i ? ", " : " ") + it->second[i];
		aText += "\n";
	}
	for (size_t i = 0; i < aStatus.mErrors.size(); i++)
		aText += "error " + aStatus.mErrors[i].mModId + ": " + aStatus.mErrors[i].mText + "\n";
	for (size_t i = 0; i < aStatus.mDropped.size(); i++)
		aText += "dropped " + aStatus.mDropped[i].mModId + ": " + aStatus.mDropped[i].mText + "\n";
	for (size_t i = 0; i < aStatus.mWarnings.size(); i++)
		aText += "warning " + aStatus.mWarnings[i].mModId + ": " + aStatus.mWarnings[i].mText + "\n";
	return aText;
}

// theFileName with theSuffix before its extension
static std::string WithSuffix(const std::string& theFileName, const std::string& theSuffix)
{
	if (theSuffix.empty())
		return theFileName;
	size_t aDot = theFileName.find_last_of('.');
	size_t aSlash = theFileName.find_last_of("/\\");
	if (aDot == std::string::npos || (aSlash != std::string::npos && aDot < aSlash))
		return theFileName + theSuffix;
	return theFileName.substr(0, aDot) + theSuffix + theFileName.substr(aDot);
}

std::string GameConfig::ScopedFileName(const std::string& theFileName)
{
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	return WithSuffix(theFileName, aKey.empty() ? std::string() : "-" + aKey);
}

std::string GameConfig::SeededFileName(const std::string& theFileName)
{
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	return WithSuffix(theFileName, (aKey.empty() ? std::string() : "-" + aKey) + "-seeded");
}
