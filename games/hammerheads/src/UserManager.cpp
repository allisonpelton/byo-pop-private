#include "UserManager.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "Application.h"
#include "Buffer.h"
#include "Common.h"
#include "platform/VFS.h"

using namespace Sexy;

namespace
{

// The roster's file: one line a player, the number of the player's file
const char* const DIRECTORY_FILE_NAME = "users\\directory.txt";

// How FindFirstFile put the names of a folder in order: by the letters without their case
bool NameBefore(const std::string& theFirst, const std::string& theSecond)
{
	size_t aLength = std::min(theFirst.size(), theSecond.size());
	for (size_t i = 0; i < aLength; i++)
	{
		int aFirst = std::toupper((unsigned char)theFirst[i]);
		int aSecond = std::toupper((unsigned char)theSecond[i]);
		if (aFirst != aSecond)
			return aFirst < aSecond;
	}
	return theFirst.size() < theSecond.size();
}

}

// 0x473710: the players are the files that the roster lists
UserManager::UserManager() :
	mCurrentUser(nullptr)
{
	ReadDirectory("users\\");

	// the player the game was left with is the one whose file says so; without one, the first player
	for (size_t i = 0; i < mIds.size(); i++)
	{
		User* aUser = LoadUser(mIds[i]);
		if (mCurrentUser == nullptr || (aUser != nullptr && aUser->mIsCurrent))
			SetCurrentUser(mIds[i]);
	}
}

// 0x473030: the roster and every player's file are written, then the players are let go
UserManager::~UserManager()
{
	SaveRoster();

	for (size_t i = 0; i < mUsers.size(); i++)
		delete mUsers[i];
	mUsers.clear();
	mIds.clear();
	mById.clear();
	mByName.clear();
	mCurrentUser = nullptr;
}

// 0x471450
User* UserManager::GetCurrentUser()
{
	return mCurrentUser;
}

// 0x471820
User* UserManager::FindUser(const std::string& theName)
{
	std::map<std::string, User*>::iterator anItr = mByName.find(theName);
	if (anItr == mByName.end())
		return nullptr;
	return anItr->second;
}

// 0x473600: a new player, with the first number that no player has, becomes the current one
User* UserManager::CreateUser(const std::string& theName)
{
	std::map<std::string, User*>::iterator anItr = mByName.find(theName);
	if (anItr != mByName.end() && anItr->second != nullptr)
		return nullptr;

	int anId = FindFreeId();
	MakeUser(theName, anId);
	SetCurrentUser(anId);
	mIds.push_back(anId);
	return mCurrentUser;
}

// 0x4733c0: the name of a player changes, unless another player has the new one
bool UserManager::RenameUser(const std::string& theOldName, const std::string& theNewName)
{
	User* aUser = FindUser(theOldName);
	User* anOwner = FindUser(theNewName);
	if (anOwner != nullptr && anOwner != aUser)
		return false;
	if (aUser == nullptr)
		return false;		// the original goes on with no player at all

	aUser->mName = theNewName;
	mByName.erase(theOldName);
	mByName[theNewName] = aUser;
	return true;
}

// 0x4716b0
void UserManager::SetCurrentUser(int theId)
{
	for (size_t i = 0; i < mUsers.size(); i++)
	{
		User* aUser = mUsers[i];
		if (aUser->mId == theId)
		{
			aUser->mIsCurrent = true;
			mCurrentUser = aUser;
		}
		else
			aUser->mIsCurrent = false;
	}
}

// 0x4716f0
void UserManager::ClearCurrentUser()
{
	for (size_t i = 0; i < mUsers.size(); i++)
		mUsers[i]->mIsCurrent = false;
	mCurrentUser = nullptr;
}

// 0x471790: the smallest number from 1 that no player has
int UserManager::FindFreeId()
{
	int anId = 1;
	for (;;)
	{
		std::map<int, User*>::iterator anItr = mById.find(anId);
		if (anItr == mById.end() || anItr->second == nullptr)
			break;
		anId++;
	}
	return anId;
}

// 0x472170: the player goes out of the roster and the file of the player is erased; when it was the current one, the player
// before it in the roster (or the first) takes over
void UserManager::DeleteUser(int theId)
{
	std::map<int, User*>::iterator anItr = mById.find(theId);
	User* aUser = anItr != mById.end() ? anItr->second : nullptr;
	if (anItr != mById.end() && aUser != nullptr)
	{
		int anIndex = 0;
		for (std::vector<User*>::iterator aUserItr = mUsers.begin(); aUserItr != mUsers.end(); ++aUserItr)
		{
			if ((*aUserItr)->mId == aUser->mId)
			{
				mUsers.erase(aUserItr);
				break;
			}
			anIndex++;
		}

		std::map<std::string, User*>::iterator aNameItr = mByName.find(aUser->mName);
		if (aNameItr != mByName.end())
			mByName.erase(aNameItr);

		std::map<int, User*>::iterator anIdItr = mById.find(aUser->mId);
		if (anIdItr != mById.end())
			mById.erase(anIdItr);

		for (std::vector<int>::iterator anIdsItr = mIds.begin(); anIdsItr != mIds.end(); ++anIdsItr)
		{
			if (*anIdsItr == aUser->mId)
			{
				mIds.erase(anIdsItr);
				break;
			}
		}

		if (aUser == mCurrentUser)
		{
			if (mUsers.empty())
				ClearCurrentUser();
			else
			{
				int aNext = anIndex > 0 ? anIndex - 1 : 0;
				if (aNext >= (int)mUsers.size())
					aNext = (int)mUsers.size() - 1;
				SetCurrentUser(mUsers[aNext]->mId);
			}
		}
		delete aUser;
	}

	gApp->EraseFile(StrFormat("users\\%d.xml", theId));
	WriteDirectory(DIRECTORY_FILE_NAME);
}

// 0x4720d0: every player's file is written, and the roster
void UserManager::SaveRoster()
{
	for (size_t i = 0; i < mUsers.size(); i++)
		mUsers[i]->SaveProfile();
	WriteDirectory(DIRECTORY_FILE_NAME);
}

// 0x472e50: the numbers of the players' files come from directory.txt; when there is none, from the files in the folder (which
// then make the file)
void UserManager::ReadDirectory(const std::string& theFolder)
{
	std::string aFileName = theFolder + "directory.txt";
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(aFileName, &aBuffer))
	{
		if (ScanFolder(theFolder))
			WriteDirectory(aFileName);
	}
	else
	{
		while (!aBuffer.AtEnd())
		{
			std::string aLine = aBuffer.ReadLine();
			mIds.push_back((int)atol(aLine.c_str()));
		}
	}
}

// 0x472d20: the files of the folder that are not folders, and whose names start with a number above 0 (users\3.xml)
bool UserManager::ScanFolder(const std::string& theFolder)
{
	std::string aFolderName = theFolder;
	for (size_t i = 0; i < aFolderName.size(); i++)
	{
		if (aFolderName[i] == '\\')
			aFolderName[i] = '/';
	}
	std::string aFolderPath = RemoveTrailingSlash(GetAppDataFolder() + aFolderName);
	if (!Platform::IsDirectory(aFolderPath))
		return false;

	std::vector<std::string> aNames = Platform::ListDirectory(aFolderPath);
	std::sort(aNames.begin(), aNames.end(), NameBefore);
	for (size_t i = 0; i < aNames.size(); i++)
	{
		if (Platform::IsDirectory(aFolderPath + "/" + aNames[i]))
			continue;

		long aNumber = atol(aNames[i].c_str());
		if (aNumber > 0)
			mIds.push_back((int)aNumber);
	}
	return true;
}

// 0x4715e0: one line for each number
void UserManager::WriteDirectory(const std::string& theFileName)
{
	Buffer aBuffer;
	for (size_t i = 0; i < mIds.size(); i++)
		aBuffer.WriteLine(StrFormat("%d", mIds[i]));
	gApp->WriteBufferToFile(theFileName, &aBuffer);
}

// 0x473660: the player of a file; one whose file is not of this version (or is not there) is not taken
User* UserManager::LoadUser(int theId)
{
	User* aUser = new User(theId);
	if (aUser->IsCurrentVersion())
	{
		AddUser(aUser);
		return aUser;
	}

	delete aUser;
	return nullptr;
}

// 0x473460
void UserManager::AddUser(User* theUser)
{
	mUsers.push_back(theUser);
	mByName[theUser->mName] = theUser;
	mById[theUser->mId] = theUser;
}

// 0x473530
User* UserManager::MakeUser(const std::string& theName, int theId)
{
	User* aUser = new User(theName, theId);
	mUsers.push_back(aUser);
	mByName[theName] = aUser;
	mById[theId] = aUser;
	return aUser;
}
