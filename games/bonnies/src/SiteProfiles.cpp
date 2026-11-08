// port: Bonnie's Bookstore's part of the site's players and volumes on the web page (framework/WebProfiles.h,
// framework/WebSettings.h). Web only. The game's side is game/SitePlayers.cpp (game/Start.h): this file hands its
// functions to the framework, and sets the game's hooks, so that the game's sources call nothing of the framework.
//
// The decision point at start is the first menu (MenuScreen::init): the game asks for a name only when a mode is chosen
// with no player, so under "create" the site's player is made or chosen there, before the menu shows whose game it is.
// A player's name is its saved game's file name, which the game never renames, so the site cannot rename one. Deleting is
// the players panel's Delete (the original's removed nothing, a fault the port fixes: docs/DEVIATIONS.md).

#ifdef __EMSCRIPTEN__
#include "../game/Start.h"
#include "WebProfiles.h"
#include "WebSettings.h"

#include <cctype>

static bool sStarted = false;

static bool SameName(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++)
		if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
			return false;
	return true;
}

class BonniesPlayers : public Byopop::SiteProfiles
{
public:
	std::vector<std::string> Names() override { return bonnies::sitePlayers(); }
	std::string Current() override { return bonnies::siteCurrent(); }
	// playerSaveExists: the saved game's file, whose name the file system compares without case
	std::string Find(const std::string& theName) override
	{
		std::vector<std::string> aNames = Names();
		for (size_t n = 0; n < aNames.size(); n++)
			if (SameName(aNames[n], theName))
				return aNames[n];
		return "";
	}
	std::string Fit(const std::string& theName) override { return bonnies::siteFit(theName); }
	bool Create(const std::string& theName, std::string* theWhy) override
	{
		bonnies::siteCreate(theName);
		return true;
	}
	bool Select(const std::string& theName) override
	{
		bonnies::siteSelect(theName);
		return true;
	}
	bool CanDelete() override { return true; }
	bool Delete(const std::string& theName, std::string* theWhy) override
	{
		bonnies::siteDelete(theName);
		return true;
	}
	const char* DeleteNote() override { return "the player's saved game: the books, levels and scores reached"; }
	const char* Rules() override { return "Letters, digits, spaces and underscores, up to 20 characters"; }
};

static void Start()
{
	if (sStarted)
		return;
	sStarted = true;
	BonniesPlayers aPlayers;
	Byopop::SiteStart("bonnies", aPlayers);
}

static void Changed()
{
	if (!sStarted)
		return;
	BonniesPlayers aPlayers;
	Byopop::SiteChanged("bonnies", aPlayers);
}

static std::string NameFor()
{
	return bonnies::siteFit(Byopop::SiteNameFor("bonnies"));
}

// The options' sound and music volumes (Options.cpp reads them at start, so the site's wait for the first menu)
static int Volumes(int theMusic, int theSfx)
{
	if (!sStarted)
		return 0;
	bonnies::siteVolumes(theMusic >= 0 ? theMusic / 100.0f : -1.0f, theSfx >= 0 ? theSfx / 100.0f : -1.0f);
	return 1;
}

static struct Register
{
	Register()
	{
		bonnies::gSiteStart = Start;
		bonnies::gSitePlayersChanged = Changed;
		bonnies::gSiteNameFor = NameFor;
		Byopop::gSiteVolume = Volumes;
	}
} sRegister;
#endif
