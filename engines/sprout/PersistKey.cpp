#include "PersistKey.h"
#include "Host.h"

using namespace engine;

namespace
{
// port: the host's name of the default value of a key, which the registry's own would give no name
const char* const DEFAULT_VALUE_NAME = "(Default)";
}

// port: the value's name below the application's registry key
std::string PersistKey::valueName(const std::string& theName) const
{
	return mRelativePath + (theName.empty() ? DEFAULT_VALUE_NAME : theName);
}

// 0x4c0590 (pizzafrenzy)
PersistKey* PersistKey::openSubKey(const std::string& theName)
{
	return new PersistKey(mPath + "\\" + theName, mRelativePath + theName + "\\");
}

// 0x4c0130 (pizzafrenzy)
bool PersistKey::deleteSubKey(const std::string& theName)
{
	return getHost()->registryDeleteKey(mRelativePath + theName);
}

// 0x4c07f0 (pizzafrenzy)
bool PersistKey::getSubKeyNames(std::vector<std::string>& theNames)
{
	std::string aKey = mRelativePath.substr(0, mRelativePath.empty() ? 0 : mRelativePath.size() - 1);
	return getHost()->registryGetSubKeys(aKey, theNames);
}

// 0x4c0910 (pizzafrenzy)
bool PersistKey::getValueNames(std::vector<std::string>& theNames)
{
	std::string aKey = mRelativePath.substr(0, mRelativePath.empty() ? 0 : mRelativePath.size() - 1);
	if (!getHost()->registryGetValueNames(aKey, theNames))
		return false;
	for (size_t i = 0; i < theNames.size(); i++)
	{
		if (theNames[i] == DEFAULT_VALUE_NAME)
			theNames[i] = "";
	}
	return true;
}

// 0x4c01b0 (pizzafrenzy)
int PersistKey::readInt(const std::string& theName, int theDefault)
{
	int aValue;
	if (!getHost()->registryReadInt(valueName(theName), aValue))
		return theDefault;
	return aValue;
}

// 0x4c0170 (pizzafrenzy)
bool PersistKey::writeInt(const std::string& theName, int theValue)
{
	return getHost()->registryWriteInt(valueName(theName), theValue);
}

// 0x4c06b0 (pizzafrenzy)
std::string PersistKey::readString(const std::string& theName, const std::string& theDefault)
{
	std::string aValue;
	if (!getHost()->registryReadString(valueName(theName), aValue))
		return theDefault;
	return aValue;
}

// 0x4c0410 (pizzafrenzy)
bool PersistKey::writeString(const std::string& theName, const std::string& theValue)
{
	return getHost()->registryWriteString(valueName(theName), theValue);
}
