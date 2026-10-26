#include "Profile.h"

#include <cstdlib>

#include "Buffer.h"
#include "Common.h"
#include "GameApp.h"
#include "GameText.h"
#include "LuaInterface.h"
#include "lauxlib.h"
#include "lua.h"

using namespace Sexy;

// The profile format's version (the default VersionMajor, VersionMinor1 and VersionMinor2)
static const int kVersionMajor = 0;
static const int kVersionMinor1 = 1;
static const int kVersionMinor2 = 5;

// The changes of the format, oldest first (the constructor's list, +0x24)
static const Profile::FormatChange kFormatChanges[] = {
	{0, 0, 2, GTA(0x7b01a8)}, {0, 0, 3, GTA(0x7b01d4)}, {0, 0, 4, GTA(0x7b0210)}, {0, 0, 5, GTA(0x7b0258)},
	{0, 0, 6, GTA(0x7b02a8)}, {0, 0, 7, GTA(0x7b0300)}, {0, 0, 8, GTA(0x7b0330)}, {0, 0, 9, GTA(0x7b0360)},
	{0, 1, 0, GTA(0x7b03b4)}, {0, 1, 1, GTA(0x7b03d8)}, {0, 1, 2, GTA(0x7b0410)}, {0, 1, 3, GTA(0x7b042c)},
	{0, 1, 4, GTA(0x7b0444)}, {0, 1, 5, GTA(0x7b046c)},
};

// Keys a file's values never set (the constructor's list at +0x10; they had versions, which nothing compared)
static const char* const kNotLoaded[] = {"Gold", "NeedsScrambleHint"};

static std::string ProfilePath(const std::string& theName)
{
	return GetAppDataFolder() + "users/" + theName + ".bwa";
}

Profile::Profile(const std::string& theName, bool theNoSave)
	: mName(theName), mNoSave(theNoSave)
{
	SetInt("CurrentBook", 1);
	SetInt("CurrentChapter", 0);
	SetInt("CurrentStage", 0);
	SetInt("VersionMajor", kVersionMajor);
	SetInt("VersionMinor1", kVersionMinor1);
	SetInt("VersionMinor2", kVersionMinor2);
	SetInt("Health", 3);
	SetInt("MaxHealth", 3);
	SetInt("XP", 0);
	SetInt("XPLevel", 0);
	SetFloat("XPPercent", 0.0f);
	SetBool("InGame", false);
	SetFloat("OffenseBonusPct", 0.0f);
	SetBool("NeedsIntroTileHelp", true);
	SetBool("EnablePips", true);
	SetInt("TM_XP", 0);
	SetInt("TM_XPLevel", 0);
	SetFloat("TM_XPPercent", 0.0f);
	SetInt("TM_Health", 4);
	SetInt("TM_MaxHealth", 4);
	SetBool("TM_InGame", false);
	SetFloat("TM_OffenseBonusPct", 0.0f);
	SetBool("AllowGoldBar", false);
	SetBool("AllowXPBar", false);
	SetBool("AllowHealthBar", true);
	SetBool("AllowItems", false);
	SetBool("AllowGems", false);
	SetBool("HideLore", true);
	SetBool("HideTreasures", true);
	SetString("LastAppVersion", gApp->mProductVersion);
	for (char aLetter = 'A'; aLetter <= 'Z'; aLetter++)
		SetInt(StrFormat("NumLetter%c", aLetter), 0);
}

bool Profile::KeyExists(const std::string& theKey) const
{
	// port: the original also checked, when asked to, that a key with a TM_ twin was asked for in the right mode
	// (TM_ keys in timed mode, the others outside it), and showed the scripts' error; that is left out
	return mValues.find(theKey) != mValues.end();
}

const Profile::Value* Profile::GetValue(const std::string& theKey) const
{
	auto it = mValues.find(theKey);
	return it == mValues.end() ? nullptr : &it->second;
}

int Profile::GetInt(const std::string& theKey) const
{
	const Value* aValue = GetValue(theKey);
	if (aValue == nullptr)
		return 0;
	if (aValue->mType == TYPE_INT)
		return aValue->mInt;
	if (aValue->mType == TYPE_FLOAT)
		return (int)aValue->mFloat;
	return 0;
}

bool Profile::GetBool(const std::string& theKey) const
{
	const Value* aValue = GetValue(theKey);
	return aValue != nullptr && aValue->mType == TYPE_BOOL && aValue->mBool;
}

void Profile::Set(const std::string& theKey, const Value& theValue)
{
	mValues[theKey] = theValue;
}

void Profile::SetInt(const std::string& theKey, int theValue)
{
	Value aValue;
	aValue.mType = TYPE_INT;
	aValue.mInt = theValue;
	Set(theKey, aValue);
}

void Profile::SetFloat(const std::string& theKey, float theValue)
{
	Value aValue;
	aValue.mType = TYPE_FLOAT;
	aValue.mFloat = theValue;
	Set(theKey, aValue);
}

void Profile::SetBool(const std::string& theKey, bool theValue)
{
	Value aValue;
	aValue.mType = TYPE_BOOL;
	aValue.mBool = theValue;
	Set(theKey, aValue);
}

void Profile::SetString(const std::string& theKey, const std::string& theValue)
{
	Value aValue;
	aValue.mType = TYPE_STRING;
	aValue.mString = theValue;
	Set(theKey, aValue);
}

// 0x488f40: a Lua table's entries, in lua_next's order; an empty table keeps one entry of TYPE_NONE
static void TableFromLua(std::vector<Profile::Entry>& theTable, lua_State* L, int theIndex)
{
	if (theIndex < 0)
		theIndex = lua_gettop(L) + theIndex + 1;
	theTable.clear();
	lua_pushnil(L);
	while (lua_next(L, theIndex) != 0) {
		Profile::Entry anEntry;
		if (lua_type(L, -2) == LUA_TSTRING) {
			anEntry.mStringKey = true;
			anEntry.mKey = lua_tostring(L, -2);
		} else {
			anEntry.mIntKey = lua_toint(L, -2);
		}
		Profile::Value& aValue = anEntry.mValue;
		switch (lua_type(L, -1)) {
		case LUA_TINT:
			aValue.mType = Profile::TYPE_INT;
			aValue.mInt = lua_toint(L, -1);
			break;
		case LUA_TNUMBER:
			aValue.mType = Profile::TYPE_FLOAT;
			aValue.mFloat = (float)lua_tonumber(L, -1);
			break;
		case LUA_TBOOLEAN:
			aValue.mType = Profile::TYPE_BOOL;
			aValue.mBool = lua_toboolean(L, -1) != 0;
			break;
		case LUA_TSTRING:
			aValue.mType = Profile::TYPE_STRING;
			aValue.mString = lua_tostring(L, -1);
			break;
		case LUA_TTABLE:
			aValue.mType = Profile::TYPE_TABLE;
			TableFromLua(aValue.mTable, L, -1);
			break;
		default:
			break;  // TYPE_NONE: functions and the like are not kept
		}
		theTable.push_back(anEntry);
		lua_pop(L, 1);
	}
	if (theTable.empty())
		theTable.push_back(Profile::Entry());
}

void Profile::SetTable(const std::string& theKey, lua_State* L, int theIndex)
{
	Value aValue;
	aValue.mType = TYPE_TABLE;
	TableFromLua(aValue.mTable, L, theIndex);
	Set(theKey, aValue);
}

void Profile::Delete(const std::string& theKey)
{
	mValues.erase(theKey);
}

// ---- the file: per value its key, its type and the value; a table's entries each say whether one follows ----

static void WriteValue(Buffer& theBuffer, const Profile::Value& theValue);

static void WriteTable(Buffer& theBuffer, const std::vector<Profile::Entry>& theTable)
{
	for (size_t i = 0; i < theTable.size(); i++) {
		const Profile::Entry& anEntry = theTable[i];
		theBuffer.WriteBoolean(i + 1 < theTable.size());
		theBuffer.WriteBoolean(anEntry.mStringKey);
		if (anEntry.mStringKey)
			theBuffer.WriteString(anEntry.mKey);
		else
			theBuffer.WriteLong(anEntry.mIntKey);
		WriteValue(theBuffer, anEntry.mValue);
	}
}

static void WriteValue(Buffer& theBuffer, const Profile::Value& theValue)
{
	theBuffer.WriteLong(theValue.mType);
	switch (theValue.mType) {
	case Profile::TYPE_INT:
		theBuffer.WriteLong(theValue.mInt);
		break;
	case Profile::TYPE_FLOAT:
		theBuffer.WriteString(StrFormat("%f", (double)theValue.mFloat));
		break;
	case Profile::TYPE_BOOL:
		theBuffer.WriteBoolean(theValue.mBool);
		break;
	case Profile::TYPE_STRING:
		theBuffer.WriteString(theValue.mString);
		break;
	case Profile::TYPE_TABLE:
		WriteTable(theBuffer, theValue.mTable);
		break;
	default:
		break;
	}
}

static void ReadValue(const Buffer& theBuffer, Profile::Value& theValue);

// 0x48aff0
static void ReadTable(const Buffer& theBuffer, std::vector<Profile::Entry>& theTable)
{
	bool aHasNext;
	do {
		Profile::Entry anEntry;
		aHasNext = theBuffer.ReadBoolean();
		anEntry.mStringKey = theBuffer.ReadBoolean();
		if (anEntry.mStringKey)
			anEntry.mKey = theBuffer.ReadString();
		else
			anEntry.mIntKey = theBuffer.ReadLong();
		ReadValue(theBuffer, anEntry.mValue);
		theTable.push_back(anEntry);
	} while (aHasNext && !theBuffer.AtEnd());
}

static void ReadValue(const Buffer& theBuffer, Profile::Value& theValue)
{
	theValue.mType = (Profile::ValueType)theBuffer.ReadLong();
	switch (theValue.mType) {
	case Profile::TYPE_INT:
		theValue.mInt = theBuffer.ReadLong();
		break;
	case Profile::TYPE_FLOAT:
		theValue.mFloat = (float)atof(theBuffer.ReadString().c_str());
		break;
	case Profile::TYPE_BOOL:
		theValue.mBool = theBuffer.ReadBoolean();
		break;
	case Profile::TYPE_STRING:
		theValue.mString = theBuffer.ReadString();
		break;
	case Profile::TYPE_TABLE:
		ReadTable(theBuffer, theValue.mTable);
		break;
	default:
		break;
	}
}

bool Profile::Save()
{
	if (mNoSave)
		return true;
	gApp->RegistryWriteString("LastUser", mName);
	Buffer aBuffer;
	for (const auto& aPair : mValues) {
		aBuffer.WriteString(aPair.first);
		WriteValue(aBuffer, aPair.second);
	}
	return gApp->WriteBufferToFile(ProfilePath(mName), &aBuffer);
}

bool Profile::Load()
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(ProfilePath(mName), &aBuffer))
		return false;
	mValues.clear();
	while (!aBuffer.AtEnd()) {
		std::string aKey = aBuffer.ReadString();
		Value aValue;
		ReadValue(aBuffer, aValue);
		bool aLoad = true;
		for (const char* aSkipped : kNotLoaded)
			if (aKey == aSkipped)
				aLoad = false;
		// port: the original then called the cheat console's _debug_profile_loaded_value (cheats are off)
		if (aLoad)
			mValues[aKey] = aValue;
	}

	int aMajor = GetInt("VersionMajor");
	int aMinor1 = GetInt("VersionMinor1");
	int aMinor2 = GetInt("VersionMinor2");
	mOutOfDateReasons.clear();
	for (const FormatChange& aChange : kFormatChanges) {
		if (aMajor > aChange.mMajor || (aMajor == aChange.mMajor && (aMinor1 > aChange.mMinor1 ||
				(aMinor1 == aChange.mMinor1 && aMinor2 >= aChange.mMinor2))))
			continue;
		// "As of <version>: <reason>", one per line, in red and black
		std::string aFormat = mOutOfDateReasons.empty() ? GT(0x7b054c) : GT(0x7b0570);
		mOutOfDateReasons += StrFormat(aFormat.c_str(), aChange.mMajor, aChange.mMinor1, aChange.mMinor2,
		                               GT(aChange.mReason).c_str());
	}
	if (!mOutOfDateReasons.empty())
		return false;

	SetInt("VersionMajor", kVersionMajor);
	SetInt("VersionMinor1", kVersionMinor1);
	SetInt("VersionMinor2", kVersionMinor2);
	// the timed mode's keys came later than some profiles
	if (!KeyExists("TM_XP"))
		SetInt("TM_XP", 0);
	if (!KeyExists("TM_XPLevel"))
		SetInt("TM_XPLevel", 0);
	if (!KeyExists("TM_XPPercent"))
		SetFloat("TM_XPPercent", 0.0f);
	if (!KeyExists("TM_Health"))
		SetInt("TM_Health", 4);
	if (!KeyExists("TM_MaxHealth"))
		SetInt("TM_MaxHealth", 4);
	if (!KeyExists("TM_InGame"))
		SetBool("TM_InGame", false);
	if (!KeyExists("TM_OffenseBonusPct"))
		SetFloat("TM_OffenseBonusPct", 0.0f);
	// NeedsXPBar became AllowXPBar
	if (KeyExists("NeedsXPBar") && GetBool("NeedsXPBar")) {
		SetBool("AllowXPBar", true);
		Delete("NeedsXPBar");
	}
	// port: the original then called the cheat console's _debug_profile_done_loading
	return true;
}

// ---- the Lua functions ----

static Profile* CurrentProfile(lua_State* L)
{
	if (gApp->mProfile == nullptr)
		luaL_error(L, "There is no profile");  // port: the original used the missing profile
	return gApp->mProfile;
}

static std::string KeyArg(lua_State* L)
{
	const char* aKey = lua_tostring(L, 1);
	return aKey != nullptr ? aKey : "";
}

// profile.GetName(): the player's name, "Lex" without one (0x48b820)
static int ProfileGetName(lua_State* L)
{
	std::string aName = gApp->mProfile != nullptr ? gApp->mProfile->mName : "";
	if (aName.empty())
		aName = GT(0x7ac5d4);
	lua_pushstring(L, aName.c_str());
	return 1;
}

// profile.Set(key, value [, check]): by the value's Lua type; nil deletes the key (0x48b950)
static int ProfileSet(lua_State* L)
{
	Profile* aProfile = CurrentProfile(L);
	std::string aKey = KeyArg(L);
	switch (lua_type(L, 2)) {
	case LUA_TINT:
		aProfile->SetInt(aKey, lua_toint(L, 2));
		break;
	case LUA_TNUMBER:
		aProfile->SetFloat(aKey, (float)lua_tonumber(L, 2));
		break;
	case LUA_TSTRING:
		aProfile->SetString(aKey, lua_tostring(L, 2));
		break;
	case LUA_TBOOLEAN:
		aProfile->SetBool(aKey, lua_toboolean(L, 2) != 0);
		break;
	case LUA_TTABLE:
		aProfile->SetTable(aKey, L, 2);
		break;
	case LUA_TNIL:
		aProfile->Delete(aKey);
		break;
	default:
		break;
	}
	return 0;
}

// profile.GetTableSize(key [, check]): the number of entries (an empty table has one) and true, or 0 and false
// when there is no such key, or (checking) it is not a table (0x48c5a0)
static int ProfileGetTableSize(lua_State* L)
{
	Profile* aProfile = CurrentProfile(L);
	bool aCheck = lua_gettop(L) != 2 || lua_toboolean(L, 2);
	const Profile::Value* aValue = aProfile->GetValue(KeyArg(L));
	if (aValue == nullptr || (aCheck && aValue->mType != Profile::TYPE_TABLE)) {
		lua_pushint(L, 0);
		lua_pushboolean(L, false);
		return 2;
	}
	lua_pushint(L, aValue->mType == Profile::TYPE_TABLE ? (int)aValue->mTable.size() : 0);
	lua_pushboolean(L, true);
	return 2;
}

// profile.Save(): an error the player is told about (0x48cb30)
static int ProfileSave(lua_State* L)
{
	Profile* aProfile = CurrentProfile(L);
	if (!aProfile->Save())
		gApp->Popup(GT(0x7b05bc) + GetAppDataFolder() + "users/" + aProfile->mName + ".bwa");
	return 0;
}

// profile.KeyExists(key [, check]) (0x48cc20)
static int ProfileKeyExists(lua_State* L)
{
	lua_pushboolean(L, CurrentProfile(L)->KeyExists(KeyArg(L)));
	return 1;
}

static void PushValue(lua_State* L, const Profile::Value& theValue);

// 0x48c820: a table value as a Lua table (the empty table's entry is not one)
static void PushTable(lua_State* L, const std::vector<Profile::Entry>& theTable)
{
	lua_newtable(L);
	for (const Profile::Entry& anEntry : theTable) {
		if (anEntry.mValue.mType == Profile::TYPE_NONE)
			continue;
		if (anEntry.mStringKey)
			lua_pushstring(L, anEntry.mKey.c_str());
		else
			lua_pushint(L, anEntry.mIntKey);
		PushValue(L, anEntry.mValue);
		lua_settable(L, -3);
	}
}

static void PushValue(lua_State* L, const Profile::Value& theValue)
{
	switch (theValue.mType) {
	case Profile::TYPE_INT:
		lua_pushint(L, theValue.mInt);
		break;
	case Profile::TYPE_FLOAT:
		lua_pushnumber(L, theValue.mFloat);
		break;
	case Profile::TYPE_BOOL:
		lua_pushboolean(L, theValue.mBool);
		break;
	case Profile::TYPE_STRING:
		lua_pushstring(L, theValue.mString.c_str());
		break;
	case Profile::TYPE_TABLE:
		PushTable(L, theValue.mTable);
		break;
	default:
		lua_pushnil(L);
		break;
	}
}

// profile.Get(key [, check]): the value, nil when there is none (0x48c0f0)
static int ProfileGet(lua_State* L)
{
	const Profile::Value* aValue = CurrentProfile(L)->GetValue(KeyArg(L));
	if (aValue == nullptr)
		lua_pushnil(L);
	else
		PushValue(L, *aValue);
	return 1;
}

// profile.Delete(key) (0x48bf10)
static int ProfileDelete(lua_State* L)
{
	CurrentProfile(L)->Delete(KeyArg(L));
	return 0;
}

void Sexy::RegisterProfileFunctions()
{
	gLuaInterface->RegisterFunction("profile.GetName", ProfileGetName);
	gLuaInterface->RegisterFunction("profile.Set", ProfileSet);
	gLuaInterface->RegisterFunction("profile.GetTableSize", ProfileGetTableSize);
	gLuaInterface->RegisterFunction("profile.Save", ProfileSave);
	gLuaInterface->RegisterFunction("profile.KeyExists", ProfileKeyExists);
	gLuaInterface->RegisterFunction("profile.Get", ProfileGet);
	gLuaInterface->RegisterFunction("profile.Delete", ProfileDelete);
}
