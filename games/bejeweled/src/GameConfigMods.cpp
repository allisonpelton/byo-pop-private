// port(variant): the half of GameConfig that reads the enabled mods (Platform::ModManager), data\diff.txt
// and the user dir; GameConfig.cpp is the framework-free half. See GameConfig.h.

#include "GameConfig.h"
#include "platform/ModManager.h"
#include "PakInterface.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
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

// What this build does not do yet (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.2: stage 1 reads, merges,
// checks and reports the rules and features; stage 2 plays seeds, so any rule gives the board its own RNG
// and seeds.perGame works, but the rules themselves are not played yet), as warnings for mods-status.json:
// one per mod, naming every key of its own that is not played; for rule keys it says what they do change,
// the RNG. Stage 3 records replays and plays them back from the command line only (-replay=<file>); the replay
// browser comes with stage 13
static void AddBuildWarnings(const GameRules& theRules, const GameFeatures& theFeatures, RulesStatus* theStatus)
{
	// per mod: its rule keys, its feature keys (features.replays apart)
	struct ModKeys
	{
		std::string mModId;
		std::string mRules;
		std::string mFeatures;
		std::string mReplays;
	};
	std::vector<ModKeys> aByMod;
	std::vector<std::string> aKeys = theRules.VariantKeys();
	const size_t aNumRuleKeys = aKeys.size();
	// The features that are on but not complete yet (the seed field comes with the Extras dialog, replays are
	// recorded but watched only from the command line); replays may be on only because the leaderboard
	// implies them
	const char* const aFeatureKeys[3] = {"features.seeds.entry", "features.replays", "features.leaderboard"};
	const bool aFeatureOn[3] = {theFeatures.mSeedEntry, theFeatures.mReplays, theFeatures.mLeaderboard};
	for (int i = 0; i < 3; i++)
	{
		if (aFeatureOn[i])
			aKeys.push_back(aFeatureKeys[i]);
	}
	for (size_t i = 0; i < aKeys.size(); i++)
	{
		// Provenance is kept per feature: features.seeds.entry comes from the mod that set features.seeds
		std::vector<std::string> aMods = ProvenanceFor(*theStatus, (aKeys[i] == "features.seeds.entry") ? "features.seeds" : aKeys[i]);
		if (aMods.empty() && (aKeys[i].compare(0, 9, "features.") == 0))
			aMods = ProvenanceFor(*theStatus, "features.leaderboard");
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
			std::string& aList = (i < aNumRuleKeys) ? aByMod[j].mRules : (aKeys[i] == "features.replays") ? aByMod[j].mReplays : aByMod[j].mFeatures;
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
		if (!aMod.mReplays.empty())
			aText += (aText.empty() ? "" : "; ") + std::string("games are recorded to the replay folder; this build plays them back only from the command line "
				"(-replay=<file>), not yet from the game: ") +
				aMod.mReplays;
		theStatus->mWarnings.push_back({aMod.mModId, aText});
	}

	if (theRules.mBoard.mGemColors > 7)
	{
		char aText[128];
		snprintf(aText, sizeof(aText), "board.gemColors %d: this build has art for 7 gem colours and plays with 7", theRules.mBoard.mGemColors);
		std::vector<std::string> aMods = ProvenanceFor(*theStatus, "board.gemColors");
		theStatus->mWarnings.push_back({aMods.empty() ? std::string() : aMods[0], aText});
	}

	std::vector<std::string> aWarned;
	const std::vector<Platform::ModImageOp>& anOps = Platform::ModManager::Get().GetImageOps();
	for (size_t i = 0; i < anOps.size(); i++)
	{
		const JsonValue& anOp = anOps[i].mOp;
		if (!anOp.IsObject() || !anOp["target"].IsString() || !anOp["op"].IsString())
			theStatus->mWarnings.push_back({anOps[i].mModId, "imageOps: each entry needs \"target\" and \"op\" strings"});
		else if (anOp["op"].AsString() != "extendRows")
			theStatus->mWarnings.push_back({anOps[i].mModId, "imageOps: unknown op \"" + anOp["op"].AsString() + "\" (did you mean extendRows?)"});
		if (std::find(aWarned.begin(), aWarned.end(), anOps[i].mModId) == aWarned.end())
		{
			aWarned.push_back(anOps[i].mModId);
			theStatus->mWarnings.push_back({anOps[i].mModId, "imageOps are not applied by this build yet"});
		}
	}
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
		aMods.Push(anEntry);
	}
	aStatus.Set("mods", aMods);
	aStatus.Set("scoreKey", JsonValue(Platform::ModManager::Get().GetScoreKey()));
	return aStatus.Dump(2) + "\n";
}

void GameConfig::Load()
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	RulesStatus aStatus;
	ResolveRules(&mRules, &mFeatures, &aStatus);
	LoadFromRules(mRules);
	mAnyModEnabled = !aMods.GetEnabledMods().empty();

	// Mods without rules or features leave the user dir as it was (no status file, no log lines)
	if (!aMods.HasRulesOrFeatures())
	{
		aMods.WriteStatus("");
		return;
	}

	AddBuildWarnings(mRules, mFeatures, &aStatus);
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
