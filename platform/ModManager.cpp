#include "ModManager.h"
#include "ImageLoader.h"
#include "UserFiles.h"
#include "VFS.h"
#include "WebPersist.h"

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <set>

namespace Platform
{

static const char* const kConfigFile = "mods.json";
static const char* const kConfigBadFile = "mods.bad.json";
static const char* const kSettingsFile = "mod-settings.json";
static const char* const kSettingsBadFile = "mod-settings.bad.json";
static const char* const kStatusFile = "mods-status.json";

// mod.json with the files that "tunables" and "rules" name read in, and what went wrong reading them
struct ModManager::ModData
{
	JsonValue mJson;
	bool mTunablesFromFile = false;
	bool mTunablesRead = false;
	std::string mTunablesName;
	std::string mTunablesError;
	std::string mRulesError;
};

// mod-settings.json as read
struct ModManager::SettingsFile
{
	std::string mState = "missing";
	JsonValue mRoot;  // always an object
};

ModManager& ModManager::Get()
{
	static ModManager sManager;
	return sManager;
}

static bool ReadTextFile(const std::string& thePath, std::string& theOut)
{
	FILE* f = fopen(thePath.c_str(), "rb");
	if (!f)
		return false;
	char buf[4096];
	size_t n;
	theOut.clear();
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		theOut.append(buf, n);
	fclose(f);
	return true;
}

static std::string UserFilePath(const char* theName)
{
	return VFS::Get().GetUserDir() + "/" + theName;
}

// A file reference of a mod that would leave its folder ("../x")
static bool LeavesRoot(const std::string& theRelPath)
{
	std::string aPath = VFS::Normalize(theRelPath);
	return (aPath == "..") || (aPath.compare(0, 3, "../") == 0);
}

// The file theRelPath names in theMod's folder ("" if missing or outside it)
static std::string ModFile(const ModInfo& theMod, const std::string& theRelPath)
{
	if (theRelPath.empty() || LeavesRoot(theRelPath))
		return "";
	return VFS::FindCaseInsensitive(theMod.mPath, theRelPath);
}

static std::string LowerPath(const std::string& thePath)
{
	std::string aPath = VFS::Normalize(thePath);
	for (char& c : aPath)
		c = (char) tolower((unsigned char) c);
	return aPath;
}

// The FNV-1a key over the sorted entries ("" without any)
static std::string HashKey(std::vector<std::string> theEntries)
{
	if (theEntries.empty())
		return "";
	std::sort(theEntries.begin(), theEntries.end());
	uint32_t h = 2166136261u;
	for (const std::string& s : theEntries)
		for (unsigned char c : s + ";")
			h = (h ^ c) * 16777619u;
	char buf[16];
	snprintf(buf, sizeof(buf), "%08x", h);
	return buf;
}

static bool HasError(const std::vector<ModSettingsError>& theErrors, const char* theKey)
{
	for (const ModSettingsError& anError : theErrors)
		if (anError.mKey == theKey)
			return true;
	return false;
}

// A member set to an object, or removed when the object is empty
static void PutObject(JsonValue* theParent, const std::string& theKey, const JsonValue& theObject)
{
	if (theObject.IsObject() && !theObject.Members().empty())
		theParent->Set(theKey, theObject);
	else
		theParent->Remove(theKey);
}

static JsonValue ObjectAt(const JsonValue& theParent, const std::string& theKey)
{
	return theParent[theKey].IsObject() ? theParent[theKey] : JsonValue::MakeObject();
}

// "files" remaps of theMods (top priority first) from their effective manifests; higher-priority remaps win
static std::map<std::string, VFSRemap> BuildRemaps(const std::vector<const ModInfo*>& theMods,
	const std::vector<const JsonValue*>& theEffective, bool theLog)
{
	// "files": { "<game path>": "<file in mod>" |
	//            { "file": ..., "order": N, "start": sec, "loopStart": sec, "volume": x } }
	// Lower-priority mods are visited first so higher-priority remaps overwrite them.
	std::map<std::string, VFSRemap> remaps;
	for (size_t i = theMods.size(); i-- > 0;)
	{
		const ModInfo& m = *theMods[i];
		for (const auto& kv : (*theEffective[i])["files"].Members())
		{
			const JsonValue& v = kv.second;
			std::string file = v.IsString() ? v.AsString() : v["file"].AsString("");
			std::string real = ModFile(m, file);
			if (real.empty())
			{
				if (theLog)
					SDL_Log("mod %s: file '%s' for '%s' not found", m.mId.c_str(), file.c_str(), kv.first.c_str());
				continue;
			}
			VFSRemap r;
			r.mRealPath = real;
			r.mModId = m.mId;
			r.mStartOrder = v["order"].AsInt(-1);
			r.mStartTime = v["start"].AsNumber(0);
			r.mLoopStart = v["loopStart"].AsNumber(0);
			r.mVolume = v["volume"].AsNumber(1.0);
			// One key per game path however it is written, so the higher priority wins (VFS::SetRemaps)
			remaps[LowerPath(kv.first)] = r;
		}
	}
	return remaps;
}

//////////////////////////////////////////////////////////////////////////
// Scanning

void ModManager::ReadModData(const ModInfo& theMod, ModData* theData) const
{
	*theData = ModData();
	theData->mJson = theMod.mManifest;
	const JsonValue& tun = theMod.mManifest["tunables"];
	if (tun.IsString())
	{
		theData->mTunablesFromFile = true;
		theData->mTunablesName = tun.AsString();
		std::string p = ModFile(theMod, tun.AsString());
		std::string text;
		JsonValue t;
		if (!p.empty() && ReadTextFile(p, text) && ParseJson(text, t, &theData->mTunablesError))
		{
			theData->mTunablesRead = true;
			theData->mJson.Set("tunables", t);
		}
	}
	const JsonValue& rules = theMod.mManifest["rules"];
	if (rules.IsString())
	{
		std::string name = rules.AsString();
		std::string p = ModFile(theMod, name);
		std::string text;
		JsonValue r;
		std::string err;
		if (p.empty())
			theData->mRulesError = "rules file " + name + ": not found";
		else if (!ReadTextFile(p, text))
			theData->mRulesError = "rules file " + name + ": cannot be read";
		else if (!ParseJson(text, r, &err))
			theData->mRulesError = "rules file " + name + ": " + err;
		else
			theData->mJson.Set("rules", r);
	}
}

void ModManager::BuildEffective(const ModInfo& theMod, const ModSettingValues& theValues, ModData* theData, JsonValue* theEffective,
	std::vector<ModSettingsError>* theErrors) const
{
	ReadModData(theMod, theData);
	theErrors->clear();
	if (theMod.HasSettings())
		theMod.mSettings.Resolve(theData->mJson, theValues, theEffective, theErrors);
	else
		*theEffective = theData->mJson;
}

void ModManager::LoadManifest(ModInfo& theMod, bool theLog)
{
	std::string manifestPath = VFS::FindCaseInsensitive(theMod.mPath, "mod.json");
	if (manifestPath.empty())
		return;
	std::string text;
	if (!ReadTextFile(manifestPath, text))
		return;
	JsonValue m;
	std::string err;
	if (!ParseJson(text, m, &err) || !m.IsObject())
	{
		theMod.mError = "mod.json: " + (err.empty() ? std::string("not an object") : err);
		if (theLog)
			SDL_Log("mod %s: %s", theMod.mId.c_str(), theMod.mError.c_str());
		return;
	}
	theMod.mHasManifest = true;
	theMod.mManifest = m;
	if (m["id"].IsString() && !m["id"].AsString().empty())
		theMod.mId = m["id"].AsString();
	theMod.mName = m["name"].AsString(theMod.mId);
	theMod.mVersion = m["version"].AsString("");
	theMod.mAuthor = m["author"].AsString("");
	theMod.mDescription = m["description"].AsString("");
	theMod.mPriority = m["priority"].AsInt(0);
	theMod.mAffectsScores = m["affectsScores"].AsBool(false) || m.Has("tunables") || m.Has("rules");

	// Settings: where their placeholders sit decides when each value applies and whether it splits the scores
	theMod.mSettings.Parse(m);
	if (theMod.HasSettings())
	{
		ModData aData;
		ReadModData(theMod, &aData);
		theMod.mSettings.Analyze(aData.mJson, mNextGameFilter, mGameplayFiles);
	}
}

void ModManager::Reanalyze(std::vector<ModInfo>* theMods)
{
	for (ModInfo& m : *theMods)
	{
		if (!m.HasSettings())
			continue;
		ModData aData;
		ReadModData(m, &aData);
		m.mSettings.Analyze(aData.mJson, mNextGameFilter, mGameplayFiles);
	}
}

void ModManager::SetNextGameFilter(const NextGameFilter& theFilter)
{
	mNextGameFilter = theFilter;
	Reanalyze(&mMods);
	Reanalyze(&mDesired);
}

void ModManager::SetGameplayFiles(const std::vector<std::string>& theFiles)
{
	mGameplayFiles = theFiles;
	Reanalyze(&mMods);
	Reanalyze(&mDesired);
}

bool ModManager::ReadConfig(JsonValue* theConfig, bool* theIsBroken) const
{
	*theConfig = JsonValue::MakeObject();
	*theIsBroken = false;
	std::string text;
	if (!ReadHostFile(UserFilePath(kConfigFile), &text))
		return false;
	JsonValue cfg;
	if (ParseJson(text, cfg) && cfg.IsObject())
		*theConfig = cfg;
	else
		*theIsBroken = true;
	return true;
}

// Saved order / enabled flags: priority first, then the saved order; mods not listed are appended (in folder
// order) and enabled
void ModManager::SortByConfig(std::vector<ModInfo>* theMods, const JsonValue& theConfig)
{
	std::stable_sort(theMods->begin(), theMods->end(), [](const ModInfo& a, const ModInfo& b) { return a.mDir < b.mDir; });
	for (ModInfo& m : *theMods)
		m.mEnabled = !m.mDuplicate;
	std::vector<std::string> order;
	if (theConfig.IsObject())
	{
		for (const JsonValue& v : theConfig["order"].Items())
			order.push_back(v.AsString());
		for (const JsonValue& v : theConfig["disabled"].Items())
			for (ModInfo& m : *theMods)
				if (m.mId == v.AsString())
					m.mEnabled = false;
	}
	auto rank = [&](const ModInfo& m) {
		auto it = std::find(order.begin(), order.end(), m.mId);
		return it == order.end() ? (int)order.size() : (int)(it - order.begin());
	};
	std::stable_sort(theMods->begin(), theMods->end(), [&](const ModInfo& a, const ModInfo& b) {
		if (a.mPriority != b.mPriority)
			return a.mPriority > b.mPriority;
		return rank(a) < rank(b);
	});
}

void ModManager::ScanInto(std::vector<ModInfo>* theMods, bool theLog)
{
	theMods->clear();
	std::vector<std::string> names = ListDirectory(mModsDir);
	std::sort(names.begin(), names.end());
	std::set<std::string> ids;
	for (const std::string& n : names)
	{
		std::string path = mModsDir + "/" + n;
		if (!IsDirectory(path))
			continue;
		ModInfo mod;
		mod.mId = n;
		mod.mName = n;
		mod.mPath = path;
		mod.mDir = n;
		LoadManifest(mod, theLog);
		// Two folders with one id would share their entries in mods.json and mod-settings.json
		if (!ids.insert(mod.mId).second)
		{
			mod.mDuplicate = true;
			mod.mError = "another mod folder has the id \"" + mod.mId + "\": this one is not loaded";
		}
		theMods->push_back(mod);
	}

	JsonValue cfg;
	bool isBroken;
	ReadConfig(&cfg, &isBroken);
	SortByConfig(theMods, cfg);
}

void ModManager::Refresh()
{
	ScanInto(&mMods, true);
	SettingsFile aFile;
	ReadSettingsFile(&aFile);
	mSettingsState = aFile.mState;
	SetValuesFromRoot(&mMods, aFile.mRoot, false);
	Apply();
	mApplied = true;

	// The desired state starts as what was just read, with the values held for the next game
	mDesired = mMods;
	SetValuesFromRoot(&mDesired, aFile.mRoot, true);
	mDesiredValid = true;
	Changed();
}

std::vector<const ModInfo*> ModManager::GetEnabledMods() const
{
	std::vector<const ModInfo*> out;
	for (const ModInfo& m : mMods)
		if (m.mEnabled)
			out.push_back(&m);
	return out;
}

void ModManager::FlattenInto(const JsonValue& theValue, const std::string& thePrefix)
{
	if (theValue.IsObject())
	{
		for (const auto& kv : theValue.Members())
			FlattenInto(kv.second, thePrefix.empty() ? kv.first : thePrefix + "." + kv.first);
	}
	else if (!thePrefix.empty())
		mTunables[thePrefix] = theValue;
}

//////////////////////////////////////////////////////////////////////////
// Applying

// "tunables", "rules" and "features" of a mod's effective manifest into its fields (not the flat tunables)
void ModManager::SetRuleData(ModInfo& m, const ModData& theData, bool theLog)
{
	const JsonValue& man = m.mEffective;
	bool isTunablesBroken = HasError(m.mSettingErrors, "tunables");
	bool isRulesBroken = HasError(m.mSettingErrors, "rules");

	const JsonValue& tun = man["tunables"];
	m.mTunablesJson = JsonValue();
	if (theData.mTunablesFromFile)
	{
		if (theData.mTunablesRead)
			m.mTunablesJson = tun;
		else if (theLog)
			SDL_Log("mod %s: cannot read tunables %s %s", m.mId.c_str(), theData.mTunablesName.c_str(), theData.mTunablesError.c_str());
	}
	else if (tun.IsObject())
		m.mTunablesJson = tun;

	// port(variant): "rules" (inline, or a JSON file in the mod like "tunables"), "features" and
	// "imageOps"; GameConfig::Load merges the rules and features (game/GameRules.h)
	m.mHasRules = man.Has("rules") || isRulesBroken;
	m.mHasFeatures = man.Has("features");
	m.mHasImageOps = man.Has("imageOps");
	m.mRulesJson = man["rules"];
	m.mRulesError = theData.mRulesError;
	if (!m.mRulesError.empty() && theLog)
		SDL_Log("mod %s: %s", m.mId.c_str(), m.mRulesError.c_str());
	if (isRulesBroken)
	{
		// As a mod whose rules fail their own checks: reported, none of its rules merged
		m.mRulesJson = JsonValue();
		for (const ModSettingsError& anError : m.mSettingErrors)
			if (anError.mKey == "rules")
				m.mRulesError = "rules: " + anError.mText;
	}
	m.mFeaturesJson = man["features"];
	m.mAffectsScores = man["affectsScores"].AsBool(false) || man.Has("tunables") || man.Has("rules") || isTunablesBroken || isRulesBroken;
}

void ModManager::RebuildTunables()
{
	mTunables.clear();
	for (auto it = mMods.rbegin(); it != mMods.rend(); ++it)
		if (it->mEnabled)
			FlattenInto(it->mTunablesJson, "");
}

void ModManager::ComputeScoreKey()
{
	// Stable key for separate high-score tables (FNV-1a over the gameplay-affecting mods).
	std::vector<std::string> scoreMods;
	for (ModInfo& m : mMods)
	{
		if (!m.mEnabled)
			continue;
		m.mScorePairs = m.mSettings.ScoreKeyPairs(m.mValues);
		if (m.AffectsScores())
			scoreMods.push_back(m.mId + "@" + m.mVersion + (m.mScorePairs.empty() ? "" : "?" + m.mScorePairs));
	}
	mScoreKey = HashKey(scoreMods);
}

void ModManager::Apply()
{
	std::vector<VFSRoot> roots;
	ClearImageScaleOverrides();
	ClearImagePatches();
	mTunables.clear();
	mStrings.clear();
	mPropertyFiles.clear();

	// The effective manifests: the files "tunables" and "rules" name read in, the settings' values put in.
	// Problems reading those files are logged below, where they were always logged.
	std::vector<ModData> aData(mMods.size());
	for (size_t i = 0; i < mMods.size(); i++)
	{
		ModInfo& m = mMods[i];
		m.mSettingErrors.clear();
		if (!m.mEnabled)
			continue;
		BuildEffective(m, m.mValues, &aData[i], &m.mEffective, &m.mSettingErrors);
		for (const ModSettingsError& anError : m.mSettingErrors)
			SDL_Log("mod %s: settings: %s: %s", m.mId.c_str(), anError.mKey.c_str(), anError.mText.c_str());
	}

	std::vector<const ModInfo*> enabled = GetEnabledMods();
	std::vector<const JsonValue*> effective;
	for (const ModInfo* m : enabled)
	{
		roots.push_back({m->mPath, m->mId});
		effective.push_back(&m->mEffective);
	}
	VFS::Get().SetModRoots(roots);
	VFS::Get().SetRemaps(BuildRemaps(enabled, effective, true));

	// port(variant): a mod that replaces data/diff.txt (a file in the mod or a "files" remap) changes the
	// level parameters, so it counts for the score key (the game's gameplay files, SetGameplayFiles)
	std::set<std::string> gameplayMods;
	for (const std::string& aFile : mGameplayFiles)
	{
		std::string owner;
		VFS::Get().Resolve(aFile, &owner);
		if (!owner.empty())
			gameplayMods.insert(owner);
	}
	mImageOps.clear();

	// Data merges run lowest priority first so higher-priority mods win.
	for (size_t i = mMods.size(); i-- > 0;)
	{
		ModInfo& m = mMods[i];
		if (!m.mEnabled)
			continue;
		const JsonValue& man = m.mEffective;
		if (man["imageScale"].AsInt(1) > 1)
			SetModDefaultImageScale(m.mId, man["imageScale"].AsInt(1));
		for (const auto& kv : man["images"].Members())
			if (kv.second["scale"].AsInt(1) > 1)
				SetImageScaleOverride(m.mId, kv.first, kv.second["scale"].AsInt(1));

		// "imagePatches": [{"target": "images/gemsheet6", "image": "patches/x@2x.png", "x": 0, "y": 520,
		//                   "scale": 2 (optional, else from the @Nx suffix), "mode": "replace" | "over"}]
		for (const JsonValue& p : man["imagePatches"].Items())
		{
			std::string image = p["image"].AsString("");
			std::string real = ModFile(m, image);
			if (real.empty() || !p["target"].IsString())
			{
				SDL_Log("mod %s: image patch '%s' not found", m.mId.c_str(), image.c_str());
				continue;
			}
			int scale = p["scale"].AsInt(0);
			if (scale <= 0)
			{
				size_t at = image.rfind('@');
				scale = at != std::string::npos ? atoi(image.c_str() + at + 1) : 1;
			}
			AddImagePatch(p["target"].AsString(), real, p["x"].AsInt(0), p["y"].AsInt(0), std::max(1, scale),
				p["mode"].AsString("replace") != "over");
		}

		const JsonValue& strings = man["strings"];
		if (strings.IsObject())
		{
			for (const auto& kv : strings.Members())
				mStrings[kv.first] = kv.second.AsString();
		}
		else if (strings.IsArray())
		{
			// Store host paths: two mods may both ship "properties/strings.xml".
			for (const JsonValue& v : strings.Items())
			{
				std::string real = ModFile(m, v.AsString());
				if (real.empty())
					SDL_Log("mod %s: strings file '%s' not found", m.mId.c_str(), v.AsString().c_str());
				else
					mPropertyFiles.push_back(real);
			}
		}

		SetRuleData(m, aData[i], true);
		FlattenInto(m.mTunablesJson, "");
		m.mOverridesDiff = gameplayMods.count(m.mId) != 0;
		for (const JsonValue& op : man["imageOps"].Items())
			mImageOps.push_back({m.mId, op});
	}

	ComputeScoreKey();
	SDL_Log("mods: %s", GetSummary().c_str());
}

bool ModManager::HasTunable(const std::string& theKey) const
{
	return mTunables.count(theKey) != 0;
}

double ModManager::GetTunable(const std::string& theKey, double theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() ? theDefault : it->second.AsNumber(theDefault);
}

int ModManager::GetTunableInt(const std::string& theKey, int theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() || !it->second.IsNumber() ? theDefault : (int)it->second.AsNumber();
}

bool ModManager::GetTunableBool(const std::string& theKey, bool theDefault) const
{
	auto it = mTunables.find(theKey);
	return it == mTunables.end() ? theDefault : it->second.AsBool(theDefault);
}

std::vector<double> ModManager::GetTunableArray(const std::string& theKey, const std::vector<double>& theDefault) const
{
	auto it = mTunables.find(theKey);
	if (it == mTunables.end() || !it->second.IsArray())
		return theDefault;
	std::vector<double> out;
	for (const JsonValue& v : it->second.Items())
		out.push_back(v.AsNumber());
	return out.empty() ? theDefault : out;
}

bool ModManager::HasRulesOrFeatures() const
{
	for (const ModInfo* m : GetEnabledMods())
		if (m->mHasRules || m->mHasFeatures)
			return true;
	return false;
}

void ModManager::WriteStatus(const std::string& theText)
{
	if (theText.empty())
	{
		mStatusWritten.clear();
		// Only a stale file from an earlier start is removed: a user dir without one stays untouched
		RemoveUserFile(kStatusFile);
		return;
	}
	if (theText == mStatusWritten)
		return;
	if (!WriteUserFileAtomic(kStatusFile, theText))
	{
		SDL_Log("mods: cannot write %s", UserFilePath(kStatusFile).c_str());
		return;
	}
	mStatusWritten = theText;
}

std::string ModManager::GetSummary() const
{
	std::string s;
	for (const ModInfo* m : GetEnabledMods())
		s += (s.empty() ? "" : ", ") + m->mName;
	return s.empty() ? "none" : s;
}

//////////////////////////////////////////////////////////////////////////
// Values

void ModManager::ReadSettingsFile(SettingsFile* theFile) const
{
	theFile->mState = "missing";
	theFile->mRoot = JsonValue::MakeObject();
	std::string text;
	if (!ReadHostFile(UserFilePath(kSettingsFile), &text))
		return;
	JsonValue root;
	if (!ParseJson(text, root) || !root.IsObject())
	{
		theFile->mState = "unreadable";
		return;
	}
	theFile->mState = (root["v"].IsNumber() && (root["v"].AsNumber() > 1)) ? "newer" : "ok";
	theFile->mRoot = root;
}

std::string ModManager::WriteSettingsFile(const SettingsFile& theFile, const JsonValue& theRoot)
{
	if (theFile.mState == "newer")
		return "mod-settings.json was made by a newer version of the game; it is left as it is";
	// Nothing changed: no write (a missing or unreadable file stands for no values)
	JsonValue anOld = (theFile.mState == "ok") ? theFile.mRoot : JsonValue::MakeObject();
	if (theRoot.Dump() == anOld.Dump())
		return "";
	JsonValue aRoot = theRoot;
	if (!aRoot.Has("v"))
	{
		aRoot = JsonValue::MakeObject();
		aRoot.Set("v", JsonValue(1.0));
		for (const auto& kv : theRoot.Members())
			aRoot.Set(kv.first, kv.second);
	}
	if (theFile.mState == "unreadable")
		RenameUserFile(kSettingsFile, kSettingsBadFile);
	if (!WriteUserFileAtomic(kSettingsFile, aRoot.Dump(2) + "\n"))
		return "mod-settings.json could not be written";
	mSettingsState = "ok";
	return "";
}

void ModManager::SetValuesFromRoot(std::vector<ModInfo>* theMods, const JsonValue& theRoot, bool theNext)
{
	const JsonValue& aSection = theRoot[theNext ? "next" : "mods"];
	for (ModInfo& m : *theMods)
	{
		if (theNext)
		{
			m.mNextValues.clear();
			if (m.HasSettings())
				m.mSettings.ReadValues(aSection[m.mId], false, &m.mNextValues, nullptr);
		}
		else
		{
			m.mValueWarnings.clear();
			m.mValues.clear();
			if (m.HasSettings())
				m.mSettings.ReadValues(aSection[m.mId], true, &m.mValues, &m.mValueWarnings);
		}
	}
}

ModInfo* ModManager::FindApplied(const std::string& theId)
{
	for (ModInfo& m : mMods)
		if ((m.mId == theId) && !m.mDuplicate)
			return &m;
	return nullptr;
}

const ModInfo* ModManager::FindApplied(const std::string& theId) const
{
	for (const ModInfo& m : mMods)
		if ((m.mId == theId) && !m.mDuplicate)
			return &m;
	return nullptr;
}

ModInfo* ModManager::FindDesiredMutable(const std::string& theId)
{
	EnsureDesired();
	for (ModInfo& m : mDesired)
		if ((m.mId == theId) && !m.mDuplicate)
			return &m;
	return nullptr;
}

const ModInfo* ModManager::FindDesired(const std::string& theId)
{
	return FindDesiredMutable(theId);
}

JsonValue ModManager::DesiredValue(const ModInfo& theMod, const ModSettingDef& theDef) const
{
	auto it = theMod.mNextValues.find(theDef.mId);
	if (it != theMod.mNextValues.end())
		return it->second;
	return theMod.mSettings.GetValue(theMod.mValues, theDef);
}

JsonValue ModManager::GetLiveValue(const std::string& theModId, const std::string& theSettingId) const
{
	const ModInfo* m = FindApplied(theModId);
	const ModSettingDef* aDef = m ? m->mSettings.Find(theSettingId) : nullptr;
	if (aDef == nullptr)
		return JsonValue();
	return m->mSettings.EffectiveValue(m->mValues, *aDef);
}

bool ModManager::HasNextGameValues() const
{
	for (const ModInfo& m : mDesired)
		if (!m.mNextValues.empty())
			return true;
	return false;
}

bool ModManager::CommitNextGameValues(bool theCanCommit)
{
	if (!theCanCommit || !HasNextGameValues())
		return false;
	SettingsFile aFile;
	ReadSettingsFile(&aFile);
	if (aFile.mState == "newer")
		return false;
	JsonValue aRoot = aFile.mRoot;
	const JsonValue aNext = aRoot["next"];
	if (!aNext.IsObject() || aNext.Members().empty())
	{
		// Changed on disk since: nothing is held any more
		SetValuesFromRoot(&mDesired, aRoot, true);
		Changed();
		return false;
	}

	JsonValue aMods = ObjectAt(aRoot, "mods");
	std::set<std::string> aChanged;
	for (const auto& aModEntry : aNext.Members())
	{
		JsonValue aModValues = ObjectAt(aMods, aModEntry.first);
		const ModInfo* aDesired = FindDesired(aModEntry.first);
		ModInfo* anApplied = FindApplied(aModEntry.first);
		for (const auto& kv : aModEntry.second.Members())
		{
			const ModSettingDef* aDef = aDesired ? aDesired->mSettings.Find(kv.first) : nullptr;
			if (aDef == nullptr)
			{
				aModValues.Set(kv.first, kv.second);
				continue;
			}
			JsonValue aValue;
			std::string aWarning;
			aDesired->mSettings.Normalize(*aDef, kv.second, &aValue, &aWarning);
			if (ModSettingSame(*aDef, aValue, aDef->mDefault))
				aModValues.Remove(kv.first);
			else
				aModValues.Set(kv.first, aValue);
			const ModSettingDef* anAppliedDef = anApplied ? anApplied->mSettings.Find(kv.first) : nullptr;
			if ((anAppliedDef != nullptr) && anApplied->mEnabled && (aDef->mLevel == MOD_LEVEL_NEXT_GAME))
			{
				anApplied->mValues[kv.first] = aValue;
				aChanged.insert(anApplied->mId);
			}
		}
		PutObject(&aMods, aModEntry.first, aModValues);
	}
	aRoot.Remove("next");
	PutObject(&aRoot, "mods", aMods);
	WriteSettingsFile(aFile, aRoot);
	SetValuesFromRoot(&mDesired, aRoot, false);
	SetValuesFromRoot(&mDesired, aRoot, true);

	// The changed mods' "tunables", "rules" and "features" (only data that applies at the next game; the rest
	// of their effective manifests stays as the start made it)
	for (ModInfo& m : mMods)
	{
		if (!aChanged.count(m.mId))
			continue;
		ModData aData;
		JsonValue anEffective;
		std::vector<ModSettingsError> anErrors;
		BuildEffective(m, m.mValues, &aData, &anEffective, &anErrors);
		static const char* const aKeys[] = {"tunables", "rules", "features"};
		std::vector<ModSettingsError> aKept;
		for (const ModSettingsError& anError : m.mSettingErrors)
			if ((anError.mKey != "tunables") && (anError.mKey != "rules") && (anError.mKey != "features"))
				aKept.push_back(anError);
		for (const char* aKey : aKeys)
		{
			if (anEffective.Has(aKey))
				m.mEffective.Set(aKey, anEffective[aKey]);
			else
				m.mEffective.Remove(aKey);
			for (const ModSettingsError& anError : anErrors)
				if (anError.mKey == aKey)
					aKept.push_back(anError);
		}
		m.mSettingErrors = aKept;
		SetRuleData(m, aData, false);
	}
	RebuildTunables();
	ComputeScoreKey();
	Changed();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// The desired state

void ModManager::Changed()
{
	mChangeSerial++;
	if (mChangeCallback)
		mChangeCallback();
}

void ModManager::EnsureDesired()
{
	if (!mDesiredValid)
		ScanDesired();
}

void ModManager::ScanDesired()
{
	ScanInto(&mDesired, false);
	SettingsFile aFile;
	ReadSettingsFile(&aFile);
	mSettingsState = aFile.mState;
	SetValuesFromRoot(&mDesired, aFile.mRoot, false);
	SetValuesFromRoot(&mDesired, aFile.mRoot, true);
	mDesiredValid = true;
	Changed();
}

const std::vector<ModInfo>& ModManager::GetDesiredMods()
{
	EnsureDesired();
	return mDesired;
}

std::string ModManager::WriteConfig(const JsonValue& theOld, const JsonValue& theConfig, bool theIsBroken)
{
	if (!theIsBroken && (theOld.Dump() == theConfig.Dump()))
		return "";
	if (theIsBroken)
		RenameUserFile(kConfigFile, kConfigBadFile);
	if (!WriteUserFileAtomic(kConfigFile, theConfig.Dump(2) + "\n"))
		return "mods.json could not be written";
	return "";
}

std::string ModManager::EditEnable(const std::string& theId, bool theEnabled)
{
	ModInfo* m = FindDesiredMutable(theId);
	if (m == nullptr)
		return "there is no mod \"" + theId + "\"";
	JsonValue cfg;
	bool isBroken;
	ReadConfig(&cfg, &isBroken);
	JsonValue anOld = cfg;
	bool isDisabled = false;
	JsonValue disabled = JsonValue::MakeArray();
	for (const JsonValue& v : cfg["disabled"].Items())
	{
		if (v.IsString() && (v.AsString() == theId))
			isDisabled = true;
		else
			disabled.Push(v);
	}
	std::string anError;
	if (isDisabled == theEnabled)
	{
		if (!theEnabled)
			disabled.Push(JsonValue(theId));
		cfg.Set("disabled", disabled);
		anError = WriteConfig(anOld, cfg, isBroken);
	}
	SortByConfig(&mDesired, anError.empty() ? cfg : anOld);
	Changed();
	return anError;
}

bool ModManager::CanMove(const std::string& theId, int theDelta)
{
	EnsureDesired();
	for (size_t i = 0; i < mDesired.size(); i++)
	{
		if (mDesired[i].mId != theId)
			continue;
		long j = (long) i + theDelta;
		return ((theDelta == -1) || (theDelta == 1)) && (j >= 0) && (j < (long) mDesired.size()) &&
			(mDesired[(size_t) j].mPriority == mDesired[i].mPriority);
	}
	return false;
}

std::string ModManager::EditMove(const std::string& theId, int theDelta)
{
	EnsureDesired();
	JsonValue cfg;
	bool isBroken;
	ReadConfig(&cfg, &isBroken);
	SortByConfig(&mDesired, cfg);
	if (FindDesired(theId) == nullptr)
		return "there is no mod \"" + theId + "\"";
	if (!CanMove(theId, theDelta))
		return "\"" + theId + "\" cannot move there: mods move only among mods of the same priority";

	// The ids in their new order (one entry per id)
	std::vector<std::string> ids;
	for (const ModInfo& m : mDesired)
		if (std::find(ids.begin(), ids.end(), m.mId) == ids.end())
			ids.push_back(m.mId);
	size_t i = (size_t) (std::find(ids.begin(), ids.end(), theId) - ids.begin());
	std::swap(ids[i], ids[(size_t) ((long) i + theDelta)]);

	// The slots of the listed ids take the new order; ids that are not on disk keep theirs
	JsonValue anOld = cfg;
	JsonValue order = JsonValue::MakeArray();
	std::set<std::string> onDisk(ids.begin(), ids.end()), seen;
	size_t next = 0;
	for (const JsonValue& v : cfg["order"].Items())
	{
		if (v.IsString() && onDisk.count(v.AsString()))
		{
			if (seen.insert(v.AsString()).second)
				order.Push(JsonValue(ids[next++]));
		}
		else
			order.Push(v);
	}
	while (next < ids.size())
		order.Push(JsonValue(ids[next++]));
	cfg.Set("order", order);
	std::string anError = WriteConfig(anOld, cfg, isBroken);
	SortByConfig(&mDesired, anError.empty() ? cfg : anOld);
	Changed();
	return anError;
}

// One setting's value into a mod-settings.json root: values that apply at the next game wait in "next"
// unless they equal the saved value; the others go into "mods", which keeps only values off their default
static void SetValueInRoot(JsonValue* theRoot, const ModInfo& theMod, const ModSettingDef& theDef, const JsonValue& theValue)
{
	JsonValue aMods = ObjectAt(*theRoot, "mods");
	JsonValue aNext = ObjectAt(*theRoot, "next");
	JsonValue aModValues = ObjectAt(aMods, theMod.mId);
	JsonValue aModNext = ObjectAt(aNext, theMod.mId);
	if (theDef.mLevel == MOD_LEVEL_NEXT_GAME)
	{
		JsonValue aSaved = theDef.mDefault;
		std::string aWarning;
		if (aModValues.Has(theDef.mId))
			theMod.mSettings.Normalize(theDef, aModValues[theDef.mId], &aSaved, &aWarning);
		if (ModSettingSame(theDef, theValue, aSaved))
			aModNext.Remove(theDef.mId);
		else
			aModNext.Set(theDef.mId, theValue);
	}
	else
	{
		if (ModSettingSame(theDef, theValue, theDef.mDefault))
			aModValues.Remove(theDef.mId);
		else
			aModValues.Set(theDef.mId, theValue);
		aModNext.Remove(theDef.mId);
	}
	// Only what changed is put back: an empty object the file already has stays (no write for a no-op)
	if (aModValues.Dump(0) != ObjectAt(aMods, theMod.mId).Dump(0))
	{
		PutObject(&aMods, theMod.mId, aModValues);
		PutObject(theRoot, "mods", aMods);
	}
	if (aModNext.Dump(0) != ObjectAt(aNext, theMod.mId).Dump(0))
	{
		PutObject(&aNext, theMod.mId, aModNext);
		PutObject(theRoot, "next", aNext);
	}
}

std::string ModManager::EditSetting(const std::string& theId, const std::string& theSettingId, const JsonValue& theValue)
{
	ModInfo* m = FindDesiredMutable(theId);
	if (m == nullptr)
		return "there is no mod \"" + theId + "\"";
	const ModSettingDef* aDef = m->mSettings.Find(theSettingId);
	if (aDef == nullptr)
		return "mod \"" + theId + "\" has no setting \"" + theSettingId + "\"";
	JsonValue aValue;
	std::string aWarning;
	if (m->mSettings.Normalize(*aDef, theValue, &aValue, &aWarning) == MOD_VALUE_WRONG)
		return aDef->mLabel + ": " + theValue.Dump(0) + " is " + aWarning;

	SettingsFile aFile;
	ReadSettingsFile(&aFile);
	JsonValue aRoot = aFile.mRoot;
	SetValueInRoot(&aRoot, *m, *aDef, aValue);
	std::string anError = WriteSettingsFile(aFile, aRoot);
	if (!anError.empty())
		return anError;
	SetValuesFromRoot(&mDesired, aRoot, false);
	SetValuesFromRoot(&mDesired, aRoot, true);
	if (aDef->mLevel == MOD_LEVEL_LIVE)
	{
		// Game code asks for it each time: it applies at once
		ModInfo* anApplied = FindApplied(theId);
		if ((anApplied != nullptr) && (anApplied->mSettings.Find(theSettingId) != nullptr))
			anApplied->mValues[theSettingId] = aValue;
	}
	Changed();
	return "";
}

std::string ModManager::EditReset(const std::string& theId)
{
	ModInfo* m = FindDesiredMutable(theId);
	if (m == nullptr)
		return "there is no mod \"" + theId + "\"";
	SettingsFile aFile;
	ReadSettingsFile(&aFile);
	JsonValue aRoot = aFile.mRoot;
	for (const ModSettingDef& aDef : m->mSettings.mDefs)
		if (aDef.IsValue())
			SetValueInRoot(&aRoot, *m, aDef, aDef.mDefault);
	std::string anError = WriteSettingsFile(aFile, aRoot);
	if (!anError.empty())
		return anError;
	SetValuesFromRoot(&mDesired, aRoot, false);
	SetValuesFromRoot(&mDesired, aRoot, true);
	ModInfo* anApplied = FindApplied(theId);
	if (anApplied != nullptr)
	{
		for (const ModSettingDef& aDef : m->mSettings.mDefs)
			if (aDef.IsValue() && (aDef.mLevel == MOD_LEVEL_LIVE) && (anApplied->mSettings.Find(aDef.mId) != nullptr))
				anApplied->mValues[aDef.mId] = aDef.mDefault;
	}
	Changed();
	return "";
}

std::string ModManager::Edit(const std::string& theJson)
{
	JsonValue anEdit;
	std::string anError;
	if (!ParseJson(theJson, anEdit, &anError) || !anEdit.IsObject())
		return "not a JSON object: " + anError;
	std::string anOp = anEdit["op"].AsString("");
	std::string aMod = anEdit["mod"].AsString("");
	if (anOp == "enable")
		return EditEnable(aMod, anEdit["on"].AsBool(true));
	if (anOp == "move")
		return EditMove(aMod, anEdit["delta"].AsInt(0));
	if (anOp == "set")
		return EditSetting(aMod, anEdit["setting"].AsString(""), anEdit["value"]);
	if (anOp == "reset")
		return EditReset(aMod);
	return "unknown op \"" + anOp + "\"";
}

ModPending ModManager::GetPending()
{
	EnsureDesired();
	ModPending aPending;
	if (!mApplied)
		return aPending;  // -nomods: nothing is applied this run
	auto isAppliedOn = [&](const std::string& theId) {
		const ModInfo* a = FindApplied(theId);
		return (a != nullptr) && a->mEnabled;
	};
	auto isDesiredOn = [&](const std::string& theId) {
		const ModInfo* d = FindDesired(theId);
		return (d != nullptr) && d->mEnabled;
	};
	for (const ModInfo& d : mDesired)
	{
		if (d.mDuplicate)
			continue;
		if (d.mEnabled && !isAppliedOn(d.mId))
			aPending.mRestart.push_back(d.mName + (FindApplied(d.mId) ? " turned on" : " added"));
		else if (!d.mEnabled && isAppliedOn(d.mId))
			aPending.mRestart.push_back(d.mName + " turned off");
	}
	for (const ModInfo& a : mMods)
		if (a.mEnabled && (FindDesired(a.mId) == nullptr))
			aPending.mRestart.push_back(a.mName + " removed");
	std::vector<std::string> anAppliedOrder, aDesiredOrder;
	for (const ModInfo& a : mMods)
		if (a.mEnabled && isDesiredOn(a.mId))
			anAppliedOrder.push_back(a.mId);
	for (const ModInfo& d : mDesired)
		if (d.mEnabled && !d.mDuplicate && isAppliedOn(d.mId))
			aDesiredOrder.push_back(d.mId);
	if (anAppliedOrder != aDesiredOrder)
		aPending.mRestart.push_back("Load order");

	for (const ModInfo& d : mDesired)
	{
		const ModInfo* a = FindApplied(d.mId);
		if (!d.mEnabled || d.mDuplicate || !d.HasSettings() || (a == nullptr) || !a->mEnabled)
			continue;
		for (const ModSettingDef& aDef : d.mSettings.mDefs)
		{
			if (!aDef.IsValue() || (aDef.mLevel == MOD_LEVEL_LIVE))
				continue;
			const ModSettingDef* anAppliedDef = a->mSettings.Find(aDef.mId);
			if ((anAppliedDef != nullptr) && ModSettingSame(aDef, DesiredValue(d, aDef), a->mSettings.GetValue(a->mValues, *anAppliedDef)))
				continue;
			(aDef.mLevel == MOD_LEVEL_NEXT_GAME ? aPending.mNextGame : aPending.mRestart).push_back(aDef.mLabel);
		}
	}
	return aPending;
}

// The desired state's mods with the values they would run with, their effective manifests and errors
struct ModManager::DesiredView
{
	std::vector<ModSettingValues> mValues;
	std::vector<JsonValue> mEffective;
	std::vector<std::vector<ModSettingsError>> mErrors;
	std::vector<std::string> mRulesErrors;
};

void ModManager::BuildDesiredView(DesiredView* theView)
{
	EnsureDesired();
	*theView = DesiredView();
	for (const ModInfo& d : mDesired)
	{
		ModSettingValues aValues = d.mValues;
		for (const auto& kv : d.mNextValues)
			aValues[kv.first] = kv.second;
		ModData aData;
		JsonValue anEffective;
		std::vector<ModSettingsError> anErrors;
		BuildEffective(d, aValues, &aData, &anEffective, &anErrors);
		theView->mValues.push_back(aValues);
		theView->mEffective.push_back(anEffective);
		theView->mErrors.push_back(anErrors);
		theView->mRulesErrors.push_back(aData.mRulesError);
	}
}

// The mods the gameplay files (data/diff.txt) resolve into with theMods enabled, as VFS::Resolve finds them
std::set<std::string> ModManager::GameplayOwners(const std::vector<const ModInfo*>& theMods,
	const std::vector<const JsonValue*>& theEffective) const
{
	VFS aVfs;
	aVfs.SetBaseDir(VFS::Get().GetBaseDir());
	std::vector<VFSRoot> aRoots;
	for (const ModInfo* m : theMods)
		aRoots.push_back({m->mPath, m->mId});
	aVfs.SetModRoots(aRoots);
	aVfs.SetRemaps(BuildRemaps(theMods, theEffective, false));
	std::set<std::string> anOwners;
	for (const std::string& aFile : mGameplayFiles)
	{
		std::string anOwner;
		aVfs.Resolve(aFile, &anOwner);
		if (!anOwner.empty())
			anOwners.insert(anOwner);
	}
	return anOwners;
}

// Would mDesired[theIndex] count for the score key with the view's values?
bool ModManager::DesiredAffectsScores(const DesiredView& theView, size_t theIndex, const std::set<std::string>& theGameplayMods) const
{
	const ModInfo& d = mDesired[theIndex];
	const JsonValue& man = theView.mEffective[theIndex];
	const std::vector<ModSettingsError>& anErrors = theView.mErrors[theIndex];
	return man["affectsScores"].AsBool(false) || man.Has("tunables") || man.Has("rules") || HasError(anErrors, "tunables") ||
		HasError(anErrors, "rules") || (theGameplayMods.count(d.mId) != 0) || !d.mSettings.ScoreKeyPairs(theView.mValues[theIndex]).empty();
}

std::string ModManager::GetNextScoreKey()
{
	DesiredView aView;
	BuildDesiredView(&aView);
	std::vector<const ModInfo*> anEnabled;
	std::vector<const JsonValue*> anEffective;
	for (size_t i = 0; i < mDesired.size(); i++)
	{
		if (!mDesired[i].mEnabled)
			continue;
		anEnabled.push_back(&mDesired[i]);
		anEffective.push_back(&aView.mEffective[i]);
	}
	std::set<std::string> aGameplayMods = GameplayOwners(anEnabled, anEffective);
	std::vector<std::string> anEntries;
	for (size_t i = 0; i < mDesired.size(); i++)
	{
		const ModInfo& d = mDesired[i];
		if (!d.mEnabled || !DesiredAffectsScores(aView, i, aGameplayMods))
			continue;
		std::string aPairs = d.mSettings.ScoreKeyPairs(aView.mValues[i]);
		anEntries.push_back(d.mId + "@" + d.mVersion + (aPairs.empty() ? "" : "?" + aPairs));
	}
	return HashKey(anEntries);
}

static JsonValue StringArray(const std::vector<std::string>& theTexts)
{
	JsonValue anArray = JsonValue::MakeArray();
	for (const std::string& aText : theTexts)
		anArray.Push(JsonValue(aText));
	return anArray;
}

std::string ModManager::GetStateJson()
{
	EnsureDesired();
	ModPending aPending = GetPending();
	DesiredView aView;
	BuildDesiredView(&aView);
	JsonValue aState = JsonValue::MakeObject();
	aState.Set("running", JsonValue(mGameRunning));
	aState.Set("nomods", JsonValue(!mApplied));
	aState.Set("scoreKey", JsonValue(mScoreKey));
	aState.Set("nextScoreKey", JsonValue(GetNextScoreKey()));
	aState.Set("settingsFile", JsonValue(mSettingsState));
	JsonValue aPendingJson = JsonValue::MakeObject();
	aPendingJson.Set("restart", StringArray(aPending.mRestart));
	aPendingJson.Set("nextGame", StringArray(aPending.mNextGame));
	aState.Set("pending", aPendingJson);

	JsonValue aMods = JsonValue::MakeArray();
	for (size_t i = 0; i < mDesired.size(); i++)
	{
		const ModInfo& d = mDesired[i];
		const ModInfo* a = d.mDuplicate ? nullptr : FindApplied(d.mId);
		bool isApplied = mApplied && (a != nullptr) && a->mEnabled;
		const ModSettingValues& aValues = aView.mValues[i];

		JsonValue aMod = JsonValue::MakeObject();
		aMod.Set("id", JsonValue(d.mId));
		aMod.Set("dir", JsonValue(d.mDir));
		aMod.Set("name", JsonValue(d.mName));
		aMod.Set("version", JsonValue(d.mVersion));
		aMod.Set("author", JsonValue(d.mAuthor));
		aMod.Set("description", JsonValue(d.mDescription));
		aMod.Set("priority", JsonValue((double) d.mPriority));
		aMod.Set("enabled", JsonValue(d.mEnabled));
		aMod.Set("applied", JsonValue(isApplied));

		std::vector<std::string> anErrors, aWarnings = d.mSettings.mWarnings;
		if (!d.mError.empty())
			anErrors.push_back(d.mError);
		for (const std::string& anError : d.mSettings.mErrors)
			anErrors.push_back(anError);
		for (const ModSettingsError& anError : aView.mErrors[i])
		{
			std::string aText = anError.mKey + ": " + anError.mText;
			if (std::find(anErrors.begin(), anErrors.end(), aText) == anErrors.end())
				anErrors.push_back(aText);
		}
		if (!aView.mRulesErrors[i].empty())
			anErrors.push_back(aView.mRulesErrors[i]);
		for (const std::string& aWarning : d.mValueWarnings)
			aWarnings.push_back(aWarning);
		if (mDataCheck && d.HasSettings() && !d.mDuplicate)
			mDataCheck(d, aView.mEffective[i], &aWarnings);

		// The mod's pending change: a restart for its enabled state; else the latest level of its settings'
		const char* aModPending = nullptr;
		if (mApplied && !d.mDuplicate && (d.mEnabled != isApplied))
			aModPending = "restart";
		JsonValue aSettings = JsonValue::MakeArray();
		for (const ModSettingDef& aDef : d.mSettings.mDefs)
		{
			JsonValue aSetting = ModSettingSchemaJson(aDef);
			if (aDef.IsValue())
			{
				JsonValue aValue = DesiredValue(d, aDef);
				aSetting.Set("value", aValue);
				const ModSettingDef* anAppliedDef = isApplied ? a->mSettings.Find(aDef.mId) : nullptr;
				JsonValue anAppliedValue = anAppliedDef ? a->mSettings.GetValue(a->mValues, *anAppliedDef) : JsonValue();
				aSetting.Set("applied", anAppliedValue);
				aSetting.Set("enabled", JsonValue(d.mSettings.IsActive(aValues, aDef)));
				const char* aSettingPending = nullptr;
				if (isApplied && (aDef.mLevel != MOD_LEVEL_LIVE) &&
					((anAppliedDef == nullptr) || !ModSettingSame(aDef, aValue, anAppliedValue)))
					aSettingPending = ModSettingLevelName(aDef.mLevel);
				aSetting.Set("pending", aSettingPending ? JsonValue(std::string(aSettingPending)) : JsonValue());
				if (aSettingPending && ((aModPending == nullptr) || (aDef.mLevel == MOD_LEVEL_RESTART)))
					aModPending = aSettingPending;
			}
			aSettings.Push(aSetting);
		}
		aMod.Set("pending", aModPending ? JsonValue(std::string(aModPending)) : JsonValue());

		// "scores": the mod counts for the score key with these values when it is enabled (the gameplay
		// files resolve among the enabled mods and it); "features": it has port features
		std::vector<const ModInfo*> aWith;
		std::vector<const JsonValue*> anEffective;
		for (size_t j = 0; j < mDesired.size(); j++)
		{
			if ((j == i) || (mDesired[j].mEnabled && !mDesired[j].mDuplicate))
			{
				aWith.push_back(&mDesired[j]);
				anEffective.push_back(&aView.mEffective[j]);
			}
		}
		aMod.Set("scores", JsonValue(DesiredAffectsScores(aView, i, GameplayOwners(aWith, anEffective))));
		aMod.Set("features", JsonValue(aView.mEffective[i].Has("features")));
		aMod.Set("hasSettings", JsonValue(d.mSettings.HasValues()));
		aMod.Set("errors", StringArray(anErrors));
		aMod.Set("warnings", StringArray(aWarnings));
		aMod.Set("settings", aSettings);
		aMods.Push(aMod);
	}
	aState.Set("mods", aMods);
	return aState.Dump(2);
}

}
