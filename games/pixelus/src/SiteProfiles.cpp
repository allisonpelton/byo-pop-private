// port: Pixelus's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// Pixelus keeps its players in UserDirectory (users/<number>, the names in each): NewUserDialog makes one, "Who are you?" chooses,
// renames and deletes. This implements the page's view of them with the same functions the dialogs call (PixelusApp::NewUserOK,
// RenameUserOK, DeleteUserOK), and registers with the framework to run when the game has loaded, before the main menu decides
// to ask for a new player.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "MainMenu.h"
#include "ImageFont.h"
#include "EditWidget.h"
#include "WidgetManager.h"
#include "WebProfiles.h"

#include <ctype.h>

using namespace Sexy;

// NewUserDialog's box: Garamond 14, 28 characters (0x1c), 150 pixels (0x96), letters, digits and spaces (UserDialogs.cpp)
static const int kMaxChars = 0x1c;
static const int kMaxPixels = 0x96;

struct NameRules : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar)
	{
		unsigned char aChar = (unsigned char) theChar;
		return isalnum(aChar) || (aChar == ' ');
	}
};
static NameRules sRules;

// NewUserDialog::GetName: leading and double spaces are dropped, and a trailing one
static std::string Normalize(const std::string& theString)
{
	std::string aResult;
	char aPrevious = ' ';
	for (size_t i = 0; i < theString.size(); i++)
	{
		char aChar = theString[i];
		if ((aChar != ' ') || (aPrevious != ' '))
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
	Players(PixelusApp* theApp) : mApp(theApp), mDirectory(theApp->mUserDirectory) {}

	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		for (size_t i = 0; i < mDirectory->mUsers.size(); i++)
			aNames.push_back(mDirectory->mUsers[i]->mName);
		return aNames;
	}

	virtual std::string Current()
	{
		UserProfile* aUser = mDirectory->GetCurrent();
		return (aUser != NULL) ? aUser->mName : std::string();
	}

	virtual std::string Find(const std::string& theName)
	{
		UserProfile* aUser = mDirectory->FindUserByName(theName);
		return (aUser != NULL) ? aUser->mName : std::string();
	}

	// the box types the name in, and GetName tidies what it holds
	virtual std::string Fit(const std::string& theName)
	{
		return Normalize(Byopop::SiteFitTyped(theName, mApp->mFonts[FONT_GARAMOND14], kMaxChars, kMaxPixels, &sRules));
	}

	// NewUserOK: AddUser (which selects), Save; the main menu hears of it
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		mDirectory->AddUser(theName);
		mDirectory->Save();
		Changed();
		return true;
	}

	virtual bool Select(const std::string& theName)
	{
		UserProfile* aUser = mDirectory->FindUserByName(theName);
		if (aUser == NULL)
			return false;
		mDirectory->Select(aUser->mId);
		Changed();
		return true;
	}

	virtual bool CanRename() { return true; }

	// RenameUserOK: RenameUser, Save (the player keeps its number, so its progress)
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		if (!mDirectory->RenameUser(theFrom, theTo))
		{
			*theWhy = "Another player has that name.";
			return false;
		}
		mDirectory->Save();
		Changed();
		return true;
	}

	virtual bool CanDelete() { return true; }

	// DeleteUserOK: DeleteUser (the player's file and best results go; the current one falls to the last player), Save. The game's
	// DeleteUser leaves no current player when it deletes another one; the one that was current stays so here, so that the game
	// does not ask for a new player after it
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		UserProfile* aUser = mDirectory->FindUserByName(theName);
		if (aUser == NULL)
			return false;
		int aCurrent = (mDirectory->GetCurrent() != NULL) ? mDirectory->GetCurrent()->mId : -1;
		int anId = aUser->mId;
		mDirectory->DeleteUser(anId);
		if ((aCurrent != -1) && (aCurrent != anId) && (mDirectory->FindUser(aCurrent) != NULL))
			mDirectory->Select(aCurrent);
		mDirectory->Save();
		Changed();
		return true;
	}

	virtual const char* Rules()
	{
		return "Letters, digits and spaces; up to 28 characters and as many as fit 150 pixels of Garamond 14; leading, double and trailing spaces are dropped";
	}

	virtual const char* DeleteNote() { return "the player, every puzzle solved and the best results"; }

private:
	void Changed()
	{
		if (mApp->mMainMenu != NULL)
			mApp->mMainMenu->UserChanged();
		if (mApp->mWidgetManager != NULL)
			mApp->mWidgetManager->MarkAllDirty();
	}

	PixelusApp*			mApp;
	UserDirectory*		mDirectory;
};

static void Start(SexyAppBase* theApp)
{
	PixelusApp* anApp = (PixelusApp*) theApp;
	if (anApp->mUserDirectory == NULL)
		return;
	Players aPlayers(anApp);
	Byopop::SiteStart("pixelus", aPlayers);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(PixelusApp* theApp)
{
	if (theApp->mUserDirectory == NULL)
		return;
	Players aPlayers(theApp);
	Byopop::SiteChanged("pixelus", aPlayers);
}

void Sexy::SiteFillNewUser(EditWidget* theEdit)
{
	Byopop::SiteType("pixelus", theEdit, false);
}
#endif
