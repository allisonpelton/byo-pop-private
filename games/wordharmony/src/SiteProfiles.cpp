// port: Word Harmony's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "Game.h"
#include "WebProfiles.h"

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

void SiteStart(Game* theGame)
{
	Scores aScores(theGame);
	Byopop::SiteStart("wordharmony", aScores);
}

std::string SiteScoreName()
{
	return Byopop::SiteFitTyped(Byopop::SiteNameFor("wordharmony"), NULL, kMaxChars, -1);
}
#endif
