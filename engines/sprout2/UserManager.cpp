#include "UserManager.h"

#include "Engine.h"

#include <cctype>

using namespace engine;

namespace
{
// 0x497c30 (feedingfrenzy2): the names in the order of CompareStringA for the user's locale, ignoring case (negative,
// zero or positive); port: the ASCII letters compared without regard to case, so names with other characters may
// sort apart from the original's order (a deviation of the players' list)
int compareNames(const std::string& theFirst, const std::string& theSecond)
{
	size_t aLength = theFirst.size() < theSecond.size() ? theFirst.size() : theSecond.size();
	for (size_t i = 0; i < aLength; i++)
	{
		int aFirst = tolower((unsigned char)theFirst[i]);
		int aSecond = tolower((unsigned char)theSecond[i]);
		if (aFirst != aSecond)
			return aFirst < aSecond ? -1 : 1;
	}
	if (theFirst.size() == theSecond.size())
		return 0;
	return theFirst.size() < theSecond.size() ? -1 : 1;
}

// 0x4964f0 (feedingfrenzy2): _stricmp of the names is 0
bool sameName(const std::string& theFirst, const std::string& theSecond)
{
	if (theFirst.size() != theSecond.size())
		return false;
	for (size_t i = 0; i < theFirst.size(); i++)
	{
		if (tolower((unsigned char)theFirst[i]) != tolower((unsigned char)theSecond[i]))
			return false;
	}
	return true;
}

UserManager* gUserManager = nullptr;  // 0x5ac62c (feedingfrenzy2)
// 0x5ad1bc (feedingfrenzy2), set from 0x55d570 (feedingfrenzy2): the players' key's value that names the current one
const char* const ACTIVE_USER_VALUE = "activeUser";
}

UserManager::UserManager() : mCurrentIndex(0)
{
}

UserManager* UserManager::instance()
{
	if (gUserManager == nullptr)
		gUserManager = new UserManager();
	return gUserManager;
}

void UserManager::destroy()
{
	if (gUserManager != nullptr)
	{
		delete gUserManager;
		gUserManager = nullptr;
	}
}

void UserManager::open(const std::string& theAppName, const std::string& theVendor)
{
	std::string aPath = theVendor + '\\' + theAppName + "\\users";
	mKey = getEngine()->openPersistKey(1, aPath);
	if (mKey != nullptr)
		load();
}

void UserManager::load()
{
	mUsers.clear();
	std::vector<std::string> aNames;
	if (!mKey->getSubKeyNames(aNames))
		return;
	for (size_t i = 0; i < aNames.size(); i++)
		loadUser(aNames[i]);
	int aCurrent = findUser(mKey->readString(ACTIVE_USER_VALUE, ""));
	if (aCurrent >= 0 && aCurrent < getUserCount())
		mCurrentIndex = aCurrent;
}

void UserManager::loadUser(const std::string& theName)
{
	Ref<PersistKey> aKey = getEngine()->openPersistKey(mKey.get(), theName);
	if (aKey == nullptr)
		return;
	Ref<UserInfo> aUser = new UserInfo();
	aUser->setName(theName);
	std::vector<std::string> aValueNames;
	if (aKey->getValueNames(aValueNames))
	{
		for (size_t i = 0; i < aValueNames.size(); i++)
			aUser->getParams().set(aValueNames[i], aKey->readString(aValueNames[i], ""));
	}
	size_t aLower = lowerBound(theName);
	if (aLower != upperBound(theName))
		return;
	mUsers.insert(mUsers.begin() + aLower, aUser);
	mCurrentIndex = (int)aLower;
}

size_t UserManager::lowerBound(const std::string& theName)
{
	size_t aFirst = 0;
	size_t aCount = mUsers.size();
	while (aCount > 0)
	{
		size_t aHalf = aCount / 2;
		if (compareNames(mUsers[aFirst + aHalf]->getName(), theName) < 0)
		{
			aFirst += aHalf + 1;
			aCount -= aHalf + 1;
		}
		else
			aCount = aHalf;
	}
	return aFirst;
}

size_t UserManager::upperBound(const std::string& theName)
{
	size_t aFirst = 0;
	size_t aCount = mUsers.size();
	while (aCount > 0)
	{
		size_t aHalf = aCount / 2;
		if (!(compareNames(theName, mUsers[aFirst + aHalf]->getName()) < 0))
		{
			aFirst += aHalf + 1;
			aCount -= aHalf + 1;
		}
		else
			aCount = aHalf;
	}
	return aFirst;
}

UserInfo* UserManager::getUser(int theIndex)
{
	if (theIndex < 0 || theIndex >= getUserCount())
		return nullptr;
	return mUsers[theIndex].get();
}

int UserManager::findUser(const std::string& theName)
{
	for (size_t i = 0; i < mUsers.size(); i++)
	{
		if (sameName(theName, mUsers[i]->getName()))
			return (int)i;
	}
	return -1;
}

void UserManager::setCurrentUser(int theIndex)
{
	if (theIndex < 0 || theIndex >= getUserCount())
		return;
	mCurrentIndex = theIndex;
	mKey->writeString(ACTIVE_USER_VALUE, mUsers[theIndex]->getName());
	mCurrentUserSignal.emit();
}

bool UserManager::addUser(UserInfo* theUser)
{
	Ref<UserInfo> aUser = theUser;
	size_t aLower = lowerBound(theUser->getName());
	if (aLower != upperBound(theUser->getName()))
		return false;
	mUsers.insert(mUsers.begin() + aLower, aUser);
	if (!saveUser(theUser))
	{
		mUsers.erase(mUsers.begin() + aLower);
		return false;
	}
	setCurrentUser((int)aLower);
	mCurrentUserSignal.emit();
	return true;
}

bool UserManager::saveUser(UserInfo* theUser)
{
	Ref<PersistKey> aKey = getEngine()->openPersistKey(mKey.get(), theUser->getName());
	if (aKey == nullptr)
		return false;
	const std::map<std::string, std::string>& aValues = theUser->getParams().getValues();
	for (std::map<std::string, std::string>::const_iterator anIt = aValues.begin(); anIt != aValues.end(); ++anIt)
	{
		if (!aKey->writeString(anIt->first, anIt->second))
			return false;
	}
	return true;
}

bool UserManager::removeUser(int theIndex)
{
	if (theIndex < 0 || theIndex >= getUserCount())
		return false;
	Ref<UserInfo> aUser = mUsers[theIndex];
	if (!mKey->deleteSubKey(aUser->getName()))
		return false;
	mUsers.erase(mUsers.begin() + theIndex);
	if (mCurrentIndex > theIndex)
		setCurrentUser(mCurrentIndex - 1);
	else if (mCurrentIndex == getUserCount())
		// The last player was the current one: the one before, or none
		setCurrentUser(getUserCount() - 1 > 0 ? getUserCount() - 1 : 0);
	mCurrentUserSignal.emit();
	return true;
}
