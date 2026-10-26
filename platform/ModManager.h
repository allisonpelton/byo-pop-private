#ifndef __PLATFORM_MODMANAGER_H__
#define __PLATFORM_MODMANAGER_H__

// Mod discovery, load order and data merging.
//
// A mod is a folder under the mods directory containing an optional mod.json and any files that
// replace or extend game data (same relative paths as the game: images/, sounds/, music/, data/,
// properties/). Example mod.json:
//
//   {
//     "id": "hd-gems", "name": "HD Gems", "version": "1.0", "author": "you",
//     "description": "Sharper gem sprites",
//     "priority": 0,                       // higher loads on top; ties keep the user's order
//     "defaultEnabled": true,              // false: off until the player turns it on (mods.json "enabled")
//     "imageScale": 2,                    // every plain-named image in this mod is 2x (optional)
//     "images": { "images/gemsheet6": { "scale": 2 } },   // per-image scale (optional)
//     "imagePatches": [ { "target": "images/gemsheet6", "image": "patches/gem@2x.png",
//                         "x": 0, "y": 520 } ],           // paste a region over a game image
//     "files": { "music/1984-network.s3m": { "file": "music/other.ogg", "start": 0, "loopStart": 2.5 },
//                "sounds/select.ogg": "sfx/click.ogg" },   // redirect game paths to mod files
//     "strings": { "EXCELLENT": "AWESOME!" },             // or ["properties/strings.xml"]
//     "tunables": { "scoring": { "hintPenalty": 25 } },   // or "tunables.json"
//     "rules": { "moves": { "allowInvalid": true } },     // code-mod gameplay rules (or "rules.json"),
//                                                         // a superset of "tunables" (game/GameRules.h)
//     "features": { "replays": true },                    // port features without gameplay effect
//     "imageOps": [ { "target": "images/gemsheet6", "op": "extendRows", ... } ],  // generated art
//     "affectsScores": false,              // forced true when the mod changes tunables or rules
//     "settings": [                        // values the player sets in the mod menu (F2)
//       { "type": "section", "label": "Play" },
//       { "id": "penalty", "label": "Hint cost", "type": "int", "default": 50, "min": 0, "max": 100,
//         "step": 5, "help": "..." },
//       { "id": "shape", "label": "Board", "type": "choice", "default": "full",
//         "choices": [ { "value": "full", "label": "8 x 8" }, { "value": "diamond", "label": "Diamond" } ] }
//     ]
//   }
//
// Settings (platform/ModSettings.h; for mod authors docs/MOD_MENU.md): each has an "id", a "type" (bool, int,
// float, choice, text, color, or section for a heading), a "label" and a "default", and by type "min", "max",
// "step", "unit", "decimals", "choices", "maxLength", "charset" or "alpha"; any may have "help", "apply",
// "affectsScores" and "enabledIf". At most 64. A value reaches the data through a placeholder that stands
// where a value would, in "tunables", "rules" and "features" (and the files they name), in the members of
// "files", "images" and "strings", in "imageScale", or in the elements of "imagePatches" and "imageOps":
//   "tunables": { "scoring": { "hintPenalty": { "$setting": "penalty" } } },
//   "rules": { "board": { "$setting": "shape", "map": { "full": { "$omit": true }, "diamond": { "mask": [...] } } } }
// ("map": an entry per value, "*" for the rest; "as": "int", "string", "rgb", "rgba" or "argb"; {"$omit": true}
// removes the member or element). Apply() reads every mod through its effective manifest: mod.json with the
// files "tunables" and "rules" name read in and every placeholder replaced by its value. A mod without
// "settings" is its own effective manifest. Where a value is used decides when a change applies (at once, at
// the next game, at a restart) and whether it splits the high scores.
//
// A mod affects scores (and so gets the enabled set its own high-score table and save) when it says so,
// has "tunables" or "rules", when data/diff.txt resolves into it (a file in the mod, or a "files"
// remap), or when a score-affecting setting is not at its default; its score key entry is then
// "id@version", plus "?sid=value&..." for those settings. The merged rules, their RulesId and any warnings
// are written to <user dir>/mods-status.json (GameConfig::Load) when an enabled mod has "rules" or "features".
//
// The enabled set and order are stored in <user dir>/mods.json:
//   { "order": ["hd-gems", "other"], "disabled": ["other"], "enabled": ["off-at-first"] }
// Mods found on disk but not listed are appended, and enabled unless their mod.json says "defaultEnabled":
// false. Only choices that differ from a mod's default are listed: "disabled" holds the mods that start on and
// were turned off, "enabled" those that start off and were turned on ("disabled" wins if both list a mod).
// Setting values are stored in
// <user dir>/mod-settings.json, only those that differ from their defaults:
//   { "v": 1, "mods": { "hd-gems": { "penalty": 25 } }, "next": { ... } }
// ("next": values held until the next fresh game, in games that apply values then; elsewhere they count as
// set). Both files are written only when the player changes something, by re-reading them, changing one thing
// and keeping every key this code does not know (a UTF-8 byte order mark in front is skipped).
//
// Two states: the applied one (GetMods, the tunables, strings, score key, VFS roots: what this process runs
// with, set by Refresh at start) and the desired one (GetDesiredMods: what the files say now, which the mod
// menu shows and edits). Edits change only the desired state and the files; they apply at a restart, at
// the next game (CommitNextGameValues) or at once (values only game code reads, GetLiveValue). The mod menu
// (framework/ModMenu.h), the web page's Mods panel (platform/WebApi.cpp, web/modpanel.js), the script command
// "modmenu" and -dumpmods (GetStateJson) are all views of this one model.

#include "Json.h"
#include "ModSettings.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Platform
{

struct ModInfo
{
	std::string mId;
	std::string mName;
	std::string mVersion;
	std::string mAuthor;
	std::string mDescription;
	std::string mPath;  // folder on the host FS
	std::string mDir;   // the folder's name
	int mPriority = 0;
	bool mEnabled = true;
	bool mDefaultEnabled = true;  // mod.json "defaultEnabled": false keeps it off until the player turns it on
	bool mAffectsScores = false;
	bool mHasManifest = false;
	bool mDuplicate = false;  // another folder (earlier by name) has the same id: never loaded
	std::string mError;  // manifest parse error, if any
	JsonValue mManifest;  // mod.json as written

	// The "settings" block and the values: in the applied state what this process runs with, in the
	// desired state the saved ones ("mods" of mod-settings.json); mNextValues only in the desired state:
	// values held for the next fresh game ("next")
	ModSettings mSettings;
	ModSettingValues mValues;
	ModSettingValues mNextValues;
	std::vector<std::string> mValueWarnings;  // stored values that could not be used as they were

	// port(variant): set by Apply() for enabled mods
	JsonValue mEffective;         // mod.json with the files "tunables" and "rules" name read in and the
	                              // settings' values put in (the manifest itself for a mod without settings)
	std::vector<ModSettingsError> mSettingErrors;  // keys of mEffective left out for wrong placeholders
	std::string mScorePairs;      // the "sid=value&..." of its score key entry ("" at the defaults)
	bool mHasRules = false;       // "rules" in mod.json
	bool mHasFeatures = false;    // "features"
	bool mHasImageOps = false;    // "imageOps"
	bool mOverridesDiff = false;  // data/diff.txt resolves into this mod
	JsonValue mTunablesJson;      // "tunables", read from its file when it names one
	JsonValue mRulesJson;         // "rules", likewise
	std::string mRulesError;      // why the file "rules" names could not be read (then reported as the mod's error)
	JsonValue mFeaturesJson;      // "features"

	// Counts for the score key: affectsScores, tunables, rules, data/diff.txt or a setting off its default
	bool AffectsScores() const { return mAffectsScores || mOverridesDiff || !mScorePairs.empty(); }
	bool HasSettings() const { return mSettings.mPresent; }
};

// port(variant): one entry of a mod's "imageOps" (applied from stage 8 of
// games/bejeweled/docs/CODE_MODS_DESIGN.md on)
struct ModImageOp
{
	std::string mModId;
	JsonValue mOp;
};

// A game's check of a mod's data as the desired values make it (its effective manifest), e.g. rule values
// against the rules' own ranges: messages the menu shows before the values apply
typedef std::function<void(const ModInfo& theMod, const JsonValue& theEffective, std::vector<std::string>* theMessages)> ModDataCheck;

// What the desired state changes against the applied one, as the menu lists it
struct ModPending
{
	std::vector<std::string> mRestart;   // "HD Gems turned off", "Load order", setting labels
	std::vector<std::string> mNextGame;  // setting labels
};

class ModManager
{
public:
	static ModManager& Get();

	void SetModsDir(const std::string& theDir) { mModsDir = theDir; }
	const std::string& GetModsDir() const { return mModsDir; }

	// Rescans the mods directory and the saved configuration, then applies the result.
	void Refresh();

	const std::vector<ModInfo>& GetMods() const { return mMods; }  // in load order, top priority first
	std::vector<const ModInfo*> GetEnabledMods() const;

	// Pushes the enabled mods into the VFS overlay, image scale overrides, tunables and strings.
	void Apply();

	// Tunables: dotted keys ("scoring.hintPenalty"); higher-priority mods override lower ones.
	bool HasTunable(const std::string& theKey) const;
	double GetTunable(const std::string& theKey, double theDefault) const;
	int GetTunableInt(const std::string& theKey, int theDefault) const;
	bool GetTunableBool(const std::string& theKey, bool theDefault) const;
	std::vector<double> GetTunableArray(const std::string& theKey, const std::vector<double>& theDefault) const;
	void SetTunableForTesting(const std::string& theKey, const JsonValue& theValue) { mTunables[theKey] = theValue; }

	// Inline string overrides (property id -> text) and extra property files (host paths), lowest
	// priority first. SexyAppBase::LoadProperties() applies both after properties/default.xml.
	const std::map<std::string, std::string>& GetStringOverrides() const { return mStrings; }
	const std::vector<std::string>& GetPropertyFiles() const { return mPropertyFiles; }

	// Non-empty when enabled mods change gameplay; used to keep separate high-score tables.
	const std::string& GetScoreKey() const { return mScoreKey; }

	// port(variant): the enabled mods' "imageOps", lowest priority first, each with its mod
	const std::vector<ModImageOp>& GetImageOps() const { return mImageOps; }

	// port(variant): an enabled mod has "rules" or "features" (then mods-status.json is written)
	bool HasRulesOrFeatures() const;

	// port(variant): writes <user dir>/mods-status.json, or removes it when theText is empty. Unchanged
	// text is not written again.
	void WriteStatus(const std::string& theText);

	// "Mods: a, b" style summary for the title screen / logs.
	std::string GetSummary() const;

	// --- Game hooks (docs/MOD_MENU_DESIGN.md 2.2, 3.5) ---

	// Which data paths a game re-reads at every new game ("rules.match.minRun"); settings used only there
	// apply at the next game. Without a filter (the default) every data value needs a restart, and values held
	// in mod-settings.json's "next" count as committed. Set it before Refresh: the start applies values by it.
	void SetNextGameFilter(const NextGameFilter& theFilter);
	// Game paths whose replacement changes play (default: data/diff.txt): a mod they resolve into, and a
	// setting in their "files" remap, count for the score key. Best set before Refresh; set after it, the
	// applied mods' owners and the score key are worked out again.
	void SetGameplayFiles(const std::vector<std::string>& theFiles);
	const std::vector<std::string>& GetGameplayFiles() const { return mGameplayFiles; }
	// The game's check of the desired data (ModDataCheck); its messages are warnings in GetStateJson
	void SetDataCheck(const ModDataCheck& theCheck) { mDataCheck = theCheck; }

	// Values held for the next fresh game (mod-settings.json "next"), and their commit: the game calls it as
	// it starts a game, with theCanCommit false while a game would be stranded (one in progress, or a save
	// under the current key). A commit moves them into "mods", re-resolves "tunables", "rules" and
	// "features" and the score key. Returns true if anything was committed.
	bool HasNextGameValues() const;
	bool CommitNextGameValues(bool theCanCommit);

	// A setting's value as this process uses it (the default if unknown; null for an unknown setting).
	// Values that no data uses are "live": an edit changes them at once.
	JsonValue GetLiveValue(const std::string& theModId, const std::string& theSettingId) const;

	// --- The desired state: the mod menu's model ---

	// Rescans the mod folders, mods.json and mod-settings.json without applying anything.
	void ScanDesired();
	const std::vector<ModInfo>& GetDesiredMods();  // in load order; scans once if needed
	const ModInfo* FindDesired(const std::string& theId);
	bool CanMove(const std::string& theId, int theDelta);  // a neighbour of the same priority

	// Edits, each written at once (re-read, one change, unknown keys kept, atomic). They return "" or why
	// nothing was changed.
	std::string EditEnable(const std::string& theId, bool theEnabled);
	std::string EditMove(const std::string& theId, int theDelta);
	std::string EditSetting(const std::string& theId, const std::string& theSettingId, const JsonValue& theValue);
	std::string EditReset(const std::string& theId);  // every setting back to its default
	// A mod whose folders are all gone (the web page's Remove): its id leaves "order" and "disabled" of mods.json,
	// so the same mod added again later starts out enabled at the end of the order. Its values in
	// mod-settings.json stay. Refused while a folder with that id is installed.
	std::string EditForget(const std::string& theId);
	// {"op": "enable"|"move"|"set"|"reset"|"forget", "mod": id, "on": bool, "delta": -1|1, "setting": id, "value": v}
	std::string Edit(const std::string& theJson);

	ModPending GetPending();
	// The score keys of the desired state. GetRestartScoreKey: what a start now would apply (the enabled mods
	// and order, the committed values), so where a restart files the game in progress. GetNextScoreKey: the key
	// of the next fresh game, with the values held for it as well (they never apply to a game in progress).
	std::string GetRestartScoreKey();
	std::string GetNextScoreKey();
	// The whole model as JSON (-dumpmods, the web panel, the script command "modmenu dump")
	std::string GetStateJson();

	// mod-settings.json as last read: "missing", "ok", "unreadable" (values at their defaults; the next
	// write moves it to mod-settings.bad.json) or "newer" (made by a newer version: read only)
	const std::string& GetSettingsFileState() const { return mSettingsState; }

	// Grows with every edit or rescan; the open menu redraws when it changes. theCallback runs after each.
	unsigned GetChangeSerial() const { return mChangeSerial; }
	void SetChangeCallback(void (*theCallback)()) { mChangeCallback = theCallback; }

	// Refresh ran (false with -nomods); the game is running (its main loop)
	bool IsApplied() const { return mApplied; }
	void SetGameRunning(bool isRunning) { mGameRunning = isRunning; }

private:
	struct ModData;
	struct SettingsFile;
	struct DesiredView;

	void LoadManifest(ModInfo& theMod, bool theLog);
	void ScanInto(std::vector<ModInfo>* theMods, bool theLog);
	void Reanalyze(std::vector<ModInfo>* theMods);
	void SortByConfig(std::vector<ModInfo>* theMods, const JsonValue& theConfig);
	void ReadModData(const ModInfo& theMod, ModData* theData) const;
	void BuildEffective(const ModInfo& theMod, const ModSettingValues& theValues, ModData* theData, JsonValue* theEffective,
		std::vector<ModSettingsError>* theErrors) const;
	void SetRuleData(ModInfo& theMod, const ModData& theData, bool theLog);
	void FlattenInto(const JsonValue& theValue, const std::string& thePrefix);
	void RebuildTunables();
	void ComputeScoreKey();
	friend void SetPlaceholders(bool on);  // -placeholders changes the score key (platform/Placeholders.h)
	void SetGameplayOwners();

	void ReadSettingsFile(SettingsFile* theFile) const;
	std::string WriteSettingsFile(const SettingsFile& theFile, const JsonValue& theRoot);
	void SetValuesFromRoot(std::vector<ModInfo>* theMods, const JsonValue& theRoot, bool theNext);
	bool ReadConfig(JsonValue* theConfig, bool* theIsBroken) const;
	std::string WriteConfig(const JsonValue& theOld, const JsonValue& theConfig, bool theIsBroken);
	ModInfo* FindDesiredMutable(const std::string& theId);
	ModInfo* FindApplied(const std::string& theId);
	const ModInfo* FindApplied(const std::string& theId) const;
	JsonValue DesiredValue(const ModInfo& theMod, const ModSettingDef& theDef) const;
	void BuildDesiredView(DesiredView* theView, bool theWithHeld);
	std::string DesiredScoreKey(bool theWithHeld);
	std::set<std::string> GameplayOwners(const std::vector<const ModInfo*>& theMods, const std::vector<const JsonValue*>& theEffective) const;
	bool DesiredAffectsScores(const DesiredView& theView, size_t theIndex, const std::set<std::string>& theGameplayMods) const;
	void EnsureDesired();
	void Changed();

	std::string mModsDir = "mods";
	std::vector<ModInfo> mMods;
	std::map<std::string, JsonValue> mTunables;
	std::map<std::string, std::string> mStrings;
	std::vector<std::string> mPropertyFiles;
	std::string mScoreKey;
	std::vector<ModImageOp> mImageOps;
	std::string mStatusWritten;

	NextGameFilter mNextGameFilter;
	ModDataCheck mDataCheck;
	std::vector<std::string> mGameplayFiles = {"data/diff.txt"};
	std::vector<ModInfo> mDesired;
	bool mDesiredValid = false;
	std::string mSettingsState = "missing";
	unsigned mChangeSerial = 0;
	void (*mChangeCallback)() = nullptr;
	bool mApplied = false;
	bool mGameRunning = false;
};

}

#endif
