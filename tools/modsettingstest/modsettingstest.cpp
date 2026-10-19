// Unit tests of the mod settings (platform/ModSettings.*) and of the mod menu's model in ModManager
// (docs/MOD_MENU_DESIGN.md 3 and 4): the schema checks of 3.6, stored values, placeholders (map, as, null,
// $omit, nesting, errors), the levels of 3.5, score key entries (example-relaxed keeps 58ebd9d7), float values
// that are the same bits on native and wasm, the data/diff.txt owner as VFS::Resolve finds it, and the
// edits of mods.json and mod-settings.json (unknown keys kept, no write for a no-op, next-game commits); the mod
// menu's controls (ModSettingStep, the step indexes of its sliders, typed characters), the quoting of the
// arguments a native restart passes on (platform/Relaunch.h), the "forget" edit of a removed mod and, in the web
// build, the C functions the page's Mods panel calls (platform/WebApi.cpp).
//
//   modsettingstest [-v] [<temp dir>]   every check; prints a summary, exit status 0 when all pass
//
// Native: build/native/modsettingstest.exe (the temp dir defaults to modsettingstest-tmp in the current
// folder and is removed afterwards). Web: build/web/modsettingstest.js under Node
// (E:/retools/emsdk/node/<version>/bin/node.exe build/web/modsettingstest.js), in its in-memory file system.
// The fixture mod tools/fixtures/mods/settings-demo and games/bejeweled/mods/example-settings are read from the
// source tree (embedded on the web).

#include "platform/ModManager.h"
#include "platform/ModSettings.h"
#include "platform/Relaunch.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

using namespace Platform;

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

static void CheckEq(const std::string& theGot, const std::string& theWant, const std::string& theWhat)
{
	Check(theGot == theWant, theWhat + (theGot == theWant ? "" : " (got " + theGot + ", want " + theWant + ")"));
}

static JsonValue J(const std::string& theText)
{
	JsonValue aValue;
	std::string anError;
	if (!ParseJson(theText, aValue, &anError))
	{
		printf("test JSON does not parse: %s\n%s\n", anError.c_str(), theText.c_str());
		exit(2);
	}
	return aValue;
}

static bool Contains(const std::vector<std::string>& theTexts, const std::string& thePart)
{
	for (const std::string& aText : theTexts)
		if (aText.find(thePart) != std::string::npos)
			return true;
	return false;
}

static NextGameFilter AllNextGame()
{
	return [](const std::string&) { return true; };
}

// A schema from a manifest's text, analysed with theFilter
static ModSettings Schema(const std::string& theManifest, const NextGameFilter& theFilter = NextGameFilter())
{
	ModSettings aSettings;
	JsonValue aManifest = J(theManifest);
	aSettings.Parse(aManifest);
	aSettings.Analyze(aManifest, theFilter, std::vector<std::string>{"data/diff.txt"});
	return aSettings;
}

// A one-setting manifest: theSetting is the JSON of the setting, theData the rest of mod.json
static std::string OneSetting(const std::string& theSetting, const std::string& theData = "")
{
	return "{ \"settings\": [ " + theSetting + " ]" + (theData.empty() ? "" : ", " + theData) + " }";
}

static std::string Fnv(const std::vector<std::string>& theEntries)
{
	uint32_t h = 2166136261u;
	for (const std::string& s : theEntries)
		for (unsigned char c : s + ";")
			h = (h ^ c) * 16777619u;
	char aBuffer[16];
	snprintf(aBuffer, sizeof(aBuffer), "%08x", h);
	return aBuffer;
}

//////////////////////////////////////////////////////////////////////////
// Files

static std::string gTemp;

static void WriteFile(const std::string& thePath, const std::string& theText)
{
	size_t aSlash = thePath.find_last_of('/');
	if (aSlash != std::string::npos)
		MakeDirs(thePath.substr(0, aSlash));
	FILE* f = fopen(thePath.c_str(), "wb");
	if (f == nullptr)
	{
		printf("cannot write %s\n", thePath.c_str());
		exit(2);
	}
	fwrite(theText.data(), 1, theText.size(), f);
	fclose(f);
}

static std::string ReadFile(const std::string& thePath)
{
	std::string aText;
	if (!ReadHostFile(thePath, &aText))
		return "<missing>";
	return aText;
}

static void RemoveTree(const std::string& thePath)
{
	if (IsDirectory(thePath))
	{
		for (const std::string& aName : ListDirectory(thePath))
			RemoveTree(thePath + "/" + aName);
#ifdef _WIN32
		_rmdir(thePath.c_str());
#else
		rmdir(thePath.c_str());
#endif
	}
	else
		remove(thePath.c_str());
}

static int gUserDataChanges = 0;
static void OnUserDataChanged()
{
	gUserDataChanges++;
}

// A fresh mods dir, user dir and (empty) game dir for one ModManager test
struct Sandbox
{
	std::string mMods, mUser, mGame;

	explicit Sandbox(const std::string& theName)
	{
		std::string aRoot = gTemp + "/" + theName;
		RemoveTree(aRoot);
		mMods = aRoot + "/mods";
		mUser = aRoot + "/user";
		mGame = aRoot + "/game";
		MakeDirs(mMods);
		MakeDirs(mUser);
		MakeDirs(mGame);
		VFS::Get().SetBaseDir(mGame);
		VFS::Get().SetUserDir(mUser);
		ModManager::Get().SetModsDir(mMods);
		ModManager::Get().SetNextGameFilter(NextGameFilter());
		ModManager::Get().SetGameplayFiles(std::vector<std::string>{"data/diff.txt"});
	}
	void Mod(const std::string& theDir, const std::string& theManifest) const { WriteFile(mMods + "/" + theDir + "/mod.json", theManifest); }
	void User(const std::string& theName, const std::string& theText) const { WriteFile(mUser + "/" + theName, theText); }
	std::string UserText(const std::string& theName) const { return ReadFile(mUser + "/" + theName); }
	bool HasUser(const std::string& theName) const { return IsFile(mUser + "/" + theName); }
};

static const ModInfo* Applied(const std::string& theId)
{
	for (const ModInfo& m : ModManager::Get().GetMods())
		if (m.mId == theId)
			return &m;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// 3.6: the schema

static void TestSchema()
{
	ModSettings s = Schema("{ \"settings\": { \"id\": \"x\" } }");
	Check(s.mPresent && s.mDefs.empty() && Contains(s.mErrors, "must be an array"), "settings that is not an array: ignored, with an error");

	s = Schema("{ \"id\": \"m\" }");
	Check(!s.mPresent && s.mDefs.empty() && s.mErrors.empty(), "no settings block: nothing");

	struct Dropped
	{
		const char* mSetting;
		const char* mError;
	};
	const Dropped aDropped[] = {
		{"{ \"id\": \"1x\", \"type\": \"bool\", \"label\": \"L\", \"default\": true }", "\"id\" must be"},
		{"{ \"id\": \"a-b\", \"type\": \"bool\", \"label\": \"L\", \"default\": true }", "\"id\" must be"},
		{"{ \"id\": \"abcdefghijabcdefghijabcdefghijabc\", \"type\": \"bool\", \"label\": \"L\", \"default\": true }", "\"id\" must be"},
		{"{ \"id\": \"x\", \"type\": \"slider\", \"label\": \"L\", \"default\": 1 }", "\"type\" must be"},
		{"{ \"id\": \"x\", \"type\": \"bool\", \"default\": true }", "\"label\" is missing"},
		{"{ \"id\": \"x\", \"type\": \"bool\", \"label\": \"\", \"default\": true }", "\"label\" must be non-empty"},
		{"{ \"id\": \"x\", \"type\": \"bool\", \"label\": \"L\" }", "\"default\" is missing"},
		{"{ \"id\": \"x\", \"type\": \"bool\", \"label\": \"L\", \"default\": 1 }", "true or false"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 7, \"min\": 0, \"max\": 5 }", "\"default\" must be a whole number"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 3, \"min\": 0, \"max\": 10, \"step\": 2 }", "on a step"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 1.5 }", "\"default\" must be a whole number"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 3, \"min\": 5, \"max\": 1 }", "min <= max"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 3, \"step\": 0 }", "min <= max and a step above 0"},
		{"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 3, \"max\": 3000000000 }", "-2147483648 to 2147483647"},
		{"{ \"id\": \"x\", \"type\": \"float\", \"label\": \"L\", \"default\": 1, \"min\": 0, \"max\": 2 }", "a float needs"},
		{"{ \"id\": \"x\", \"type\": \"float\", \"label\": \"L\", \"default\": 1, \"min\": 0, \"max\": 2, \"step\": 0.00001 }", "more than 100000 steps"},
		{"{ \"id\": \"x\", \"type\": \"float\", \"label\": \"L\", \"default\": 0.30000000000000004, \"min\": 0, \"max\": 1, \"step\": 0.1 }",
			"at most 9 decimals"},
		{"{ \"id\": \"x\", \"type\": \"float\", \"label\": \"L\", \"default\": 0.25, \"min\": 0, \"max\": 1, \"step\": 0.1 }", "on a step"},
		{"{ \"id\": \"x\", \"type\": \"float\", \"label\": \"L\", \"default\": 1, \"min\": 0, \"max\": 2, \"step\": -0.5 }", "a step above 0"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": \"a\", \"choices\": [ { \"value\": \"a\", \"label\": \"A\" } ] }",
			"2 to 32 entries"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": \"a\", \"choices\": [ { \"value\": \"a\", \"label\": \"A\" }, "
			"{ \"value\": \"a\", \"label\": \"B\" } ] }", "listed twice"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": 1, \"choices\": [ { \"value\": 1, \"label\": \"A\" }, "
			"{ \"value\": \"1\", \"label\": \"B\" } ] }", "listed twice"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": true, \"choices\": [ { \"value\": true, \"label\": \"A\" }, "
			"{ \"value\": \"true\", \"label\": \"B\" } ] }", "listed twice"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": 1.5, \"choices\": [ { \"value\": 1.5, \"label\": \"A\" }, "
			"{ \"value\": 2, \"label\": \"B\" } ] }", "\"value\" must be an id"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": \"a&b=c\", \"choices\": [ { \"value\": \"a&b=c\", \"label\": \"A\" }, "
			"{ \"value\": \"d\", \"label\": \"B\" } ] }", "\"value\" must be an id"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": \"a\", \"choices\": [ { \"value\": \"a\" }, "
			"{ \"value\": \"b\", \"label\": \"B\" } ] }", "\"label\" is missing"},
		{"{ \"id\": \"x\", \"type\": \"choice\", \"label\": \"L\", \"default\": \"c\", \"choices\": [ { \"value\": \"a\", \"label\": \"A\" }, "
			"{ \"value\": \"b\", \"label\": \"B\" } ] }", "one of the choices"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"a\", \"maxLength\": 0 }", "1..64"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"a\", \"maxLength\": 65 }", "1..64"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"abcdef\", \"maxLength\": 5 }", "fits maxLength"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"Abc\", \"charset\": \"upper\" }", "fits maxLength and the charset"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"12a\", \"charset\": \"digits\" }", "the charset"},
		{"{ \"id\": \"x\", \"type\": \"text\", \"label\": \"L\", \"default\": \"a\", \"charset\": \"klingon\" }", "\"charset\" must be"},
		{"{ \"id\": \"x\", \"type\": \"color\", \"label\": \"L\", \"default\": \"#12345\" }", "a colour #rrggbb"},
		{"{ \"id\": \"x\", \"type\": \"color\", \"label\": \"L\", \"default\": \"#11223344\" }", "a colour #rrggbb"},
	};
	for (const Dropped& aCase : aDropped)
	{
		s = Schema(OneSetting(aCase.mSetting));
		Check(!s.HasValues() && Contains(s.mErrors, aCase.mError),
			std::string("dropped with an error \"") + aCase.mError + "\": " + aCase.mSetting + (s.mErrors.empty() ? "" : " -> " + s.mErrors[0]));
	}

	s = Schema("{ \"settings\": [ { \"id\": \"x\", \"type\": \"bool\", \"label\": \"L\", \"default\": true }, "
		"{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 1 } ] }");
	Check((s.Find("x") != nullptr) && (s.Find("x")->mType == MOD_SETTING_BOOL) && Contains(s.mErrors, "used twice"), "a second setting with the same id is dropped");

	std::string aMany = "{ \"settings\": [ { \"type\": \"section\", \"label\": \"S\" }";
	for (int i = 0; i < 70; i++)
		aMany += ", { \"id\": \"s" + std::to_string(i) + "\", \"type\": \"bool\", \"label\": \"L\", \"default\": false }";
	s = Schema(aMany + " ] }");
	Check((s.mDefs.size() == 65) && (s.Find("s63") != nullptr) && (s.Find("s64") == nullptr) && Contains(s.mErrors, "at most 64 settings"),
		"at most 64 settings (sections do not count)");

	// Accepted with adjustments
	s = Schema(OneSetting("{ \"id\": \"x\", \"type\": \"bool\", \"label\": \"An extremely long label that goes on and on and on\", \"default\": true }"));
	Check(s.Find("x") && (s.Find("x")->mLabel.size() == 40) && Contains(s.mWarnings, "cut to 40"), "a long label is cut to 40 characters");
	s = Schema(OneSetting("{ \"id\": \"c\", \"type\": \"color\", \"label\": \"L\", \"default\": \"#FF8800\", \"alpha\": true }"));
	Check(s.Find("c") && (s.Find("c")->mDefault.AsString() == "#ff8800ff"), "an alpha colour's #RRGGBB default becomes #rrggbbff");
	s = Schema(OneSetting("{ \"id\": \"i\", \"type\": \"int\", \"label\": \"L\", \"default\": 0 }"));
	Check(s.Find("i") && (s.Find("i")->mMin == -1000000) && (s.Find("i")->mMax == 1000000) && (s.Find("i")->mStep == 1),
		"an int without a range: -1000000..1000000, step 1");
	s = Schema(OneSetting("{ \"id\": \"f\", \"type\": \"float\", \"label\": \"L\", \"default\": 0.15, \"min\": 0.05, \"max\": 0.5, \"step\": 0.05 }"));
	Check(s.Find("f") && (s.Find("f")->mScale == 2) && (s.Find("f")->mMin == 5) && (s.Find("f")->mStep == 5) && (s.Find("f")->mDecimals == 2),
		"a float in hundredths: units 5, step 5, 2 decimals");

	// enabledIf and apply: warnings, the setting stays
	s = Schema("{ \"settings\": [ { \"id\": \"a\", \"type\": \"int\", \"label\": \"A\", \"default\": 1, \"enabledIf\": { \"setting\": \"b\", \"equals\": true } }, "
		"{ \"id\": \"b\", \"type\": \"bool\", \"label\": \"B\", \"default\": true } ] }");
	Check(s.Find("a") && s.Find("a")->mIfSetting.empty() && Contains(s.mWarnings, "\"enabledIf\""), "enabledIf naming a later setting: ignored with a warning");
	s = Schema("{ \"settings\": [ { \"id\": \"n\", \"type\": \"int\", \"label\": \"N\", \"default\": 1 }, "
		"{ \"id\": \"a\", \"type\": \"int\", \"label\": \"A\", \"default\": 1, \"enabledIf\": { \"setting\": \"n\", \"equals\": 1 } } ] }");
	Check(s.Find("a") && s.Find("a")->mIfSetting.empty() && Contains(s.mWarnings, "\"enabledIf\""), "enabledIf naming an int: ignored with a warning");
	s = Schema("{ \"settings\": [ { \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": \"p\", \"choices\": [ { \"value\": \"p\", \"label\": \"P\" }, "
		"{ \"value\": \"q\", \"label\": \"Q\" } ] }, { \"id\": \"a\", \"type\": \"int\", \"label\": \"A\", \"default\": 1, \"enabledIf\": { \"setting\": \"c\", \"equals\": \"z\" } } ] }");
	Check(s.Find("a") && s.Find("a")->mIfSetting.empty() && Contains(s.mWarnings, "\"enabledIf\""), "enabledIf with a value the choice does not have: ignored");
	s = Schema(OneSetting("{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 1, \"apply\": \"live\" }",
		"\"strings\": { \"K\": { \"$setting\": \"x\" } }"), AllNextGame());
	Check(s.Find("x") && (s.Find("x")->mLevel == MOD_LEVEL_RESTART) && Contains(s.mWarnings, "earlier than its data allows"),
		"apply looser than the data: the derived level, with a warning");
	s = Schema(OneSetting("{ \"id\": \"x\", \"type\": \"int\", \"label\": \"L\", \"default\": 1, \"apply\": \"soon\" }"));
	Check(s.Find("x") && Contains(s.mWarnings, "\"apply\" must be"), "an unknown apply: ignored with a warning");
}

//////////////////////////////////////////////////////////////////////////
// 3.6: stored values

static std::string Norm(const ModSettings& theSettings, const char* theId, const std::string& theStored, int* theResult = nullptr,
	std::string* theWarning = nullptr)
{
	JsonValue aValue;
	std::string aWarning;
	int aResult = theSettings.Normalize(*theSettings.Find(theId), J(theStored), &aValue, &aWarning);
	if (theResult)
		*theResult = aResult;
	if (theWarning)
		*theWarning = aWarning;
	return aValue.Dump(0);
}

static void TestValues()
{
	ModSettings s = Schema("{ \"settings\": ["
		"{ \"id\": \"b\", \"type\": \"bool\", \"label\": \"B\", \"default\": false },"
		"{ \"id\": \"i\", \"type\": \"int\", \"label\": \"I\", \"default\": 4, \"min\": 0, \"max\": 9, \"step\": 2 },"
		"{ \"id\": \"f\", \"type\": \"float\", \"label\": \"F\", \"default\": 1, \"min\": 0.5, \"max\": 2, \"step\": 0.1 },"
		"{ \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": 2, \"choices\": [ { \"value\": 1, \"label\": \"One\" }, { \"value\": 2, \"label\": \"Two\" }, "
		"{ \"value\": \"three\", \"label\": \"Three\" } ] },"
		"{ \"id\": \"t\", \"type\": \"text\", \"label\": \"T\", \"default\": \"HI\", \"maxLength\": 6, \"charset\": \"upper\" },"
		"{ \"id\": \"n\", \"type\": \"text\", \"label\": \"N\", \"default\": \"x\", \"maxLength\": 8, \"charset\": \"name\" },"
		"{ \"id\": \"k\", \"type\": \"color\", \"label\": \"K\", \"default\": \"#000000\" } ] }");
	int r;
	std::string w;
	CheckEq(Norm(s, "b", "true", &r), "true", "bool true");
	Check(r == MOD_VALUE_OK, "bool true is fine");
	CheckEq(Norm(s, "b", "\"yes\"", &r, &w), "false", "bool of the wrong type: the default");
	Check((r == MOD_VALUE_WRONG) && !w.empty(), "... with a warning");
	CheckEq(Norm(s, "i", "6", &r), "6", "int on a step");
	CheckEq(Norm(s, "i", "42", &r, &w), "8", "int above max: clamped to the top step");
	Check((r == MOD_VALUE_ADJUSTED) && (w.find("outside") != std::string::npos), "... with a warning");
	CheckEq(Norm(s, "i", "-5", &r, &w), "0", "int below min: clamped");
	CheckEq(Norm(s, "i", "5", &r, &w), "6", "int off a step: the nearest (half up)");
	Check(w.empty(), "... without a warning");
	CheckEq(Norm(s, "i", "2.5", &r), "4", "int that is not whole: the default");
	Check(r == MOD_VALUE_WRONG, "... as a wrong type");
	CheckEq(Norm(s, "f", "1.23", &r, &w), "1.2", "float off a step: snapped");
	Check((r == MOD_VALUE_ADJUSTED) && w.empty(), "... silently");
	CheckEq(Norm(s, "f", "0.30000000000000004", &r, &w), "0.5", "float below min: clamped");
	CheckEq(Norm(s, "f", "1.7", &r), "1.7", "float on a step");
	Check(r == MOD_VALUE_OK, "... is fine");
	CheckEq(Norm(s, "f", "1e300", &r), "2", "a huge float: max");
	CheckEq(Norm(s, "c", "\"three\"", &r), "\"three\"", "choice by its value");
	CheckEq(Norm(s, "c", "\"1\"", &r), "1", "choice \"1\" for the value 1");
	Check(r == MOD_VALUE_ADJUSTED, "... adjusted to the number");
	CheckEq(Norm(s, "c", "\"four\"", &r, &w), "2", "a choice that no longer exists: the default");
	Check((r == MOD_VALUE_WRONG) && !w.empty(), "... with a warning");
	CheckEq(Norm(s, "t", "\"abcdefgh\"", &r), "\"ABCDEF\"", "text: capitals and cut to maxLength");
	CheckEq(Norm(s, "t", "\"\\u00e9t\\u00e9\"", &r), "\"\xc3\x89T\xc3\x89\"", "text: Latin-1 lower case made capitals");
	CheckEq(Norm(s, "t", "\"A\\u20acB\"", &r), "\"A?B\"", "text: a character outside Latin-1 becomes ?");
	CheckEq(Norm(s, "t", "\"A\\tB\"", &r), "\"A?B\"", "text: a control character becomes ?");
	CheckEq(Norm(s, "n", "\"Bob! O'K\"", &r), "\"Bob? O?K\"", "text: characters outside the name charset become ?");
	CheckEq(Norm(s, "t", "7", &r), "\"HI\"", "text of the wrong type: the default");
	CheckEq(Norm(s, "k", "\"#A0B1C2\"", &r), "\"#a0b1c2\"", "colour: lower case");
	CheckEq(Norm(s, "k", "\"red\"", &r), "\"#000000\"", "colour that is no colour: the default");

	// ReadValues: every setting gets one, warnings say what was replaced
	ModSettingValues aValues;
	std::vector<std::string> aWarnings;
	s.ReadValues(J("{ \"i\": 42, \"c\": \"four\", \"zz\": 1 }"), true, &aValues, &aWarnings);
	Check((aValues.size() == 7) && (aValues["i"].AsNumber() == 8) && (aValues["c"].AsNumber() == 2) && (aValues["b"].AsBool() == false),
		"stored values read: missing ones at their defaults, unknown ids ignored");
	Check((aWarnings.size() == 2) && Contains(aWarnings, "\"i\"") && Contains(aWarnings, "using the default"), "two warnings: clamped and unknown choice");
}

//////////////////////////////////////////////////////////////////////////
// 3.3, 3.7: canonical text and float bits

static void TestCanonical()
{
	ModSettings s = Schema("{ \"settings\": ["
		"{ \"id\": \"f\", \"type\": \"float\", \"label\": \"F\", \"default\": 1, \"min\": 0, \"max\": 2, \"step\": 0.1, \"decimals\": 2 },"
		"{ \"id\": \"g\", \"type\": \"float\", \"label\": \"G\", \"default\": 0.15, \"min\": 0.05, \"max\": 0.5, \"step\": 0.05 },"
		"{ \"id\": \"t\", \"type\": \"text\", \"label\": \"T\", \"default\": \"a\", \"maxLength\": 20 },"
		"{ \"id\": \"i\", \"type\": \"int\", \"label\": \"I\", \"default\": -3, \"min\": -10 },"
		"{ \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": false, \"choices\": [ { \"value\": false, \"label\": \"No\" }, "
		"{ \"value\": -7, \"label\": \"Minus\" } ] } ] }");
	const ModSettingDef& f = *s.Find("f");
	const ModSettingDef& g = *s.Find("g");
	// 3 steps of 0.1 are units 3 / 10: the double of "0.3", not 0.1 + 0.1 + 0.1 (0.30000000000000004)
	JsonValue aThree;
	std::string w;
	s.Normalize(f, J("0.3"), &aThree, &w);
	CheckEq(ModSettingCanonical(f, aThree), "3fd3333333333333", "float 0.3: the bits of the double of \"0.3\"");
	CheckEq(ModSettingCanonical(f, JsonValue(0.1 + 0.1 + 0.1)), "3fd3333333333333", "float 0.1+0.1+0.1 snaps to the same bits");
	CheckEq(ModSettingCanonical(f, J("1.1")), "3ff199999999999a", "float 1.1 bits");
	CheckEq(ModSettingCanonical(g, J("0.15")), "3fc3333333333333", "float 0.15 bits");
	CheckEq(ModSettingCanonical(g, J("0.45")), "3fdccccccccccccd", "float 0.45 bits");
	CheckEq(ModSettingText(f, J("1.1")), "1.10", "float shown with its decimals");
	CheckEq(ModSettingText(g, J("0.05")), "0.05", "float 0.05 shown");
	CheckEq(ModSettingCanonical(*s.Find("t"), JsonValue(std::string("A&B=C?D;E%F G\xc3\xa9"))), "A%26B%3DC%3FD%3BE%25F G%E9",
		"text: separators and non-ASCII percent-escaped");
	CheckEq(ModSettingCanonical(*s.Find("i"), J("-3")), "-3", "int: decimal");
	CheckEq(ModSettingCanonical(*s.Find("c"), J("false")), "false", "choice false");
	CheckEq(ModSettingCanonical(*s.Find("c"), J("-7")), "-7", "choice -7");
}

//////////////////////////////////////////////////////////////////////////
// 3.4: placeholders

static std::string Effective(const ModSettings& theSettings, const std::string& theData, const std::string& theValues,
	std::vector<ModSettingsError>* theErrors = nullptr)
{
	ModSettingValues aValues;
	theSettings.ReadValues(J(theValues), true, &aValues, nullptr);
	JsonValue anEffective;
	std::vector<ModSettingsError> anErrors;
	theSettings.Resolve(J(theData), aValues, &anEffective, &anErrors);
	if (theErrors)
		*theErrors = anErrors;
	return anEffective.Dump(0);
}

static std::string FixtureManifest()
{
	std::string aText = ReadFile(std::string(MODSETTINGSTEST_FIXTURES) + "/settings-demo/mod.json");
	if (aText == "<missing>")
	{
		printf("the fixture tools/fixtures/mods/settings-demo/mod.json is missing\n");
		exit(2);
	}
	return aText;
}

// theManifest without its "settings" (to compare the data parts)
static std::string DataOnly(const std::string& theEffective)
{
	JsonValue aValue = J(theEffective);
	aValue.Remove("settings");
	return aValue.Dump(0);
}

static void TestPlaceholders()
{
	std::string aDemo = FixtureManifest();
	ModSettings s = Schema(aDemo, AllNextGame());
	Check(s.mErrors.empty() && s.mWarnings.empty(), "settings-demo: no errors or warnings" + (s.mErrors.empty() ? std::string() : ": " + s.mErrors[0]));

	std::vector<ModSettingsError> anErrors;
	std::string aDefaults = DataOnly(Effective(s, aDemo, "{}", &anErrors));
	CheckEq(aDefaults,
		"{\"id\":\"settings-demo\",\"name\":\"Settings Demo\",\"version\":\"1.0\",\"author\":\"byo-pop tests\","
		"\"description\":\"Every kind of setting and placeholder, for tools/modsettingstest (docs/MOD_MENU_DESIGN.md 3.1). JSON only: the files it names do not exist.\","
		"\"rules\":{\"match\":{\"minRun\":3}},\"tunables\":{\"timing\":{\"speed\":1}},\"features\":{\"replays\":false,\"tint\":[255,136,0]},"
		"\"strings\":{\"EXCELLENT\":\"EXCELLENT!\"},\"files\":{}}",
		"settings-demo at its defaults: $omit drops undo and board, null drops the music remap");
	Check(anErrors.empty(), "... without errors");

	std::string aSet = DataOnly(Effective(s, aDemo,
		"{ \"minRun\": 2, \"undo\": true, \"undoDepth\": 4, \"shape\": \"diamond\", \"speed\": 1.5, \"replays\": true, \"praise\": \"super!\","
		" \"music\": \"remix\", \"tint\": \"#102030\" }", &anErrors));
	JsonValue anEff = J(aSet);
	CheckEq(anEff["rules"].Dump(0),
		"{\"match\":{\"minRun\":2},\"undo\":{\"enabled\":true,\"depth\":4},\"board\":{\"mask\":[\"...##...\",\"..####..\",\".######.\",\"########\","
		"\"########\",\".######.\",\"..####..\",\"...##...\"]}}",
		"settings-demo with values: map entries, a nested placeholder");
	CheckEq(anEff["tunables"].Dump(0), "{\"timing\":{\"speed\":1.5}}", "a float value in tunables");
	CheckEq(anEff["features"].Dump(0), "{\"replays\":true,\"tint\":[16,32,48]}", "a bool in features, a colour as rgb");
	CheckEq(anEff["strings"].Dump(0), "{\"EXCELLENT\":\"SUPER!\"}", "a text value in strings (made capitals)");
	CheckEq(anEff["files"].Dump(0), "{\"music/1984-network.s3m\":\"extra/remix.ogg\"}", "a choice mapped to a file");

	// enabledIf: an inactive setting resolves to its default
	ModSettings t = Schema(
		"{ \"settings\": [ { \"id\": \"on\", \"type\": \"bool\", \"label\": \"On\", \"default\": false }, "
		"{ \"id\": \"n\", \"type\": \"int\", \"label\": \"N\", \"default\": 1, \"enabledIf\": { \"setting\": \"on\", \"equals\": true } } ], "
		"\"rules\": { \"x\": { \"$setting\": \"n\" } } }");
	std::string aData = "{ \"rules\": { \"x\": { \"$setting\": \"n\" } } }";
	CheckEq(Effective(t, aData, "{ \"on\": false, \"n\": 5 }"), "{\"rules\":{\"x\":1}}", "enabledIf false: the default");
	CheckEq(Effective(t, aData, "{ \"on\": true, \"n\": 5 }"), "{\"rules\":{\"x\":5}}", "enabledIf true: the value");

	// as, *, null and $omit
	ModSettings u = Schema(
		"{ \"settings\": [ { \"id\": \"k\", \"type\": \"color\", \"label\": \"K\", \"default\": \"#80ff0040\", \"alpha\": true }, "
		"{ \"id\": \"f\", \"type\": \"float\", \"label\": \"F\", \"default\": 2.5, \"min\": -5, \"max\": 5, \"step\": 0.5 }, "
		"{ \"id\": \"m\", \"type\": \"float\", \"label\": \"M\", \"default\": -2.5, \"min\": -5, \"max\": 5, \"step\": 0.5 }, "
		"{ \"id\": \"i\", \"type\": \"int\", \"label\": \"I\", \"default\": 3, \"min\": 0, \"max\": 9 }, "
		"{ \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": \"a\", \"choices\": [ { \"value\": \"a\", \"label\": \"A\" }, "
		"{ \"value\": \"b\", \"label\": \"B\" }, { \"value\": \"z\", \"label\": \"Z\" } ] } ] }");
	CheckEq(Effective(u, "{ \"features\": { \"a\": { \"$setting\": \"k\", \"as\": \"rgba\" }, \"b\": { \"$setting\": \"k\", \"as\": \"argb\" }, "
		"\"c\": { \"$setting\": \"k\", \"as\": \"rgb\" }, \"d\": { \"$setting\": \"k\", \"as\": \"string\" } } }", "{}"),
		"{\"features\":{\"a\":[128,255,0,64],\"b\":1082195712,\"c\":[128,255,0],\"d\":\"#80ff0040\"}}", "as rgba, argb (signed), rgb, string");
	CheckEq(Effective(u, "{ \"features\": { \"b\": { \"$setting\": \"k\", \"as\": \"argb\" } } }", "{ \"k\": \"#ff0000ff\" }"),
		"{\"features\":{\"b\":-65536}}", "as argb with alpha ff: a negative int32");
	CheckEq(Effective(u, "{ \"rules\": { \"a\": { \"$setting\": \"f\", \"as\": \"int\" }, \"b\": { \"$setting\": \"m\", \"as\": \"int\" }, "
		"\"c\": { \"$setting\": \"f\", \"as\": \"string\" } } }", "{}"),
		"{\"rules\":{\"a\":3,\"b\":-3,\"c\":\"2.5\"}}", "as int rounds half away from zero; as string shows the decimals");
	CheckEq(Effective(u, "{ \"rules\": { \"x\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"*\": 9 } }, "
		"\"y\": { \"$setting\": \"i\", \"map\": { \"3\": { \"$omit\": true }, \"*\": 1 } } } }", "{}"),
		"{\"rules\":{\"x\":null}}", "null under rules stays (a reset); $omit removes");
	CheckEq(Effective(u, "{ \"rules\": { \"x\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"*\": 9 } } } }", "{ \"c\": \"z\" }"),
		"{\"rules\":{\"x\":9}}", "a map's \"*\"");
	CheckEq(Effective(u, "{ \"strings\": { \"A\": { \"$setting\": \"i\" }, \"B\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"*\": \"Q\" } } }, "
		"\"imagePatches\": [ { \"$setting\": \"c\", \"map\": { \"a\": null, \"*\": { \"image\": \"x.png\" } } }, { \"image\": \"y.png\" } ], "
		"\"images\": { \"images/a\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"*\": { \"scale\": 2 } } } } }", "{}"),
		"{\"strings\":{\"A\":\"3\"},\"imagePatches\":[{\"image\":\"y.png\"}],\"images\":{}}",
		"null removes a strings/images member and an imagePatches element; a number in strings becomes text");
	CheckEq(Effective(u, "{ \"features\": { \"$setting\": \"c\", \"map\": { \"a\": { \"$omit\": true }, \"*\": { \"replays\": true } } }, \"rules\": {} }", "{}"),
		"{\"rules\":{}}", "a whole key that resolves to $omit is left out");

	// Errors: the key is left out
	struct Bad
	{
		const char* mData;
		const char* mKey;
		const char* mError;
	};
	const Bad aBad[] = {
		{"{ \"rules\": { \"x\": { \"$setting\": \"nope\" } } }", "rules", "not declared"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"i\", \"map\": { \"4\": 1 } } } }", "rules", "no entry for 3"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"i\", \"map\": { \"12\": 1, \"*\": 2 } } } }", "rules", "can never match"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"c\", \"map\": { \"q\": 1, \"*\": 2 } } } }", "rules", "can never match"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"f\", \"map\": { \"*\": 2 } } } }", "rules", "only bool, int and choice"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"k\", \"map\": { \"*\": 2 } } } }", "rules", "only bool, int and choice"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"i\", \"as\": \"rgb\" } } }", "rules", "does not fit"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"k\", \"as\": \"int\" } } }", "rules", "does not fit"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"c\", \"map\": { \"*\": 1 }, \"as\": \"int\" } } }", "rules", "cannot be used together"},
		{"{ \"rules\": { \"x\": { \"$setting\": \"i\", \"oops\": 1 } } }", "rules", "unknown key \"oops\""},
		{"{ \"name\": { \"$setting\": \"i\", \"as\": \"string\" } }", "name", "not allowed in this key"},
		{"{ \"priority\": { \"$setting\": \"i\" } }", "priority", "not allowed in this key"},
		{"{ \"files\": { \"$setting\": \"c\", \"map\": { \"*\": {} } } }", "files", "only for its members"},
		{"{ \"files\": { \"music/a.ogg\": { \"$setting\": \"k\", \"as\": \"string\" } } }", "files", "cannot name a file"},
	};
	for (const Bad& aCase : aBad)
	{
		std::string aManifest = std::string(aCase.mData);
		JsonValue aParsed = J(aManifest);
		JsonValue aWithSettings = J("{ \"settings\": [] }");
		ModSettings v = u;
		v.Analyze(aParsed, NextGameFilter(), std::vector<std::string>{"data/diff.txt"});
		JsonValue anEffective;
		std::vector<ModSettingsError> anErrs;
		ModSettingValues aValues;
		v.Resolve(aParsed, aValues, &anEffective, &anErrs);
		bool isReported = false;
		for (const ModSettingsError& e : anErrs)
			isReported = isReported || ((e.mKey == aCase.mKey) && (e.mText.find(aCase.mError) != std::string::npos));
		Check(!anEffective.Has(aCase.mKey) && isReported,
			std::string("error \"") + aCase.mError + "\" leaves out \"" + aCase.mKey + "\": " + aCase.mData + (anErrs.empty() ? "" : " -> " + anErrs[0].mText));
	}

	// Nesting deeper than 8 placeholders
	std::string aDeep = "1";
	for (int i = 0; i < 9; i++)
		aDeep = "{ \"$setting\": \"c\", \"map\": { \"*\": " + aDeep + " } }";
	std::vector<ModSettingsError> aDeepErrors;
	ModSettings w = u;
	JsonValue aDeepData = J("{ \"rules\": { \"x\": " + aDeep + " } }");
	w.Analyze(aDeepData, NextGameFilter(), std::vector<std::string>{"data/diff.txt"});
	Check(w.mBadKeys.count("rules") && Contains(w.mErrors, "nested deeper than 8"), "placeholders nested 9 deep: an error");

	// A mod without settings is never touched
	ModSettings aNone = Schema("{ \"rules\": { \"x\": { \"$setting\": \"i\" } } }");
	CheckEq(Effective(aNone, "{ \"rules\": { \"x\": { \"$setting\": \"i\" } } }", "{}"), "{\"rules\":{\"x\":{\"$setting\":\"i\"}}}",
		"a mod without settings: the manifest as it is");
}

//////////////////////////////////////////////////////////////////////////
// 3.5, 3.7: levels and score-affecting settings

static void TestLevels()
{
	std::string aDemo = FixtureManifest();
	ModSettings s = Schema(aDemo, AllNextGame());
	struct Expect
	{
		const char* mId;
		int mLevel;
		bool mScores;
	};
	const Expect anExpected[] = {
		{"minRun", MOD_LEVEL_NEXT_GAME, true}, {"undo", MOD_LEVEL_NEXT_GAME, true}, {"undoDepth", MOD_LEVEL_NEXT_GAME, true},
		{"shape", MOD_LEVEL_NEXT_GAME, true}, {"speed", MOD_LEVEL_NEXT_GAME, true}, {"replays", MOD_LEVEL_NEXT_GAME, false},
		{"praise", MOD_LEVEL_RESTART, false}, {"music", MOD_LEVEL_RESTART, false}, {"tint", MOD_LEVEL_NEXT_GAME, false},
		{"volume", MOD_LEVEL_LIVE, false},
	};
	for (const Expect& e : anExpected)
	{
		const ModSettingDef* d = s.Find(e.mId);
		Check(d && (d->mLevel == e.mLevel) && (d->mScores == e.mScores),
			std::string("settings-demo ") + e.mId + ": " + ModSettingLevelName(e.mLevel) + (e.mScores ? ", scores" : "") +
			(d ? std::string(" (got ") + ModSettingLevelName(d->mLevel) + (d->mScores ? ", scores)" : ")") : ""));
	}
	ModSettings aNoFilter = Schema(aDemo);
	Check((aNoFilter.Find("minRun")->mLevel == MOD_LEVEL_RESTART) && (aNoFilter.Find("replays")->mLevel == MOD_LEVEL_RESTART) &&
		(aNoFilter.Find("volume")->mLevel == MOD_LEVEL_LIVE), "without a next-game filter rules and features need a restart; unused stays live");

	// A game whose load-time art limits a rule (Bejeweled's board.gemColors): such paths are restart-level
	NextGameFilter aGemColors = [](const std::string& thePath) {
		const std::string aKey = "rules.board.gemColors";
		return !((thePath == aKey) || (aKey.compare(0, thePath.size() + 1, thePath + ".") == 0) ||
			(thePath.compare(0, aKey.size() + 1, aKey + ".") == 0));
	};
	std::string aSettings = "\"settings\": [ { \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": \"a\", \"choices\": [ "
		"{ \"value\": \"a\", \"label\": \"A\" }, { \"value\": \"b\", \"label\": \"B\" } ] }, { \"id\": \"n\", \"type\": \"int\", \"label\": \"N\", \"default\": 7, \"min\": 3, \"max\": 12 } ]";
	ModSettings g = Schema("{ " + aSettings + ", \"rules\": { \"board\": { \"gemColors\": { \"$setting\": \"n\" } } } }", aGemColors);
	Check(g.Find("n")->mLevel == MOD_LEVEL_RESTART, "a placeholder at rules.board.gemColors: restart (the filter refuses it)");
	g = Schema("{ " + aSettings + ", \"rules\": { \"board\": { \"$setting\": \"c\", \"map\": { \"a\": { \"$omit\": true }, \"b\": { \"gemColors\": 8 } } } } }", aGemColors);
	Check(g.Find("c")->mLevel == MOD_LEVEL_RESTART, "a map that can set rules.board.gemColors: restart");
	g = Schema("{ " + aSettings + ", \"rules\": { \"board\": { \"$setting\": \"c\", \"map\": { \"a\": { \"$omit\": true }, \"b\": { \"mask\": [\"#\"] } } } } }", aGemColors);
	Check(g.Find("c")->mLevel == MOD_LEVEL_NEXT_GAME, "a map that sets only rules.board.mask: next game");
	g = Schema("{ " + aSettings + ", \"rules\": { \"board\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"b\": { \"mask\": [\"#\"] } } } } }", aGemColors);
	Check(g.Find("c")->mLevel == MOD_LEVEL_RESTART, "a map that resets all of rules.board (null): restart");

	// Scores
	ModSettings f = Schema("{ " + aSettings + ", \"files\": { \"Data\\\\Diff.TXT\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"b\": \"d.txt\" } } } }");
	Check(f.Find("c")->mScores && (f.Find("c")->mLevel == MOD_LEVEL_RESTART), "a files remap of data/diff.txt (any spelling): scores, restart");
	f = Schema("{ " + aSettings + ", \"files\": { \"music/x.ogg\": { \"$setting\": \"c\", \"map\": { \"a\": null, \"b\": \"d.ogg\" } } } }");
	Check(!f.Find("c")->mScores, "a files remap of music: no scores");
	f = Schema(OneSetting("{ \"id\": \"w\", \"type\": \"bool\", \"label\": \"W\", \"default\": false, \"affectsScores\": true }"), AllNextGame());
	Check(f.Find("w")->mScores && (f.Find("w")->mLevel == MOD_LEVEL_NEXT_GAME), "affectsScores without a data use: scores, and never live");
	f = Schema(OneSetting("{ \"id\": \"w\", \"type\": \"bool\", \"label\": \"W\", \"default\": false, \"affectsScores\": true }"));
	Check(f.Find("w")->mLevel == MOD_LEVEL_RESTART, "... restart without next-game support");
	f = Schema(OneSetting("{ \"id\": \"w\", \"type\": \"bool\", \"label\": \"W\", \"default\": false, \"apply\": \"restart\" }"));
	Check(f.Find("w")->mLevel == MOD_LEVEL_RESTART, "apply restart without a data use");

	// enabledIf: the condition applies no earlier than what depends on it, and counts for the scores with it
	f = Schema("{ \"settings\": [ { \"id\": \"on\", \"type\": \"bool\", \"label\": \"On\", \"default\": false }, "
		"{ \"id\": \"n\", \"type\": \"int\", \"label\": \"N\", \"default\": 1, \"enabledIf\": { \"setting\": \"on\", \"equals\": true } } ], "
		"\"rules\": { \"x\": { \"$setting\": \"n\" } } }", AllNextGame());
	Check((f.Find("on")->mLevel == MOD_LEVEL_NEXT_GAME) && f.Find("on")->mScores, "an enabledIf condition takes its dependant's level and scores");

	// '%' never reaches a strings value
	f = Schema(OneSetting("{ \"id\": \"t\", \"type\": \"text\", \"label\": \"T\", \"default\": \"A\" }", "\"strings\": { \"LEVEL\": { \"$setting\": \"t\" } }"));
	CheckEq(Norm(f, "t", "\"%n%s\""), "\"?n?s\"", "text that goes into strings: % becomes ?");
	f = Schema(OneSetting("{ \"id\": \"t\", \"type\": \"text\", \"label\": \"T\", \"default\": \"100%\" }", "\"strings\": { \"LEVEL\": { \"$setting\": \"t\" } }"));
	Check(f.mBadKeys.count("strings") && Contains(f.mErrors, "cannot contain %"), "a strings text setting whose default has %: strings left out");

	// The score key pairs
	ModSettingValues aValues;
	s.ReadValues(J("{ \"minRun\": 2, \"replays\": true, \"praise\": \"WOW\", \"undoDepth\": 5, \"speed\": 1.5 }"), true, &aValues, nullptr);
	CheckEq(s.ScoreKeyPairs(aValues), "minRun=2&speed=3ff8000000000000", "key pairs: score-affecting values off their defaults, inactive ones left out");
	s.ReadValues(J("{ \"undo\": true, \"undoDepth\": 5 }"), true, &aValues, nullptr);
	CheckEq(s.ScoreKeyPairs(aValues), "undo=true&undoDepth=5", "key pairs sorted by id");
	s.ReadValues(J("{ \"replays\": true }"), true, &aValues, nullptr);
	CheckEq(s.ScoreKeyPairs(aValues), "", "a feature value: no pairs");
}

//////////////////////////////////////////////////////////////////////////
// ModManager: the score key, the data/diff.txt owner, the effective manifests

static const char* const kRelaxed =
	"{ \"id\": \"example-relaxed\", \"name\": \"Relaxed Hints\", \"version\": \"1.0\", \"author\": \"byo-pop examples\", "
	"\"description\": \"Hints cost 10 points instead of 50 (per level multiplier). Uses its own high-score table.\", "
	"\"tunables\": { \"scoring\": { \"hintPenalty\": 10 } } }";
static const char* const kRelaxedSettings =
	"{ \"id\": \"example-relaxed\", \"name\": \"Relaxed Hints\", \"version\": \"1.0\", \"author\": \"byo-pop examples\", "
	"\"settings\": [ { \"id\": \"hintPenalty\", \"label\": \"Hint cost\", \"type\": \"int\", \"default\": 10, \"min\": 0, \"max\": 50, \"step\": 5 } ], "
	"\"tunables\": { \"scoring\": { \"hintPenalty\": { \"$setting\": \"hintPenalty\" } } } }";

static void TestScoreKey()
{
	CheckEq(Fnv({"example-relaxed@1.0"}), "58ebd9d7", "FNV-1a of example-relaxed@1.0");
	{
		Sandbox aBox("key-plain");
		aBox.Mod("example-relaxed", kRelaxed);
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), "58ebd9d7", "example-relaxed: score key 58ebd9d7");
		CheckEq(ModManager::Get().GetNextScoreKey(), "58ebd9d7", "... and the desired state's key is the same");
		Check(ModManager::Get().GetTunable("scoring.hintPenalty", 50) == 10, "... hintPenalty 10");
		Check(!aBox.HasUser("mod-settings.json") && !aBox.HasUser("mods.json") && !aBox.HasUser("mods-status.json"),
			"Refresh writes nothing into the user dir");
	}
	{
		Sandbox aBox("key-settings");
		aBox.Mod("example-relaxed", kRelaxedSettings);
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), "58ebd9d7", "example-relaxed with a defaults-only settings block: still 58ebd9d7");
		Check(ModManager::Get().GetTunable("scoring.hintPenalty", 50) == 10, "... hintPenalty 10 from the default");
		aBox.User("mod-settings.json", "{ \"v\": 1, \"mods\": { \"example-relaxed\": { \"hintPenalty\": 25 } } }");
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), Fnv({"example-relaxed@1.0?hintPenalty=25"}), "with hintPenalty 25: the key of example-relaxed@1.0?hintPenalty=25");
		Check(ModManager::Get().GetTunable("scoring.hintPenalty", 50) == 25, "... and hintPenalty 25 is applied");
		aBox.User("mod-settings.json", "{ \"v\": 1, \"mods\": { \"example-relaxed\": { \"hintPenalty\": 10 } } }");
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), "58ebd9d7", "a stored value equal to the default: today's key");
	}
	{
		// A mod whose only link to the scores is a data/diff.txt remap that is off by default
		Sandbox aBox("key-diff");
		aBox.Mod("hard", "{ \"id\": \"hard\", \"version\": \"2\", \"settings\": [ { \"id\": \"hard\", \"label\": \"Hard\", \"type\": \"bool\", \"default\": false } ], "
			"\"files\": { \"data/diff.txt\": { \"$setting\": \"hard\", \"map\": { \"false\": null, \"true\": \"hard.txt\" } } } }");
		WriteFile(aBox.mMods + "/hard/hard.txt", "1 2 3 4 5 6 7 8 9 10 11 12");
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), "", "a diff.txt remap off by default: no key entry (principle 5)");
		Check(!Applied("hard")->AffectsScores(), "... the mod does not affect scores");
		ModManager::Get().EditSetting("hard", "hard", JsonValue(true));
		CheckEq(ModManager::Get().GetNextScoreKey(), Fnv({"hard@2?hard=true"}), "turned on: the desired key has hard@2?hard=true");
		ModManager::Get().Refresh();
		CheckEq(ModManager::Get().GetScoreKey(), Fnv({"hard@2?hard=true"}), "... and so has the applied key after a restart");
		Check(Applied("hard")->mOverridesDiff, "... data/diff.txt resolves into it");
	}
}

static void TestDiffOwner()
{
	// data/diff.txt: a "files" remap wins over a file in a mod; among remaps and among files the higher priority
	// wins; the remap's key may be spelt in any case and with backslashes
	Sandbox aBox("diff-owner");
	aBox.Mod("a-file", "{ \"id\": \"a-file\", \"priority\": 1 }");
	WriteFile(aBox.mMods + "/a-file/data/diff.txt", "1");
	aBox.Mod("b-remap", "{ \"id\": \"b-remap\", \"files\": { \"Data\\\\Diff.TXT\": \"mine.txt\" } }");
	WriteFile(aBox.mMods + "/b-remap/mine.txt", "2");
	aBox.Mod("c-remap", "{ \"id\": \"c-remap\", \"priority\": 2, \"files\": { \"data/diff.txt\": \"missing.txt\" } }");
	aBox.Mod("d-plain", "{ \"id\": \"d-plain\" }");

	struct Case
	{
		const char* mConfig;
		const char* mOwner;
	};
	const Case aCases[] = {
		{"{}", "b-remap"},
		{"{ \"disabled\": [\"b-remap\"] }", "a-file"},
		{"{ \"disabled\": [\"b-remap\", \"a-file\"] }", ""},
	};
	for (const Case& aCase : aCases)
	{
		aBox.User("mods.json", aCase.mConfig);
		ModManager::Get().Refresh();
		std::string anOwner;
		VFS::Get().Resolve("data/diff.txt", &anOwner);
		CheckEq(anOwner, aCase.mOwner, std::string("VFS::Resolve owner with ") + aCase.mConfig);
		std::string anApplied;
		for (const ModInfo& m : ModManager::Get().GetMods())
			if (m.mEnabled && m.mOverridesDiff)
				anApplied = m.mId;
		CheckEq(anApplied, aCase.mOwner, "... ModManager marks the same mod");
		CheckEq(ModManager::Get().GetNextScoreKey(), ModManager::Get().GetScoreKey(), "... and the desired state's key (its own owner) agrees");
		CheckEq(ModManager::Get().GetScoreKey(), std::string(aCase.mOwner).empty() ? "" : Fnv({std::string(aCase.mOwner) + "@"}),
			"... the key is the owner's entry");
	}
}

//////////////////////////////////////////////////////////////////////////
// ModManager: mods.json and mod-settings.json edits

static void TestEdits()
{
	gUserDataChanges = 0;
	SetUserDataChangedCallback(&OnUserDataChanged);

	Sandbox aBox("edits");
	aBox.Mod("hd", "{ \"id\": \"hd\", \"name\": \"HD\" }");
	aBox.Mod("low", "{ \"id\": \"low\", \"priority\": -1 }");
	aBox.Mod("top", "{ \"id\": \"top\", \"priority\": 5 }");
	aBox.Mod("zz", "{ \"id\": \"zz\" }");
	std::string aDemo = FixtureManifest();
	aBox.Mod("settings-demo", aDemo);
	aBox.User("mods.json", "{ \"order\": [\"gone\", \"zz\", \"hd\"], \"disabled\": [], \"future\": { \"x\": 1 } }");
	ModManager& m = ModManager::Get();
	m.Refresh();

	auto Order = [&]() {
		std::string s;
		for (const ModInfo& d : m.GetDesiredMods())
			s += (s.empty() ? "" : ",") + d.mId + (d.mEnabled ? "" : "-");
		return s;
	};
	CheckEq(Order(), "top,zz,hd,settings-demo,low", "order: priority first, then mods.json, then the rest by folder");

	int aChanges = gUserDataChanges;
	CheckEq(m.EditEnable("hd", true), "", "enabling an enabled mod");
	Check(gUserDataChanges == aChanges, "... writes nothing");
	CheckEq(m.EditEnable("hd", false), "", "disabling hd");
	CheckEq(aBox.UserText("mods.json"), "{\n  \"order\": [\n    \"gone\",\n    \"zz\",\n    \"hd\"\n  ],\n  \"disabled\": [\n    \"hd\"\n  ],\n"
		"  \"future\": {\n    \"x\": 1\n  }\n}\n", "mods.json keeps its unknown key and the order");
	Check(gUserDataChanges == aChanges + 1, "... one user data change");
	Check(!aBox.HasUser("mods.json.tmp"), "... no temporary file left");
	CheckEq(Order(), "top,zz,hd-,settings-demo,low", "hd off in the desired state");
	Check(Applied("hd")->mEnabled, "... still on in the applied state");
	ModPending aPending = m.GetPending();
	Check(Contains(aPending.mRestart, "HD turned off"), "pending: HD turned off");
	CheckEq(m.EditEnable("hd", true), "", "hd on again");
	CheckEq(J(aBox.UserText("mods.json"))["disabled"].Dump(0), "[]", "... out of \"disabled\"");
	Check(m.GetPending().mRestart.empty(), "nothing pending once the files match the applied state");

	Check(!m.CanMove("top", 1) && !m.CanMove("low", -1) && !m.CanMove("top", -1), "no moves across priorities or past the ends");
	Check(m.EditMove("top", 1) != "", "... refused");
	CheckEq(m.EditMove("settings-demo", -1), "", "settings-demo up");
	CheckEq(Order(), "top,zz,settings-demo,hd,low", "settings-demo before hd");
	JsonValue aConfig = J(aBox.UserText("mods.json"));
	CheckEq(aConfig["order"].Dump(0), "[\"gone\",\"top\",\"zz\",\"settings-demo\",\"hd\",\"low\"]",
		"the order keeps the slot of an id not on disk (gone) and lists the others");
	Check(aConfig.Has("future"), "... the unknown key stays");
	Check(Contains(m.GetPending().mRestart, "Load order"), "pending: load order");
	CheckEq(m.EditMove("settings-demo", 1), "", "settings-demo back down");
	Check(m.GetPending().mRestart.empty(), "nothing pending in the original order");

	// mod-settings.json
	Check(!aBox.HasUser("mod-settings.json"), "no mod-settings.json until a value changes");
	aChanges = gUserDataChanges;
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(3.0)), "", "setting minRun to its default");
	Check(!aBox.HasUser("mod-settings.json") && (gUserDataChanges == aChanges), "... writes nothing");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(2.0)), "", "minRun 2");
	CheckEq(aBox.UserText("mod-settings.json"), "{\n  \"v\": 1,\n  \"mods\": {\n    \"settings-demo\": {\n      \"minRun\": 2\n    }\n  }\n}\n",
		"mod-settings.json holds only the changed value");
	Check(Contains(m.GetPending().mRestart, "Shortest match"), "without next-game support: pending a restart");
	Check(m.EditSetting("settings-demo", "minRun", JsonValue(std::string("two"))) != "", "a value of the wrong type is refused");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(9.0)), "", "a value out of range is clamped");
	CheckEq(J(aBox.UserText("mod-settings.json"))["mods"]["settings-demo"]["minRun"].Dump(0), "5", "... to 5");
	Check(m.EditSetting("nobody", "x", JsonValue(1.0)) != "" && m.EditSetting("settings-demo", "nothing", JsonValue(1.0)) != "", "unknown mods and settings are refused");

	// Unknown content stays
	aBox.User("mod-settings.json", "{ \"v\": 1, \"extra\": [1, 2], \"mods\": { \"settings-demo\": { \"minRun\": 4, \"old\": true }, \"elsewhere\": { \"a\": 1 } } }");
	CheckEq(m.EditSetting("settings-demo", "praise", JsonValue(std::string("Wow"))), "", "praise WOW");
	JsonValue aSettings = J(aBox.UserText("mod-settings.json"));
	CheckEq(aSettings.Dump(0), "{\"v\":1,\"extra\":[1,2],\"mods\":{\"settings-demo\":{\"minRun\":4,\"old\":true,\"praise\":\"WOW\"},\"elsewhere\":{\"a\":1}}}",
		"a write keeps unknown keys, unknown settings and mods that are not installed");
	CheckEq(m.EditReset("settings-demo"), "", "reset demo");
	CheckEq(J(aBox.UserText("mod-settings.json")).Dump(0), "{\"v\":1,\"extra\":[1,2],\"mods\":{\"settings-demo\":{\"old\":true},\"elsewhere\":{\"a\":1}}}",
		"reset removes the mod's values (and keeps what it does not know)");

	// Live values apply at once; restart values wait
	CheckEq(m.GetLiveValue("settings-demo", "volume").Dump(0), "100", "live value: the default");
	CheckEq(m.EditSetting("settings-demo", "volume", JsonValue(40.0)), "", "volume 40");
	CheckEq(m.GetLiveValue("settings-demo", "volume").Dump(0), "40", "a live value changes at once");
	Check(!Contains(m.GetPending().mRestart, "Volume") && !Contains(m.GetPending().mNextGame, "Volume"), "... and is never pending");
	CheckEq(m.EditSetting("settings-demo", "praise", JsonValue(std::string("yes"))), "", "praise YES");
	CheckEq(m.GetLiveValue("settings-demo", "praise").Dump(0), "\"EXCELLENT!\"", "a restart value keeps its applied value");
	Check(Contains(m.GetPending().mRestart, "Praise text"), "... and is pending a restart");

	// The state JSON parses and describes the model
	JsonValue aState = J(m.GetStateJson());
	Check(aState["running"].IsBool() && !aState["nomods"].AsBool() && aState["mods"].IsArray() && (aState["mods"].Size() == 5), "state JSON: five mods");
	const JsonValue* aDemoState = nullptr;
	for (const JsonValue& aMod : aState["mods"].Items())
		if (aMod["id"].AsString() == "settings-demo")
			aDemoState = &aMod;
	Check(aDemoState && (*aDemoState)["scores"].AsBool() && (*aDemoState)["features"].AsBool() && ((*aDemoState)["pending"].AsString("") == "restart") &&
		((*aDemoState)["settings"].Size() == 12), "state JSON: settings-demo scores, has features, a restart pending, 12 rows");
	if (aDemoState)
	{
		const JsonValue& aPraise = (*aDemoState)["settings"][8];
		CheckEq(aPraise.Dump(0), "{\"id\":\"praise\",\"label\":\"Praise text\",\"type\":\"text\",\"default\":\"EXCELLENT!\",\"maxLength\":16,"
			"\"charset\":\"upper\",\"noPercent\":true,\"level\":\"restart\",\"scores\":false,\"value\":\"YES\",\"applied\":\"EXCELLENT!\",\"enabled\":true,"
			"\"pending\":\"restart\"}", "state JSON: the praise row");
	}

	// The game's check of the desired data
	m.SetDataCheck([](const ModInfo& theMod, const JsonValue& theEffective, std::vector<std::string>* theMessages) {
		if (theEffective["rules"]["match"]["minRun"].AsInt(3) < 3)
			theMessages->push_back(theMod.mId + ": rules: match.minRun below 3");
	});
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(2.0)), "", "minRun 2 for the data check");
	Check(m.GetStateJson().find("settings-demo: rules: match.minRun below 3") != std::string::npos, "the game's data check: a warning in the state");
	m.SetDataCheck(ModDataCheck());

	// An unreadable file: values at their defaults; the first write keeps it as mod-settings.bad.json
	aBox.User("mod-settings.json", "{ not json");
	m.ScanDesired();
	CheckEq(m.GetSettingsFileState(), "unreadable", "an unreadable mod-settings.json is reported");
	CheckEq(m.FindDesired("settings-demo")->mValues.at("minRun").Dump(0), "3", "... its values are the defaults");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(4.0)), "", "a write after it");
	CheckEq(ReadFile(aBox.mUser + "/mod-settings.bad.json"), "{ not json", "... moves the broken file to mod-settings.bad.json");
	CheckEq(J(aBox.UserText("mod-settings.json")).Dump(0), "{\"v\":1,\"mods\":{\"settings-demo\":{\"minRun\":4}}}", "... and writes a new one");

	// A newer version's file is read only
	std::string aNewer = "{ \"v\": 2, \"mods\": { \"settings-demo\": { \"minRun\": 2 } } }";
	aBox.User("mod-settings.json", aNewer);
	m.ScanDesired();
	CheckEq(m.GetSettingsFileState(), "newer", "a mod-settings.json of a newer version");
	CheckEq(m.FindDesired("settings-demo")->mValues.at("minRun").Dump(0), "2", "... is read");
	Check(m.EditSetting("settings-demo", "minRun", JsonValue(5.0)).find("newer version") != std::string::npos, "... but not written");
	CheckEq(aBox.UserText("mod-settings.json"), aNewer, "... it stays as it was");

	// A broken mods.json is kept as mods.bad.json by the first write
	aBox.User("mods.json", "[1, 2");
	CheckEq(m.EditEnable("zz", false), "", "disabling with a broken mods.json");
	CheckEq(ReadFile(aBox.mUser + "/mods.bad.json"), "[1, 2", "... keeps it as mods.bad.json");

	// A status write reaches the backup (the user data callback)
	aChanges = gUserDataChanges;
	m.WriteStatus("{ \"a\": 1 }\n");
	Check((gUserDataChanges == aChanges + 1) && (aBox.UserText("mods-status.json") == "{ \"a\": 1 }\n"), "WriteStatus writes and reports the change");
	m.WriteStatus("{ \"a\": 1 }\n");
	Check(gUserDataChanges == aChanges + 1, "... the same text again: nothing");
	m.WriteStatus("");
	Check(!aBox.HasUser("mods-status.json") && (gUserDataChanges == aChanges + 2), "... an empty text removes it and reports the change");
	m.WriteStatus("");
	Check(gUserDataChanges == aChanges + 2, "... and nothing when there is no file");

	// Duplicate ids
	aBox.Mod("zz-copy", "{ \"id\": \"zz\" }");
	m.ScanDesired();
	int aCopies = 0;
	for (const ModInfo& d : m.GetDesiredMods())
		if ((d.mId == "zz") && d.mDuplicate && !d.mEnabled && (d.mError.find("has the id") != std::string::npos))
			aCopies++;
	Check(aCopies == 1, "a second folder with the same id: flagged and off");
	SetUserDataChangedCallback(nullptr);
}

//////////////////////////////////////////////////////////////////////////
// ModManager: values held for the next game

static void TestNextGame()
{
	Sandbox aBox("next");
	std::string aDemo = FixtureManifest();
	aBox.Mod("settings-demo", aDemo);
	ModManager& m = ModManager::Get();
	m.SetNextGameFilter(AllNextGame());
	m.Refresh();
	const ModInfo* a = Applied("settings-demo");
	CheckEq(a->mRulesJson.Dump(0), "{\"match\":{\"minRun\":3}}", "applied rules at the defaults");
	Check(a->mHasRules && a->mHasFeatures && a->AffectsScores(), "... rules and features, scores");
	std::string aKey = m.GetScoreKey();
	CheckEq(aKey, Fnv({"settings-demo@1.0"}), "score key settings-demo@1.0");

	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(2.0)), "", "minRun 2 (next game)");
	CheckEq(J(aBox.UserText("mod-settings.json")).Dump(0), "{\"v\":1,\"next\":{\"settings-demo\":{\"minRun\":2}}}", "held under \"next\"");
	Check(Contains(m.GetPending().mNextGame, "Shortest match") && m.GetPending().mRestart.empty(), "pending: the next game");
	CheckEq(m.GetNextScoreKey(), Fnv({"settings-demo@1.0?minRun=2"}), "the next key has minRun=2");
	CheckEq(m.GetScoreKey(), aKey, "the applied key is unchanged");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(3.0)), "", "minRun back to 3");
	Check(!aBox.HasUser("mod-settings.json") || (J(aBox.UserText("mod-settings.json")).Dump(0) == "{\"v\":1}"), "... nothing held any more");
	Check(!m.HasNextGameValues(), "... no next-game values");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(2.0)), "", "minRun 2 again");
	CheckEq(m.EditSetting("settings-demo", "undo", JsonValue(true)), "", "undo on");
	CheckEq(m.EditSetting("settings-demo", "praise", JsonValue(std::string("NICE"))), "", "praise NICE (restart)");

	Check(!m.CommitNextGameValues(false), "a commit that would strand a game: nothing");
	CheckEq(a->mRulesJson.Dump(0), "{\"match\":{\"minRun\":3}}", "... the rules stay");
	Check(m.CommitNextGameValues(true), "the commit at a fresh game");
	CheckEq(J(aBox.UserText("mod-settings.json")).Dump(0), "{\"v\":1,\"mods\":{\"settings-demo\":{\"praise\":\"NICE\",\"minRun\":2,\"undo\":true}}}",
		"... moves the values into \"mods\"");
	CheckEq(a->mRulesJson.Dump(0), "{\"match\":{\"minRun\":2},\"undo\":{\"enabled\":true,\"depth\":1}}", "... re-resolves the rules");
	CheckEq(m.GetScoreKey(), Fnv({"settings-demo@1.0?minRun=2&undo=true"}), "... and the score key");
	CheckEq(m.GetLiveValue("settings-demo", "praise").Dump(0), "\"EXCELLENT!\"", "... restart values stay as they were");
	Check(m.GetPending().mNextGame.empty() && Contains(m.GetPending().mRestart, "Praise text"), "only the restart value is pending");
	Check(!m.CommitNextGameValues(true), "a second commit: nothing to do");

	// Tunables are rebuilt with the commit
	CheckEq(m.EditSetting("settings-demo", "speed", JsonValue(1.5)), "", "speed 1.5");
	Check(m.GetTunable("timing.speed", 0) == 1.0, "the tunable is applied only at the commit");
	m.CommitNextGameValues(true);
	Check(m.GetTunable("timing.speed", 0) == 1.5, "... then it is 1.5");
	m.SetNextGameFilter(NextGameFilter());
}

//////////////////////////////////////////////////////////////////////////
// The example mods

static void TestExamples()
{
	// games/bejeweled/mods/example-settings: valid, and at its defaults the game's own values
	std::string aText = ReadFile(MODSETTINGSTEST_EXAMPLE);
	Check(aText != "<missing>", "games/bejeweled/mods/example-settings/mod.json is there");
	ModSettings s = Schema(aText, AllNextGame());
	Check(s.mErrors.empty() && s.mWarnings.empty() && (s.mDefs.size() == 7), "example-settings: valid, 4 settings in 3 sections");
	JsonValue anEffective;
	std::vector<ModSettingsError> anErrors;
	s.Resolve(J(aText), ModSettingValues(), &anEffective, &anErrors);
	Check(anErrors.empty() && !anEffective.Has("features") && (anEffective["tunables"].Dump(0) ==
		"{\"scoring\":{\"hintPenalty\":50},\"timing\":{\"warningFraction\":0.15}}"), "example-settings at its defaults: the game's own values, no features");
	Check((s.Find("hintPenalty")->mLevel == MOD_LEVEL_NEXT_GAME) && s.Find("hintPenalty")->mScores && !s.Find("replays")->mScores &&
		(s.Find("excellent")->mLevel == MOD_LEVEL_RESTART), "example-settings levels");
}

//////////////////////////////////////////////////////////////////////////
// The mod menu's controls (ModSettingStep and friends) and its restart (Relaunch's quoting)

static void TestMenuControls()
{
	ModSettings s = Schema("{ \"settings\": ["
		"{ \"id\": \"b\", \"type\": \"bool\", \"label\": \"B\", \"default\": false },"
		"{ \"id\": \"i\", \"type\": \"int\", \"label\": \"I\", \"default\": 4, \"min\": 0, \"max\": 9, \"step\": 2 },"
		"{ \"id\": \"f\", \"type\": \"float\", \"label\": \"F\", \"default\": 1, \"min\": 0.5, \"max\": 2, \"step\": 0.1 },"
		"{ \"id\": \"c\", \"type\": \"choice\", \"label\": \"C\", \"default\": 2, \"choices\": [ { \"value\": 1, \"label\": \"One\" }, { \"value\": 2, \"label\": \"Two\" }, "
		"{ \"value\": \"three\", \"label\": \"Three\" } ] },"
		"{ \"id\": \"t\", \"type\": \"text\", \"label\": \"T\", \"default\": \"HI\", \"maxLength\": 6, \"charset\": \"upper\" },"
		"{ \"id\": \"d\", \"type\": \"text\", \"label\": \"D\", \"default\": \"1\", \"charset\": \"digits\" },"
		"{ \"id\": \"k\", \"type\": \"color\", \"label\": \"K\", \"default\": \"#000000\" },"
		"{ \"id\": \"a\", \"type\": \"color\", \"label\": \"A\", \"default\": \"#11223344\", \"alpha\": true } ] }");
	const ModSettingDef& b = *s.Find("b");
	const ModSettingDef& i = *s.Find("i");
	const ModSettingDef& f = *s.Find("f");
	const ModSettingDef& c = *s.Find("c");
	const ModSettingDef& t = *s.Find("t");
	const ModSettingDef& d = *s.Find("d");
	const ModSettingDef& k = *s.Find("k");
	const ModSettingDef& a = *s.Find("a");

	CheckEq(std::to_string(ModSettingStepCount(i)) + " " + std::to_string(ModSettingStepCount(f)) + " " +
		std::to_string(ModSettingStepCount(c)) + " " + std::to_string(ModSettingStepCount(b)) + " " + std::to_string(ModSettingStepCount(t)),
		"4 15 2 1 0", "step counts: int 0..8 by 2, float 0.5..2 by 0.1, three choices, a bool, text none");
	CheckEq(ModSettingStep(i, J("4"), 1).Dump(0) + " " + ModSettingStep(i, J("4"), 10).Dump(0) + " " + ModSettingStep(i, J("4"), -10).Dump(0),
		"6 8 0", "int steps stop at the range's ends (8 is the last step under 9)");
	CheckEq(ModSettingStep(f, J("1"), 1).Dump(0) + " " + ModSettingStep(f, J("1"), -10).Dump(0) + " " + ModSettingStep(f, J("1"), 100).Dump(0),
		"1.1 0.5 2", "float steps: the same doubles as the JSON text, within the range");
	Check(ModSettingStep(f, J("0.5"), 2).AsNumber() == 0.7, "... 0.5 + 2 steps is the double 0.7 (not 0.7000000000000001)");
	CheckEq(ModSettingStep(c, J("2"), 1).Dump(0) + " " + ModSettingStep(c, J("\"three\""), 1).Dump(0) + " " + ModSettingStep(c, J("1"), -1).Dump(0),
		"\"three\" 1 \"three\"", "choices go round");
	CheckEq(ModSettingStep(b, J("false"), 1).Dump(0) + " " + ModSettingStep(b, J("false"), 2).Dump(0) + " " + ModSettingStep(b, J("true"), -1).Dump(0),
		"true false false", "a bool toggles on an odd count");
	CheckEq(ModSettingStep(k, J("\"#ff0000\""), 1).Dump(0) + " " + ModSettingStep(k, J("\"#ff0000\""), -1).Dump(0), "\"#ff8000\" \"#000000\"",
		"a palette colour steps to its neighbours");
	CheckEq(ModSettingStep(k, J("\"#ff8800\""), 1).Dump(0) + " " + ModSettingStep(k, J("\"#ff8800\""), 2).Dump(0), "\"#ff8000\" \"#ffff00\"",
		"another colour steps to the nearest palette colour first");
	CheckEq(ModSettingStep(k, J("\"#ffffff\""), -1).Dump(0), "\"#ff0080\"", "the palette goes round");
	CheckEq(ModSettingStep(a, J("\"#11223344\""), 1).Dump(0), "\"#00000044\"", "with alpha the alpha stays");
	CheckEq(ModSettingStep(t, J("\"HI\""), 1).Dump(0), "\"HI\"", "text does not step");
	CheckEq(std::to_string(ModSettingStepIndex(i, J("6"))) + " " + std::to_string(ModSettingStepIndex(f, J("2"))) + " " +
		std::to_string(ModSettingStepIndex(c, J("\"three\""))), "3 15 2", "step indexes (the sliders)");
	CheckEq(ModSettingAtStep(f, 7).Dump(0) + " " + ModSettingAtStep(i, 99).Dump(0) + " " + ModSettingAtStep(c, 0).Dump(0), "1.2 8 1",
		"values at a step index, clamped");

	CheckEq(std::string(1, (char) ModSettingTypedChar(t, 'a')) + (char) ModSettingTypedChar(t, '%') + (char) ModSettingTypedChar(t, '?') +
		(char) ModSettingTypedChar(t, 0xE9), "A%?\xC9", "typing into an upper-case text makes capitals");
	Check(ModSettingTypedChar(d, 'x') == 0, "... a digit-only text takes no letter");
	CheckEq(std::string(1, (char) ModSettingTypedChar(k, 'F')) + (char) ModSettingTypedChar(k, '#'), "f#", "a colour takes hex digits (lower case) and #");
	Check(ModSettingTypedChar(k, 'g') == 0, "... and nothing else");
	Check(ModSettingTypedChar(b, 'x') == 0, "nothing is typed into a bool");
}

static void TestRelaunchQuoting()
{
	CheckEq(QuoteCommandLineArg("-gamedir=C:/Games/Bejeweled"), "-gamedir=C:/Games/Bejeweled", "an argument without spaces as it is");
	CheckEq(QuoteCommandLineArg("-userdir=C:/My Games/x"), "\"-userdir=C:/My Games/x\"", "one with a space in quotes");
	CheckEq(QuoteCommandLineArg(""), "\"\"", "an empty one as \"\"");
	CheckEq(QuoteCommandLineArg("a\"b"), "\"a\\\"b\"", "a quote escaped");
	CheckEq(QuoteCommandLineArg("C:\\dir with space\\"), "\"C:\\dir with space\\\\\"", "a trailing backslash doubled before the closing quote");
	CheckEq(QuoteCommandLineArg("a\\\\\"b c"), "\"a\\\\\\\\\\\"b c\"", "backslashes before a quote doubled, plus the quote's own");
	CheckEq(QuoteCommandLineArg("C:\\a\\b"), "C:\\a\\b", "backslashes elsewhere stay single");
}

// An edit that changes nothing leaves a hand-written file alone, empty objects in it included
static void TestNoOpBytes()
{
	gUserDataChanges = 0;
	SetUserDataChangedCallback(&OnUserDataChanged);
	Sandbox aBox("noop");
	aBox.Mod("settings-demo", FixtureManifest());
	std::string aText = "{\"v\":1,\"mods\":{\"settings-demo\":{\"minRun\":4},\"gone\":{}},\"next\":{},\"x\":0}";
	aBox.User("mod-settings.json", aText);
	ModManager& m = ModManager::Get();
	m.Refresh();
	int aChanges = gUserDataChanges;
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(4.0)), "", "setting a value it already has");
	CheckEq(m.EditSetting("settings-demo", "shape", J("\"full\"")), "", "setting a value at its default (not stored)");
	CheckEq(aBox.UserText("mod-settings.json"), aText, "... the file keeps its bytes (its empty objects too)");
	Check(gUserDataChanges == aChanges, "... nothing written");
	CheckEq(m.EditSetting("settings-demo", "minRun", JsonValue(5.0)), "", "a real change");
	JsonValue aRoot = J(aBox.UserText("mod-settings.json"));
	CheckEq(aRoot["mods"].Dump(0), "{\"settings-demo\":{\"minRun\":5},\"gone\":{}}", "... changes that mod only");
	Check(aRoot.Has("next") && aRoot.Has("x"), "... the rest stays");
	SetUserDataChangedCallback(nullptr);
}

// The web page's Remove: a mod whose folder is gone leaves mods.json ("forget")
static void TestForget()
{
	gUserDataChanges = 0;
	SetUserDataChangedCallback(&OnUserDataChanged);
	Sandbox aBox("forget");
	aBox.Mod("a", "{ \"id\": \"a\" }");
	aBox.Mod("b", "{ \"id\": \"b\" }");
	ModManager& m = ModManager::Get();
	m.ScanDesired();
	int aChanges = gUserDataChanges;
	CheckEq(m.EditForget("gone"), "", "forgetting a mod without mods.json");
	Check(!aBox.HasUser("mods.json") && (gUserDataChanges == aChanges), "... writes nothing");

	std::string aText = "{\"order\":[\"b\",\"gone\",\"a\"],\"disabled\":[\"gone\",\"a\"],\"x\":1}";
	aBox.User("mods.json", aText);
	Check(m.EditForget("a") != "", "a mod that is still installed is not forgotten");
	CheckEq(m.Edit("{\"op\":\"forget\",\"mod\":\"other\"}"), "", "forgetting an id mods.json does not have");
	CheckEq(aBox.UserText("mods.json"), aText, "... keeps its bytes");
	Check(gUserDataChanges == aChanges, "... nothing written");
	CheckEq(m.Edit("{\"op\":\"forget\",\"mod\":\"gone\"}"), "", "forgetting a removed mod");
	JsonValue aConfig = J(aBox.UserText("mods.json"));
	CheckEq(aConfig["order"].Dump(0), "[\"b\",\"a\"]", "... it leaves the order");
	CheckEq(aConfig["disabled"].Dump(0), "[\"a\"]", "... and the disabled list");
	Check(aConfig["x"].AsNumber() == 1, "... the unknown key stays");
	Check(gUserDataChanges == aChanges + 1, "... one write");

	aBox.User("mods.json", "{ broken");
	CheckEq(m.EditForget("gone"), "", "a broken mods.json");
	CheckEq(aBox.UserText("mods.json"), "{ broken", "... is left as it is");
	SetUserDataChangedCallback(nullptr);
}

#ifdef __EMSCRIPTEN__
extern "C" {
void byopop_mods_open(const char* theUserDir, const char* theModsDir);
const char* byopop_mods_state();
const char* byopop_mods_edit(const char* theJson);
void byopop_mods_rescan();
}

// The page's calls (platform/WebApi.cpp), as web/modpanel.js makes them before the game starts
static void TestWebApi()
{
	Sandbox aBox("webapi");
	aBox.Mod("settings-demo", FixtureManifest());
	aBox.Mod("plain", "{ \"id\": \"plain\", \"name\": \"Plain\", \"version\": \"2\" }");
	aBox.User("mods.json", "{\"order\":[\"plain\"],\"disabled\":[],\"keep\":true}");
	VFS::Get().SetUserDir("/nowhere");
	ModManager::Get().SetModsDir("/nowhere");
	byopop_mods_open(aBox.mUser.c_str(), aBox.mMods.c_str());
	Check(VFS::Get().GetUserDir() == aBox.mUser && ModManager::Get().GetModsDir() == aBox.mMods, "byopop_mods_open sets both folders");
	JsonValue aState = J(byopop_mods_state());
	CheckEq(std::to_string(aState["mods"].Items().size()), "2", "byopop_mods_state lists both mods");
	CheckEq(aState["mods"][(size_t) 0]["id"].AsString("") + "," + aState["mods"][(size_t) 1]["id"].AsString(""), "plain,settings-demo", "... in mods.json's order");
	Check(!aState["running"].AsBool(true), "... the game is not running");
	CheckEq(aState["mods"][(size_t) 0]["dir"].AsString(""), "plain", "... with each mod's folder");
	Check(aState["mods"][(size_t) 1]["hasSettings"].AsBool(false), "... and whether it has settings");

	CheckEq(byopop_mods_edit("{\"op\":\"enable\",\"mod\":\"plain\",\"on\":false}"), "", "byopop_mods_edit turns plain off");
	JsonValue aConfig = J(aBox.UserText("mods.json"));
	CheckEq(aConfig["disabled"].Dump(0), "[\"plain\"]", "... in mods.json");
	Check(aConfig["keep"].AsBool(false), "... which keeps its unknown key");
	Check(byopop_mods_edit("{\"op\":\"enable\",\"mod\":\"none\"}")[0] != 0, "an edit of an unknown mod says why not");
	Check(std::string(byopop_mods_edit("not json")).find("not a JSON object") == 0, "... as does text that is not JSON");

	// The page removes a folder, rescans, and forgets the mod
	RemoveTree(aBox.mMods + "/plain");
	byopop_mods_rescan();
	CheckEq(std::to_string(J(byopop_mods_state())["mods"].Items().size()), "1", "after a rescan the removed mod is gone");
	CheckEq(byopop_mods_edit("{\"op\":\"forget\",\"mod\":\"plain\"}"), "", "forget");
	aConfig = J(aBox.UserText("mods.json"));
	CheckEq(aConfig["order"].Dump(0) + aConfig["disabled"].Dump(0), "[][]", "... takes it out of mods.json");
}
#endif

int main(int argc, char** argv)
{
	std::string aTemp;
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-v"))
			gVerbose = true;
		else
			aTemp = argv[i];
	}
#ifdef __EMSCRIPTEN__
	gTemp = aTemp.empty() ? "/tmp/modsettingstest" : aTemp;
#else
	gTemp = aTemp.empty() ? "modsettingstest-tmp" : aTemp;
#endif
	RemoveTree(gTemp);
	MakeDirs(gTemp);

	TestSchema();
	TestValues();
	TestCanonical();
	TestPlaceholders();
	TestLevels();
	TestScoreKey();
	TestDiffOwner();
	TestEdits();
	TestNextGame();
	TestExamples();
	TestMenuControls();
	TestRelaunchQuoting();
	TestNoOpBytes();
	TestForget();
#ifdef __EMSCRIPTEN__
	TestWebApi();
#endif

	RemoveTree(gTemp);
	printf("modsettingstest: %d checks, %d failed\n", gChecks, gFailures);
	return gFailures == 0 ? 0 : 1;
}
