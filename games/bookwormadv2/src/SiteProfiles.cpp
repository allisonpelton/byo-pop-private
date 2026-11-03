// port: Bookworm Adventures Volume 2's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// The players are the profiles in users/<name>.bwa (every player's progress is in its file, as a store of named values) and
// users/data/<name>.dat. The new-player dialog makes one (GameApp::NewUserDialogDone), the main menu's player list chooses
// (GameApp::SelectUser) and deletes (MainMenu::DialogButtonDepress: the file goes), up to MaxProfiles (10); the game has no rename:
// renaming here writes the profile under the new name (Profile::Save, which also makes it the last player) and moves its data,
// then removes the old file. This implements the page's view with those functions, and registers with the framework to run when
// the game has loaded (GameApp::LoadingThreadCompleted has chosen the last player by then), before any dialog asks for a name.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "GameApp.h"
#include "Profile.h"
#include "NewUserDialog.h"
#include "Res.h"
#include "JeffLib.h"
#include "EditWidget.h"
#include "Font.h"
#include "WebProfiles.h"

#include <SDL.h>

using namespace Sexy;

// NewUserDialog's characters (IsNameChar): letters, digits, space and a few marks; ^ only arrives as pasted text
static bool IsNameChar(char theChar)
{
	if ((theChar >= 'a' && theChar <= 'z') || (theChar >= 'A' && theChar <= 'Z') || (theChar >= '0' && theChar <= '9'))
		return true;
	switch (theChar) {
	case ' ':
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '&':
	case '(':
	case ')':
		return true;
	}
	return false;
}

struct NameRules : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar) { return IsNameChar(theChar); }
};
static NameRules sRules;

class Players : public Byopop::SiteProfiles
{
public:
	Players(GameApp* theApp) : mApp(theApp) {}

	// FillUserList: the files of users/*.bwa, as many as MaxProfiles
	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		std::vector<std::string> aFiles = JeffLib::GetFilesInDirectory("users/*.bwa");
		for (size_t i = 0; (i < aFiles.size()) && ((int) i < MaxPlayers()); i++)
			aNames.push_back(aFiles[i].substr(0, aFiles[i].find(".bwa")));
		return aNames;
	}

	// (the intro's player has no name yet and is never saved: it is not a player)
	virtual std::string Current()
	{
		if ((mApp->mProfile == NULL) || mApp->mProfile->mNoSave)
			return "";
		return Find(mApp->mProfile->mName);
	}

	virtual std::string Find(const std::string& theName)
	{
		std::vector<std::string> aNames = Names();
		for (size_t i = 0; i < aNames.size(); i++)
			if (SDL_strcasecmp(aNames[i].c_str(), theName.c_str()) == 0)
				return aNames[i];
		return "";
	}

	// the box types the name in (the large font, 189 pixels); the OK refuses a name of spaces only
	virtual std::string Fit(const std::string& theName)
	{
		std::string aName = Byopop::SiteFitTyped(theName, FONT_LARGE, -1, 189, &sRules);
		return (aName.find_first_not_of(' ') == std::string::npos) ? std::string() : aName;
	}

	// NewUserDialogDone, as when the dialog is not over the main menu: a profile of its own, saved, and a player of that name starts
	// without data
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		if ((int) Names().size() >= MaxPlayers())
		{
			*theWhy = "The game keeps at most " + std::to_string(MaxPlayers()) + " players.";
			return false;
		}
		delete mApp->mProfile;
		mApp->mProfile = new Profile(theName, false);
		mApp->mProfile->Save();
		mApp->DeleteUserData(theName);
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual bool Select(const std::string& theName)
	{
		bool aResult = mApp->SelectUser(theName);
		SiteProfilesChanged(mApp);
		return aResult;
	}

	virtual bool CanRename() { return true; }

	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		Profile aProfile(theFrom, true);
		if (!aProfile.Load())
		{
			*theWhy = "The player's file could not be read.";
			return false;
		}
		aProfile.mName = theTo;
		aProfile.mNoSave = false;
		if (!aProfile.Save())
		{
			*theWhy = "The player's file could not be written.";
			return false;
		}
		// its data, then the old file; Save made the renamed player the last one: the current player is
		std::string aFolder = GetAppDataFolder() + "users/";
		Buffer aData;
		if (mApp->ReadBufferFromFile(aFolder + "data/" + theFrom + ".dat", &aData) &&
			mApp->WriteBufferToFile(aFolder + "data/" + theTo + ".dat", &aData))
			mApp->EraseFile(aFolder + "data/" + theFrom + ".dat");
		mApp->EraseFile(aFolder + theFrom + ".bwa");
		if ((mApp->mProfile != NULL) && (SDL_strcasecmp(mApp->mProfile->mName.c_str(), theFrom.c_str()) == 0))
			mApp->mProfile->mName = theTo;
		if ((mApp->mProfile != NULL) && !mApp->mProfile->mNoSave)
			mApp->RegistryWriteString("LastUser", mApp->mProfile->mName);
		else
			mApp->RegistryEraseValue("LastUser");
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual bool CanDelete() { return true; }

	// The player list's delete (MainMenu::DialogButtonDepress): the file goes, and when it was the player now playing the profile
	// too, and the first of the others plays
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		bool isCurrent = (mApp->mProfile != NULL) && (SDL_strcasecmp(mApp->mProfile->mName.c_str(), theName.c_str()) == 0);
		if (isCurrent)
		{
			delete mApp->mProfile;
			mApp->mProfile = NULL;
			mApp->RegistryEraseValue("LastUser");
		}
		mApp->EraseFile(GetAppDataFolder() + "users/" + theName + ".bwa");
		if (isCurrent)
		{
			std::vector<std::string> aNames = Names();
			if (!aNames.empty())
				mApp->SelectUser(aNames[0]);
		}
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual const char* Rules()
	{
		return "Letters, digits, spaces and ! @ # $ % & ( ); as many characters as fit 189 pixels of the large font; not a name another player has";
	}

	virtual const char* DeleteNote()
	{
		return "the player's profile: the books, chapters and stages played, and everything it has won (its saved data file stays)";
	}

	virtual int MaxPlayers() { return mApp->GetInteger("MaxProfiles", 10); }

private:
	GameApp*	mApp;
};

static void Start(SexyAppBase* theApp)
{
	Players aPlayers((GameApp*) theApp);
	Byopop::SiteStart("bookwormadv2", aPlayers);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(GameApp* theApp)
{
	Players aPlayers(theApp);
	Byopop::SiteChanged("bookwormadv2", aPlayers);
}

void Sexy::SiteFillNewUser(EditWidget* theEdit)
{
	Byopop::SiteType("bookwormadv2", theEdit, true);
}
#endif
