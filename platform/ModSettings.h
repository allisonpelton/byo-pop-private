#ifndef __PLATFORM_MODSETTINGS_H__
#define __PLATFORM_MODSETTINGS_H__

// Configurable values of a mod: the "settings" block of mod.json and the placeholders that put the values
// into the mod's own data (docs/MOD_MENU_DESIGN.md 3; for mod authors docs/MOD_MENU.md). SDL-free.
//
//   "settings": [
//     { "type": "section", "label": "Play" },
//     { "id": "minRun", "label": "Shortest match", "type": "int", "default": 3, "min": 2, "max": 5 },
//     { "id": "shape", "label": "Board", "type": "choice", "default": "full",
//       "choices": [ { "value": "full", "label": "8 x 8" }, { "value": "diamond", "label": "Diamond" } ] } ],
//   "rules": { "match": { "minRun": { "$setting": "minRun" } },
//              "board": { "$setting": "shape", "map": { "full": { "$omit": true }, "diamond": { ... } } } }
//
// Types: bool, int, float, choice, text, color, and section (a heading). A placeholder
// {"$setting": id, "map": {...}, "as": ...} replaces itself with the value, with the map entry for the value's
// canonical text (or "*"), or with the value converted ("as": "int", "string", "rgb", "rgba", "argb"). A
// result of {"$omit": true} removes the member or element; null keeps its meaning in the data (under "rules"
// a reset, under "features" off) and removes a member of "files", "images" and "strings" and an element of
// "imagePatches" and "imageOps". Placeholders may sit in "tunables", "rules" and "features" (also in the files
// "tunables" and "rules" name), in the members of "files", "images" and "strings", in "imageScale" and in the
// elements of "imagePatches" and "imageOps". A top-level key with a wrong placeholder is left out of the
// effective manifest.
//
// Where a value is used decides when it applies (its level) and whether it splits the high scores:
//   rules, tunables, features             next game (when the game re-reads them, NextGameFilter), else restart
//   files, images, imageScale, imagePatches, imageOps, strings      restart
//   nowhere (the game asks ModManager::GetLiveValue)                 live, or the author's "apply"
// A value under "tunables" or "rules", in the "files" remap of a gameplay file (data/diff.txt), or of a setting
// with "affectsScores": true is score-affecting: when it is not at its default, the mod's score key entry
// carries it.

#include "Json.h"

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Platform
{

enum ModSettingType
{
	MOD_SETTING_SECTION,
	MOD_SETTING_BOOL,
	MOD_SETTING_INT,
	MOD_SETTING_FLOAT,
	MOD_SETTING_CHOICE,
	MOD_SETTING_TEXT,
	MOD_SETTING_COLOR
};

// When a changed value takes effect; a later level is a "larger" one
enum ModSettingLevel
{
	MOD_LEVEL_LIVE,
	MOD_LEVEL_NEXT_GAME,
	MOD_LEVEL_RESTART
};

enum ModSettingCharset
{
	MOD_CHARSET_ANY,     // printable Latin-1
	MOD_CHARSET_UPPER,   // printable Latin-1 without lower case (letters are made capitals)
	MOD_CHARSET_DIGITS,  // 0-9
	MOD_CHARSET_NAME     // A-Z, a-z, 0-9, space, '-', '_'
};

// What Normalize made of a value
enum ModSettingNormal
{
	MOD_VALUE_OK,        // valid as it was
	MOD_VALUE_ADJUSTED,  // clamped, snapped to a step, cut or with characters replaced
	MOD_VALUE_WRONG      // not usable (wrong type, unknown choice, bad colour): the default instead
};

// Can a changed value at this data path ("rules.match.minRun", "tunables.scoring.hintPenalty",
// "features.replays") take effect at the next game? A game that re-reads its rules at every new game supplies
// one (ModManager::SetNextGameFilter); without one every data value needs a restart.
typedef std::function<bool(const std::string& thePath)> NextGameFilter;

struct ModSettingChoice
{
	JsonValue mValue;         // a string id, an integer or a bool
	std::string mLabel;
	std::string mCanonical;   // its text: score key and map lookups
};

struct ModSettingDef
{
	std::string mId;          // empty for a section
	int mType = MOD_SETTING_SECTION;
	std::string mLabel;
	std::string mHelp;
	std::string mUnit;
	JsonValue mDefault;
	// int: the values; float: units of 10^-mScale (a value is units / 10^mScale, exact like its JSON text)
	int64_t mMin = 0;
	int64_t mMax = 0;
	int64_t mStep = 1;
	int mScale = 0;           // float: decimals of the units
	int mDecimals = 0;        // float: decimals shown
	std::vector<ModSettingChoice> mChoices;
	int mMaxLength = 24;      // text: characters (Latin-1)
	int mCharset = MOD_CHARSET_ANY;
	bool mAlpha = false;      // color: #rrggbbaa
	int mApply = -1;          // "apply": the author's level, -1 without
	bool mAffectsScores = false;  // "affectsScores": true
	std::string mIfSetting;   // "enabledIf": the setting it depends on (empty without)
	std::string mIfCanonical; // ... and the canonical text of the value it is compared with
	bool mIfEquals = true;    // "equals" (or "notEquals")

	// Derived from the data (ModSettings::Analyze)
	int mLevel = MOD_LEVEL_LIVE;
	bool mScores = false;     // score-affecting
	bool mUsed = false;       // a placeholder names it
	bool mNoPercent = false;  // text that lands in "strings" ('%' would reach printf formats)

	bool IsValue() const { return mType != MOD_SETTING_SECTION; }
};

typedef std::map<std::string, JsonValue> ModSettingValues;

// An error of the effective manifest: the top-level key it left out and why
struct ModSettingsError
{
	std::string mKey;
	std::string mText;
};

class ModSettings
{
public:
	bool mPresent = false;                // mod.json has "settings"
	std::vector<ModSettingDef> mDefs;     // in order, sections included
	std::vector<std::string> mErrors;     // the schema's and the placeholders' errors
	std::vector<std::string> mWarnings;
	std::set<std::string> mBadKeys;       // top-level keys with wrong placeholders (Analyze)
	size_t mSchemaErrors = 0;             // how many of mErrors and mWarnings Parse found (Analyze adds its own)
	size_t mSchemaWarnings = 0;

	// Reads theManifest["settings"]; a setting with an error is left out.
	void Parse(const JsonValue& theManifest);

	// Finds the placeholders in theData (mod.json with the files "tunables" and "rules" name read in) and
	// derives each setting's level and whether it affects scores. Run again when the filter changes.
	void Analyze(const JsonValue& theData, const NextGameFilter& theFilter, const std::vector<std::string>& theGameplayFiles);

	const ModSettingDef* Find(const std::string& theId) const;
	ModSettingDef* FindForAnalysis(const std::string& theId);
	bool HasValues() const;  // at least one setting that holds a value

	// A value for theDef made from theStored (the default when it is unusable); theWarning says what was wrong
	int Normalize(const ModSettingDef& theDef, const JsonValue& theStored, JsonValue* theValue, std::string* theWarning) const;
	// The values of a stored object ({"minRun": 2, ...}); with theFillDefaults every setting gets one
	void ReadValues(const JsonValue& theStored, bool theFillDefaults, ModSettingValues* theValues,
		std::vector<std::string>* theWarnings) const;
	JsonValue GetValue(const ModSettingValues& theValues, const ModSettingDef& theDef) const;  // the default if missing
	bool IsActive(const ModSettingValues& theValues, const ModSettingDef& theDef) const;      // its enabledIf holds
	JsonValue EffectiveValue(const ModSettingValues& theValues, const ModSettingDef& theDef) const;  // the default while inactive

	// The effective manifest: theData with every placeholder replaced by theValues. Keys with wrong
	// placeholders are left out and reported in theErrors. Without "settings" it is theData itself.
	void Resolve(const JsonValue& theData, const ModSettingValues& theValues, JsonValue* theEffective,
		std::vector<ModSettingsError>* theErrors) const;

	// "sid=value&sid=value" over the score-affecting settings whose effective value is not their default
	// (sorted by id), "" when all are at their defaults
	std::string ScoreKeyPairs(const ModSettingValues& theValues) const;
};

// The canonical text of a value: the score key and map lookups. bool true/false; int decimal; float the 16
// hex digits of its IEEE bits; choice its value's text; text its Latin-1 bytes with %&=?; and non-printable
// bytes percent-escaped; color the text.
std::string ModSettingCanonical(const ModSettingDef& theDef, const JsonValue& theValue);
// The text shown for a value (and "as": "string"): a float with its decimals, text as it is
std::string ModSettingText(const ModSettingDef& theDef, const JsonValue& theValue);
bool ModSettingSame(const ModSettingDef& theDef, const JsonValue& theA, const JsonValue& theB);
const char* ModSettingTypeName(int theType);
const char* ModSettingLevelName(int theLevel);
// The schema of one setting as JSON (the state JSON of ModManager::GetStateJson)
JsonValue ModSettingSchemaJson(const ModSettingDef& theDef);

}

#endif
