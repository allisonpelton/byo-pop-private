// port: Ning Po's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "NingPo.h"
#include "GameText.h"
#include "WebProfiles.h"
#include "WebSettings.h"

#include <cctype>
#include <cstring>

using namespace ningpo;

// NingPo::enterName's box: setMaxLength(0xc), and the characters from 0x20 up (PGDialogText::handleEvent)
static const int kMaxChars = 12;

static bool sStarted = false, sReady = false, sMusicSet = false;

static bool SameName(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++)
		if (tolower((unsigned char) a[i]) != tolower((unsigned char) b[i]))
			return false;
	return true;
}

// Ning Po remembers no name, so the names it knows are the ones in its best times (all 18 layouts), but for the starting
// entries: the default names and the cleared ones (resetBestTimes, loadBestTimes) all have 144 tiles left at 99:59
class NingPoScoreNames : public Byopop::SiteScoreNames
{
public:
	NingPoScoreNames() : Byopop::SiteScoreNames(NULL, NULL, NULL, kMaxChars, -1, "Up to 12 characters; ASCII, case kept") {}

	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		NingPo* anApp = GetNingPo();
		if (anApp == NULL)
			return aNames;
		for (int aLayout = 0; aLayout < NUM_LAYOUTS; aLayout++)
			for (int aRank = 0; aRank < NUM_BEST_TIMES; aRank++)
			{
				const BestTime& aTime = anApp->mBestTimes[aLayout][aRank];
				if ((aTime.mTilesLeft == 0x90) && (aTime.mTime == 5999) && (aTime.mRawTime == 5999))
					continue;
				std::string aName(aTime.mName, strnlen(aTime.mName, sizeof(aTime.mName)));
				if (aName.empty() || (aName == GT(0x467a24)))
					continue;
				bool isNew = true;
				for (size_t n = 0; n < aNames.size(); n++)
					if (SameName(aNames[n], aName))
						isNew = false;
				if (isNew)
					aNames.push_back(aName);
			}
		return aNames;
	}

	virtual std::string Find(const std::string& theName)
	{
		std::vector<std::string> aNames = Names();
		for (size_t n = 0; n < aNames.size(); n++)
			if (SameName(aNames[n], theName))
				return aNames[n];
		return "";
	}
};

void ningpo::SiteStart()
{
	NingPoScoreNames aScores;
	Byopop::SiteStart("ningpo", aScores);
	sStarted = true;
}

void ningpo::SiteScoresChanged()
{
	if (!sStarted)
		return;
	NingPoScoreNames aScores;
	Byopop::SiteChanged("ningpo", aScores);
}

// The site's volumes (framework/WebSettings.h): the options' two settings, 0 to 1, as their sliders set them (SoundSlider and
// MusicSlider in NingPoDialogs.cpp: the music one also sets the playing music's volume). After loadSettings, which has read them
// from the registry, so that the site's value wins over the saved one; the playing music takes it once init() is done.
static int Volumes(int theMusic, int theSfx)
{
	NingPo* anApp = GetNingPo();
	if (!sStarted || (anApp == NULL))
		return 0;
	if (theSfx >= 0)
		anApp->mSoundSetting = theSfx / 100.0f;
	if (theMusic >= 0)
	{
		anApp->mMusicSetting = theMusic / 100.0f;
		sMusicSet = true;
		if (sReady)
			anApp->setMusicVolume(anApp->mMusicSetting);
	}
	return 1;
}

void ningpo::SiteReady()
{
	sReady = true;
	// only what the site set: with no site volume the game's music keeps the volume it has
	NingPo* anApp = GetNingPo();
	if (sMusicSet && (anApp != NULL))
		anApp->setMusicVolume(anApp->mMusicSetting);
}

static struct Register
{
	Register() { Byopop::gSiteVolume = Volumes; }
} sRegister;

std::string ningpo::SiteScoreName()
{
	return Byopop::SiteFitTyped(Byopop::SiteNameFor("ningpo"), NULL, kMaxChars, -1);
}
#endif
