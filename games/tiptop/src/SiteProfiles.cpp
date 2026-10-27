// port: TipTop's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "Gui.h"
#include "WebProfiles.h"

#include <string>

using namespace tiptop;

// The score's text field takes at most 10 characters (TextInputProc's creation: the longest text 0xa), the printable ones
static const int kMaxChars = 10;

void tiptop::SiteStart()
{
	Byopop::SiteScoreNames aScores(NULL, NULL, NULL, kMaxChars, -1, "Up to 10 characters; ASCII, case kept");
	Byopop::SiteStart("tiptop", aScores);
}

void tiptop::SiteFillScoreName(Handler* theField)
{
	std::string aName = Byopop::SiteFitTyped(Byopop::SiteNameFor("tiptop"), NULL, kMaxChars, -1);
	for (size_t i = 0; i < aName.size(); i++)
		Send(theField, MSG_CHAR, (unsigned char) aName[i], 0);
}
#endif
