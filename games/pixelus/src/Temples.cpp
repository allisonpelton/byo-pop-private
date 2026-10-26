#include "Temples.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "Res.h"

using namespace Sexy;

// 0x53d740 names, 0x53d378 pictures, 0x53d3a0 places, 0x53d3f0 glow centres, 0x53d718 thresholds, 0x543550 colours
const TempleInfo Sexy::gTemples[NUM_TEMPLES] =
{
	{ "Tutorial",   IMAGE_TEMPLETUTORIAL,   300, 277, 337, 317,   0, 132, 225, 127 },
	{ "Flora",      IMAGE_TEMPLEFLORA,      324, 339, 370, 400,   0, 132, 225, 127 },
	{ "Cupid",      IMAGE_TEMPLECUPID,      213, 178, 285, 238,   9, 255,  96,  96 },
	{ "Neptune",    IMAGE_TEMPLENEPTUNE,    518, 187, 593, 245,  20,  32, 128, 255 },
	{ "Venus",      IMAGE_TEMPLEVENUS,      524, 307, 577, 373,  34, 255,  97, 199 },
	{ "Bacchus",    IMAGE_TEMPLEBACCHUS,    353, 154, 403, 199,  50, 255, 128,  32 },
	{ "Vulcan",     IMAGE_TEMPLEVULCAN,     477,  82, 512, 126,  72, 255,  32, 255 },
	{ "Diana",      IMAGE_TEMPLEDIANE,      377, 227, 430, 292, 100,  64, 255,  64 },
	{ "Proserpina", IMAGE_TEMPLEPROSERPINE, 138,  69, 183, 141, 128,  64,  64, 255 },
	{ "Jupiter",    IMAGE_TEMPLEJUPITER,    267,  46, 363, 117, 160, 255, 255, 255 }
};

// the puzzles solved outside the tutorial
static int SolvedOutsideTutorial(PixelusApp* theApp)
{
	UserProfile* aUser = theApp->mUserDirectory->GetCurrent();
	if (aUser == NULL)
		return 0;
	return aUser->CountSolved() - aUser->CountSolved(gTemples[0].mName);
}

// 0x433640
bool Sexy::IsTempleOpen(PixelusApp* theApp, int theTemple)
{
	UserProfile* aUser = theApp->mUserDirectory->GetCurrent();
	if (theTemple == 1 && (aUser == NULL || !aUser->mFlag79))
		return false;
	return SolvedOutsideTutorial(theApp) >= gTemples[theTemple].mUnlock;
}

// 0x433570
bool Sexy::IsTempleLocked(PixelusApp* theApp, int theTemple)
{
	return SolvedOutsideTutorial(theApp) < gTemples[theTemple].mUnlock;
}
