// port: Bejeweled 2's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// Bejeweled 2 keeps its players in Profile: a folder for each (users/<name>/, with profile.dat and the saved games) and the
// roster of their names. CreateUserDialog makes one (by itself when there is none: WinDMApp::LoadingCompleted starts the menu's
// timer for it), "Who are you?" (UserDialog) chooses, renames and deletes. This implements the page's view of them with the
// functions those dialogs end in (WinDMApp::FinishCreateUserDialog, FinishUserDialog, FinishRenameUserDialog,
// FinishConfirmDeleteUserDialog), and registers with the framework to run when the game has loaded, before the title's play
// link leads to the menu that would ask for a new player.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "MainMenu.h"
#include "Res.h"
#include "EditWidget.h"
#include "WidgetManager.h"
#include "WebProfiles.h"

#include <ctype.h>
#include <strings.h>

using namespace Sexy;

// CreateUserDialog's box: Continuum medium 14, 12 characters, 200 pixels, letters, digits and the space (UserDialogs.cpp)
static const int kMaxChars = 12;
static const int kMaxPixels = 200;

struct NameRules : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar)
	{
		return isalnum((unsigned char) theChar) != 0 || theChar == ' ';
	}
};
static NameRules sRules;

// CreateUserDialog::GetName: runs of spaces become one, and no space stays at either end
static std::string Normalize(const std::string& theString)
{
	std::string aResult;
	char aPrevious = ' ';
	for (size_t i = 0; i < theString.size(); i++)
	{
		char aChar = theString[i];
		if (aChar != ' ' || aPrevious != ' ')
			aResult += aChar;
		aPrevious = aChar;
	}
	if (!aResult.empty() && aResult[aResult.size() - 1] == ' ')
		aResult.erase(aResult.size() - 1);
	return aResult;
}

class Players : public Byopop::SiteProfiles
{
public:
	Players(WinDMApp* theApp) : mApp(theApp), mProfile(theApp->mProfile) {}

	virtual std::vector<std::string> Names() { return mProfile->mNames; }

	virtual std::string Current() { return mProfile->mName; }

	// the roster compares names without regard to case (Profile::ListAddName)
	virtual std::string Find(const std::string& theName)
	{
		for (size_t i = 0; i < mProfile->mNames.size(); i++)
		{
			if (strcasecmp(mProfile->mNames[i].c_str(), theName.c_str()) == 0)
				return mProfile->mNames[i];
		}
		return "";
	}

	// the box types the name in, and GetName tidies what it holds
	virtual std::string Fit(const std::string& theName)
	{
		return Normalize(Byopop::SiteFitTyped(theName, FONT_CONTINUUM_MED_14, kMaxChars, kMaxPixels, &sRules));
	}

	// FinishCreateUserDialog: CreateProfile (which makes the new player the current one); the first player gets the welcome
	// message when the menu is there. The profile is written at once, so that the folder that makes it a player exists
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		if (!mProfile->CreateProfile(theName))
		{
			*theWhy = "Another player has that name.";
			return false;
		}
		mProfile->WriteProfile();
		if (mProfile->mNames.size() == 1 && mApp->mMainMenu != NULL)
			mApp->mMainMenu->mWelcomeTimer = 20;
		Changed();
		return true;
	}

	// FinishUserDialog: LoadProfile
	virtual bool Select(const std::string& theName)
	{
		std::string aName = Find(theName);
		if (aName.empty() || !mProfile->LoadProfile(aName))
			return false;
		Changed();
		return true;
	}

	virtual bool CanRename() { return true; }

	// FinishRenameUserDialog: RenameProfile (the player's folder takes the new name, so everything stays), and the current
	// player is loaded again under it
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		bool aCurrent = strcasecmp(mProfile->mName.c_str(), theFrom.c_str()) == 0;
		if (!mProfile->RenameProfile(theFrom, theTo))
		{
			*theWhy = "Another player has that name.";
			return false;
		}
		if (aCurrent)
			mProfile->LoadProfile(theTo);
		Changed();
		return true;
	}

	virtual bool CanDelete() { return true; }

	// FinishConfirmDeleteUserDialog: DeleteProfile (the folder goes), then another player is loaded. The dialog loads the first
	// of the roster after every delete; here the current player stays unless it was the one deleted, so that deleting another
	// player from the page does not change who plays
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		std::string aName = Find(theName);
		if (aName.empty())
			return false;
		bool aCurrent = strcasecmp(mProfile->mName.c_str(), aName.c_str()) == 0;
		mProfile->DeleteProfile(aName);
		if (aCurrent)
			mProfile->GetAnyProfile();
		mApp->WriteToRegistry();
		Changed();
		return true;
	}

	virtual const char* Rules()
	{
		return "Letters, digits and spaces; up to 12 characters and as many as fit 200 pixels of the dialog's font; leading, double and trailing spaces are dropped";
	}

	virtual const char* DeleteNote() { return "the player, the saved games, the puzzle progress and the unlocked modes"; }

private:
	// what the dialogs do after a change: everything is drawn again and the menu shows the player
	void Changed()
	{
		if (mApp->mWidgetManager != NULL)
			mApp->mWidgetManager->MarkAllDirty();
		if (mApp->mMainMenu != NULL)
			mApp->mMainMenu->SyncProfile();
	}

	WinDMApp*		mApp;
	Profile*		mProfile;
};

static void Start(SexyAppBase* theApp)
{
	WinDMApp* anApp = (WinDMApp*) theApp;
	if (anApp->mProfile == NULL)
		return;
	Players aPlayers(anApp);
	Byopop::SiteStart("bejeweled2", aPlayers);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(WinDMApp* theApp)
{
	if (theApp->mProfile == NULL)
		return;
	Players aPlayers(theApp);
	Byopop::SiteChanged("bejeweled2", aPlayers);
}

void Sexy::SiteFillNewUser(EditWidget* theEdit)
{
	Byopop::SiteType("bejeweled2", theEdit);
}
#endif
