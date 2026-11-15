#ifndef __SPROUT_PERSISTKEY_H__
#define __SPROUT_PERSISTKEY_H__

// A registry key of the player's settings (0x20 bytes, constructor 0x4c00b0 (pizzafrenzy); Iggle Pop's Mac build names
// the class PersistKey). The application's slot 0x98 (0x4bccf0 (pizzafrenzy)) makes one and opens
// HKEY_CURRENT_USER\Software\<path> (0x4c05e0 (pizzafrenzy)); its slot 0x00 opens a key below one
// (0x4c0590 (pizzafrenzy)). Its values are DWORDs and strings. The port's registry is the host's, which keys its
// values under the application's own registry key already, so a key is the path below that one.

#include "Object.h"

#include <string>
#include <vector>

namespace engine
{

class PersistKey : public Object
{
public:
	// port: the path the original opens, Software\<publisher>\<game> (the host's registry already holds it)
	explicit PersistKey(const std::string& thePath) : mPath(thePath) {}
	// port: a key below the application's own, as the host names it (theRelativePath "" is the application's key)
	PersistKey(const std::string& thePath, const std::string& theRelativePath) :
		mPath(thePath), mRelativePath(theRelativePath)
	{
	}

	// 0x00: 0x4c0590 (pizzafrenzy): the key below this one by that name
	PersistKey* openSubKey(const std::string& theName);
	// 0x0c: 0x4c07f0 (pizzafrenzy): the names of the keys below this one; false when it cannot be read
	bool getSubKeyNames(std::vector<std::string>& theNames);
	// 0x10: 0x4c0130 (pizzafrenzy): the key below this one by that name deleted
	bool deleteSubKey(const std::string& theName);
	// 0x14: 0x4c0910 (pizzafrenzy): the names of the values in this key (the default value's is ""); false when it
	// cannot be read
	bool getValueNames(std::vector<std::string>& theNames);
	// 0x24: 0x4c01b0 (pizzafrenzy): a DWORD value, theDefault when it is not there or is not a DWORD
	int readInt(const std::string& theName, int theDefault);
	// 0x20: 0x4c0170 (pizzafrenzy): false when the value cannot be set
	bool writeInt(const std::string& theName, int theValue);
	// 0x1c: 0x4c06b0 (pizzafrenzy): a string value, theDefault when it is not there
	std::string readString(const std::string& theName, const std::string& theDefault);
	// 0x18: 0x4c0410 (pizzafrenzy): false when the value cannot be set
	bool writeString(const std::string& theName, const std::string& theValue);

	const std::string& getPath() const { return mPath; }

private:
	// port: the host's name of a value of this key (the default value, "", is kept under a name of its own)
	std::string valueName(const std::string& theName) const;

	std::string mPath;          // port
	std::string mRelativePath;  // port
};

} // namespace engine

#endif
