// port: Ning Po's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "WebProfiles.h"

using namespace ningpo;

// NingPo::enterName's box: setMaxLength(0xc), and the characters from 0x20 up (PGDialogText::handleEvent)
static const int kMaxChars = 12;

void ningpo::SiteStart()
{
	Byopop::SiteScoreNames aScores(NULL, NULL, NULL, kMaxChars, -1, "Up to 12 characters; ASCII, case kept");
	Byopop::SiteStart("ningpo", aScores);
}

std::string ningpo::SiteScoreName()
{
	return Byopop::SiteFitTyped(Byopop::SiteNameFor("ningpo"), NULL, kMaxChars, -1);
}
#endif
