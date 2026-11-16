#ifndef __FEEDINGFRENZY_GAMEMAPLAYOUT_H__
#define __FEEDINGFRENZY_GAMEMAPLAYOUT_H__

// The screen with the map of the stages (GameMapLayout, 0x16c bytes, constructor 0x423bd0). The
// controller keeps two: the map the player chooses a way on when a new game starts, and the one shown while a stage's
// files load (it shows the fun fact and how far the load has got).

#include "ScreenLayout.h"

namespace feedingfrenzy
{

class GameMapLayout : public engine::ScreenLayout
{
public:
	GameMapLayout();  // 0x423bd0

	// slot 0xfc: whether it is the map that is chosen on (true) or the one shown while loading (false)
	virtual void setIsMap(bool theIsMap);
	// slot 0x104: the map is made ready to show: the stages beaten, the one to go to, the fun fact
	virtual void prepare();
	// slot 0x108: the stage's files are loaded (true) or not
	virtual void setLoaded(bool theLoaded);
};

} // namespace feedingfrenzy

#endif
