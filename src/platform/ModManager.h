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
//     "imageScale": 2,                     // every plain-named image in this mod is 2x (optional)
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
//     "affectsScores": false               // forced true when the mod changes tunables or rules
//   }
//
// A mod affects scores (and so gets the enabled set its own high-score table and save) when it says so,
// has "tunables" or "rules", or when data/diff.txt resolves into it (a file in the mod, or a "files"
// remap). The merged rules, their RulesId and any warnings are written to <user dir>/mods-status.json
// (GameConfig::Load) when an enabled mod has "rules" or "features".
//
// The enabled set and order are stored in <user dir>/mods.json:
//   { "order": ["hd-gems", "other"], "disabled": ["other"] }
// Mods found on disk but not listed are appended and enabled.

#include "Json.h"
#include <map>
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
	int mPriority = 0;
	bool mEnabled = true;
	bool mAffectsScores = false;
	bool mHasManifest = false;
	std::string mError;  // manifest parse error, if any
	JsonValue mManifest;

	// port(variant): set by Apply() for enabled mods
	bool mHasRules = false;       // "rules" in mod.json
	bool mHasFeatures = false;    // "features"
	bool mHasImageOps = false;    // "imageOps"
	bool mOverridesDiff = false;  // data/diff.txt resolves into this mod
	JsonValue mTunablesJson;      // "tunables", read from its file when it names one
	JsonValue mRulesJson;         // "rules", likewise
	std::string mRulesError;      // why the file "rules" names could not be read (then reported as the mod's error)
	JsonValue mFeaturesJson;      // "features"

	// Counts for the score key: affectsScores, tunables, rules or data/diff.txt
	bool AffectsScores() const { return mAffectsScores || mOverridesDiff; }
};

// port(variant): one entry of a mod's "imageOps" (applied from docs/CODE_MODS_DESIGN.md stage 8 on)
struct ModImageOp
{
	std::string mModId;
	JsonValue mOp;
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

	void SetEnabled(const std::string& theId, bool theEnabled);
	void Move(const std::string& theId, int theDelta);  // -1 = higher priority
	void SaveConfig();

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

private:
	void LoadManifest(ModInfo& theMod);
	void FlattenInto(const JsonValue& theValue, const std::string& thePrefix);

	std::string mModsDir = "mods";
	std::vector<ModInfo> mMods;
	std::map<std::string, JsonValue> mTunables;
	std::map<std::string, std::string> mStrings;
	std::vector<std::string> mPropertyFiles;
	std::string mScoreKey;
	std::vector<ModImageOp> mImageOps;
	std::string mStatusWritten;
};

}

#endif
