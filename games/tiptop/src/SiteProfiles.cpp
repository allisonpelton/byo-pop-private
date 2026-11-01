// port: TipTop's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "Gui.h"
#include "Music.h"
#include "WebProfiles.h"
#include "WebSettings.h"

#include <cstring>
#include <string>

using namespace tiptop;

// The score's text field takes at most 10 characters (TextInputProc's creation: the longest text 0xa), the printable ones
static const int kMaxChars = 10;

static bool sStarted = false;
static HighScore sStartingScores[4][5];  // the configuration's tables, before Preferences.dat replaces them

static bool SameName(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++)
		if (tolower((unsigned char) a[i]) != tolower((unsigned char) b[i]))
			return false;
	return true;
}

// TipTop remembers no name, so the names it knows are the ones in its high score tables, but for the configuration's own entries
// and the empty entries' dashes
class TipTopScoreNames : public Byopop::SiteScoreNames
{
public:
	TipTopScoreNames() : Byopop::SiteScoreNames(NULL, NULL, NULL, kMaxChars, -1, "Up to 10 characters; ASCII, case kept") {}

	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		for (int aMode = 0; aMode < 4; aMode++)
			for (int i = 0; i < 5; i++)
			{
				const HighScore& aScore = gHighScores[aMode][i];
				std::string aName(aScore.mName, strnlen(aScore.mName, sizeof(aScore.mName)));
				if (aName.empty() || aName == GTC(0x0046c380) || IsStarting(aScore))
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

private:
	static bool IsStarting(const HighScore& theScore)
	{
		for (int aMode = 0; aMode < 4; aMode++)
			for (int i = 0; i < 5; i++)
			{
				const HighScore& aStart = sStartingScores[aMode][i];
				if ((aStart.mScore == theScore.mScore) && (aStart.mLevel == theScore.mLevel) &&
					(strncmp(aStart.mName, theScore.mName, sizeof(aStart.mName)) == 0))
					return true;
			}
		return false;
	}
};

void tiptop::SiteKeepStartingScores()
{
	memcpy(sStartingScores, gHighScores, sizeof(sStartingScores));
}

void tiptop::SiteStart()
{
	TipTopScoreNames aScores;
	Byopop::SiteStart("tiptop", aScores);
	sStarted = true;
}

void tiptop::SiteScoresChanged()
{
	if (!sStarted)
		return;
	TipTopScoreNames aScores;
	Byopop::SiteChanged("tiptop", aScores);
}

// The site's volumes (framework/WebSettings.h): the options' two sliders' variables, as the sliders set them (SliderProc of the
// sound: gSoundVolume, 0 to 0xffff; of the music: BuildMusicVolumes, 0 to 0x100). After GameInit, which reads Preferences.dat, so
// that the site's value wins over the saved one.
static int Volumes(int theMusic, int theSfx)
{
	if (!sStarted)
		return 0;
	if (theSfx >= 0)
		gSoundVolume = (uint16_t) ((theSfx * 0xffff + 50) / 100);
	if (theMusic >= 0)
		BuildMusicVolumes((uint32_t) ((theMusic * 0x100 + 50) / 100));
	return 1;
}

static struct Register
{
	Register() { Byopop::gSiteVolume = Volumes; }
} sRegister;

void tiptop::SiteFillScoreName(Handler* theField)
{
	std::string aName = Byopop::SiteFitTyped(Byopop::SiteNameFor("tiptop"), NULL, kMaxChars, -1);
	for (size_t i = 0; i < aName.size(); i++)
		Send(theField, MSG_CHAR, (unsigned char) aName[i], 0);
}
#endif
