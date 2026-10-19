#include "UserManager.h"
#include "BookWormApp.h"

#include <algorithm>
#include <cctype>

using namespace Sexy;

// The registry key of the profiles (HKEY_LOCAL_MACHINE\SOFTWARE\<mRegKey>\Users in the original)
static const char* const gUsersKey = "Users";

static SexyString UserValueName(const SexyString& theUserName, const SexyString& theValueName)
{
	return SexyString(gUsersKey) + "\\" + theUserName + "\\" + theValueName;
}

// port: the order RegEnumKeyEx returns subkeys in: Windows keeps them sorted by their upper-cased names
// (NAMES.md L5; the emulated registry's own order differs)
static bool RegistryKeyLess(const SexyString& a, const SexyString& b)
{
	size_t aLength = std::min(a.length(), b.length());
	for (size_t i = 0; i < aLength; i++)
	{
		int ca = toupper((unsigned char) a[i]);
		int cb = toupper((unsigned char) b[i]);
		if (ca != cb)
			return ca < cb;
	}
	return a.length() < b.length();
}

// 0x434960
User::User(const SexyString& theName)
{
	mName = theName;
	mAlias = theName;
}

// 0x435b50: the profiles in the registry's order, then the last player
UserManager::UserManager(BookWormApp* theApp)
{
	mApp = theApp;
	mCurrentUser = "";

	StringVector aNames;
	if (mApp->RegistryGetSubKeys(gUsersKey, &aNames))
	{
		std::stable_sort(aNames.begin(), aNames.end(), RegistryKeyLess);
		for (size_t i = 0; i < aNames.size(); i++)
		{
			// port: the original pushes LoadUser's result even when it is NULL (a key it just enumerated
			// that does not open); the emulated registry cannot fail there, and other modules would
			// dereference a NULL entry
			User* aUser = LoadUser(aNames[i]);
			if (aUser != NULL)
				mUsers.push_back(aUser);
		}
	}

	mCurrentUser = GetLastUser();
}

// 0x434880
UserManager::~UserManager()
{
	for (std::list<User*>::iterator anItr = mUsers.begin(); anItr != mUsers.end(); ++anItr)
		delete *anItr;
	mUsers.clear();
}

// 0x434800
User* UserManager::FindUser(const SexyString& theName)
{
	for (std::list<User*>::iterator anItr = mUsers.begin(); anItr != mUsers.end(); ++anItr)
	{
		if (_stricmp((*anItr)->mName.c_str(), theName.c_str()) == 0)
			return *anItr;
	}
	return NULL;
}

// 0x4349e0: a DWORD of the profile of an existing user (the key named as stored)
bool UserManager::ReadUserInt(const SexyString& theUserName, const SexyString& theValueName, int* theValue)
{
	User* aUser = FindUser(theUserName);
	if (aUser == NULL)
		return false;

	int aValue;
	if (!mApp->RegistryReadInteger(UserValueName(aUser->mName, theValueName), &aValue))
		return false;
	if (theValue != NULL)
		*theValue = aValue;
	return true;
}

// 0x434b70: REG_SZ (the original always writes 256 bytes: the string, its NUL and stack garbage)
bool UserManager::WriteUserString(const SexyString& theUserName, const SexyString& theValueName,
	const SexyString& theValue)
{
	User* aUser = FindUser(theUserName);
	if (aUser == NULL)
		return false;
	return mApp->RegistryWriteString(UserValueName(aUser->mName, theValueName), theValue);
}

// 0x434d10: REG_DWORD (HelpWidget's help-at-startup checkbox)
bool UserManager::WriteUserInt(const SexyString& theUserName, const SexyString& theValueName, int theValue)
{
	User* aUser = FindUser(theUserName);
	if (aUser == NULL)
		return false;
	return mApp->RegistryWriteInteger(UserValueName(aUser->mName, theValueName), theValue);
}

// 0x434e90: opens the profile's key, else creates it, then writes the alias and the app's current options
// and play count. WriteUserString / WriteUserInt look the user up by name, so for a User not yet in the
// list (the app saves a new player before adding it, ButtonDepress 2022) only the empty key is made; its
// values come with the next save while it is the current player (WriteToRegistry, the options).
void UserManager::SaveUser(User* theUser)
{
	// port: a key of the emulated registry exists only while a value is stored below it. A new key gets an
	// empty "Alias", which LoadUser reads back as the original reads the missing value (""); without it the
	// player would vanish from the roster unless they were current at a later save.
	StringVector aSubKeys;
	if (!mApp->RegistryGetSubKeys(SexyString(gUsersKey) + "\\" + theUser->mName, &aSubKeys))
		mApp->RegistryWriteString(UserValueName(theUser->mName, "Alias"), "");

	WriteUserString(theUser->mName, "Alias", theUser->mAlias);
	WriteUserInt(theUser->mName, "ShowHelp", mApp->mShowHelp ? 1 : 0);
	WriteUserInt(theUser->mName, "ShowHints", mApp->mShowHints ? 1 : 0);
	WriteUserInt(theUser->mName, "ShowDefs", mApp->mShowDefs ? 1 : 0);
	WriteUserInt(theUser->mName, "TimesPlayed", mApp->mUserTimesPlayed);
}

// 0x4352e0: every profile with that name (case-insensitive) whose key deletes; the key deleted is the one
// named by the argument. The player's saved games stay.
void UserManager::DeleteUser(const SexyString& theName)
{
	std::list<User*>::iterator anItr = mUsers.begin();
	while (anItr != mUsers.end())
	{
		if ((_stricmp((*anItr)->mName.c_str(), theName.c_str()) == 0) &&
			(mApp->RegistryEraseKey(SexyString(gUsersKey) + "\\" + theName)))
		{
			delete *anItr;
			anItr = mUsers.erase(anItr);
		}
		else
			++anItr;
	}
}

// 0x435470: NULL when the key does not open; the alias is "" when the profile has none
User* UserManager::LoadUser(const SexyString& theName)
{
	// port: a key of the emulated registry exists while a value is stored below it
	StringVector aSubKeys;
	if (!mApp->RegistryGetSubKeys(SexyString(gUsersKey) + "\\" + theName, &aSubKeys))
		return NULL;

	SexyString anAlias;
	mApp->RegistryReadString(UserValueName(theName, "Alias"), &anAlias);
	User* aUser = new User(theName);
	aUser->mAlias = anAlias;
	return aUser;
}

// 0x4356d0: only for a user in the list (the name written is the argument)
bool UserManager::SetCurrentUser(const SexyString& theName)
{
	for (std::list<User*>::iterator anItr = mUsers.begin(); anItr != mUsers.end(); ++anItr)
	{
		if (_stricmp((*anItr)->mName.c_str(), theName.c_str()) == 0)
		{
			bool success = mApp->RegistryWriteString(SexyString(gUsersKey) + "\\User", theName);
			if (success)
				mCurrentUser = theName;
			return success;
		}
	}
	return false;
}

// 0x435880 (the internet alias; its callers are not ported)
bool UserManager::SetCurrentAlias(const SexyString& theAlias)
{
	User* aUser = FindUser(mCurrentUser);
	if (aUser == NULL)
		return false;
	aUser->mAlias = theAlias;
	SaveUser(aUser);
	return true;
}

// 0x435910
SexyString UserManager::GetCurrentAlias()
{
	SexyString aName = mCurrentUser;
	if (aName.length() != 0)
	{
		User* aUser = FindUser(aName);
		if ((aUser != NULL) && (aUser->mAlias.length() != 0))
			return aUser->mAlias;
	}
	return aName;
}

// 0x4359b0: Users\User (not checked against the list), else the first user that can be made current
SexyString UserManager::GetLastUser()
{
	SexyString aName;
	mApp->RegistryReadString(SexyString(gUsersKey) + "\\User", &aName);

	if (aName.length() == 0)
	{
		for (std::list<User*>::iterator anItr = mUsers.begin(); anItr != mUsers.end(); ++anItr)
		{
			if (SetCurrentUser((*anItr)->mName))
			{
				aName = (*anItr)->mName;
				break;
			}
		}
	}
	return aName;
}
