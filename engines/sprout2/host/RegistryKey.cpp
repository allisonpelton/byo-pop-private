#include "RegistryKey.h"

#include "Common.h"
#include "Engine.h"
#include "SexyAppBase.h"

namespace engine
{

namespace
{
// port: the framework's name of the application's key, with a backslash after it
std::string applicationKeyPrefix()
{
	return Sexy::RemoveTrailingSlash("SOFTWARE\\" + Sexy::gSexyAppBase->mRegKey) + "\\";
}
}

RegistryKey::RegistryKey(const std::string& thePath) : mPath(thePath)
{
	std::vector<std::string> aSubKeys;
	if (!Sexy::gSexyAppBase->RegistryGetSubKeys(mPath, &aSubKeys))
		Sexy::gSexyAppBase->RegistryWriteData(valueName(""), nullptr, 0);
}

bool RegistryKey::pathBelowApplication(const std::string& thePath, std::string& thePathBelow)
{
	const std::string& anApplicationKey = Sexy::gSexyAppBase->mRegKey;
	if (anApplicationKey.empty())
		return false;
	if (thePath.size() == anApplicationKey.size() &&
		sexy_strnicmp(thePath.c_str(), anApplicationKey.c_str(), thePath.size()) == 0)
	{
		thePathBelow = "";
		return true;
	}
	std::string aPrefix = anApplicationKey + "\\";
	if (sexy_strnicmp(thePath.c_str(), aPrefix.c_str(), aPrefix.size()) != 0)
		return false;
	thePathBelow = thePath.substr(aPrefix.size());
	return true;
}

std::string RegistryKey::valueName(const std::string& theName) const
{
	return mPath.empty() ? theName : mPath + "\\" + theName;
}

// 0x494220 (feedingfrenzy2): RegEnumKeyEx
bool RegistryKey::getSubKeyNames(std::vector<std::string>& theNames)
{
	std::vector<std::string> aSubKeys;
	Sexy::gSexyAppBase->RegistryGetSubKeys(mPath, &aSubKeys);
	theNames.insert(theNames.end(), aSubKeys.begin(), aSubKeys.end());
	return true;
}

// 0x5079a2 (feedingfrenzy2): RegDeleteKey
bool RegistryKey::deleteSubKey(const std::string& theName)
{
	return Sexy::gSexyAppBase->RegistryEraseKey(valueName(theName));
}

// 0x4187d0 (feedingfrenzy2): RegEnumValue
bool RegistryKey::getValueNames(std::vector<std::string>& theNames)
{
	std::vector<std::string> aSubKeys;
	Sexy::gSexyAppBase->RegistryGetSubKeys(mPath, &aSubKeys);  // port: reads the registry in
	std::string aPrefix = applicationKeyPrefix() + (mPath.empty() ? "" : mPath + "\\");
	const Sexy::RegistryValueMap& aRegistry = Sexy::gSexyAppBase->mRegistry;
	for (Sexy::RegistryValueMap::const_iterator anIt = aRegistry.begin(); anIt != aRegistry.end(); ++anIt)
	{
		if (sexy_strnicmp(anIt->first.c_str(), aPrefix.c_str(), aPrefix.size()) != 0)
			continue;
		std::string aName = anIt->first.substr(aPrefix.size());
		if (!aName.empty() && aName.find('\\') == std::string::npos)
			theNames.push_back(aName);
	}
	return true;
}

// 0x4767c0 (feedingfrenzy2): RegSetValueEx, REG_SZ
bool RegistryKey::writeString(const std::string& theName, const std::string& theValue)
{
	if (Sexy::gSexyAppBase->RegistryWriteString(valueName(theName), theValue))
		return true;
	getEngine()->log("Failed to set registry value %s.\n", theName.c_str());
	return false;
}

// 0x46f0a0 (feedingfrenzy2): RegQueryValueEx, a REG_SZ only
std::string RegistryKey::readString(const std::string& theName, const std::string& theDefault)
{
	std::string aValue;
	if (!Sexy::gSexyAppBase->RegistryReadString(valueName(theName), &aValue))
		return theDefault;
	return aValue;
}

} // namespace engine
