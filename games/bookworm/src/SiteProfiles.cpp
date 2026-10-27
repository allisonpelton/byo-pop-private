// port: BookWorm's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// BookWorm keeps its players in UserManager: a registry key for each (Users\<name>: Alias, ShowHelp, ShowHints, ShowDefs,
// TimesPlayed) and a folder of saved games (players\<name>\). The new-player dialog makes one (BookWormApp::ButtonDepress,
// DIALOG_NEW_USER), the player list chooses (SelectUserDialog) and deletes (DeleteUserDialog), up to five players, and the
// game has no rename: renaming here makes the player under the new name with the old one's values and saved games, and removes
// the old. This implements the page's view with those functions, and registers with the framework to run when the game has
// loaded, before the game selector decides to ask for a new player.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "BookWormApp.h"
#include "UserManager.h"
#include "NewUserDialog.h"
#include "SelectUserDialog.h"
#include "ImageFont.h"
#include "EditWidget.h"
#include "Buffer.h"
#include "WebProfiles.h"
#include "platform/VFS.h"

using namespace Sexy;

// NewUserDialog's box: Verdana 12 bold, 22 characters, 200 pixels, letters and spaces (NewUserDialog::AllowChar)
struct NameRules : public EditListener
{
	virtual bool AllowChar(int theId, SexyChar theChar)
	{
		unsigned char aChar = (unsigned char) theChar;
		return ((aChar >= 'A') && (aChar <= 'Z')) || ((aChar >= 'a') && (aChar <= 'z')) || (aChar == ' ');
	}
};
static NameRules sRules;

// the button's TrimString(aName, " \t\r\n")
static std::string TrimEnds(const std::string& theString)
{
	size_t aStart = theString.find_first_not_of(" \t\r\n");
	if (aStart == std::string::npos)
		return "";
	size_t anEnd = theString.find_last_not_of(" \t\r\n");
	return theString.substr(aStart, anEnd - aStart + 1);
}

class Players : public Byopop::SiteProfiles
{
public:
	Players(BookWormApp* theApp) : mApp(theApp), mUsers(theApp->mUserManager) {}

	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		for (std::list<User*>::iterator anItr = mUsers->mUsers.begin(); anItr != mUsers->mUsers.end(); ++anItr)
			aNames.push_back((*anItr)->mName);
		return aNames;
	}

	// (the registry's "last player" may name one that is gone; that is not a current player)
	virtual std::string Current() { return Find(mUsers->GetCurrentUserName()); }

	virtual std::string Find(const std::string& theName)
	{
		User* aUser = mUsers->FindUser(theName);
		return (aUser != NULL) ? aUser->mName : std::string();
	}

	virtual std::string Fit(const std::string& theName)
	{
		return TrimEnds(Byopop::SiteFitTyped(theName, mApp->mFonts[FONT_VERDANA12BOLD], NewUserDialog::NAME_MAX_CHARS,
			NewUserDialog::NAME_MAX_PIXELS, &sRules));
	}

	// The new-player dialog's OK: a User kept with the app's current options (SaveUser makes only its key, as the User is not
	// in the list yet), added to the list and chosen. Five players at most: the player list offers no more.
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		if ((int) mUsers->mUsers.size() >= SelectUserDialog::MAX_LISTED_USERS)
		{
			*theWhy = "BookWorm keeps at most 5 players.";
			return false;
		}
		User* aUser = new User(theName);
		mApp->mUserTimesPlayed = 0;
		mUsers->SaveUser(aUser);
		mUsers->mUsers.push_back(aUser);
		mUsers->SetCurrentUser(aUser->mName);
		SiteProfilesChanged(mApp);
		return true;
	}

	// The player list's OK: the player becomes current and its options are loaded
	virtual bool Select(const std::string& theName)
	{
		User* aUser = mUsers->FindUser(theName);
		if (aUser == NULL)
			return false;
		mUsers->SetCurrentUser(aUser->mName);
		mApp->LoadUserSettings();
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual bool CanRename() { return true; }

	// The game has no rename: the new name gets a player of its own with the old one's values (alias and options) and its saved
	// games (players\<name>\), then the old player goes the way the game's delete does. The player stays current if it was.
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		User* anOld = mUsers->FindUser(theFrom);
		if (anOld == NULL)
			return false;
		std::string aFrom = anOld->mName;
		bool wasCurrent = (mUsers->GetCurrentUserName() == aFrom);
		static const char* const kInts[] = { "ShowHelp", "ShowHints", "ShowDefs", "TimesPlayed" };
		int aValues[4] = { 0, 0, 0, 0 };
		bool aHave[4];
		for (int i = 0; i < 4; i++)
			aHave[i] = mUsers->ReadUserInt(aFrom, kInts[i], &aValues[i]);

		User* aNew = new User(theTo);
		aNew->mAlias = (anOld->mAlias == aFrom) ? theTo : anOld->mAlias;
		mUsers->SaveUser(aNew);
		mUsers->mUsers.push_back(aNew);
		mUsers->WriteUserString(theTo, "Alias", aNew->mAlias);
		for (int i = 0; i < 4; i++)
			if (aHave[i])
				mUsers->WriteUserInt(theTo, kInts[i], aValues[i]);

		MoveSavedGames(aFrom, theTo);

		mUsers->DeleteUser(aFrom);
		if (wasCurrent)
			mUsers->SetCurrentUser(theTo);
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual bool CanDelete() { return true; }

	// The delete dialog's: the registry key and the player go; the saved games stay on disk, as in the game. The first player left
	// becomes current when it was the current one.
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		User* aUser = mUsers->FindUser(theName);
		if (aUser == NULL)
			return false;
		std::string aName = aUser->mName;
		bool wasCurrent = (mUsers->GetCurrentUserName() == aName);
		mUsers->DeleteUser(aName);
		if (wasCurrent && !mUsers->mUsers.empty())
			mUsers->SetCurrentUser(mUsers->mUsers.front()->mName);
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual const char* Rules()
	{
		return "Letters and spaces only; up to 22 characters and as many as fit 200 pixels of Verdana 12 bold; the ends are trimmed";
	}

	virtual const char* DeleteNote()
	{
		return "the player and its options; its saved games stay on disk, as when you delete a player in the game";
	}

	virtual int MaxPlayers() { return SelectUserDialog::MAX_LISTED_USERS; }

private:
	// players\<from>\* to players\<to>\*, file by file (the user dir's folders, as the saved games are written)
	void MoveSavedGames(const std::string& theFrom, const std::string& theTo)
	{
		std::string aDir = Platform::VFS::Get().Resolve("userdata/players/" + theFrom);
		if (aDir.empty())
			return;
		std::vector<std::string> aFiles = Platform::ListDirectory(aDir);
		for (size_t i = 0; i < aFiles.size(); i++)
		{
			Buffer aBuffer;
			std::string anOld = "players/" + theFrom + "/" + aFiles[i], aNew = "players/" + theTo + "/" + aFiles[i];
			if (mApp->ReadBufferFromFile(anOld, &aBuffer) && mApp->WriteBufferToFile(aNew, &aBuffer))
				mApp->EraseFile(anOld);
		}
	}

	BookWormApp*	mApp;
	UserManager*	mUsers;
};

static void Start(SexyAppBase* theApp)
{
	BookWormApp* anApp = (BookWormApp*) theApp;
	if (anApp->mUserManager == NULL)
		return;
	Players aPlayers(anApp);
	Byopop::SiteStart("bookworm", aPlayers);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(BookWormApp* theApp)
{
	if (theApp->mUserManager == NULL)
		return;
	Players aPlayers(theApp);
	Byopop::SiteChanged("bookworm", aPlayers);
}

void Sexy::SiteFillNewUser(EditWidget* theEdit)
{
	Byopop::SiteType("bookworm", theEdit, false);
}
#endif
