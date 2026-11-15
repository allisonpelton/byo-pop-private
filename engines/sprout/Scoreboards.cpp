#include "Scoreboards.h"

using namespace engine;

namespace
{
Scoreboards* gScoreboards = nullptr;  // 0x532f68 (pizzafrenzy)
}

// 0x4691e0 (pizzafrenzy)
Scoreboards* Scoreboards::instance()
{
	if (gScoreboards == nullptr)
		gScoreboards = new Scoreboards();
	return gScoreboards;
}

// 0x469250 (pizzafrenzy): the key is opened by the application's slot 0x98 (0x4bccf0 (pizzafrenzy)); the port's
// registry is the host's (see PersistKey.h)
bool Scoreboards::open(const std::string& thePublisher, const std::string& theGame)
{
	std::string aPath = thePublisher + "\\" + theGame + "\\scoreboards";
	mKey = new PersistKey(aPath, "scoreboards\\");
	return true;
}
