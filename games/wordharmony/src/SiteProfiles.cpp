// port: Word Harmony's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "Game.h"
#include "WebProfiles.h"
#include "WebSettings.h"

// HighScoreEntryLayout::keyDown: 16 characters, the keys 0x20 to 0x87
static const int kMaxChars = 16;

// The last name given (Settings::mDefaultScoreName, saved with the other settings), as the page's list of names
class Scores : public Byopop::SiteScoreNames
{
public:
	Scores(Game* theGame) :
		Byopop::SiteScoreNames(NULL, &theGame->mSettings.mDefaultScoreName, NULL, kMaxChars, -1,
			"Up to 16 characters; ASCII, case kept"),
		mGame(theGame)
	{
	}

	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		mGame->mSettings.setDefaultScoreName(theTo);
		mGame->mSettings.save();
		return true;
	}

private:
	Game*	mGame;
};

static bool sStarted = false;

void SiteStart(Game* theGame)
{
	Scores aScores(theGame);
	Byopop::SiteStart("wordharmony", aScores);
	sStarted = true;
}

// The site's volumes (framework/WebSettings.h): Word Harmony's sound and music are two category volumes of its sound system, which
// Game::applyVolumes sets to 1 or 0 from the options' two boxes; the site's level is what 1 means (SiteCategoryLevel), so
// the boxes still turn them off. After the game is configured, so that the game's own start comes first.
static float sLevels[2] = { 1.0f, 1.0f };

float SiteCategoryLevel(int theCategory)
{
	return ((theCategory >= 0) && (theCategory < 2)) ? sLevels[theCategory] : 1.0f;
}

static int Volumes(int theMusic, int theSfx)
{
	Game* aGame = Game::instance();
	if (!sStarted || (aGame == NULL))
		return 0;
	if (theSfx >= 0)
		sLevels[0] = theSfx / 100.0f;
	if (theMusic >= 0)
		sLevels[1] = theMusic / 100.0f;
	aGame->siteApplyVolumes();
	return 1;
}

static struct Register
{
	Register() { Byopop::gSiteVolume = Volumes; }
} sRegister;

std::string SiteScoreName()
{
	return Byopop::SiteFitTyped(Byopop::SiteNameFor("wordharmony"), NULL, kMaxChars, -1);
}
#endif
