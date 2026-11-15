#include "Players.h"

#include <cctype>

using namespace engine;

// 0x469090 (pizzafrenzy)
char engine::filterPlayerNameChar(char theChar)
{
	if ((theChar < 'a' || 'z' < theChar) && (theChar < 'A' || 'Z' < theChar) && (theChar < '0' || '9' < theChar) &&
		theChar != ' ' && theChar != '-' && theChar != '.' && ((int)theChar < 0xc0 || 0xff < (int)theChar))
		return '\0';
	return theChar;
}

namespace
{
// 0x46da80 (pizzafrenzy): the names compared without regard to case (the exe asks CompareStringA for the user's
// locale; the port compares the ASCII letters)
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

Players* gPlayers = nullptr;  // 0x532f6c (pizzafrenzy)
// 0x521228 (pizzafrenzy): the value of the players' key that names the current player
const char* const ACTIVE_USER_VALUE = "activeUser";
}

// 0x46a700 (pizzafrenzy)
Players* Players::instance()
{
	if (gPlayers == nullptr)
		gPlayers = new Players();
	return gPlayers;
}

// 0x46acb0 (pizzafrenzy): the key is opened by the application's slot 0x98 (0x4bccf0 (pizzafrenzy)); the port's
// registry is the host's, which keys it under the application's own key (see PersistKey.h), so the key is "users"
bool Players::open(const std::string& thePublisher, const std::string& theGame)
{
	std::string aPath = thePublisher + "\\" + theGame + "\\users";
	mKey = new PersistKey(aPath, "users\\");
	return load();
}

// 0x46aae0 (pizzafrenzy): each key below the players' is a player; the value "activeUser" of the players' key names
// the current one
bool Players::load()
{
	std::vector<std::string> aNames;
	if (!mKey->getSubKeyNames(aNames))
		return false;
	for (size_t i = 0; i < aNames.size(); i++)
		loadUser(aNames[i]);
	std::string aCurrentName = mKey->readString(ACTIVE_USER_VALUE, "");
	size_t aFound = 0;  // 0x46a280 (pizzafrenzy): the player of that name
	while (aFound < mUsers.size() && compareNames(mUsers[aFound]->getName(), aCurrentName) != 0)
		aFound++;
	if (aFound != mUsers.size())
		mCurrentIndex = (int)aFound;
	return true;
}

// 0x46a770 (pizzafrenzy): the player is put in the order of the names, and becomes the current one; a name the players
// have already is not added
bool Players::loadUser(const std::string& theName)
{
	Ref<PersistKey> aKey = mKey->openSubKey(theName);
	Ref<UserInfo> aUser = new UserInfo();
	aUser->setName(theName);
	std::vector<std::string> aValueNames;
	if (aKey->getValueNames(aValueNames))
	{
		for (size_t i = 0; i < aValueNames.size(); i++)
			aUser->getParams().setString(aValueNames[i], aKey->readString(aValueNames[i], ""));
	}
	size_t aLower = 0;  // 0x46a0d0 (pizzafrenzy)
	while (aLower < mUsers.size() && compareNames(mUsers[aLower]->getName(), theName) < 0)
		aLower++;
	size_t anUpper = aLower;  // 0x46a130 (pizzafrenzy)
	while (anUpper < mUsers.size() && compareNames(mUsers[anUpper]->getName(), theName) <= 0)
		anUpper++;
	if (aLower != anUpper)
		return false;
	mUsers.insert(mUsers.begin() + aLower, aUser);
	mCurrentIndex = (int)aLower;
	return true;
}

// 0x469f40 (pizzafrenzy)
UserInfo* Players::current()
{
	if (mCurrentIndex < 0 || mCurrentIndex >= (int)mUsers.size())
		return nullptr;
	return mUsers[mCurrentIndex];
}

// 0x469f00 (pizzafrenzy)
UserInfo* Players::getUser(int theIndex)
{
	if (theIndex < 0 || theIndex >= (int)mUsers.size())
		return nullptr;
	return mUsers[theIndex];
}

// 0x469eb0 (pizzafrenzy)
void Players::choose(int theIndex)
{
	if (theIndex == mCurrentIndex || theIndex < 0 || theIndex >= (int)mUsers.size())
		return;
	mCurrentIndex = theIndex;
	mKey->writeString(ACTIVE_USER_VALUE, mUsers[theIndex]->getName());
	emit();  // 0x469e80 (pizzafrenzy)
}

// 0x46a600 (pizzafrenzy)
bool Players::add(UserInfo* theUser)
{
	const std::string& aName = theUser->getName();
	size_t aLower = 0;  // 0x46a0d0 (pizzafrenzy)
	while (aLower < mUsers.size() && compareNames(mUsers[aLower]->getName(), aName) < 0)
		aLower++;
	size_t anUpper = aLower;  // 0x46a130 (pizzafrenzy)
	while (anUpper < mUsers.size() && compareNames(mUsers[anUpper]->getName(), aName) <= 0)
		anUpper++;
	if (aLower != anUpper)
		return false;
	mUsers.insert(mUsers.begin() + aLower, Ref<UserInfo>(theUser));
	if (!saveUser(theUser))
	{
		mUsers.erase(mUsers.begin() + aLower);
		return false;
	}
	mCurrentIndex = (int)aLower;
	emit();
	return true;
}

// 0x46a190 (pizzafrenzy): when the player removed is before the current one, the current one is the one before; when
// the current one was the last, the one before it
bool Players::remove(int theIndex)
{
	if (theIndex < 0 || theIndex >= (int)mUsers.size())
		return false;
	if (!mKey->deleteSubKey(mUsers[theIndex]->getName()))
		return false;
	mUsers.erase(mUsers.begin() + theIndex);
	if (theIndex < mCurrentIndex)
		choose(mCurrentIndex - 1);
	else if (mCurrentIndex == (int)mUsers.size())
	{
		int aLast = (int)mUsers.size() - 1;
		choose(aLast > 0 ? aLast : 0);
	}
	emit();
	return true;
}

// 0x469f70 (pizzafrenzy)
bool Players::saveUser(UserInfo* theUser)
{
	Ref<PersistKey> aKey = mKey->openSubKey(theUser->getName());
	Params& aParams = theUser->getParams();
	bool aSaved = true;
	for (Params::iterator anIt = aParams.begin(); anIt != aParams.end() && aSaved; ++anIt)
		aSaved = aKey->writeString(anIt->first, anIt->second);
	return aSaved;
}
