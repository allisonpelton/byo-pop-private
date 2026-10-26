// Sexy::Profile (0x6c bytes, no vtable, constructor 0x487ba0; the app's +0x950): a player's profile, saved as
// users/<name>.bwa in the user folder. It is a store of named values: the scripts read and write it through the
// Lua functions profile.Get, profile.Set and the others (RegisterProfileFunctions), and the C++ code reads a few
// of them (the current book, chapter and stage, NeedsIntroTileHelp, ...). A value is an int, a float, a bool, a
// string or a table: a list of entries with string or int keys, as a Lua table converts to one.
#pragma once

#include <map>
#include <string>
#include <vector>

struct lua_State;

namespace Sexy {

class Buffer;

class Profile {
public:
	// The value types, as the file stores them
	enum ValueType {
		TYPE_INT = 0,
		TYPE_FLOAT = 1,  // saved as text ("%f")
		TYPE_BOOL = 2,
		TYPE_STRING = 3,
		TYPE_TABLE = 4,
		TYPE_NONE = 5,   // the one entry of an empty table
	};

	struct Entry;
	struct Value {
		ValueType mType = TYPE_NONE;
		int mInt = 0;
		float mFloat = 0.0f;
		bool mBool = false;
		std::string mString;
		std::vector<Entry> mTable;  // the original's linked list of 0x240c-byte entries
	};
	struct Entry {
		bool mStringKey = false;
		int mIntKey = 0;
		std::string mKey;
		Value mValue;
	};

	// A change of the profile format (0x28 bytes): a profile saved by a version before it is out of date (the
	// list of the original's +0x24, the reasons shown to the player)
	struct FormatChange {
		int mMajor, mMinor1, mMinor2;
		unsigned int mReason;  // the text's address in the exe (GameText)
	};

	std::map<std::string, Value> mValues;  // +0x04
	std::string mName;                     // +0x34
	std::string mOutOfDateReasons;         // +0x50: set by Load when the file is from an older format
	bool mNoSave;                          // +0x68: a profile that is never saved (the intro's, before a name)

	// 0x487ba0: the defaults of a new player
	Profile(const std::string& theName, bool theNoSave);

	bool KeyExists(const std::string& theKey) const;  // 0x489a50
	const Value* GetValue(const std::string& theKey) const;  // NULL when there is none (0x4898f0)
	int GetInt(const std::string& theKey) const;       // 0x48b2a0: a float truncates, others are 0
	float GetFloat(const std::string& theKey) const;   // 0x48b420: an int converts, others are 0
	bool GetBool(const std::string& theKey) const;     // 0x48b210
	std::string GetString(const std::string& theKey) const;  // 0x48b5a0: empty when it is not a string
	void SetInt(const std::string& theKey, int theValue);          // 0x488a30
	void SetFloat(const std::string& theKey, float theValue);      // 0x488b50
	void SetBool(const std::string& theKey, bool theValue);        // 0x488c80
	void SetString(const std::string& theKey, const std::string& theValue);  // 0x488db0
	void SetTable(const std::string& theKey, lua_State* L, int theIndex);    // 0x4892d0: a Lua table's entries
	void Delete(const std::string& theKey);            // 0x48c030
	// 0x48d130: a new timed-mode run: its TM_ keys back to their starts and those of the last run's progress deleted;
	// the scripts are told (main.NewTimedModeStarted)
	void ResetTimedMode();

	// 0x489f10: users/<name>.bwa (and the registry's LastUser), unless mNoSave. False when it could not write.
	bool Save();
	// 0x48a390: reads users/<name>.bwa in place of the values. False when there is no file, or when it is from an
	// older format: that sets mOutOfDateReasons (the caller resets the profile). A current one gets the keys added since.
	bool Load();

private:
	void Set(const std::string& theKey, const Value& theValue);
};

// 0x48cec0 (part of LuaInterface::RegisterSexyFunctions): profile.GetName, Set, GetTableSize, Save, KeyExists,
// Get and Delete, on the app's current profile
void RegisterProfileFunctions();

}  // namespace Sexy
