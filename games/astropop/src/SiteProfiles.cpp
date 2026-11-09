// port: AstroPop's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// The players of the game are the names under the registry key Users (AstroPopApp::mUserNames), each with a file
// users\data<n>.usr. The new-player box (NewUserDialog) takes a name of up to 24 characters that fit 305 pixels, and its OK
// refuses a name that is empty after the spaces at its ends are cut, that is one of the two entries of the player list, or
// that matches (capitals alike) a player the game has; the player list makes a new one at most up to ten players. The page's
// view of the players is made with the game's own functions in the order its dialogs use them, without the dialogs, and
// registers with the framework to run when the game has loaded.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "AstroPopApp.h"
#include "EditWidget.h"
#include "GameText.h"
#include "Res.h"
#include "UserProfile.h"
#include "WebProfiles.h"

#include <stdio.h>

using namespace Sexy;

// NewUserDialog's box: font 0xb0 (the typed text), 24 characters and 305 pixels
static const int kMaxChars = 0x18;
static const int kMaxPixels = 0x131;
static const int kMaxPlayers = 10;

// 0x410b80: the letters a .. z become capitals (the names are compared that way)
static std::string UpperName(const std::string& theName)
{
	std::string aResult = theName;
	for (size_t i = 0; i < aResult.size(); i++)
	{
		if (aResult[i] > '`' && aResult[i] < '{')
			aResult[i] = (char) (aResult[i] - 0x20);
	}
	return aResult;
}

// 0x410c90: the spaces, tabs and line ends at both ends of a name go
static std::string TrimName(const std::string& theName)
{
	size_t aStart = 0;
	size_t anEnd = theName.size();
	while (aStart < anEnd && (theName[aStart] == ' ' || theName[aStart] == '\n' || theName[aStart] == '\r' || theName[aStart] == '\t'))
		aStart++;
	while (anEnd > aStart && (theName[anEnd - 1] == ' ' || theName[anEnd - 1] == '\n' || theName[anEnd - 1] == '\r' || theName[anEnd - 1] == '\t'))
		anEnd--;
	return theName.substr(aStart, anEnd - aStart);
}

namespace
{

class Players : public Byopop::SiteProfiles
{
public:
	Players(AstroPopApp* theApp) : mApp(theApp) {}

	// (the list is read from the registry again: the game reads it when its title is done, which may be after this)
	virtual std::vector<std::string> Names()
	{
		mApp->ListUsers();
		return mApp->mUserNames;
	}

	virtual std::string Current()
	{
		std::string aName;
		if (mApp->mUser != NULL)
			mApp->mUser->GetString(PROFILE_NAME, &aName);
		return aName;
	}

	virtual std::string Find(const std::string& theName)
	{
		std::string anUpper = UpperName(theName);
		for (size_t i = 0; i < mApp->mUserNames.size(); i++)
			if (UpperName(mApp->mUserNames[i]) == anUpper)
				return mApp->mUserNames[i];
		return "";
	}

	virtual std::string Fit(const std::string& theName)
	{
		return TrimName(Byopop::SiteFitTyped(theName, GetFontById(0xb0), kMaxChars, kMaxPixels));
	}

	// NewUserOk: the player is made, saved and becomes the current one
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		mApp->ListUsers();
		std::string anUpper = UpperName(theName);
		if (anUpper.empty() || anUpper == UpperName(GT(0x55dce0)) || anUpper == UpperName(GT(0x55dcd0)))
		{
			*theWhy = "That name is one of the entries of the player list.";
			return false;
		}
		if ((int) mApp->mUserNames.size() >= kMaxPlayers)
		{
			*theWhy = "AstroPop keeps at most 10 players.";
			return false;
		}
		if (mApp->mUser != NULL)
		{
			mApp->mUser->Save();
			delete mApp->mUser;
		}
		mApp->mUser = new UserProfile();
		mApp->mUser->InitNew(theName, -1);
		mApp->mUser->Save();
		mApp->ValidateUser();
		mApp->ListUsers();
		SiteProfilesChanged(mApp);
		return true;
	}

	// UserSelectOk: that player is loaded in place of the one that is playing (saved first) and is the one of the next session
	virtual bool Select(const std::string& theName)
	{
		std::string aName = Find(theName);
		if (aName.empty())
			return false;
		std::string aCurrent = Current();
		if (aCurrent != aName)
		{
			if (mApp->mUser != NULL)
			{
				mApp->mUser->Save();
				delete mApp->mUser;
				mApp->mUser = NULL;
			}
			mApp->mUser = new UserProfile();
			mApp->mUser->SetString(PROFILE_NAME, aName);
			if (!mApp->mUser->Load(NULL, NULL, false))
				mApp->ResetFailedUserLoad();
		}
		mApp->SetLastUserName(aName);
		mApp->ValidateUser();
		return true;
	}

	virtual bool CanDelete() { return true; }

	// DeleteUser(true) without its boxes: the registry key and the data file go; the player that was playing goes on, unless
	// it was the one deleted (then the first of the rest). The last player is kept: the game would ask for a new one at once.
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		std::string aDeleted = Find(theName);
		if (aDeleted.empty())
			return false;
		if (mApp->mUserNames.size() < 2)
		{
			*theWhy = "AstroPop needs at least one player.";
			return false;
		}

		std::string aCurrent = Current();
		if (mApp->mUser != NULL)
		{
			mApp->mUser->Save();
			delete mApp->mUser;
		}
		mApp->mUser = new UserProfile();
		mApp->mUser->SetString(PROFILE_NAME, aDeleted);
		mApp->mUser->Load(NULL, NULL, false);

		int aNumber = -1;
		bool aFine = mApp->RegistryEraseKey(StrFormat("Users\\%s", aDeleted.c_str()));
		if (!mApp->mUser->GetInt(PROFILE_NUMBER, &aNumber))
			aFine = false;
		if (aNumber != -1 && !mApp->EraseFile(StrFormat("users\\data%d.usr", aNumber)))
			aFine = false;
		delete mApp->mUser;
		mApp->mUser = NULL;
		mApp->ListUsers();

		mApp->mUser = new UserProfile();
		if (aDeleted == aCurrent && !mApp->mUserNames.empty())
		{
			aCurrent = mApp->mUserNames[0];
			mApp->SetLastUserName(aCurrent);
		}
		mApp->mUser->SetString(PROFILE_NAME, aCurrent);
		if (!mApp->mUser->Load(NULL, NULL, false))
			mApp->ResetFailedUserLoad();
		mApp->ValidateUser();
		SiteProfilesChanged(mApp);
		if (!aFine)
		{
			*theWhy = "The player's files could not all be removed.";
			return false;
		}
		return true;
	}

	virtual const char* Rules()
	{
		return "Up to 24 characters and as many as fit 305 pixels of the box's font; not blank, and not one that another player has (capitals alike)";
	}

	virtual int MaxPlayers() { return kMaxPlayers; }

private:
	AstroPopApp*	mApp;
};

}

static void Start(SexyAppBase* theApp)
{
	Players aPlayers((AstroPopApp*) theApp);
	Byopop::SiteStart("astropop", aPlayers);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(AstroPopApp* theApp)
{
	Players aPlayers(theApp);
	Byopop::SiteChanged("astropop", aPlayers);
}

void Sexy::SiteFillUserName(EditWidget* theEdit)
{
	Byopop::SiteType("astropop", theEdit);
}
#endif
